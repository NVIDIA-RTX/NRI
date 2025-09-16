// © 2021 NVIDIA Corporation

template <typename DescComponent, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobjectType>
struct alignas(void*) PipelineDescComponent {
    PipelineDescComponent() = default;

    inline void operator=(const DescComponent& d) {
        desc = d;
    }

    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = subobjectType;
    DescComponent desc = {};
};

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
typedef PipelineDescComponent<D3D12_RASTERIZER_DESC1, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER> PipelineRasterizer;
typedef PipelineDescComponent<D3D12_DEPTH_STENCIL_DESC2, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2> PipelineDepthStencil;
#else
typedef PipelineDescComponent<D3D12_RASTERIZER_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER> PipelineRasterizer;
typedef PipelineDescComponent<D3D12_DEPTH_STENCIL_DESC1, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1> PipelineDepthStencil;
#endif

typedef PipelineDescComponent<ID3D12RootSignature*, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE> PipelineRootSignature;
typedef PipelineDescComponent<D3D12_INPUT_LAYOUT_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT> PipelineInputLayout;
typedef PipelineDescComponent<D3D12_INDEX_BUFFER_STRIP_CUT_VALUE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE> PipelineIndexBufferStripCutValue;
typedef PipelineDescComponent<D3D12_PRIMITIVE_TOPOLOGY_TYPE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY> PipelinePrimitiveTopology;
typedef PipelineDescComponent<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS> PipelineVertexShader;
typedef PipelineDescComponent<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS> PipelineHullShader;
typedef PipelineDescComponent<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS> PipelineDomainShader;
typedef PipelineDescComponent<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS> PipelineGeometryShader;
typedef PipelineDescComponent<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS> PipelineAmplificationShader;
typedef PipelineDescComponent<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS> PipelineMeshShader;
typedef PipelineDescComponent<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS> PipelinePixelShader;
typedef PipelineDescComponent<UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK> PipelineNodeMask;
typedef PipelineDescComponent<DXGI_SAMPLE_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC> PipelineSampleDesc;
typedef PipelineDescComponent<UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK> PipelineSampleMask;
typedef PipelineDescComponent<D3D12_BLEND_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND> PipelineBlend;
typedef PipelineDescComponent<DXGI_FORMAT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT> PipelineDepthStencilFormat;
typedef PipelineDescComponent<D3D12_RT_FORMAT_ARRAY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS> PipelineRenderTargetFormats;
typedef PipelineDescComponent<D3D12_PIPELINE_STATE_FLAGS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS> PipelineFlags;
typedef PipelineDescComponent<D3D12_VIEW_INSTANCING_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING> PipelineViewInstancing;

static_assert((uint32_t)PrimitiveRestart::DISABLED == D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED, "Enum mismatch");
static_assert((uint32_t)PrimitiveRestart::INDICES_UINT16 == D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF, "Enum mismatch");
static_assert((uint32_t)PrimitiveRestart::INDICES_UINT32 == D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF, "Enum mismatch");

