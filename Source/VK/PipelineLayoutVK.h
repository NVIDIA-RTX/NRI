// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct PushConstantBindingDesc {
    VkShaderStageFlags stages;
    uint32_t offset;
};

struct PushDescriptorBindingDesc {
    uint32_t registerSpace;
    uint32_t registerIndex;
};

struct BindingInfo {
    BindingInfo(StdAllocator<uint8_t>& allocator);

    Vector<bool> hasVariableDescriptorNum;
    Vector<DescriptorRangeDesc> descriptorSetRangeDescs;
    Vector<DynamicConstantBufferDesc> dynamicConstantBufferDescs;
    Vector<DescriptorSetDesc> descriptorSetDescs;
    Vector<PushConstantBindingDesc> pushConstantBindings;
    Vector<PushDescriptorBindingDesc> pushDescriptorBindings;
};

struct PipelineLayoutVK final : public DebugNameBase {
    inline PipelineLayoutVK(DeviceVK& device)
        : m_Device(device)
        , m_BindingInfo(device.GetStdAllocator())
        , m_DescriptorSetLayouts(device.GetStdAllocator()) {
    }

    inline operator VkPipelineLayout() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline const BindingInfo& GetBindingInfo() const {
        return m_BindingInfo;
    }

    inline VkDescriptorSetLayout GetDescriptorSetLayout(uint32_t setIndex) const {
        return m_DescriptorSetLayouts[setIndex];
    }

    inline VkPipelineBindPoint GetPipelineBindPoint() const {
        return m_PipelineBindPoint;
    }

    ~PipelineLayoutVK();

    Result Create(const PipelineLayoutDesc& pipelineLayoutDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

private:
    void CreateSetLayout(VkDescriptorSetLayout* setLayout, const DescriptorSetDesc& descriptorSetDesc, bool ignoreGlobalSPIRVOffsets, bool isPush);

private:
    DeviceVK& m_Device;
    VkPipelineLayout m_Handle = VK_NULL_HANDLE;
    VkPipelineBindPoint m_PipelineBindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
    BindingInfo m_BindingInfo;
    Vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
};

} // namespace nri