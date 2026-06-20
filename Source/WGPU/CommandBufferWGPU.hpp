// © 2026 NVIDIA Corporation

CommandBufferWGPU::~CommandBufferWGPU() {
    ReleaseRootBindGroups();
    ReleaseTransientObjects();

    if (m_RenderPass)
        wgpuRenderPassEncoderRelease(m_RenderPass);
    ReleaseRenderPassTransientObjects();

    if (m_ComputePass)
        wgpuComputePassEncoderRelease(m_ComputePass);
    if (m_CommandBuffer)
        wgpuCommandBufferRelease(m_CommandBuffer);
    if (m_CommandEncoder)
        wgpuCommandEncoderRelease(m_CommandEncoder);

    for (ClearPipelineWGPU& clearPipeline : m_ClearPipelines) {
        if (clearPipeline.pipeline)
            wgpuRenderPipelineRelease(clearPipeline.pipeline);
        if (clearPipeline.layout)
            wgpuPipelineLayoutRelease(clearPipeline.layout);
    }

    if (m_ClearStorageBufferPipeline.pipeline)
        wgpuComputePipelineRelease(m_ClearStorageBufferPipeline.pipeline);
    if (m_ClearStorageBufferPipeline.pipelineLayout)
        wgpuPipelineLayoutRelease(m_ClearStorageBufferPipeline.pipelineLayout);
    if (m_ClearStorageBufferPipeline.bindGroupLayout)
        wgpuBindGroupLayoutRelease(m_ClearStorageBufferPipeline.bindGroupLayout);

    for (ClearStorageTexturePipelineWGPU& clearPipeline : m_ClearStorageTexturePipelines) {
        if (clearPipeline.pipeline)
            wgpuComputePipelineRelease(clearPipeline.pipeline);
        if (clearPipeline.pipelineLayout)
            wgpuPipelineLayoutRelease(clearPipeline.pipelineLayout);
        if (clearPipeline.bindGroupLayout)
            wgpuBindGroupLayoutRelease(clearPipeline.bindGroupLayout);
    }
}

void CommandBufferWGPU::ReleaseRootBindGroups() {
    if (m_GraphicsRootBindGroup) {
        wgpuBindGroupRelease(m_GraphicsRootBindGroup);
        m_GraphicsRootBindGroup = nullptr;
    }

    if (m_ComputeRootBindGroup) {
        wgpuBindGroupRelease(m_ComputeRootBindGroup);
        m_ComputeRootBindGroup = nullptr;
    }
}

void CommandBufferWGPU::ReleaseTransientObjects() {
    for (WGPUBuffer buffer : m_TemporaryBuffers)
        wgpuBufferRelease(buffer);
    m_TemporaryBuffers.clear();
}

void CommandBufferWGPU::ReleaseRenderPassTransientObjects() {
    if (m_RenderDepthStencilView) {
        wgpuTextureViewRelease(m_RenderDepthStencilView);
        m_RenderDepthStencilView = nullptr;
    }
}

void CommandBufferWGPU::PopPassAnnotations(AnnotationScopeWGPU scope) {
    if (scope == AnnotationScopeWGPU::RENDER_PASS) {
        while (!m_AnnotationScopes.empty() && m_AnnotationScopes.back() == AnnotationScopeWGPU::RENDER_PASS) {
            wgpuRenderPassEncoderPopDebugGroup(m_RenderPass);
            m_AnnotationScopes.pop_back();
        }

        return;
    }

    while (!m_AnnotationScopes.empty() && m_AnnotationScopes.back() == AnnotationScopeWGPU::COMPUTE_PASS) {
        wgpuComputePassEncoderPopDebugGroup(m_ComputePass);
        m_AnnotationScopes.pop_back();
    }
}

void CommandBufferWGPU::FlushDeferredEncoderAnnotationPops() {
    if (!m_CommandEncoder || m_RenderPass || m_ComputePass)
        return;

    for (; m_DeferredEncoderAnnotationPopNum; m_DeferredEncoderAnnotationPopNum--)
        wgpuCommandEncoderPopDebugGroup(m_CommandEncoder);
}

static uint32_t GetFormatComponentNumWGPU(Format format) {
    const FormatProps& props = GetFormatProps(format);
    uint32_t componentNum = 0;
    componentNum += props.redBits ? 1 : 0;
    componentNum += props.greenBits ? 1 : 0;
    componentNum += props.blueBits ? 1 : 0;
    componentNum += props.alphaBits ? 1 : 0;

    return std::max(componentNum, 1u);
}

static const char* GetFormatScalarTypeWGPU(Format format) {
    const FormatProps& props = GetFormatProps(format);
    if (props.isInteger)
        return props.isSigned ? "i32" : "u32";

    return "f32";
}

static std::string GetFormatShaderTypeWGPU(Format format) {
    const char* scalarType = GetFormatScalarTypeWGPU(format);
    uint32_t componentNum = GetFormatComponentNumWGPU(format);
    if (componentNum == 1)
        return scalarType;

    char type[32] = {};
    snprintf(type, sizeof(type), "vec%u<%s>", componentNum, scalarType);

    return type;
}

static std::string GetClearShaderValueWGPU(Format format) {
    uint32_t componentNum = GetFormatComponentNumWGPU(format);
    if (componentNum == 1)
        return "c.color.x";
    if (componentNum == 2)
        return "c.color.xy";
    if (componentNum == 3)
        return "c.color.xyz";

    return "c.color";
}

static std::string GetZeroShaderValueWGPU(Format format) {
    std::string type = GetFormatShaderTypeWGPU(format);
    return type + "(0)";
}

static PlaneBits GetFormatPlanesWGPU(Format format) {
    const FormatProps& props = GetFormatProps(format);
    if (props.isDepth && props.isStencil)
        return PlaneBits::DEPTH | PlaneBits::STENCIL;
    if (props.isDepth)
        return PlaneBits::DEPTH;
    if (props.isStencil)
        return PlaneBits::STENCIL;

    return PlaneBits::COLOR;
}

static PlaneBits NormalizeClearPlanesWGPU(PlaneBits planes, Format format) {
    return planes == PlaneBits::ALL ? GetFormatPlanesWGPU(format) : planes;
}

static WGPUTextureView CreateDepthStencilViewWGPU(const DescriptorWGPU& descriptor) {
    const TextureWGPU* texture = descriptor.GetTexture();
    if (!texture)
        return nullptr;

    const TextureDesc& textureDesc = texture->GetDesc();
    const TextureViewDesc& viewDesc = descriptor.GetTextureViewDesc();

    WGPUTextureViewDescriptor desc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    desc.format = GetTextureFormat(viewDesc.format == Format::UNKNOWN ? textureDesc.format : viewDesc.format);
    desc.dimension = GetTextureViewDimension(viewDesc.type, textureDesc);
    desc.baseMipLevel = viewDesc.mipOffset;
    desc.mipLevelCount = viewDesc.mipNum == REMAINING ? WGPU_MIP_LEVEL_COUNT_UNDEFINED : viewDesc.mipNum;
    desc.baseArrayLayer = viewDesc.layerOffset;
    desc.arrayLayerCount = viewDesc.layerNum == REMAINING ? WGPU_ARRAY_LAYER_COUNT_UNDEFINED : viewDesc.layerNum;
    desc.aspect = WGPUTextureAspect_All;

    return wgpuTextureCreateView(*texture, &desc);
}

