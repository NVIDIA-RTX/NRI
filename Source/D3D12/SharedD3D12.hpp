// © 2021 NVIDIA Corporation

constexpr std::array<D3D12_COMMAND_LIST_TYPE, (size_t)QueueType::MAX_NUM> g_CommandListTypes = {
    D3D12_COMMAND_LIST_TYPE_DIRECT,  // GRAPHICS,
    D3D12_COMMAND_LIST_TYPE_COMPUTE, // COMPUTE,
    D3D12_COMMAND_LIST_TYPE_COPY,    // COPY,
};
VALIDATE_ARRAY(g_CommandListTypes);

D3D12_COMMAND_LIST_TYPE nri::GetCommandListType(QueueType queueType) {
    return g_CommandListTypes[(size_t)queueType];
}

constexpr std::array<D3D12_RESOURCE_DIMENSION, (size_t)TextureType::MAX_NUM> g_ResourceDimensions = {
    D3D12_RESOURCE_DIMENSION_TEXTURE1D, // TEXTURE_1D,
    D3D12_RESOURCE_DIMENSION_TEXTURE2D, // TEXTURE_2D,
    D3D12_RESOURCE_DIMENSION_TEXTURE3D, // TEXTURE_3D,
};
VALIDATE_ARRAY(g_ResourceDimensions);

D3D12_RESOURCE_DIMENSION nri::GetResourceDimension(TextureType textureType) {
    return g_ResourceDimensions[(size_t)textureType];
}

constexpr std::array<D3D12_DESCRIPTOR_RANGE_TYPE, (size_t)DescriptorType::MAX_NUM> g_DescriptorRangeTypes = {
    D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, // SAMPLER
    D3D12_DESCRIPTOR_RANGE_TYPE_CBV,     // CONSTANT_BUFFER
    D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     // TEXTURE
    D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // STORAGE_TEXTURE
    D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     // BUFFER
    D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // STORAGE_BUFFER
    D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     // STRUCTURED_BUFFER
    D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // STORAGE_STRUCTURED_BUFFER
    D3D12_DESCRIPTOR_RANGE_TYPE_SRV      // ACCELERATION_STRUCTURE
};

// VALIDATE_ARRAY(g_DescriptorRangeTypes); // TODO: 0 is expected for ACCELERATION_STRUCTURE

D3D12_DESCRIPTOR_RANGE_TYPE nri::GetDescriptorRangesType(DescriptorType descriptorType) {
    return g_DescriptorRangeTypes[(size_t)descriptorType];
}

constexpr std::array<D3D12_PRIMITIVE_TOPOLOGY_TYPE, (size_t)Topology::MAX_NUM> g_PrimitiveTopologyTypes = {
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,    // POINT_LIST
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,     // LINE_LIST
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,     // LINE_STRIP
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, // TRIANGLE_LIST
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, // TRIANGLE_STRIP
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,     // LINE_LIST_WITH_ADJACENCY
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,     // LINE_STRIP_WITH_ADJACENCY
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, // TRIANGLE_LIST_WITH_ADJACENCY
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, // TRIANGLE_STRIP_WITH_ADJACENCY
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH     // PATCH_LIST
};
VALIDATE_ARRAY(g_PrimitiveTopologyTypes);

D3D12_PRIMITIVE_TOPOLOGY_TYPE nri::GetPrimitiveTopologyType(Topology topology) {
    return g_PrimitiveTopologyTypes[(size_t)topology];
}

constexpr std::array<D3D_PRIMITIVE_TOPOLOGY, (size_t)Topology::MAX_NUM - 1> g_PrimitiveTopologies = {
    D3D_PRIMITIVE_TOPOLOGY_POINTLIST,        // POINT_LIST
    D3D_PRIMITIVE_TOPOLOGY_LINELIST,         // LINE_LIST
    D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,        // LINE_STRIP
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,     // TRIANGLE_LIST
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,    // TRIANGLE_STRIP
    D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ,     // LINE_LIST_WITH_ADJACENCY
    D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ,    // LINE_STRIP_WITH_ADJACENCY
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ, // TRIANGLE_LIST_WITH_ADJACENCY
    D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ // TRIANGLE_STRIP_WITH_ADJACENCY
};
VALIDATE_ARRAY(g_PrimitiveTopologyTypes);

