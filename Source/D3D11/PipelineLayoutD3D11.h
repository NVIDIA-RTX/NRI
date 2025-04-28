// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorD3D11;
struct DescriptorSetD3D11;

struct BindingSet {
    uint32_t descriptorNum;
    uint32_t startRangeOfDynamicConstantBuffers;

    union {
        uint32_t endRangeOfDynamicConstantBuffers;
        uint32_t startRange;
    };

    uint32_t endRange;
};

struct BindingRange {
    uint32_t baseSlot;
    uint32_t descriptorNum;
    uint32_t descriptorOffset;
    StageBits shaderStages;
    DescriptorTypeDX11 descriptorType;
};

struct ConstantBuffer {
    ComPtr<ID3D11Buffer> buffer;
    uint32_t slot;
    StageBits shaderStages;
};

struct PipelineLayoutD3D11 final : public DebugNameBase {
    inline PipelineLayoutD3D11(DeviceD3D11& device)
        : m_Device(device)
        , m_BindingSets(device.GetStdAllocator())
        , m_BindingRanges(device.GetStdAllocator())
        , m_ConstantBuffers(device.GetStdAllocator()) {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    inline const BindingSet& GetBindingSet(uint32_t set) const {
        return m_BindingSets[set];
    }

    inline const BindingRange& GetBindingRange(uint32_t range) const {
        return m_BindingRanges[range];
    }

    inline uint32_t GetRootBindingIndex(uint32_t rootDescriptorIndex) const {
        return m_RootBindingOffset + rootDescriptorIndex;
    }

    Result Create(const PipelineLayoutDesc& pipelineDesc);
    void SetRootConstants(ID3D11DeviceContextBest* deferredContext, uint32_t rootConstantIndex, const void* data, uint32_t size) const;
    void Bind(ID3D11DeviceContextBest* deferredContext);
    void BindDescriptorSet(BindingState& currentBindingState, ID3D11DeviceContextBest* deferredContext, uint32_t setIndex, const DescriptorSetD3D11* descriptorSet, const DescriptorD3D11* descriptor, const uint32_t* dynamicConstantBufferOffsets) const;

private:
    template <bool isGraphics>
    void BindDescriptorSetImpl(BindingState& currentBindingState, ID3D11DeviceContextBest* deferredContext, uint32_t setIndex, const DescriptorSetD3D11* descriptorSet, const DescriptorD3D11* descriptor, const uint32_t* dynamicConstantBufferOffsets) const;

    DeviceD3D11& m_Device;
    Vector<BindingSet> m_BindingSets;
    Vector<BindingRange> m_BindingRanges;
    Vector<ConstantBuffer> m_ConstantBuffers;
    uint32_t m_RootBindingOffset = 0;
    bool m_IsGraphicsPipelineLayout = false;
};

} // namespace nri
