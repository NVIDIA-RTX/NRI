// © 2021 NVIDIA Corporation

#pragma once

#include "DescriptorSetD3D12.h"

namespace nri {

constexpr uint16_t ROOT_PARAMETER_UNUSED = uint16_t(-1);

struct DescriptorRangeMapping {
    DescriptorHeapType descriptorHeapType;
    uint32_t heapOffset;
    uint32_t descriptorNum;
};

struct DescriptorSetMapping {
    inline DescriptorSetMapping(StdAllocator<uint8_t>& allocator)
        : descriptorRangeMappings(allocator) {
    }

    std::array<uint32_t, DescriptorHeapType::MAX_NUM> descriptorNum = {};
    Vector<DescriptorRangeMapping> descriptorRangeMappings;
};

struct DescriptorSetRootMapping {
    inline DescriptorSetRootMapping(StdAllocator<uint8_t>& allocator)
        : rootOffsets(allocator) {
    }

    Vector<uint16_t> rootOffsets;
};

struct DynamicConstantBufferMapping {
    uint16_t rootConstantNum;
    uint16_t rootOffset;
};

struct PipelineLayoutD3D12 final : public DebugNameBase {
    inline PipelineLayoutD3D12(DeviceD3D12& device)
        : m_Device(device)
        , m_DescriptorSetMappings(device.GetStdAllocator())
        , m_DescriptorSetRootMappings(device.GetStdAllocator())
        , m_DynamicConstantBufferMappings(device.GetStdAllocator()) {
    }

    inline operator ID3D12RootSignature*() const {
        return m_RootSignature.GetInterface();
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline bool IsDrawParametersEmulationEnabled() const {
        return m_DrawParametersEmulation;
    }

    inline const DescriptorSetMapping& GetDescriptorSetMapping(uint32_t setIndex) const {
        return m_DescriptorSetMappings[setIndex];
    }

    inline const DynamicConstantBufferMapping& GetDynamicConstantBufferMapping(uint32_t setIndex) const {
        return m_DynamicConstantBufferMappings[setIndex];
    }

    Result Create(const PipelineLayoutDesc& pipelineLayoutDesc);
    void SetDescriptorSet(ID3D12GraphicsCommandList* graphicsCommandList, BindPoint bindPoint, const SetDescriptorSetDesc& setDescriptorSetDesc) const;
    void SetRootConstants(ID3D12GraphicsCommandList* graphicsCommandList, BindPoint bindPoint, const SetRootConstantsDesc& setRootConstantsDesc) const;
    void SetRootDescriptor(ID3D12GraphicsCommandList* graphicsCommandList, BindPoint bindPoint, const SetRootDescriptorDesc& setRootDescriptorDesc) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_RootSignature, name);
    }

private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12RootSignature> m_RootSignature;
    Vector<DescriptorSetMapping> m_DescriptorSetMappings;
    Vector<DescriptorSetRootMapping> m_DescriptorSetRootMappings;
    Vector<DynamicConstantBufferMapping> m_DynamicConstantBufferMappings;
    uint32_t m_BaseRootConstant = 0;
    uint32_t m_BaseRootDescriptor = 0;
    bool m_DrawParametersEmulation = false;
};

} // namespace nri