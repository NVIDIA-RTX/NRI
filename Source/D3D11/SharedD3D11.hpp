// © 2021 NVIDIA Corporation

static inline D3D11_TEXTURE_ADDRESS_MODE GetAddressMode(AddressMode mode) {
    return (D3D11_TEXTURE_ADDRESS_MODE)(D3D11_TEXTURE_ADDRESS_WRAP + (uint32_t)mode);
}

static inline D3D11_FILTER GetFilterIsotropic(Filter mip, Filter magnification, Filter minification, ReductionMode filterExt, bool useComparison) {
    uint32_t combinedMask = mip == Filter::LINEAR ? 0x1 : 0;
    combinedMask |= magnification == Filter::LINEAR ? 0x4 : 0;
    combinedMask |= minification == Filter::LINEAR ? 0x10 : 0;

    if (useComparison)
        combinedMask |= 0x80;
    else if (filterExt == ReductionMode::MIN)
        combinedMask |= 0x100;
    else if (filterExt == ReductionMode::MAX)
        combinedMask |= 0x180;

    return (D3D11_FILTER)combinedMask;
}

static inline D3D11_FILTER GetFilterAnisotropic(ReductionMode filterExt, bool useComparison) {
    if (filterExt == ReductionMode::MIN)
        return D3D11_FILTER_MINIMUM_ANISOTROPIC;
    else if (filterExt == ReductionMode::MAX)
        return D3D11_FILTER_MAXIMUM_ANISOTROPIC;

    return useComparison ? D3D11_FILTER_COMPARISON_ANISOTROPIC : D3D11_FILTER_ANISOTROPIC;
}

constexpr std::array<D3D11_LOGIC_OP, (size_t)LogicOp::MAX_NUM> g_LogicOps = {
    D3D11_LOGIC_OP_CLEAR,         // NONE,
    D3D11_LOGIC_OP_CLEAR,         // CLEAR,
    D3D11_LOGIC_OP_AND,           // AND,
    D3D11_LOGIC_OP_AND_REVERSE,   // AND_REVERSE,
    D3D11_LOGIC_OP_COPY,          // COPY,
    D3D11_LOGIC_OP_AND_INVERTED,  // AND_INVERTED,
    D3D11_LOGIC_OP_XOR,           // XOR,
    D3D11_LOGIC_OP_OR,            // OR,
    D3D11_LOGIC_OP_NOR,           // NOR,
    D3D11_LOGIC_OP_EQUIV,         // EQUIVALENT,
    D3D11_LOGIC_OP_INVERT,        // INVERT,
    D3D11_LOGIC_OP_OR_REVERSE,    // OR_REVERSE,
    D3D11_LOGIC_OP_COPY_INVERTED, // COPY_INVERTED,
    D3D11_LOGIC_OP_OR_INVERTED,   // OR_INVERTED,
    D3D11_LOGIC_OP_NAND,          // NAND,
    D3D11_LOGIC_OP_SET            // SET
};
VALIDATE_ARRAY(g_LogicOps);

D3D11_LOGIC_OP nri::GetD3D11LogicOp(LogicOp logicOp) {
    return g_LogicOps[(size_t)logicOp];
}

constexpr std::array<D3D11_BLEND_OP, (size_t)BlendOp::MAX_NUM> g_BlendOps = {
    D3D11_BLEND_OP_ADD,          // ADD,
    D3D11_BLEND_OP_SUBTRACT,     // SUBTRACT,
    D3D11_BLEND_OP_REV_SUBTRACT, // REVERSE_SUBTRACT,
    D3D11_BLEND_OP_MIN,          // MIN,
    D3D11_BLEND_OP_MAX           // MAX
};
VALIDATE_ARRAY(g_BlendOps);

D3D11_BLEND_OP nri::GetD3D11BlendOp(BlendOp blendFunc) {
    return g_BlendOps[(size_t)blendFunc];
}

