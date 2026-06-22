// © 2026 NVIDIA Corporation

WGPUShaderStage nri::GetShaderStageFlags(StageBits stageBits) {
    if (stageBits == StageBits::ALL)
        return WGPUShaderStage_Vertex | WGPUShaderStage_Fragment | WGPUShaderStage_Compute;

    WGPUShaderStage flags = WGPUShaderStage_None;

    if (stageBits & StageBits::VERTEX_SHADER)
        flags |= WGPUShaderStage_Vertex;
    if (stageBits & StageBits::FRAGMENT_SHADER)
        flags |= WGPUShaderStage_Fragment;
    if (stageBits & StageBits::COMPUTE_SHADER)
        flags |= WGPUShaderStage_Compute;

    return flags;
}

WGPUTextureFormat nri::GetTextureFormat(Format format) {
    switch (format) {
        case Format::R8_UNORM:
            return WGPUTextureFormat_R8Unorm;
        case Format::R8_SNORM:
            return WGPUTextureFormat_R8Snorm;
        case Format::R8_UINT:
            return WGPUTextureFormat_R8Uint;
        case Format::R8_SINT:
            return WGPUTextureFormat_R8Sint;
        case Format::RG8_UNORM:
            return WGPUTextureFormat_RG8Unorm;
        case Format::RG8_SNORM:
            return WGPUTextureFormat_RG8Snorm;
        case Format::RG8_UINT:
            return WGPUTextureFormat_RG8Uint;
        case Format::RG8_SINT:
            return WGPUTextureFormat_RG8Sint;
        case Format::BGRA8_UNORM:
            return WGPUTextureFormat_BGRA8Unorm;
        case Format::BGRA8_SRGB:
            return WGPUTextureFormat_BGRA8UnormSrgb;
        case Format::RGBA8_UNORM:
            return WGPUTextureFormat_RGBA8Unorm;
        case Format::RGBA8_SRGB:
            return WGPUTextureFormat_RGBA8UnormSrgb;
        case Format::RGBA8_SNORM:
            return WGPUTextureFormat_RGBA8Snorm;
        case Format::RGBA8_UINT:
            return WGPUTextureFormat_RGBA8Uint;
        case Format::RGBA8_SINT:
            return WGPUTextureFormat_RGBA8Sint;
        case Format::R16_UNORM:
            return WGPUTextureFormat_R16Unorm;
        case Format::R16_SNORM:
            return WGPUTextureFormat_R16Snorm;
        case Format::R16_UINT:
            return WGPUTextureFormat_R16Uint;
        case Format::R16_SINT:
            return WGPUTextureFormat_R16Sint;
        case Format::R16_SFLOAT:
            return WGPUTextureFormat_R16Float;
        case Format::RG16_UNORM:
            return WGPUTextureFormat_RG16Unorm;
        case Format::RG16_SNORM:
            return WGPUTextureFormat_RG16Snorm;
        case Format::RG16_UINT:
            return WGPUTextureFormat_RG16Uint;
        case Format::RG16_SINT:
            return WGPUTextureFormat_RG16Sint;
        case Format::RG16_SFLOAT:
            return WGPUTextureFormat_RG16Float;
        case Format::RGBA16_UNORM:
            return WGPUTextureFormat_RGBA16Unorm;
        case Format::RGBA16_SNORM:
            return WGPUTextureFormat_RGBA16Snorm;
        case Format::RGBA16_UINT:
            return WGPUTextureFormat_RGBA16Uint;
        case Format::RGBA16_SINT:
            return WGPUTextureFormat_RGBA16Sint;
        case Format::RGBA16_SFLOAT:
            return WGPUTextureFormat_RGBA16Float;
        case Format::R32_UINT:
            return WGPUTextureFormat_R32Uint;
        case Format::R32_SINT:
            return WGPUTextureFormat_R32Sint;
        case Format::R32_SFLOAT:
            return WGPUTextureFormat_R32Float;
        case Format::RG32_UINT:
            return WGPUTextureFormat_RG32Uint;
        case Format::RG32_SINT:
            return WGPUTextureFormat_RG32Sint;
        case Format::RG32_SFLOAT:
            return WGPUTextureFormat_RG32Float;
        case Format::RGBA32_UINT:
            return WGPUTextureFormat_RGBA32Uint;
        case Format::RGBA32_SINT:
            return WGPUTextureFormat_RGBA32Sint;
        case Format::RGBA32_SFLOAT:
            return WGPUTextureFormat_RGBA32Float;
        case Format::R10_G10_B10_A2_UINT:
            return WGPUTextureFormat_RGB10A2Uint;
        case Format::R10_G10_B10_A2_UNORM:
            return WGPUTextureFormat_RGB10A2Unorm;
        case Format::R11_G11_B10_UFLOAT:
            return WGPUTextureFormat_RG11B10Ufloat;
        case Format::R9_G9_B9_E5_UFLOAT:
            return WGPUTextureFormat_RGB9E5Ufloat;
        case Format::BC1_RGBA_UNORM:
            return WGPUTextureFormat_BC1RGBAUnorm;
        case Format::BC1_RGBA_SRGB:
            return WGPUTextureFormat_BC1RGBAUnormSrgb;
        case Format::BC2_RGBA_UNORM:
            return WGPUTextureFormat_BC2RGBAUnorm;
        case Format::BC2_RGBA_SRGB:
            return WGPUTextureFormat_BC2RGBAUnormSrgb;
        case Format::BC3_RGBA_UNORM:
            return WGPUTextureFormat_BC3RGBAUnorm;
        case Format::BC3_RGBA_SRGB:
            return WGPUTextureFormat_BC3RGBAUnormSrgb;
        case Format::BC4_R_UNORM:
            return WGPUTextureFormat_BC4RUnorm;
        case Format::BC4_R_SNORM:
            return WGPUTextureFormat_BC4RSnorm;
        case Format::BC5_RG_UNORM:
            return WGPUTextureFormat_BC5RGUnorm;
        case Format::BC5_RG_SNORM:
            return WGPUTextureFormat_BC5RGSnorm;
        case Format::BC6H_RGB_UFLOAT:
            return WGPUTextureFormat_BC6HRGBUfloat;
        case Format::BC6H_RGB_SFLOAT:
            return WGPUTextureFormat_BC6HRGBFloat;
        case Format::BC7_RGBA_UNORM:
            return WGPUTextureFormat_BC7RGBAUnorm;
        case Format::BC7_RGBA_SRGB:
            return WGPUTextureFormat_BC7RGBAUnormSrgb;
        case Format::ETC2_RGB8_UNORM:
            return WGPUTextureFormat_ETC2RGB8Unorm;
        case Format::ETC2_RGB8_SRGB:
            return WGPUTextureFormat_ETC2RGB8UnormSrgb;
        case Format::ETC2_RGB8_A1_UNORM:
            return WGPUTextureFormat_ETC2RGB8A1Unorm;
        case Format::ETC2_RGB8_A1_SRGB:
            return WGPUTextureFormat_ETC2RGB8A1UnormSrgb;
        case Format::ETC2_RGB8_A8_UNORM:
            return WGPUTextureFormat_ETC2RGBA8Unorm;
        case Format::ETC2_RGB8_A8_SRGB:
            return WGPUTextureFormat_ETC2RGBA8UnormSrgb;
        case Format::ETC2_R11_UNORM:
            return WGPUTextureFormat_EACR11Unorm;
        case Format::ETC2_R11_SNORM:
            return WGPUTextureFormat_EACR11Snorm;
        case Format::ETC2_R11_G11_UNORM:
            return WGPUTextureFormat_EACRG11Unorm;
        case Format::ETC2_R11_G11_SNORM:
            return WGPUTextureFormat_EACRG11Snorm;
        case Format::ASTC_4X4_UNORM:
            return WGPUTextureFormat_ASTC4x4Unorm;
        case Format::ASTC_4X4_SRGB:
            return WGPUTextureFormat_ASTC4x4UnormSrgb;
        case Format::ASTC_5X4_UNORM:
            return WGPUTextureFormat_ASTC5x4Unorm;
        case Format::ASTC_5X4_SRGB:
            return WGPUTextureFormat_ASTC5x4UnormSrgb;
        case Format::ASTC_5X5_UNORM:
            return WGPUTextureFormat_ASTC5x5Unorm;
        case Format::ASTC_5X5_SRGB:
            return WGPUTextureFormat_ASTC5x5UnormSrgb;
        case Format::ASTC_6X5_UNORM:
            return WGPUTextureFormat_ASTC6x5Unorm;
        case Format::ASTC_6X5_SRGB:
            return WGPUTextureFormat_ASTC6x5UnormSrgb;
        case Format::ASTC_6X6_UNORM:
            return WGPUTextureFormat_ASTC6x6Unorm;
        case Format::ASTC_6X6_SRGB:
            return WGPUTextureFormat_ASTC6x6UnormSrgb;
        case Format::ASTC_8X5_UNORM:
            return WGPUTextureFormat_ASTC8x5Unorm;
        case Format::ASTC_8X5_SRGB:
            return WGPUTextureFormat_ASTC8x5UnormSrgb;
        case Format::ASTC_8X6_UNORM:
            return WGPUTextureFormat_ASTC8x6Unorm;
        case Format::ASTC_8X6_SRGB:
            return WGPUTextureFormat_ASTC8x6UnormSrgb;
        case Format::ASTC_8X8_UNORM:
            return WGPUTextureFormat_ASTC8x8Unorm;
        case Format::ASTC_8X8_SRGB:
            return WGPUTextureFormat_ASTC8x8UnormSrgb;
        case Format::ASTC_10X5_UNORM:
            return WGPUTextureFormat_ASTC10x5Unorm;
        case Format::ASTC_10X5_SRGB:
            return WGPUTextureFormat_ASTC10x5UnormSrgb;
        case Format::ASTC_10X6_UNORM:
            return WGPUTextureFormat_ASTC10x6Unorm;
        case Format::ASTC_10X6_SRGB:
            return WGPUTextureFormat_ASTC10x6UnormSrgb;
        case Format::ASTC_10X8_UNORM:
            return WGPUTextureFormat_ASTC10x8Unorm;
        case Format::ASTC_10X8_SRGB:
            return WGPUTextureFormat_ASTC10x8UnormSrgb;
        case Format::ASTC_10X10_UNORM:
            return WGPUTextureFormat_ASTC10x10Unorm;
        case Format::ASTC_10X10_SRGB:
            return WGPUTextureFormat_ASTC10x10UnormSrgb;
        case Format::ASTC_12X10_UNORM:
            return WGPUTextureFormat_ASTC12x10Unorm;
        case Format::ASTC_12X10_SRGB:
            return WGPUTextureFormat_ASTC12x10UnormSrgb;
        case Format::ASTC_12X12_UNORM:
            return WGPUTextureFormat_ASTC12x12Unorm;
        case Format::ASTC_12X12_SRGB:
            return WGPUTextureFormat_ASTC12x12UnormSrgb;
        case Format::D16_UNORM:
            return WGPUTextureFormat_Depth16Unorm;
        case Format::D32_SFLOAT:
            return WGPUTextureFormat_Depth32Float;
        case Format::D24_UNORM_S8_UINT:
            return WGPUTextureFormat_Depth24PlusStencil8;
        case Format::D32_SFLOAT_S8_UINT:
            return WGPUTextureFormat_Depth32FloatStencil8;
        default:
            return WGPUTextureFormat_Undefined;
    }
}

