// © 2026 NVIDIA Corporation

BufferWGPU::~BufferWGPU() {
    if (m_Buffer)
        wgpuBufferRelease(m_Buffer);
}

Result BufferWGPU::Create(const BufferDesc& bufferDesc) {
    return Create(bufferDesc, MemoryLocation::DEVICE);
}

Result BufferWGPU::Create(const BufferDesc& bufferDesc, MemoryLocation memoryLocation) {
    m_Desc = bufferDesc;
    m_MemoryLocation = memoryLocation;

    return CreateNativeBuffer();
}

Result BufferWGPU::CreateNativeBuffer() {
    uint64_t nativeSize = Align(std::max(m_Desc.size, 4ull), 4);

    WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
    desc.size = nativeSize;
    desc.usage = m_MemoryLocation == MemoryLocation::HOST_READBACK ? WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst : GetBufferUsage(m_Desc.usage);

    WGPUBuffer buffer = wgpuDeviceCreateBuffer(m_Device, &desc);
    if (!buffer)
        return Result::FAILURE;

    if (m_Buffer)
        wgpuBufferRelease(m_Buffer);

    m_Buffer = buffer;

    if (m_MemoryLocation != MemoryLocation::DEVICE && m_MemoryLocation != MemoryLocation::HOST_READBACK)
        m_CpuMemory.resize((size_t)nativeSize);
    else
        m_CpuMemory.clear();

    return Result::SUCCESS;
}

Result BufferWGPU::SetHostVisible(MemoryLocation memoryLocation) {
    if (m_MemoryLocation == memoryLocation)
        return Result::SUCCESS;

    MemoryLocation oldMemoryLocation = m_MemoryLocation;
    m_MemoryLocation = memoryLocation;

    if (m_Buffer && CreateNativeBuffer() != Result::SUCCESS) {
        m_MemoryLocation = oldMemoryLocation;
        return Result::FAILURE;
    }

    if (m_MemoryLocation != MemoryLocation::DEVICE && m_MemoryLocation != MemoryLocation::HOST_READBACK && m_CpuMemory.empty())
        m_CpuMemory.resize((size_t)Align(std::max(m_Desc.size, 4ull), 4));

    return Result::SUCCESS;
}

void* BufferWGPU::Map(uint64_t offset, uint64_t size) {
    m_MapOffset = offset;
    m_MapSize = size == WHOLE_SIZE ? m_Desc.size - offset : size;

    if (m_MemoryLocation == MemoryLocation::HOST_READBACK) {
        struct MapContext {
            bool completed;
            WGPUMapAsyncStatus status;
        } context = {};

        WGPUBufferMapCallbackInfo callbackInfo = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
        callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        callbackInfo.userdata1 = &context;
        callbackInfo.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void* userdata1, void*) {
            MapContext& context = *(MapContext*)userdata1;
            context.completed = true;
            context.status = status;
        };

        size_t mapOffset = (size_t)(m_MapOffset & ~7ull);
        size_t mapSize = (size_t)Align(m_MapOffset + m_MapSize - mapOffset, 4);
        wgpuBufferMapAsync(m_Buffer, WGPUMapMode_Read, mapOffset, mapSize, callbackInfo);

        // TODO: Readback map is blocking and pumps the device until completion.
        while (!context.completed) {
            wgpuDevicePoll(m_Device, WGPU_TRUE, nullptr);
            wgpuInstanceProcessEvents(m_Device.GetInstance());
        }

        if (context.status != WGPUMapAsyncStatus_Success) {
            m_MapOffset = 0;
            m_MapSize = 0;
            return nullptr;
        }

        m_MappedReadback = wgpuBufferGetConstMappedRange(m_Buffer, mapOffset, mapSize);
        return (uint8_t*)m_MappedReadback + (m_MapOffset - mapOffset);
    }

    if (m_CpuMemory.empty())
        m_CpuMemory.resize((size_t)Align(std::max(m_Desc.size, 4ull), 4));

    // TODO: Host-visible upload buffers are CPU-shadowed and flushed through "wgpuQueueWriteBuffer" on unmap.
    return m_CpuMemory.data() + offset;
}

void BufferWGPU::Unmap() {
    if (m_MemoryLocation == MemoryLocation::HOST_READBACK) {
        if (m_MappedReadback) {
            wgpuBufferUnmap(m_Buffer);
            m_MappedReadback = nullptr;
        }

        m_MapOffset = 0;
        m_MapSize = 0;
        return;
    }

    if (m_MapSize) {
        uint64_t writeOffset = m_MapOffset & ~3ull;
        uint64_t writeEnd = Align(m_MapOffset + m_MapSize, 4);

        if (writeEnd > m_MapOffset + m_MapSize)
            memset(m_CpuMemory.data() + m_MapOffset + m_MapSize, 0, (size_t)(writeEnd - (m_MapOffset + m_MapSize)));

        wgpuQueueWriteBuffer(m_Device.GetQueue(), m_Buffer, writeOffset, m_CpuMemory.data() + writeOffset, (size_t)(writeEnd - writeOffset));
    }

    m_MapOffset = 0;
    m_MapSize = 0;
}
