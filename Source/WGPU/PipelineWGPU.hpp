// © 2026 NVIDIA Corporation

PipelineWGPU::~PipelineWGPU() {
    if (m_RenderPipeline)
        wgpuRenderPipelineRelease(m_RenderPipeline);
    if (m_ComputePipeline)
        wgpuComputePipelineRelease(m_ComputePipeline);
}

static bool IsSpirvBytecodeWGPU(const ShaderDesc& shaderDesc) {
    constexpr uint32_t SPIRV_MAGIC = 0x07230203;

    const uint32_t* spirv = (const uint32_t*)shaderDesc.bytecode;
    return spirv && shaderDesc.size >= sizeof(uint32_t) && spirv[0] == SPIRV_MAGIC;
}

static bool AddNonReadableDecorationsForWriteOnlyStorageImagesWGPU(DeviceWGPU& device, const ShaderDesc& shaderDesc, Vector<uint32_t>& patchedSpirv) {
    // TODO: This patches SPIR-V to satisfy WGPU write-only storage texture rules. Prefer generating correct decorations in shaders.
    constexpr uint16_t OP_TYPE_IMAGE = 25;
    constexpr uint16_t OP_TYPE_POINTER = 32;
    constexpr uint16_t OP_VARIABLE = 59;
    constexpr uint16_t OP_DECORATE = 71;
    constexpr uint16_t OP_MEMBER_DECORATE = 72;
    constexpr uint16_t OP_IMAGE_READ = 98;
    constexpr uint16_t OP_DECORATE_ID = 332;
    constexpr uint32_t DIM_BUFFER = 5;
    constexpr uint32_t STORAGE_CLASS_UNIFORM_CONSTANT = 0;
    constexpr uint32_t DECORATION_NON_READABLE = 25;

    uint32_t wordNum = (uint32_t)(shaderDesc.size / sizeof(uint32_t));
    const uint32_t* spirv = (const uint32_t*)shaderDesc.bytecode;
    if (!IsSpirvBytecodeWGPU(shaderDesc) || wordNum < 5)
        return false;

    uint32_t idBound = spirv[3];
    Vector<uint8_t> storageImageTypes(device.GetStdAllocator());
    Vector<uint8_t> nonReadableVariables(device.GetStdAllocator());
    Vector<uint32_t> pointerPointeeTypes(device.GetStdAllocator());
    Vector<uint32_t> storageImageVariables(device.GetStdAllocator());
    storageImageTypes.resize(idBound);
    nonReadableVariables.resize(idBound);
    pointerPointeeTypes.resize(idBound);

    bool hasImageRead = false;
    uint32_t insertionWord = 5;

    for (uint32_t word = 5; word < wordNum;) {
        uint32_t instruction = spirv[word];
        uint16_t op = (uint16_t)(instruction & 0xFFFF);
        uint16_t wordCount = (uint16_t)(instruction >> 16);
        if (!wordCount || word + wordCount > wordNum)
            return false;

        if (op == OP_DECORATE || op == OP_MEMBER_DECORATE || op == OP_DECORATE_ID)
            insertionWord = word + wordCount;

        if (op == OP_TYPE_IMAGE && wordCount >= 9) {
            uint32_t resultId = spirv[word + 1];
            uint32_t dim = spirv[word + 3];
            uint32_t sampled = spirv[word + 7];
            if (resultId < idBound && sampled == 2 && dim != DIM_BUFFER)
                storageImageTypes[resultId] = 1;
        } else if (op == OP_TYPE_POINTER && wordCount >= 4) {
            uint32_t resultId = spirv[word + 1];
            if (resultId < idBound)
                pointerPointeeTypes[resultId] = spirv[word + 3];
        } else if (op == OP_VARIABLE && wordCount >= 4) {
            uint32_t resultTypeId = spirv[word + 1];
            uint32_t resultId = spirv[word + 2];
            uint32_t storageClass = spirv[word + 3];
            if (resultTypeId < idBound && resultId < idBound && storageClass == STORAGE_CLASS_UNIFORM_CONSTANT) {
                uint32_t pointeeTypeId = pointerPointeeTypes[resultTypeId];
                if (pointeeTypeId < idBound && storageImageTypes[pointeeTypeId])
                    storageImageVariables.push_back(resultId);
            }
        } else if (op == OP_DECORATE && wordCount >= 3) {
            uint32_t targetId = spirv[word + 1];
            uint32_t decoration = spirv[word + 2];
            if (targetId < idBound && decoration == DECORATION_NON_READABLE)
                nonReadableVariables[targetId] = 1;
        } else if (op == OP_IMAGE_READ)
            hasImageRead = true;

        word += wordCount;
    }

    if (hasImageRead)
        return false;

    uint32_t decorationNum = 0;
    for (uint32_t variable : storageImageVariables) {
        if (variable < idBound && !nonReadableVariables[variable])
            decorationNum++;
    }

    if (!decorationNum)
        return false;

    patchedSpirv.clear();
    patchedSpirv.reserve(wordNum + decorationNum * 3);
    patchedSpirv.insert(patchedSpirv.end(), spirv, spirv + insertionWord);

    for (uint32_t variable : storageImageVariables) {
        if (variable < idBound && !nonReadableVariables[variable]) {
            patchedSpirv.push_back((3u << 16) | OP_DECORATE);
            patchedSpirv.push_back(variable);
            patchedSpirv.push_back(DECORATION_NON_READABLE);
        }
    }

    patchedSpirv.insert(patchedSpirv.end(), spirv + insertionWord, spirv + wordNum);

    return true;
}