WGPURenderPipeline CommandBufferWGPU::GetClearPipeline(uint32_t colorAttachmentIndex, PlaneBits planes, WGPUPipelineLayout& pipelineLayout) {
    for (ClearPipelineWGPU& clearPipeline : m_ClearPipelines) {
        bool isSame = clearPipeline.depthStencilFormat == m_RenderDepthStencilFormat
            && clearPipeline.colorNum == m_RenderColorNum
            && clearPipeline.colorAttachmentIndex == colorAttachmentIndex
            && clearPipeline.planes == planes
            && clearPipeline.sampleNum == m_RenderSampleNum;
        for (uint32_t i = 0; isSame && i < m_RenderColorNum; i++)
            isSame = clearPipeline.colorFormats[i] == m_RenderColorFormats[i];

        if (isSame) {
            pipelineLayout = clearPipeline.layout;
            return clearPipeline.pipeline;
        }
    }

    Format immediateFormat = (planes & PlaneBits::COLOR) && colorAttachmentIndex < m_RenderColorNum ? m_RenderColorFormats[colorAttachmentIndex] : Format::RGBA32_SFLOAT;
    std::string clearShaderSource = "struct ClearConstants { color: vec4<";
    clearShaderSource += GetFormatScalarTypeWGPU(immediateFormat);
    clearShaderSource += ">, }\n";
    clearShaderSource +=
        "var<immediate> c: ClearConstants;\n"
        "@vertex\n"
        "fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> @builtin(position) vec4<f32> {\n"
        "    var positions = array<vec2<f32>, 3>(vec2<f32>(-1.0, -1.0), vec2<f32>(3.0, -1.0), vec2<f32>(-1.0, 3.0));\n";
    clearShaderSource += (planes & (PlaneBits::DEPTH | PlaneBits::STENCIL)) ? "    return vec4<f32>(positions[vertexIndex], c.color.x, 1.0);\n" : "    return vec4<f32>(positions[vertexIndex], 0.0, 1.0);\n";
    clearShaderSource += "}\n";

    if (m_RenderColorNum) {
        clearShaderSource += "struct FragmentOutput {\n";
        for (uint32_t i = 0; i < m_RenderColorNum; i++) {
            char location[128] = {};
            snprintf(location, sizeof(location), "    @location(%u) color%u: ", i, i);
            clearShaderSource += location;
            clearShaderSource += GetFormatShaderTypeWGPU(m_RenderColorFormats[i]);
            clearShaderSource += ",\n";
        }

        clearShaderSource +=
            "}\n"
            "@fragment\n"
            "fn fs_main() -> FragmentOutput {\n"
            "    var output: FragmentOutput;\n";
        for (uint32_t i = 0; i < m_RenderColorNum; i++) {
            char output[64] = {};
            snprintf(output, sizeof(output), "    output.color%u = ", i);
            clearShaderSource += output;
            if ((planes & PlaneBits::COLOR) && i == colorAttachmentIndex)
                clearShaderSource += GetClearShaderValueWGPU(m_RenderColorFormats[i]);
            else
                clearShaderSource += GetZeroShaderValueWGPU(m_RenderColorFormats[i]);
            clearShaderSource += ";\n";
        }

        clearShaderSource +=
            "    return output;\n"
            "}\n";
    }

    WGPUShaderSourceWGSL wgsl = WGPU_SHADER_SOURCE_WGSL_INIT;
    wgsl.code = {clearShaderSource.data(), clearShaderSource.size()};

    WGPUShaderModuleDescriptor shaderDesc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    shaderDesc.nextInChain = &wgsl.chain;

    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(m_Device, &shaderDesc);
    if (!shader)
        return nullptr;

    WGPUPipelineLayoutExtras layoutExtras = {};
    layoutExtras.chain.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras;
    layoutExtras.immediateDataSize = sizeof(Color32f);

    WGPUPipelineLayoutDescriptor layoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    layoutDesc.nextInChain = &layoutExtras.chain;

    pipelineLayout = wgpuDeviceCreatePipelineLayout(m_Device, &layoutDesc);
    if (!pipelineLayout) {
        wgpuShaderModuleRelease(shader);
        return nullptr;
    }

    WGPUFragmentState fragment = WGPU_FRAGMENT_STATE_INIT;
    Scratch<WGPUColorTargetState> colorTargets = NRI_ALLOCATE_SCRATCH(m_Device, WGPUColorTargetState, m_RenderColorNum);
    if (m_RenderColorNum) {
        for (uint32_t i = 0; i < m_RenderColorNum; i++) {
            WGPUColorTargetState& colorTarget = colorTargets[i];
            colorTarget = WGPU_COLOR_TARGET_STATE_INIT;
            colorTarget.format = GetTextureFormat(m_RenderColorFormats[i]);
            colorTarget.writeMask = ((planes & PlaneBits::COLOR) && i == colorAttachmentIndex) ? WGPUColorWriteMask_All : WGPUColorWriteMask_None;
        }

        fragment.module = shader;
        fragment.entryPoint = WGPUString("fs_main");
        fragment.targetCount = m_RenderColorNum;
        fragment.targets = colorTargets;
    }

    WGPUPrimitiveState primitive = WGPU_PRIMITIVE_STATE_INIT;
    primitive.topology = WGPUPrimitiveTopology_TriangleList;
    primitive.frontFace = WGPUFrontFace_CCW;
    primitive.cullMode = WGPUCullMode_None;

    WGPUMultisampleState multisample = WGPU_MULTISAMPLE_STATE_INIT;
    multisample.count = GetCountOrOne((uint32_t)m_RenderSampleNum);
    multisample.mask = 0xFFFFFFFF;

    WGPUDepthStencilState depthStencil = WGPU_DEPTH_STENCIL_STATE_INIT;
    if (m_RenderDepthStencilFormat != Format::UNKNOWN) {
        depthStencil.format = GetTextureFormat(m_RenderDepthStencilFormat);
        depthStencil.depthWriteEnabled = (planes & PlaneBits::DEPTH) ? WGPUOptionalBool_True : WGPUOptionalBool_False;
        depthStencil.depthCompare = WGPUCompareFunction_Always;

        if (planes & PlaneBits::STENCIL) {
            depthStencil.stencilFront.compare = WGPUCompareFunction_Always;
            depthStencil.stencilFront.failOp = WGPUStencilOperation_Keep;
            depthStencil.stencilFront.depthFailOp = WGPUStencilOperation_Keep;
            depthStencil.stencilFront.passOp = WGPUStencilOperation_Replace;
            depthStencil.stencilBack = depthStencil.stencilFront;
            depthStencil.stencilWriteMask = 0xFF;
        } else
            depthStencil.stencilWriteMask = 0;
    }

    WGPUVertexState vertex = WGPU_VERTEX_STATE_INIT;
    vertex.module = shader;
    vertex.entryPoint = WGPUString("vs_main");

    WGPURenderPipelineDescriptor pipelineDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    pipelineDesc.layout = pipelineLayout;
    pipelineDesc.vertex = vertex;
    pipelineDesc.primitive = primitive;
    pipelineDesc.multisample = multisample;
    pipelineDesc.fragment = m_RenderColorNum ? &fragment : nullptr;
    pipelineDesc.depthStencil = m_RenderDepthStencilFormat == Format::UNKNOWN ? nullptr : &depthStencil;

    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(m_Device, &pipelineDesc);
    wgpuShaderModuleRelease(shader);

    if (!pipeline) {
        wgpuPipelineLayoutRelease(pipelineLayout);
        pipelineLayout = nullptr;
        return nullptr;
    }

    ClearPipelineWGPU clearPipeline = {};
    clearPipeline.colorFormats = m_RenderColorFormats;
    clearPipeline.depthStencilFormat = m_RenderDepthStencilFormat;
    clearPipeline.colorNum = m_RenderColorNum;
    clearPipeline.colorAttachmentIndex = colorAttachmentIndex;
    clearPipeline.planes = planes;
    clearPipeline.sampleNum = m_RenderSampleNum;
    clearPipeline.layout = pipelineLayout;
    clearPipeline.pipeline = pipeline;
    m_ClearPipelines.push_back(clearPipeline);

    return pipeline;
}

struct ClearStorageBufferConstantsWGPU {
    std::array<uint32_t, 4> words;
    uint32_t wordNum;
    uint32_t period;
    uint32_t pad0;
    uint32_t pad1;
};

struct ClearStorageTextureConstantsWGPU {
    Color32f f;
    Color32ui u;
    Color32i i;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t pad;
};

static WGPUShaderModule CreateShaderModuleWGPU(DeviceWGPU& device, const std::string& source) {
    WGPUShaderSourceWGSL wgsl = WGPU_SHADER_SOURCE_WGSL_INIT;
    wgsl.code = {source.data(), source.size()};

    WGPUShaderModuleDescriptor desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    desc.nextInChain = &wgsl.chain;

    return wgpuDeviceCreateShaderModule(device, &desc);
}

static uint32_t DivideUpWGPU(uint32_t x, uint32_t y) {
    return (x + y - 1) / y;
}

WGPUComputePipeline CommandBufferWGPU::GetClearStorageBufferPipeline(WGPUBindGroupLayout& bindGroupLayout) {
    bindGroupLayout = m_ClearStorageBufferPipeline.bindGroupLayout;
    if (m_ClearStorageBufferPipeline.pipeline)
        return m_ClearStorageBufferPipeline.pipeline;

    static const std::string shaderSource =
        "struct ClearConstants {\n"
        "    words: vec4<u32>,\n"
        "    wordNum: u32,\n"
        "    period: u32,\n"
        "    pad0: u32,\n"
        "    pad1: u32,\n"
        "}\n"
        "var<immediate> c: ClearConstants;\n"
        "@group(0) @binding(0) var<storage, read_write> dst: array<u32>;\n"
        "@compute @workgroup_size(64)\n"
        "fn main(@builtin(global_invocation_id) id: vec3<u32>) {\n"
        "    let i = id.x;\n"
        "    if (i >= c.wordNum) {\n"
        "        return;\n"
        "    }\n"
        "    let p = i % c.period;\n"
        "    var value = c.words.x;\n"
        "    if (p == 1u) {\n"
        "        value = c.words.y;\n"
        "    } else if (p == 2u) {\n"
        "        value = c.words.z;\n"
        "    } else if (p == 3u) {\n"
        "        value = c.words.w;\n"
        "    }\n"
        "    dst[i] = value;\n"
        "}\n";

    WGPUShaderModule shader = CreateShaderModuleWGPU(m_Device, shaderSource);
    if (!shader)
        return nullptr;

    WGPUBindGroupLayoutEntry entry = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
    entry.binding = 0;
    entry.visibility = WGPUShaderStage_Compute;
    entry.buffer.type = WGPUBufferBindingType_Storage;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &entry;

    WGPUBindGroupLayout layout = wgpuDeviceCreateBindGroupLayout(m_Device, &bindGroupLayoutDesc);
    if (!layout) {
        wgpuShaderModuleRelease(shader);
        return nullptr;
    }

    WGPUPipelineLayoutExtras layoutExtras = {};
    layoutExtras.chain.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras;
    layoutExtras.immediateDataSize = sizeof(ClearStorageBufferConstantsWGPU);

    WGPUPipelineLayoutDescriptor pipelineLayoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    pipelineLayoutDesc.nextInChain = &layoutExtras.chain;
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = &layout;

    WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(m_Device, &pipelineLayoutDesc);
    if (!pipelineLayout) {
        wgpuBindGroupLayoutRelease(layout);
        wgpuShaderModuleRelease(shader);
        return nullptr;
    }

    WGPUComputePipelineDescriptor pipelineDesc = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
    pipelineDesc.layout = pipelineLayout;
    pipelineDesc.compute.module = shader;
    pipelineDesc.compute.entryPoint = WGPUString("main");

    WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(m_Device, &pipelineDesc);
    wgpuShaderModuleRelease(shader);

    if (!pipeline) {
        wgpuPipelineLayoutRelease(pipelineLayout);
        wgpuBindGroupLayoutRelease(layout);
        return nullptr;
    }

    m_ClearStorageBufferPipeline.bindGroupLayout = layout;
    m_ClearStorageBufferPipeline.pipelineLayout = pipelineLayout;
    m_ClearStorageBufferPipeline.pipeline = pipeline;
    bindGroupLayout = layout;

    return pipeline;
}

static const char* GetStorageTextureFormatNameWGPU(Format format) {
    switch (format) {
        case Format::BGRA8_UNORM: return "bgra8unorm";
        case Format::RGBA8_UNORM: return "rgba8unorm";
        case Format::RGBA8_SNORM: return "rgba8snorm";
        case Format::RGBA8_UINT: return "rgba8uint";
        case Format::RGBA8_SINT: return "rgba8sint";
        case Format::RGBA16_UINT: return "rgba16uint";
        case Format::RGBA16_SINT: return "rgba16sint";
        case Format::RGBA16_SFLOAT: return "rgba16float";
        case Format::R32_UINT: return "r32uint";
        case Format::R32_SINT: return "r32sint";
        case Format::R32_SFLOAT: return "r32float";
        case Format::RG32_UINT: return "rg32uint";
        case Format::RG32_SINT: return "rg32sint";
        case Format::RG32_SFLOAT: return "rg32float";
        case Format::RGBA32_UINT: return "rgba32uint";
        case Format::RGBA32_SINT: return "rgba32sint";
        case Format::RGBA32_SFLOAT: return "rgba32float";
        default: return nullptr;
    }
}

static const char* GetStorageTextureDimensionNameWGPU(WGPUTextureViewDimension dimension) {
    switch (dimension) {
        case WGPUTextureViewDimension_1D: return "1d";
        case WGPUTextureViewDimension_2D: return "2d";
        case WGPUTextureViewDimension_2DArray: return "2d_array";
        case WGPUTextureViewDimension_3D: return "3d";
        default: return nullptr;
    }
}

static const char* GetStorageTextureValueWGPU(Format format) {
    const FormatProps& props = GetFormatProps(format);
    if (props.isInteger)
        return props.isSigned ? "c.i" : "c.u";

    return "c.f";
}

static void AppendStorageTextureStoreWGPU(std::string& shaderSource, WGPUTextureViewDimension dimension, const char* value) {
    switch (dimension) {
        case WGPUTextureViewDimension_1D:
            shaderSource +=
                "    if (id.x >= c.width) {\n"
                "        return;\n"
                "    }\n"
                "    textureStore(dst, i32(id.x), ";
            shaderSource += value;
            shaderSource += ");\n";
            break;
        case WGPUTextureViewDimension_2D:
            shaderSource +=
                "    if (id.x >= c.width || id.y >= c.height) {\n"
                "        return;\n"
                "    }\n"
                "    textureStore(dst, vec2<i32>(id.xy), ";
            shaderSource += value;
            shaderSource += ");\n";
            break;
        case WGPUTextureViewDimension_2DArray:
            shaderSource +=
                "    if (id.x >= c.width || id.y >= c.height || id.z >= c.depth) {\n"
                "        return;\n"
                "    }\n"
                "    textureStore(dst, vec2<i32>(id.xy), i32(id.z), ";
            shaderSource += value;
            shaderSource += ");\n";
            break;
        case WGPUTextureViewDimension_3D:
            shaderSource +=
                "    if (id.x >= c.width || id.y >= c.height || id.z >= c.depth) {\n"
                "        return;\n"
                "    }\n"
                "    textureStore(dst, vec3<i32>(id.xyz), ";
            shaderSource += value;
            shaderSource += ");\n";
            break;
        default:
            break;
    }
}

