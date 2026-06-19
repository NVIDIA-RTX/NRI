// © 2026 NVIDIA Corporation

QueryPoolWGPU::~QueryPoolWGPU() {
    if (m_QuerySet)
        wgpuQuerySetRelease(m_QuerySet);
}

Result QueryPoolWGPU::Create(const QueryPoolDesc& queryPoolDesc) {
    m_Desc = queryPoolDesc;

    WGPUQuerySetDescriptor desc = WGPU_QUERY_SET_DESCRIPTOR_INIT;
    desc.count = queryPoolDesc.capacity;

    switch (queryPoolDesc.queryType) {
        case QueryType::TIMESTAMP:
            if (!m_Device.GetDesc().features.timestamp)
                return Result::UNSUPPORTED;
            desc.type = WGPUQueryType_Timestamp;
            m_QuerySize = sizeof(uint64_t);
            break;
        case QueryType::OCCLUSION:
            if (!m_Device.GetDesc().features.occlusion)
                return Result::UNSUPPORTED;
            desc.type = WGPUQueryType_Occlusion;
            m_QuerySize = sizeof(uint64_t);
            break;
        default:
            return Result::UNSUPPORTED;
    }

    m_QuerySet = wgpuDeviceCreateQuerySet(m_Device, &desc);
    return m_QuerySet ? Result::SUCCESS : Result::FAILURE;
}

void QueryPoolWGPU::Reset(uint32_t offset, uint32_t num) {
    MaybeUnused(offset, num);
}