D3D_PRIMITIVE_TOPOLOGY nri::GetPrimitiveTopology(Topology topology, uint8_t tessControlPointNum) {
    if (topology == Topology::PATCH_LIST)
        return (D3D_PRIMITIVE_TOPOLOGY)((uint8_t)D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ + tessControlPointNum);
    else
        return g_PrimitiveTopologies[(size_t)topology];
}

constexpr std::array<D3D12_FILL_MODE, (size_t)FillMode::MAX_NUM> g_FillModes = {
    D3D12_FILL_MODE_SOLID,    // SOLID
    D3D12_FILL_MODE_WIREFRAME // WIREFRAME
};
VALIDATE_ARRAY(g_FillModes);

D3D12_FILL_MODE nri::GetFillMode(FillMode fillMode) {
    return g_FillModes[(size_t)fillMode];
}

constexpr std::array<D3D12_CULL_MODE, (size_t)CullMode::MAX_NUM> g_CullModes = {
    D3D12_CULL_MODE_NONE,  // NONE
    D3D12_CULL_MODE_FRONT, // FRONT
    D3D12_CULL_MODE_BACK   // BACK
};
VALIDATE_ARRAY(g_CullModes);

D3D12_CULL_MODE nri::GetCullMode(CullMode cullMode) {
    return g_CullModes[(size_t)cullMode];
}

constexpr std::array<D3D12_COMPARISON_FUNC, (size_t)CompareOp::MAX_NUM> g_ComparisonFuncs = {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    D3D12_COMPARISON_FUNC_NONE, // NONE
#else
    D3D12_COMPARISON_FUNC(0), // NONE
#endif
    D3D12_COMPARISON_FUNC_ALWAYS,        // ALWAYS
    D3D12_COMPARISON_FUNC_NEVER,         // NEVER
    D3D12_COMPARISON_FUNC_EQUAL,         // EQUAL
    D3D12_COMPARISON_FUNC_NOT_EQUAL,     // NOT_EQUAL
    D3D12_COMPARISON_FUNC_LESS,          // LESS
    D3D12_COMPARISON_FUNC_LESS_EQUAL,    // LESS_EQUAL
    D3D12_COMPARISON_FUNC_GREATER,       // GREATER
    D3D12_COMPARISON_FUNC_GREATER_EQUAL, // GREATER_EQUAL
};
VALIDATE_ARRAY(g_ComparisonFuncs);

D3D12_COMPARISON_FUNC nri::GetCompareOp(CompareOp compareOp) {
    return g_ComparisonFuncs[(size_t)compareOp];
}

constexpr std::array<D3D12_STENCIL_OP, (size_t)StencilOp::MAX_NUM> g_StencilOps = {
    D3D12_STENCIL_OP_KEEP,     // KEEP
    D3D12_STENCIL_OP_ZERO,     // ZERO
    D3D12_STENCIL_OP_REPLACE,  // REPLACE
    D3D12_STENCIL_OP_INCR_SAT, // INCREMENT_AND_CLAMP
    D3D12_STENCIL_OP_DECR_SAT, // DECREMENT_AND_CLAMP
    D3D12_STENCIL_OP_INVERT,   // INVERT
    D3D12_STENCIL_OP_INCR,     // INCREMENT_AND_WRAP
    D3D12_STENCIL_OP_DECR      // DECREMENT_AND_WRAP
};
VALIDATE_ARRAY(g_StencilOps);

D3D12_STENCIL_OP nri::GetStencilOp(StencilOp stencilFunc) {
    return g_StencilOps[(size_t)stencilFunc];
}

