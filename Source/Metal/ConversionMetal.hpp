// © 2026 NVIDIA Corporation

struct FormatMetal {
    MTL::PixelFormat pixelFormat;
    MTL::VertexFormat vertexFormat;
};

constexpr std::array<FormatMetal, (size_t)Format::MAX_NUM> g_Formats = {{
    {MTL::PixelFormatInvalid, MTL::VertexFormatInvalid},                    // UNKNOWN
    {MTL::PixelFormatR8Unorm, MTL::VertexFormatUCharNormalized},            // R8_UNORM
    {MTL::PixelFormatR8Snorm, MTL::VertexFormatCharNormalized},             // R8_SNORM
    {MTL::PixelFormatR8Uint, MTL::VertexFormatUChar},                       // R8_UINT
    {MTL::PixelFormatR8Sint, MTL::VertexFormatChar},                        // R8_SINT
    {MTL::PixelFormatRG8Unorm, MTL::VertexFormatUChar2Normalized},          // RG8_UNORM
    {MTL::PixelFormatRG8Snorm, MTL::VertexFormatChar2Normalized},           // RG8_SNORM
    {MTL::PixelFormatRG8Uint, MTL::VertexFormatUChar2},                     // RG8_UINT
    {MTL::PixelFormatRG8Sint, MTL::VertexFormatChar2},                      // RG8_SINT
    {MTL::PixelFormatBGRA8Unorm, MTL::VertexFormatUChar4Normalized_BGRA},   // BGRA8_UNORM
    {MTL::PixelFormatBGRA8Unorm_sRGB, MTL::VertexFormatInvalid},            // BGRA8_SRGB
    {MTL::PixelFormatRGBA8Unorm, MTL::VertexFormatUChar4Normalized},        // RGBA8_UNORM
    {MTL::PixelFormatRGBA8Unorm_sRGB, MTL::VertexFormatInvalid},            // RGBA8_SRGB
    {MTL::PixelFormatRGBA8Snorm, MTL::VertexFormatChar4Normalized},         // RGBA8_SNORM
    {MTL::PixelFormatRGBA8Uint, MTL::VertexFormatUChar4},                   // RGBA8_UINT
    {MTL::PixelFormatRGBA8Sint, MTL::VertexFormatChar4},                    // RGBA8_SINT
    {MTL::PixelFormatR16Unorm, MTL::VertexFormatUShortNormalized},          // R16_UNORM
    {MTL::PixelFormatR16Snorm, MTL::VertexFormatShortNormalized},           // R16_SNORM
    {MTL::PixelFormatR16Uint, MTL::VertexFormatUShort},                     // R16_UINT
    {MTL::PixelFormatR16Sint, MTL::VertexFormatShort},                      // R16_SINT
    {MTL::PixelFormatR16Float, MTL::VertexFormatHalf},                      // R16_SFLOAT
    {MTL::PixelFormatRG16Unorm, MTL::VertexFormatUShort2Normalized},        // RG16_UNORM
    {MTL::PixelFormatRG16Snorm, MTL::VertexFormatShort2Normalized},         // RG16_SNORM
    {MTL::PixelFormatRG16Uint, MTL::VertexFormatUShort2},                   // RG16_UINT
    {MTL::PixelFormatRG16Sint, MTL::VertexFormatShort2},                    // RG16_SINT
    {MTL::PixelFormatRG16Float, MTL::VertexFormatHalf2},                    // RG16_SFLOAT
    {MTL::PixelFormatRGBA16Unorm, MTL::VertexFormatUShort4Normalized},      // RGBA16_UNORM
    {MTL::PixelFormatRGBA16Snorm, MTL::VertexFormatShort4Normalized},       // RGBA16_SNORM
    {MTL::PixelFormatRGBA16Uint, MTL::VertexFormatUShort4},                 // RGBA16_UINT
    {MTL::PixelFormatRGBA16Sint, MTL::VertexFormatShort4},                  // RGBA16_SINT
    {MTL::PixelFormatRGBA16Float, MTL::VertexFormatHalf4},                  // RGBA16_SFLOAT
    {MTL::PixelFormatR32Uint, MTL::VertexFormatUInt},                       // R32_UINT
    {MTL::PixelFormatR32Sint, MTL::VertexFormatInt},                        // R32_SINT
    {MTL::PixelFormatR32Float, MTL::VertexFormatFloat},                     // R32_SFLOAT
    {MTL::PixelFormatRG32Uint, MTL::VertexFormatUInt2},                     // RG32_UINT
    {MTL::PixelFormatRG32Sint, MTL::VertexFormatInt2},                      // RG32_SINT
    {MTL::PixelFormatRG32Float, MTL::VertexFormatFloat2},                   // RG32_SFLOAT
    {MTL::PixelFormatInvalid, MTL::VertexFormatUInt3},                      // RGB32_UINT
    {MTL::PixelFormatInvalid, MTL::VertexFormatInt3},                       // RGB32_SINT
    {MTL::PixelFormatInvalid, MTL::VertexFormatFloat3},                     // RGB32_SFLOAT
    {MTL::PixelFormatRGBA32Uint, MTL::VertexFormatUInt4},                   // RGBA32_UINT
    {MTL::PixelFormatRGBA32Sint, MTL::VertexFormatInt4},                    // RGBA32_SINT
    {MTL::PixelFormatRGBA32Float, MTL::VertexFormatFloat4},                 // RGBA32_SFLOAT
    {MTL::PixelFormatB5G6R5Unorm, MTL::VertexFormatInvalid},                // B5_G6_R5_UNORM
    {MTL::PixelFormatBGR5A1Unorm, MTL::VertexFormatInvalid},                // B5_G5_R5_A1_UNORM
    {MTL::PixelFormatInvalid, MTL::VertexFormatInvalid},                    // B4_G4_R4_A4_UNORM (Metal only has "ABGR4Unorm", which has a different channel order)
    {MTL::PixelFormatRGB10A2Unorm, MTL::VertexFormatUInt1010102Normalized}, // R10_G10_B10_A2_UNORM
    {MTL::PixelFormatRGB10A2Uint, MTL::VertexFormatInvalid},                // R10_G10_B10_A2_UINT
    {MTL::PixelFormatRG11B10Float, MTL::VertexFormatFloatRG11B10},          // R11_G11_B10_UFLOAT
    {MTL::PixelFormatRGB9E5Float, MTL::VertexFormatFloatRGB9E5},            // R9_G9_B9_E5_UFLOAT
    {MTL::PixelFormatInvalid, MTL::VertexFormatInvalid},                    // NV12_UNORM
    {MTL::PixelFormatInvalid, MTL::VertexFormatInvalid},                    // P010_UNORM
    {MTL::PixelFormatInvalid, MTL::VertexFormatInvalid},                    // P016_UNORM
    {MTL::PixelFormatBC1_RGBA, MTL::VertexFormatInvalid},                   // BC1_RGBA_UNORM
    {MTL::PixelFormatBC1_RGBA_sRGB, MTL::VertexFormatInvalid},              // BC1_RGBA_SRGB
    {MTL::PixelFormatBC2_RGBA, MTL::VertexFormatInvalid},                   // BC2_RGBA_UNORM
    {MTL::PixelFormatBC2_RGBA_sRGB, MTL::VertexFormatInvalid},              // BC2_RGBA_SRGB
    {MTL::PixelFormatBC3_RGBA, MTL::VertexFormatInvalid},                   // BC3_RGBA_UNORM
    {MTL::PixelFormatBC3_RGBA_sRGB, MTL::VertexFormatInvalid},              // BC3_RGBA_SRGB
    {MTL::PixelFormatBC4_RUnorm, MTL::VertexFormatInvalid},                 // BC4_R_UNORM
    {MTL::PixelFormatBC4_RSnorm, MTL::VertexFormatInvalid},                 // BC4_R_SNORM
    {MTL::PixelFormatBC5_RGUnorm, MTL::VertexFormatInvalid},                // BC5_RG_UNORM
    {MTL::PixelFormatBC5_RGSnorm, MTL::VertexFormatInvalid},                // BC5_RG_SNORM
    {MTL::PixelFormatBC6H_RGBUfloat, MTL::VertexFormatInvalid},             // BC6H_RGB_UFLOAT
    {MTL::PixelFormatBC6H_RGBFloat, MTL::VertexFormatInvalid},              // BC6H_RGB_SFLOAT
    {MTL::PixelFormatBC7_RGBAUnorm, MTL::VertexFormatInvalid},              // BC7_RGBA_UNORM
    {MTL::PixelFormatBC7_RGBAUnorm_sRGB, MTL::VertexFormatInvalid},         // BC7_RGBA_SRGB
    {MTL::PixelFormatETC2_RGB8, MTL::VertexFormatInvalid},                  // ETC2_RGB8_UNORM
    {MTL::PixelFormatETC2_RGB8_sRGB, MTL::VertexFormatInvalid},             // ETC2_RGB8_SRGB
    {MTL::PixelFormatETC2_RGB8A1, MTL::VertexFormatInvalid},                // ETC2_RGB8_A1_UNORM
    {MTL::PixelFormatETC2_RGB8A1_sRGB, MTL::VertexFormatInvalid},           // ETC2_RGB8_A1_SRGB
    {MTL::PixelFormatEAC_RGBA8, MTL::VertexFormatInvalid},                  // ETC2_RGB8_A8_UNORM
    {MTL::PixelFormatEAC_RGBA8_sRGB, MTL::VertexFormatInvalid},             // ETC2_RGB8_A8_SRGB
    {MTL::PixelFormatEAC_R11Unorm, MTL::VertexFormatInvalid},               // ETC2_R11_UNORM
    {MTL::PixelFormatEAC_R11Snorm, MTL::VertexFormatInvalid},               // ETC2_R11_SNORM
    {MTL::PixelFormatEAC_RG11Unorm, MTL::VertexFormatInvalid},              // ETC2_R11_G11_UNORM
    {MTL::PixelFormatEAC_RG11Snorm, MTL::VertexFormatInvalid},              // ETC2_R11_G11_SNORM
    {MTL::PixelFormatASTC_4x4_LDR, MTL::VertexFormatInvalid},               // ASTC_4X4_UNORM
    {MTL::PixelFormatASTC_4x4_sRGB, MTL::VertexFormatInvalid},              // ASTC_4X4_SRGB
    {MTL::PixelFormatASTC_5x4_LDR, MTL::VertexFormatInvalid},               // ASTC_5X4_UNORM
    {MTL::PixelFormatASTC_5x4_sRGB, MTL::VertexFormatInvalid},              // ASTC_5X4_SRGB
    {MTL::PixelFormatASTC_5x5_LDR, MTL::VertexFormatInvalid},               // ASTC_5X5_UNORM
    {MTL::PixelFormatASTC_5x5_sRGB, MTL::VertexFormatInvalid},              // ASTC_5X5_SRGB
    {MTL::PixelFormatASTC_6x5_LDR, MTL::VertexFormatInvalid},               // ASTC_6X5_UNORM
    {MTL::PixelFormatASTC_6x5_sRGB, MTL::VertexFormatInvalid},              // ASTC_6X5_SRGB
    {MTL::PixelFormatASTC_6x6_LDR, MTL::VertexFormatInvalid},               // ASTC_6X6_UNORM
    {MTL::PixelFormatASTC_6x6_sRGB, MTL::VertexFormatInvalid},              // ASTC_6X6_SRGB
    {MTL::PixelFormatASTC_8x5_LDR, MTL::VertexFormatInvalid},               // ASTC_8X5_UNORM
    {MTL::PixelFormatASTC_8x5_sRGB, MTL::VertexFormatInvalid},              // ASTC_8X5_SRGB
    {MTL::PixelFormatASTC_8x6_LDR, MTL::VertexFormatInvalid},               // ASTC_8X6_UNORM
    {MTL::PixelFormatASTC_8x6_sRGB, MTL::VertexFormatInvalid},              // ASTC_8X6_SRGB
    {MTL::PixelFormatASTC_8x8_LDR, MTL::VertexFormatInvalid},               // ASTC_8X8_UNORM
    {MTL::PixelFormatASTC_8x8_sRGB, MTL::VertexFormatInvalid},              // ASTC_8X8_SRGB
    {MTL::PixelFormatASTC_10x5_LDR, MTL::VertexFormatInvalid},              // ASTC_10X5_UNORM
    {MTL::PixelFormatASTC_10x5_sRGB, MTL::VertexFormatInvalid},             // ASTC_10X5_SRGB
    {MTL::PixelFormatASTC_10x6_LDR, MTL::VertexFormatInvalid},              // ASTC_10X6_UNORM
    {MTL::PixelFormatASTC_10x6_sRGB, MTL::VertexFormatInvalid},             // ASTC_10X6_SRGB
    {MTL::PixelFormatASTC_10x8_LDR, MTL::VertexFormatInvalid},              // ASTC_10X8_UNORM
    {MTL::PixelFormatASTC_10x8_sRGB, MTL::VertexFormatInvalid},             // ASTC_10X8_SRGB
    {MTL::PixelFormatASTC_10x10_LDR, MTL::VertexFormatInvalid},             // ASTC_10X10_UNORM
    {MTL::PixelFormatASTC_10x10_sRGB, MTL::VertexFormatInvalid},            // ASTC_10X10_SRGB
    {MTL::PixelFormatASTC_12x10_LDR, MTL::VertexFormatInvalid},             // ASTC_12X10_UNORM
    {MTL::PixelFormatASTC_12x10_sRGB, MTL::VertexFormatInvalid},            // ASTC_12X10_SRGB
    {MTL::PixelFormatASTC_12x12_LDR, MTL::VertexFormatInvalid},             // ASTC_12X12_UNORM
    {MTL::PixelFormatASTC_12x12_sRGB, MTL::VertexFormatInvalid},            // ASTC_12X12_SRGB
    {MTL::PixelFormatDepth16Unorm, MTL::VertexFormatInvalid},               // D16_UNORM
    {MTL::PixelFormatDepth32Float, MTL::VertexFormatInvalid},               // D32_SFLOAT
    {MTL::PixelFormatInvalid, MTL::VertexFormatInvalid},                    // D24_UNORM_S8_UINT (unavailable on Apple GPUs)
    {MTL::PixelFormatDepth32Float_Stencil8, MTL::VertexFormatInvalid},      // D32_SFLOAT_S8_UINT
}};
NRI_VALIDATE_ARRAY_BY_FIELD(g_Formats, pixelFormat);