WGPUComputePipeline CommandBufferWGPU::GetClearStorageTexturePipeline(Format format, WGPUTextureViewDimension dimension, WGPUBindGroupLayout& bindGroupLayout) {
    for (ClearStorageTexturePipelineWGPU& clearPipeline : m_ClearStorageTexturePipelines) {
        if (clearPipeline.format == format && clearPipeline.dimension == dimension) {
            bindGroupLayout = clearPipeline.bindGroupLayout;
            return clearPipeline.pipeline;
        }
    }

    const char* formatName = GetStorageTextureFormatNameWGPU(format);
    const char* dimensionName = GetStorageTextureDimensionNameWGPU(dimension);
    if (!formatName || !dimensionName)
        return nullptr;

    std::string shaderSource =
        "struct ClearConstants {\n"
        "    f: vec4<f32>,\n"
        "    u: vec4<u32>,\n"
        "    i: vec4<i32>,\n"
        "    width: u32,\n"
        "    height: u32,\n"
        "    depth: u32,\n"
        "    pad: u32,\n"
        "}\n"
        "var<immediate> c: ClearConstants;\n"
        "@group(0) @binding(0) var dst: texture_storage_";
    shaderSource += dimensionName;
    shaderSource += "<";
    shaderSource += formatName;
    shaderSource +=
        ", write>;\n"
        "@compute @workgroup_size(8, 8, 1)\n"
        "fn main(@builtin(global_invocation_id) id: vec3<u32>) {\n";
    AppendStorageTextureStoreWGPU(shaderSource, dimension, GetStorageTextureValueWGPU(format));
    shaderSource += "}\n";

    WGPUShaderModule shader = CreateShaderModuleWGPU(m_Device, shaderSource);
    if (!shader)
        return nullptr;

    WGPUBindGroupLayoutEntry entry = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
    entry.binding = 0;
    entry.visibility = WGPUShaderStage_Compute;
    entry.storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
    entry.storageTexture.format = GetTextureFormat(format);
    entry.storageTexture.viewDimension = dimension;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &entry;

    WGPUBindGroupLayout layout = wgpuDeviceCreateBindGroupLayout(m_Device, &bindGroupLayoutDesc);
    if (!layout) {
        wgpuShaderModuleRelease(shader);
        return nullptr;
    }

    WGPUPipelineLayoutExtras layoutExtras = {};
    layoutExtras.chain.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras;
    layoutExtras.immediateDataSize = sizeof(ClearStorageTextureConstantsWGPU);

    WGPUPipelineLayoutDescriptor pipelineLayoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    pipelineLayoutDesc.nextInChain = &layoutExtras.chain;
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = &layout;

    WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(m_Device, &pipelineLayoutDesc);
    if (!pipelineLayout) {
        wgpuBindGroupLayoutRelease(layout);
        wgpuShaderModuleRelease(shader);
        return nullptr;
    }

    WGPUComputePipelineDescriptor pipelineDesc = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
    pipelineDesc.layout = pipelineLayout;
    pipelineDesc.compute.module = shader;
    pipelineDesc.compute.entryPoint = WGPUString("main");

    WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(m_Device, &pipelineDesc);
    wgpuShaderModuleRelease(shader);

    if (!pipeline) {
        wgpuPipelineLayoutRelease(pipelineLayout);
        wgpuBindGroupLayoutRelease(layout);
        return nullptr;
    }

    ClearStorageTexturePipelineWGPU clearPipeline = {};
    clearPipeline.format = format;
    clearPipeline.dimension = dimension;
    clearPipeline.bindGroupLayout = layout;
    clearPipeline.pipelineLayout = pipelineLayout;
    clearPipeline.pipeline = pipeline;
    m_ClearStorageTexturePipelines.push_back(clearPipeline);
    bindGroupLayout = layout;

    return pipeline;
}

Result CommandBufferWGPU::Create(const CommandAllocator& commandAllocator) {
    MaybeUnused(commandAllocator);

    return Result::SUCCESS;
}

Result CommandBufferWGPU::Begin(const DescriptorPool* descriptorPool) {
    MaybeUnused(descriptorPool);

    ReleaseRootBindGroups();
    ReleaseTransientObjects();

    if (m_CommandBuffer) {
        wgpuCommandBufferRelease(m_CommandBuffer);
        m_CommandBuffer = nullptr;
    }

    if (m_CommandEncoder) {
        wgpuCommandEncoderRelease(m_CommandEncoder);
        m_CommandEncoder = nullptr;
    }

    WGPUCommandEncoderDescriptor desc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
    m_CommandEncoder = wgpuDeviceCreateCommandEncoder(m_Device, &desc);
    m_PipelineLayout = nullptr;
    m_RenderPipeline = nullptr;
    m_ComputePipeline = nullptr;
    m_BoundComputePipeline = nullptr;
    m_BindPoint = BindPoint::GRAPHICS;
    for (Format& format : m_RenderColorFormats)
        format = Format::UNKNOWN;
    m_RenderDepthStencilFormat = Format::UNKNOWN;
    m_RenderColorNum = 0;
    m_RenderSampleNum = 1;
    m_RenderWidth = 0;
    m_RenderHeight = 0;
    m_GraphicsDescriptorSets.clear();
    m_ComputeDescriptorSets.clear();
    m_GraphicsDescriptorSetDirty.clear();
    m_ComputeDescriptorSetDirty.clear();
    m_RootDescriptorBindings.clear();
    m_RootDynamicOffsets.clear();
    m_GraphicsRootConstants.data.clear();
    m_GraphicsRootConstants.mask.clear();
    m_ComputeRootConstants.data.clear();
    m_ComputeRootConstants.mask.clear();
    m_AnnotationScopes.clear();
    m_DeferredEncoderAnnotationPopNum = 0;
    m_GraphicsDirtyDescriptorSetMin = uint32_t(-1);
    m_GraphicsDirtyDescriptorSetMax = 0;
    m_ComputeDirtyDescriptorSetMin = uint32_t(-1);
    m_ComputeDirtyDescriptorSetMax = 0;
    m_GraphicsRootGroupDirty = true;
    m_ComputeRootGroupDirty = true;
    m_HasViewport = false;
    m_HasScissor = false;
    m_StencilReference = 0;

    return m_CommandEncoder ? Result::SUCCESS : Result::FAILURE;
}

Result CommandBufferWGPU::End() {
    EndPass();

    WGPUCommandBufferDescriptor desc = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
    m_CommandBuffer = wgpuCommandEncoderFinish(m_CommandEncoder, &desc);
    wgpuCommandEncoderRelease(m_CommandEncoder);
    m_CommandEncoder = nullptr;

    return m_CommandBuffer ? Result::SUCCESS : Result::FAILURE;
}

void CommandBufferWGPU::EndPass() {
    if (m_RenderPass) {
        PopPassAnnotations(AnnotationScopeWGPU::RENDER_PASS);

        wgpuRenderPassEncoderEnd(m_RenderPass);
        wgpuRenderPassEncoderRelease(m_RenderPass);
        m_RenderPass = nullptr;
        m_RenderPipeline = nullptr;
        ReleaseRenderPassTransientObjects();
    }

    if (m_ComputePass) {
        PopPassAnnotations(AnnotationScopeWGPU::COMPUTE_PASS);

        wgpuComputePassEncoderEnd(m_ComputePass);
        wgpuComputePassEncoderRelease(m_ComputePass);
        m_ComputePass = nullptr;
        m_BoundComputePipeline = nullptr;
    }

    FlushDeferredEncoderAnnotationPops();
}

void CommandBufferWGPU::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    if (!m_RenderPass)
        return;

    for (uint32_t i = 0; i < viewportNum; i++) {
        const Viewport& viewport = viewports[i];
        wgpuRenderPassEncoderSetViewport(m_RenderPass, viewport.x, viewport.y, viewport.width, viewport.height, viewport.depthMin, viewport.depthMax);

        m_Viewport = viewport;
        m_HasViewport = true;
    }
}

bool CommandBufferWGPU::SetScissorRect(const Rect& rect) {
    int32_t x0 = std::max<int32_t>(rect.x, 0);
    int32_t y0 = std::max<int32_t>(rect.y, 0);
    int32_t x1 = std::min<int32_t>((int32_t)m_RenderWidth, (int32_t)rect.x + (int32_t)rect.width);
    int32_t y1 = std::min<int32_t>((int32_t)m_RenderHeight, (int32_t)rect.y + (int32_t)rect.height);

    if (x1 <= x0 || y1 <= y0) {
        wgpuRenderPassEncoderSetScissorRect(m_RenderPass, 0, 0, 0, 0);
        return false;
    }

    wgpuRenderPassEncoderSetScissorRect(m_RenderPass, (uint32_t)x0, (uint32_t)y0, (uint32_t)(x1 - x0), (uint32_t)(y1 - y0));
    return true;
}

void CommandBufferWGPU::SetScissors(const Rect* rects, uint32_t rectNum) {
    if (!m_RenderPass)
        return;

    for (uint32_t i = 0; i < rectNum; i++) {
        const Rect& rect = rects[i];
        SetScissorRect(rect);

        m_Scissor = rect;
        m_HasScissor = true;
    }
}

void CommandBufferWGPU::SetPipelineLayout(BindPoint bindPoint, const PipelineLayout& pipelineLayout) {
    m_PipelineLayout = (PipelineLayoutWGPU*)&pipelineLayout;
    m_BindPoint = bindPoint == BindPoint::INHERIT ? m_BindPoint : bindPoint;
    ReleaseRootBindGroups();
    m_RootDescriptorBindings.resize(m_PipelineLayout->GetRootDescriptors().size());
    m_RootDynamicOffsets.resize(m_PipelineLayout->GetRootDynamicOffsetNum());
    m_GraphicsRootConstants.data.resize(m_PipelineLayout->GetImmediateDataSize());
    m_GraphicsRootConstants.mask.resize(m_PipelineLayout->GetImmediateDataSize());
    m_ComputeRootConstants.data.resize(m_PipelineLayout->GetImmediateDataSize());
    m_ComputeRootConstants.mask.resize(m_PipelineLayout->GetImmediateDataSize());
    if (!m_GraphicsRootConstants.data.empty()) {
        memset(m_GraphicsRootConstants.data.data(), 0, m_GraphicsRootConstants.data.size());
        memset(m_GraphicsRootConstants.mask.data(), 0, m_GraphicsRootConstants.mask.size());
        memset(m_ComputeRootConstants.data.data(), 0, m_ComputeRootConstants.data.size());
        memset(m_ComputeRootConstants.mask.data(), 0, m_ComputeRootConstants.mask.size());
    }

    m_GraphicsRootGroupDirty = true;
    m_ComputeRootGroupDirty = true;
    MarkDescriptorSetsDirty(m_BindPoint);
}

