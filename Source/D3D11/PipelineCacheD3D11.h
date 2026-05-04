// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

// D3D11 has only an implicit driver shader cache - PipelineCache is a NOP stub here so portable code stays portable
struct PipelineCacheD3D11 final : public DebugNameBase {
    inline PipelineCacheD3D11(DeviceD3D11& device)
        : m_Device(device) {
    }

    inline ~PipelineCacheD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    inline Result Create(const PipelineCacheDesc&) {
        return Result::SUCCESS;
    }

    inline Result GetData(void*, uint64_t& size) const {
        size = 0;
        return Result::SUCCESS;
    }

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    inline void SetDebugName(const char*) NRI_DEBUG_NAME_OVERRIDE {
    }

private:
    DeviceD3D11& m_Device;
};

} // namespace nri
