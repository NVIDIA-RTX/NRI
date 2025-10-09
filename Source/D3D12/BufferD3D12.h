// © 2021 NVIDIA Corporation

#pragma once

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
struct ID3D12Resource2;
typedef ID3D12Resource2 ID3D12ResourceBest;
#else
struct ID3D12Resource;
typedef ID3D12Resource ID3D12ResourceBest;
#endif

namespace nri {

struct MemoryD3D12;

struct BufferD3D12 final : public DebugNameBase {
    inline BufferD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline ~BufferD3D12() {
    }

    inline operator ID3D12ResourceBest*() const {
        return m_Buffer.GetInterface();
    }

    inline const BufferDesc& GetDesc() const {
        return m_Desc;
    }

    inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVA() const {
        return m_Buffer->GetGPUVirtualAddress();
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    Result Create(const BufferDesc& bufferDesc);
    Result Create(const BufferD3D12Desc& bufferD3D12Desc);
    Result Allocate(MemoryLocation memoryLocation, float priority, bool committed);
    Result BindMemory(const MemoryD3D12& memory, uint64_t offset);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_Buffer, name);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    void* Map(uint64_t offset);

private:
    Result SetPriorityAndPersistentlyMap(float priority, const D3D12_HEAP_PROPERTIES& heapProps);

private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12ResourceBest> m_Buffer;
    ComPtr<D3D12MA::Allocation> m_VmaAllocation = nullptr;
    uint8_t* m_MappedMemory = nullptr;
    BufferDesc m_Desc = {};
};

inline D3D12_GPU_VIRTUAL_ADDRESS GetBufferAddress(const Buffer* buffer, uint64_t offset) {
    if (!buffer)
        return 0;

    if (buffer == HAS_BUFFER)
        return 1;

    return ((BufferD3D12*)buffer)->GetGPUVA() + offset;
}

} // namespace nri
