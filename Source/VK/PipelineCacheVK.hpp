// © 2026 NVIDIA Corporation

PipelineCacheVK::~PipelineCacheVK() {
    if (m_Handle) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.DestroyPipelineCache(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }
}

Result PipelineCacheVK::Create(const PipelineCacheDesc& pipelineCacheDesc) {
    if (!m_Device.GetDesc().features.pipelineCache)
        return Result::SUCCESS;

    VkPipelineCacheCreateInfo info = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    info.initialDataSize = (size_t)pipelineCacheDesc.size;
    info.pInitialData = pipelineCacheDesc.data;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreatePipelineCache(m_Device, &info, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreatePipelineCache");

    size_t size = 0;
    vkResult = vk.GetPipelineCacheData(m_Device, m_Handle, &size, nullptr);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetPipelineCacheData");

    // VK returns "SUCCESS" for any variant of stale/incompatible data, try to guess...
    return size < info.initialDataSize ? Result::OUT_OF_DATE : Result::SUCCESS;
}

Result PipelineCacheVK::GetData(void* dst, uint64_t& size) const {
    if (!m_Handle) {
        size = 0;
        return Result::SUCCESS;
    }

    // Theoretically may be smaller than needed to fit the entire cache...
    size_t vkSize = (size_t)size;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.GetPipelineCacheData(m_Device, m_Handle, &vkSize, dst);

    // ...and even if "VK_INCOMPLETE" is returned, we consider it a "SUCCESS"
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetPipelineCacheData");

    size = vkSize;

    return Result::SUCCESS;
}

NRI_INLINE void PipelineCacheVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_PIPELINE_CACHE, (uint64_t)m_Handle, name);
}