WGPUTextureSampleType nri::GetTextureSampleType(Format format) {
    const FormatProps& props = GetFormatProps(format);
    if (props.isDepth)
        return WGPUTextureSampleType_Depth;
    if (props.isInteger)
        return props.isSigned ? WGPUTextureSampleType_Sint : WGPUTextureSampleType_Uint;

    return WGPUTextureSampleType_Float;
}

WGPUTextureFormat nri::GetSwapChainTextureFormat(SwapChainFormat format) {
    switch (format) {
        case SwapChainFormat::BT709_G22_8BIT:
            return WGPUTextureFormat_RGBA8Unorm;
        case SwapChainFormat::BT709_G10_16BIT:
            return WGPUTextureFormat_RGBA16Float;
        default:
            return WGPUTextureFormat_BGRA8Unorm;
    }
}

Format nri::GetNRIFormat(WGPUTextureFormat format) {
    switch (format) {
        case WGPUTextureFormat_BGRA8Unorm:
            return Format::BGRA8_UNORM;
        case WGPUTextureFormat_BGRA8UnormSrgb:
            return Format::BGRA8_SRGB;
        case WGPUTextureFormat_RGBA8Unorm:
            return Format::RGBA8_UNORM;
        case WGPUTextureFormat_RGBA8UnormSrgb:
            return Format::RGBA8_SRGB;
        case WGPUTextureFormat_RGBA16Float:
            return Format::RGBA16_SFLOAT;
        default:
            return Format::UNKNOWN;
    }
}