WGPUShaderModule PipelineWGPU::CreateShaderModule(const ShaderDesc& shaderDesc) {
    WGPUShaderModuleDescriptor desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;

    // TODO: Shader bytecode type is inferred from the SPIR-V magic header; everything else is treated as WGSL text.
    if (!IsSpirvBytecodeWGPU(shaderDesc)) {
        WGPUShaderSourceWGSL wgsl = WGPU_SHADER_SOURCE_WGSL_INIT;
        wgsl.code = {(const char*)shaderDesc.bytecode, (size_t)shaderDesc.size};
        desc.nextInChain = &wgsl.chain;

        return wgpuDeviceCreateShaderModule(m_Device, &desc);
    }

    Vector<uint32_t> patchedSpirv(m_Device.GetStdAllocator());
    bool hasPatchedSpirv = AddNonReadableDecorationsForWriteOnlyStorageImagesWGPU(m_Device, shaderDesc, patchedSpirv);

    WGPUShaderSourceSPIRV spirv = WGPU_SHADER_SOURCE_SPIRV_INIT;
    spirv.codeSize = hasPatchedSpirv ? (uint32_t)patchedSpirv.size() : (uint32_t)(shaderDesc.size / sizeof(uint32_t));
    spirv.code = hasPatchedSpirv ? patchedSpirv.data() : (const uint32_t*)shaderDesc.bytecode;

    desc.nextInChain = &spirv.chain;

    return wgpuDeviceCreateShaderModule(m_Device, &desc);
}

static uint64_t GetVertexStreamStride(const VertexInputDesc& vertexInput, uint32_t streamIndex) {
    // TODO: Compatibility fallback for older samples. Prefer explicit "VertexStreamDesc::stride" in sample code.
    uint64_t stride = 0;

    for (uint32_t i = 0; i < vertexInput.attributeNum; i++) {
        const VertexAttributeDesc& attribute = vertexInput.attributes[i];
        if (attribute.streamIndex != streamIndex)
            continue;

        stride = std::max(stride, (uint64_t)attribute.offset + GetFormatProps(attribute.format).stride);
    }

    return stride;
}

static void FillStencilFace(WGPUStencilFaceState& out, const StencilDesc& in) {
    out.compare = in.compareOp == CompareOp::NONE ? WGPUCompareFunction_Always : GetCompareFunction(in.compareOp);
    out.failOp = GetStencilOperation(in.failOp);
    out.depthFailOp = GetStencilOperation(in.depthFailOp);
    out.passOp = GetStencilOperation(in.passOp);
}