void CommandBufferWGPU::SetPipeline(const Pipeline& pipeline) {
    const PipelineWGPU& pipelineWGPU = (PipelineWGPU&)pipeline;
    WGPURenderPipeline renderPipeline = pipelineWGPU.GetRenderPipeline();

    if (m_RenderPass && renderPipeline && renderPipeline != m_RenderPipeline) {
        wgpuRenderPassEncoderSetPipeline(m_RenderPass, renderPipeline);
        m_RenderPipeline = renderPipeline;
    }

    if (m_RenderPass && renderPipeline)
        RestoreRootConstants(BindPoint::GRAPHICS);

    if (pipelineWGPU.GetComputePipeline()) {
        m_ComputePipeline = pipelineWGPU.GetComputePipeline();
        if (m_ComputePass && m_ComputePipeline != m_BoundComputePipeline) {
            wgpuComputePassEncoderSetPipeline(m_ComputePass, m_ComputePipeline);
            m_BoundComputePipeline = m_ComputePipeline;
        }

        if (m_ComputePass)
            RestoreRootConstants(BindPoint::COMPUTE);
    }

}

void CommandBufferWGPU::SetDescriptorSet(const SetDescriptorSetDesc& setDescriptorSetDesc) {
    DescriptorSetWGPU& descriptorSet = *(DescriptorSetWGPU*)setDescriptorSetDesc.descriptorSet;
    BindPoint bindPoint = setDescriptorSetDesc.bindPoint == BindPoint::INHERIT ? m_BindPoint : setDescriptorSetDesc.bindPoint;
    uint32_t bindGroupIndex = m_PipelineLayout ? m_PipelineLayout->GetDescriptorSetMapping(setDescriptorSetDesc.setIndex).bindGroupIndex : setDescriptorSetDesc.setIndex;
    Vector<const DescriptorSetWGPU*>& descriptorSets = bindPoint == BindPoint::COMPUTE ? m_ComputeDescriptorSets : m_GraphicsDescriptorSets;

    if (bindGroupIndex >= descriptorSets.size())
        descriptorSets.resize(bindGroupIndex + 1);

    Vector<uint8_t>& dirtySets = bindPoint == BindPoint::COMPUTE ? m_ComputeDescriptorSetDirty : m_GraphicsDescriptorSetDirty;
    if (bindGroupIndex >= dirtySets.size())
        dirtySets.resize(bindGroupIndex + 1, 0);

    if (descriptorSets[bindGroupIndex] != &descriptorSet) {
        descriptorSets[bindGroupIndex] = &descriptorSet;
        MarkDescriptorSetDirty(bindPoint, bindGroupIndex);
    }
}

void CommandBufferWGPU::MarkDescriptorSetDirty(BindPoint bindPoint, uint32_t bindGroupIndex) {
    Vector<uint8_t>& dirtySets = bindPoint == BindPoint::COMPUTE ? m_ComputeDescriptorSetDirty : m_GraphicsDescriptorSetDirty;
    if (bindGroupIndex >= dirtySets.size())
        dirtySets.resize(bindGroupIndex + 1, 0);

    dirtySets[bindGroupIndex] = 1;

    uint32_t& dirtyMin = bindPoint == BindPoint::COMPUTE ? m_ComputeDirtyDescriptorSetMin : m_GraphicsDirtyDescriptorSetMin;
    uint32_t& dirtyMax = bindPoint == BindPoint::COMPUTE ? m_ComputeDirtyDescriptorSetMax : m_GraphicsDirtyDescriptorSetMax;
    dirtyMin = std::min(dirtyMin, bindGroupIndex);
    dirtyMax = std::max(dirtyMax, bindGroupIndex + 1);
}

void CommandBufferWGPU::MarkDescriptorSetsDirty(BindPoint bindPoint) {
    Vector<const DescriptorSetWGPU*>& descriptorSets = bindPoint == BindPoint::COMPUTE ? m_ComputeDescriptorSets : m_GraphicsDescriptorSets;
    Vector<uint8_t>& dirtySets = bindPoint == BindPoint::COMPUTE ? m_ComputeDescriptorSetDirty : m_GraphicsDescriptorSetDirty;
    for (uint8_t& dirty : dirtySets)
        dirty = 1;

    uint32_t& dirtyMin = bindPoint == BindPoint::COMPUTE ? m_ComputeDirtyDescriptorSetMin : m_GraphicsDirtyDescriptorSetMin;
    uint32_t& dirtyMax = bindPoint == BindPoint::COMPUTE ? m_ComputeDirtyDescriptorSetMax : m_GraphicsDirtyDescriptorSetMax;
    dirtyMin = descriptorSets.empty() ? uint32_t(-1) : 0;
    dirtyMax = (uint32_t)descriptorSets.size();
}

void CommandBufferWGPU::BindDescriptorSets(BindPoint bindPoint) {
    if (bindPoint == BindPoint::COMPUTE) {
        if (!m_ComputePass)
            return;

        uint32_t dirtyMin = uint32_t(-1);
        uint32_t dirtyMax = 0;
        for (uint32_t i = m_ComputeDirtyDescriptorSetMin; i < std::min(m_ComputeDirtyDescriptorSetMax, (uint32_t)m_ComputeDescriptorSets.size()); i++) {
            if (i >= m_ComputeDescriptorSetDirty.size() || !m_ComputeDescriptorSetDirty[i])
                continue;

            const DescriptorSetWGPU* descriptorSet = m_ComputeDescriptorSets[i];
            WGPUBindGroup bindGroup = descriptorSet ? descriptorSet->GetBindGroup() : nullptr;
            if (bindGroup) {
                wgpuComputePassEncoderSetBindGroup(m_ComputePass, i, bindGroup, 0, nullptr);
                m_ComputeDescriptorSetDirty[i] = 0;
            } else {
                dirtyMin = std::min(dirtyMin, i);
                dirtyMax = std::max(dirtyMax, i + 1);
            }
        }

        m_ComputeDirtyDescriptorSetMin = dirtyMin;
        m_ComputeDirtyDescriptorSetMax = dirtyMax;

        return;
    }

    if (!m_RenderPass)
        return;

    uint32_t dirtyMin = uint32_t(-1);
    uint32_t dirtyMax = 0;
    for (uint32_t i = m_GraphicsDirtyDescriptorSetMin; i < std::min(m_GraphicsDirtyDescriptorSetMax, (uint32_t)m_GraphicsDescriptorSets.size()); i++) {
        if (i >= m_GraphicsDescriptorSetDirty.size() || !m_GraphicsDescriptorSetDirty[i])
            continue;

        const DescriptorSetWGPU* descriptorSet = m_GraphicsDescriptorSets[i];
        WGPUBindGroup bindGroup = descriptorSet ? descriptorSet->GetBindGroup() : nullptr;
        if (bindGroup) {
            wgpuRenderPassEncoderSetBindGroup(m_RenderPass, i, bindGroup, 0, nullptr);
            m_GraphicsDescriptorSetDirty[i] = 0;
        } else {
            dirtyMin = std::min(dirtyMin, i);
            dirtyMax = std::max(dirtyMax, i + 1);
        }
    }

    m_GraphicsDirtyDescriptorSetMin = dirtyMin;
    m_GraphicsDirtyDescriptorSetMax = dirtyMax;
}

WGPUBindGroup CommandBufferWGPU::CreateRootBindGroup(BindPoint bindPoint) {
    if (!m_PipelineLayout || !m_PipelineLayout->GetRootBindGroupLayout())
        return nullptr;

    if (m_PipelineLayout->GetRootSamplerBindGroup())
        return m_PipelineLayout->GetRootSamplerBindGroup();

    const Vector<RootDescriptorMappingWGPU>& rootDescriptors = m_PipelineLayout->GetRootDescriptors();
    for (uint32_t i = 0; i < (uint32_t)rootDescriptors.size(); i++) {
        if (!m_RootDescriptorBindings[i].descriptor)
            return nullptr;
    }

    Scratch<WGPUBindGroupEntry> entries = NRI_ALLOCATE_SCRATCH(m_Device, WGPUBindGroupEntry, m_PipelineLayout->GetRootSamplers().size() + rootDescriptors.size());
    uint32_t entryNum = 0;

    for (const RootSamplerMappingWGPU& rootSampler : m_PipelineLayout->GetRootSamplers()) {
        WGPUBindGroupEntry& entry = entries[entryNum++];
        entry = WGPU_BIND_GROUP_ENTRY_INIT;
        entry.binding = rootSampler.binding;
        entry.sampler = rootSampler.sampler;
    }

    for (uint32_t i = 0; i < (uint32_t)rootDescriptors.size(); i++) {
        const RootDescriptorMappingWGPU& rootDescriptor = rootDescriptors[i];
        const RootDescriptorBindingWGPU& rootBinding = m_RootDescriptorBindings[i];
        const DescriptorWGPU& descriptor = *rootBinding.descriptor;

        WGPUBindGroupEntry& entry = entries[entryNum++];
        entry = WGPU_BIND_GROUP_ENTRY_INIT;
        entry.binding = rootDescriptor.binding;
        entry.buffer = descriptor.GetBuffer();
        entry.offset = descriptor.GetOffset();
        if (rootDescriptor.dynamicOffsetIndex == uint32_t(-1))
            entry.offset += rootBinding.offset;
        entry.size = descriptor.GetSize();
    }

    WGPUBindGroupDescriptor desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    desc.layout = m_PipelineLayout->GetRootBindGroupLayout();
    desc.entryCount = entryNum;
    desc.entries = entries;

    WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(m_Device, &desc);
    if (bindPoint == BindPoint::COMPUTE) {
        if (m_ComputeRootBindGroup)
            wgpuBindGroupRelease(m_ComputeRootBindGroup);
        m_ComputeRootBindGroup = bindGroup;
    } else {
        if (m_GraphicsRootBindGroup)
            wgpuBindGroupRelease(m_GraphicsRootBindGroup);
        m_GraphicsRootBindGroup = bindGroup;
    }

    return bindGroup;
}

void CommandBufferWGPU::BindRootGroup(BindPoint bindPoint) {
    if (!m_PipelineLayout || !m_PipelineLayout->GetRootBindGroupLayout())
        return;

    BindPoint resolvedBindPoint = bindPoint == BindPoint::INHERIT ? m_BindPoint : bindPoint;
    bool& dirty = resolvedBindPoint == BindPoint::COMPUTE ? m_ComputeRootGroupDirty : m_GraphicsRootGroupDirty;
    if (!dirty)
        return;

    WGPUBindGroup bindGroup = resolvedBindPoint == BindPoint::COMPUTE ? m_ComputeRootBindGroup : m_GraphicsRootBindGroup;
    if (!bindGroup)
        bindGroup = CreateRootBindGroup(resolvedBindPoint);
    if (!bindGroup)
        return;

    uint32_t dynamicOffsetNum = m_PipelineLayout->GetRootDynamicOffsetNum();
    const uint32_t* dynamicOffsets = dynamicOffsetNum ? m_RootDynamicOffsets.data() : nullptr;

    if (resolvedBindPoint == BindPoint::COMPUTE) {
        if (m_ComputePass)
            wgpuComputePassEncoderSetBindGroup(m_ComputePass, m_PipelineLayout->GetRootSamplerGroupIndex(), bindGroup, dynamicOffsetNum, dynamicOffsets);
    } else if (m_RenderPass)
        wgpuRenderPassEncoderSetBindGroup(m_RenderPass, m_PipelineLayout->GetRootSamplerGroupIndex(), bindGroup, dynamicOffsetNum, dynamicOffsets);

    dirty = false;
}

RootConstantStateWGPU& CommandBufferWGPU::GetRootConstantState(BindPoint bindPoint) {
    return bindPoint == BindPoint::COMPUTE ? m_ComputeRootConstants : m_GraphicsRootConstants;
}

