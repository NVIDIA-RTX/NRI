// © 2021 NVIDIA Corporation

constexpr std::array<D3D12_COMMAND_LIST_TYPE, (size_t)QueueType::MAX_NUM> COMMAND_LIST_TYPES = {
    D3D12_COMMAND_LIST_TYPE_DIRECT,  // GRAPHICS,
    D3D12_COMMAND_LIST_TYPE_COMPUTE, // COMPUTE,
    D3D12_COMMAND_LIST_TYPE_COPY,    // COPY,
};

D3D12_COMMAND_LIST_TYPE nri::GetCommandListType(QueueType queueType) {
    return COMMAND_LIST_TYPES[(size_t)queueType];
}

constexpr std::array<D3D12_RESOURCE_DIMENSION, (size_t)TextureType::MAX_NUM> RESOURCE_DIMENSIONS = {
    D3D12_RESOURCE_DIMENSION_TEXTURE1D, // TEXTURE_1D,
    D3D12_RESOURCE_DIMENSION_TEXTURE2D, // TEXTURE_2D,
    D3D12_RESOURCE_DIMENSION_TEXTURE3D, // TEXTURE_3D,
};

D3D12_RESOURCE_DIMENSION nri::GetResourceDimension(TextureType textureType) {
    return RESOURCE_DIMENSIONS[(size_t)textureType];
}

