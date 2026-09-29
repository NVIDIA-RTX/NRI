// © 2026 NVIDIA Corporation
#pragma once

#include "DescriptorSetMetal.h"

namespace nri {

struct DescriptorPoolMetal final : public DebugNameBase {
    DescriptorPoolMetal(DeviceMetal& device);
    ~DescriptorPoolMetal();
    Result Create(const DescriptorPoolDesc& desc);
    Result Create(const DescriptorHeapDesc& desc);
    Result WriteResourceDescriptors(const WriteResourceDescriptorsDesc* descs, uint32_t num);
    Result WriteSamplerDescriptors(const WriteSamplerDescriptorsDesc* descs, uint32_t num);
    DeviceMetal& GetDevice() const;
    uint64_t GetResourceHeapAddress() const;
    uint64_t GetSamplerHeapAddress() const;
    uint8_t* GetEntry(bool sampler, uint32_t index) const;
    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result AllocateDescriptorSets(const PipelineLayout& layout, uint32_t setIndex, DescriptorSet** sets, uint32_t instanceNum, uint32_t variableNum);
    void Reset();

private:
    DeviceMetal& m_Device;
    Vector<DescriptorSetMetal> m_Sets;
    MTL::Buffer* m_ResourceHeap = nullptr;
    MTL::Buffer* m_SamplerHeap = nullptr;
    uint64_t m_ResourceHeapAddress = 0; // cached, since heap addresses are queried on descriptor set binding paths
    uint64_t m_SamplerHeapAddress = 0;
    uint8_t* m_ResourceEntries = nullptr;
    uint8_t* m_SamplerEntries = nullptr;
    uint32_t m_ResourceCapacity = 0;
    uint32_t m_SamplerCapacity = 0;
    uint32_t m_ResourceUsed = 0;
    uint32_t m_SamplerUsed = 0;
    uint32_t m_SetNum = 0;
    Lock m_Lock;
};

} // namespace nri