WGPUTextureUsage nri::GetTextureUsage(TextureUsageBits usage) {
    WGPUTextureUsage result = WGPUTextureUsage_CopySrc | WGPUTextureUsage_CopyDst;

    if (usage & (TextureUsageBits::SHADER_RESOURCE | TextureUsageBits::INPUT_ATTACHMENT))
        result |= WGPUTextureUsage_TextureBinding;
    if (usage & TextureUsageBits::SHADER_RESOURCE_STORAGE)
        result |= WGPUTextureUsage_StorageBinding;
    if (usage & (TextureUsageBits::COLOR_ATTACHMENT | TextureUsageBits::DEPTH_STENCIL_ATTACHMENT))
        result |= WGPUTextureUsage_RenderAttachment;

    return result;
}

WGPUBufferUsage nri::GetBufferUsage(BufferUsageBits usage) {
    WGPUBufferUsage result = WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst | WGPUBufferUsage_QueryResolve;

    if (usage & BufferUsageBits::VERTEX_BUFFER)
        result |= WGPUBufferUsage_Vertex;
    if (usage & BufferUsageBits::INDEX_BUFFER)
        result |= WGPUBufferUsage_Index;
    if (usage & BufferUsageBits::CONSTANT_BUFFER)
        result |= WGPUBufferUsage_Uniform;
    if (usage & (BufferUsageBits::SHADER_RESOURCE | BufferUsageBits::SHADER_RESOURCE_STORAGE))
        result |= WGPUBufferUsage_Storage;
    if (usage & BufferUsageBits::ARGUMENT_BUFFER)
        result |= WGPUBufferUsage_Indirect;

    return result;
}