constexpr std::array<D3D12_LOGIC_OP, (size_t)LogicOp::MAX_NUM> g_LogicOps = {
    D3D12_LOGIC_OP_NOOP,          // NONE
    D3D12_LOGIC_OP_CLEAR,         // CLEAR
    D3D12_LOGIC_OP_AND,           // AND
    D3D12_LOGIC_OP_AND_REVERSE,   // AND_REVERSE
    D3D12_LOGIC_OP_COPY,          // COPY
    D3D12_LOGIC_OP_AND_INVERTED,  // AND_INVERTED
    D3D12_LOGIC_OP_XOR,           // XOR
    D3D12_LOGIC_OP_OR,            // OR
    D3D12_LOGIC_OP_NOR,           // NOR
    D3D12_LOGIC_OP_EQUIV,         // EQUIVALENT
    D3D12_LOGIC_OP_INVERT,        // INVERT
    D3D12_LOGIC_OP_OR_REVERSE,    // OR_REVERSE
    D3D12_LOGIC_OP_COPY_INVERTED, // COPY_INVERTED
    D3D12_LOGIC_OP_OR_INVERTED,   // OR_INVERTED
    D3D12_LOGIC_OP_NAND,          // NAND
    D3D12_LOGIC_OP_SET            // SET
};
VALIDATE_ARRAY(g_LogicOps);

D3D12_LOGIC_OP nri::GetLogicOp(LogicOp logicOp) {
    return g_LogicOps[(size_t)logicOp];
}

constexpr std::array<D3D12_BLEND, (size_t)BlendFactor::MAX_NUM> g_BlendFactors = {
    D3D12_BLEND_ZERO,             // ZERO
    D3D12_BLEND_ONE,              // ONE
    D3D12_BLEND_SRC_COLOR,        // SRC_COLOR
    D3D12_BLEND_INV_SRC_COLOR,    // ONE_MINUS_SRC_COLOR
    D3D12_BLEND_DEST_COLOR,       // DST_COLOR
    D3D12_BLEND_INV_DEST_COLOR,   // ONE_MINUS_DST_COLOR
    D3D12_BLEND_SRC_ALPHA,        // SRC_ALPHA
    D3D12_BLEND_INV_SRC_ALPHA,    // ONE_MINUS_SRC_ALPHA
    D3D12_BLEND_DEST_ALPHA,       // DST_ALPHA
    D3D12_BLEND_INV_DEST_ALPHA,   // ONE_MINUS_DST_ALPHA
    D3D12_BLEND_BLEND_FACTOR,     // CONSTANT_COLOR
    D3D12_BLEND_INV_BLEND_FACTOR, // ONE_MINUS_CONSTANT_COLOR
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    D3D12_BLEND_ALPHA_FACTOR,     // CONSTANT_ALPHA
    D3D12_BLEND_INV_ALPHA_FACTOR, // ONE_MINUS_CONSTANT_ALPHA
#else
    // Non-exposed in old SDK
    D3D12_BLEND_BLEND_FACTOR,     // CONSTANT_ALPHA
    D3D12_BLEND_INV_BLEND_FACTOR, // ONE_MINUS_CONSTANT_ALPHA
#endif
    D3D12_BLEND_SRC_ALPHA_SAT,  // SRC_ALPHA_SATURATE
    D3D12_BLEND_SRC1_COLOR,     // SRC1_COLOR
    D3D12_BLEND_INV_SRC1_COLOR, // ONE_MINUS_SRC1_COLOR
    D3D12_BLEND_SRC1_ALPHA,     // SRC1_ALPHA
    D3D12_BLEND_INV_SRC1_ALPHA  // ONE_MINUS_SRC1_ALPHA
};
VALIDATE_ARRAY(g_BlendFactors);

D3D12_BLEND nri::GetBlend(BlendFactor blendFactor) {
    return g_BlendFactors[(size_t)blendFactor];
}

