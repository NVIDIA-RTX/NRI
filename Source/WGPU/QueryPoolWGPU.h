// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct QueryPoolWGPU final : public DebugNameBase {
    inline QueryPoolWGPU(DeviceWGPU& device)
        : m_Device(device) {
    }

    Result Create(const QueryPoolDesc& queryPoolDesc);

    inline DeviceWGPU& GetDevice() const {
        return m_Device;
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
    uint32_t m_QuerySize = 0;
};

} // namespace nri
