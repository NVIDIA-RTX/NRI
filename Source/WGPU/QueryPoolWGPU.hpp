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
            // TODO: WebGPU supports occlusion query sets, but WGPU render-pass creation does not pass "occlusionQuerySet" yet.
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
    // TODO: WebGPU has no direct query reset command. Keep this a no-op unless a backend-side query lifetime workaround is added.
    MaybeUnused(offset, num);
}