WGPUVertexFormat nri::GetVertexFormat(Format format) {
    switch (format) {
        case Format::R8_UNORM:
            return WGPUVertexFormat_Unorm8;
        case Format::R8_SNORM:
            return WGPUVertexFormat_Snorm8;
        case Format::R8_UINT:
            return WGPUVertexFormat_Uint8;
        case Format::R8_SINT:
            return WGPUVertexFormat_Sint8;
        case Format::RG8_UNORM:
            return WGPUVertexFormat_Unorm8x2;
        case Format::RG8_SNORM:
            return WGPUVertexFormat_Snorm8x2;
        case Format::RG8_UINT:
            return WGPUVertexFormat_Uint8x2;
        case Format::RG8_SINT:
            return WGPUVertexFormat_Sint8x2;
        case Format::BGRA8_UNORM:
            return WGPUVertexFormat_Unorm8x4BGRA;
        case Format::RGBA8_UNORM:
            return WGPUVertexFormat_Unorm8x4;
        case Format::RGBA8_SNORM:
            return WGPUVertexFormat_Snorm8x4;
        case Format::RGBA8_UINT:
            return WGPUVertexFormat_Uint8x4;
        case Format::RGBA8_SINT:
            return WGPUVertexFormat_Sint8x4;
        case Format::R10_G10_B10_A2_UNORM:
            return WGPUVertexFormat_Unorm10_10_10_2;
        case Format::R16_UNORM:
            return WGPUVertexFormat_Unorm16;
        case Format::R16_SNORM:
            return WGPUVertexFormat_Snorm16;
        case Format::R16_UINT:
            return WGPUVertexFormat_Uint16;
        case Format::R16_SINT:
            return WGPUVertexFormat_Sint16;
        case Format::R16_SFLOAT:
            return WGPUVertexFormat_Float16;
        case Format::RG16_UNORM:
            return WGPUVertexFormat_Unorm16x2;
        case Format::RG16_SNORM:
            return WGPUVertexFormat_Snorm16x2;
        case Format::RG16_UINT:
            return WGPUVertexFormat_Uint16x2;
        case Format::RG16_SINT:
            return WGPUVertexFormat_Sint16x2;
        case Format::RG16_SFLOAT:
            return WGPUVertexFormat_Float16x2;
        case Format::RGBA16_UNORM:
            return WGPUVertexFormat_Unorm16x4;
        case Format::RGBA16_SNORM:
            return WGPUVertexFormat_Snorm16x4;
        case Format::RGBA16_UINT:
            return WGPUVertexFormat_Uint16x4;
        case Format::RGBA16_SINT:
            return WGPUVertexFormat_Sint16x4;
        case Format::RGBA16_SFLOAT:
            return WGPUVertexFormat_Float16x4;
        case Format::R32_UINT:
            return WGPUVertexFormat_Uint32;
        case Format::R32_SINT:
            return WGPUVertexFormat_Sint32;
        case Format::R32_SFLOAT:
            return WGPUVertexFormat_Float32;
        case Format::RG32_UINT:
            return WGPUVertexFormat_Uint32x2;
        case Format::RG32_SINT:
            return WGPUVertexFormat_Sint32x2;
        case Format::RG32_SFLOAT:
            return WGPUVertexFormat_Float32x2;
        case Format::RGB32_UINT:
            return WGPUVertexFormat_Uint32x3;
        case Format::RGB32_SINT:
            return WGPUVertexFormat_Sint32x3;
        case Format::RGB32_SFLOAT:
            return WGPUVertexFormat_Float32x3;
        case Format::RGBA32_UINT:
            return WGPUVertexFormat_Uint32x4;
        case Format::RGBA32_SINT:
            return WGPUVertexFormat_Sint32x4;
        case Format::RGBA32_SFLOAT:
            return WGPUVertexFormat_Float32x4;
        default:
            return WGPUVertexFormat_Force32;
    }
}

