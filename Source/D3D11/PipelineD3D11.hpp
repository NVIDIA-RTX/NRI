// © 2021 NVIDIA Corporation

Result PipelineD3D11::Create(const GraphicsPipelineDesc& pipelineDesc) {
    const ShaderDesc* vertexShader = nullptr;
    HRESULT hr;

    m_PipelineLayout = (PipelineLayoutD3D11*)pipelineDesc.pipelineLayout;

    { // Shaders
        for (uint32_t i = 0; i < pipelineDesc.shaderNum; i++) {
            const ShaderDesc* shaderDesc = pipelineDesc.shaders + i;

            if (shaderDesc->stage == StageBits::VERTEX_SHADER) {
                vertexShader = shaderDesc;
                hr = m_Device->CreateVertexShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_VertexShader);
                RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateVertexShader");
            } else if (shaderDesc->stage == StageBits::TESS_CONTROL_SHADER) {
                hr = m_Device->CreateHullShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_TessControlShader);
                RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateHullShader");
            } else if (shaderDesc->stage == StageBits::TESS_EVALUATION_SHADER) {
                hr = m_Device->CreateDomainShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_TessEvaluationShader);
                RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateDomainShader");
            } else if (shaderDesc->stage == StageBits::GEOMETRY_SHADER) {
                hr = m_Device->CreateGeometryShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_GeometryShader);
                RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateGeometryShader");
            } else if (shaderDesc->stage == StageBits::FRAGMENT_SHADER) {
                hr = m_Device->CreatePixelShader(shaderDesc->bytecode, (size_t)shaderDesc->size, nullptr, &m_FragmentShader);
                RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreatePixelShader");
            } else
                return Result::INVALID_ARGUMENT;
        }
    }

    { // Input assembly
        const InputAssemblyDesc& ia = pipelineDesc.inputAssembly;

        m_Topology = GetD3D11TopologyFromTopology(ia.topology, ia.tessControlPointNum);
    }

    // Vertex input
    if (pipelineDesc.vertexInput) {
        const VertexInputDesc& vi = *pipelineDesc.vertexInput;

        Scratch<D3D11_INPUT_ELEMENT_DESC> inputElements = AllocateScratch(m_Device, D3D11_INPUT_ELEMENT_DESC, vi.attributeNum);
        for (uint32_t i = 0; i < vi.attributeNum; i++) {
            const VertexAttributeDesc& attribute = vi.attributes[i];
            const VertexStreamDesc& stream = vi.streams[attribute.streamIndex];
            D3D11_INPUT_ELEMENT_DESC& inputElementDesc = inputElements[i];
            const DxgiFormat& dxgiFormat = GetDxgiFormat(attribute.format);

            inputElementDesc.SemanticName = attribute.d3d.semanticName;
            inputElementDesc.SemanticIndex = attribute.d3d.semanticIndex;
            inputElementDesc.Format = dxgiFormat.typed;
            inputElementDesc.InputSlot = stream.bindingSlot;
            inputElementDesc.AlignedByteOffset = attribute.offset;
            inputElementDesc.InstanceDataStepRate = stream.stepRate == VertexStreamStepRate::PER_VERTEX ? 0 : 1;
            inputElementDesc.InputSlotClass = stream.stepRate == VertexStreamStepRate::PER_VERTEX ? D3D11_INPUT_PER_VERTEX_DATA : D3D11_INPUT_PER_INSTANCE_DATA;
        };

        CHECK(vertexShader != nullptr, "VS can't be NULL");
        hr = m_Device->CreateInputLayout(&inputElements[0], vi.attributeNum, vertexShader->bytecode, (size_t)vertexShader->size, &m_InputLayout);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateInputLayout");
    }

    // Multisample
    Sample_t sampleNum = 1;
    if (pipelineDesc.multisample) {
        if (pipelineDesc.multisample->sampleMask != ALL)
            m_SampleMask = pipelineDesc.multisample->sampleMask;

        sampleNum = pipelineDesc.multisample->sampleNum;
    }

    { // Rasterization
        const RasterizationDesc& r = pipelineDesc.rasterization;

        D3D11_RASTERIZER_DESC2 rasterizerDesc = {};
        // D3D11_RASTERIZER_DESC
        rasterizerDesc.FillMode = r.fillMode == FillMode::SOLID ? D3D11_FILL_SOLID : D3D11_FILL_WIREFRAME;
        rasterizerDesc.CullMode = GetD3D11CullModeFromCullMode(r.cullMode);
        rasterizerDesc.FrontCounterClockwise = r.frontCounterClockwise;
        rasterizerDesc.DepthBias = (INT)r.depthBias.constant;
        rasterizerDesc.DepthBiasClamp = r.depthBias.clamp;
        rasterizerDesc.SlopeScaledDepthBias = r.depthBias.slope;
        rasterizerDesc.DepthClipEnable = r.depthClamp;
        rasterizerDesc.ScissorEnable = TRUE;
        rasterizerDesc.AntialiasedLineEnable = r.lineSmoothing;
        rasterizerDesc.MultisampleEnable = sampleNum > 1 ? TRUE : FALSE;
        // D3D11_RASTERIZER_DESC1
        // TODO: rasterizerDesc.ForcedSampleCount?
        // D3D11_RASTERIZER_DESC2
        rasterizerDesc.ConservativeRaster = r.conservativeRaster ? D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        RasterizerState rasterizerState = {};
        if (m_Device.GetVersion() >= 3) {
            hr = m_Device->CreateRasterizerState2(&rasterizerDesc, &rasterizerState.ptr);
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device3::CreateRasterizerState2");
        } else if (m_Device.GetVersion() >= 1) {
            hr = m_Device->CreateRasterizerState1((D3D11_RASTERIZER_DESC1*)&rasterizerDesc, (ID3D11RasterizerState1**)&rasterizerState.ptr);
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device3::CreateRasterizerState1");
        } else {
            hr = m_Device->CreateRasterizerState((D3D11_RASTERIZER_DESC*)&rasterizerDesc, (ID3D11RasterizerState**)&rasterizerState.ptr);
            RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateRasterizerState");
        }
        m_RasterizerStates.push_back(rasterizerState);

        // Ex
        memcpy(&m_RasterizerDesc, &rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
#if NRI_ENABLE_NVAPI
        // TODO: m_RasterizerDesc.ForcedSampleCount?
        m_RasterizerDesc.ProgrammableSamplePositionsEnable = true;
        m_RasterizerDesc.SampleCount = sampleNum;
        m_RasterizerDesc.ConservativeRasterEnable = r.conservativeRaster;
        m_RasterizerDesc.TargetIndepentRasterWithDepth = true;
#endif
    }

    { // Depth-stencil
        const DepthAttachmentDesc& da = pipelineDesc.outputMerger.depth;
        const StencilAttachmentDesc& sa = pipelineDesc.outputMerger.stencil;

        D3D11_DEPTH_STENCIL_DESC depthStencilState = {};
        depthStencilState.DepthEnable = da.compareOp == CompareOp::NONE ? FALSE : TRUE;
        depthStencilState.DepthWriteMask = da.write ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        depthStencilState.DepthFunc = GetD3D11ComparisonFuncFromCompareOp(da.compareOp);
        depthStencilState.StencilEnable = (sa.front.compareOp == CompareOp::NONE && sa.back.compareOp == CompareOp::NONE) ? FALSE : TRUE;
        depthStencilState.StencilReadMask = sa.front.compareMask;
        depthStencilState.StencilWriteMask = sa.front.writeMask;

        depthStencilState.FrontFace.StencilFailOp = GetD3D11StencilOpFromStencilOp(sa.front.failOp);
        depthStencilState.FrontFace.StencilDepthFailOp = GetD3D11StencilOpFromStencilOp(sa.front.depthFailOp);
        depthStencilState.FrontFace.StencilPassOp = GetD3D11StencilOpFromStencilOp(sa.front.passOp);
        depthStencilState.FrontFace.StencilFunc = GetD3D11ComparisonFuncFromCompareOp(sa.front.compareOp);

        depthStencilState.BackFace.StencilFailOp = GetD3D11StencilOpFromStencilOp(sa.front.failOp);
        depthStencilState.BackFace.StencilDepthFailOp = GetD3D11StencilOpFromStencilOp(sa.front.depthFailOp);
        depthStencilState.BackFace.StencilPassOp = GetD3D11StencilOpFromStencilOp(sa.front.passOp);
        depthStencilState.BackFace.StencilFunc = GetD3D11ComparisonFuncFromCompareOp(sa.back.compareOp);

        hr = m_Device->CreateDepthStencilState(&depthStencilState, &m_DepthStencilState);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateDepthStencilState");
    }

    { // Blending
        const OutputMergerDesc& om = pipelineDesc.outputMerger;

        D3D11_BLEND_DESC1 blendState1 = {};
        blendState1.AlphaToCoverageEnable = (pipelineDesc.multisample && pipelineDesc.multisample->alphaToCoverage) ? TRUE : FALSE;
        blendState1.IndependentBlendEnable = TRUE;
        for (uint32_t i = 0; i < om.colorNum; i++) {
            const ColorAttachmentDesc& bs = om.colors[i];
            blendState1.RenderTarget[i].BlendEnable = bs.blendEnabled;
            blendState1.RenderTarget[i].SrcBlend = GetD3D11BlendFromBlendFactor(bs.colorBlend.srcFactor);
            blendState1.RenderTarget[i].DestBlend = GetD3D11BlendFromBlendFactor(bs.colorBlend.dstFactor);
            blendState1.RenderTarget[i].BlendOp = GetD3D11BlendOp(bs.colorBlend.op);
            blendState1.RenderTarget[i].SrcBlendAlpha = GetD3D11BlendFromBlendFactor(bs.alphaBlend.srcFactor);
            blendState1.RenderTarget[i].DestBlendAlpha = GetD3D11BlendFromBlendFactor(bs.alphaBlend.dstFactor);
            blendState1.RenderTarget[i].BlendOpAlpha = GetD3D11BlendOp(bs.alphaBlend.op);
            blendState1.RenderTarget[i].RenderTargetWriteMask = uint8_t(bs.colorWriteMask);
            blendState1.RenderTarget[i].LogicOpEnable = om.logicOp != LogicOp::NONE ? TRUE : FALSE;
            blendState1.RenderTarget[i].LogicOp = GetD3D11LogicOp(om.logicOp);
        }

        if (m_Device.GetVersion() >= 1)
            hr = m_Device->CreateBlendState1(&blendState1, &m_BlendState);
        else {
            D3D11_BLEND_DESC blendState = {};
            blendState.AlphaToCoverageEnable = blendState1.AlphaToCoverageEnable;
            blendState.IndependentBlendEnable = blendState1.IndependentBlendEnable;
            for (uint32_t i = 0; i < om.colorNum; i++) {
                blendState.RenderTarget[i].BlendEnable = blendState1.RenderTarget[i].BlendEnable;
                blendState.RenderTarget[i].SrcBlend = blendState1.RenderTarget[i].SrcBlend;
                blendState.RenderTarget[i].DestBlend = blendState1.RenderTarget[i].DestBlend;
                blendState.RenderTarget[i].BlendOp = blendState1.RenderTarget[i].BlendOp;
                blendState.RenderTarget[i].SrcBlendAlpha = blendState1.RenderTarget[i].SrcBlendAlpha;
                blendState.RenderTarget[i].DestBlendAlpha = blendState1.RenderTarget[i].DestBlendAlpha;
                blendState.RenderTarget[i].BlendOpAlpha = blendState1.RenderTarget[i].BlendOpAlpha;
                blendState.RenderTarget[i].RenderTargetWriteMask = blendState1.RenderTarget[i].RenderTargetWriteMask;
            }

            hr = m_Device->CreateBlendState(&blendState, (ID3D11BlendState**)&m_BlendState);
        }

        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device1::CreateBlendState1");
    }

    return Result::SUCCESS;
}