constexpr std::array<D3D11_BLEND, (size_t)BlendFactor::MAX_NUM> g_BlendFactors = {
    D3D11_BLEND_ZERO,             // ZERO,
    D3D11_BLEND_ONE,              // ONE,
    D3D11_BLEND_SRC_COLOR,        // SRC_COLOR,
    D3D11_BLEND_INV_SRC_COLOR,    // ONE_MINUS_SRC_COLOR,
    D3D11_BLEND_DEST_COLOR,       // DST_COLOR,
    D3D11_BLEND_INV_DEST_COLOR,   // ONE_MINUS_DST_COLOR,
    D3D11_BLEND_SRC_ALPHA,        // SRC_ALPHA,
    D3D11_BLEND_INV_SRC_ALPHA,    // ONE_MINUS_SRC_ALPHA,
    D3D11_BLEND_DEST_ALPHA,       // DST_ALPHA,
    D3D11_BLEND_INV_DEST_ALPHA,   // ONE_MINUS_DST_ALPHA,
    D3D11_BLEND_BLEND_FACTOR,     // CONSTANT_COLOR,
    D3D11_BLEND_INV_BLEND_FACTOR, // ONE_MINUS_CONSTANT_COLOR,
    D3D11_BLEND_BLEND_FACTOR,     // CONSTANT_ALPHA,
    D3D11_BLEND_INV_BLEND_FACTOR, // ONE_MINUS_CONSTANT_ALPHA,
    D3D11_BLEND_SRC_ALPHA_SAT,    // SRC_ALPHA_SATURATE,
    D3D11_BLEND_SRC1_COLOR,       // SRC1_COLOR,
    D3D11_BLEND_INV_SRC1_COLOR,   // ONE_MINUS_SRC1_COLOR,
    D3D11_BLEND_SRC1_ALPHA,       // SRC1_ALPHA,
    D3D11_BLEND_INV_SRC1_ALPHA    // ONE_MINUS_SRC1_ALPHA,
};
VALIDATE_ARRAY(g_BlendFactors);

D3D11_BLEND nri::GetD3D11BlendFromBlendFactor(BlendFactor blendFactor) {
    return g_BlendFactors[(size_t)blendFactor];
}

constexpr std::array<D3D11_STENCIL_OP, (size_t)StencilOp::MAX_NUM> g_StencilOps = {
    D3D11_STENCIL_OP_KEEP,     // KEEP,
    D3D11_STENCIL_OP_ZERO,     // ZERO,
    D3D11_STENCIL_OP_REPLACE,  // REPLACE,
    D3D11_STENCIL_OP_INCR_SAT, // INCREMENT_AND_CLAMP,
    D3D11_STENCIL_OP_DECR_SAT, // DECREMENT_AND_CLAMP,
    D3D11_STENCIL_OP_INVERT,   // INVERT,
    D3D11_STENCIL_OP_INCR,     // INCREMENT_AND_WRAP,
    D3D11_STENCIL_OP_DECR      // DECREMENT_AND_WRAP
};
VALIDATE_ARRAY(g_StencilOps);

D3D11_STENCIL_OP nri::GetD3D11StencilOpFromStencilOp(StencilOp stencilFunc) {
    return g_StencilOps[(size_t)stencilFunc];
}

constexpr std::array<D3D11_COMPARISON_FUNC, (size_t)CompareOp::MAX_NUM> g_ComparisonFuncs = {
    D3D11_COMPARISON_FUNC(0),       // NONE,
    D3D11_COMPARISON_ALWAYS,        // ALWAYS,
    D3D11_COMPARISON_NEVER,         // NEVER,
    D3D11_COMPARISON_EQUAL,         // EQUAL,
    D3D11_COMPARISON_NOT_EQUAL,     // NOT_EQUAL,
    D3D11_COMPARISON_LESS,          // LESS,
    D3D11_COMPARISON_LESS_EQUAL,    // LESS_EQUAL,
    D3D11_COMPARISON_GREATER,       // GREATER,
    D3D11_COMPARISON_GREATER_EQUAL, // GREATER_EQUAL,
};
VALIDATE_ARRAY(g_ComparisonFuncs);

D3D11_COMPARISON_FUNC nri::GetD3D11ComparisonFuncFromCompareOp(CompareOp compareOp) {
    return g_ComparisonFuncs[(size_t)compareOp];
}

constexpr std::array<D3D11_CULL_MODE, (size_t)CullMode::MAX_NUM> g_CullModes = {
    D3D11_CULL_NONE,  // NONE,
    D3D11_CULL_FRONT, // FRONT,
    D3D11_CULL_BACK   // BACK
};
VALIDATE_ARRAY(g_CullModes);

D3D11_CULL_MODE nri::GetD3D11CullModeFromCullMode(CullMode cullMode) {
    return g_CullModes[(size_t)cullMode];
}