WGPUTextureDimension nri::GetTextureDimension(TextureType type) {
    switch (type) {
        case TextureType::TEXTURE_1D:
            return WGPUTextureDimension_1D;
        case TextureType::TEXTURE_3D:
            return WGPUTextureDimension_3D;
        default:
            return WGPUTextureDimension_2D;
    }
}

WGPUTextureViewDimension nri::GetTextureViewDimension(TextureView type, const TextureDesc& textureDesc) {
    switch (type) {
        case TextureView::TEXTURE_ARRAY:
        case TextureView::STORAGE_TEXTURE_ARRAY:
            return textureDesc.type == TextureType::TEXTURE_1D ? WGPUTextureViewDimension_1D : WGPUTextureViewDimension_2DArray;
        case TextureView::TEXTURE_CUBE:
            return WGPUTextureViewDimension_Cube;
        case TextureView::TEXTURE_CUBE_ARRAY:
            return WGPUTextureViewDimension_CubeArray;
        default:
            if (textureDesc.type == TextureType::TEXTURE_1D)
                return WGPUTextureViewDimension_1D;
            if (textureDesc.type == TextureType::TEXTURE_3D)
                return WGPUTextureViewDimension_3D;
            return WGPUTextureViewDimension_2D;
    }
}

WGPUTextureAspect nri::GetTextureAspect(PlaneBits planes) {
    if ((planes & PlaneBits::DEPTH) && (planes & PlaneBits::STENCIL))
        return WGPUTextureAspect_All;

    if (planes & PlaneBits::DEPTH)
        return WGPUTextureAspect_DepthOnly;
    if (planes & PlaneBits::STENCIL)
        return WGPUTextureAspect_StencilOnly;

    return WGPUTextureAspect_All;
}

WGPUAddressMode nri::GetAddressMode(AddressMode mode) {
    switch (mode) {
        case AddressMode::REPEAT:
            return WGPUAddressMode_Repeat;
        case AddressMode::MIRRORED_REPEAT:
            return WGPUAddressMode_MirrorRepeat;
        default:
            return WGPUAddressMode_ClampToEdge;
    }
}

WGPUFilterMode nri::GetFilterMode(Filter filter) {
    return filter == Filter::LINEAR ? WGPUFilterMode_Linear : WGPUFilterMode_Nearest;
}

WGPUMipmapFilterMode nri::GetMipmapFilterMode(Filter filter) {
    return filter == Filter::LINEAR ? WGPUMipmapFilterMode_Linear : WGPUMipmapFilterMode_Nearest;
}