void CommandBufferWGPU::RestoreRootConstants(BindPoint bindPoint) {
    RootConstantStateWGPU& state = GetRootConstantState(bindPoint);
    if (state.mask.empty())
        return;

    if (bindPoint == BindPoint::COMPUTE) {
        if (!m_ComputePass || !m_BoundComputePipeline)
            return;
    } else if (!m_RenderPass || !m_RenderPipeline)
        return;

    uint32_t rootConstantSize = (uint32_t)state.mask.size();
    for (uint32_t begin = 0; begin < rootConstantSize;) {
        while (begin < rootConstantSize && !state.mask[begin])
            begin++;

        uint32_t end = begin;
        while (end < rootConstantSize && state.mask[end])
            end++;

        if (end == begin)
            continue;

        if (bindPoint == BindPoint::COMPUTE)
            wgpuComputePassEncoderSetImmediates(m_ComputePass, begin, end - begin, state.data.data() + begin);
        else
            wgpuRenderPassEncoderSetImmediates(m_RenderPass, begin, end - begin, state.data.data() + begin);

        begin = end;
    }
}

void CommandBufferWGPU::SetRootConstants(const SetRootConstantsDesc& setRootConstantsDesc) {
    BindPoint bindPoint = setRootConstantsDesc.bindPoint == BindPoint::INHERIT ? m_BindPoint : setRootConstantsDesc.bindPoint;
    RootConstantStateWGPU& state = GetRootConstantState(bindPoint);
    uint32_t offset = (m_PipelineLayout ? m_PipelineLayout->GetRootConstantOffset(setRootConstantsDesc.rootConstantIndex) : 0) + setRootConstantsDesc.offset;
    if (setRootConstantsDesc.size) {
        uint32_t end = offset + setRootConstantsDesc.size;
        if (end > state.data.size()) {
            state.data.resize(end);
            state.mask.resize(end);
        }

        memcpy(state.data.data() + offset, setRootConstantsDesc.data, setRootConstantsDesc.size);
        memset(state.mask.data() + offset, 1, setRootConstantsDesc.size);
    }

    if (m_RenderPass && m_RenderPipeline && bindPoint == BindPoint::GRAPHICS)
        wgpuRenderPassEncoderSetImmediates(m_RenderPass, offset, setRootConstantsDesc.size, setRootConstantsDesc.data);

    if (m_ComputePass && m_BoundComputePipeline && bindPoint == BindPoint::COMPUTE)
        wgpuComputePassEncoderSetImmediates(m_ComputePass, offset, setRootConstantsDesc.size, setRootConstantsDesc.data);
}

void CommandBufferWGPU::SetRootDescriptor(const SetRootDescriptorDesc& setRootDescriptorDesc) {
    if (!m_PipelineLayout)
        return;

    if (setRootDescriptorDesc.rootDescriptorIndex >= m_RootDescriptorBindings.size())
        return;

    BindPoint bindPoint = setRootDescriptorDesc.bindPoint == BindPoint::INHERIT ? m_BindPoint : setRootDescriptorDesc.bindPoint;
    RootDescriptorBindingWGPU& rootBinding = m_RootDescriptorBindings[setRootDescriptorDesc.rootDescriptorIndex];
    const RootDescriptorMappingWGPU& rootDescriptor = m_PipelineLayout->GetRootDescriptorMapping(setRootDescriptorDesc.rootDescriptorIndex);
    DescriptorWGPU* descriptor = (DescriptorWGPU*)setRootDescriptorDesc.descriptor;
    bool recreateBindGroup = rootBinding.descriptor != descriptor || (rootDescriptor.dynamicOffsetIndex == uint32_t(-1) && rootBinding.offset != setRootDescriptorDesc.offset);
    bool dynamicOffsetChanged = rootDescriptor.dynamicOffsetIndex != uint32_t(-1) && m_RootDynamicOffsets[rootDescriptor.dynamicOffsetIndex] != (uint32_t)setRootDescriptorDesc.offset;
    if (!recreateBindGroup && !dynamicOffsetChanged)
        return;

    rootBinding = {descriptor, setRootDescriptorDesc.offset};

    if (rootDescriptor.dynamicOffsetIndex != uint32_t(-1))
        m_RootDynamicOffsets[rootDescriptor.dynamicOffsetIndex] = (uint32_t)setRootDescriptorDesc.offset;

    if (recreateBindGroup) {
        ReleaseRootBindGroups();
        m_GraphicsRootGroupDirty = true;
        m_ComputeRootGroupDirty = true;
    } else if (bindPoint == BindPoint::COMPUTE)
        m_ComputeRootGroupDirty = true;
    else
        m_GraphicsRootGroupDirty = true;
}

void CommandBufferWGPU::SetVertexBuffers(uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum) {
    if (!m_RenderPass)
        return;

    for (uint32_t i = 0; i < vertexBufferNum; i++) {
        const VertexBufferDesc& vertexBufferDesc = vertexBufferDescs[i];
        const BufferWGPU& buffer = *(BufferWGPU*)vertexBufferDesc.buffer;
        wgpuRenderPassEncoderSetVertexBuffer(m_RenderPass, baseSlot + i, buffer, vertexBufferDesc.offset, buffer.GetSize() - vertexBufferDesc.offset);
    }
}

void CommandBufferWGPU::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    if (!m_RenderPass)
        return;

    const BufferWGPU& bufferWGPU = (BufferWGPU&)buffer;
    wgpuRenderPassEncoderSetIndexBuffer(m_RenderPass, bufferWGPU, GetIndexFormat(indexType), offset, bufferWGPU.GetSize() - offset);
}

void CommandBufferWGPU::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    MaybeUnused(backRef);

    if (m_RenderPass)
        wgpuRenderPassEncoderSetStencilReference(m_RenderPass, frontRef);

    m_StencilReference = frontRef;
}

void CommandBufferWGPU::SetBlendConstants(const Color32f& color) {
    if (!m_RenderPass)
        return;

    WGPUColor wgpuColor = {color.x, color.y, color.z, color.w};
    wgpuRenderPassEncoderSetBlendConstant(m_RenderPass, &wgpuColor);
}

void CommandBufferWGPU::BeginRendering(const RenderingDesc& renderingDesc) {
    EndPass();

    Scratch<WGPURenderPassColorAttachment> colorAttachments = NRI_ALLOCATE_SCRATCH(m_Device, WGPURenderPassColorAttachment, renderingDesc.colorNum);
    WGPURenderPassDepthStencilAttachment depthStencilAttachment = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
    const WGPURenderPassDepthStencilAttachment* depthStencilAttachmentPtr = nullptr;
    for (Format& format : m_RenderColorFormats)
        format = Format::UNKNOWN;
    m_RenderDepthStencilFormat = Format::UNKNOWN;
    m_RenderColorNum = std::min<uint32_t>(renderingDesc.colorNum, COLOR_ATTACHMENT_MAX_NUM_WGPU);
    m_RenderSampleNum = 1;
    m_RenderWidth = 0;
    m_RenderHeight = 0;

    for (uint32_t i = 0; i < renderingDesc.colorNum; i++) {
        const AttachmentDesc& in = renderingDesc.colors[i];
        DescriptorWGPU& descriptor = *(DescriptorWGPU*)in.descriptor;
        const TextureDesc* textureDesc = descriptor.GetTextureDesc();
        WGPURenderPassColorAttachment& out = colorAttachments[i];
        out = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
        out.view = descriptor.GetTextureView();
        out.loadOp = GetLoadOp(in.loadOp);
        out.storeOp = GetStoreOp(in.storeOp);
        out.clearValue = {in.clearValue.color.f.x, in.clearValue.color.f.y, in.clearValue.color.f.z, in.clearValue.color.f.w};
        if (in.resolveDst)
            out.resolveTarget = ((DescriptorWGPU*)in.resolveDst)->GetTextureView();

        if (i < COLOR_ATTACHMENT_MAX_NUM_WGPU)
            m_RenderColorFormats[i] = descriptor.GetFormat();

        if (textureDesc) {
            if (!m_RenderWidth) {
                m_RenderWidth = GetDimension(GraphicsAPI::WGPU, *textureDesc, 0, descriptor.GetTextureViewDesc().mipOffset);
                m_RenderHeight = GetDimension(GraphicsAPI::WGPU, *textureDesc, 1, descriptor.GetTextureViewDesc().mipOffset);
            }

            m_RenderSampleNum = textureDesc->sampleNum;
        }
    }

    const AttachmentDesc* depthOrStencil = renderingDesc.depth.descriptor ? &renderingDesc.depth : (renderingDesc.stencil.descriptor ? &renderingDesc.stencil : nullptr);
    if (depthOrStencil) {
        DescriptorWGPU& descriptor = *(DescriptorWGPU*)depthOrStencil->descriptor;
        const FormatProps& formatProps = GetFormatProps(descriptor.GetFormat());
        const TextureDesc* textureDesc = descriptor.GetTextureDesc();
        m_RenderDepthStencilFormat = descriptor.GetFormat();

        if (renderingDesc.depth.descriptor && renderingDesc.stencil.descriptor) {
            // TODO: WGPU needs a single depth-stencil view for the render pass. NRI can provide separate plane descriptors.
            m_RenderDepthStencilView = CreateDepthStencilViewWGPU(descriptor);
            depthStencilAttachment.view = m_RenderDepthStencilView;
        } else
            depthStencilAttachment.view = descriptor.GetTextureView();

        if (!m_RenderWidth && textureDesc) {
            m_RenderWidth = GetDimension(GraphicsAPI::WGPU, *textureDesc, 0, descriptor.GetTextureViewDesc().mipOffset);
            m_RenderHeight = GetDimension(GraphicsAPI::WGPU, *textureDesc, 1, descriptor.GetTextureViewDesc().mipOffset);
        }

        if (textureDesc)
            m_RenderSampleNum = textureDesc->sampleNum;

        if (formatProps.isDepth) {
            const AttachmentDesc& depth = renderingDesc.depth.descriptor ? renderingDesc.depth : *depthOrStencil;
            depthStencilAttachment.depthLoadOp = GetLoadOp(depth.loadOp);
            depthStencilAttachment.depthStoreOp = GetStoreOp(depth.storeOp);
            depthStencilAttachment.depthClearValue = depth.clearValue.depthStencil.depth;
        }

        if (formatProps.isStencil) {
            const AttachmentDesc& stencil = renderingDesc.stencil.descriptor ? renderingDesc.stencil : *depthOrStencil;
            depthStencilAttachment.stencilLoadOp = GetLoadOp(stencil.loadOp);
            depthStencilAttachment.stencilStoreOp = GetStoreOp(stencil.storeOp);
            depthStencilAttachment.stencilClearValue = stencil.clearValue.depthStencil.stencil;
        }

        depthStencilAttachmentPtr = &depthStencilAttachment;
    }

    WGPURenderPassDescriptor desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    desc.colorAttachmentCount = renderingDesc.colorNum;
    desc.colorAttachments = colorAttachments;
    desc.depthStencilAttachment = depthStencilAttachmentPtr;

    m_RenderPass = wgpuCommandEncoderBeginRenderPass(m_CommandEncoder, &desc);
    if (!m_RenderPass)
        ReleaseRenderPassTransientObjects();

    m_HasViewport = false;
    m_HasScissor = false;
    m_StencilReference = 0;
    MarkDescriptorSetsDirty(BindPoint::GRAPHICS);
    m_GraphicsRootGroupDirty = true;
}

