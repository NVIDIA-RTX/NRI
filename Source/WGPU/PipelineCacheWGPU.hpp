// © 2026 NVIDIA Corporation

Result PipelineCacheWGPU::Create(const PipelineCacheDesc& pipelineCacheDesc) {
    if (pipelineCacheDesc.data && pipelineCacheDesc.size) {
        m_Data.resize((size_t)pipelineCacheDesc.size);
        memcpy(m_Data.data(), pipelineCacheDesc.data, (size_t)pipelineCacheDesc.size);
    }

    return Result::SUCCESS;
}

Result PipelineCacheWGPU::GetData(void* dst, uint64_t& size) const {
    if (dst && !m_Data.empty())
        memcpy(dst, m_Data.data(), m_Data.size());

    size = m_Data.size();

    return Result::SUCCESS;
}
