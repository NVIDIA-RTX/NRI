// © 2026 NVIDIA Corporation

Result QueryPoolWGPU::Create(const QueryPoolDesc& queryPoolDesc) {
    m_Desc = queryPoolDesc;

    switch (queryPoolDesc.queryType) {
        case QueryType::TIMESTAMP:
        case QueryType::TIMESTAMP_COPY_QUEUE:
        case QueryType::OCCLUSION:
            m_QuerySize = sizeof(uint64_t);
            break;
        case QueryType::PIPELINE_STATISTICS:
            m_QuerySize = sizeof(PipelineStatisticsDesc);
            break;
        default:
            m_QuerySize = sizeof(uint64_t);
            break;
    }

    return Result::SUCCESS;
}
