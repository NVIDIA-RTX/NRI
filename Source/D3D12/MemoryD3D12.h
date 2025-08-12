// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct MemoryD3D12 final : public DebugNameBase {
    inline MemoryD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline ~MemoryD3D12() {
    }

    inline operator ID3D12Heap*() const {
        return m_Heap.GetInterface();
    }

    inline const D3D12_HEAP_DESC& GetHeapDesc() const {
        return m_HeapDesc;
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline bool IsDummy() const {
        return m_Heap.GetInterface() == nullptr;
    }

    inline float GetPriority() const {
        return m_Priority;
    }

    Result Create(const AllocateMemoryDesc& allocateMemoryDesc);
    Result Create(const MemoryD3D12Desc& memoryD3D12Desc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_Heap, name);
    }

private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12Heap> m_Heap;
    D3D12_HEAP_DESC m_HeapDesc = {};
    float m_Priority = 0.0f;
};

} // namespace nri
