// © 2021 NVIDIA Corporation

#pragma once

struct ID3D12Resource;

namespace nri {

struct AccelerationStructureD3D12;

struct DescriptorD3D12 final : public DebugNameBase {
    inline DescriptorD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline ~DescriptorD3D12() {
        m_Device.FreeDescriptorHandle(m_Handle);
    }

    inline operator ID3D12Resource*() const {
        return m_Resource;
    }

    inline DescriptorHandleCPU GetDescriptorHandleCPU() const {
        return m_DescriptorHandleCPU;
    }

    inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVA() const {
        return m_BufferLocation;
    }

    inline BufferViewType GetBufferViewType() const {
        return m_BufferViewType;
    }

    inline bool IsIntegerFormat() const {
        return m_IsIntegerFormat;
    }

    inline bool IsAccelerationStructure() const {
        return m_IsAccelerationStructure;
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
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
    Result CreateUnorderedAccessView(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, Format format);
    Result CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& desc);
    Result CreateDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc);

private:
    DeviceD3D12& m_Device;
    ID3D12Resource* m_Resource = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS m_BufferLocation = 0;
    DescriptorHandleCPU m_DescriptorHandleCPU = {};
    DescriptorHandle m_Handle = {};
    BufferViewType m_BufferViewType = BufferViewType::MAX_NUM;
    bool m_IsIntegerFormat = false;
    bool m_IsAccelerationStructure = false;
};

} // namespace nri
