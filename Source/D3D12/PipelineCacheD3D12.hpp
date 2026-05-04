// © 2026 NVIDIA Corporation

Result PipelineCacheD3D12::Create(const PipelineCacheDesc& pipelineCacheDesc) {
    if (!m_Device.GetDesc().features.pipelineCache)
        return Result::SUCCESS;

    // Copy initial data to mimic "vkCreatePipelineCache" behavior
    if (pipelineCacheDesc.data && pipelineCacheDesc.size) {
        m_Blob.resize((size_t)pipelineCacheDesc.size);
        memcpy(m_Blob.data(), pipelineCacheDesc.data, (size_t)pipelineCacheDesc.size);
    }

    HRESULT hr = m_Device->CreatePipelineLibrary(m_Blob.data(), m_Blob.size(), IID_PPV_ARGS(&m_Library));

    // If pipeline cache is supported it may fail only on stale/incompatible data, map to "OUT_OF_DATE"
    return FAILED(hr) ? Result::OUT_OF_DATE : Result::SUCCESS;
}

Result PipelineCacheD3D12::GetData(void* dst, uint64_t& size) const {
    if (!m_Library) {
        size = 0;
        return Result::SUCCESS;
    }

    if (dst) {
        HRESULT hr = m_Library->Serialize(dst, (SIZE_T)size);
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12PipelineLibrary::Serialize");
    }

    size = (uint64_t)m_Library->GetSerializedSize();

    return Result::SUCCESS;
}