static inline D3D12_SHADER_VISIBILITY GetShaderVisibility(StageBits shaderStages) {
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

    if (shaderStages == StageBits::TASK_SHADER)
        return D3D12_SHADER_VISIBILITY_AMPLIFICATION;

    if (shaderStages == StageBits::MESH_SHADER)
        return D3D12_SHADER_VISIBILITY_MESH;

    return D3D12_SHADER_VISIBILITY_ALL;
}

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
static inline void FillRasterizerState(D3D12_RASTERIZER_DESC1& rasterizerDesc, const GraphicsPipelineDesc& graphicsPipelineDesc) {
#else
static inline void FillRasterizerState(D3D12_RASTERIZER_DESC& rasterizerDesc, const GraphicsPipelineDesc& graphicsPipelineDesc) {
#endif
    const RasterizationDesc& r = graphicsPipelineDesc.rasterization;

    rasterizerDesc.FillMode = GetFillMode(r.fillMode);
    rasterizerDesc.CullMode = GetCullMode(r.cullMode);
    rasterizerDesc.FrontCounterClockwise = (BOOL)r.frontCounterClockwise;
    rasterizerDesc.DepthBiasClamp = r.depthBias.clamp;
    rasterizerDesc.SlopeScaledDepthBias = r.depthBias.slope;
    rasterizerDesc.DepthClipEnable = (BOOL)r.depthClamp;
    rasterizerDesc.AntialiasedLineEnable = (BOOL)r.lineSmoothing;
    rasterizerDesc.ConservativeRaster = r.conservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    rasterizerDesc.DepthBias = r.depthBias.constant;
#else
    rasterizerDesc.DepthBias = (int32_t)r.depthBias.constant;
#endif

    if (graphicsPipelineDesc.multisample) {
        rasterizerDesc.MultisampleEnable = graphicsPipelineDesc.multisample->sampleNum > 1 ? TRUE : FALSE;
        // TODO: rasterizerDesc.ForcedSampleCount?
    }
}

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
static inline void FillDepthStencilState(D3D12_DEPTH_STENCIL_DESC2& depthStencilDesc, const OutputMergerDesc& om) {
#else
static inline void FillDepthStencilState(D3D12_DEPTH_STENCIL_DESC1& depthStencilDesc, const OutputMergerDesc& om) {
#endif
    depthStencilDesc.DepthEnable = om.depth.compareOp == CompareOp::NONE ? FALSE : TRUE;
    depthStencilDesc.DepthWriteMask = om.depth.write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc = GetCompareOp(om.depth.compareOp);
    depthStencilDesc.StencilEnable = (om.stencil.front.compareOp == CompareOp::NONE && om.stencil.back.compareOp == CompareOp::NONE) ? FALSE : TRUE;
    depthStencilDesc.FrontFace.StencilFailOp = GetStencilOp(om.stencil.front.failOp);
    depthStencilDesc.FrontFace.StencilDepthFailOp = GetStencilOp(om.stencil.front.depthFailOp);
    depthStencilDesc.FrontFace.StencilPassOp = GetStencilOp(om.stencil.front.passOp);
    depthStencilDesc.FrontFace.StencilFunc = GetCompareOp(om.stencil.front.compareOp);
    depthStencilDesc.BackFace.StencilFailOp = GetStencilOp(om.stencil.back.failOp);
    depthStencilDesc.BackFace.StencilDepthFailOp = GetStencilOp(om.stencil.back.depthFailOp);
    depthStencilDesc.BackFace.StencilPassOp = GetStencilOp(om.stencil.back.passOp);
    depthStencilDesc.BackFace.StencilFunc = GetCompareOp(om.stencil.back.compareOp);
    depthStencilDesc.DepthBoundsTestEnable = om.depth.boundsTest ? 1 : 0;

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    depthStencilDesc.FrontFace.StencilReadMask = om.stencil.front.compareMask;
    depthStencilDesc.FrontFace.StencilWriteMask = om.stencil.front.writeMask;
    depthStencilDesc.BackFace.StencilReadMask = om.stencil.back.compareMask;
    depthStencilDesc.BackFace.StencilWriteMask = om.stencil.back.writeMask;
#else
    depthStencilDesc.StencilReadMask = (UINT8)om.stencil.front.compareMask;
    depthStencilDesc.StencilWriteMask = (UINT8)om.stencil.front.writeMask;
#endif
}

static inline void FillInputLayout(D3D12_INPUT_LAYOUT_DESC& inputLayoutDesc, const GraphicsPipelineDesc& graphicsPipelineDesc) {
    if (!graphicsPipelineDesc.vertexInput)
        return;

    const VertexInputDesc& vi = *graphicsPipelineDesc.vertexInput;

    inputLayoutDesc.NumElements = vi.attributeNum;

    D3D12_INPUT_ELEMENT_DESC* inputElementsDescs = (D3D12_INPUT_ELEMENT_DESC*)inputLayoutDesc.pInputElementDescs;
    for (uint32_t i = 0; i < vi.attributeNum; i++) {
        const VertexAttributeDesc& attribute = vi.attributes[i];
        const VertexStreamDesc& stream = vi.streams[attribute.streamIndex];
        bool isPerVertexData = stream.stepRate == VertexStreamStepRate::PER_VERTEX;

        inputElementsDescs[i].SemanticName = attribute.d3d.semanticName;
        inputElementsDescs[i].SemanticIndex = attribute.d3d.semanticIndex;
        inputElementsDescs[i].Format = GetDxgiFormat(attribute.format).typed;
        inputElementsDescs[i].InputSlot = stream.bindingSlot;
        inputElementsDescs[i].AlignedByteOffset = attribute.offset;
        inputElementsDescs[i].InputSlotClass = isPerVertexData ? D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA : D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
        inputElementsDescs[i].InstanceDataStepRate = isPerVertexData ? 0 : 1;
    }
}

static inline void FillShaderBytecode(D3D12_SHADER_BYTECODE& shaderBytecode, const ShaderDesc& shaderDesc) {
    shaderBytecode.pShaderBytecode = shaderDesc.bytecode;
    shaderBytecode.BytecodeLength = (size_t)shaderDesc.size;
}

static inline void FillBlendState(D3D12_BLEND_DESC& blendDesc, const GraphicsPipelineDesc& graphicsPipelineDesc) {
    const OutputMergerDesc& om = graphicsPipelineDesc.outputMerger;

    blendDesc.AlphaToCoverageEnable = (graphicsPipelineDesc.multisample && graphicsPipelineDesc.multisample->alphaToCoverage) ? TRUE : FALSE;
    blendDesc.IndependentBlendEnable = TRUE;

    for (uint32_t i = 0; i < om.colorNum; i++) {
        const ColorAttachmentDesc& colorAttachmentDesc = om.colors[i];

        blendDesc.RenderTarget[i].RenderTargetWriteMask = GetRenderTargetWriteMask(colorAttachmentDesc.colorWriteMask);
        blendDesc.RenderTarget[i].BlendEnable = colorAttachmentDesc.blendEnabled;

        if (colorAttachmentDesc.blendEnabled) {
            blendDesc.RenderTarget[i].LogicOp = GetLogicOp(om.logicOp);
            blendDesc.RenderTarget[i].LogicOpEnable = om.logicOp != LogicOp::NONE ? TRUE : FALSE;
            blendDesc.RenderTarget[i].SrcBlend = GetBlend(colorAttachmentDesc.colorBlend.srcFactor);
            blendDesc.RenderTarget[i].DestBlend = GetBlend(colorAttachmentDesc.colorBlend.dstFactor);
            blendDesc.RenderTarget[i].BlendOp = GetBlendOp(colorAttachmentDesc.colorBlend.op);
            blendDesc.RenderTarget[i].SrcBlendAlpha = GetBlend(colorAttachmentDesc.alphaBlend.srcFactor);
            blendDesc.RenderTarget[i].DestBlendAlpha = GetBlend(colorAttachmentDesc.alphaBlend.dstFactor);
            blendDesc.RenderTarget[i].BlendOpAlpha = GetBlendOp(colorAttachmentDesc.alphaBlend.op);
        }
    }
}

static inline uint32_t FillSampleDesc(DXGI_SAMPLE_DESC& sampleDesc, const GraphicsPipelineDesc& graphicsPipelineDesc) {
    if (!graphicsPipelineDesc.multisample) {
        sampleDesc.Count = 1;
        return uint32_t(-1);
    }

    sampleDesc.Count = graphicsPipelineDesc.multisample->sampleNum;
    sampleDesc.Quality = 0;

    return graphicsPipelineDesc.multisample->sampleMask != ALL ? graphicsPipelineDesc.multisample->sampleMask : uint32_t(-1);
}

Result PipelineD3D12::CreateFromStream(const GraphicsPipelineDesc& graphicsPipelineDesc) {
    struct PipelineStateStream {
        PipelineRootSignature rootSignature;
        PipelinePrimitiveTopology primitiveTopology;
        PipelineInputLayout inputLayout;
        PipelineIndexBufferStripCutValue indexBufferStripCutValue;
        PipelineVertexShader vertexShader;
        PipelineHullShader hullShader;
        PipelineDomainShader domainShader;
        PipelineGeometryShader geometryShader;
        PipelineAmplificationShader amplificationShader;
        PipelineMeshShader meshShader;
        PipelinePixelShader pixelShader;
        PipelineSampleDesc sampleDesc;
        PipelineSampleMask sampleMask;
        PipelineRasterizer rasterizer;
        PipelineBlend blend;
        PipelineRenderTargetFormats renderTargetFormats;
        PipelineDepthStencil depthStencil;
        PipelineDepthStencilFormat depthStencilFormat;
        PipelineNodeMask nodeMask;
        PipelineFlags flags;
        PipelineViewInstancing viewInstancing;
    };

    PipelineStateStream stateStream = {};
    stateStream.rootSignature = *m_PipelineLayout;
    stateStream.nodeMask = NODE_MASK;

    // Shaders
    for (uint32_t i = 0; i < graphicsPipelineDesc.shaderNum; i++) {
        const ShaderDesc& shader = graphicsPipelineDesc.shaders[i];
        if (shader.stage == StageBits::VERTEX_SHADER)
            FillShaderBytecode(stateStream.vertexShader.desc, shader);
        else if (shader.stage == StageBits::TESS_CONTROL_SHADER)
            FillShaderBytecode(stateStream.hullShader.desc, shader);
        else if (shader.stage == StageBits::TESS_EVALUATION_SHADER)
            FillShaderBytecode(stateStream.domainShader.desc, shader);
        else if (shader.stage == StageBits::GEOMETRY_SHADER)
            FillShaderBytecode(stateStream.geometryShader.desc, shader);
        else if (shader.stage == StageBits::TASK_SHADER)
            FillShaderBytecode(stateStream.amplificationShader.desc, shader);
        else if (shader.stage == StageBits::MESH_SHADER)
            FillShaderBytecode(stateStream.meshShader.desc, shader);
        else if (shader.stage == StageBits::FRAGMENT_SHADER)
            FillShaderBytecode(stateStream.pixelShader.desc, shader);
        else
            return Result::INVALID_ARGUMENT;
    }

    // Vertex input
    uint32_t attributeNum = graphicsPipelineDesc.vertexInput ? graphicsPipelineDesc.vertexInput->attributeNum : 0;
    Scratch<D3D12_INPUT_ELEMENT_DESC> scratch1 = AllocateScratch(m_Device, D3D12_INPUT_ELEMENT_DESC, attributeNum);
    if (graphicsPipelineDesc.vertexInput) {
        stateStream.inputLayout.desc.pInputElementDescs = scratch1;
        FillInputLayout(stateStream.inputLayout.desc, graphicsPipelineDesc);
    }

    // Input assembly
    m_PrimitiveTopology = ::GetPrimitiveTopology(graphicsPipelineDesc.inputAssembly.topology, graphicsPipelineDesc.inputAssembly.tessControlPointNum);
    stateStream.primitiveTopology = GetPrimitiveTopologyType(graphicsPipelineDesc.inputAssembly.topology);
    stateStream.indexBufferStripCutValue = (D3D12_INDEX_BUFFER_STRIP_CUT_VALUE)graphicsPipelineDesc.inputAssembly.primitiveRestart;

    // Multisample
    stateStream.sampleMask.desc = FillSampleDesc(stateStream.sampleDesc.desc, graphicsPipelineDesc);

    // Rasterizer
    FillRasterizerState(stateStream.rasterizer.desc, graphicsPipelineDesc);
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    if (IsDepthBiasEnabled(graphicsPipelineDesc.rasterization.depthBias))
        stateStream.flags = D3D12_PIPELINE_STATE_FLAG_DYNAMIC_DEPTH_BIAS;
#endif

    // Depth stencil
    FillDepthStencilState(stateStream.depthStencil.desc, graphicsPipelineDesc.outputMerger);
    stateStream.depthStencilFormat = GetDxgiFormat(graphicsPipelineDesc.outputMerger.depthStencilFormat).typed;

    // Output merger
    FillBlendState(stateStream.blend.desc, graphicsPipelineDesc);

    stateStream.renderTargetFormats.desc.NumRenderTargets = graphicsPipelineDesc.outputMerger.colorNum;
    for (uint32_t i = 0; i < graphicsPipelineDesc.outputMerger.colorNum; i++)
        stateStream.renderTargetFormats.desc.RTFormats[i] = GetDxgiFormat(graphicsPipelineDesc.outputMerger.colors[i].format).typed;

    // View instancing
    uint32_t viewNum = 0;
    uint32_t viewMask = graphicsPipelineDesc.outputMerger.viewMask;
    while (viewMask) {
        viewNum++;
        viewMask >>= 1;
    }

    Scratch<D3D12_VIEW_INSTANCE_LOCATION> scratch2 = AllocateScratch(m_Device, D3D12_VIEW_INSTANCE_LOCATION, viewNum);
    D3D12_VIEW_INSTANCE_LOCATION* pViewInstanceLocations = scratch2;
    if (viewNum) {
        for (uint32_t i = 0; i < viewNum; i++) {
            pViewInstanceLocations[i].ViewportArrayIndex = graphicsPipelineDesc.outputMerger.multiview == Multiview::VIEWPORT_BASED ? i : 0;
            pViewInstanceLocations[i].RenderTargetArrayIndex = graphicsPipelineDesc.outputMerger.multiview == Multiview::LAYER_BASED ? i : 0;
        }

        stateStream.viewInstancing.desc.ViewInstanceCount = viewNum;
        stateStream.viewInstancing.desc.pViewInstanceLocations = pViewInstanceLocations;
        stateStream.viewInstancing.desc.Flags = graphicsPipelineDesc.outputMerger.multiview == Multiview::FLEXIBLE ? D3D12_VIEW_INSTANCING_FLAG_ENABLE_VIEW_INSTANCE_MASKING : D3D12_VIEW_INSTANCING_FLAG_NONE;
    }

    // Create
    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {};
    pipelineStateStreamDesc.pPipelineStateSubobjectStream = &stateStream;
    pipelineStateStreamDesc.SizeInBytes = sizeof(stateStream);

    HRESULT hr = m_Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device2::CreatePipelineState");

    return Result::SUCCESS;
}

Result PipelineD3D12::Create(const GraphicsPipelineDesc& graphicsPipelineDesc) {
    m_PipelineLayout = (PipelineLayoutD3D12*)graphicsPipelineDesc.pipelineLayout;

    return CreateFromStream(graphicsPipelineDesc);
}

Result PipelineD3D12::Create(const ComputePipelineDesc& computePipelineDesc) {
    m_PipelineLayout = (PipelineLayoutD3D12*)computePipelineDesc.pipelineLayout;

    D3D12_COMPUTE_PIPELINE_STATE_DESC computePipleineStateDesc = {};
    computePipleineStateDesc.NodeMask = NODE_MASK;
    computePipleineStateDesc.pRootSignature = *m_PipelineLayout;

    FillShaderBytecode(computePipleineStateDesc.CS, computePipelineDesc.shader);

    HRESULT hr = m_Device->CreateComputePipelineState(&computePipleineStateDesc, IID_PPV_ARGS(&m_PipelineState));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateComputePipelineState");

    return Result::SUCCESS;
}

Result PipelineD3D12::Create(const RayTracingPipelineDesc& rayTracingPipelineDesc) {
    m_PipelineLayout = (PipelineLayoutD3D12*)rayTracingPipelineDesc.pipelineLayout;

    ID3D12RootSignature* rootSignature = *m_PipelineLayout;

    uint32_t stateSubobjectNum = 0;
    uint32_t shaderNum = rayTracingPipelineDesc.shaderLibrary ? rayTracingPipelineDesc.shaderLibrary->shaderNum : 0;
    uint32_t stateObjectNum = 1 // pipeline config
        + 1                     // shader config
        + 1                     // node mask
        + shaderNum             // DXIL libraries
        + rayTracingPipelineDesc.shaderGroupNum
        + (rootSignature ? 1 : 0);
    Scratch<D3D12_STATE_SUBOBJECT> stateSubobjects = AllocateScratch(m_Device, D3D12_STATE_SUBOBJECT, stateObjectNum);

    D3D12_RAYTRACING_PIPELINE_CONFIG1 rayTracingPipelineConfig = {};
    {
        rayTracingPipelineConfig.MaxTraceRecursionDepth = rayTracingPipelineDesc.recursionMaxDepth;
        rayTracingPipelineConfig.Flags = D3D12_RAYTRACING_PIPELINE_FLAG_NONE;

        if (rayTracingPipelineDesc.flags & RayTracingPipelineBits::SKIP_TRIANGLES)
            rayTracingPipelineConfig.Flags |= D3D12_RAYTRACING_PIPELINE_FLAG_SKIP_TRIANGLES;
        if (rayTracingPipelineDesc.flags & RayTracingPipelineBits::SKIP_AABBS)
            rayTracingPipelineConfig.Flags |= D3D12_RAYTRACING_PIPELINE_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
#ifdef NRI_D3D12_HAS_OPACITY_MICROMAP
        if (rayTracingPipelineDesc.flags & RayTracingPipelineBits::ALLOW_MICROMAPS)
            rayTracingPipelineConfig.Flags |= D3D12_RAYTRACING_PIPELINE_FLAG_ALLOW_OPACITY_MICROMAPS;
#endif

        stateSubobjects[stateSubobjectNum].Type = m_Device.GetDesc().tiers.rayTracing > 1 ? D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG1 : D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        stateSubobjects[stateSubobjectNum].pDesc = &rayTracingPipelineConfig;
        stateSubobjectNum++;
    }

    D3D12_RAYTRACING_SHADER_CONFIG rayTracingShaderConfig = {};
    {
        rayTracingShaderConfig.MaxPayloadSizeInBytes = rayTracingPipelineDesc.rayPayloadMaxSize;
        rayTracingShaderConfig.MaxAttributeSizeInBytes = rayTracingPipelineDesc.rayHitAttributeMaxSize;

        stateSubobjects[stateSubobjectNum].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        stateSubobjects[stateSubobjectNum].pDesc = &rayTracingShaderConfig;
        stateSubobjectNum++;
    }

    D3D12_NODE_MASK nodeMask = {};
    {
        nodeMask.NodeMask = NODE_MASK;

        stateSubobjects[stateSubobjectNum].Type = D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK;
        stateSubobjects[stateSubobjectNum].pDesc = &nodeMask;
        stateSubobjectNum++;
    }

    D3D12_GLOBAL_ROOT_SIGNATURE globalRootSignature = {};
    if (rootSignature) {
        globalRootSignature.pGlobalRootSignature = rootSignature;

        stateSubobjects[stateSubobjectNum].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        stateSubobjects[stateSubobjectNum].pDesc = &globalRootSignature;
        stateSubobjectNum++;
    }

    Scratch<D3D12_DXIL_LIBRARY_DESC> libraryDescs = AllocateScratch(m_Device, D3D12_DXIL_LIBRARY_DESC, rayTracingPipelineDesc.shaderLibrary->shaderNum);
    for (uint32_t i = 0; i < rayTracingPipelineDesc.shaderLibrary->shaderNum; i++) {
        libraryDescs[i].DXILLibrary.pShaderBytecode = rayTracingPipelineDesc.shaderLibrary->shaders[i].bytecode;
        libraryDescs[i].DXILLibrary.BytecodeLength = (size_t)rayTracingPipelineDesc.shaderLibrary->shaders[i].size;
        libraryDescs[i].NumExports = 0;
        libraryDescs[i].pExports = nullptr;

        stateSubobjects[stateSubobjectNum].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        stateSubobjects[stateSubobjectNum].pDesc = &libraryDescs[i];
        stateSubobjectNum++;
    }

    Vector<std::wstring> wEntryPointNames(rayTracingPipelineDesc.shaderLibrary->shaderNum, m_Device.GetStdAllocator());
    for (uint32_t i = 0; i < rayTracingPipelineDesc.shaderLibrary->shaderNum; i++) {
        const ShaderDesc& shader = rayTracingPipelineDesc.shaderLibrary->shaders[i];
        const size_t entryPointNameLength = shader.entryPointName != nullptr ? strlen(shader.entryPointName) + 1 : 0;
        wEntryPointNames[i].resize(entryPointNameLength);
        ConvertCharToWchar(shader.entryPointName, wEntryPointNames[i].data(), entryPointNameLength);
    }

    uint32_t hitGroupNum = 0;
    Scratch<D3D12_HIT_GROUP_DESC> hitGroups = AllocateScratch(m_Device, D3D12_HIT_GROUP_DESC, rayTracingPipelineDesc.shaderGroupNum);
    memset(&hitGroups[0], 0, rayTracingPipelineDesc.shaderGroupNum * sizeof(D3D12_HIT_GROUP_DESC)); // some fields can stay untouched
    m_ShaderGroupNames.reserve(rayTracingPipelineDesc.shaderGroupNum);
    for (uint32_t i = 0; i < rayTracingPipelineDesc.shaderGroupNum; i++) {
        bool isHitGroup = true;
        bool hasIntersectionShader = false;
        std::wstring shaderIndentifierName;
        for (uint32_t j = 0; j < GetCountOf(rayTracingPipelineDesc.shaderGroups[i].shaderIndices); j++) {
            const uint32_t& shaderIndex = rayTracingPipelineDesc.shaderGroups[i].shaderIndices[j];
            if (shaderIndex) {
                uint32_t lookupIndex = shaderIndex - 1;
                const ShaderDesc& shader = rayTracingPipelineDesc.shaderLibrary->shaders[lookupIndex];
                const std::wstring& entryPointName = wEntryPointNames[lookupIndex];
                if (shader.stage == StageBits::RAYGEN_SHADER || shader.stage == StageBits::MISS_SHADER || shader.stage == StageBits::CALLABLE_SHADER) {
                    shaderIndentifierName = entryPointName;
                    isHitGroup = false;
                    break;
                }

                switch (shader.stage) {
                    case StageBits::INTERSECTION_SHADER:
                        hitGroups[hitGroupNum].IntersectionShaderImport = entryPointName.c_str();
                        hasIntersectionShader = true;
                        break;
                    case StageBits::CLOSEST_HIT_SHADER:
                        hitGroups[hitGroupNum].ClosestHitShaderImport = entryPointName.c_str();
                        break;
                    case StageBits::ANY_HIT_SHADER:
                        hitGroups[hitGroupNum].AnyHitShaderImport = entryPointName.c_str();
                        break;
                    default:
                        CHECK(false, "Unexpected");
                        break;
                }

                shaderIndentifierName = std::to_wstring(i);
            }
        }

        m_ShaderGroupNames.push_back(shaderIndentifierName);

        if (isHitGroup) {
            hitGroups[hitGroupNum].HitGroupExport = m_ShaderGroupNames[i].c_str();
            hitGroups[hitGroupNum].Type = hasIntersectionShader ? D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE : D3D12_HIT_GROUP_TYPE_TRIANGLES;

            stateSubobjects[stateSubobjectNum].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            stateSubobjects[stateSubobjectNum].pDesc = &hitGroups[hitGroupNum++];
            stateSubobjectNum++;
        }
    }

    D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
    stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    stateObjectDesc.NumSubobjects = stateSubobjectNum;
    stateObjectDesc.pSubobjects = stateSubobjectNum ? &stateSubobjects[0] : nullptr;

    HRESULT hr = m_Device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&m_StateObject));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device5::CreateStateObject");

    m_StateObject->QueryInterface(&m_StateObjectProperties);

    return Result::SUCCESS;
}

void PipelineD3D12::Bind(ID3D12GraphicsCommandList* graphicsCommandList) const {
    if (m_StateObject)
        ((ID3D12GraphicsCommandList4*)graphicsCommandList)->SetPipelineState1(m_StateObject);
    else
        graphicsCommandList->SetPipelineState(m_PipelineState);

    if (m_PrimitiveTopology != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
        graphicsCommandList->IASetPrimitiveTopology(m_PrimitiveTopology);
}

NRI_INLINE Result PipelineD3D12::WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* dst) const {
    uint8_t* ptr = (uint8_t*)dst;
    size_t identifierSize = (size_t)m_Device.GetDesc().shaderStage.rayTracing.shaderGroupIdentifierSize;
    uint32_t shaderGroupIndex = baseShaderGroupIndex;

    for (uint32_t i = 0; i < shaderGroupNum; i++) {
        const void* identifier = m_StateObjectProperties->GetShaderIdentifier(m_ShaderGroupNames[shaderGroupIndex].c_str());
        memcpy(ptr, identifier, identifierSize);

        ptr += identifierSize;
        shaderGroupIndex++;
    }

    return Result::SUCCESS;
}