WGPUCompareFunction nri::GetCompareFunction(CompareOp compareOp) {
    switch (compareOp) {
        case CompareOp::ALWAYS:
            return WGPUCompareFunction_Always;
        case CompareOp::NEVER:
            return WGPUCompareFunction_Never;
        case CompareOp::EQUAL:
            return WGPUCompareFunction_Equal;
        case CompareOp::NOT_EQUAL:
            return WGPUCompareFunction_NotEqual;
        case CompareOp::LESS:
            return WGPUCompareFunction_Less;
        case CompareOp::LESS_EQUAL:
            return WGPUCompareFunction_LessEqual;
        case CompareOp::GREATER:
            return WGPUCompareFunction_Greater;
        case CompareOp::GREATER_EQUAL:
            return WGPUCompareFunction_GreaterEqual;
        default:
            return WGPUCompareFunction_Undefined;
    }
}

WGPUStencilOperation nri::GetStencilOperation(StencilOp stencilOp) {
    switch (stencilOp) {
        case StencilOp::KEEP:
            return WGPUStencilOperation_Keep;
        case StencilOp::ZERO:
            return WGPUStencilOperation_Zero;
        case StencilOp::REPLACE:
            return WGPUStencilOperation_Replace;
        case StencilOp::INCREMENT_AND_CLAMP:
            return WGPUStencilOperation_IncrementClamp;
        case StencilOp::DECREMENT_AND_CLAMP:
            return WGPUStencilOperation_DecrementClamp;
        case StencilOp::INVERT:
            return WGPUStencilOperation_Invert;
        case StencilOp::INCREMENT_AND_WRAP:
            return WGPUStencilOperation_IncrementWrap;
        case StencilOp::DECREMENT_AND_WRAP:
            return WGPUStencilOperation_DecrementWrap;
        default:
            return WGPUStencilOperation_Keep;
    }
}

WGPUPrimitiveTopology nri::GetPrimitiveTopology(Topology topology) {
    switch (topology) {
        case Topology::POINT_LIST:
            return WGPUPrimitiveTopology_PointList;
        case Topology::LINE_LIST:
            return WGPUPrimitiveTopology_LineList;
        case Topology::LINE_STRIP:
            return WGPUPrimitiveTopology_LineStrip;
        case Topology::TRIANGLE_STRIP:
            return WGPUPrimitiveTopology_TriangleStrip;
        default:
            return WGPUPrimitiveTopology_TriangleList;
    }
}

WGPUIndexFormat nri::GetIndexFormat(IndexType indexType) {
    return indexType == IndexType::UINT32 ? WGPUIndexFormat_Uint32 : WGPUIndexFormat_Uint16;
}

WGPUFrontFace nri::GetFrontFace(bool frontCounterClockwise) {
    return frontCounterClockwise ? WGPUFrontFace_CCW : WGPUFrontFace_CW;
}

WGPUCullMode nri::GetCullMode(CullMode cullMode) {
    switch (cullMode) {
        case CullMode::FRONT:
            return WGPUCullMode_Front;
        case CullMode::BACK:
            return WGPUCullMode_Back;
        default:
            return WGPUCullMode_None;
    }
}

