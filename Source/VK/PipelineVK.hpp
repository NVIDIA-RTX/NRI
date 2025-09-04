// © 2021 NVIDIA Corporation

static inline bool IsConstantColorReferenced(BlendFactor factor) {
    return factor == BlendFactor::CONSTANT_COLOR || factor == BlendFactor::CONSTANT_ALPHA || factor == BlendFactor::ONE_MINUS_CONSTANT_COLOR || factor == BlendFactor::ONE_MINUS_CONSTANT_ALPHA;
}

static bool FillPipelineRobustness(const DeviceVK& device, Robustness robustness, VkPipelineRobustnessCreateInfoEXT& robustnessInfo) {
    if (!device.m_IsSupported.pipelineRobustness || robustness == Robustness::DEFAULT)
        return false;

    if (!device.m_IsSupported.robustness2)
        robustness = robustness == Robustness::D3D12 ? Robustness::VK : robustness;
    if (!device.m_IsSupported.robustness)
        robustness = robustness == Robustness::VK ? Robustness::OFF : robustness;

    if (robustness == Robustness::VK) {
        robustnessInfo.images = VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS_EXT;
        robustnessInfo.storageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_EXT;
        robustnessInfo.uniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_EXT;
        robustnessInfo.vertexInputs = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_EXT;
    } else if (robustness == Robustness::D3D12) {
        robustnessInfo.images = VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS_2_EXT;
        robustnessInfo.storageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2_EXT;
        robustnessInfo.uniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2_EXT;
        robustnessInfo.vertexInputs = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2_EXT;
    } else if (robustness == Robustness::OFF) {
        robustnessInfo.images = VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DISABLED_EXT;
        robustnessInfo.storageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED_EXT;
        robustnessInfo.uniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED_EXT;
        robustnessInfo.vertexInputs = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED_EXT;
    }

    return true;
}

