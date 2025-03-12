// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineVal final : public ObjectVal {
    PipelineVal(DeviceVal& device, Pipeline* pipeline);
    PipelineVal(DeviceVal& device, Pipeline* pipeline, const GraphicsPipelineDesc& graphicsPipelineDesc);
    PipelineVal(DeviceVal& device, Pipeline* pipeline, const ComputePipelineDesc& computePipelineDesc);
    PipelineVal(DeviceVal& device, Pipeline* pipeline, const RayTracingPipelineDesc& rayTracingPipelineDesc);

    inline Pipeline* GetImpl() const {
        return (Pipeline*)m_Impl;
    }

    inline const PipelineLayout* GetPipelineLayout() const {
        return m_PipelineLayout;
    }

    inline bool WritesToDepth() const {
        return m_WritesToDepth;
    }

    inline bool WritesToStencil() const {
        return m_WritesToStencil;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* dst);

private:
    const PipelineLayout* m_PipelineLayout = nullptr;
    bool m_WritesToDepth = false;
    bool m_WritesToStencil = false;
};

} // namespace nri