constexpr std::array<D3D_PRIMITIVE_TOPOLOGY, (size_t)Topology::MAX_NUM> g_Topologies = {
    D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,                // POINT_LIST,
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST,                 // LINE_LIST,
    D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,                // LINE_STRIP,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,             // TRIANGLE_LIST,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,            // TRIANGLE_STRIP,
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ,             // LINE_LIST_WITH_ADJACENCY,
    D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ,            // LINE_STRIP_WITH_ADJACENCY,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ,         // TRIANGLE_LIST_WITH_ADJACENCY,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ,        // TRIANGLE_STRIP_WITH_ADJACENCY,
    D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST // PATCH_LIST
};
VALIDATE_ARRAY(g_Topologies);

D3D11_PRIMITIVE_TOPOLOGY nri::GetD3D11TopologyFromTopology(Topology topology, uint32_t patchPoints) {
    uint32_t res = g_Topologies[(size_t)topology];

    if (res == D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST)
        res += patchPoints - 1;

    return (D3D11_PRIMITIVE_TOPOLOGY)res;
}

void nri::FillSamplerDesc(const SamplerDesc& samplerDesc, D3D11_SAMPLER_DESC& desc) {
    bool isAnisotropy = samplerDesc.anisotropy > 1;
    bool isComparison = samplerDesc.compareOp != CompareOp::NONE;

    desc = {};
    desc.AddressU = GetAddressMode(samplerDesc.addressModes.u);
    desc.AddressV = GetAddressMode(samplerDesc.addressModes.v);
    desc.AddressW = GetAddressMode(samplerDesc.addressModes.w);
    desc.MipLODBias = samplerDesc.mipBias;
    desc.MaxAnisotropy = samplerDesc.anisotropy;
    desc.ComparisonFunc = GetD3D11ComparisonFuncFromCompareOp(samplerDesc.compareOp);
    desc.MinLOD = samplerDesc.mipMin;
    desc.MaxLOD = samplerDesc.mipMax;
    desc.Filter = isAnisotropy
        ? GetFilterAnisotropic(samplerDesc.filters.ext, isComparison)
        : GetFilterIsotropic(samplerDesc.filters.mip, samplerDesc.filters.mag, samplerDesc.filters.min, samplerDesc.filters.ext, isComparison);

    if (!samplerDesc.isInteger) { // TODO: the spec is not clear about the behavior, keep black
        desc.BorderColor[0] = samplerDesc.borderColor.f.x;
        desc.BorderColor[1] = samplerDesc.borderColor.f.y;
        desc.BorderColor[2] = samplerDesc.borderColor.f.z;
        desc.BorderColor[3] = samplerDesc.borderColor.f.w;
    }
}