constexpr std::array<D3D12_BLEND_OP, (size_t)BlendOp::MAX_NUM> g_BlendOps = {
    D3D12_BLEND_OP_ADD,          // ADD
    D3D12_BLEND_OP_SUBTRACT,     // SUBTRACT
    D3D12_BLEND_OP_REV_SUBTRACT, // REVERSE_SUBTRACT
    D3D12_BLEND_OP_MIN,          // MIN
    D3D12_BLEND_OP_MAX           // MAX
};
VALIDATE_ARRAY(g_BlendOps);

D3D12_BLEND_OP nri::GetBlendOp(BlendOp blendFunc) {
    return g_BlendOps[(size_t)blendFunc];
}

D3D12_TEXTURE_ADDRESS_MODE nri::GetAddressMode(AddressMode addressMode) {
    return (D3D12_TEXTURE_ADDRESS_MODE)(D3D12_TEXTURE_ADDRESS_MODE_WRAP + (uint32_t)addressMode);
}

constexpr std::array<D3D12_SHADING_RATE, (size_t)ShadingRate::MAX_NUM> g_ShadingRates = {
    D3D12_SHADING_RATE_1X1, // FRAGMENT_SIZE_1X1,
    D3D12_SHADING_RATE_1X2, // FRAGMENT_SIZE_1X2,
    D3D12_SHADING_RATE_2X1, // FRAGMENT_SIZE_2X1,
    D3D12_SHADING_RATE_2X2, // FRAGMENT_SIZE_2X2,
    D3D12_SHADING_RATE_2X4, // FRAGMENT_SIZE_2X4,
    D3D12_SHADING_RATE_4X2, // FRAGMENT_SIZE_4X2,
    D3D12_SHADING_RATE_4X4, // FRAGMENT_SIZE_4X4
};
VALIDATE_ARRAY(g_ShadingRates);

D3D12_SHADING_RATE nri::GetShadingRate(ShadingRate shadingRate) {
    return g_ShadingRates[(size_t)shadingRate];
}

constexpr std::array<D3D12_SHADING_RATE_COMBINER, (size_t)ShadingRateCombiner::MAX_NUM> g_ShadingRateCombiners = {
    D3D12_SHADING_RATE_COMBINER_PASSTHROUGH, // KEEP,
    D3D12_SHADING_RATE_COMBINER_OVERRIDE,    // REPLACE,
    D3D12_SHADING_RATE_COMBINER_MIN,         // MIN,
    D3D12_SHADING_RATE_COMBINER_MAX,         // MAX,
    D3D12_SHADING_RATE_COMBINER_SUM,         // SUM,
};
VALIDATE_ARRAY(g_ShadingRateCombiners);

D3D12_SHADING_RATE_COMBINER nri::GetShadingRateCombiner(ShadingRateCombiner shadingRateCombiner) {
    return g_ShadingRateCombiners[(size_t)shadingRateCombiner];
}

D3D12_FILTER nri::GetFilter(const SamplerDesc& samplerDesc) {
    bool anisotropy = samplerDesc.anisotropy > 1 ? true : false;
    bool comparison = samplerDesc.compareOp != CompareOp::NONE;

    D3D12_FILTER_REDUCTION_TYPE reductionType = D3D12_FILTER_REDUCTION_TYPE_STANDARD;
    if (samplerDesc.filters.ext == ReductionMode::MIN)
        reductionType = D3D12_FILTER_REDUCTION_TYPE_MINIMUM;
    else if (samplerDesc.filters.ext == ReductionMode::MAX)
        reductionType = D3D12_FILTER_REDUCTION_TYPE_MAXIMUM;
    reductionType = comparison ? D3D12_FILTER_REDUCTION_TYPE_COMPARISON : reductionType;

    D3D12_FILTER_TYPE min = (D3D12_FILTER_TYPE)samplerDesc.filters.min;
    D3D12_FILTER_TYPE mag = (D3D12_FILTER_TYPE)samplerDesc.filters.mag;
    if (anisotropy) {
        min = D3D12_FILTER_TYPE_LINEAR;
        mag = D3D12_FILTER_TYPE_LINEAR;
    }

    uint32_t filter = D3D12_ENCODE_BASIC_FILTER(min, mag, (D3D12_FILTER_TYPE)samplerDesc.filters.mip, reductionType);

    if (anisotropy)
        filter |= D3D12_ANISOTROPIC_FILTERING_BIT;

    return (D3D12_FILTER)filter;
}

