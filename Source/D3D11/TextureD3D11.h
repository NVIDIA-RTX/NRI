// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct TextureD3D11 final : public DebugNameBase {
    inline TextureD3D11(DeviceD3D11& device)
        : m_Device(device) {
    }

    inline ~TextureD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    inline const TextureDesc& GetDesc() const {
        return m_Desc;
    }

    inline operator ID3D11Resource*() const {
        return m_Texture;
    }

    inline operator ID3D11Texture1D*() const {
        return (ID3D11Texture1D*)m_Texture.GetInterface();
    }

    inline operator ID3D11Texture2D*() const {
        return (ID3D11Texture2D*)m_Texture.GetInterface();
    }

    inline operator ID3D11Texture3D*() const {
        return (ID3D11Texture3D*)m_Texture.GetInterface();
    }

    inline uint32_t GetSubresourceIndex(uint32_t layerOffset, uint32_t mipOffset) const {
        // https://learn.microsoft.com/en-us/windows/win32/direct3d12/subresources#plane-slice
        return mipOffset + layerOffset * m_Desc.mipNum;
    }

    inline Dim_t GetSize(Dim_t dimensionIndex, Dim_t mip = 0) const {
        return GetDimension(GraphicsAPI::D3D11, m_Desc, dimensionIndex, mip);
    }

    Result Create(const TextureDesc& textureDesc);
    Result Create(const TextureD3D11Desc& textureD3D11Desc);
    Result Allocate(MemoryLocation memoryLocation, float priority);

    static uint32_t GetMipmappedSize(const TextureDesc& textureDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_Texture, name);
    }

private:
    DeviceD3D11& m_Device;
    ComPtr<ID3D11Resource> m_Texture;
    TextureDesc m_Desc = {};
};

} // namespace nri
