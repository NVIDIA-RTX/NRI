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

struct TextureD3D12 final : public DebugNameBase {
    inline TextureD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline ~TextureD3D12() {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline const TextureDesc& GetDesc() const {
        return m_Desc;
    }

    inline operator ID3D12ResourceBest*() const {
        return m_Texture.GetInterface();
    }

    inline uint32_t GetSubresourceIndex(uint32_t layerOffset, uint32_t mipOffset, PlaneBits planes) const {
        // https://learn.microsoft.com/en-us/windows/win32/direct3d12/subresources#plane-slice
        uint32_t planeIndex = 0;
        if (planes == PlaneBits::ALL || (planes & PlaneBits::STENCIL) != 0)
            planeIndex = 1;
        if (planes == PlaneBits::ALL || (planes & PlaneBits::DEPTH) != 0) // fallthrough
            planeIndex = 0;
        if (planes == PlaneBits::ALL || (planes & PlaneBits::COLOR) != 0) // fallthrough
            planeIndex = 0;

        return mipOffset + (layerOffset + planeIndex * m_Desc.layerNum) * m_Desc.mipNum;
    }

    inline Dim_t GetSize(Dim_t dimensionIndex, Dim_t mip = 0) const {
        return GetDimension(GraphicsAPI::D3D12, m_Desc, dimensionIndex, mip);
    }

    Result Create(const TextureDesc& textureDesc);
    Result Create(const TextureD3D12Desc& textureD3D12Desc);
    Result Allocate(MemoryLocation memoryLocation, float priority, bool committed);
    Result BindMemory(const MemoryD3D12& memory, uint64_t offset);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_Texture, name);
    }

private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12ResourceBest> m_Texture;
    ComPtr<D3D12MA::Allocation> m_VmaAllocation = nullptr;
    TextureDesc m_Desc = {};
};

} // namespace nri
