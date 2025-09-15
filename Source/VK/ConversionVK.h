// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

uint32_t ConvertBotomLevelGeometries(
    VkAccelerationStructureBuildRangeInfoKHR* vkRanges,
    VkAccelerationStructureGeometryKHR* vkGeometries,
    VkAccelerationStructureTrianglesOpacityMicromapEXT* vkTrianglesMicromaps,
    const BottomLevelGeometryDesc* geometries, uint32_t geometryNum);

QueryType GetQueryTypeVK(uint32_t queryTypeVK);

constexpr std::array<VkIndexType, (size_t)IndexType::MAX_NUM> g_IndexTypes = {
    VK_INDEX_TYPE_UINT16, // UINT16
    VK_INDEX_TYPE_UINT32, // UINT32
};
VALIDATE_ARRAY(g_IndexTypes);

constexpr VkIndexType GetIndexType(IndexType indexType) {
    return g_IndexTypes[(size_t)indexType];
}

constexpr std::array<VkImageLayout, (size_t)Layout::MAX_NUM> g_ImageLayouts = {
    VK_IMAGE_LAYOUT_UNDEFINED,                                    // UNDEFINED
    VK_IMAGE_LAYOUT_GENERAL,                                      // GENERAL
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                              // PRESENT
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,                     // COLOR_ATTACHMENT
    VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR, // SHADING_RATE_ATTACHMENT
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,             // DEPTH_STENCIL_ATTACHMENT
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,              // DEPTH_STENCIL_READONLY
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,                     // SHADER_RESOURCE
    VK_IMAGE_LAYOUT_GENERAL,                                      // SHADER_RESOURCE_STORAGE
    IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                            // COPY_SOURCE
    IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                            // COPY_DESTINATION
    IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                            // RESOLVE_SOURCE
    IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                            // RESOLVE_DESTINATION
};
VALIDATE_ARRAY(g_ImageLayouts);

constexpr VkImageLayout GetImageLayout(Layout layout) {
    return g_ImageLayouts[(size_t)layout];
}

constexpr std::array<VkDescriptorType, (size_t)DescriptorType::MAX_NUM> g_DescriptorTypes = {
    VK_DESCRIPTOR_TYPE_SAMPLER,                    // SAMPLER
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             // CONSTANT_BUFFER
    VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,              // TEXTURE
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              // STORAGE_TEXTURE
    VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,       // BUFFER
    VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,       // STORAGE_BUFFER
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             // STRUCTURED_BUFFER
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             // STORAGE_STRUCTURED_BUFFER
    VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, // ACCELERATION_STRUCTURE
};
VALIDATE_ARRAY(g_DescriptorTypes);

constexpr VkDescriptorType GetDescriptorType(DescriptorType type) {
    return g_DescriptorTypes[(size_t)type];
}

constexpr std::array<VkPrimitiveTopology, (size_t)Topology::MAX_NUM> g_Topologies = {
    VK_PRIMITIVE_TOPOLOGY_POINT_LIST,                    // POINT_LIST
    VK_PRIMITIVE_TOPOLOGY_LINE_LIST,                     // LINE_LIST
    VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,                    // LINE_STRIP
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,                 // TRIANGLE_LIST
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,                // TRIANGLE_STRIP
    VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,      // LINE_LIST_WITH_ADJACENCY
    VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,     // LINE_STRIP_WITH_ADJACENCY
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,  // TRIANGLE_LIST_WITH_ADJACENCY
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY, // TRIANGLE_STRIP_WITH_ADJACENCY
    VK_PRIMITIVE_TOPOLOGY_PATCH_LIST                     // PATCH_LIST
};
VALIDATE_ARRAY(g_Topologies);

constexpr VkPrimitiveTopology GetTopology(Topology topology) {
    return g_Topologies[(size_t)topology];
}

