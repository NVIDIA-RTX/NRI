// © 2026 NVIDIA Corporation

QueueMetal::~QueueMetal() {
    // Feedback handlers of in-flight commits access this queue
    if (m_PendingCommits) {
        dispatch_group_wait(m_PendingCommits, DISPATCH_TIME_FOREVER);
        dispatch_release(m_PendingCommits);
    }

    NS::Error* error = m_Error.load();

    if (error)
        error->release();

    if (m_TransferBuffer) {
        m_Device.RemoveResidency(m_TransferBuffer);
        m_TransferBuffer->release();
    }

    if (m_TransferCommandBuffer)
        m_TransferCommandBuffer->release();

    if (m_TransferAllocator)
        m_TransferAllocator->release();

    if (m_IdleEvent)
        m_IdleEvent->release();

    if (m_Queue) {
        m_Queue->removeResidencySet(m_Device.GetResidencySet());
        m_Queue->release();
    }
}

Result QueueMetal::Create() {
    m_Queue = m_Device.GetNativeObject()->newMTL4CommandQueue();

    return Finalize();
}

Result QueueMetal::Create(MTL4::CommandQueue* queue) {
    m_Queue = queue;
    m_Queue->retain();

    return Finalize();
}

Result QueueMetal::Finalize() {
    if (!m_Queue)
        return Result::FAILURE;

    // The device residency set covers all command buffers committed to the queue
    m_Queue->addResidencySet(m_Device.GetResidencySet());

    m_IdleEvent = m_Device.GetNativeObject()->newSharedEvent();
    m_PendingCommits = dispatch_group_create();

    if (!m_IdleEvent || !m_PendingCommits)
        return Result::OUT_OF_MEMORY;

    return Result::SUCCESS;
}

Result QueueMetal::Commit(const MTL4::CommandBuffer* const* commandBuffers, uint32_t commandBufferNum) {
    if (m_Error.load())
        return Result::DEVICE_LOST;

    if (!commandBufferNum)
        return Result::SUCCESS;

    // Commit and feedback handler registration can create autoreleased objects
    AutoreleasePoolMetal autoreleasePool;

    // "MTL4CommitOptions" can't be reused: a feedback handler is called only for the first commit
    MTL4::CommitOptions* options = MTL4::CommitOptions::alloc()->init();
    options->addFeedbackHandler([this](MTL4::CommitFeedback* result) {
        if (NS::Error* error = result->error()) {
            error->retain();
            NS::Error* expected = nullptr;

            if (!m_Error.compare_exchange_strong(expected, error))
                error->release();
        }

        m_FeedbackNum.fetch_add(1, std::memory_order_release);
        dispatch_group_leave(m_PendingCommits); // the last access to "this"
    });

    dispatch_group_enter(m_PendingCommits);
    m_Queue->commit(commandBuffers, commandBufferNum, options);
    options->release();

    m_CommitNum++;

    return Result::SUCCESS;
}

uint64_t QueueMetal::SignalIdle(uint64_t& commitNum) {
    commitNum = m_CommitNum;
    m_Queue->signalEvent(m_IdleEvent, ++m_IdleValue);

    return m_IdleValue;
}

Result QueueMetal::Submit(const QueueSubmitDesc& queueSubmitDesc) {
    Scratch<const MTL4::CommandBuffer*> commandBuffers = NRI_ALLOCATE_SCRATCH(m_Device, const MTL4::CommandBuffer*, queueSubmitDesc.commandBufferNum);

    for (uint32_t i = 0; i < queueSubmitDesc.commandBufferNum; i++)
        commandBuffers[i] = ((CommandBufferMetal*)queueSubmitDesc.commandBuffers[i])->GetNativeObject();

    m_Device.CommitResidency();

    std::lock_guard<std::mutex> lock(m_Lock);

    for (uint32_t i = 0; i < queueSubmitDesc.waitFenceNum; i++) {
        const FenceSubmitDesc& fenceSubmitDesc = queueSubmitDesc.waitFences[i];
        FenceMetal& fence = *(FenceMetal*)fenceSubmitDesc.fence;
        uint64_t value = fence.IsSwapChainSemaphore() ? fence.GetScheduledValue() : fenceSubmitDesc.value;

        m_Queue->wait(fence.GetNativeObject(), value);
    }

    Result result = Commit(commandBuffers, queueSubmitDesc.commandBufferNum);

    if (result != Result::SUCCESS)
        return result;

    for (uint32_t i = 0; i < queueSubmitDesc.signalFenceNum; i++) {
        const FenceSubmitDesc& fenceSubmitDesc = queueSubmitDesc.signalFences[i];
        FenceMetal& fence = *(FenceMetal*)fenceSubmitDesc.fence;
        uint64_t value = fence.IsSwapChainSemaphore() ? fence.NextSignalValue() : fenceSubmitDesc.value;

        m_Queue->signalEvent(fence.GetNativeObject(), value);
    }

    return Result::SUCCESS;
}