constexpr std::array<D3D12_DESCRIPTOR_RANGE_TYPE, (size_t)DescriptorType::MAX_NUM> DESCRIPTOR_RANGE_TYPES = {
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

D3D12_DESCRIPTOR_RANGE_TYPE nri::GetDescriptorRangesType(DescriptorType descriptorType) {
    return DESCRIPTOR_RANGE_TYPES[(size_t)descriptorType];
}

constexpr std::array<D3D12_PRIMITIVE_TOPOLOGY_TYPE, (size_t)Topology::MAX_NUM> PRIMITIVE_TOPOLOGY_TYPES = {
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

D3D12_PRIMITIVE_TOPOLOGY_TYPE nri::GetPrimitiveTopologyType(Topology topology) {
    return PRIMITIVE_TOPOLOGY_TYPES[(size_t)topology];
}

constexpr std::array<D3D_PRIMITIVE_TOPOLOGY, 9> PRIMITIVE_TOPOLOGIES = {
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

D3D_PRIMITIVE_TOPOLOGY nri::GetPrimitiveTopology(Topology topology, uint8_t tessControlPointNum) {
    if (topology == Topology::PATCH_LIST)
        return (D3D_PRIMITIVE_TOPOLOGY)((uint8_t)D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ + tessControlPointNum);
    else
        return PRIMITIVE_TOPOLOGIES[(size_t)topology];
}

constexpr std::array<D3D12_FILL_MODE, (size_t)FillMode::MAX_NUM> FILL_MODES = {
    D3D12_FILL_MODE_SOLID,    // SOLID
    D3D12_FILL_MODE_WIREFRAME // WIREFRAME
};

D3D12_FILL_MODE nri::GetFillMode(FillMode fillMode) {
    return FILL_MODES[(size_t)fillMode];
}

constexpr std::array<D3D12_CULL_MODE, (size_t)CullMode::MAX_NUM> CULL_MODES = {
    D3D12_CULL_MODE_NONE,  // NONE
    D3D12_CULL_MODE_FRONT, // FRONT
    D3D12_CULL_MODE_BACK   // BACK
};

D3D12_CULL_MODE nri::GetCullMode(CullMode cullMode) {
    return CULL_MODES[(size_t)cullMode];
}

constexpr std::array<D3D12_COMPARISON_FUNC, (size_t)CompareFunc::MAX_NUM> COMPARISON_FUNCS = {
#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
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

D3D12_COMPARISON_FUNC nri::GetComparisonFunc(CompareFunc compareFunc) {
    return COMPARISON_FUNCS[(size_t)compareFunc];
}

constexpr std::array<D3D12_STENCIL_OP, (size_t)StencilFunc::MAX_NUM> STENCIL_OPS = {
    D3D12_STENCIL_OP_KEEP,     // KEEP
    D3D12_STENCIL_OP_ZERO,     // ZERO
    D3D12_STENCIL_OP_REPLACE,  // REPLACE
    D3D12_STENCIL_OP_INCR_SAT, // INCREMENT_AND_CLAMP
    D3D12_STENCIL_OP_DECR_SAT, // DECREMENT_AND_CLAMP
    D3D12_STENCIL_OP_INVERT,   // INVERT
    D3D12_STENCIL_OP_INCR,     // INCREMENT_AND_WRAP
    D3D12_STENCIL_OP_DECR      // DECREMENT_AND_WRAP
};

D3D12_STENCIL_OP nri::GetStencilOp(StencilFunc stencilFunc) {
    return STENCIL_OPS[(size_t)stencilFunc];
}

constexpr std::array<D3D12_LOGIC_OP, (size_t)LogicFunc::MAX_NUM> LOGIC_OPS = {
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

D3D12_LOGIC_OP nri::GetLogicOp(LogicFunc logicFunc) {
    return LOGIC_OPS[(size_t)logicFunc];
}

constexpr std::array<D3D12_BLEND, (size_t)BlendFactor::MAX_NUM> BLENDS = {
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
#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
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

D3D12_BLEND nri::GetBlend(BlendFactor blendFactor) {
    return BLENDS[(size_t)blendFactor];
}

constexpr std::array<D3D12_BLEND_OP, (size_t)BlendFunc::MAX_NUM> BLEND_OPS = {
    D3D12_BLEND_OP_ADD,          // ADD
    D3D12_BLEND_OP_SUBTRACT,     // SUBTRACT
    D3D12_BLEND_OP_REV_SUBTRACT, // REVERSE_SUBTRACT
    D3D12_BLEND_OP_MIN,          // MIN
    D3D12_BLEND_OP_MAX           // MAX
};

D3D12_BLEND_OP nri::GetBlendOp(BlendFunc blendFunc) {
    return BLEND_OPS[(size_t)blendFunc];
}

D3D12_TEXTURE_ADDRESS_MODE nri::GetAddressMode(AddressMode addressMode) {
    return (D3D12_TEXTURE_ADDRESS_MODE)(D3D12_TEXTURE_ADDRESS_MODE_WRAP + (uint32_t)addressMode);
}

constexpr std::array<D3D12_HEAP_TYPE, (size_t)MemoryLocation::MAX_NUM> HEAP_TYPES = {
    D3D12_HEAP_TYPE_DEFAULT, // DEVICE
#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
    D3D12_HEAP_TYPE_GPU_UPLOAD, // DEVICE_UPLOAD (Prerequisite: D3D12_FEATURE_D3D12_OPTIONS16)
#else
    D3D12_HEAP_TYPE_UPLOAD, // DEVICE_UPLOAD (silent fallback to HOST_UPLOAD)
#endif
    D3D12_HEAP_TYPE_UPLOAD,   // HOST_UPLOAD
    D3D12_HEAP_TYPE_READBACK, // HOST_READBACK
};

D3D12_HEAP_TYPE nri::GetHeapType(MemoryLocation memoryLocation) {
    return HEAP_TYPES[(size_t)memoryLocation];
}

constexpr std::array<D3D12_SHADING_RATE, (size_t)ShadingRate::MAX_NUM> SHADING_RATES = {
    D3D12_SHADING_RATE_1X1, // FRAGMENT_SIZE_1X1,
    D3D12_SHADING_RATE_1X2, // FRAGMENT_SIZE_1X2,
    D3D12_SHADING_RATE_2X1, // FRAGMENT_SIZE_2X1,
    D3D12_SHADING_RATE_2X2, // FRAGMENT_SIZE_2X2,
    D3D12_SHADING_RATE_2X4, // FRAGMENT_SIZE_2X4,
    D3D12_SHADING_RATE_4X2, // FRAGMENT_SIZE_4X2,
    D3D12_SHADING_RATE_4X4, // FRAGMENT_SIZE_4X4
};

D3D12_SHADING_RATE nri::GetShadingRate(ShadingRate shadingRate) {
    return SHADING_RATES[(size_t)shadingRate];
}

constexpr std::array<D3D12_SHADING_RATE_COMBINER, (size_t)ShadingRate::MAX_NUM> SHADING_RATE_COMBINERS = {
    D3D12_SHADING_RATE_COMBINER_OVERRIDE,    // REPLACE,
    D3D12_SHADING_RATE_COMBINER_PASSTHROUGH, // KEEP,
    D3D12_SHADING_RATE_COMBINER_MIN,         // MIN,
    D3D12_SHADING_RATE_COMBINER_MAX,         // MAX,
    D3D12_SHADING_RATE_COMBINER_SUM,         // SUM,
};

D3D12_SHADING_RATE_COMBINER nri::GetShadingRateCombiner(ShadingRateCombiner shadingRateCombiner) {
    return SHADING_RATE_COMBINERS[(size_t)shadingRateCombiner];
}

UINT8 nri::GetRenderTargetWriteMask(ColorWriteBits colorWriteMask) {
    return colorWriteMask & ColorWriteBits::RGBA;
}

D3D12_DESCRIPTOR_HEAP_TYPE nri::GetDescriptorHeapType(DescriptorType descriptorType) {
    return descriptorType == DescriptorType::SAMPLER ? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER : D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
}

D3D12_HEAP_FLAGS nri::GetHeapFlags(MemoryType memoryType) {
    return (D3D12_HEAP_FLAGS)(memoryType & 0xffff);
}

D3D12_RESOURCE_FLAGS nri::GetBufferFlags(BufferUsageBits bufferUsage) {
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    if (bufferUsage & (BufferUsageBits::SHADER_RESOURCE_STORAGE | BufferUsageBits::SCRATCH_BUFFER))
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    if (bufferUsage & BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE)
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE;

    return flags;
}

D3D12_FILTER nri::GetFilterIsotropic(Filter mip, Filter magnification, Filter minification, FilterExt filterExt, bool useComparison) {
    uint32_t combinedMask = 0;
    combinedMask |= mip == Filter::LINEAR ? 0x1 : 0;
    combinedMask |= magnification == Filter::LINEAR ? 0x4 : 0;
    combinedMask |= minification == Filter::LINEAR ? 0x10 : 0;

    if (useComparison)
        combinedMask |= 0x80;
    else if (filterExt == FilterExt::MIN)
        combinedMask |= 0x100;
    else if (filterExt == FilterExt::MAX)
        combinedMask |= 0x180;

    return (D3D12_FILTER)combinedMask;
}

D3D12_FILTER nri::GetFilterAnisotropic(FilterExt filterExt, bool useComparison) {
    if (filterExt == FilterExt::MIN)
        return D3D12_FILTER_MINIMUM_ANISOTROPIC;
    else if (filterExt == FilterExt::MAX)
        return D3D12_FILTER_MAXIMUM_ANISOTROPIC;

    return useComparison ? D3D12_FILTER_COMPARISON_ANISOTROPIC : D3D12_FILTER_ANISOTROPIC;
}

D3D12_SHADER_VISIBILITY nri::GetShaderVisibility(StageBits shaderStages) {
    if (shaderStages == StageBits::ALL || shaderStages == StageBits::COMPUTE_SHADER || (shaderStages & StageBits::RAY_TRACING_SHADERS) != 0)
        return D3D12_SHADER_VISIBILITY_ALL;

    if (shaderStages == StageBits::VERTEX_SHADER)
        return D3D12_SHADER_VISIBILITY_VERTEX;

    if (shaderStages == StageBits::TESS_CONTROL_SHADER)
        return D3D12_SHADER_VISIBILITY_HULL;

    if (shaderStages == StageBits::TESS_EVALUATION_SHADER)
        return D3D12_SHADER_VISIBILITY_DOMAIN;

    if (shaderStages == StageBits::GEOMETRY_SHADER)
        return D3D12_SHADER_VISIBILITY_GEOMETRY;

    if (shaderStages == StageBits::FRAGMENT_SHADER)
        return D3D12_SHADER_VISIBILITY_PIXEL;

    if (shaderStages == StageBits::MESH_CONTROL_SHADER)
        return D3D12_SHADER_VISIBILITY_AMPLIFICATION;

    if (shaderStages == StageBits::MESH_EVALUATION_SHADER)
        return D3D12_SHADER_VISIBILITY_MESH;

    // Should be unreachable
    return D3D12_SHADER_VISIBILITY_ALL;
}

D3D12_RESOURCE_FLAGS nri::GetTextureFlags(TextureUsageBits textureUsage) {
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    if (textureUsage & TextureUsageBits::SHADER_RESOURCE_STORAGE)
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    if (textureUsage & TextureUsageBits::COLOR_ATTACHMENT)
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    if (textureUsage & TextureUsageBits::DEPTH_STENCIL_ATTACHMENT) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        if (!(textureUsage & TextureUsageBits::SHADER_RESOURCE))
            flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }

    return flags;
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE nri::GetAccelerationStructureType(AccelerationStructureType accelerationStructureType) {
    static_assert(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL == (uint32_t)AccelerationStructureType::TOP_LEVEL, "Enum mismatch");
    static_assert(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL == (uint32_t)AccelerationStructureType::BOTTOM_LEVEL, "Enum mismatch");

    return (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE)accelerationStructureType;
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS nri::GetAccelerationStructureBuildFlags(AccelerationStructureBuildBits accelerationStructureBuildFlags) {
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    if (accelerationStructureBuildFlags & AccelerationStructureBuildBits::ALLOW_UPDATE)
        flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

    if (accelerationStructureBuildFlags & AccelerationStructureBuildBits::ALLOW_COMPACTION)
        flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;

    if (accelerationStructureBuildFlags & AccelerationStructureBuildBits::PREFER_FAST_TRACE)
        flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    if (accelerationStructureBuildFlags & AccelerationStructureBuildBits::PREFER_FAST_BUILD)
        flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;

    if (accelerationStructureBuildFlags & AccelerationStructureBuildBits::MINIMIZE_MEMORY)
        flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;

    return flags;
}

D3D12_RAYTRACING_GEOMETRY_TYPE GetGeometryType(GeometryType geometryType) {
    static_assert(D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES == (uint32_t)GeometryType::TRIANGLES, "Enum mismatch");
    static_assert(D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS == (uint32_t)GeometryType::AABBS, "Enum mismatch");

    return (D3D12_RAYTRACING_GEOMETRY_TYPE)geometryType;
}

D3D12_RAYTRACING_GEOMETRY_FLAGS GetGeometryFlags(BottomLevelGeometryBits geometryFlagMask) {
    static_assert(D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE == (uint32_t)BottomLevelGeometryBits::OPAQUE_GEOMETRY, "Enum mismatch");
    static_assert(D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION == (uint32_t)BottomLevelGeometryBits::NO_DUPLICATE_ANY_HIT_INVOCATION, "Enum mismatch");

    return (D3D12_RAYTRACING_GEOMETRY_FLAGS)geometryFlagMask;
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE nri::GetCopyMode(CopyMode copyMode) {
    static_assert(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE == (uint32_t)CopyMode::CLONE, "Enum mismatch");
    static_assert(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT == (uint32_t)CopyMode::COMPACT, "Enum mismatch");

    return (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE)copyMode;
}

void nri::ConvertGeometryDescs(D3D12_RAYTRACING_GEOMETRY_DESC* geometryDescs, const GeometryObject* geometryObjects, uint32_t geometryObjectNum) {
    for (uint32_t i = 0; i < geometryObjectNum; i++) {
        geometryDescs[i].Type = GetGeometryType(geometryObjects[i].type);
        geometryDescs[i].Flags = GetGeometryFlags(geometryObjects[i].flags);

        if (geometryDescs[i].Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
            const Triangles& triangles = geometryObjects[i].geometry.triangles;
            geometryDescs[i].Triangles.Transform3x4 = triangles.transformBuffer ? ((BufferD3D12*)triangles.transformBuffer)->GetPointerGPU() + triangles.transformOffset : 0;
            geometryDescs[i].Triangles.IndexFormat = triangles.indexType == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            geometryDescs[i].Triangles.VertexFormat = GetDxgiFormat(triangles.vertexFormat).typed;
            geometryDescs[i].Triangles.IndexCount = triangles.indexNum;
            geometryDescs[i].Triangles.VertexCount = triangles.vertexNum;
            geometryDescs[i].Triangles.IndexBuffer = triangles.indexBuffer ? ((BufferD3D12*)triangles.indexBuffer)->GetPointerGPU() + triangles.indexOffset : 0;
            geometryDescs[i].Triangles.VertexBuffer.StartAddress = ((BufferD3D12*)triangles.vertexBuffer)->GetPointerGPU() + triangles.vertexOffset;
            geometryDescs[i].Triangles.VertexBuffer.StrideInBytes = triangles.vertexStride;
        } else if (geometryDescs[i].Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
            const AABBs& aabbs = geometryObjects[i].geometry.aabbs;
            geometryDescs[i].AABBs.AABBCount = aabbs.boxNum;
            geometryDescs[i].AABBs.AABBs.StartAddress = ((BufferD3D12*)aabbs.buffer)->GetPointerGPU() + aabbs.offset;
            geometryDescs[i].AABBs.AABBs.StrideInBytes = aabbs.stride;
        }
    }
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
    textureDesc.mipNum = (Mip_t)desc.MipLevels;
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
