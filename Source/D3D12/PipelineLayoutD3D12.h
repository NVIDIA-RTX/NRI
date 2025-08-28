// © 2021 NVIDIA Corporation

#pragma once

#include "DescriptorSetD3D12.h"

namespace nri {

typedef uint16_t RootParameterIndexType;
constexpr RootParameterIndexType ROOT_PARAMETER_UNUSED = RootParameterIndexType(-1);

struct DescriptorRangeMapping {
    uint32_t heapOffset;
    uint32_t descriptorNum;
    RootParameterIndexType rootParameterIndex;
    DescriptorHeapType descriptorHeapType;
};

struct DescriptorSetMapping {
    inline DescriptorSetMapping(StdAllocator<uint8_t>& allocator)
        : descriptorRangeMappings(allocator) {
    }

    Vector<DescriptorRangeMapping> descriptorRangeMappings;
    std::array<uint32_t, DescriptorHeapType::MAX_NUM> descriptorNum = {};
};

struct PipelineLayoutD3D12 final : public DebugNameBase {
    inline PipelineLayoutD3D12(DeviceD3D12& device)
        : m_Device(device)
        , m_DescriptorSetMappings(device.GetStdAllocator()) {
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
    uint32_t m_BaseRootConstant = 0;
    uint32_t m_BaseRootDescriptor = 0;
    bool m_DrawParametersEmulation = false;
};

} // namespace nri