constexpr std::array<VkCullModeFlags, (size_t)CullMode::MAX_NUM> g_CullModes = {
    VK_CULL_MODE_NONE,      // NONE
    VK_CULL_MODE_FRONT_BIT, // FRONT
    VK_CULL_MODE_BACK_BIT   // BACK
};
VALIDATE_ARRAY(g_CullModes);

constexpr VkCullModeFlags GetCullMode(CullMode cullMode) {
    return g_CullModes[(size_t)cullMode];
}

constexpr std::array<VkPolygonMode, (size_t)FillMode::MAX_NUM> g_FillModes = {
    VK_POLYGON_MODE_FILL, // SOLID
    VK_POLYGON_MODE_LINE, // WIREFRAME
};
VALIDATE_ARRAY(g_FillModes);

constexpr VkPolygonMode GetPolygonMode(FillMode fillMode) {
    return g_FillModes[(size_t)fillMode];
}

constexpr std::array<VkCompareOp, (size_t)CompareOp::MAX_NUM> g_CompareOps = {
    VK_COMPARE_OP_NEVER,            // NONE
    VK_COMPARE_OP_ALWAYS,           // ALWAYS
    VK_COMPARE_OP_NEVER,            // NEVER
    VK_COMPARE_OP_EQUAL,            // EQUAL
    VK_COMPARE_OP_NOT_EQUAL,        // NOT_EQUAL
    VK_COMPARE_OP_LESS,             // LESS
    VK_COMPARE_OP_LESS_OR_EQUAL,    // LESS_EQUAL
    VK_COMPARE_OP_GREATER,          // GREATER
    VK_COMPARE_OP_GREATER_OR_EQUAL, // GREATER_EQUAL
};
VALIDATE_ARRAY(g_CompareOps);

constexpr VkCompareOp GetCompareOp(CompareOp compareOp) {
    return g_CompareOps[(size_t)compareOp];
}

constexpr std::array<VkStencilOp, (size_t)StencilOp::MAX_NUM> g_StencilOps = {
    VK_STENCIL_OP_KEEP,                // KEEP,
    VK_STENCIL_OP_ZERO,                // ZERO,
    VK_STENCIL_OP_REPLACE,             // REPLACE,
    VK_STENCIL_OP_INCREMENT_AND_CLAMP, // INCREMENT_AND_CLAMP,
    VK_STENCIL_OP_DECREMENT_AND_CLAMP, // DECREMENT_AND_CLAMP,
    VK_STENCIL_OP_INVERT,              // INVERT,
    VK_STENCIL_OP_INCREMENT_AND_WRAP,  // INCREMENT_AND_WRAP,
    VK_STENCIL_OP_DECREMENT_AND_WRAP   // DECREMENT_AND_WRAP
};
VALIDATE_ARRAY(g_StencilOps);

constexpr VkStencilOp GetStencilOp(StencilOp stencilFunc) {
    return g_StencilOps[(size_t)stencilFunc];
}

constexpr std::array<VkLogicOp, (size_t)LogicOp::MAX_NUM> g_LogicOps = {
    VK_LOGIC_OP_MAX_ENUM,      // NONE
    VK_LOGIC_OP_CLEAR,         // CLEAR
    VK_LOGIC_OP_AND,           // AND
    VK_LOGIC_OP_AND_REVERSE,   // AND_REVERSE
    VK_LOGIC_OP_COPY,          // COPY
    VK_LOGIC_OP_AND_INVERTED,  // AND_INVERTED
    VK_LOGIC_OP_XOR,           // XOR
    VK_LOGIC_OP_OR,            // OR
    VK_LOGIC_OP_NOR,           // NOR
    VK_LOGIC_OP_EQUIVALENT,    // EQUIVALENT
    VK_LOGIC_OP_INVERT,        // INVERT
    VK_LOGIC_OP_OR_REVERSE,    // OR_REVERSE
    VK_LOGIC_OP_COPY_INVERTED, // COPY_INVERTED
    VK_LOGIC_OP_OR_INVERTED,   // OR_INVERTED
    VK_LOGIC_OP_NAND,          // NAND
    VK_LOGIC_OP_SET            // SET
};
VALIDATE_ARRAY(g_LogicOps);

