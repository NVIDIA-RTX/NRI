// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorRangeMappingWGPU {
    DescriptorType type = DescriptorType::TEXTURE;
    uint32_t descriptorOffset = 0;
    uint32_t bindingBase = 0;
    uint32_t descriptorNum = 0;
    WGPUShaderStage visibility = WGPUShaderStage_None;
    WGPUTextureSampleType textureSampleType = WGPUTextureSampleType_Float;
    WGPUTextureViewDimension textureViewDimension = WGPUTextureViewDimension_2D;
    WGPUBool textureMultisampled = WGPU_FALSE;
    WGPUTextureFormat storageTextureFormat = WGPUTextureFormat_Undefined;
    WGPUTextureViewDimension storageTextureViewDimension = WGPUTextureViewDimension_2D;
    WGPUStorageTextureAccess storageTextureAccess = WGPUStorageTextureAccess_WriteOnly;
    bool isArray = false;
};

struct DescriptorSetMappingWGPU {
    inline DescriptorSetMappingWGPU(const StdAllocator<uint8_t>& allocator)
        : ranges(allocator) {
    }

    Vector<DescriptorRangeMappingWGPU> ranges;
    WGPUBindGroupLayout layout = nullptr;
    uint32_t bindGroupIndex = 0;
    uint32_t layoutVersion = 1;
};

struct DescriptorSetBindGroupWGPU {
    WGPUBindGroupLayout layout = nullptr;
    WGPUBindGroup bindGroup = nullptr;
    uint64_t updateVersion = 0;
};

struct DescriptorSetWGPU final : public DebugNameBase {
    DescriptorSetWGPU(DeviceWGPU& device, const DescriptorSetMappingWGPU& mapping);
    ~DescriptorSetWGPU();

    inline WGPUBindGroup GetBindGroup() const {
        return GetBindGroup(m_Mapping);
    }

    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    void UpdateRange(uint32_t rangeIndex, uint32_t baseDescriptor, const Descriptor* const* descriptors, uint32_t descriptorNum);
    void CopyRangeFrom(uint32_t dstRangeIndex, uint32_t dstBaseDescriptor, const DescriptorSetWGPU& srcDescriptorSet, uint32_t srcRangeIndex, uint32_t srcBaseDescriptor, uint32_t descriptorNum);
    void FinalizeUpdate() const;
    void GetOffsets(uint32_t& resourceHeapOffset, uint32_t& samplerHeapOffset) const;
    WGPUBindGroup GetBindGroup(const DescriptorSetMappingWGPU& mapping) const;

private:
    bool RecreateBindGroup(const DescriptorSetMappingWGPU& mapping, DescriptorSetBindGroupWGPU& cache) const;

private:
    DeviceWGPU& m_Device;
    const DescriptorSetMappingWGPU& m_Mapping;
    Vector<DescriptorWGPU*> m_Descriptors;
    mutable Vector<DescriptorSetBindGroupWGPU> m_BindGroups;
    mutable Lock m_BindGroupLock;
    uint64_t m_UpdateVersion = 1;
};

} // namespace nri
