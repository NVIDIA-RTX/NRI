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

    inline uint32_t GetElementOffset() const {
        return m_ElementOffset;
    }

    inline uint32_t GetElementNum() const {
        return m_ElementNum;
    }

    inline uint32_t IsIntegerFormat() const {
        return m_IsIntegerFormat;
    }

    inline const SubresourceInfo& GetSubresourceInfo() const {
        return m_SubresourceInfo;
    }

    inline bool IsBuffer() const {
        return m_ElementNum != 0;
    }

    Result Create(const Texture1DViewDesc& textureViewDesc);
    Result Create(const Texture2DViewDesc& textureViewDesc);
    Result Create(const Texture3DViewDesc& textureViewDesc);
    Result Create(const BufferViewDesc& bufferViewDesc);
    Result Create(const SamplerDesc& samplerDesc);

    DescriptorD3D11(DeviceD3D11& device, ID3D11ShaderResourceView* resource);
    DescriptorD3D11(DeviceD3D11& device, ID3D11UnorderedAccessView* storage);
    DescriptorD3D11(DeviceD3D11& device, ID3D11RenderTargetView* randerTarget);
    DescriptorD3D11(DeviceD3D11& device, ID3D11DepthStencilView* depthStencil);
    DescriptorD3D11(DeviceD3D11& device, ID3D11Buffer* constantBuffer, uint32_t elementOffset = 0, uint32_t elementNum = 0);
    DescriptorD3D11(DeviceD3D11& device, ID3D11SamplerState* sampler);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_Descriptor, name);
    }

private:
    DeviceD3D11& m_Device;
    ComPtr<ID3D11DeviceChild> m_Descriptor;
    SubresourceInfo m_SubresourceInfo = {};
    uint32_t m_ElementOffset = 0;
    uint32_t m_ElementNum = 0;
    DescriptorTypeDX11 m_Type = DescriptorTypeDX11::NO_SHADER_VISIBLE;
    bool m_IsIntegerFormat = false;
};

} // namespace nri
