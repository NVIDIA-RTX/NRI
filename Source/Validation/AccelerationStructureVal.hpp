// © 2021 NVIDIA Corporation

AccelerationStructureVal::~AccelerationStructureVal() {
    if (m_Memory)
        m_Memory->Unbind(*this);

    Destroy(m_Buffer);
}

NRI_INLINE uint64_t AccelerationStructureVal::GetUpdateScratchBufferSize() const {
    return GetRayTracingInterfaceImpl().GetAccelerationStructureUpdateScratchBufferSize(*GetImpl());
}

NRI_INLINE uint64_t AccelerationStructureVal::GetBuildScratchBufferSize() const {
    return GetRayTracingInterfaceImpl().GetAccelerationStructureBuildScratchBufferSize(*GetImpl());
}

NRI_INLINE uint64_t AccelerationStructureVal::GetHandle() const {
    RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "AccelerationStructure is not bound to memory");

    return GetRayTracingInterfaceImpl().GetAccelerationStructureHandle(*GetImpl());
}

NRI_INLINE uint64_t AccelerationStructureVal::GetNativeObject() const {
    RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "AccelerationStructure is not bound to memory");

    return GetRayTracingInterfaceImpl().GetAccelerationStructureNativeObject(GetImpl());
}

NRI_INLINE Buffer* AccelerationStructureVal::GetBuffer() {
    RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), 0, "AccelerationStructure is not bound to memory");

    if (!m_Buffer) {
        Buffer* buffer = GetRayTracingInterfaceImpl().GetAccelerationStructureBuffer(*GetImpl());
        m_Buffer = Allocate<BufferVal>(m_Device.GetAllocationCallbacks(), m_Device, buffer, false);
    }

    return (Buffer*)m_Buffer;
}

NRI_INLINE Result AccelerationStructureVal::CreateDescriptor(Descriptor*& descriptor) {
    RETURN_ON_FAILURE(&m_Device, IsBoundToMemory(), Result::INVALID_ARGUMENT, "AccelerationStructure is not bound to memory");

    Descriptor* descriptorImpl = nullptr;
    const Result result = GetRayTracingInterfaceImpl().CreateAccelerationStructureDescriptor(*GetImpl(), descriptorImpl);

    descriptor = nullptr;
    if (result == Result::SUCCESS)
        descriptor = (Descriptor*)Allocate<DescriptorVal>(m_Device.GetAllocationCallbacks(), m_Device, descriptorImpl, ResourceType::ACCELERATION_STRUCTURE);

    return result;
}