MTL::PixelFormat nri::GetPixelFormatMetal(Format format) {
    return g_Formats[(size_t)format].pixelFormat;
}

MTL::VertexFormat nri::GetVertexFormatMetal(Format format) {
    return g_Formats[(size_t)format].vertexFormat;
}

// Metal Feature Set Tables (May 21, 2026): "Texture capabilities by pixel format" and "Texture buffer pixel formats" for Apple7+
nri::FormatSupportBits nri::GetFormatSupportMetal(MTL::Device& device, Format format) {
    const MTL::PixelFormat pixelFormat = GetPixelFormatMetal(format);
    const MTL::VertexFormat vertexFormat = GetVertexFormatMetal(format);
    const FormatProps& props = GetFormatProps(format);

    FormatSupportBits support = FormatSupportBits::UNSUPPORTED;

    if (vertexFormat != MTL::VertexFormatInvalid)
        support |= FormatSupportBits::VERTEX_BUFFER;

    if (pixelFormat == MTL::PixelFormatInvalid)
        return support;

    const bool isBc = format >= Format::BC1_RGBA_UNORM && format <= Format::BC7_RGBA_SRGB;

    if (isBc && !device.supportsBCTextureCompression())
        return support;

    support |= FormatSupportBits::TEXTURE;

    if (!props.isDepth && !props.isStencil)
        support |= FormatSupportBits::HOST_COPY;

    // Compressed formats can't be multisampled, rendered to or written
    if (props.isCompressed)
        return support;

    FormatSupportBits multisample = FormatSupportBits::UNSUPPORTED;

    if (device.supportsTextureSampleCount(2))
        multisample |= FormatSupportBits::MULTISAMPLE_2X;

    if (device.supportsTextureSampleCount(4))
        multisample |= FormatSupportBits::MULTISAMPLE_4X;

    if (device.supportsTextureSampleCount(8))
        multisample |= FormatSupportBits::MULTISAMPLE_8X;

    // Depth resolves support MIN/MAX (not AVERAGE)
    if (props.isDepth || props.isStencil)
        return support | FormatSupportBits::DEPTH_STENCIL_ATTACHMENT | FormatSupportBits::MULTISAMPLE_RESOLVE | multisample;

    // Every ordinary and packed color format is a renderable MSAA target on Apple7+
    support |= FormatSupportBits::COLOR_ATTACHMENT | multisample;

    const bool isPacked16 = props.stride == 2 && props.isPacked;

    if (!props.isInteger)
        support |= FormatSupportBits::BLEND;

    // Integer formats have no native "Resolve" capability, but MIN/MAX resolves are done by a shader (AVERAGE is unsupported, like in VK)
    support |= FormatSupportBits::MULTISAMPLE_RESOLVE;

    // Packed 16-bit formats have no "Write" capability. sRGB and shared exponent formats are not exposed as storage for parity with other backends
    if (!isPacked16 && !props.isSrgb && !props.isExpShared)
        support |= FormatSupportBits::STORAGE_TEXTURE;

    // Texture buffers don't support packed 16-bit, sRGB, BGRA and shared exponent formats
    if (!isPacked16 && !props.isSrgb && !props.isBgr && !props.isExpShared)
        support |= FormatSupportBits::BUFFER | FormatSupportBits::STORAGE_BUFFER;

    if (format == Format::R32_UINT || format == Format::R32_SINT)
        support |= FormatSupportBits::STORAGE_TEXTURE_ATOMICS | FormatSupportBits::STORAGE_BUFFER_ATOMICS;

    // MSL texture access types don't declare the pixel format
    if (support & (FormatSupportBits::STORAGE_TEXTURE | FormatSupportBits::STORAGE_BUFFER))
        support |= FormatSupportBits::STORAGE_READ_WITHOUT_FORMAT | FormatSupportBits::STORAGE_WRITE_WITHOUT_FORMAT;

    return support;
}

MTL::CompareFunction nri::GetCompareMetal(CompareOp compareOp) {
    constexpr std::array<MTL::CompareFunction, (size_t)CompareOp::MAX_NUM> g_CompareFunctions = {
        MTL::CompareFunctionAlways, MTL::CompareFunctionAlways, MTL::CompareFunctionNever, MTL::CompareFunctionEqual, MTL::CompareFunctionNotEqual,
        MTL::CompareFunctionLess, MTL::CompareFunctionLessEqual, MTL::CompareFunctionGreater, MTL::CompareFunctionGreaterEqual};
    NRI_VALIDATE_ARRAY(g_CompareFunctions);

    return g_CompareFunctions[(uint32_t)compareOp];
}