WGPUBlendFactor nri::GetBlendFactor(BlendFactor blendFactor) {
    switch (blendFactor) {
        case BlendFactor::ZERO:
            return WGPUBlendFactor_Zero;
        case BlendFactor::ONE:
            return WGPUBlendFactor_One;
        case BlendFactor::SRC_COLOR:
            return WGPUBlendFactor_Src;
        case BlendFactor::ONE_MINUS_SRC_COLOR:
            return WGPUBlendFactor_OneMinusSrc;
        case BlendFactor::DST_COLOR:
            return WGPUBlendFactor_Dst;
        case BlendFactor::ONE_MINUS_DST_COLOR:
            return WGPUBlendFactor_OneMinusDst;
        case BlendFactor::SRC_ALPHA:
            return WGPUBlendFactor_SrcAlpha;
        case BlendFactor::ONE_MINUS_SRC_ALPHA:
            return WGPUBlendFactor_OneMinusSrcAlpha;
        case BlendFactor::DST_ALPHA:
            return WGPUBlendFactor_DstAlpha;
        case BlendFactor::ONE_MINUS_DST_ALPHA:
            return WGPUBlendFactor_OneMinusDstAlpha;
        case BlendFactor::CONSTANT_COLOR:
        case BlendFactor::CONSTANT_ALPHA:
            return WGPUBlendFactor_Constant;
        case BlendFactor::ONE_MINUS_CONSTANT_COLOR:
        case BlendFactor::ONE_MINUS_CONSTANT_ALPHA:
            return WGPUBlendFactor_OneMinusConstant;
        case BlendFactor::SRC_ALPHA_SATURATE:
            return WGPUBlendFactor_SrcAlphaSaturated;
        case BlendFactor::SRC1_COLOR:
            return WGPUBlendFactor_Src1;
        case BlendFactor::ONE_MINUS_SRC1_COLOR:
            return WGPUBlendFactor_OneMinusSrc1;
        case BlendFactor::SRC1_ALPHA:
            return WGPUBlendFactor_Src1Alpha;
        case BlendFactor::ONE_MINUS_SRC1_ALPHA:
            return WGPUBlendFactor_OneMinusSrc1Alpha;
        default:
            return WGPUBlendFactor_One;
    }
}

WGPUBlendOperation nri::GetBlendOperation(BlendOp blendOp) {
    switch (blendOp) {
        case BlendOp::SUBTRACT:
            return WGPUBlendOperation_Subtract;
        case BlendOp::REVERSE_SUBTRACT:
            return WGPUBlendOperation_ReverseSubtract;
        case BlendOp::MIN:
            return WGPUBlendOperation_Min;
        case BlendOp::MAX:
            return WGPUBlendOperation_Max;
        default:
            return WGPUBlendOperation_Add;
    }
}

WGPUColorWriteMask nri::GetColorWriteMask(ColorWriteBits colorWriteMask) {
    WGPUColorWriteMask result = WGPUColorWriteMask_None;

    if (colorWriteMask & ColorWriteBits::R)
        result |= WGPUColorWriteMask_Red;
    if (colorWriteMask & ColorWriteBits::G)
        result |= WGPUColorWriteMask_Green;
    if (colorWriteMask & ColorWriteBits::B)
        result |= WGPUColorWriteMask_Blue;
    if (colorWriteMask & ColorWriteBits::A)
        result |= WGPUColorWriteMask_Alpha;

    return result;
}

WGPULoadOp nri::GetLoadOp(LoadOp loadOp) {
    return loadOp == LoadOp::CLEAR ? WGPULoadOp_Clear : WGPULoadOp_Load;
}

WGPUStoreOp nri::GetStoreOp(StoreOp storeOp) {
    return storeOp == StoreOp::DISCARD ? WGPUStoreOp_Discard : WGPUStoreOp_Store;
}

WGPUComponentSwizzle nri::GetComponentSwizzle(ComponentSwizzle componentSwizzle) {
    switch (componentSwizzle) {
        case ComponentSwizzle::ZERO:
            return WGPUComponentSwizzle_Zero;
        case ComponentSwizzle::ONE:
            return WGPUComponentSwizzle_One;
        case ComponentSwizzle::R:
            return WGPUComponentSwizzle_R;
        case ComponentSwizzle::G:
            return WGPUComponentSwizzle_G;
        case ComponentSwizzle::B:
            return WGPUComponentSwizzle_B;
        case ComponentSwizzle::A:
            return WGPUComponentSwizzle_A;
        default:
            return WGPUComponentSwizzle_Undefined;
    }
}

