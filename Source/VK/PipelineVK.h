// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineVK final : public DebugNameBase {
    inline PipelineVK(DeviceVK& device)
        : m_Device(device)
        , m_VertexStreamStrides(device.GetStdAllocator()) {
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

    inline uint32_t GetVertexStreamStride(uint32_t streamSlot) const {
        return m_VertexStreamStrides[streamSlot];
    }

    ~PipelineVK();

    Result Create(const GraphicsPipelineDesc& graphicsPipelineDesc);
    Result Create(const ComputePipelineDesc& computePipelineDesc);
    Result Create(const RayTracingPipelineDesc& rayTracingPipelineDesc);
    Result Create(VkPipelineBindPoint bindPoint, VKNonDispatchableHandle vkPipeline);

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
    Vector<uint32_t> m_VertexStreamStrides;
    VkPipeline m_Handle = VK_NULL_HANDLE;
    VkPipelineBindPoint m_BindPoint = (VkPipelineBindPoint)0;
    DepthBiasDesc m_DepthBias = {};
    bool m_OwnsNativeObjects = true;
};

} // namespace nri