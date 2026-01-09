// © 2021 NVIDIA Corporation

#pragma once

struct ID3D12Resource;

namespace nri {

struct AccelerationStructureD3D12;

struct TexViewDesc {
    Dim_t layerOffset;
    Dim_t layerNum;
    Dim_t mipOffset;
    Dim_t mipNum;
};

struct DescriptorD3D12 final : public DebugNameBase {
    inline DescriptorD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline ~DescriptorD3D12() {
        m_Device.FreeDescriptorHandle(m_Handle);
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline DescriptorType GetType() const {
        return m_Type;
    }

    inline Format GetFormat() const {
        return m_Format;
    }

    inline DescriptorHandleCPU GetDescriptorHandleCPU() const {
        return m_DescriptorHandleCPU;
    }

    inline D3D12_GPU_VIRTUAL_ADDRESS GetDeviceAddress() const {
        return m_ViewDesc.bufferGPUVA;
    }

    inline ID3D12Resource* GetResource() const {
        return m_Resource;
    }
    inline const TexViewDesc& GetTexViewDesc() const {
        return m_ViewDesc.texture;
    }

    Result Create(const BufferViewDesc& bufferViewDesc);
    Result Create(const Texture1DViewDesc& textureViewDesc);
    Result Create(const Texture2DViewDesc& textureViewDesc);
    Result Create(const Texture3DViewDesc& textureViewDesc);
    Result Create(const AccelerationStructureD3D12& accelerationStructure);
    Result Create(const SamplerDesc& samplerDesc);

private:
    Result CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc);
    Result CreateShaderResourceView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
    Result CreateUnorderedAccessView(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);
    Result CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& desc);
    Result CreateDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc);

private:
    DeviceD3D12& m_Device;
    DescriptorHandleCPU m_DescriptorHandleCPU = {};
    ID3D12Resource* m_Resource = nullptr;

    union ViewDesc {
        TexViewDesc texture = {}; // larger first
        D3D12_GPU_VIRTUAL_ADDRESS bufferGPUVA;
    } m_ViewDesc;

    DescriptorHandle m_Handle = {};
    DescriptorType m_Type = DescriptorType::MAX_NUM;
    Format m_Format = Format::UNKNOWN;
};

} // namespace nri
