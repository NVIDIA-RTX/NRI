// © 2026 NVIDIA Corporation

PipelineCacheVK::~PipelineCacheVK() {
    const auto& vk = m_Device.GetDispatchTable();
    if (m_Handle != VK_NULL_HANDLE)
        vk.DestroyPipelineCache(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
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

    return size < info.initialDataSize ? Result::OUT_OF_DATE : Result::SUCCESS;
}

Result PipelineCacheVK::GetData(void* dst, uint64_t& size) const {
    if (!m_Handle) {
        size = 0;
        return Result::SUCCESS;
    }

    size_t vkSize = (size_t)size;
    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.GetPipelineCacheData(m_Device, m_Handle, &vkSize, dst);
    size = vkSize;

    // "dst" was smaller than the cache content (e.g., the cache grew between the size-query call and this call).
    // Vulkan still wrote a partial-but-valid blob and "size" reflects the bytes written. Pass it through as success. The partial blob
    // can still be fed back into "CreatePipelineCache". The caller can re-query if it needs the full content
    // https://registry.khronos.org/vulkan/specs/latest/man/html/vkGetPipelineCacheData.html
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetPipelineCacheData");

    return Result::SUCCESS;
}

NRI_INLINE void PipelineCacheVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_PIPELINE_CACHE, (uint64_t)m_Handle, name);
}
