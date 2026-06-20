// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct RootSamplerMappingWGPU {
    WGPUSampler sampler = nullptr;
    uint32_t binding = 0;
};

struct RootDescriptorMappingWGPU {
    DescriptorType type = DescriptorType::CONSTANT_BUFFER;
    uint32_t binding = 0;
    uint32_t dynamicOffsetIndex = uint32_t(-1);
};

struct PipelineLayoutWGPU final : public DebugNameBase {
    inline PipelineLayoutWGPU(DeviceWGPU& device)
        : m_Device(device)
        , m_SetMappings(device.GetStdAllocator())
        , m_BindGroupLayouts(device.GetStdAllocator())
        , m_RootSamplers(device.GetStdAllocator())
        , m_RootDescriptors(device.GetStdAllocator())
        , m_RootConstantOffsets(device.GetStdAllocator()) {
    }

    ~PipelineLayoutWGPU();

    inline operator WGPUPipelineLayout() const {
        return m_PipelineLayout;
    }

    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    inline uint32_t GetImmediateDataSize() const {
        return m_ImmediateDataSize;
    }

    const DescriptorSetMappingWGPU& GetDescriptorSetMapping(uint32_t setIndex) const;

    WGPUBindGroup GetRootSamplerBindGroup() const {
        return m_RootSamplerBindGroup;
    }

    uint32_t GetRootSamplerGroupIndex() const {
        return m_RootSamplerGroupIndex;
    }

    WGPUBindGroupLayout GetRootBindGroupLayout() const {
        return m_RootSamplerLayout;
    }

    const Vector<RootSamplerMappingWGPU>& GetRootSamplers() const {
        return m_RootSamplers;
    }

    const Vector<RootDescriptorMappingWGPU>& GetRootDescriptors() const {
        return m_RootDescriptors;
    }

    const RootDescriptorMappingWGPU& GetRootDescriptorMapping(uint32_t rootDescriptorIndex) const {
        return m_RootDescriptors[rootDescriptorIndex];
    }

    uint32_t GetRootDynamicOffsetNum() const {
        return m_RootDynamicOffsetNum;
    }

    uint32_t GetRootConstantOffset(uint32_t rootConstantIndex) const {
        return rootConstantIndex < m_RootConstantOffsets.size() ? m_RootConstantOffsets[rootConstantIndex] : 0;
    }

    Result Create(const PipelineLayoutDesc& pipelineLayoutDesc);
    Result UpdateStorageTextureFormats(const ShaderDesc* shaderDescs, uint32_t shaderDescNum);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        MaybeUnused(name);
    }

private:
    DeviceWGPU& m_Device;
    Vector<DescriptorSetMappingWGPU> m_SetMappings;
    Vector<WGPUBindGroupLayout> m_BindGroupLayouts;
    Vector<RootSamplerMappingWGPU> m_RootSamplers;
    Vector<RootDescriptorMappingWGPU> m_RootDescriptors;
    Vector<uint32_t> m_RootConstantOffsets;
    WGPUPipelineLayout m_PipelineLayout = nullptr;
    WGPUBindGroupLayout m_RootSamplerLayout = nullptr;
    WGPUBindGroup m_RootSamplerBindGroup = nullptr;
    uint32_t m_RootSamplerGroupIndex = uint32_t(-1);
    uint32_t m_RootDynamicOffsetNum = 0;
    uint32_t m_ImmediateDataSize = 0;
    bool m_IsFinalized = false;
};

} // namespace nri