constexpr VkLogicOp GetLogicOp(LogicOp logicOp) {
    return g_LogicOps[(size_t)logicOp];
}

constexpr std::array<VkBlendFactor, (size_t)BlendFactor::MAX_NUM> g_BlendFactors = {
    VK_BLEND_FACTOR_ZERO,                     // ZERO
    VK_BLEND_FACTOR_ONE,                      // ONE
    VK_BLEND_FACTOR_SRC_COLOR,                // SRC_COLOR
    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,      // ONE_MINUS_SRC_COLOR
    VK_BLEND_FACTOR_DST_COLOR,                // DST_COLOR
    VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,      // ONE_MINUS_DST_COLOR
    VK_BLEND_FACTOR_SRC_ALPHA,                // SRC_ALPHA
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,      // ONE_MINUS_SRC_ALPHA
    VK_BLEND_FACTOR_DST_ALPHA,                // DST_ALPHA
    VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,      // ONE_MINUS_DST_ALPHA
    VK_BLEND_FACTOR_CONSTANT_COLOR,           // CONSTANT_COLOR
    VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR, // ONE_MINUS_CONSTANT_COLOR
    VK_BLEND_FACTOR_CONSTANT_ALPHA,           // CONSTANT_ALPHA
    VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA, // ONE_MINUS_CONSTANT_ALPHA
    VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,       // SRC_ALPHA_SATURATE
    VK_BLEND_FACTOR_SRC1_COLOR,               // SRC1_COLOR
    VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,     // ONE_MINUS_SRC1_COLOR
    VK_BLEND_FACTOR_SRC1_ALPHA,               // SRC1_ALPHA
    VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,     // ONE_MINUS_SRC1_ALPHA
};
VALIDATE_ARRAY(g_BlendFactors);

constexpr VkBlendFactor GetBlendFactor(BlendFactor blendFactor) {
    return g_BlendFactors[(size_t)blendFactor];
}

constexpr std::array<VkBlendOp, (size_t)BlendOp::MAX_NUM> g_BlendOps = {
    VK_BLEND_OP_ADD,              // ADD
    VK_BLEND_OP_SUBTRACT,         // SUBTRACT
    VK_BLEND_OP_REVERSE_SUBTRACT, // REVERSE_SUBTRACT
    VK_BLEND_OP_MIN,              // MIN
    VK_BLEND_OP_MAX               // MAX
};
VALIDATE_ARRAY(g_BlendOps);

constexpr VkBlendOp GetBlendOp(BlendOp blendFunc) {
    return g_BlendOps[(size_t)blendFunc];
}

constexpr std::array<VkImageType, (size_t)TextureType::MAX_NUM> g_ImageTypes = {
    VK_IMAGE_TYPE_1D, // TEXTURE_1D
    VK_IMAGE_TYPE_2D, // TEXTURE_2D
    VK_IMAGE_TYPE_3D, // TEXTURE_3D
};
VALIDATE_ARRAY(g_ImageTypes);

constexpr VkImageType GetImageType(TextureType type) {
    return g_ImageTypes[(size_t)type];
}

constexpr std::array<VkFilter, (size_t)Filter::MAX_NUM> g_Filters = {
    VK_FILTER_NEAREST, // NEAREST
    VK_FILTER_LINEAR,  // LINEAR
};
VALIDATE_ARRAY(g_Filters);

constexpr VkFilter GetFilter(Filter filter) {
    return g_Filters[(size_t)filter];
}

constexpr std::array<VkSamplerReductionMode, (size_t)ReductionMode::MAX_NUM> g_ExtFilters = {
    VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE, // NONE
    VK_SAMPLER_REDUCTION_MODE_MIN,              // MIN
    VK_SAMPLER_REDUCTION_MODE_MAX,              // MAX
};
VALIDATE_ARRAY(g_ExtFilters);

constexpr VkSamplerReductionMode GetFilterExt(ReductionMode filterExt) {
    return g_ExtFilters[(size_t)filterExt];
}

