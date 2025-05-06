// © 2021 NVIDIA Corporation

QueryPoolVal::QueryPoolVal(DeviceVal& device, QueryPool* queryPool, QueryType queryType, uint32_t queryNum)
    : ObjectVal(device, queryPool)
    , m_QueryType(queryType) {
    m_QueryNum = queryNum;
}

void QueryPoolVal::ResetQueries(uint32_t offset, uint32_t num) {
    GetCoreInterfaceImpl().ResetQueries(*GetImpl(), offset, num);
}

NRI_INLINE uint32_t QueryPoolVal::GetQuerySize() const {
    return GetCoreInterfaceImpl().GetQuerySize(*GetImpl());
}
