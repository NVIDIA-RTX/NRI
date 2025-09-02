// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineLayoutD3D11;

struct RasterizerState {
    ComPtr<ID3D11RasterizerState2> ptr;
    uint64_t samplePositionHash = 0;
};

struct PipelineD3D11 final : public DebugNameBase {
    inline PipelineD3D11(DeviceD3D11& device)
        : m_Device(device)
        , m_RasterizerStates(device.GetStdAllocator()) {
    }

    inline ~PipelineD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    Result Create(const GraphicsPipelineDesc& pipelineDesc);
    Result Create(const ComputePipelineDesc& pipelineDesc);
    void Bind(ID3D11DeviceContextBest* deferredContext, const PipelineD3D11* currentPipeline, uint8_t stencilRef, const Color32f& blendFactor, const SamplePositionsState& samplePositionState);

    // Dynamic state
    void ChangeRasterizerState(ID3D11DeviceContextBest* deferredContext, const SamplePositionsState& samplePositionState);
    void ChangeStencilReference(ID3D11DeviceContextBest* deferredContext, uint8_t stencilRef);
    void ChangeBlendConstants(ID3D11DeviceContextBest* deferredContext, const Color32f& color);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

private:
    inline bool IsCompute() const {
        return m_ComputeShader != nullptr;
    }

private:
    DeviceD3D11& m_Device;
    const PipelineLayoutD3D11* m_PipelineLayout = nullptr;
    Vector<RasterizerState> m_RasterizerStates;
    ComPtr<ID3D11VertexShader> m_VertexShader;
    ComPtr<ID3D11HullShader> m_TessControlShader;
    ComPtr<ID3D11DomainShader> m_TessEvaluationShader;
    ComPtr<ID3D11GeometryShader> m_GeometryShader;
    ComPtr<ID3D11PixelShader> m_FragmentShader;
    ComPtr<ID3D11ComputeShader> m_ComputeShader;
    ComPtr<ID3D11InputLayout> m_InputLayout;
    ComPtr<ID3D11DepthStencilState> m_DepthStencilState;
    ComPtr<ID3D11BlendState1> m_BlendState;
#if NRI_ENABLE_NVAPI
    NvAPI_D3D11_RASTERIZER_DESC_EX m_RasterizerDesc = {};
#else
    D3D11_RASTERIZER_DESC m_RasterizerDesc = {};
#endif
    D3D11_PRIMITIVE_TOPOLOGY m_Topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    uint32_t m_SampleMask = uint32_t(-1);
};

} // namespace nri
