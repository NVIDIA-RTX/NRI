// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct PushConstantBindingDesc {
    VkShaderStageFlags stages;
    uint32_t offset;
};

struct BindingInfo {
    BindingInfo(StdAllocator<uint8_t>& allocator)
        : ranges(allocator)
        , sets(allocator)
        , pushConstants(allocator)
        , pushDescriptors(allocator) {
    }

    Vector<DescriptorRangeDesc> ranges;
    Vector<DescriptorSetDesc> sets;
    Vector<PushConstantBindingDesc> pushConstants;
    Vector<uint32_t> pushDescriptors;
    uint32_t rootRegisterSpace;
    uint32_t rootSamplerBindingOffset;
};

struct PipelineLayoutVK final : public DebugNameBase {
    inline PipelineLayoutVK(DeviceVK& device)
        : m_Device(device)
        , m_BindingInfo(device.GetStdAllocator())
        , m_DescriptorSetLayouts(device.GetStdAllocator())
        , m_ImmutableSamplers(device.GetStdAllocator()) {
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

    ~PipelineLayoutVK();

    Result Create(const PipelineLayoutDesc& pipelineLayoutDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

private:
    void CreateSetLayout(VkDescriptorSetLayout* setLayout, const DescriptorSetDesc& descriptorSetDesc, const RootSamplerDesc* rootSamplers, uint32_t rootSamplerNum, bool ignoreGlobalSPIRVOffsets, bool isPush);

private:
    DeviceVK& m_Device;
    VkPipelineLayout m_Handle = VK_NULL_HANDLE;
    BindingInfo m_BindingInfo;
    Vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
    Vector<VkSampler> m_ImmutableSamplers;
};

} // namespace nri