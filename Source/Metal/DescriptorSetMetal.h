// © 2026 NVIDIA Corporation
#pragma once

namespace nri {

struct DescriptorRangeMappingMetal {
    uint32_t offset = 0;
    uint32_t descriptorNum = 0;
    bool sampler = false;
};

struct DescriptorSetMappingMetal {
    DescriptorSetMappingMetal(StdAllocator<uint8_t>& allocator)
        : ranges(allocator) {
    }

    Vector<DescriptorRangeMappingMetal> ranges;
    uint32_t resourceNum = 0;
    uint32_t samplerNum = 0;
    uint32_t variableRange = UINT32_MAX;
};

struct DescriptorSetMetal final : public DebugNameBase {
    inline DescriptorSetMetal() {
    }

    void Create(const DescriptorPoolMetal* pool, const DescriptorSetMappingMetal* mapping, uint32_t resourceOffset, uint32_t samplerOffset);
    DeviceMetal& GetDevice() const;
    void GetOffsets(uint32_t& resourceOffset, uint32_t& samplerOffset) const;
    uint64_t GetResourceAddress() const;
    uint64_t GetSamplerAddress() const;
    void Update(uint32_t rangeIndex, uint32_t baseDescriptor, const Descriptor* const* descriptors, uint32_t num);
    void Copy(uint32_t dstRange, uint32_t dstBase, const DescriptorSetMetal& source, uint32_t srcRange, uint32_t srcBase, uint32_t num);

    //================================================================================================================
    // NRI
    //================================================================================================================

    static void UpdateDescriptorRanges(const UpdateDescriptorRangeDesc* descs, uint32_t num);
    static void Copy(const CopyDescriptorRangeDesc* descs, uint32_t num);

private:
    uint8_t* GetEntry(const DescriptorRangeMappingMetal& range, uint32_t index) const;

    const DescriptorPoolMetal* m_Pool = nullptr;
    const DescriptorSetMappingMetal* m_Mapping = nullptr; // saves 1 indirection
    uint32_t m_ResourceOffset = 0;
    uint32_t m_SamplerOffset = 0;
};

} // namespace nri
