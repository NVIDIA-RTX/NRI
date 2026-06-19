// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineWGPU final : public DebugNameBase {
    inline PipelineWGPU(DeviceWGPU& device)
        : m_Device(device) {
    }

    ~PipelineWGPU();

    inline WGPURenderPipeline GetRenderPipeline() const {
        return m_RenderPipeline;
    }

    inline WGPUComputePipeline GetComputePipeline() const {
        return m_ComputePipeline;
    }

    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    inline const PipelineLayoutWGPU* GetPipelineLayout() const {
        return m_PipelineLayout;
    }

    Result Create(const GraphicsPipelineDesc& graphicsPipelineDesc);
    Result Create(const ComputePipelineDesc& computePipelineDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        MaybeUnused(name);
    }

private:
    WGPUShaderModule CreateShaderModule(const ShaderDesc& shaderDesc);

private:
    DeviceWGPU& m_Device;
    WGPURenderPipeline m_RenderPipeline = nullptr;
    WGPUComputePipeline m_ComputePipeline = nullptr;
    PipelineLayoutWGPU* m_PipelineLayout = nullptr;
};

} // namespace nri