constexpr std::array<VkSamplerMipmapMode, (size_t)Filter::MAX_NUM> g_SamplerMipmapModes = {
    VK_SAMPLER_MIPMAP_MODE_NEAREST, // NEAREST
    VK_SAMPLER_MIPMAP_MODE_LINEAR,  // LINEAR
};
VALIDATE_ARRAY(g_SamplerMipmapModes);

constexpr VkSamplerMipmapMode GetSamplerMipmapMode(Filter filter) {
    return g_SamplerMipmapModes[(size_t)filter];
}

constexpr VkSamplerAddressMode GetSamplerAddressMode(AddressMode addressMode) {
    return (VkSamplerAddressMode)(VK_SAMPLER_ADDRESS_MODE_REPEAT + (uint32_t)addressMode);
}

constexpr std::array<VkImageUsageFlags, (size_t)Texture1DViewType::MAX_NUM> g_ImageViewUsage1D = {
    VK_IMAGE_USAGE_SAMPLED_BIT,                  // SHADER_RESOURCE_1D,
    VK_IMAGE_USAGE_SAMPLED_BIT,                  // SHADER_RESOURCE_1D_ARRAY,
    VK_IMAGE_USAGE_STORAGE_BIT,                  // SHADER_RESOURCE_STORAGE_1D,
    VK_IMAGE_USAGE_STORAGE_BIT,                  // SHADER_RESOURCE_STORAGE_1D_ARRAY,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,         // COLOR_ATTACHMENT,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, // DEPTH_STENCIL_ATTACHMENT
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, // DEPTH_READONLY_STENCIL_ATTACHMENT,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, // DEPTH_ATTACHMENT_STENCIL_READONLY,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, // DEPTH_STENCIL_READONLY,
};
VALIDATE_ARRAY(g_ImageViewUsage1D);

constexpr VkImageUsageFlags GetImageViewUsage(Texture1DViewType type) {
    return g_ImageViewUsage1D[(size_t)type];
}

constexpr std::array<VkImageUsageFlags, (size_t)Texture2DViewType::MAX_NUM> g_ImageViewUsage2D = {
    VK_IMAGE_USAGE_SAMPLED_BIT,                              // SHADER_RESOURCE_2D,
    VK_IMAGE_USAGE_SAMPLED_BIT,                              // SHADER_RESOURCE_2D_ARRAY,
    VK_IMAGE_USAGE_SAMPLED_BIT,                              // SHADER_RESOURCE_CUBE,
    VK_IMAGE_USAGE_SAMPLED_BIT,                              // SHADER_RESOURCE_CUBE_ARRAY,
    VK_IMAGE_USAGE_STORAGE_BIT,                              // SHADER_RESOURCE_STORAGE_2D,
    VK_IMAGE_USAGE_STORAGE_BIT,                              // SHADER_RESOURCE_STORAGE_2D_ARRAY,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,                     // COLOR_ATTACHMENT,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,             // DEPTH_STENCIL_ATTACHMENT
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,             // DEPTH_READONLY_STENCIL_ATTACHMENT,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,             // DEPTH_ATTACHMENT_STENCIL_READONLY,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,             // DEPTH_STENCIL_READONLY,
    VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR, // SHADING_RATE_ATTACHMENT
};
VALIDATE_ARRAY(g_ImageViewUsage2D);

constexpr VkImageUsageFlags GetImageViewUsage(Texture2DViewType type) {
    return g_ImageViewUsage2D[(size_t)type];
}

constexpr std::array<VkImageUsageFlags, (size_t)Texture3DViewType::MAX_NUM> g_ImageViewUsage3D = {
    VK_IMAGE_USAGE_SAMPLED_BIT,          // SHADER_RESOURCE_3D,
    VK_IMAGE_USAGE_STORAGE_BIT,          // SHADER_RESOURCE_STORAGE_3D,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // COLOR_ATTACHMENT
};
VALIDATE_ARRAY(g_ImageViewUsage3D);