Result PipelineD3D11::Create(const ComputePipelineDesc& pipelineDesc) {
    if (pipelineDesc.shader.bytecode) {
        HRESULT hr = m_Device->CreateComputeShader(pipelineDesc.shader.bytecode, (size_t)pipelineDesc.shader.size, nullptr, &m_ComputeShader);

        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateComputeShader");
    }

    m_PipelineLayout = (PipelineLayoutD3D11*)pipelineDesc.pipelineLayout;

    return Result::SUCCESS;
}

void PipelineD3D11::ChangeRasterizerState(ID3D11DeviceContextBest* deferredContext, const SamplePositionsState& samplePositionState) {
    MaybeUnused(deferredContext, samplePositionState);
    CHECK(!IsCompute(), "A graphics pipeline expected");

    // Find in cached states
    size_t i = 0;
#if NRI_ENABLE_NVAPI
    for (; i < m_RasterizerStates.size(); i++) {
        if (m_RasterizerStates[i].samplePositionHash == samplePositionState.positionHash)
            break;
    }

    // Add a new state, if not found
    if (i == m_RasterizerStates.size()) {
        RasterizerState newState = {};
        newState.samplePositionHash = samplePositionState.positionHash;

        m_RasterizerDesc.InterleavedSamplingEnable = samplePositionState.positionNum > m_RasterizerDesc.SampleCount;
        for (uint32_t j = 0; j < samplePositionState.positionNum; j++) {
            m_RasterizerDesc.SamplePositionsX[j] = samplePositionState.positions[j].x + 8;
            m_RasterizerDesc.SamplePositionsY[j] = samplePositionState.positions[j].y + 8;
        }

        if (m_Device.HasNvExt())
            REPORT_ERROR_ON_BAD_NVAPI_STATUS(&m_Device, NvAPI_D3D11_CreateRasterizerState(m_Device.GetNativeObject(), &m_RasterizerDesc, (ID3D11RasterizerState**)&newState.ptr));

        if (newState.ptr)
            m_RasterizerStates.push_back(newState);
        else
            i = 0;
    }
#endif

    // Bind
    ID3D11RasterizerState2* stateWithPSL = m_RasterizerStates[i].ptr;
    deferredContext->RSSetState(stateWithPSL);
}

