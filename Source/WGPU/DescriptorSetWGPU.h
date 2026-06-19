// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorRangeMappingWGPU {
    DescriptorType type = DescriptorType::TEXTURE;
    uint32_t descriptorOffset = 0;
    uint32_t bindingBase = 0;
    uint32_t descriptorNum = 0;
    WGPUShaderStage visibility = WGPUShaderStage_None;
    WGPUTextureFormat storageTextureFormat = WGPUTextureFormat_Undefined;
    bool isArray = false;
};

struct DescriptorSetMappingWGPU {
    inline DescriptorSetMappingWGPU(const StdAllocator<uint8_t>& allocator)
        : ranges(allocator) {
    }

    Vector<DescriptorRangeMappingWGPU> ranges;
    WGPUBindGroupLayout layout = nullptr;
    uint32_t bindGroupIndex = 0;
};

struct DescriptorSetWGPU final : public DebugNameBase {
    DescriptorSetWGPU(DeviceWGPU& device, const DescriptorSetMappingWGPU& mapping);
    ~DescriptorSetWGPU();

    inline WGPUBindGroup GetBindGroup() const {
        return m_BindGroup;
    }

    void UpdateRange(uint32_t rangeIndex, uint32_t baseDescriptor, const Descriptor* const* descriptors, uint32_t descriptorNum);
    void CopyRangeFrom(uint32_t dstRangeIndex, uint32_t dstBaseDescriptor, const DescriptorSetWGPU& srcDescriptorSet, uint32_t srcRangeIndex, uint32_t srcBaseDescriptor, uint32_t descriptorNum);
    void GetOffsets(uint32_t& resourceHeapOffset, uint32_t& samplerHeapOffset) const;

private:
    void RecreateBindGroup() const;

private:
    DeviceWGPU& m_Device;
    const DescriptorSetMappingWGPU& m_Mapping;
    Vector<DescriptorWGPU*> m_Descriptors;
    mutable WGPUBindGroup m_BindGroup = nullptr;
    mutable bool m_IsDirty = false;
};

} // namespace nri