void CommandBufferWGPU::EndRendering() {
    EndPass();
}

void CommandBufferWGPU::ClearAttachments(const ClearAttachmentDesc* clearAttachmentDescs, uint32_t clearAttachmentDescNum, const Rect* rects, uint32_t rectNum) {
    if (!m_RenderPass)
        return;

    // TODO: WebGPU has no mid-pass attachment clear command. This is draw-emulated and must keep restoring caller-visible graphics state.
    WGPURenderPipeline renderPipeline = m_RenderPipeline;
    Viewport viewport = m_Viewport;
    Rect scissor = m_Scissor;
    bool hasViewport = m_HasViewport;
    bool hasScissor = m_HasScissor;
    uint8_t stencilReference = m_StencilReference;

    auto drawClear = [&](const Rect* clearRects, uint32_t clearRectNum) {
        if (clearRectNum) {
            for (uint32_t j = 0; j < clearRectNum; j++) {
                if (SetScissorRect(clearRects[j]))
                    wgpuRenderPassEncoderDraw(m_RenderPass, 3, 1, 0, 0);
            }
        } else {
            wgpuRenderPassEncoderSetScissorRect(m_RenderPass, 0, 0, m_RenderWidth, m_RenderHeight);
            wgpuRenderPassEncoderDraw(m_RenderPass, 3, 1, 0, 0);
        }
    };

    wgpuRenderPassEncoderSetViewport(m_RenderPass, 0.0f, 0.0f, (float)m_RenderWidth, (float)m_RenderHeight, 0.0f, 1.0f);

    for (uint32_t i = 0; i < clearAttachmentDescNum; i++) {
        const ClearAttachmentDesc& clearAttachmentDesc = clearAttachmentDescs[i];

        uint32_t colorAttachmentIndex = clearAttachmentDesc.colorAttachmentIndex;
        if (colorAttachmentIndex < m_RenderColorNum && (NormalizeClearPlanesWGPU(clearAttachmentDesc.planes, m_RenderColorFormats[colorAttachmentIndex]) & PlaneBits::COLOR) && m_RenderColorFormats[colorAttachmentIndex] != Format::UNKNOWN) {
            WGPUPipelineLayout clearPipelineLayout = nullptr;
            WGPURenderPipeline clearPipeline = GetClearPipeline(colorAttachmentIndex, PlaneBits::COLOR, clearPipelineLayout);
            MaybeUnused(clearPipelineLayout);
            if (clearPipeline) {
                const FormatProps& props = GetFormatProps(m_RenderColorFormats[colorAttachmentIndex]);
                const void* clearData = props.isInteger ? (props.isSigned ? (const void*)&clearAttachmentDesc.value.color.i : (const void*)&clearAttachmentDesc.value.color.ui) : (const void*)&clearAttachmentDesc.value.color.f;

                wgpuRenderPassEncoderSetPipeline(m_RenderPass, clearPipeline);
                wgpuRenderPassEncoderSetImmediates(m_RenderPass, 0, sizeof(clearAttachmentDesc.value.color), clearData);
                drawClear(rects, rectNum);
            }
        }

        PlaneBits depthStencilPlanes = (PlaneBits)(NormalizeClearPlanesWGPU(clearAttachmentDesc.planes, m_RenderDepthStencilFormat) & (PlaneBits::DEPTH | PlaneBits::STENCIL));
        if (depthStencilPlanes != PlaneBits::NONE && depthStencilPlanes != PlaneBits::ALL && m_RenderDepthStencilFormat != Format::UNKNOWN) {
            WGPUPipelineLayout clearPipelineLayout = nullptr;
            WGPURenderPipeline clearPipeline = GetClearPipeline(0, depthStencilPlanes, clearPipelineLayout);
            MaybeUnused(clearPipelineLayout);
            if (clearPipeline) {
                Color32f clearValue = {clearAttachmentDesc.value.depthStencil.depth, 0.0f, 0.0f, 0.0f};
                wgpuRenderPassEncoderSetPipeline(m_RenderPass, clearPipeline);
                wgpuRenderPassEncoderSetImmediates(m_RenderPass, 0, sizeof(clearValue), &clearValue);
                if (depthStencilPlanes & PlaneBits::STENCIL)
                    wgpuRenderPassEncoderSetStencilReference(m_RenderPass, clearAttachmentDesc.value.depthStencil.stencil);

                drawClear(rects, rectNum);
            }
        }
    }

    if (renderPipeline)
        wgpuRenderPassEncoderSetPipeline(m_RenderPass, renderPipeline);

    m_RenderPipeline = renderPipeline;

    if (hasViewport) {
        wgpuRenderPassEncoderSetViewport(m_RenderPass, viewport.x, viewport.y, viewport.width, viewport.height, viewport.depthMin, viewport.depthMax);
        m_Viewport = viewport;
        m_HasViewport = true;
    }

    if (hasScissor) {
        SetScissorRect(scissor);
        m_Scissor = scissor;
        m_HasScissor = true;
    } else {
        wgpuRenderPassEncoderSetScissorRect(m_RenderPass, 0, 0, m_RenderWidth, m_RenderHeight);
        m_HasScissor = false;
    }

    wgpuRenderPassEncoderSetStencilReference(m_RenderPass, stencilReference);
    m_StencilReference = stencilReference;

    RestoreRootConstants(BindPoint::GRAPHICS);
    m_GraphicsRootGroupDirty = true;
    MarkDescriptorSetsDirty(BindPoint::GRAPHICS);
    BindRootGroup(BindPoint::GRAPHICS);
    BindDescriptorSets(BindPoint::GRAPHICS);
}

static TextureRegionDesc GetWholeTextureRegion(const TextureDesc& textureDesc) {
    TextureRegionDesc region = {};
    region.width = WHOLE_SIZE;
    region.height = WHOLE_SIZE;
    region.depth = textureDesc.type == TextureType::TEXTURE_3D ? WHOLE_SIZE : textureDesc.layerNum;
    region.planes = PlaneBits::ALL;

    return region;
}

static void FillTexelCopyTexture(WGPUTexelCopyTextureInfo& out, const TextureWGPU& texture, const TextureRegionDesc& region) {
    out = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    out.texture = texture;
    out.mipLevel = region.mipOffset;
    out.origin.x = region.x;
    out.origin.y = region.y;
    out.origin.z = (uint32_t)region.z + region.layerOffset;
    out.aspect = GetTextureAspect(region.planes);
}

static WGPUExtent3D GetCopySize(const TextureDesc& textureDesc, const TextureRegionDesc& region) {
    WGPUExtent3D size = {};
    size.width = region.width == WHOLE_SIZE ? GetDimension(GraphicsAPI::WGPU, textureDesc, 0, region.mipOffset) : region.width;
    size.height = region.height == WHOLE_SIZE ? GetDimension(GraphicsAPI::WGPU, textureDesc, 1, region.mipOffset) : region.height;
    if (textureDesc.type == TextureType::TEXTURE_3D)
        size.depthOrArrayLayers = region.depth == WHOLE_SIZE ? (uint32_t)GetDimension(GraphicsAPI::WGPU, textureDesc, 2, region.mipOffset) : (uint32_t)GetCountOrOne(region.depth);
    else
        size.depthOrArrayLayers = region.depth == WHOLE_SIZE ? textureDesc.layerNum - region.layerOffset : (uint32_t)GetCountOrOne(region.depth);

    return size;
}

void CommandBufferWGPU::Draw(const DrawDesc& drawDesc) {
    if (m_RenderPass) {
        BindRootGroup(BindPoint::GRAPHICS);
        BindDescriptorSets(BindPoint::GRAPHICS);
        wgpuRenderPassEncoderDraw(m_RenderPass, drawDesc.vertexNum, drawDesc.instanceNum, drawDesc.baseVertex, drawDesc.baseInstance);
    }
}

void CommandBufferWGPU::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    if (m_RenderPass) {
        BindRootGroup(BindPoint::GRAPHICS);
        BindDescriptorSets(BindPoint::GRAPHICS);
        wgpuRenderPassEncoderDrawIndexed(m_RenderPass, drawIndexedDesc.indexNum, drawIndexedDesc.instanceNum, drawIndexedDesc.baseIndex, drawIndexedDesc.baseVertex, drawIndexedDesc.baseInstance);
    }
}

void CommandBufferWGPU::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    // TODO: WebGPU has no count-buffer indirect draw variant, so "countBuffer" is ignored and "drawNum" commands are emitted.
    MaybeUnused(countBuffer, countBufferOffset);

    if (!m_RenderPass)
        return;

    BindRootGroup(BindPoint::GRAPHICS);
    BindDescriptorSets(BindPoint::GRAPHICS);

    const BufferWGPU& bufferWGPU = (BufferWGPU&)buffer;
    for (uint32_t i = 0; i < drawNum; i++)
        wgpuRenderPassEncoderDrawIndirect(m_RenderPass, bufferWGPU, offset + (uint64_t)i * stride);
}

void CommandBufferWGPU::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    // TODO: WebGPU has no count-buffer indirect draw variant, so "countBuffer" is ignored and "drawNum" commands are emitted.
    MaybeUnused(countBuffer, countBufferOffset);

    if (!m_RenderPass)
        return;

    BindRootGroup(BindPoint::GRAPHICS);
    BindDescriptorSets(BindPoint::GRAPHICS);

    const BufferWGPU& bufferWGPU = (BufferWGPU&)buffer;
    for (uint32_t i = 0; i < drawNum; i++)
        wgpuRenderPassEncoderDrawIndexedIndirect(m_RenderPass, bufferWGPU, offset + (uint64_t)i * stride);
}

void CommandBufferWGPU::Dispatch(const DispatchDesc& dispatchDesc) {
    if (!m_ComputePass) {
        WGPUComputePassDescriptor desc = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
        m_ComputePass = wgpuCommandEncoderBeginComputePass(m_CommandEncoder, &desc);
        MarkDescriptorSetsDirty(BindPoint::COMPUTE);
        m_ComputeRootGroupDirty = true;
    }

    if (m_ComputePipeline && m_ComputePipeline != m_BoundComputePipeline) {
        wgpuComputePassEncoderSetPipeline(m_ComputePass, m_ComputePipeline);
        m_BoundComputePipeline = m_ComputePipeline;
        RestoreRootConstants(BindPoint::COMPUTE);
    }

    BindRootGroup(BindPoint::COMPUTE);
    BindDescriptorSets(BindPoint::COMPUTE);
    wgpuComputePassEncoderDispatchWorkgroups(m_ComputePass, dispatchDesc.x, dispatchDesc.y, dispatchDesc.z);
}

void CommandBufferWGPU::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    if (!m_ComputePass) {
        WGPUComputePassDescriptor desc = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
        m_ComputePass = wgpuCommandEncoderBeginComputePass(m_CommandEncoder, &desc);
        MarkDescriptorSetsDirty(BindPoint::COMPUTE);
        m_ComputeRootGroupDirty = true;
    }

    if (m_ComputePipeline && m_ComputePipeline != m_BoundComputePipeline) {
        wgpuComputePassEncoderSetPipeline(m_ComputePass, m_ComputePipeline);
        m_BoundComputePipeline = m_ComputePipeline;
        RestoreRootConstants(BindPoint::COMPUTE);
    }

    BindRootGroup(BindPoint::COMPUTE);
    BindDescriptorSets(BindPoint::COMPUTE);
    wgpuComputePassEncoderDispatchWorkgroupsIndirect(m_ComputePass, (BufferWGPU&)buffer, offset);
}

