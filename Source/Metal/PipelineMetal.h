// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

// Converted shader, a private format for "PipelineCacheMetal" entries and Metal converter bundles (little-endian): the header, then data at offsets
// from the start (within "size"): 4-byte aligned "ConvertedVertexInputMetal" array, null-terminated strings and the 8-byte aligned metallib
constexpr uint32_t CONVERTED_SHADER_MAGIC = 0x5343524E; // "NRCS"
constexpr uint32_t CONVERTED_SHADER_VERSION = 1;
constexpr uint32_t CONVERTED_SHADER_SAMPLER_LOD_BIAS = 1 << 0;
constexpr uint32_t CONVERTED_SHADER_DUAL_SOURCE_BLENDING = 1 << 1;

struct ConvertedVertexInputMetal {
    uint32_t nameOffset;     // lower-case semantic name and index, i.e. "texcoord1" (reflection "vertex_inputs[].name")
    uint32_t attributeIndex; // reflection "vertex_inputs[].index", less than "CONVERTED_VERTEX_ATTRIBUTE_NUM"
};

struct ConvertedShaderHeaderMetal {
    uint32_t magic;
    uint32_t version;
    uint64_t rootSignatureHash;
    uint32_t size;
    StageBits stage;
    uint32_t flags; // "CONVERTED_SHADER_*"
    uint32_t gpuFamily;
    uint32_t inputTopology;
    uint32_t sampleMask;
    uint32_t threadGroupSize[3];
    uint32_t payloadSize;
    uint32_t entryPointOffset;
    uint32_t functionNameOffset;
    uint32_t vertexInputOffset;
    uint32_t vertexInputNum;
    uint32_t metallibOffset;
    uint32_t metallibSize;
};

struct ShaderLoadDescMetal {
    PipelineCacheMetal* cache = nullptr; // converted shaders are looked up and added
    const VertexInputDesc* vertexInput = nullptr;
    uint8_t* vertexAttributeSlots = nullptr; // converted vertex shaders: receives Metal attribute indices for "vertexInput->attributes"
    MTL::Library** stageInLibrary = nullptr;
    uint32_t sampleMask = ALL;
    Topology topology = Topology::MAX_NUM; // "MAX_NUM" - not a primitive pipeline
    bool emulation = false;
    bool dualSourceBlending = false;
    bool failOnCacheMiss = false;
};

struct PipelineMetal final : public DebugNameBase {
    PipelineMetal(DeviceMetal& device);
    ~PipelineMetal();
    Result Create(const GraphicsPipelineDesc& desc);
    Result Create(const ComputePipelineDesc& desc);
    Result Create(const RayTracingPipelineDesc& desc);
    Result WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, uint32_t dstStride, void* dst) const;
    DeviceMetal& GetDevice() const;
    MTL::RenderPipelineState* GetRenderPipeline() const;
    MTL::ComputePipelineState* GetComputePipeline() const;
    MTL::ResourceID GetVisibleFunctionTableResourceID() const;
    MTL::ResourceID GetIntersectionFunctionTableResourceID() const;
    MTL::DepthStencilState* GetDepthStencilState() const;
    MTL::PrimitiveType GetPrimitiveType() const;
    MTL::CullMode GetCullMode() const;
    MTL::Winding GetWinding() const;
    MTL::TriangleFillMode GetFillMode() const;
    MTL::DepthClipMode GetDepthClipMode() const;
    bool IsDepthBoundsEnabled() const;
    bool HasSampleLocations() const;

    Multiview GetMultiview() const;
    uint32_t GetViewMask() const;

    const DepthBiasDesc& GetDepthBias() const;
    MTL::Size GetThreadGroupSize() const;
    MTL::Size GetMeshThreadGroupSize() const;
    MTL::Size GetTaskThreadGroupSize() const;
    bool IsConverted() const;
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    bool IsGeometryEmulation() const;
    bool IsTessellationEmulation() const;
    const IRRuntimeGeometryPipelineConfig& GetGeometryConfig() const;
    const IRRuntimeTessellationPipelineConfig& GetTessellationConfig() const;
    IRRuntimePrimitiveType GetEmulationPrimitive() const;
#endif
    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    Result LoadFunction(const ShaderDesc& shader, MTL::Library*& library, MTL4::FunctionDescriptor*& function, const ShaderLoadDescMetal& loadDesc);
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    Result ConvertShader(const ShaderDesc& shader, const ShaderLoadDescMetal& loadDesc, Vector<uint8_t>& storage, const uint8_t*& container);
#endif

    DeviceMetal& m_Device;
    const PipelineLayoutMetal* m_Layout = nullptr;
    MTL::RenderPipelineState* m_Render = nullptr;
    MTL::ComputePipelineState* m_Compute = nullptr;
    MTL::VisibleFunctionTable* m_VisibleFunctionTable = nullptr;
    MTL::IntersectionFunctionTable* m_IntersectionFunctionTable = nullptr;
    Vector<uint8_t> m_ShaderGroupIdentifiers;
    MTL::DepthStencilState* m_DepthStencil = nullptr;
    MTL::PrimitiveType m_Primitive = MTL::PrimitiveTypeTriangle;
    MTL::CullMode m_Cull = MTL::CullModeNone;
    MTL::Winding m_Winding = MTL::WindingClockwise;
    MTL::TriangleFillMode m_Fill = MTL::TriangleFillModeFill;
    MTL::DepthClipMode m_DepthClip = MTL::DepthClipModeClip;
    bool m_DepthBounds = false;
    bool m_SampleLocations = false;
    Multiview m_Multiview = Multiview::FLEXIBLE;
    uint32_t m_ViewMask = 0;
    DepthBiasDesc m_DepthBias = {};
    MTL::Size m_ThreadGroup = MTL::Size(1, 1, 1), m_MeshGroup = MTL::Size(1, 1, 1), m_TaskGroup = MTL::Size(1, 1, 1);
    uint32_t m_MeshPayloadSize = 0;
    bool m_Converted = false;
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    IRRuntimeGeometryPipelineConfig m_GeometryConfig = {};
    IRRuntimeTessellationPipelineConfig m_TessellationConfig = {};
    IRRuntimePrimitiveType m_EmulationPrimitive = IRRuntimePrimitiveTypeTriangle;
    bool m_GeometryEmulation = false;
    bool m_TessellationEmulation = false;
#endif
};

} // namespace nri
