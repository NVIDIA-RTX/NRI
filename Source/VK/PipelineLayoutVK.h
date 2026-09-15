// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct PushConstantBindingDesc {
    VkShaderStageFlags stages;
    uint32_t offset;
};

struct DescriptorHeapMappingSamplerVK {
    VkSamplerCreateInfo sampler;
    VkSamplerReductionModeCreateInfo reduction;
};

struct BindingInfo {
    BindingInfo(StdAllocator<uint8_t>& allocator)
        : ranges(allocator)
        , sets(allocator)
        , pushConstants(allocator)
        , pushDescriptors(allocator)
        , rootConstants(allocator)
        , rootDescriptors(allocator)
        , rootSamplers(allocator) {
    }

    Vector<DescriptorRangeDesc> ranges;
    Vector<DescriptorSetDesc> sets;
    Vector<PushConstantBindingDesc> pushConstants;
    Vector<uint32_t> pushDescriptors;
    Vector<RootConstantDesc> rootConstants;
    Vector<RootDescriptorDesc> rootDescriptors;
    Vector<RootSamplerDesc> rootSamplers;
    uint32_t rootRegisterSpace = 0;
    uint32_t rootSamplerBindingOffset = 0;
    bool ignoreGlobalSPIRVOffsets = false;
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

    inline bool IsDescriptorHeap() const {
        return m_IsDescriptorHeap;
    }

    inline uint32_t GetDescriptorHeapMappingMaxNum() const {
        return m_IsDescriptorHeap ? (uint32_t)(m_BindingInfo.rootConstants.size() + m_BindingInfo.rootDescriptors.size() + m_BindingInfo.rootSamplers.size()) : 0;
    }

    inline uint32_t GetDescriptorHeapSamplerMaxNum() const {
        return m_IsDescriptorHeap ? (uint32_t)m_BindingInfo.rootSamplers.size() : 0;
    }

    ~PipelineLayoutVK();

    Result Create(const PipelineLayoutDesc& pipelineLayoutDesc);
    uint32_t SetupDescriptorHeapMappings(VkShaderStageFlagBits stage, VkDescriptorSetAndBindingMappingEXT* mappings, DescriptorHeapMappingSamplerVK* samplers) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    void CreateSetLayout(VkDescriptorSetLayout* setLayout, const DescriptorSetDesc& descriptorSetDesc, const RootSamplerDesc* rootSamplers, uint32_t rootSamplerNum, bool ignoreGlobalSPIRVOffsets, bool isPush);

private:
    DeviceVK& m_Device;
    VkPipelineLayout m_Handle = VK_NULL_HANDLE;
    BindingInfo m_BindingInfo;
    Vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
    Vector<VkSampler> m_ImmutableSamplers;
    bool m_IsDescriptorHeap = false;
};

} // namespace nri
