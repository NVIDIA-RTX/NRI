// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct FenceD3D12 final : public DebugNameBase {
    inline FenceD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline ~FenceD3D12() {
        if (m_Event != 0 && m_Event != INVALID_HANDLE_VALUE)
            CloseHandle(m_Event);
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    Result Create(const FenceDesc& fenceDesc);
    Result Create(const FenceD3D12Desc& fenceD3D12Desc);
    Result GetSharedHandle(SharedHandle& sharedHandle) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_Fence, name);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    uint64_t GetFenceValue() const;
    void QueueSignal(QueueD3D12& queue, uint64_t value);
    void QueueWait(QueueD3D12& queue, uint64_t value);
    Result Wait(uint64_t value);

private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12Fence> m_Fence;
    HANDLE m_Event = 0;
    bool m_IsShared = false;
};

} // namespace nri
