// © 2021 NVIDIA Corporation

#pragma once

#include "DescriptorSetVal.h"

namespace nri {

struct DescriptorPoolVal final : public ObjectVal {
    DescriptorPoolVal(DeviceVal& device, DescriptorPool* descriptorPool, uint32_t descriptorSetMaxNum)
        : ObjectVal(device, descriptorPool)
        , m_DescriptorSets(device.GetStdAllocator())
        , m_SkipValidation(true) // TODO: we have to request "DescriptorPoolDesc" in "DescriptorPoolVKDesc"
    {
        m_Desc.descriptorSetMaxNum = descriptorSetMaxNum;
        m_DescriptorSets.reserve(m_Desc.descriptorSetMaxNum);
        for (uint32_t i = 0; i < m_Desc.descriptorSetMaxNum; i++)
            m_DescriptorSets.emplace_back(DescriptorSetVal(device));
    }

    DescriptorPoolVal(DeviceVal& device, DescriptorPool* descriptorPool, const DescriptorPoolDesc& descriptorPoolDesc)
        : ObjectVal(device, descriptorPool)
        , m_Desc(descriptorPoolDesc)
        , m_DescriptorSets(device.GetStdAllocator()) {
        m_DescriptorSets.reserve(m_Desc.descriptorSetMaxNum);
        for (uint32_t i = 0; i < m_Desc.descriptorSetMaxNum; i++)
            m_DescriptorSets.emplace_back(DescriptorSetVal(device));
    }

    inline DescriptorPool* GetImpl() const {
        return (DescriptorPool*)m_Impl;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    void Reset();
    Result AllocateDescriptorSets(const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum);

private:
    DescriptorPoolDesc m_Desc = {}; // .natvis
    Vector<DescriptorSetVal> m_DescriptorSets;
    uint32_t m_DescriptorSetsNum = 0;
    uint32_t m_SamplerNum = 0;
    uint32_t m_ConstantBufferNum = 0;
    uint32_t m_TextureNum = 0;
    uint32_t m_StorageTextureNum = 0;
    uint32_t m_BufferNum = 0;
    uint32_t m_StorageBufferNum = 0;
    uint32_t m_StructuredBufferNum = 0;
    uint32_t m_StorageStructuredBufferNum = 0;
    uint32_t m_AccelerationStructureNum = 0;
    bool m_SkipValidation = false;
};

} // namespace nri
