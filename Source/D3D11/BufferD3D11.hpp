// © 2021 NVIDIA Corporation

struct MultiThreadProtection {
    MultiThreadProtection(DeviceD3D11& device)
        : device(device) {
        device.EnterCriticalSection();
    }

    ~MultiThreadProtection() {
        device.LeaveCriticalSection();
    }

    DeviceD3D11& device;
};

BufferD3D11::~BufferD3D11() {
    Destroy(m_ReadbackTexture);
}

Result BufferD3D11::Allocate(MemoryLocation memoryLocation, float priority) {
    CHECK(!m_Buffer, "Unexpected");

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (uint32_t)m_Desc.size;

    if (m_Desc.structureStride) {
        if (m_Desc.structureStride == 4)
            // It's a hack and spec violation, but allows to create multiple views with different "structured" layouts for a single buffer
            desc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        else {
            desc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            desc.StructureByteStride = m_Desc.structureStride;
        }
    }

    if (m_Desc.usage & BufferUsageBits::ARGUMENT_BUFFER)
        desc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;

    if (memoryLocation == MemoryLocation::HOST_UPLOAD || memoryLocation == MemoryLocation::DEVICE_UPLOAD) {
        if (m_Desc.usage == BufferUsageBits::NONE) { // special case for "UploadBufferToTexture"
            desc.Usage = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE; // TODO: not the best solution, but currently needed for "UploadBufferToTexture"
        } else {
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        }
    } else if (memoryLocation == MemoryLocation::HOST_READBACK) {
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE; // TODO: not the best solution, but currently needed for "ReadbackTextureToBuffer" and queries
    } else {
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;
    }

    if (m_Desc.usage & BufferUsageBits::VERTEX_BUFFER)
        desc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;

    if (m_Desc.usage & BufferUsageBits::INDEX_BUFFER)
        desc.BindFlags |= D3D11_BIND_INDEX_BUFFER;

    if (m_Desc.usage & BufferUsageBits::CONSTANT_BUFFER)
        desc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;

    if (m_Desc.usage & BufferUsageBits::SHADER_RESOURCE)
        desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

    if (m_Desc.usage & BufferUsageBits::SHADER_RESOURCE_STORAGE)
        desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

    HRESULT hr = m_Device->CreateBuffer(&desc, nullptr, &m_Buffer);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateBuffer");

    // Priority
    uint32_t evictionPriority = ConvertPriority(priority);
    if (evictionPriority != 0)
        m_Buffer->SetEvictionPriority(evictionPriority);

    return Result::SUCCESS;
}

Result BufferD3D11::Create(const BufferDesc& bufferDesc) {
    m_Desc = bufferDesc;

    return Result::SUCCESS;
}

Result BufferD3D11::Create(const BufferD3D11Desc& bufferD3D11Desc) {
    if (bufferD3D11Desc.desc)
        m_Desc = *bufferD3D11Desc.desc;
    else if (!GetBufferDesc(bufferD3D11Desc, m_Desc))
        return Result::INVALID_ARGUMENT;

    m_Buffer = (ID3D11Buffer*)bufferD3D11Desc.d3d11Resource;

    return Result::SUCCESS;
}

TextureD3D11& BufferD3D11::RecreateReadbackTexture(const TextureD3D11& srcTexture, const TextureRegionDesc& srcRegion, const TextureDataLayoutDesc& readbackDataLayoutDesc) {
    bool isChanged = true;
    if (m_ReadbackTexture) {
        const TextureDesc& curr = m_ReadbackTexture->GetDesc();
        isChanged = curr.format != srcTexture.GetDesc().format || curr.width != srcRegion.width || curr.height != srcRegion.height || curr.depth != srcRegion.depth;
    }

    if (isChanged) {
        TextureDesc textureDesc = {};
        textureDesc.mipNum = 1;
        textureDesc.sampleNum = 1;
        textureDesc.layerNum = 1;
        textureDesc.format = srcTexture.GetDesc().format;
        textureDesc.width = srcRegion.width;
        textureDesc.height = srcRegion.height;
        textureDesc.depth = srcRegion.depth;

        textureDesc.type = TextureType::TEXTURE_2D;
        if (srcRegion.depth > 1)
            textureDesc.type = TextureType::TEXTURE_3D;
        else if (srcRegion.height == 1)
            textureDesc.type = TextureType::TEXTURE_1D;

        Destroy(m_ReadbackTexture);

        Result result = m_Device.CreateImplementation<TextureD3D11>(m_ReadbackTexture, textureDesc);
        if (result == Result::SUCCESS) {
            result = m_ReadbackTexture->Allocate(MemoryLocation::HOST_READBACK, 0.0f);
            if (result != Result::SUCCESS)
                Destroy(m_ReadbackTexture);
        }
    }

    m_IsReadbackDataChanged = true;
    m_ReadbackDataLayoutDesc = readbackDataLayoutDesc;

    return *m_ReadbackTexture;
}

