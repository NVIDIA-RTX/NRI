// © 2021 NVIDIA Corporation

QueryPoolVK::~QueryPoolVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.DestroyQueryPool(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }
}

Result QueryPoolVK::Create(const QueryPoolDesc& queryPoolDesc) {
    if (queryPoolDesc.queryType == QueryType::TIMESTAMP || queryPoolDesc.queryType == QueryType::TIMESTAMP_COPY_QUEUE)
        m_Type = VK_QUERY_TYPE_TIMESTAMP;
    else if (queryPoolDesc.queryType == QueryType::OCCLUSION)
        m_Type = VK_QUERY_TYPE_OCCLUSION;
    else if (queryPoolDesc.queryType == QueryType::PIPELINE_STATISTICS)
        m_Type = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    else if (queryPoolDesc.queryType == QueryType::ACCELERATION_STRUCTURE_SIZE)
        m_Type = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR;
    else if (queryPoolDesc.queryType == QueryType::ACCELERATION_STRUCTURE_COMPACTED_SIZE)
        m_Type = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
    else if (queryPoolDesc.queryType == QueryType::MICROMAP_COMPACTED_SIZE)
        m_Type = VK_QUERY_TYPE_MICROMAP_COMPACTED_SIZE_EXT;
    else
        return Result::INVALID_ARGUMENT;

    VkQueryPipelineStatisticFlags pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT
        | VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT
        | VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT
        | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT
        | VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT
        | VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

    if (m_Device.GetDesc().features.meshShader)
        pipelineStatistics |= VK_QUERY_PIPELINE_STATISTIC_TASK_SHADER_INVOCATIONS_BIT_EXT | VK_QUERY_PIPELINE_STATISTIC_MESH_SHADER_INVOCATIONS_BIT_EXT;

    VkQueryPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, nullptr, (VkQueryPoolCreateFlags)0, m_Type, queryPoolDesc.capacity, pipelineStatistics};

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateQueryPool(m_Device, &poolInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateQueryPool");

    m_QuerySize = (m_Type == VK_QUERY_TYPE_PIPELINE_STATISTICS ? (m_Device.GetDesc().features.meshShader ? 13 : 11) : 1) * sizeof(uint64_t);

    return Result::SUCCESS;
}

Result QueryPoolVK::Create(const QueryPoolVKDesc& queryPoolVKDesc) {
    m_OwnsNativeObjects = false;
    m_Type = (VkQueryType)queryPoolVKDesc.vkQueryType;
    m_Handle = (VkQueryPool)queryPoolVKDesc.vkQueryPool;
    m_QuerySize = (m_Type == VK_QUERY_TYPE_PIPELINE_STATISTICS ? (m_Device.GetDesc().features.meshShader ? 13 : 11) : 1) * sizeof(uint64_t);

    return Result::SUCCESS;
}

NRI_INLINE void QueryPoolVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_QUERY_POOL, (uint64_t)m_Handle, name);
}

NRI_INLINE void QueryPoolVK::Reset(uint32_t offset, uint32_t num) {
    const auto& vk = m_Device.GetDispatchTable();
    vk.ResetQueryPool(m_Device, m_Handle, offset, num);
}
