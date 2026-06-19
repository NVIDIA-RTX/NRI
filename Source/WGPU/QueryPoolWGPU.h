// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct QueryPoolWGPU final : public DebugNameBase {
    inline QueryPoolWGPU(DeviceWGPU& device)
        : m_Device(device) {
    }

    ~QueryPoolWGPU();

    inline operator WGPUQuerySet() const {
        return m_QuerySet;
    }

    Result Create(const QueryPoolDesc& queryPoolDesc);
    void Reset(uint32_t offset, uint32_t num);

    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    inline QueryType GetType() const {
        return m_Desc.queryType;
    }

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        if (m_QuerySet)
            wgpuQuerySetSetLabel(m_QuerySet, WGPUString(name));
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline uint32_t GetQuerySize() const {
        return m_QuerySize;
    }

private:
    DeviceWGPU& m_Device;
    QueryPoolDesc m_Desc = {};
    WGPUQuerySet m_QuerySet = nullptr;
    uint32_t m_QuerySize = 0;
};

} // namespace nri
