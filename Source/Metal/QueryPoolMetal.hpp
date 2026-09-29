// © 2026 NVIDIA Corporation

QueryPoolMetal::~QueryPoolMetal() {
    if (m_CounterHeap)
        m_CounterHeap->release();

    if (m_VisibilityBuffer) {
        m_Device.RemoveResidency(m_VisibilityBuffer);
        m_VisibilityBuffer->release();
    }
}

Result QueryPoolMetal::Create(const QueryPoolDesc& desc) {
    m_Type = desc.queryType;

    if (m_Type == QueryType::TIMESTAMP || m_Type == QueryType::TIMESTAMP_COPY_QUEUE) {
        MTL4::CounterHeapDescriptor* heapDesc = MTL4::CounterHeapDescriptor::alloc()->init();
        heapDesc->setType(MTL4::CounterHeapTypeTimestamp);
        heapDesc->setCount(desc.capacity);

        AutoreleasePoolMetal autoreleasePool; // for "error"
        NS::Error* error = nullptr;
        m_CounterHeap = m_Device.GetNativeObject()->newCounterHeap(heapDesc, &error);
        heapDesc->release();

        // Entries are resolved as-is into the destination buffer
        m_QuerySize = (uint32_t)m_Device.GetNativeObject()->sizeOfCounterHeapEntry(MTL4::CounterHeapTypeTimestamp);

        return m_CounterHeap ? Result::SUCCESS : Result::UNSUPPORTED;
    }

    if (m_Type == QueryType::OCCLUSION || m_Type == QueryType::ACCELERATION_STRUCTURE_SIZE || m_Type == QueryType::ACCELERATION_STRUCTURE_COMPACTED_SIZE) {
        // Occlusion results accumulate, host visible memory allows "ResetQueries"
        const MTL::ResourceOptions storageMode = m_Type == QueryType::OCCLUSION ? MTL::ResourceStorageModeShared : MTL::ResourceStorageModePrivate;

        m_VisibilityBuffer = m_Device.GetNativeObject()->newBuffer(uint64_t(desc.capacity) * sizeof(uint64_t), storageMode | MTL::ResourceHazardTrackingModeUntracked);

        if (!m_VisibilityBuffer)
            return Result::OUT_OF_MEMORY;

        m_Device.AddResidency(m_VisibilityBuffer);

        return Result::SUCCESS;
    }

    return Result::UNSUPPORTED;
}

void QueryPoolMetal::Reset(uint32_t offset, uint32_t num) {
    if (m_CounterHeap)
        m_CounterHeap->invalidateCounterRange(NS::Range(offset, num));
    else if (m_Type == QueryType::OCCLUSION)
        memset((uint8_t*)m_VisibilityBuffer->contents() + uint64_t(offset) * sizeof(uint64_t), 0, uint64_t(num) * sizeof(uint64_t));
}

void QueryPoolMetal::SetDebugName(const char* name) {
    NS::String* label = NS::String::alloc()->init(name, NS::UTF8StringEncoding);

    if (m_CounterHeap)
        m_CounterHeap->setLabel(label);

    if (m_VisibilityBuffer)
        m_VisibilityBuffer->setLabel(label);

    label->release();
}
