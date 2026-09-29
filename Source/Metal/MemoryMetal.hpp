// © 2026 NVIDIA Corporation

MemoryMetal::~MemoryMetal() {
    if (m_Heap) {
        m_Device.RemoveResidency(m_Heap);
        m_Heap->release();
    }
}

Result MemoryMetal::Create(const AllocateMemoryDesc& desc) {
    m_Location = (MemoryLocation)desc.type;

    // Metal heaps are already explicit suballocators. VMA suballocation and priority have no Metal equivalents.
    MTL::HeapDescriptor* heapDesc = MTL::HeapDescriptor::alloc()->init();
    heapDesc->setType(MTL::HeapTypePlacement);
    heapDesc->setSize((NS::UInteger)desc.size);
    heapDesc->setStorageMode(m_Location == MemoryLocation::DEVICE ? MTL::StorageModePrivate : MTL::StorageModeShared);
    heapDesc->setCpuCacheMode(m_Location == MemoryLocation::DEVICE || m_Location == MemoryLocation::HOST_READBACK ? MTL::CPUCacheModeDefaultCache : MTL::CPUCacheModeWriteCombined);
    heapDesc->setHazardTrackingMode(MTL::HazardTrackingModeUntracked);
    m_Heap = m_Device.GetNativeObject()->newHeap(heapDesc);
    heapDesc->release();

    if (!m_Heap)
        return Result::OUT_OF_MEMORY;

    m_Device.AddResidency(m_Heap);

    return Result::SUCCESS;
}

void MemoryMetal::SetDebugName(const char* name) {
    if (!m_Heap)
        return;

    NS::String* label = NS::String::alloc()->init(name, NS::UTF8StringEncoding);
    m_Heap->setLabel(label);
    label->release();
}