static bool IsColorRenderableWGPU(Format format) {
    switch (format) {
        case Format::R8_UNORM:
        case Format::RG8_UNORM:
        case Format::BGRA8_UNORM:
        case Format::BGRA8_SRGB:
        case Format::RGBA8_UNORM:
        case Format::RGBA8_SRGB:
        case Format::RGBA8_UINT:
        case Format::RGBA8_SINT:
        case Format::R16_UNORM:
        case Format::R16_UINT:
        case Format::R16_SINT:
        case Format::R16_SFLOAT:
        case Format::RG16_UNORM:
        case Format::RG16_UINT:
        case Format::RG16_SINT:
        case Format::RG16_SFLOAT:
        case Format::RGBA16_UNORM:
        case Format::RGBA16_UINT:
        case Format::RGBA16_SINT:
        case Format::RGBA16_SFLOAT:
        case Format::R32_UINT:
        case Format::R32_SINT:
        case Format::R32_SFLOAT:
        case Format::RG32_UINT:
        case Format::RG32_SINT:
        case Format::RG32_SFLOAT:
        case Format::RGBA32_UINT:
        case Format::RGBA32_SINT:
        case Format::RGBA32_SFLOAT:
        case Format::R10_G10_B10_A2_UINT:
        case Format::R10_G10_B10_A2_UNORM:
        case Format::R11_G11_B10_UFLOAT:
            return true;
        default:
            return false;
    }
}

static bool IsBlendSupportedWGPU(Format format) {
    switch (format) {
        case Format::R8_UNORM:
        case Format::RG8_UNORM:
        case Format::BGRA8_UNORM:
        case Format::BGRA8_SRGB:
        case Format::RGBA8_UNORM:
        case Format::RGBA8_SRGB:
        case Format::R16_UNORM:
        case Format::R16_SFLOAT:
        case Format::RG16_UNORM:
        case Format::RG16_SFLOAT:
        case Format::RGBA16_UNORM:
        case Format::RGBA16_SFLOAT:
        case Format::R11_G11_B10_UFLOAT:
            return true;
        default:
            return false;
    }
}

static bool IsStorageTextureSupportedWGPU(Format format) {
    switch (format) {
        case Format::RGBA8_UNORM:
        case Format::RGBA8_SNORM:
        case Format::RGBA8_UINT:
        case Format::RGBA8_SINT:
        case Format::RGBA16_UINT:
        case Format::RGBA16_SINT:
        case Format::RGBA16_SFLOAT:
        case Format::R32_UINT:
        case Format::R32_SINT:
        case Format::R32_SFLOAT:
        case Format::RG32_UINT:
        case Format::RG32_SINT:
        case Format::RG32_SFLOAT:
        case Format::RGBA32_UINT:
        case Format::RGBA32_SINT:
        case Format::RGBA32_SFLOAT:
            return true;
        default:
            return false;
    }
}

FormatSupportBits nri::GetFormatSupportWGPU(Format format) {
    if (GetTextureFormat(format) == WGPUTextureFormat_Undefined)
        return FormatSupportBits::UNSUPPORTED;

    const FormatProps& props = GetFormatProps(format);
    FormatSupportBits support = FormatSupportBits::TEXTURE;

    if (IsStorageTextureSupportedWGPU(format))
        support |= FormatSupportBits::STORAGE_TEXTURE;
    if (!props.isCompressed && !props.isDepth && !props.isStencil) {
        if (GetVertexFormat(format) != WGPUVertexFormat_Force32)
            support |= FormatSupportBits::VERTEX_BUFFER;
    }

    if (IsColorRenderableWGPU(format))
        support |= FormatSupportBits::COLOR_ATTACHMENT | FormatSupportBits::MULTISAMPLE_4X | FormatSupportBits::MULTISAMPLE_RESOLVE;
    if (IsBlendSupportedWGPU(format))
        support |= FormatSupportBits::BLEND;
    if (props.isDepth || props.isStencil)
        support |= FormatSupportBits::DEPTH_STENCIL_ATTACHMENT;

    return support;
}

Vendor nri::GetVendorFromPCIID(uint32_t vendorId) {
    switch (vendorId) {
        case 0x10DE:
            return Vendor::NVIDIA;
        case 0x1002:
        case 0x1022:
            return Vendor::AMD;
        case 0x8086:
            return Vendor::INTEL;
        default:
            return Vendor::UNKNOWN;
    }
}

Architecture nri::GetArchitecture(WGPUAdapterType adapterType) {
    switch (adapterType) {
        case WGPUAdapterType_DiscreteGPU:
            return Architecture::DISCRETE;
        case WGPUAdapterType_IntegratedGPU:
            return Architecture::INTEGRATED;
        case WGPUAdapterType_CPU:
            return Architecture::SOFTWARE;
        default:
            return Architecture::UNKNOWN;
    }
}