constexpr VkImageUsageFlags GetImageViewUsage(Texture3DViewType type) {
    return g_ImageViewUsage3D[(size_t)type];
}

constexpr std::array<VkImageLayout, (size_t)Texture1DViewType::MAX_NUM> g_ImageLayout1D = {
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,                   // SHADER_RESOURCE_1D,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,                   // SHADER_RESOURCE_1D_ARRAY,
    VK_IMAGE_LAYOUT_GENERAL,                                    // SHADER_RESOURCE_STORAGE_1D,
    VK_IMAGE_LAYOUT_GENERAL,                                    // SHADER_RESOURCE_STORAGE_1D_ARRAY,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,                   // COLOR_ATTACHMENT,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,           // DEPTH_STENCIL_ATTACHMENT
    VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL, // DEPTH_READONLY_STENCIL_ATTACHMENT,
    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL, // DEPTH_ATTACHMENT_STENCIL_READONLY,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,            // DEPTH_STENCIL_READONLY,
};
VALIDATE_ARRAY(g_ImageLayout1D);

constexpr VkImageLayout GetImageLayoutForView(Texture1DViewType type) {
    return g_ImageLayout1D[(size_t)type];
}

constexpr std::array<VkImageLayout, (size_t)Texture2DViewType::MAX_NUM> g_ImageLayout2D = {
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,                     // SHADER_RESOURCE_2D,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,                     // SHADER_RESOURCE_2D_ARRAY,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,                     // SHADER_RESOURCE_CUBE,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,                     // SHADER_RESOURCE_CUBE_ARRAY,
    VK_IMAGE_LAYOUT_GENERAL,                                      // SHADER_RESOURCE_STORAGE_2D,
    VK_IMAGE_LAYOUT_GENERAL,                                      // SHADER_RESOURCE_STORAGE_2D_ARRAY,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,                     // COLOR_ATTACHMENT,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,             // DEPTH_STENCIL_ATTACHMENT
    VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,   // DEPTH_READONLY_STENCIL_ATTACHMENT,
    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,   // DEPTH_ATTACHMENT_STENCIL_READONLY,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,              // DEPTH_STENCIL_READONLY,
    VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR, // SHADING_RATE_ATTACHMENT
};
VALIDATE_ARRAY(g_ImageLayout2D);

constexpr VkImageLayout GetImageLayoutForView(Texture2DViewType type) {
    return g_ImageLayout2D[(size_t)type];
}

constexpr std::array<VkImageLayout, (size_t)Texture3DViewType::MAX_NUM> g_ImageLayout3D = {
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // SHADER_RESOURCE_3D,
    VK_IMAGE_LAYOUT_GENERAL,                  // SHADER_RESOURCE_STORAGE_3D,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // COLOR_ATTACHMENT
};
VALIDATE_ARRAY(g_ImageLayout3D);

constexpr VkImageLayout GetImageLayoutForView(Texture3DViewType type) {
    return g_ImageLayout3D[(size_t)type];
}

constexpr std::array<VkFragmentShadingRateCombinerOpKHR, (size_t)ShadingRateCombiner::MAX_NUM> g_ShadingRateCombiner = {
    VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,    // KEEP,
    VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR, // REPLACE,
    VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MIN_KHR,     // MIN,
    VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR,     // MAX,
    VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MUL_KHR,     // SUM, // TODO: SUM vs MUL?
};
VALIDATE_ARRAY(g_ShadingRateCombiner);

constexpr VkFragmentShadingRateCombinerOpKHR GetShadingRateCombiner(ShadingRateCombiner combiner) {
    return g_ShadingRateCombiner[(size_t)combiner];
}

constexpr std::array<TextureType, (size_t)TextureType::MAX_NUM> g_TextureTypes = {
    TextureType::TEXTURE_1D, // VK_IMAGE_TYPE_1D
    TextureType::TEXTURE_2D, // VK_IMAGE_TYPE_2D
    TextureType::TEXTURE_3D, // VK_IMAGE_TYPE_3D
};
VALIDATE_ARRAY(g_TextureTypes);