bool nri::GetTextureDesc(const TextureD3D11Desc& textureD3D11Desc, TextureDesc& textureDesc) {
    textureDesc = {};

    ID3D11Resource* resource = textureD3D11Desc.d3d11Resource;
    if (!resource)
        return false;

    D3D11_RESOURCE_DIMENSION type;
    resource->GetType(&type);

    if (type < D3D11_RESOURCE_DIMENSION_TEXTURE1D)
        return false;

    uint32_t bindFlags = 0;
    if (type == D3D11_RESOURCE_DIMENSION_TEXTURE1D) {
        ID3D11Texture1D* texture = (ID3D11Texture1D*)resource;
        D3D11_TEXTURE1D_DESC desc = {};
        texture->GetDesc(&desc);

        textureDesc.width = (Dim_t)desc.Width;
        textureDesc.height = 1;
        textureDesc.depth = 1;
        textureDesc.mipNum = (Dim_t)desc.MipLevels;
        textureDesc.layerNum = (Dim_t)desc.ArraySize;
        textureDesc.sampleNum = 1;
        textureDesc.type = TextureType::TEXTURE_1D;
        textureDesc.format = DXGIFormatToNRIFormat(desc.Format);

        bindFlags = desc.BindFlags;
    } else if (type == D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
        ID3D11Texture2D* texture = (ID3D11Texture2D*)resource;
        D3D11_TEXTURE2D_DESC desc = {};
        texture->GetDesc(&desc);

        textureDesc.width = (Dim_t)desc.Width;
        textureDesc.height = (Dim_t)desc.Height;
        textureDesc.depth = 1;
        textureDesc.mipNum = (Dim_t)desc.MipLevels;
        textureDesc.layerNum = (Dim_t)desc.ArraySize;
        textureDesc.sampleNum = (Sample_t)desc.SampleDesc.Count;
        textureDesc.type = TextureType::TEXTURE_2D;
        textureDesc.format = DXGIFormatToNRIFormat(desc.Format);

        bindFlags = desc.BindFlags;
    } else if (type == D3D11_RESOURCE_DIMENSION_TEXTURE3D) {
        ID3D11Texture3D* texture = (ID3D11Texture3D*)resource;
        D3D11_TEXTURE3D_DESC desc = {};
        texture->GetDesc(&desc);

        textureDesc.width = (Dim_t)desc.Width;
        textureDesc.height = (Dim_t)desc.Height;
        textureDesc.depth = (Dim_t)desc.Depth;
        textureDesc.mipNum = (Dim_t)desc.MipLevels;
        textureDesc.layerNum = 1;
        textureDesc.sampleNum = 1;
        textureDesc.type = TextureType::TEXTURE_3D;
        textureDesc.format = DXGIFormatToNRIFormat(desc.Format);

        bindFlags = desc.BindFlags;
    }

    if (bindFlags & D3D11_BIND_RENDER_TARGET)
        textureDesc.usage |= TextureUsageBits::COLOR_ATTACHMENT;
    if (bindFlags & D3D11_BIND_DEPTH_STENCIL)
        textureDesc.usage |= TextureUsageBits::DEPTH_STENCIL_ATTACHMENT;
    if (bindFlags & D3D11_BIND_SHADER_RESOURCE)
        textureDesc.usage |= TextureUsageBits::SHADER_RESOURCE;
    if (bindFlags & D3D11_BIND_UNORDERED_ACCESS)
        textureDesc.usage |= TextureUsageBits::SHADER_RESOURCE_STORAGE;

    if (textureD3D11Desc.format)
        textureDesc.format = DXGIFormatToNRIFormat(textureD3D11Desc.format);

    return true;
}

bool nri::GetBufferDesc(const BufferD3D11Desc& bufferD3D11Desc, BufferDesc& bufferDesc) {
    bufferDesc = {};

    ID3D11Resource* resource = bufferD3D11Desc.d3d11Resource;
    if (!resource)
        return false;

    D3D11_RESOURCE_DIMENSION type;
    resource->GetType(&type);

    if (type != D3D11_RESOURCE_DIMENSION_BUFFER)
        return false;

    ID3D11Buffer* buffer = (ID3D11Buffer*)resource;
    D3D11_BUFFER_DESC desc = {};
    buffer->GetDesc(&desc);

    bufferDesc.size = desc.ByteWidth;
    bufferDesc.structureStride = desc.StructureByteStride;

    if (desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
        bufferDesc.usage |= BufferUsageBits::VERTEX_BUFFER;
    if (desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
        bufferDesc.usage |= BufferUsageBits::INDEX_BUFFER;
    if (desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
        bufferDesc.usage |= BufferUsageBits::CONSTANT_BUFFER;
    if (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
        bufferDesc.usage |= BufferUsageBits::SHADER_RESOURCE;
    if (desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
        bufferDesc.usage |= BufferUsageBits::SHADER_RESOURCE_STORAGE;
    if (desc.MiscFlags & D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS)
        bufferDesc.usage |= BufferUsageBits::ARGUMENT_BUFFER;

    return true;
}

uint32_t nri::ConvertPriority(float priority) {
    if (priority == 0.0f)
        return 0;

    float p = priority * 0.5f + 0.5f;
    float level = 0.0f;

    uint32_t result = 0;
    if (p < 0.2f) {
        result = DXGI_RESOURCE_PRIORITY_MINIMUM;
        level = 0.0f;
    } else if (p < 0.4f) {
        result = DXGI_RESOURCE_PRIORITY_LOW;
        level = 0.2f;
    } else if (p < 0.6f) {
        result = DXGI_RESOURCE_PRIORITY_NORMAL;
        level = 0.4f;
    } else if (p < 0.8f) {
        result = DXGI_RESOURCE_PRIORITY_HIGH;
        level = 0.6f;
    } else {
        result = DXGI_RESOURCE_PRIORITY_MAXIMUM;
        level = 0.8f;
    }

    uint32_t bonus = uint32_t(((p - level) / 0.2f) * 65535.0f);
    if (bonus > 0xFFFF)
        bonus = 0xFFFF;

    result |= bonus;

    return result;
}