NRI_INLINE void* BufferD3D11::Map(uint64_t offset) {
    MultiThreadProtection mutiThreadProtection(m_Device);

    // Map
    D3D11_BUFFER_DESC desc = {};
    m_Buffer->GetDesc(&desc);

    D3D11_MAP map = D3D11_MAP_WRITE;
    if (desc.CPUAccessFlags == D3D11_CPU_ACCESS_WRITE)
        map = desc.Usage == D3D11_USAGE_DYNAMIC ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE;
    else if (desc.CPUAccessFlags == D3D11_CPU_ACCESS_READ)
        map = D3D11_MAP_READ;
    else if (desc.CPUAccessFlags == (D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE))
        map = D3D11_MAP_READ_WRITE;
    else
        CHECK(false, "No CPU access");

    D3D11_MAPPED_SUBRESOURCE mappedData = {};
    HRESULT hr = m_Device.GetImmediateContext()->Map(m_Buffer, 0, map, 0, &mappedData);
    if (FAILED(hr)) {
        REPORT_ERROR(&m_Device, "ID3D11DeviceContext::Map() failed!");
        return nullptr;
    }

    uint8_t* ptr = (uint8_t*)mappedData.pData;

    // Finalize queries
    if (m_QueryRange.pool) {
        m_QueryRange.pool->GetData(ptr + m_QueryRange.bufferOffset, m_QueryRange.offset, m_QueryRange.num);
        m_QueryRange.pool = nullptr;
    }

    // Finalize readback
    if (m_IsReadbackDataChanged) {
        D3D11_MAPPED_SUBRESOURCE srcData = {};
        hr = m_Device.GetImmediateContext()->Map(*m_ReadbackTexture, 0, D3D11_MAP_READ, 0, &srcData);
        if (FAILED(hr)) {
            m_Device.GetImmediateContext()->Unmap(m_Buffer, 0);
            REPORT_ERROR(&m_Device, "ID3D11DeviceContext::Map() failed!");
            return nullptr;
        }

        const TextureDesc& readbackTextureDesc = m_ReadbackTexture->GetDesc();
        uint32_t d = readbackTextureDesc.depth;
        uint32_t h = readbackTextureDesc.height;
        uint32_t rowSize = readbackTextureDesc.width * GetFormatProps(readbackTextureDesc.format).stride;

        const uint8_t* src = (uint8_t*)srcData.pData;
        uint8_t* dst = ptr;
        for (uint32_t i = 0; i < d; i++) {
            for (uint32_t j = 0; j < h; j++) {
                const uint8_t* srcLocal = src + j * srcData.RowPitch;
                uint8_t* dstLocal = dst + j * m_ReadbackDataLayoutDesc.rowPitch;
                memcpy(dstLocal, srcLocal, rowSize);
            }
            src += srcData.DepthPitch;
            dst += m_ReadbackDataLayoutDesc.slicePitch;
        }

        m_Device.GetImmediateContext()->Unmap(*m_ReadbackTexture, 0);

        m_IsReadbackDataChanged = false;
    }

    return ptr + offset;
}

NRI_INLINE void BufferD3D11::Unmap() {
    MultiThreadProtection mutiThreadProtection(m_Device);

    m_Device.GetImmediateContext()->Unmap(m_Buffer, 0);
}