Result PipelineWGPU::Create(const GraphicsPipelineDesc& graphicsPipelineDesc) {
    m_PipelineLayout = (PipelineLayoutWGPU*)graphicsPipelineDesc.pipelineLayout;
    Result result = m_PipelineLayout->UpdateStorageTextureFormats(graphicsPipelineDesc.shaders, graphicsPipelineDesc.shaderNum);
    if (result != Result::SUCCESS)
        return result;

    WGPUShaderModule vertexShader = nullptr;
    WGPUShaderModule fragmentShader = nullptr;

    for (uint32_t i = 0; i < graphicsPipelineDesc.shaderNum; i++) {
        const ShaderDesc& shaderDesc = graphicsPipelineDesc.shaders[i];
        if (shaderDesc.stage == StageBits::VERTEX_SHADER)
            vertexShader = CreateShaderModule(shaderDesc);
        else if (shaderDesc.stage == StageBits::FRAGMENT_SHADER)
            fragmentShader = CreateShaderModule(shaderDesc);
    }

    // TODO: Depth-only graphics pipelines are not handled yet because a fragment shader is currently required.
    if (!vertexShader || !fragmentShader)
        return Result::FAILURE;

    const VertexInputDesc* vertexInput = graphicsPipelineDesc.vertexInput;
    Scratch<WGPUVertexAttribute> attributes = NRI_ALLOCATE_SCRATCH(m_Device, WGPUVertexAttribute, vertexInput ? (size_t)vertexInput->attributeNum : 0ull);
    Scratch<WGPUVertexBufferLayout> streams = NRI_ALLOCATE_SCRATCH(m_Device, WGPUVertexBufferLayout, vertexInput ? (size_t)vertexInput->streamNum : 0ull);

    if (vertexInput) {
        uint32_t attributeOffset = 0;
        for (uint32_t i = 0; i < vertexInput->streamNum; i++) {
            const VertexStreamDesc& in = vertexInput->streams[i];
            WGPUVertexBufferLayout& out = streams[i];
            out = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
            out.arrayStride = in.stride ? in.stride : GetVertexStreamStride(*vertexInput, i);
            out.stepMode = in.stepRate == VertexStreamStepRate::PER_INSTANCE ? WGPUVertexStepMode_Instance : WGPUVertexStepMode_Vertex;
            out.attributes = attributes + attributeOffset;

            for (uint32_t j = 0; j < vertexInput->attributeNum; j++) {
                const VertexAttributeDesc& attributeDesc = vertexInput->attributes[j];
                if (attributeDesc.streamIndex != i)
                    continue;

                WGPUVertexAttribute& attribute = attributes[attributeOffset + out.attributeCount++];
                attribute = WGPU_VERTEX_ATTRIBUTE_INIT;
                attribute.format = GetVertexFormat(attributeDesc.format);
                attribute.offset = attributeDesc.offset;
                attribute.shaderLocation = attributeDesc.vk.location;
            }

            attributeOffset += (uint32_t)out.attributeCount;
        }
    }

    Scratch<WGPUBlendState> blends = NRI_ALLOCATE_SCRATCH(m_Device, WGPUBlendState, graphicsPipelineDesc.outputMerger.colorNum);
    Scratch<WGPUColorTargetState> colorTargets = NRI_ALLOCATE_SCRATCH(m_Device, WGPUColorTargetState, graphicsPipelineDesc.outputMerger.colorNum);

    for (uint32_t i = 0; i < graphicsPipelineDesc.outputMerger.colorNum; i++) {
        const ColorAttachmentDesc& in = graphicsPipelineDesc.outputMerger.colors[i];
        WGPUColorTargetState& out = colorTargets[i];
        out = WGPU_COLOR_TARGET_STATE_INIT;
        out.format = GetTextureFormat(in.format);
        out.writeMask = GetColorWriteMask(in.colorWriteMask);

        if (in.blendEnabled) {
            WGPUBlendState& blend = blends[i];
            blend = WGPU_BLEND_STATE_INIT;
            blend.color.srcFactor = GetBlendFactor(in.colorBlend.srcFactor);
            blend.color.dstFactor = GetBlendFactor(in.colorBlend.dstFactor);
            blend.color.operation = GetBlendOperation(in.colorBlend.op);
            blend.alpha.srcFactor = GetBlendFactor(in.alphaBlend.srcFactor);
            blend.alpha.dstFactor = GetBlendFactor(in.alphaBlend.dstFactor);
            blend.alpha.operation = GetBlendOperation(in.alphaBlend.op);
            out.blend = &blend;
        }
    }

    WGPUFragmentState fragment = WGPU_FRAGMENT_STATE_INIT;
    fragment.module = fragmentShader;
    // TODO: ShaderDesc::entryPointName is ignored for now. WGPU pipelines currently expect "main".
    fragment.entryPoint = WGPUString("main");
    fragment.targets = colorTargets;
    fragment.targetCount = graphicsPipelineDesc.outputMerger.colorNum;

    WGPUMultisampleState multisample = WGPU_MULTISAMPLE_STATE_INIT;
    multisample.count = graphicsPipelineDesc.multisample ? GetCountOrOne((uint32_t)graphicsPipelineDesc.multisample->sampleNum) : 1;
    multisample.mask = (graphicsPipelineDesc.multisample && graphicsPipelineDesc.multisample->sampleMask != ALL) ? graphicsPipelineDesc.multisample->sampleMask : 0xFFFFFFFF;
    multisample.alphaToCoverageEnabled = (graphicsPipelineDesc.multisample && graphicsPipelineDesc.multisample->alphaToCoverage) ? WGPU_TRUE : WGPU_FALSE;

    WGPUPrimitiveState primitive = WGPU_PRIMITIVE_STATE_INIT;
    primitive.topology = GetPrimitiveTopology(graphicsPipelineDesc.inputAssembly.topology);
    primitive.frontFace = GetFrontFace(graphicsPipelineDesc.rasterization.frontCounterClockwise);
    primitive.cullMode = GetCullMode(graphicsPipelineDesc.rasterization.cullMode);

    WGPUDepthStencilState depthStencil = WGPU_DEPTH_STENCIL_STATE_INIT;
    if (graphicsPipelineDesc.outputMerger.depthStencilFormat != Format::UNKNOWN) {
        const OutputMergerDesc& outputMerger = graphicsPipelineDesc.outputMerger;
        const RasterizationDesc& rasterization = graphicsPipelineDesc.rasterization;
        depthStencil.format = GetTextureFormat(outputMerger.depthStencilFormat);
        depthStencil.depthWriteEnabled = outputMerger.depth.write ? WGPUOptionalBool_True : WGPUOptionalBool_False;
        depthStencil.depthCompare = outputMerger.depth.compareOp == CompareOp::NONE ? WGPUCompareFunction_Always : GetCompareFunction(outputMerger.depth.compareOp);
        depthStencil.stencilReadMask = outputMerger.stencil.front.compareMask;
        depthStencil.stencilWriteMask = outputMerger.stencil.front.writeMask;
        depthStencil.depthBias = (int32_t)rasterization.depthBias.constant;
        depthStencil.depthBiasClamp = rasterization.depthBias.clamp;
        depthStencil.depthBiasSlopeScale = rasterization.depthBias.slope;
        FillStencilFace(depthStencil.stencilFront, outputMerger.stencil.front);
        FillStencilFace(depthStencil.stencilBack, outputMerger.stencil.back);
    }

    WGPUVertexState vertex = WGPU_VERTEX_STATE_INIT;
    vertex.module = vertexShader;
    // TODO: ShaderDesc::entryPointName is ignored for now. WGPU pipelines currently expect "main".
    vertex.entryPoint = WGPUString("main");
    vertex.buffers = streams;
    vertex.bufferCount = vertexInput ? vertexInput->streamNum : 0;

    WGPURenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    desc.layout = *m_PipelineLayout;
    desc.vertex = vertex;
    desc.primitive = primitive;
    desc.multisample = multisample;
    desc.fragment = &fragment;
    desc.depthStencil = graphicsPipelineDesc.outputMerger.depthStencilFormat != Format::UNKNOWN ? &depthStencil : nullptr;

    m_RenderPipeline = wgpuDeviceCreateRenderPipeline(m_Device, &desc);

    wgpuShaderModuleRelease(vertexShader);
    wgpuShaderModuleRelease(fragmentShader);

    return m_RenderPipeline ? Result::SUCCESS : Result::FAILURE;
}

Result PipelineWGPU::Create(const ComputePipelineDesc& computePipelineDesc) {
    m_PipelineLayout = (PipelineLayoutWGPU*)computePipelineDesc.pipelineLayout;
    Result result = m_PipelineLayout->UpdateStorageTextureFormats(&computePipelineDesc.shader, 1);
    if (result != Result::SUCCESS)
        return result;

    WGPUShaderModule shader = CreateShaderModule(computePipelineDesc.shader);
    if (!shader)
        return Result::FAILURE;

    WGPUComputePipelineDescriptor desc = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
    desc.layout = *m_PipelineLayout;
    desc.compute.module = shader;
    // TODO: ShaderDesc::entryPointName is ignored for now. WGPU pipelines currently expect "main".
    desc.compute.entryPoint = WGPUString("main");

    m_ComputePipeline = wgpuDeviceCreateComputePipeline(m_Device, &desc);
    wgpuShaderModuleRelease(shader);

    return m_ComputePipeline ? Result::SUCCESS : Result::FAILURE;
}