void CommandBufferWGPU::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    EndPass();

    wgpuCommandEncoderCopyBufferToBuffer(m_CommandEncoder, (BufferWGPU&)srcBuffer, srcOffset, (BufferWGPU&)dstBuffer, dstOffset, size);
}

void CommandBufferWGPU::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    EndPass();

    TextureWGPU& dstTextureWGPU = (TextureWGPU&)dstTexture;
    const TextureWGPU& srcTextureWGPU = (const TextureWGPU&)srcTexture;
    const TextureDesc& dstTextureDesc = dstTextureWGPU.GetDesc();
    const TextureDesc& srcTextureDesc = srcTextureWGPU.GetDesc();
    TextureRegionDesc dstWholeRegion = GetWholeTextureRegion(dstTextureDesc);
    TextureRegionDesc srcWholeRegion = GetWholeTextureRegion(srcTextureDesc);
    const TextureRegionDesc& dst = dstRegion ? *dstRegion : dstWholeRegion;
    const TextureRegionDesc& src = srcRegion ? *srcRegion : srcWholeRegion;

    WGPUTexelCopyTextureInfo srcInfo = {};
    WGPUTexelCopyTextureInfo dstInfo = {};
    FillTexelCopyTexture(srcInfo, srcTextureWGPU, src);
    FillTexelCopyTexture(dstInfo, dstTextureWGPU, dst);

    WGPUExtent3D size = GetCopySize(srcTextureDesc, src);
    wgpuCommandEncoderCopyTextureToTexture(m_CommandEncoder, &srcInfo, &dstInfo, &size);
}

void CommandBufferWGPU::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegion, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayout) {
    EndPass();

    TextureWGPU& textureWGPU = (TextureWGPU&)dstTexture;
    const TextureDesc& textureDesc = textureWGPU.GetDesc();

    WGPUTexelCopyBufferInfo src = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
    src.buffer = (BufferWGPU&)srcBuffer;
    src.layout.offset = srcDataLayout.offset;
    src.layout.bytesPerRow = srcDataLayout.rowPitch;
    if (srcDataLayout.slicePitch && srcDataLayout.rowPitch)
        src.layout.rowsPerImage = srcDataLayout.slicePitch / srcDataLayout.rowPitch;

    WGPUTexelCopyTextureInfo dst = {};
    FillTexelCopyTexture(dst, textureWGPU, dstRegion);

    WGPUExtent3D size = GetCopySize(textureDesc, dstRegion);
    wgpuCommandEncoderCopyBufferToTexture(m_CommandEncoder, &src, &dst, &size);
}

void CommandBufferWGPU::ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayout, const Texture& srcTexture, const TextureRegionDesc& srcRegion) {
    EndPass();

    const TextureWGPU& textureWGPU = (const TextureWGPU&)srcTexture;
    const TextureDesc& textureDesc = textureWGPU.GetDesc();

    WGPUTexelCopyTextureInfo src = {};
    FillTexelCopyTexture(src, textureWGPU, srcRegion);

    WGPUTexelCopyBufferInfo dst = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
    dst.buffer = (BufferWGPU&)dstBuffer;
    dst.layout.offset = dstDataLayout.offset;
    dst.layout.bytesPerRow = dstDataLayout.rowPitch;
    if (dstDataLayout.slicePitch && dstDataLayout.rowPitch)
        dst.layout.rowsPerImage = dstDataLayout.slicePitch / dstDataLayout.rowPitch;

    WGPUExtent3D size = GetCopySize(textureDesc, srcRegion);
    wgpuCommandEncoderCopyTextureToBuffer(m_CommandEncoder, &src, &dst, &size);
}

void CommandBufferWGPU::ZeroBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    EndPass();

    BufferWGPU& bufferWGPU = (BufferWGPU&)buffer;
    uint64_t clearSize = size == WHOLE_SIZE ? bufferWGPU.GetSize() - offset : size;
    clearSize &= ~3ull;
    if (clearSize)
        wgpuCommandEncoderClearBuffer(m_CommandEncoder, bufferWGPU, offset, clearSize);
}

static uint32_t GetClearChannelBits(const FormatProps& props, uint32_t channelIndex) {
    switch (channelIndex) {
        case 0:
            return props.redBits;
        case 1:
            return props.greenBits;
        case 2:
            return props.blueBits;
        case 3:
            return props.alphaBits;
        default:
            return 0;
    }
}

static uint32_t FloatToUnorm(float value, uint32_t bits) {
    value = std::min(std::max(value, 0.0f), 1.0f);
    uint32_t maxValue = bits == 32 ? 0xFFFFFFFFu : ((1u << bits) - 1u);
    return (uint32_t)(value * (float)maxValue + 0.5f);
}

static int32_t FloatToSnorm(float value, uint32_t bits) {
    value = std::min(std::max(value, -1.0f), 1.0f);
    int32_t maxValue = (1 << (bits - 1)) - 1;
    int32_t minValue = -maxValue;
    return std::min(std::max((int32_t)(value * (float)maxValue + (value >= 0.0f ? 0.5f : -0.5f)), minValue), maxValue);
}

static uint16_t FloatToFloat16(float value) {
    uint32_t f = 0;
    memcpy(&f, &value, sizeof(f));

    uint32_t sign = (f >> 16) & 0x8000;
    uint32_t mantissa = f & 0x007FFFFF;
    int32_t exponent = (int32_t)((f >> 23) & 0xFF) - 127 + 15;

    if (exponent <= 0) {
        if (exponent < -10)
            return (uint16_t)sign;

        mantissa = (mantissa | 0x00800000) >> (1 - exponent);
        return (uint16_t)(sign | ((mantissa + 0x00001000) >> 13));
    }

    if (exponent >= 31) {
        if (mantissa)
            return (uint16_t)(sign | 0x7E00);

        return (uint16_t)(sign | 0x7C00);
    }

    return (uint16_t)(sign | ((uint32_t)exponent << 10) | ((mantissa + 0x00001000) >> 13));
}

static void StoreClearChannel(uint8_t*& dst, const Color& value, const FormatProps& props, uint32_t channelIndex) {
    uint32_t bits = GetClearChannelBits(props, channelIndex);
    if (!bits)
        return;

    uint32_t byteNum = bits / 8;
    uint32_t bitsValue = 0;

    if (props.isFloat) {
        if (bits == 32)
            memcpy(&bitsValue, &((&value.f.x)[channelIndex]), sizeof(float));
        else if (bits == 16)
            bitsValue = FloatToFloat16((&value.f.x)[channelIndex]);
    } else if (props.isNorm) {
        if (props.isSigned)
            bitsValue = (uint32_t)FloatToSnorm((&value.f.x)[channelIndex], bits);
        else
            bitsValue = FloatToUnorm((&value.f.x)[channelIndex], bits);
    } else if (props.isInteger) {
        bitsValue = props.isSigned ? (uint32_t)(&value.i.x)[channelIndex] : (&value.ui.x)[channelIndex];
    } else
        memcpy(&bitsValue, &((&value.f.x)[channelIndex]), std::min<uint32_t>(byteNum, sizeof(bitsValue)));

    memcpy(dst, &bitsValue, byteNum);
    dst += byteNum;
}

static void FillClearPattern(uint8_t* dst, Format format, const Color& value) {
    const FormatProps& props = GetFormatProps(format);
    memset(dst, 0, props.stride);

    if (props.isPacked || props.isCompressed)
        return;

    uint8_t* at = dst;
    StoreClearChannel(at, value, props, 0);
    StoreClearChannel(at, value, props, 1);
    StoreClearChannel(at, value, props, 2);
    StoreClearChannel(at, value, props, 3);
}

static uint32_t GetPatternWordPeriod(uint32_t stride) {
    uint32_t a = stride;
    uint32_t b = 4;
    while (b) {
        uint32_t t = a % b;
        a = b;
        b = t;
    }

    uint32_t gcd = a;
    return std::max(stride / gcd, 1u);
}

static bool FillClearPatternWords(std::array<uint32_t, 4>& words, uint32_t& period, Format format, const Color& value) {
    const FormatProps& props = GetFormatProps(format);
    if (!props.stride || props.isPacked || props.isCompressed)
        return false;

    period = GetPatternWordPeriod(props.stride);
    if (period > 4)
        return false;

    std::array<uint8_t, 16> pattern = {};
    FillClearPattern(pattern.data(), format, value);

    for (uint32_t i = 0; i < period; i++) {
        std::array<uint8_t, 4> word = {};
        for (uint32_t j = 0; j < 4; j++)
            word[j] = pattern[(i * 4 + j) % props.stride];

        memcpy(&words[i], word.data(), word.size());
    }

    return true;
}

static bool IsClearValueZero(const Color& value) {
    return value.ui.x == 0 && value.ui.y == 0 && value.ui.z == 0 && value.ui.w == 0;
}

WGPUBuffer CreateTemporaryUploadBuffer(DeviceWGPU& device, uint64_t size, const void* data) {
    WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
    desc.size = Align(std::max(size, 4ull), 4);
    desc.usage = WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst;

    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &desc);
    if (buffer && data)
        wgpuQueueWriteBuffer(device.GetQueue(), buffer, 0, data, (size_t)size);

    return buffer;
}

void CommandBufferWGPU::ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion, ResolveOp resolveOp) {
    // TODO: WebGPU exposes only the default render-pass resolve behavior. "resolveOp" is ignored; keep "features.resolveOpMinMax = false".
    MaybeUnused(resolveOp);
    EndPass();

    const TextureWGPU& srcTextureWGPU = (const TextureWGPU&)srcTexture;
    TextureWGPU& dstTextureWGPU = (TextureWGPU&)dstTexture;
    const TextureDesc& srcTextureDesc = srcTextureWGPU.GetDesc();
    const TextureDesc& dstTextureDesc = dstTextureWGPU.GetDesc();

    TextureRegionDesc srcWholeRegion = GetWholeTextureRegion(srcTextureDesc);
    TextureRegionDesc dstWholeRegion = GetWholeTextureRegion(dstTextureDesc);
    const TextureRegionDesc& src = srcRegion ? *srcRegion : srcWholeRegion;
    const TextureRegionDesc& dst = dstRegion ? *dstRegion : dstWholeRegion;

    WGPUTextureViewDescriptor srcViewDesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    srcViewDesc.format = GetTextureFormat(srcTextureDesc.format);
    srcViewDesc.dimension = WGPUTextureViewDimension_2D;
    srcViewDesc.baseMipLevel = src.mipOffset;
    srcViewDesc.mipLevelCount = 1;
    srcViewDesc.baseArrayLayer = src.layerOffset;
    srcViewDesc.arrayLayerCount = 1;
    srcViewDesc.aspect = GetTextureAspect(src.planes);

    WGPUTextureViewDescriptor dstViewDesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    dstViewDesc.format = GetTextureFormat(dstTextureDesc.format);
    dstViewDesc.dimension = WGPUTextureViewDimension_2D;
    dstViewDesc.baseMipLevel = dst.mipOffset;
    dstViewDesc.mipLevelCount = 1;
    dstViewDesc.baseArrayLayer = dst.layerOffset;
    dstViewDesc.arrayLayerCount = 1;
    dstViewDesc.aspect = GetTextureAspect(dst.planes);

    WGPUTextureView srcView = wgpuTextureCreateView(srcTextureWGPU, &srcViewDesc);
    WGPUTextureView dstView = wgpuTextureCreateView(dstTextureWGPU, &dstViewDesc);

    WGPURenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
    colorAttachment.view = srcView;
    colorAttachment.resolveTarget = dstView;
    colorAttachment.loadOp = WGPULoadOp_Load;
    colorAttachment.storeOp = WGPUStoreOp_Discard;

    WGPURenderPassDescriptor desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    desc.colorAttachmentCount = 1;
    desc.colorAttachments = &colorAttachment;

    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(m_CommandEncoder, &desc);
    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);
    wgpuTextureViewRelease(srcView);
    wgpuTextureViewRelease(dstView);
}