UINT8 nri::GetRenderTargetWriteMask(ColorWriteBits colorWriteMask) {
    return colorWriteMask & ColorWriteBits::RGBA;
}

D3D12_DESCRIPTOR_HEAP_TYPE nri::GetDescriptorHeapType(DescriptorType descriptorType) {
    return descriptorType == DescriptorType::SAMPLER ? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER : D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE nri::GetAccelerationStructureType(AccelerationStructureType accelerationStructureType) {
    static_assert(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL == (uint32_t)AccelerationStructureType::TOP_LEVEL, "Enum mismatch");
    static_assert(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL == (uint32_t)AccelerationStructureType::BOTTOM_LEVEL, "Enum mismatch");

    return (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE)accelerationStructureType;
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS nri::GetAccelerationStructureFlags(AccelerationStructureBits accelerationStructureBits) {
    uint32_t flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    if (accelerationStructureBits & AccelerationStructureBits::ALLOW_UPDATE)
        flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

    if (accelerationStructureBits & AccelerationStructureBits::ALLOW_COMPACTION)
        flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;

#if NRI_ENABLE_NVAPI
    if (accelerationStructureBits & AccelerationStructureBits::ALLOW_DATA_ACCESS)
        flags |= NVAPI_D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_DATA_ACCESS_EX;

    if (accelerationStructureBits & AccelerationStructureBits::ALLOW_MICROMAP_UPDATE)
        flags |= NVAPI_D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_OMM_UPDATE_EX;

    if (accelerationStructureBits & AccelerationStructureBits::ALLOW_DISABLE_MICROMAPS)
        flags |= NVAPI_D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_DISABLE_OMMS_EX;
#endif

    if (accelerationStructureBits & AccelerationStructureBits::PREFER_FAST_TRACE)
        flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    if (accelerationStructureBits & AccelerationStructureBits::PREFER_FAST_BUILD)
        flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;

    if (accelerationStructureBits & AccelerationStructureBits::MINIMIZE_MEMORY)
        flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;

    return (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS)flags;
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS nri::GetMicromapFlags(MicromapBits micromapBits) {
    uint32_t flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    if (micromapBits & MicromapBits::ALLOW_COMPACTION)
        flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;

    if (micromapBits & MicromapBits::PREFER_FAST_TRACE)
        flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    if (micromapBits & MicromapBits::PREFER_FAST_BUILD)
        flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;

    return (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS)flags;
}

D3D12_RAYTRACING_GEOMETRY_TYPE nri::GetGeometryType(BottomLevelGeometryType geometryType) {
    static_assert(D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES == (uint32_t)BottomLevelGeometryType::TRIANGLES, "Enum mismatch");
    static_assert(D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS == (uint32_t)BottomLevelGeometryType::AABBS, "Enum mismatch");

    return (D3D12_RAYTRACING_GEOMETRY_TYPE)geometryType;
}

D3D12_RAYTRACING_GEOMETRY_FLAGS nri::GetGeometryFlags(BottomLevelGeometryBits bottomLevelGeometryBits) {
    static_assert(D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE == (uint32_t)BottomLevelGeometryBits::OPAQUE_GEOMETRY, "Enum mismatch");
    static_assert(D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION == (uint32_t)BottomLevelGeometryBits::NO_DUPLICATE_ANY_HIT_INVOCATION, "Enum mismatch");

    return (D3D12_RAYTRACING_GEOMETRY_FLAGS)bottomLevelGeometryBits;
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE nri::GetCopyMode(CopyMode copyMode) {
    static_assert(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE == (uint32_t)CopyMode::CLONE, "Enum mismatch");
    static_assert(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT == (uint32_t)CopyMode::COMPACT, "Enum mismatch");

    return (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE)copyMode;
}

uint64_t nri::GetMemorySizeD3D12(const MemoryD3D12Desc& memoryD3D12Desc) {
    return memoryD3D12Desc.d3d12Heap->GetDesc().SizeInBytes;
}

D3D12_RESIDENCY_PRIORITY nri::ConvertPriority(float priority) {
    if (priority == 0.0f)
        return (D3D12_RESIDENCY_PRIORITY)0;

    float p = priority * 0.5f + 0.5f;
    float level = 0.0f;

    uint32_t result = 0;
    if (p < 0.2f) {
        result = (uint32_t)D3D12_RESIDENCY_PRIORITY_MINIMUM;
        level = 0.0f;
    } else if (p < 0.4f) {
        result = (uint32_t)D3D12_RESIDENCY_PRIORITY_LOW;
        level = 0.2f;
    } else if (p < 0.6f) {
        result = (uint32_t)D3D12_RESIDENCY_PRIORITY_NORMAL;
        level = 0.4f;
    } else if (p < 0.8f) {
        result = (uint32_t)D3D12_RESIDENCY_PRIORITY_HIGH;
        level = 0.6f;
    } else {
        result = (uint32_t)D3D12_RESIDENCY_PRIORITY_MAXIMUM;
        level = 0.8f;
    }

    uint32_t bonus = uint32_t(((p - level) / 0.2f) * 65535.0f);
    if (bonus > 0xFFFF)
        bonus = 0xFFFF;

    result |= bonus;

    return (D3D12_RESIDENCY_PRIORITY)result;
}

bool nri::GetTextureDesc(const TextureD3D12Desc& textureD3D12Desc, TextureDesc& textureDesc) {
    textureDesc = {};

    ID3D12Resource* resource = textureD3D12Desc.d3d12Resource;
    if (!resource)
        return false;

    D3D12_RESOURCE_DESC desc = resource->GetDesc();
    if (desc.Dimension < D3D12_RESOURCE_DIMENSION_TEXTURE1D)
        return false;

    textureDesc.type = (TextureType)(desc.Dimension - D3D12_RESOURCE_DIMENSION_TEXTURE1D);
    textureDesc.format = DXGIFormatToNRIFormat(desc.Format);
    textureDesc.width = (Dim_t)desc.Width;
    textureDesc.height = (Dim_t)desc.Height;
    textureDesc.depth = textureDesc.type == TextureType::TEXTURE_3D ? (Dim_t)desc.DepthOrArraySize : 1;
    textureDesc.mipNum = (Dim_t)desc.MipLevels;
    textureDesc.layerNum = textureDesc.type == TextureType::TEXTURE_3D ? 1 : (Dim_t)desc.DepthOrArraySize;
    textureDesc.sampleNum = (uint8_t)desc.SampleDesc.Count;

    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
        textureDesc.usage |= TextureUsageBits::COLOR_ATTACHMENT;
    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
        textureDesc.usage |= TextureUsageBits::DEPTH_STENCIL_ATTACHMENT;
    if (!(desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
        textureDesc.usage |= TextureUsageBits::SHADER_RESOURCE;
    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
        textureDesc.usage |= TextureUsageBits::SHADER_RESOURCE_STORAGE;

    if (textureD3D12Desc.format)
        textureDesc.format = DXGIFormatToNRIFormat(textureD3D12Desc.format);

    return true;
}

bool nri::GetBufferDesc(const BufferD3D12Desc& bufferD3D12Desc, BufferDesc& bufferDesc) {
    bufferDesc = {};

    ID3D12Resource* resource = bufferD3D12Desc.d3d12Resource;
    if (!resource)
        return false;

    D3D12_RESOURCE_DESC desc = resource->GetDesc();
    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
        return false;

    bufferDesc.size = desc.Width;
    bufferDesc.structureStride = bufferD3D12Desc.structureStride;

    // There are almost no restrictions on usages in D3D12
    bufferDesc.usage = BufferUsageBits::VERTEX_BUFFER | BufferUsageBits::INDEX_BUFFER | BufferUsageBits::CONSTANT_BUFFER | BufferUsageBits::ARGUMENT_BUFFER | BufferUsageBits::ACCELERATION_STRUCTURE_BUILD_INPUT;

    if (!(desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
        bufferDesc.usage |= BufferUsageBits::SHADER_RESOURCE;
    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
        bufferDesc.usage |= BufferUsageBits::SHADER_RESOURCE_STORAGE;

    return true;
}

void nri::ConvertBotomLevelGeometries(const BottomLevelGeometryDesc* geometries, uint32_t geometryNum,
    D3D12_RAYTRACING_GEOMETRY_DESC* geometryDescs,
    D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC* triangleDescs,
    D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC* micromapDescs) {
    MaybeUnused(triangleDescs, micromapDescs);

    for (uint32_t i = 0; i < geometryNum; i++) {
        const BottomLevelGeometryDesc& in = geometries[i];
        D3D12_RAYTRACING_GEOMETRY_DESC& out = geometryDescs[i];

        out = {};
        out.Type = GetGeometryType(in.type);
        out.Flags = GetGeometryFlags(in.flags);

        if (out.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
            const BottomLevelTrianglesDesc& triangles = in.triangles;
            D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC* outTriangles = &out.Triangles;

#ifdef NRI_D3D12_HAS_OPACITY_MICROMAP
            if (in.triangles.micromap) {
                const BottomLevelMicromapDesc& micromapDesc = *in.triangles.micromap;

                outTriangles = triangleDescs++;
                D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC* outMicromap = micromapDescs++;

                out.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES;
                out.OmmTriangles.pTriangles = outTriangles;
                out.OmmTriangles.pOmmLinkage = outMicromap;

                *outMicromap = {};
                outMicromap->OpacityMicromapBaseLocation = micromapDesc.baseTriangle;

                if (micromapDesc.micromap)
                    outMicromap->OpacityMicromapArray = ((MicromapD3D12*)micromapDesc.micromap)->GetHandle();

                if (micromapDesc.indexBuffer) {
                    outMicromap->OpacityMicromapIndexBuffer.StartAddress = GetBufferAddress(micromapDesc.indexBuffer, micromapDesc.indexOffset);
                    outMicromap->OpacityMicromapIndexBuffer.StrideInBytes = micromapDesc.indexType == IndexType::UINT16 ? sizeof(uint16_t) : sizeof(uint32_t);
                    outMicromap->OpacityMicromapIndexFormat = micromapDesc.indexType == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
                }
            }
#endif

            *outTriangles = {};
            outTriangles->VertexFormat = GetDxgiFormat(triangles.vertexFormat).typed;
            outTriangles->VertexCount = triangles.vertexNum;
            outTriangles->VertexBuffer.StrideInBytes = triangles.vertexStride;
            outTriangles->VertexBuffer.StartAddress = GetBufferAddress(triangles.vertexBuffer, triangles.vertexOffset);
            outTriangles->Transform3x4 = GetBufferAddress(triangles.transformBuffer, triangles.transformOffset);

            if (triangles.indexBuffer) {
                outTriangles->IndexFormat = triangles.indexType == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
                outTriangles->IndexCount = triangles.indexNum;
                outTriangles->IndexBuffer = GetBufferAddress(triangles.indexBuffer, triangles.indexOffset);
            }
        } else if (out.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
            const BottomLevelAabbsDesc& aabbs = in.aabbs;
            D3D12_RAYTRACING_GEOMETRY_AABBS_DESC* outAABBs = &out.AABBs;

            *outAABBs = {};
            outAABBs->AABBCount = aabbs.num;
            outAABBs->AABBs.StrideInBytes = aabbs.stride;
            outAABBs->AABBs.StartAddress = GetBufferAddress(aabbs.buffer, aabbs.offset);
        }
    }
}