PipelineVK::~PipelineVK() {
    if (m_OwnsNativeObjects) {
        const auto& vk = m_Device.GetDispatchTable();
        vk.DestroyPipeline(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }
}

Result PipelineVK::Create(const GraphicsPipelineDesc& graphicsPipelineDesc) {
    m_BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // Shaders
    Scratch<VkPipelineShaderStageCreateInfo> stages = AllocateScratch(m_Device, VkPipelineShaderStageCreateInfo, graphicsPipelineDesc.shaderNum);
    Scratch<VkShaderModule> modules = AllocateScratch(m_Device, VkShaderModule, graphicsPipelineDesc.shaderNum);

    for (uint32_t i = 0; i < graphicsPipelineDesc.shaderNum; i++) {
        const ShaderDesc& shaderDesc = graphicsPipelineDesc.shaders[i];
        Result res = SetupShaderStage(stages[i], shaderDesc, modules[i]);
        if (res != Result::SUCCESS)
            return res;

        stages[i].pName = shaderDesc.entryPointName ? shaderDesc.entryPointName : "main";
    }

    // Vertex input
    const VertexInputDesc* vi = graphicsPipelineDesc.vertexInput;
    uint32_t attributeNum = vi ? vi->attributeNum : 0u;
    uint32_t streamNum = vi ? vi->streamNum : 0u;

    Scratch<VkVertexInputAttributeDescription> vertexAttributeDescs = AllocateScratch(m_Device, VkVertexInputAttributeDescription, attributeNum);
    Scratch<VkVertexInputBindingDescription> vertexBindingDescs = AllocateScratch(m_Device, VkVertexInputBindingDescription, streamNum);

    VkPipelineVertexInputStateCreateInfo vertexInputState = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputState.pVertexAttributeDescriptions = vertexAttributeDescs;
    vertexInputState.pVertexBindingDescriptions = vertexBindingDescs;

    if (vi) {
        vertexInputState.vertexAttributeDescriptionCount = vi->attributeNum;
        vertexInputState.vertexBindingDescriptionCount = vi->streamNum;

        for (uint32_t i = 0; i < vi->attributeNum; i++) {
            const VertexAttributeDesc& attribute = vi->attributes[i];

            VkVertexInputAttributeDescription& vertexAttributeDesc = vertexAttributeDescs[i];
            vertexAttributeDesc = {};
            vertexAttributeDesc.location = attribute.vk.location;
            vertexAttributeDesc.binding = attribute.streamIndex;
            vertexAttributeDesc.format = GetVkFormat(attribute.format);
            vertexAttributeDesc.offset = attribute.offset;
        }

        for (uint32_t i = 0; i < vi->streamNum; i++) {
            const VertexStreamDesc& stream = vi->streams[i];

            VkVertexInputBindingDescription& vertexBindingDesc = vertexBindingDescs[i];
            vertexBindingDesc = {};
            vertexBindingDesc.binding = stream.bindingSlot;
            vertexBindingDesc.inputRate = stream.stepRate == VertexStreamStepRate::PER_VERTEX ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
        }
    }

    // Input assembly
    const InputAssemblyDesc& ia = graphicsPipelineDesc.inputAssembly;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssemblyState.topology = GetTopology(ia.topology);
    inputAssemblyState.primitiveRestartEnable = ia.primitiveRestart != PrimitiveRestart::DISABLED;

    VkPipelineTessellationStateCreateInfo tessellationState = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};
    tessellationState.patchControlPoints = ia.tessControlPointNum;

    // Multisample
    const MultisampleDesc* ms = graphicsPipelineDesc.multisample;

    VkPipelineSampleLocationsStateCreateInfoEXT sampleLocationsState = {VK_STRUCTURE_TYPE_PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT};
    sampleLocationsState.sampleLocationsInfo.sType = VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT;

    VkPipelineMultisampleStateCreateInfo multisampleState = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampleState.rasterizationSamples = ms ? (VkSampleCountFlagBits)ms->sampleNum : VK_SAMPLE_COUNT_1_BIT;

    if (graphicsPipelineDesc.multisample) {
        multisampleState.sampleShadingEnable = false;
        multisampleState.minSampleShading = 0.0f;
        multisampleState.pSampleMask = ms->sampleMask != ALL ? &ms->sampleMask : nullptr;
        multisampleState.alphaToCoverageEnable = ms->alphaToCoverage;
        multisampleState.alphaToOneEnable = false;

        const void** tail = &multisampleState.pNext;
        if (ms->sampleLocations) {
            sampleLocationsState.sampleLocationsEnable = VK_TRUE;

            APPEND_STRUCT(sampleLocationsState);
        }
    }

    // Rasterization
    const RasterizationDesc& r = graphicsPipelineDesc.rasterization;

    VkPipelineRasterizationStateCreateInfo rasterizationState = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizationState.depthClampEnable = r.depthClamp;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE; // TODO: D3D doesn't have this
    rasterizationState.polygonMode = GetPolygonMode(r.fillMode);
    rasterizationState.cullMode = GetCullMode(r.cullMode);
    rasterizationState.frontFace = r.frontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    rasterizationState.depthBiasEnable = IsDepthBiasEnabled(r.depthBias) ? VK_TRUE : VK_FALSE;
    rasterizationState.depthBiasConstantFactor = r.depthBias.constant;
    rasterizationState.depthBiasClamp = r.depthBias.clamp;
    rasterizationState.depthBiasSlopeFactor = r.depthBias.slope;
    rasterizationState.lineWidth = 1.0f;

    const void** tail = &rasterizationState.pNext;
    VkPipelineRasterizationConservativeStateCreateInfoEXT consetvativeRasterizationState = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT};
    if (r.conservativeRaster) {
        consetvativeRasterizationState.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
        consetvativeRasterizationState.extraPrimitiveOverestimationSize = 0.0f;

        APPEND_STRUCT(consetvativeRasterizationState);
    }

    VkPipelineRasterizationLineStateCreateInfoKHR lineState = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_KHR};
    if (r.lineSmoothing) {
        lineState.lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_KHR;
        APPEND_STRUCT(lineState);
    }

    m_DepthBias = r.depthBias;

    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};

    // Depth-stencil
    const DepthAttachmentDesc& da = graphicsPipelineDesc.outputMerger.depth;
    const StencilAttachmentDesc& sa = graphicsPipelineDesc.outputMerger.stencil;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencilState.depthTestEnable = da.compareOp != CompareOp::NONE;
    depthStencilState.depthWriteEnable = da.write;
    depthStencilState.depthCompareOp = GetCompareOp(da.compareOp);
    depthStencilState.depthBoundsTestEnable = da.boundsTest;
    depthStencilState.stencilTestEnable = (sa.front.compareOp == CompareOp::NONE && sa.back.compareOp == CompareOp::NONE) ? VK_FALSE : VK_TRUE;
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;

    depthStencilState.front.failOp = GetStencilOp(sa.front.failOp);
    depthStencilState.front.passOp = GetStencilOp(sa.front.passOp);
    depthStencilState.front.depthFailOp = GetStencilOp(sa.front.depthFailOp);
    depthStencilState.front.compareOp = GetCompareOp(sa.front.compareOp);
    depthStencilState.front.compareMask = sa.front.compareMask;
    depthStencilState.front.writeMask = sa.front.writeMask;

    depthStencilState.back.failOp = GetStencilOp(sa.back.failOp);
    depthStencilState.back.passOp = GetStencilOp(sa.back.passOp);
    depthStencilState.back.depthFailOp = GetStencilOp(sa.back.depthFailOp);
    depthStencilState.back.compareOp = GetCompareOp(sa.back.compareOp);
    depthStencilState.back.compareMask = sa.back.compareMask;
    depthStencilState.back.writeMask = sa.back.writeMask;

    // Blending
    const OutputMergerDesc& om = graphicsPipelineDesc.outputMerger;
    Scratch<VkPipelineColorBlendAttachmentState> scratch = AllocateScratch(m_Device, VkPipelineColorBlendAttachmentState, om.colorNum);

    VkPipelineColorBlendStateCreateInfo colorBlendState = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlendState.logicOpEnable = om.logicOp != LogicOp::NONE ? VK_TRUE : VK_FALSE;
    colorBlendState.logicOp = GetLogicOp(om.logicOp);
    colorBlendState.attachmentCount = om.colorNum;
    colorBlendState.pAttachments = scratch;

    bool isConstantColorReferenced = false;
    VkPipelineColorBlendAttachmentState* attachments = const_cast<VkPipelineColorBlendAttachmentState*>(colorBlendState.pAttachments);
    for (uint32_t i = 0; i < om.colorNum; i++) {
        const ColorAttachmentDesc& attachmentDesc = om.colors[i];

        attachments[i] = {
            VkBool32(attachmentDesc.blendEnabled),
            GetBlendFactor(attachmentDesc.colorBlend.srcFactor),
            GetBlendFactor(attachmentDesc.colorBlend.dstFactor),
            GetBlendOp(attachmentDesc.colorBlend.op),
            GetBlendFactor(attachmentDesc.alphaBlend.srcFactor),
            GetBlendFactor(attachmentDesc.alphaBlend.dstFactor),
            GetBlendOp(attachmentDesc.alphaBlend.op),
            GetColorComponent(attachmentDesc.colorWriteMask),
        };

        if (IsConstantColorReferenced(attachmentDesc.colorBlend.srcFactor) || IsConstantColorReferenced(attachmentDesc.colorBlend.dstFactor) || IsConstantColorReferenced(attachmentDesc.alphaBlend.srcFactor) || IsConstantColorReferenced(attachmentDesc.alphaBlend.dstFactor))
            isConstantColorReferenced = true;
    }

    // Formats
    const FormatProps& depthStencilFormatProps = GetFormatProps(om.depthStencilFormat);

    Scratch<VkFormat> colorFormats = AllocateScratch(m_Device, VkFormat, om.colorNum);
    for (uint32_t i = 0; i < om.colorNum; i++)
        colorFormats[i] = GetVkFormat(om.colors[i].format);

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    pipelineRenderingCreateInfo.viewMask = om.viewMask;
    pipelineRenderingCreateInfo.colorAttachmentCount = om.colorNum;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats;
    pipelineRenderingCreateInfo.depthAttachmentFormat = GetVkFormat(om.depthStencilFormat);
    pipelineRenderingCreateInfo.stencilAttachmentFormat = depthStencilFormatProps.isStencil ? GetVkFormat(om.depthStencilFormat) : VK_FORMAT_UNDEFINED;

    // Dynamic state
    uint32_t dynamicStateNum = 0;
    std::array<VkDynamicState, 16> dynamicStates;
    dynamicStates[dynamicStateNum++] = VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT;
    dynamicStates[dynamicStateNum++] = VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT;
    if (vi)
        dynamicStates[dynamicStateNum++] = VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE;
    if (rasterizationState.depthBiasEnable)
        dynamicStates[dynamicStateNum++] = VK_DYNAMIC_STATE_DEPTH_BIAS;
    if (depthStencilState.depthBoundsTestEnable)
        dynamicStates[dynamicStateNum++] = VK_DYNAMIC_STATE_DEPTH_BOUNDS;
    if (depthStencilState.stencilTestEnable)
        dynamicStates[dynamicStateNum++] = VK_DYNAMIC_STATE_STENCIL_REFERENCE;
    if (sampleLocationsState.sampleLocationsEnable)
        dynamicStates[dynamicStateNum++] = VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT;
    if (isConstantColorReferenced)
        dynamicStates[dynamicStateNum++] = VK_DYNAMIC_STATE_BLEND_CONSTANTS;
    if (r.shadingRate)
        dynamicStates[dynamicStateNum++] = VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR;

    VkPipelineDynamicStateCreateInfo dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicState.dynamicStateCount = dynamicStateNum;
    dynamicState.pDynamicStates = dynamicStates.data();

    // Create
    VkPipelineCreateFlags flags = 0;
    if (r.shadingRate)
        flags |= VK_PIPELINE_CREATE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;

    const PipelineLayoutVK& pipelineLayoutVK = *(const PipelineLayoutVK*)graphicsPipelineDesc.pipelineLayout;

    VkGraphicsPipelineCreateInfo info = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        &pipelineRenderingCreateInfo,
        flags,
        graphicsPipelineDesc.shaderNum,
        stages,
        &vertexInputState,
        &inputAssemblyState,
        &tessellationState,
        &viewportState,
        &rasterizationState,
        &multisampleState,
        &depthStencilState,
        &colorBlendState,
        &dynamicState,
        pipelineLayoutVK,
        VK_NULL_HANDLE,
        0,
        VK_NULL_HANDLE,
        -1,
    };

    VkPipelineRobustnessCreateInfoEXT robustnessInfo = {VK_STRUCTURE_TYPE_PIPELINE_ROBUSTNESS_CREATE_INFO_EXT};
    if (FillPipelineRobustness(m_Device, graphicsPipelineDesc.robustness, robustnessInfo))
        pipelineRenderingCreateInfo.pNext = &robustnessInfo;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &info, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateGraphicsPipelines");

    for (size_t i = 0; i < graphicsPipelineDesc.shaderNum; i++)
        vk.DestroyShaderModule(m_Device, modules[i], m_Device.GetVkAllocationCallbacks());

    return Result::SUCCESS;
}

