// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorD3D11 final : public DebugNameBase {
    inline DescriptorD3D11(DeviceD3D11& device)
        : m_Device(device) {
    }

    inline ~DescriptorD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    inline Format GetFormat() const {
        return m_Format;
    }

    inline const SubresourceInfo& GetSubresourceInfo() const {
        return m_SubresourceInfo;
    }

    inline operator ID3D11View*() const {
        return (ID3D11View*)m_Descriptor.GetInterface();
    }

    inline operator ID3D11RenderTargetView*() const {
        return (ID3D11RenderTargetView*)m_Descriptor.GetInterface();
    }

    inline operator ID3D11DepthStencilView*() const {
        return (ID3D11DepthStencilView*)m_Descriptor.GetInterface();
    }

    inline operator ID3D11UnorderedAccessView*() const {
        return (ID3D11UnorderedAccessView*)m_Descriptor.GetInterface();
    }

#if NRI_ENABLE_NVAPI
    inline operator ID3D11NvShadingRateResourceView*() const {
        return (ID3D11NvShadingRateResourceView*)m_Descriptor.GetInterface();
    }
#endif

    Result Create(const Texture1DViewDesc& textureViewDesc);
    Result Create(const Texture2DViewDesc& textureViewDesc);
    Result Create(const Texture3DViewDesc& textureViewDesc);
    Result Create(const BufferViewDesc& bufferViewDesc);
    Result Create(const SamplerDesc& samplerDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_Descriptor, name);
    }

private:
    DeviceD3D11& m_Device;
    ComPtr<ID3D11DeviceChild> m_Descriptor;
    SubresourceInfo m_SubresourceInfo = {};
    Format m_Format = Format::UNKNOWN;
};

} // namespace nri
