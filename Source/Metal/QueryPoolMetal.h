// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct QueryPoolMetal final : public DebugNameBase {
    inline QueryPoolMetal(DeviceMetal& device) : m_Device(device) {
    }

    ~QueryPoolMetal();

    Result Create(const QueryPoolDesc& desc);

    inline DeviceMetal& GetDevice() const {
        return m_Device;
    }

    inline uint32_t GetQuerySize() const {
        return m_QuerySize;
    }

    inline QueryType GetType() const {
        return m_Type;
    }

    inline MTL4::CounterHeap* GetCounterHeap() const {
        return m_CounterHeap;
    }

    inline MTL::Buffer* GetVisibilityBuffer() const {
        return m_VisibilityBuffer;
    }

    void Reset(uint32_t offset, uint32_t num);
    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    DeviceMetal& m_Device;
    MTL4::CounterHeap* m_CounterHeap = nullptr;
    MTL::Buffer* m_VisibilityBuffer = nullptr;
    QueryType m_Type = QueryType::MAX_NUM;
    uint32_t m_QuerySize = sizeof(uint64_t);
};

} // namespace nri