void QueueMetal::WaitForDrawable(CA::MetalDrawable* drawable, FenceMetal& acquireFence) {
    std::lock_guard<std::mutex> lock(m_Lock);

    m_Queue->wait(drawable);
    m_Queue->signalEvent(acquireFence.GetNativeObject(), acquireFence.NextSignalValue());
}

void QueueMetal::SignalDrawable(CA::MetalDrawable* drawable, FenceMetal& releaseFence) {
    std::lock_guard<std::mutex> lock(m_Lock);

    m_Queue->wait(releaseFence.GetNativeObject(), releaseFence.GetScheduledValue());
    m_Queue->signalDrawable(drawable);
}

void QueueMetal::GetCalibratedTimestamps(uint64_t& timestampGPU, uint64_t& timestampCPU) {
    // "sampleTimestamps" returns nanoseconds for both. GPU: converted to counter heap timestamp ticks ("timestampFrequencyHz").
    // CPU: nanoseconds in the "CLOCK_UPTIME_RAW" domain ("mach_absolute_time" converted to nanoseconds)
    MTL::Timestamp cpu = 0;
    MTL::Timestamp gpu = 0;
    m_Device.GetNativeObject()->sampleTimestamps(&cpu, &gpu);

    const uint64_t frequency = m_Device.GetDesc().other.timestampFrequencyHz;
    timestampGPU = uint64_t((unsigned __int128)gpu * frequency / 1000000000ull);
    timestampCPU = cpu;
}

