// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineVK final : public DebugNameBase {
    inline PipelineVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline operator VkPipeline() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline VkPipelineBindPoint GetBindPoint() const {
        return m_BindPoint;
    }

    inline const DepthBiasDesc& GetDepthBias() const {
        return m_DepthBias;
    }

    ~PipelineVK();

    Result Create(const GraphicsPipelineDesc& graphicsPipelineDesc);
    Result Create(const ComputePipelineDesc& computePipelineDesc);
    Result Create(const RayTracingPipelineDesc& rayTracingPipelineDesc);
    Result Create(const PipelineVKDesc& pipelineVKDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* dst) const;

private:
    Result SetupShaderStage(VkPipelineShaderStageCreateInfo& stage, const ShaderDesc& shaderDesc, VkShaderModule& module);

private:
    DeviceVK& m_Device;
    VkPipeline m_Handle = VK_NULL_HANDLE;
    VkPipelineBindPoint m_BindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
    DepthBiasDesc m_DepthBias = {};
    bool m_OwnsNativeObjects = true;
};

} // namespace nri