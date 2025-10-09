// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct MemoryD3D12 final : public DebugNameBase {
    inline MemoryD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline ~MemoryD3D12() {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline const D3D12_HEAP_DESC& GetHeapDesc() const {
        return m_HeapDesc;
    }

    inline float GetPriority() const {
        return m_Priority;
    }

    inline bool IsDummy() const {
        return !m_VmaAllocation && !m_Heap;
    }

    // Starting from "0" (must be used with "GetOffset")
    inline operator ID3D12Heap*() const {
        return m_VmaAllocation ? m_VmaAllocation->GetHeap() : m_Heap.GetInterface();
    }

    inline uint64_t GetOffset() const {
        return m_VmaAllocation ? m_VmaAllocation->GetOffset() : m_Offset;
    }

    Result Create(const AllocateMemoryDesc& allocateMemoryDesc);
    Result Create(const MemoryD3D12Desc& memoryD3D12Desc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_Heap, name);
        // TODO: m_VmaAllocation->SetName()?
    }

private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12Heap> m_Heap;
    ComPtr<D3D12MA::Allocation> m_VmaAllocation;
    uint64_t m_Offset = 0;
    D3D12_HEAP_DESC m_HeapDesc = {};
    float m_Priority = 0.0f;
};

} // namespace nri
