// © 2021 NVIDIA Corporation

Result QueryPoolD3D11::Create(const QueryPoolDesc& queryPoolDesc) {
    D3D11_QUERY_DESC queryDesc = {};
    if (queryPoolDesc.queryType == QueryType::TIMESTAMP || queryPoolDesc.queryType == QueryType::TIMESTAMP_COPY_QUEUE)
        queryDesc.Query = D3D11_QUERY_TIMESTAMP;
    else if (queryPoolDesc.queryType == QueryType::OCCLUSION)
        queryDesc.Query = D3D11_QUERY_OCCLUSION;
    else if (queryPoolDesc.queryType == QueryType::PIPELINE_STATISTICS)
        queryDesc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
    else
        return queryPoolDesc.queryType < QueryType::MAX_NUM ? Result::UNSUPPORTED : Result::INVALID_ARGUMENT;

    m_Type = queryPoolDesc.queryType;

    m_QueryPool.reserve(queryPoolDesc.capacity);
    for (uint32_t i = 0; i < queryPoolDesc.capacity; i++) {
        ComPtr<ID3D11Query> query = nullptr;
        HRESULT hr = m_Device->CreateQuery(&queryDesc, &query);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateQuery");

        m_QueryPool.push_back(query);
    }

    return Result::SUCCESS;
}

void QueryPoolD3D11::BeginQuery(ID3D11DeviceContextBest* deferredContext, uint32_t offset) {
    ID3D11Query* query = m_QueryPool[offset];
    deferredContext->Begin(query);
}

void QueryPoolD3D11::EndQuery(ID3D11DeviceContextBest* deferredContext, uint32_t offset) {
    ID3D11Query* query = m_QueryPool[offset];
    deferredContext->End(query);
}

void QueryPoolD3D11::GetData(uint8_t* dstMemory, uint32_t offset, uint32_t num) const {
    uint32_t querySize = GetQuerySize();

    for (uint32_t i = 0; i < num; i++) {
        ID3D11Query* query = m_QueryPool[offset + i];

        while (m_Device.GetImmediateContext()->GetData(query, dstMemory, querySize, 0) == S_FALSE)
            ;

        dstMemory += querySize;
    }
}

NRI_INLINE void QueryPoolD3D11::SetDebugName(const char* name) {
    for (ComPtr<ID3D11Query>& query : m_QueryPool)
        SET_D3D_DEBUG_OBJECT_NAME(query, name);
}
