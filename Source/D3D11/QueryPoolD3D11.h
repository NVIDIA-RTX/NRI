// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct QueryPoolD3D11 final : public DebugNameBase {
    inline QueryPoolD3D11(DeviceD3D11& device)
        : m_Device(device)
        , m_QueryPool(device.GetStdAllocator()) {
    }

    inline ~QueryPoolD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    Result Create(const QueryPoolDesc& queryPoolDesc);
    void BeginQuery(ID3D11DeviceContextBest* deferredContext, uint32_t offset);
    void EndQuery(ID3D11DeviceContextBest* deferredContext, uint32_t offset);
    void GetData(uint8_t* dstMemory, uint32_t offset, uint32_t num) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline uint32_t GetQuerySize() const {
        return m_Type == QueryType::PIPELINE_STATISTICS ? sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS) : sizeof(uint64_t);
    }

private:
    DeviceD3D11& m_Device;
    Vector<ComPtr<ID3D11Query>> m_QueryPool;
    QueryType m_Type = QueryType::MAX_NUM;
};

} // namespace nri
