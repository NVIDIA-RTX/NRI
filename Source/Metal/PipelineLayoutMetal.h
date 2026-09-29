// © 2026 NVIDIA Corporation
#pragma once

namespace nri {

// Argument-table slots shared by native and converted shaders (see "NRI.metal"). Converted slots match "kIR*BindPoint"
constexpr uint32_t ARGUMENT_SLOT_RESOURCE_HEAP = 0;
constexpr uint32_t ARGUMENT_SLOT_SAMPLER_HEAP = 1;
constexpr uint32_t ARGUMENT_SLOT_ROOT = 2;
constexpr uint32_t ARGUMENT_SLOT_MULTIVIEW = 3;      // native only
constexpr uint32_t ARGUMENT_SLOT_DRAW_ARGUMENTS = 4; // converted only
constexpr uint32_t ARGUMENT_SLOT_DRAW_UNIFORMS = 5;  // converted only
constexpr uint32_t ARGUMENT_SLOT_VERTEX_BUFFER_BASE = 6;

// Converted vertex shaders read stage-in attributes from "kIRStageInAttributeStartIndex + reflected attribute index"
constexpr uint32_t CONVERTED_VERTEX_ATTRIBUTE_BASE = 11;
constexpr uint32_t CONVERTED_VERTEX_ATTRIBUTE_NUM = 20;
static_assert(CONVERTED_VERTEX_ATTRIBUTE_BASE + CONVERTED_VERTEX_ATTRIBUTE_NUM == 31, "'MTLVertexDescriptor' has 31 attributes");

// Reserved DXIL register spaces
constexpr uint32_t DRAW_EMULATION_SPACE = 999;    // "NRI_BASE_ATTRIBUTES_EMULATION_SPACE" in "NRI.hlsl"
constexpr uint32_t FRAMEBUFFER_FETCH_SPACE = 998; // input attachments

// Descriptor table entry ("IRDescriptorTableEntry" / "NriDescriptorEntry")
struct DescriptorEntryMetal {
    uint64_t bufferAddress; // buffer GPU address or sampler resource ID
    uint64_t resourceId;    // texture resource ID
    uint64_t metadata;      // see "NRI.metal"
};

constexpr uint64_t DESCRIPTOR_ENTRY_SIZE = sizeof(DescriptorEntryMetal);

struct PipelineLayoutMetal final : public DebugNameBase {
    PipelineLayoutMetal(DeviceMetal& device);
    ~PipelineLayoutMetal();
    Result Create(const PipelineLayoutDesc& desc);
    DeviceMetal& GetDevice() const;
    uint64_t GetRootSignatureHash() const;
    uint32_t GetRootDataSize() const;
    uint32_t GetRootConstantOffset(uint32_t index) const;
    uint32_t GetRootDescriptorOffset(uint32_t index) const;
    uint32_t GetDrawParametersOffset() const;
    uint32_t GetDrawIndexOffset() const;
    bool IsDrawParametersEmulationEnabled() const;
    bool IsDrawIndexEmulationEnabled() const;
    const DescriptorSetMappingMetal& GetDescriptorSetMapping(uint32_t index) const;
    void InitRootData(void* data) const;
    void WriteSetPointers(void* data, uint32_t setIndex, const DescriptorSetMetal& set) const;
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    IRRootSignature* GetRootSignature() const;
#endif
    void SetDebugName(const char*) NRI_DEBUG_NAME_OVERRIDE {
    }

private:
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    IRVersionedRootSignatureDescriptor GetRootSignatureDesc() const;
#endif

    DeviceMetal& m_Device;
    Vector<DescriptorSetMappingMetal> m_Sets;
    Vector<uint32_t> m_ConstantOffsets;
    Vector<uint32_t> m_DescriptorOffsets;
    Vector<uint32_t> m_SetOffsets;
    Vector<DescriptorMetal*> m_RootSamplers;
    MTL::Buffer* m_RootSamplerBuffer = nullptr;
    uint32_t m_RootSamplerOffset = UINT32_MAX;
    uint32_t m_DrawParametersOffset = UINT32_MAX;
    uint32_t m_DrawIndexOffset = UINT32_MAX;
    uint32_t m_RootDataSize = 0;
    uint64_t m_RootSignatureHash = HashMetal(nullptr, 0);
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    Vector<IRRootParameter1> m_RootParameters; // descriptor tables point into "m_RootRanges"
    Vector<IRDescriptorRange1> m_RootRanges;
    IRRootSignature* m_RootSignature = nullptr;
    IRRootSignatureFlags m_RootSignatureFlags = IRRootSignatureFlagNone;
#endif
};

} // namespace nri
