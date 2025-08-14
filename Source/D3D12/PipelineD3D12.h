// © 2021 NVIDIA Corporation

#pragma once

#include "DescriptorSetD3D12.h"

namespace nri {

struct PipelineLayoutD3D12;

struct PipelineD3D12 final : public DebugNameBase {
    inline PipelineD3D12(DeviceD3D12& device)
        : m_Device(device)
        , m_ShaderGroupNames(device.GetStdAllocator()) {
    }

    inline ~PipelineD3D12() {
    }

    inline operator ID3D12PipelineState*() const {
        return m_PipelineState.GetInterface();
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline const PipelineLayoutD3D12& GetPipelineLayout() const {
        return *m_PipelineLayout;
    }

    Result Create(const GraphicsPipelineDesc& graphicsPipelineDesc);
    Result Create(const ComputePipelineDesc& computePipelineDesc);
    Result Create(const RayTracingPipelineDesc& rayTracingPipelineDesc);

    void Bind(ID3D12GraphicsCommandList* graphicsCommandList) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_PipelineState, name);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* dst) const;

private:
    Result CreateFromStream(const GraphicsPipelineDesc& graphicsPipelineDesc);

private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12PipelineState> m_PipelineState;
    ComPtr<ID3D12StateObject> m_StateObject;
    ComPtr<ID3D12StateObjectProperties> m_StateObjectProperties;
    Vector<std::wstring> m_ShaderGroupNames;
    const PipelineLayoutD3D12* m_PipelineLayout = nullptr;
    D3D_PRIMITIVE_TOPOLOGY m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
};

} // namespace nri
