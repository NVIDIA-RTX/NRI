// © 2026 NVIDIA Corporation

Result PipelineCacheD3D12::Create(const PipelineCacheDesc& pipelineCacheDesc) {
    // "ID3D12Device1::CreatePipelineLibrary" requires the source blob to remain valid for the library's lifetime,
    // so we own a copy here instead of trusting the caller to keep their buffer alive across all subsequent
    // pipeline creates. (Vulkan's "vkCreatePipelineCache" copies internally - this matches that behavior.)
    if (pipelineCacheDesc.data && pipelineCacheDesc.size > 0) {
        m_Blob.resize((size_t)pipelineCacheDesc.size);
        memcpy(m_Blob.data(), pipelineCacheDesc.data, (size_t)pipelineCacheDesc.size);
    }

    HRESULT hr = m_Device->CreatePipelineLibrary(m_Blob.data(), m_Blob.size(), IID_PPV_ARGS(&m_Library));
    NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device1::CreatePipelineLibrary");

    return Result::SUCCESS;
}

Result PipelineCacheD3D12::GetData(void* dst, uint64_t& size) const {
    if (!m_Library) {
        size = 0;
        return Result::SUCCESS;
    }

    if (!dst) {
        size = (uint64_t)m_Library->GetSerializedSize();
        return Result::SUCCESS;
    }

    HRESULT hr = m_Library->Serialize(dst, (SIZE_T)size);
    NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12PipelineLibrary::Serialize");

    size = (uint64_t)m_Library->GetSerializedSize();
    return Result::SUCCESS;
}