Result QueueMetal::WaitForSignal(uint64_t value, uint64_t commitNum) {
    const bool isCompleted = m_IdleEvent->waitUntilSignaledValue(value, NRI_TIMEOUT_FENCE);

    // Commit errors are reported by feedback handlers, which are called asynchronously after completion
    if (isCompleted) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(NRI_TIMEOUT_FENCE);

        while (m_FeedbackNum.load(std::memory_order_acquire) < commitNum && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    if (NS::Error* error = m_Error.load()) {
        AutoreleasePoolMetal autoreleasePool;
        m_Device.ReportMessage(Message::ERROR, Result::DEVICE_LOST, __FILE__, __LINE__, "Metal submission failed: %s", error->localizedDescription()->utf8String());

        return Result::DEVICE_LOST;
    }

    return isCompleted ? Result::SUCCESS : Result::FAILURE;
}

Result QueueMetal::WaitIdle() {
    uint64_t value = 0;
    uint64_t commitNum = 0;

    {
        std::lock_guard<std::mutex> lock(m_Lock);
        value = SignalIdle(commitNum);
    }

    return WaitForSignal(value, commitNum);
}

struct HostTextureCopyLayoutMetal {
    uint64_t offset;
    uint32_t rowSize;
    uint32_t rowPitch;
    uint32_t rowNum;
    uint32_t slicePitch;
    uint32_t depth;
    MTL::Size size;
};

static HostTextureCopyLayoutMetal GetHostTextureCopyLayoutMetal(const TextureMetal& texture, const TextureRegionDesc& region, uint64_t& stagingSize) {
    const auto& alignment = texture.GetDevice().GetDesc().memoryAlignment;
    const TextureDesc& textureDesc = texture.GetDesc();
    const FormatProps& formatProps = GetFormatProps(textureDesc.format);
    uint32_t width = region.width == WHOLE_SIZE ? std::max(1u, uint32_t(textureDesc.width) >> region.mipOffset) : region.width;
    uint32_t height = region.height == WHOLE_SIZE ? std::max(1u, uint32_t(textureDesc.height) >> region.mipOffset) : region.height;
    uint32_t depth = region.depth == WHOLE_SIZE ? std::max(1u, uint32_t(textureDesc.depth) >> region.mipOffset) : region.depth;

    HostTextureCopyLayoutMetal layout = {};
    layout.offset = Align(stagingSize, (uint64_t)alignment.uploadBufferTextureSlice);
    layout.rowSize = ((width + formatProps.blockWidth - 1) / formatProps.blockWidth) * formatProps.stride;
    layout.rowPitch = Align(layout.rowSize, alignment.uploadBufferTextureRow);
    layout.rowNum = (height + formatProps.blockHeight - 1) / formatProps.blockHeight;
    layout.slicePitch = layout.rowPitch * layout.rowNum;
    layout.depth = depth;
    layout.size = MTL::Size(width, height, depth);
    stagingSize = layout.offset + uint64_t(layout.slicePitch) * depth;

    return layout;
}

Result QueueMetal::BeginTransfer(uint64_t stagingSize, MTL4::ComputeCommandEncoder*& encoder) {
    // The previous transfer may have timed out
    if (m_IdleEvent->signaledValue() < m_TransferValue && !m_IdleEvent->waitUntilSignaledValue(m_TransferValue, NRI_TIMEOUT_FENCE))
        return Result::FAILURE;

    if (!m_TransferAllocator)
        m_TransferAllocator = m_Device.GetNativeObject()->newCommandAllocator();

    if (!m_TransferCommandBuffer)
        m_TransferCommandBuffer = m_Device.GetNativeObject()->newCommandBuffer();

    if (!m_TransferAllocator || !m_TransferCommandBuffer)
        return Result::OUT_OF_MEMORY;

    if (!m_TransferBuffer || m_TransferBuffer->length() < stagingSize) {
        if (m_TransferBuffer) {
            m_Device.RemoveResidency(m_TransferBuffer);
            m_TransferBuffer->release();
        }

        m_TransferBuffer = m_Device.GetNativeObject()->newBuffer(stagingSize, MTL::ResourceStorageModeShared | MTL::ResourceHazardTrackingModeUntracked);

        if (!m_TransferBuffer)
            return Result::OUT_OF_MEMORY;

        m_Device.AddResidency(m_TransferBuffer);
    }

    m_Device.CommitResidency();

    // The previous transfer has completed
    m_TransferAllocator->reset();
    m_TransferCommandBuffer->beginCommandBuffer(m_TransferAllocator);
    encoder = m_TransferCommandBuffer->computeCommandEncoder();

    return Result::SUCCESS;
}

Result QueueMetal::EndTransfer() {
    m_TransferCommandBuffer->endCommandBuffer();

    const MTL4::CommandBuffer* commandBuffers[] = {m_TransferCommandBuffer};
    uint64_t commitNum = 0;
    Result result = Result::SUCCESS;

    {
        std::lock_guard<std::mutex> lock(m_Lock);
        result = Commit(commandBuffers, 1);

        if (result == Result::SUCCESS)
            m_TransferValue = SignalIdle(commitNum);
    }

    // Wait only for this submission (and prior work on the queue)
    if (result == Result::SUCCESS)
        result = WaitForSignal(m_TransferValue, commitNum);

    return result;
}

void QueueMetal::TrimTransfer() {
    // Don't keep huge staging buffers (unless still in use)
    if (m_TransferBuffer && m_TransferBuffer->length() > MAX_CACHED_HOST_COPY_RESOURCE_SIZE && m_IdleEvent->signaledValue() >= m_TransferValue) {
        m_Device.RemoveResidency(m_TransferBuffer);
        m_TransferBuffer->release();
        m_TransferBuffer = nullptr;
    }
}

Result QueueMetal::UploadHostMemoryToTexture(const UploadHostMemoryToTextureDesc* copyDescs, uint32_t copyDescNum) {
    if (!copyDescNum)
        return Result::SUCCESS;

    Scratch<HostTextureCopyLayoutMetal> layouts = NRI_ALLOCATE_SCRATCH(m_Device, HostTextureCopyLayoutMetal, copyDescNum);
    uint64_t stagingSize = 0;

    for (uint32_t i = 0; i < copyDescNum; i++)
        layouts[i] = GetHostTextureCopyLayoutMetal(*(TextureMetal*)copyDescs[i].dstTexture, copyDescs[i].dstRegion, stagingSize);

    // "computeCommandEncoder" returns an autoreleased object
    AutoreleasePoolMetal autoreleasePool;
    std::lock_guard<std::mutex> lock(m_TransferLock);

    // No waiting before: all prior GPU access to the copied subresources must be complete before the call
    MTL4::ComputeCommandEncoder* encoder = nullptr;
    Result result = BeginTransfer(stagingSize, encoder);

    if (result != Result::SUCCESS)
        return result;

    uint8_t* stagingData = (uint8_t*)m_TransferBuffer->contents();

    for (uint32_t i = 0; i < copyDescNum; i++) {
        const UploadHostMemoryToTextureDesc& copyDesc = copyDescs[i];
        const HostTextureCopyLayoutMetal& layout = layouts[i];
        uint32_t srcRowPitch = copyDesc.srcRowPitch ? copyDesc.srcRowPitch : layout.rowSize;
        uint32_t srcSlicePitch = copyDesc.srcSlicePitch ? copyDesc.srcSlicePitch : srcRowPitch * layout.rowNum;

        for (uint32_t z = 0; z < layout.depth; z++) {
            for (uint32_t y = 0; y < layout.rowNum; y++)
                memcpy(stagingData + layout.offset + uint64_t(z) * layout.slicePitch + uint64_t(y) * layout.rowPitch, (const uint8_t*)copyDesc.srcData + uint64_t(z) * srcSlicePitch + uint64_t(y) * srcRowPitch, layout.rowSize);
        }

        encoder->copyFromBuffer(m_TransferBuffer, layout.offset, layout.rowPitch, layout.slicePitch, layout.size, ((TextureMetal*)copyDesc.dstTexture)->GetNativeObject(), copyDesc.dstRegion.layerOffset, copyDesc.dstRegion.mipOffset, MTL::Origin(copyDesc.dstRegion.x, copyDesc.dstRegion.y, copyDesc.dstRegion.z));
    }

    encoder->endEncoding();

    result = EndTransfer();
    TrimTransfer();

    return result;
}

Result QueueMetal::ReadbackTextureToHostMemory(const ReadbackTextureToHostMemoryDesc* copyDescs, uint32_t copyDescNum) {
    if (!copyDescNum)
        return Result::SUCCESS;

    Scratch<HostTextureCopyLayoutMetal> layouts = NRI_ALLOCATE_SCRATCH(m_Device, HostTextureCopyLayoutMetal, copyDescNum);
    uint64_t stagingSize = 0;

    for (uint32_t i = 0; i < copyDescNum; i++)
        layouts[i] = GetHostTextureCopyLayoutMetal(*(TextureMetal*)copyDescs[i].srcTexture, copyDescs[i].srcRegion, stagingSize);

    // "computeCommandEncoder" returns an autoreleased object
    AutoreleasePoolMetal autoreleasePool;
    std::lock_guard<std::mutex> lock(m_TransferLock);

    // No waiting before: all prior GPU access to the copied subresources must be complete before the call
    MTL4::ComputeCommandEncoder* encoder = nullptr;
    Result result = BeginTransfer(stagingSize, encoder);

    if (result != Result::SUCCESS)
        return result;

    for (uint32_t i = 0; i < copyDescNum; i++) {
        const ReadbackTextureToHostMemoryDesc& copyDesc = copyDescs[i];
        const HostTextureCopyLayoutMetal& layout = layouts[i];
        encoder->copyFromTexture(((TextureMetal*)copyDesc.srcTexture)->GetNativeObject(), copyDesc.srcRegion.layerOffset, copyDesc.srcRegion.mipOffset, MTL::Origin(copyDesc.srcRegion.x, copyDesc.srcRegion.y, copyDesc.srcRegion.z), layout.size, m_TransferBuffer, layout.offset, layout.rowPitch, layout.slicePitch);
    }

    encoder->endEncoding();

    result = EndTransfer();

    if (result == Result::SUCCESS) {
        const uint8_t* stagingData = (const uint8_t*)m_TransferBuffer->contents();

        for (uint32_t i = 0; i < copyDescNum; i++) {
            const ReadbackTextureToHostMemoryDesc& copyDesc = copyDescs[i];
            const HostTextureCopyLayoutMetal& layout = layouts[i];
            uint32_t dstRowPitch = copyDesc.dstRowPitch ? copyDesc.dstRowPitch : layout.rowSize;
            uint32_t dstSlicePitch = copyDesc.dstSlicePitch ? copyDesc.dstSlicePitch : dstRowPitch * layout.rowNum;

            for (uint32_t z = 0; z < layout.depth; z++) {
                for (uint32_t y = 0; y < layout.rowNum; y++)
                    memcpy((uint8_t*)copyDesc.dstData + uint64_t(z) * dstSlicePitch + uint64_t(y) * dstRowPitch, stagingData + layout.offset + uint64_t(z) * layout.slicePitch + uint64_t(y) * layout.rowPitch, layout.rowSize);
            }
        }
    }

    TrimTransfer();

    return result;
}

void QueueMetal::SetDebugName(const char* name) {
    // "MTL4CommandQueue::label" is read-only, it can only be set via "MTL4CommandQueueDescriptor" at creation
    MaybeUnused(name);
}