constexpr TextureType GetTextureType(VkImageType type) {
    if ((size_t)type < g_TextureTypes.size())
        return g_TextureTypes[(size_t)type];

    return TextureType::MAX_NUM;
}

constexpr VkPipelineStageFlags2 GetPipelineStageFlags(StageBits stageBits) {
    // Check non-mask values first
    if (stageBits == StageBits::ALL)
        return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    if (stageBits == StageBits::NONE)
        return VK_PIPELINE_STAGE_2_NONE;

    // Gather bits
    VkPipelineStageFlags2 flags = 0;

    if (stageBits & StageBits::INDEX_INPUT)
        flags |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;

    if (stageBits & StageBits::VERTEX_SHADER)
        flags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;

    if (stageBits & StageBits::TESS_CONTROL_SHADER)
        flags |= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT;

    if (stageBits & StageBits::TESS_EVALUATION_SHADER)
        flags |= VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT;

    if (stageBits & StageBits::GEOMETRY_SHADER)
        flags |= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT;

    if (stageBits & StageBits::TASK_SHADER)
        flags |= VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT;

    if (stageBits & StageBits::MESH_SHADER)
        flags |= VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT;

    if (stageBits & StageBits::FRAGMENT_SHADER)
        flags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

    if (stageBits & StageBits::DEPTH_STENCIL_ATTACHMENT)
        flags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

    if (stageBits & StageBits::COLOR_ATTACHMENT)
        flags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    if (stageBits & StageBits::COMPUTE_SHADER)
        flags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

    if (stageBits & StageBits::RAY_TRACING_SHADERS)
        flags |= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;

    if (stageBits & StageBits::INDIRECT)
        flags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;

    if (stageBits & StageBits::COPY)
        flags |= VK_PIPELINE_STAGE_2_COPY_BIT;

    if (stageBits & StageBits::RESOLVE)
        flags |= VK_PIPELINE_STAGE_2_RESOLVE_BIT;

    if (stageBits & StageBits::CLEAR_STORAGE)
        flags |= VK_PIPELINE_STAGE_2_CLEAR_BIT;

    if (stageBits & StageBits::ACCELERATION_STRUCTURE)
        flags |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR; // already includes "VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR" (more strict according to the spec)

    if (stageBits & StageBits::MICROMAP)
        flags |= VK_PIPELINE_STAGE_2_MICROMAP_BUILD_BIT_EXT;

    return flags;
}

constexpr VkShaderStageFlags GetShaderStageFlags(StageBits stage) {
    // Check non-mask values first
    if (stage == StageBits::ALL)
        return VK_SHADER_STAGE_ALL;

    if (stage == StageBits::NONE)
        return 0;

    // Gather bits
    VkShaderStageFlags stageFlags = 0;

    if (stage & StageBits::VERTEX_SHADER)
        stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;

    if (stage & StageBits::TESS_CONTROL_SHADER)
        stageFlags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

    if (stage & StageBits::TESS_EVALUATION_SHADER)
        stageFlags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

    if (stage & StageBits::GEOMETRY_SHADER)
        stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

    if (stage & StageBits::FRAGMENT_SHADER)
        stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;

    if (stage & StageBits::COMPUTE_SHADER)
        stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;

    if (stage & StageBits::RAYGEN_SHADER)
        stageFlags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    if (stage & StageBits::MISS_SHADER)
        stageFlags |= VK_SHADER_STAGE_MISS_BIT_KHR;

    if (stage & StageBits::INTERSECTION_SHADER)
        stageFlags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;

    if (stage & StageBits::CLOSEST_HIT_SHADER)
        stageFlags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    if (stage & StageBits::ANY_HIT_SHADER)
        stageFlags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

    if (stage & StageBits::CALLABLE_SHADER)
        stageFlags |= VK_SHADER_STAGE_CALLABLE_BIT_KHR;

    if (stage & StageBits::TASK_SHADER)
        stageFlags |= VK_SHADER_STAGE_TASK_BIT_EXT;

    if (stage & StageBits::MESH_SHADER)
        stageFlags |= VK_SHADER_STAGE_MESH_BIT_EXT;

    return stageFlags;
}