Result PipelineVK::Create(const ComputePipelineDesc& computePipelineDesc) {
    m_BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

    const PipelineLayoutVK& pipelineLayoutVK = *(const PipelineLayoutVK*)computePipelineDesc.pipelineLayout;

    const VkShaderModuleCreateInfo moduleInfo = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        nullptr,
        (VkShaderModuleCreateFlags)0,
        (size_t)computePipelineDesc.shader.size,
        (const uint32_t*)computePipelineDesc.shader.bytecode,
    };

    VkShaderModule module = VK_NULL_HANDLE;
    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateShaderModule(m_Device, &moduleInfo, m_Device.GetVkAllocationCallbacks(), &module);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateShaderModule");

    VkPipelineShaderStageCreateInfo stage = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr,
        (VkPipelineShaderStageCreateFlags)0,
        VK_SHADER_STAGE_COMPUTE_BIT,
        module,
        computePipelineDesc.shader.entryPointName ? computePipelineDesc.shader.entryPointName : "main",
        nullptr,
    };

    VkComputePipelineCreateInfo info = {
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        nullptr,
        (VkPipelineCreateFlags)0,
        stage,
        pipelineLayoutVK,
        VK_NULL_HANDLE,
        -1,
    };

    VkPipelineRobustnessCreateInfoEXT robustnessInfo = {VK_STRUCTURE_TYPE_PIPELINE_ROBUSTNESS_CREATE_INFO_EXT};
    if (FillPipelineRobustness(m_Device, computePipelineDesc.robustness, robustnessInfo))
        info.pNext = &robustnessInfo;

    vkResult = vk.CreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &info, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateComputePipelines");

    vk.DestroyShaderModule(m_Device, module, m_Device.GetVkAllocationCallbacks());

    return Result::SUCCESS;
}