void PipelineD3D11::ChangeStencilReference(ID3D11DeviceContextBest* deferredContext, uint8_t stencilRef) {
    CHECK(!IsCompute(), "A graphics pipeline expected");
    deferredContext->OMSetDepthStencilState(m_DepthStencilState, stencilRef);
}

void PipelineD3D11::ChangeBlendConstants(ID3D11DeviceContextBest* deferredContext, const Color32f& color) {
    CHECK(!IsCompute(), "A graphics pipeline expected");
    deferredContext->OMSetBlendState(m_BlendState, &color.x, m_SampleMask);
}

void PipelineD3D11::Bind(ID3D11DeviceContextBest* deferredContext, const PipelineD3D11* currentPipeline, uint8_t stencilRef, const Color32f& blendFactor,
    const SamplePositionsState& samplePositionState) {
    if (IsCompute()) {
        if (!currentPipeline || m_ComputeShader != currentPipeline->m_ComputeShader)
            deferredContext->CSSetShader(m_ComputeShader, nullptr, 0);
    } else {
        if (!currentPipeline || m_Topology != currentPipeline->m_Topology)
            deferredContext->IASetPrimitiveTopology(m_Topology);

        if (!currentPipeline || m_InputLayout != currentPipeline->m_InputLayout)
            deferredContext->IASetInputLayout(m_InputLayout);

        if (!currentPipeline || m_VertexShader != currentPipeline->m_VertexShader)
            deferredContext->VSSetShader(m_VertexShader, nullptr, 0);

        if (!currentPipeline || m_TessControlShader != currentPipeline->m_TessControlShader)
            deferredContext->HSSetShader(m_TessControlShader, nullptr, 0);

        if (!currentPipeline || m_TessEvaluationShader != currentPipeline->m_TessEvaluationShader)
            deferredContext->DSSetShader(m_TessEvaluationShader, nullptr, 0);

        if (!currentPipeline || m_GeometryShader != currentPipeline->m_GeometryShader)
            deferredContext->GSSetShader(m_GeometryShader, nullptr, 0);

        if (!currentPipeline || m_FragmentShader != currentPipeline->m_FragmentShader)
            deferredContext->PSSetShader(m_FragmentShader, nullptr, 0);

        // Dynamic state // TODO: uncached
        deferredContext->OMSetDepthStencilState(m_DepthStencilState, stencilRef);
        deferredContext->OMSetBlendState(m_BlendState, &blendFactor.x, m_SampleMask);

        ChangeRasterizerState(deferredContext, samplePositionState);
    }
}

NRI_INLINE void PipelineD3D11::SetDebugName(const char* name) {
    SET_D3D_DEBUG_OBJECT_NAME(m_VertexShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_TessControlShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_TessEvaluationShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_GeometryShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_FragmentShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_ComputeShader, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_InputLayout, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_DepthStencilState, name);
    SET_D3D_DEBUG_OBJECT_NAME(m_BlendState, name);

    for (size_t i = 0; i < m_RasterizerStates.size(); i++)
        SET_D3D_DEBUG_OBJECT_NAME(m_RasterizerStates[i].ptr, name);
}