constexpr VkImageAspectFlags GetImageAspectFlags(Format format) {
    switch (format) {
        case Format::D16_UNORM:
        case Format::D32_SFLOAT:
        case Format::R24_UNORM_X8:
        case Format::R32_SFLOAT_X8_X24:
            return VK_IMAGE_ASPECT_DEPTH_BIT;

        case Format::D24_UNORM_S8_UINT:
        case Format::D32_SFLOAT_S8_UINT_X24:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

        case Format::X32_G8_UINT_X24:
        case Format::X24_G8_UINT:
            return VK_IMAGE_ASPECT_STENCIL_BIT;

        default:
            return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

constexpr VkImageAspectFlags GetImageAspectFlags(PlaneBits planes) {
    VkImageAspectFlags aspectFlags = 0;
    if (planes & PlaneBits::COLOR)
        aspectFlags |= VK_IMAGE_ASPECT_COLOR_BIT;
    if (planes & PlaneBits::DEPTH)
        aspectFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;
    if (planes & PlaneBits::STENCIL)
        aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;

    return aspectFlags;
}

constexpr Result GetResultFromVkResult(VkResult vkResult) {
    if (vkResult >= 0)
        return Result::SUCCESS;

    switch (vkResult) {
        case VK_ERROR_DEVICE_LOST:
            return Result::DEVICE_LOST;

        case VK_ERROR_SURFACE_LOST_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
            return Result::OUT_OF_DATE;

        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
        case VK_ERROR_INCOMPATIBLE_DRIVER:
        case VK_ERROR_FEATURE_NOT_PRESENT:
        case VK_ERROR_EXTENSION_NOT_PRESENT:
        case VK_ERROR_LAYER_NOT_PRESENT:
            return Result::UNSUPPORTED;

        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_OUT_OF_POOL_MEMORY:
        case VK_ERROR_FRAGMENTATION:
        case VK_ERROR_FRAGMENTED_POOL:
            return Result::OUT_OF_MEMORY;

        default:
            return Result::FAILURE;
    }
}

constexpr VkColorComponentFlags GetColorComponent(ColorWriteBits colorWriteMask) {
    return VkColorComponentFlags(colorWriteMask & ColorWriteBits::RGBA);
}

constexpr VkBuildMicromapFlagsEXT GetBuildMicromapFlags(MicromapBits micromapBits) {
    VkBuildMicromapFlagsEXT flags = 0;

    if (micromapBits & MicromapBits::ALLOW_COMPACTION)
        flags |= VK_BUILD_MICROMAP_ALLOW_COMPACTION_BIT_EXT;

    if (micromapBits & MicromapBits::PREFER_FAST_TRACE)
        flags |= VK_BUILD_MICROMAP_PREFER_FAST_TRACE_BIT_EXT;

    if (micromapBits & MicromapBits::PREFER_FAST_BUILD)
        flags |= VK_BUILD_MICROMAP_PREFER_FAST_BUILD_BIT_EXT;

    return flags;
}

constexpr VkAccelerationStructureTypeKHR GetAccelerationStructureType(AccelerationStructureType type) {
    static_assert(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR == (uint32_t)AccelerationStructureType::TOP_LEVEL, "Enum mismatch");
    static_assert(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR == (uint32_t)AccelerationStructureType::BOTTOM_LEVEL, "Enum mismatch");

    return (VkAccelerationStructureTypeKHR)type;
}

constexpr VkBuildAccelerationStructureFlagsKHR GetBuildAccelerationStructureFlags(AccelerationStructureBits accelerationStructureBits) {
    VkBuildAccelerationStructureFlagsKHR flags = 0;

    if (accelerationStructureBits & AccelerationStructureBits::ALLOW_UPDATE)
        flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;

    if (accelerationStructureBits & AccelerationStructureBits::ALLOW_COMPACTION)
        flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;

    if (accelerationStructureBits & AccelerationStructureBits::ALLOW_DATA_ACCESS)
        flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR;

    if (accelerationStructureBits & AccelerationStructureBits::ALLOW_MICROMAP_UPDATE)
        flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_OPACITY_MICROMAP_UPDATE_EXT;

    if (accelerationStructureBits & AccelerationStructureBits::ALLOW_DISABLE_MICROMAPS)
        flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DISABLE_OPACITY_MICROMAPS_EXT;

    if (accelerationStructureBits & AccelerationStructureBits::PREFER_FAST_TRACE)
        flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;

    if (accelerationStructureBits & AccelerationStructureBits::PREFER_FAST_BUILD)
        flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;

    if (accelerationStructureBits & AccelerationStructureBits::MINIMIZE_MEMORY)
        flags |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;

    return flags;
}

constexpr VkGeometryFlagsKHR GetGeometryFlags(BottomLevelGeometryBits geometryFlags) {
    static_assert(VK_GEOMETRY_OPAQUE_BIT_KHR == (uint32_t)BottomLevelGeometryBits::OPAQUE_GEOMETRY, "Enum mismatch");
    static_assert(VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR == (uint32_t)BottomLevelGeometryBits::NO_DUPLICATE_ANY_HIT_INVOCATION, "Enum mismatch");

    return (VkGeometryFlagsKHR)geometryFlags;
}

constexpr VkGeometryTypeKHR GetGeometryType(BottomLevelGeometryType geometryType) {
    static_assert(VK_GEOMETRY_TYPE_TRIANGLES_KHR == (uint32_t)BottomLevelGeometryType::TRIANGLES, "Enum mismatch");
    static_assert(VK_GEOMETRY_TYPE_AABBS_KHR == (uint32_t)BottomLevelGeometryType::AABBS, "Enum mismatch");

    return (VkGeometryTypeKHR)geometryType;
}

constexpr VkCopyAccelerationStructureModeKHR GetAccelerationStructureCopyMode(CopyMode copyMode) {
    if (copyMode == CopyMode::CLONE)
        return VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR;

    if (copyMode == CopyMode::COMPACT)
        return VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;

    return VK_COPY_ACCELERATION_STRUCTURE_MODE_MAX_ENUM_KHR;
}

constexpr VkCopyMicromapModeEXT GetMicromapCopyMode(CopyMode copyMode) {
    if (copyMode == CopyMode::CLONE)
        return VK_COPY_MICROMAP_MODE_CLONE_EXT;

    if (copyMode == CopyMode::COMPACT)
        return VK_COPY_MICROMAP_MODE_COMPACT_EXT;

    return VK_COPY_MICROMAP_MODE_MAX_ENUM_EXT;
}

inline VkFormat GetVkFormat(Format format, bool demoteSrgb = false) {
    if (demoteSrgb) {
        const FormatProps& formatProps = GetFormatProps(format);
        if (formatProps.isSrgb)
            format = (Format)((uint32_t)format - 1);
    }

    return (VkFormat)NRIFormatToVKFormat(format);
}

inline VkExtent2D GetShadingRate(ShadingRate shadingRate) {
    switch (shadingRate) {
        case ShadingRate::FRAGMENT_SIZE_1X2:
            return {1, 2};
        case ShadingRate::FRAGMENT_SIZE_2X1:
            return {2, 1};
        case ShadingRate::FRAGMENT_SIZE_2X2:
            return {2, 2};
        case ShadingRate::FRAGMENT_SIZE_2X4:
            return {2, 4};
        case ShadingRate::FRAGMENT_SIZE_4X2:
            return {4, 2};
        case ShadingRate::FRAGMENT_SIZE_4X4:
            return {4, 4};
        default:
            return {1, 1};
    }
}

} // namespace nri