Result PipelineVK::Create(const RayTracingPipelineDesc& rayTracingPipelineDesc) {
    m_BindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;

    const PipelineLayoutVK& pipelineLayoutVK = *(const PipelineLayoutVK*)rayTracingPipelineDesc.pipelineLayout;

    const uint32_t stageNum = rayTracingPipelineDesc.shaderLibrary->shaderNum;
    Scratch<VkPipelineShaderStageCreateInfo> stages = AllocateScratch(m_Device, VkPipelineShaderStageCreateInfo, stageNum);
    Scratch<VkShaderModule> modules = AllocateScratch(m_Device, VkShaderModule, stageNum);

    for (uint32_t i = 0; i < stageNum; i++) {
        const ShaderDesc& shaderDesc = rayTracingPipelineDesc.shaderLibrary->shaders[i];
        Result result = SetupShaderStage(stages[i], shaderDesc, modules[i]);
        if (result != Result::SUCCESS)
            return result;

        stages[i].pName = shaderDesc.entryPointName ? shaderDesc.entryPointName : "main";
    }

    Scratch<VkRayTracingShaderGroupCreateInfoKHR> groupArray = AllocateScratch(m_Device, VkRayTracingShaderGroupCreateInfoKHR, rayTracingPipelineDesc.shaderGroupNum);
    for (uint32_t i = 0; i < rayTracingPipelineDesc.shaderGroupNum; i++) {
        const ShaderGroupDesc& srcGroup = rayTracingPipelineDesc.shaderGroups[i];

        VkRayTracingShaderGroupCreateInfoKHR& dstGroup = groupArray[i];
        dstGroup = {VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
        dstGroup.generalShader = VK_SHADER_UNUSED_KHR;
        dstGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
        dstGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
        dstGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

        for (uint32_t j = 0; j < GetCountOf(srcGroup.shaderIndices); j++) {
            if (srcGroup.shaderIndices[j]) {
                const uint32_t index = srcGroup.shaderIndices[j] - 1;
                const VkPipelineShaderStageCreateInfo& stageInfo = stages[index];

                switch (stageInfo.stage) {
                    case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
                    case VK_SHADER_STAGE_MISS_BIT_KHR:
                    case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
                        dstGroup.generalShader = index;
                        break;

                    case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
                        dstGroup.anyHitShader = index;
                        break;

                    case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
                        dstGroup.closestHitShader = index;
                        break;

                    case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
                        dstGroup.intersectionShader = index;
                        break;

                    default:
                        // already initialized
                        break;
                }
            }
        }

        if (dstGroup.intersectionShader != VK_SHADER_UNUSED_KHR)
            dstGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
        else if (dstGroup.generalShader != VK_SHADER_UNUSED_KHR)
            dstGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        else
            dstGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    }

    VkRayTracingPipelineInterfaceCreateInfoKHR interfaceCreateInfo = {VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR};
    interfaceCreateInfo.maxPipelineRayPayloadSize = rayTracingPipelineDesc.rayPayloadMaxSize;
    interfaceCreateInfo.maxPipelineRayHitAttributeSize = rayTracingPipelineDesc.rayHitAttributeMaxSize;

    VkRayTracingPipelineCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
    createInfo.stageCount = stageNum;
    createInfo.pStages = stages;
    createInfo.groupCount = rayTracingPipelineDesc.shaderGroupNum;
    createInfo.pGroups = groupArray;
    createInfo.maxPipelineRayRecursionDepth = rayTracingPipelineDesc.recursionMaxDepth;
    createInfo.pLibraryInterface = &interfaceCreateInfo;
    createInfo.layout = pipelineLayoutVK;
    createInfo.basePipelineIndex = -1;

    if (rayTracingPipelineDesc.flags & RayTracingPipelineBits::SKIP_TRIANGLES)
        createInfo.flags |= VK_PIPELINE_CREATE_RAY_TRACING_SKIP_TRIANGLES_BIT_KHR;
    if (rayTracingPipelineDesc.flags & RayTracingPipelineBits::SKIP_AABBS)
        createInfo.flags |= VK_PIPELINE_CREATE_RAY_TRACING_SKIP_AABBS_BIT_KHR;
    if (rayTracingPipelineDesc.flags & RayTracingPipelineBits::ALLOW_MICROMAPS)
        createInfo.flags |= VK_PIPELINE_CREATE_RAY_TRACING_OPACITY_MICROMAP_BIT_EXT;

    VkPipelineRobustnessCreateInfoEXT robustnessInfo = {VK_STRUCTURE_TYPE_PIPELINE_ROBUSTNESS_CREATE_INFO_EXT};
    if (FillPipelineRobustness(m_Device, rayTracingPipelineDesc.robustness, robustnessInfo))
        createInfo.pNext = &robustnessInfo;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateRayTracingPipelinesKHR(m_Device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateRayTracingPipelinesKHR");

    for (size_t i = 0; i < stageNum; i++)
        vk.DestroyShaderModule(m_Device, modules[i], m_Device.GetVkAllocationCallbacks());

    return Result::SUCCESS;
}

Result PipelineVK::Create(const PipelineVKDesc& pipelineVKDesc) {
    m_OwnsNativeObjects = false;
    m_Handle = (VkPipeline)pipelineVKDesc.vkPipeline;
    m_BindPoint = (VkPipelineBindPoint)pipelineVKDesc.vkPipelineBindPoint;

    return Result::SUCCESS;
}

Result PipelineVK::SetupShaderStage(VkPipelineShaderStageCreateInfo& stage, const ShaderDesc& shaderDesc, VkShaderModule& module) {
    const VkShaderModuleCreateInfo moduleInfo = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        nullptr,
        (VkShaderModuleCreateFlags)0,
        (size_t)shaderDesc.size,
        (const uint32_t*)shaderDesc.bytecode,
    };

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.CreateShaderModule(m_Device, &moduleInfo, m_Device.GetVkAllocationCallbacks(), &module);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateShaderModule");

    stage = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr,
        (VkPipelineShaderStageCreateFlags)0,
        (VkShaderStageFlagBits)GetShaderStageFlags(shaderDesc.stage),
        module,
        nullptr,
        nullptr,
    };

    return Result::SUCCESS;
}

NRI_INLINE void PipelineVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_Handle, name);
}

NRI_INLINE Result PipelineVK::WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* dst) const {
    const size_t dataSize = (size_t)shaderGroupNum * m_Device.GetDesc().shaderStage.rayTracing.shaderGroupIdentifierSize;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult vkResult = vk.GetRayTracingShaderGroupHandlesKHR(m_Device, m_Handle, baseShaderGroupIndex, shaderGroupNum, dataSize, dst);
    RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetRayTracingShaderGroupHandlesKHR");

    return Result::SUCCESS;
}