void CommandBufferWGPU::ClearStorage(const ClearStorageDesc& clearStorageDesc) {
    EndPass();

    const DescriptorWGPU& descriptor = *(DescriptorWGPU*)clearStorageDesc.descriptor;

    if (descriptor.GetDescriptorType() == DescriptorType::STORAGE_BUFFER) {
        uint64_t size = descriptor.GetSize();
        uint64_t offset = descriptor.GetOffset();
        WGPUBuffer buffer = descriptor.GetBuffer();
        uint64_t clearSize = size & ~3ull;
        if (!clearSize)
            return;

        if (IsClearValueZero(clearStorageDesc.value)) {
            wgpuCommandEncoderClearBuffer(m_CommandEncoder, buffer, offset, clearSize);
            return;
        }

        Format format = descriptor.GetBufferFormat() == Format::UNKNOWN ? Format::R32_UINT : descriptor.GetBufferFormat();
        ClearStorageBufferConstantsWGPU constants = {};
        if (!FillClearPatternWords(constants.words, constants.period, format, clearStorageDesc.value))
            return;

        constants.wordNum = (uint32_t)(clearSize / 4);

        WGPUBindGroupLayout bindGroupLayout = nullptr;
        WGPUComputePipeline pipeline = GetClearStorageBufferPipeline(bindGroupLayout);
        if (!pipeline)
            return;

        WGPUBindGroupEntry entry = WGPU_BIND_GROUP_ENTRY_INIT;
        entry.binding = 0;
        entry.buffer = buffer;
        entry.offset = offset;
        entry.size = clearSize;

        WGPUBindGroupDescriptor bindGroupDesc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
        bindGroupDesc.layout = bindGroupLayout;
        bindGroupDesc.entryCount = 1;
        bindGroupDesc.entries = &entry;

        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(m_Device, &bindGroupDesc);
        if (!bindGroup)
            return;

        WGPUComputePassDescriptor passDesc = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
        WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(m_CommandEncoder, &passDesc);
        if (pass) {
            wgpuComputePassEncoderSetPipeline(pass, pipeline);
            wgpuComputePassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
            wgpuComputePassEncoderSetImmediates(pass, 0, sizeof(constants), &constants);
            wgpuComputePassEncoderDispatchWorkgroups(pass, DivideUpWGPU(constants.wordNum, 64u), 1, 1);
            wgpuComputePassEncoderEnd(pass);
            wgpuComputePassEncoderRelease(pass);
        }

        wgpuBindGroupRelease(bindGroup);
        return;
    }

    if (descriptor.GetDescriptorType() != DescriptorType::STORAGE_TEXTURE)
        return;

    const TextureWGPU* texture = descriptor.GetTexture();
    if (!texture)
        return;

    const TextureDesc& textureDesc = texture->GetDesc();
    const TextureViewDesc& viewDesc = descriptor.GetTextureViewDesc();
    Format format = descriptor.GetFormat();
    uint32_t width = GetDimension(GraphicsAPI::WGPU, textureDesc, 0, viewDesc.mipOffset);
    uint32_t height = GetDimension(GraphicsAPI::WGPU, textureDesc, 1, viewDesc.mipOffset);
    uint32_t layerNum = viewDesc.layerNum == REMAINING ? textureDesc.layerNum - viewDesc.layerOffset : viewDesc.layerNum;
    uint32_t depth = textureDesc.type == TextureType::TEXTURE_3D ? GetDimension(GraphicsAPI::WGPU, textureDesc, 2, viewDesc.mipOffset) : std::max(layerNum, 1u);
    WGPUTextureViewDimension dimension = GetTextureViewDimension(viewDesc.type, textureDesc);

    WGPUBindGroupLayout bindGroupLayout = nullptr;
    WGPUComputePipeline pipeline = GetClearStorageTexturePipeline(format, dimension, bindGroupLayout);
    if (!pipeline)
        return;

    WGPUTextureView textureView = ((DescriptorWGPU&)descriptor).GetTextureView();
    if (!textureView)
        return;

    WGPUBindGroupEntry entry = WGPU_BIND_GROUP_ENTRY_INIT;
    entry.binding = 0;
    entry.textureView = textureView;

    WGPUBindGroupDescriptor bindGroupDesc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &entry;

    WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(m_Device, &bindGroupDesc);
    if (!bindGroup)
        return;

    ClearStorageTextureConstantsWGPU constants = {};
    constants.f = clearStorageDesc.value.f;
    constants.u = clearStorageDesc.value.ui;
    constants.i = clearStorageDesc.value.i;
    constants.width = width;
    constants.height = std::max(height, 1u);
    constants.depth = depth;

    WGPUComputePassDescriptor passDesc = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
    WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(m_CommandEncoder, &passDesc);
    if (pass) {
        wgpuComputePassEncoderSetPipeline(pass, pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
        wgpuComputePassEncoderSetImmediates(pass, 0, sizeof(constants), &constants);
        wgpuComputePassEncoderDispatchWorkgroups(pass, DivideUpWGPU(width, 8u), DivideUpWGPU(constants.height, 8u), depth);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
    }

    wgpuBindGroupRelease(bindGroup);
}

void CommandBufferWGPU::Barrier(const BarrierDesc& barrierDesc) {
    // TODO: WebGPU owns resource-state tracking. This no-op is usually correct, but pass boundaries/copy ordering still matter.
    MaybeUnused(barrierDesc);
}

void CommandBufferWGPU::ResetQueries(QueryPool& queryPool, uint32_t offset, uint32_t num) {
    QueryPoolWGPU& queryPoolWGPU = (QueryPoolWGPU&)queryPool;
    queryPoolWGPU.Reset(offset, num);
}

void CommandBufferWGPU::BeginQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolWGPU& queryPoolWGPU = (QueryPoolWGPU&)queryPool;

    // TODO: Occlusion queries also require wiring the query set into WGPURenderPassDescriptor::occlusionQuerySet.
    if (queryPoolWGPU.GetType() == QueryType::OCCLUSION && m_RenderPass)
        wgpuRenderPassEncoderBeginOcclusionQuery(m_RenderPass, offset);
}

void CommandBufferWGPU::EndQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolWGPU& queryPoolWGPU = (QueryPoolWGPU&)queryPool;

    if (queryPoolWGPU.GetType() == QueryType::TIMESTAMP) {
        if (m_RenderPass)
            wgpuRenderPassEncoderWriteTimestamp(m_RenderPass, queryPoolWGPU, offset);
        else if (m_ComputePass)
            wgpuComputePassEncoderWriteTimestamp(m_ComputePass, queryPoolWGPU, offset);
        else
            wgpuCommandEncoderWriteTimestamp(m_CommandEncoder, queryPoolWGPU, offset);
        return;
    }

    if (queryPoolWGPU.GetType() == QueryType::OCCLUSION && m_RenderPass)
        wgpuRenderPassEncoderEndOcclusionQuery(m_RenderPass);
}

void CommandBufferWGPU::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    EndPass();

    const QueryPoolWGPU& queryPoolWGPU = (const QueryPoolWGPU&)queryPool;
    BufferWGPU& dstBufferWGPU = (BufferWGPU&)dstBuffer;
    uint64_t queryDataSize = (uint64_t)num * queryPoolWGPU.GetQuerySize();
    if (!queryDataSize)
        return;

    if (dstBufferWGPU.IsHostReadback()) {
        WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
        desc.size = Align(std::max(queryDataSize, 4ull), 4);
        desc.usage = WGPUBufferUsage_QueryResolve | WGPUBufferUsage_CopySrc;

        WGPUBuffer resolveBuffer = wgpuDeviceCreateBuffer(m_Device, &desc);
        if (!resolveBuffer)
            return;

        m_TemporaryBuffers.push_back(resolveBuffer);
        wgpuCommandEncoderResolveQuerySet(m_CommandEncoder, queryPoolWGPU, offset, num, resolveBuffer, 0);
        wgpuCommandEncoderCopyBufferToBuffer(m_CommandEncoder, resolveBuffer, 0, dstBufferWGPU, dstOffset, queryDataSize);
        return;
    }

    wgpuCommandEncoderResolveQuerySet(m_CommandEncoder, queryPoolWGPU, offset, num, dstBufferWGPU, dstOffset);
}

void CommandBufferWGPU::BeginAnnotation(const char* name, uint32_t bgra) {
    MaybeUnused(bgra);

    if (m_RenderPass) {
        wgpuRenderPassEncoderPushDebugGroup(m_RenderPass, WGPUString(name));
        m_AnnotationScopes.push_back(AnnotationScopeWGPU::RENDER_PASS);
        return;
    }

    if (m_ComputePass) {
        wgpuComputePassEncoderPushDebugGroup(m_ComputePass, WGPUString(name));
        m_AnnotationScopes.push_back(AnnotationScopeWGPU::COMPUTE_PASS);
        return;
    }

    if (m_CommandEncoder) {
        wgpuCommandEncoderPushDebugGroup(m_CommandEncoder, WGPUString(name));
        m_AnnotationScopes.push_back(AnnotationScopeWGPU::COMMAND_ENCODER);
    }
}

void CommandBufferWGPU::EndAnnotation() {
    if (m_AnnotationScopes.empty())
        return;

    AnnotationScopeWGPU scope = m_AnnotationScopes.back();
    m_AnnotationScopes.pop_back();

    if (scope == AnnotationScopeWGPU::RENDER_PASS) {
        if (m_RenderPass)
            wgpuRenderPassEncoderPopDebugGroup(m_RenderPass);

        return;
    }

    if (scope == AnnotationScopeWGPU::COMPUTE_PASS) {
        if (m_ComputePass)
            wgpuComputePassEncoderPopDebugGroup(m_ComputePass);

        return;
    }

    if (m_RenderPass || m_ComputePass) {
        m_DeferredEncoderAnnotationPopNum++;
        return;
    }

    if (m_CommandEncoder)
        wgpuCommandEncoderPopDebugGroup(m_CommandEncoder);
}

void CommandBufferWGPU::Annotation(const char* name, uint32_t bgra) {
    MaybeUnused(bgra);

    if (m_RenderPass) {
        wgpuRenderPassEncoderInsertDebugMarker(m_RenderPass, WGPUString(name));
        return;
    }

    if (m_ComputePass) {
        wgpuComputePassEncoderInsertDebugMarker(m_ComputePass, WGPUString(name));
        return;
    }

    if (m_CommandEncoder)
        wgpuCommandEncoderInsertDebugMarker(m_CommandEncoder, WGPUString(name));
}
