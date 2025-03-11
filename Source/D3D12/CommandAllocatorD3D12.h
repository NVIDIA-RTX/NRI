// © 2021 NVIDIA Corporation

#pragma once

struct ID3D12CommandAllocator;
enum D3D12_COMMAND_LIST_TYPE;

namespace nri {

struct CommandAllocatorD3D12 final : public DebugNameBase {
    inline CommandAllocatorD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline ~CommandAllocatorD3D12() {
    }

    inline operator ID3D12CommandAllocator*() const {
        return m_CommandAllocator.GetInterface();
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    Result Create(const Queue& queue);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_CommandAllocator, name);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result CreateCommandBuffer(CommandBuffer*& commandBuffer);
    void Reset();

private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    D3D12_COMMAND_LIST_TYPE m_CommandListType = D3D12_COMMAND_LIST_TYPE(-1);
    Lock m_Lock;
};

} // namespace nri
