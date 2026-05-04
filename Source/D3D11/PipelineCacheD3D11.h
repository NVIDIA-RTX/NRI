// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

// Offer a fake cache to honor support and minimize supported/unsupported branching
struct PipelineCacheD3D11 final : public DebugNameBase {
    inline PipelineCacheD3D11(DeviceD3D11& device)
        : m_Device(device) {
    }

    inline ~PipelineCacheD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    inline void SetDebugName(const char*) NRI_DEBUG_NAME_OVERRIDE {
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline Result Create(const PipelineCacheDesc&) {
        return Result::SUCCESS;
    }

    inline Result GetData(void*, uint64_t& size) const {
        size = 0;
        return Result::SUCCESS;
    }

private:
    DeviceD3D11& m_Device;
};

} // namespace nri
