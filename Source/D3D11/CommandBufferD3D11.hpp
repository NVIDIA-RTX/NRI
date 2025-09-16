// © 2021 NVIDIA Corporation

uint8_t QueryLatestDeviceContext(ComPtr<ID3D11DeviceContextBest>& in, ComPtr<ID3D11DeviceContextBest>& out) {
    static const IID versions[] = {
        __uuidof(ID3D11DeviceContext4),
        __uuidof(ID3D11DeviceContext3),
        __uuidof(ID3D11DeviceContext2),
        __uuidof(ID3D11DeviceContext1),
        __uuidof(ID3D11DeviceContext),
    };
    const uint8_t n = (uint8_t)GetCountOf(versions);

    uint8_t i = 0;
    for (; i < n; i++) {
        HRESULT hr = in->QueryInterface(versions[i], (void**)&out);
        if (SUCCEEDED(hr))
            break;
    }

    return n - i - 1;
}

CommandBufferD3D11::CommandBufferD3D11(DeviceD3D11& device)
    : m_Device(device)
    , m_DeferredContext(device.GetImmediateContext())
    , m_BindingState(device.GetStdAllocator())
    , m_Version(device.GetImmediateContextVersion()) {
    m_DeferredContext->QueryInterface(IID_PPV_ARGS(&m_Annotation));
}

CommandBufferD3D11::~CommandBufferD3D11() {
    if (m_DeferredContext && m_DeferredContext->GetType() == D3D11_DEVICE_CONTEXT_DEFERRED) {
#if NRI_ENABLE_NVAPI
        if (m_Device.HasNvExt()) {
            NvAPI_Status status = NvAPI_D3D11_EndUAVOverlap(m_DeferredContext);
            if (status != NVAPI_OK)
                REPORT_WARNING(&m_Device, "NvAPI_D3D11_EndUAVOverlap() failed!");
        }
#endif
#if NRI_ENABLE_AMDAGS
        if (m_Device.HasAmdExt()) {
            const AmdExtD3D11& amdExt = m_Device.GetAmdExt();
            AGSReturnCode res = amdExt.EndUAVOverlap(amdExt.context, m_DeferredContext);
            if (res != AGS_SUCCESS)
                REPORT_WARNING(&m_Device, "agsDriverExtensionsDX11_EndUAVOverlap() failed!");
        }
#endif
    }
}

Result CommandBufferD3D11::Create(ID3D11DeviceContext* precreatedContext) {
    // Release inherited interfaces from the immediate context
    m_DeferredContext = nullptr;
    m_Annotation = nullptr;

    // Create deferred context
    ComPtr<ID3D11DeviceContextBest> context = (ID3D11DeviceContextBest*)precreatedContext; // can be immediate
    if (!precreatedContext) {
        HRESULT hr = m_Device->CreateDeferredContext(0, (ID3D11DeviceContext**)&context);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11Device::CreateDeferredContext");
    }

    m_Version = QueryLatestDeviceContext(context, m_DeferredContext);

    HRESULT hr = m_DeferredContext->QueryInterface(IID_PPV_ARGS(&m_Annotation));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "QueryInterface(ID3DUserDefinedAnnotation)");

    // Skip UAV barriers by default on the deferred context
    if (m_DeferredContext && m_DeferredContext->GetType() == D3D11_DEVICE_CONTEXT_DEFERRED) {
#if NRI_ENABLE_NVAPI
        if (m_Device.HasNvExt()) {
            NvAPI_Status res = NvAPI_D3D11_BeginUAVOverlap(m_DeferredContext);
            RETURN_ON_FAILURE(&m_Device, res == NVAPI_OK, Result::FAILURE, "NvAPI_D3D11_BeginUAVOverlap() failed!");
        }
#endif
#if NRI_ENABLE_AMDAGS
        if (m_Device.HasAmdExt()) {
            const AmdExtD3D11& amdExt = m_Device.GetAmdExt();
            AGSReturnCode res = amdExt.BeginUAVOverlap(amdExt.context, m_DeferredContext);
            RETURN_ON_FAILURE(&m_Device, res == AGS_SUCCESS, Result::FAILURE, "agsDriverExtensionsDX11_BeginUAVOverlap() failed!");
        }
#endif
    }

    return Result::SUCCESS;
}

void CommandBufferD3D11::Submit() {
    m_Device.GetImmediateContext()->ExecuteCommandList(m_CommandList, FALSE);
    m_CommandList = nullptr;
}

NRI_INLINE Result CommandBufferD3D11::Begin(const DescriptorPool* descriptorPool) {
    m_CommandList = nullptr;
    m_Pipeline = nullptr;
    m_PipelineLayout = nullptr;
    m_PipelineBindPoint = BindPoint::INHERIT;

    ResetAttachments();

    // Dynamic state
    m_SamplePositionsState.Reset();
    m_StencilRef = 0;
    m_BlendFactor = {};

    if (descriptorPool)
        SetDescriptorPool(*descriptorPool);

    return Result::SUCCESS;
}

NRI_INLINE Result CommandBufferD3D11::End() {
    HRESULT hr = m_DeferredContext->FinishCommandList(FALSE, &m_CommandList);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D11DeviceContext::FinishCommandList");

    m_BindingState.UnbindAndReset(m_DeferredContext);

    return Result::SUCCESS;
}

NRI_INLINE void CommandBufferD3D11::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    Scratch<D3D11_VIEWPORT> d3dViewports = AllocateScratch(m_Device, D3D11_VIEWPORT, viewportNum);
    for (uint32_t i = 0; i < viewportNum; i++) {
        const Viewport& in = viewports[i];
        D3D11_VIEWPORT& out = d3dViewports[i];
        out.TopLeftX = in.x;
        out.TopLeftY = in.y;
        out.Width = in.width;
        out.Height = in.height;
        out.MinDepth = in.depthMin;
        out.MaxDepth = in.depthMax;
    }

    m_DeferredContext->RSSetViewports(viewportNum, d3dViewports);
}

NRI_INLINE void CommandBufferD3D11::SetScissors(const Rect* rects, uint32_t rectNum) {
    Scratch<D3D11_RECT> rectsD3D = AllocateScratch(m_Device, D3D11_RECT, rectNum);

    for (uint32_t i = 0; i < rectNum; i++) {
        const Rect& rect = rects[i];
        rectsD3D[i] = {rect.x, rect.y, (LONG)(rect.x + rect.width), (LONG)(rect.y + rect.height)};
    }

    m_DeferredContext->RSSetScissorRects(rectNum, &rectsD3D[0]);
}

NRI_INLINE void CommandBufferD3D11::SetDepthBounds(float boundsMin, float boundsMax) {
    if (m_DepthBounds[0] != boundsMin || m_DepthBounds[1] != boundsMax) {
        bool isEnabled = boundsMin != 0.0f || boundsMax != 1.0f;
        MaybeUnused(isEnabled);
#if NRI_ENABLE_NVAPI
        if (m_Device.HasNvExt()) {
            NvAPI_Status status = NvAPI_D3D11_SetDepthBoundsTest(m_DeferredContext, isEnabled, boundsMin, boundsMax);
            RETURN_ON_FAILURE(&m_Device, status == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_SetDepthBoundsTest() failed!");
        }
#endif
#if NRI_ENABLE_AMDAGS
        if (m_Device.HasAmdExt()) {
            const AmdExtD3D11& amdExt = m_Device.GetAmdExt();
            AGSReturnCode res = amdExt.SetDepthBounds(amdExt.context, m_DeferredContext, isEnabled, boundsMin, boundsMax);
            RETURN_ON_FAILURE(&m_Device, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_SetDepthBounds() failed!");
        }
#endif

        m_DepthBounds[0] = boundsMin;
        m_DepthBounds[1] = boundsMax;
    }
}

NRI_INLINE void CommandBufferD3D11::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    MaybeUnused(backRef);

    if (m_Pipeline)
        m_Pipeline->ChangeStencilReference(m_DeferredContext, frontRef);

    m_StencilRef = frontRef;
}

NRI_INLINE void CommandBufferD3D11::SetSampleLocations(const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    MaybeUnused(sampleNum); // already have this in "m_RasterizerDesc"

    m_SamplePositionsState.Set(locations, locationNum);

    if (m_Pipeline)
        m_Pipeline->ChangeRasterizerState(m_DeferredContext, m_SamplePositionsState);
}

NRI_INLINE void CommandBufferD3D11::SetBlendConstants(const Color32f& color) {
    if (m_Pipeline)
        m_Pipeline->ChangeBlendConstants(m_DeferredContext, color);

    m_BlendFactor = color;
}

NRI_INLINE void CommandBufferD3D11::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    if (!clearDescNum)
        return;

    if (!rectNum) {
        for (uint32_t i = 0; i < clearDescNum; i++) {
            const ClearDesc& clearDesc = clearDescs[i];

            if (clearDesc.planes & PlaneBits::COLOR)
                m_DeferredContext->ClearRenderTargetView(m_RenderTargets[clearDesc.colorAttachmentIndex], &clearDesc.value.color.f.x);
            else {
                uint32_t clearFlags = 0;
                if (clearDescs[i].planes & PlaneBits::DEPTH)
                    clearFlags |= D3D11_CLEAR_DEPTH;
                if (clearDescs[i].planes & PlaneBits::STENCIL)
                    clearFlags |= D3D11_CLEAR_STENCIL;

                m_DeferredContext->ClearDepthStencilView(m_DepthStencil, clearFlags, clearDesc.value.depthStencil.depth, clearDesc.value.depthStencil.stencil);
            }
        }
    } else {
        Scratch<D3D11_RECT> rectsD3D = AllocateScratch(m_Device, D3D11_RECT, rectNum);
        for (uint32_t i = 0; i < rectNum; i++) {
            const Rect& rect = rects[i];
            rectsD3D[i] = {rect.x, rect.y, (LONG)(rect.x + rect.width), (LONG)(rect.y + rect.height)};
        }

        if (m_Version >= 1) {
            // https://learn.microsoft.com/en-us/windows/win32/api/d3d11_1/nf-d3d11_1-id3d11devicecontext1-clearview
            FLOAT color[4] = {};
            for (uint32_t i = 0; i < clearDescNum; i++) {
                const ClearDesc& clearDesc = clearDescs[i];

                if (clearDesc.planes & PlaneBits::COLOR)
                    m_DeferredContext->ClearView(m_RenderTargets[clearDesc.colorAttachmentIndex], &clearDesc.value.color.f.x, rectsD3D, rectNum);
                else if (clearDesc.planes & PlaneBits::DEPTH) {
                    color[0] = clearDesc.value.depthStencil.depth;
                    m_DeferredContext->ClearView(m_DepthStencil, color, rectsD3D, rectNum);
                } else
                    CHECK(false, "Bad or unsupported plane");
            }
        } else
            CHECK(false, "'ClearView' emulation for 11.0 is not implemented!");
    }
}

NRI_INLINE void CommandBufferD3D11::ClearStorage(const ClearStorageDesc& clearDesc) {
    DescriptorD3D11& storage = *(DescriptorD3D11*)clearDesc.storage;

    if (storage.IsIntegerFormat() || storage.IsBuffer())
        m_DeferredContext->ClearUnorderedAccessViewUint(storage, &clearDesc.value.ui.x);
    else
        m_DeferredContext->ClearUnorderedAccessViewFloat(storage, &clearDesc.value.f.x);
}

NRI_INLINE void CommandBufferD3D11::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    // Render targets
    m_RenderTargetNum = attachmentsDesc.colors ? attachmentsDesc.colorNum : 0;

    size_t i = 0;
    for (; i < m_RenderTargetNum; i++) {
        const DescriptorD3D11& descriptor = *(DescriptorD3D11*)attachmentsDesc.colors[i];
        m_RenderTargets[i] = descriptor;
    }
    for (; i < m_RenderTargets.size(); i++)
        m_RenderTargets[i] = nullptr;

    if (attachmentsDesc.depthStencil) {
        const DescriptorD3D11& descriptor = *(DescriptorD3D11*)attachmentsDesc.depthStencil;
        m_DepthStencil = descriptor;
    } else
        m_DepthStencil = nullptr;

    m_DeferredContext->OMSetRenderTargets(m_RenderTargetNum, m_RenderTargets.data(), m_DepthStencil);

    // Shading rate
#if NRI_ENABLE_NVAPI
    if (m_Device.HasNvExt() && m_Device.GetDesc().tiers.shadingRate >= 2) {
        ID3D11NvShadingRateResourceView* shadingRateImage = nullptr;
        if (attachmentsDesc.shadingRate) {
            const DescriptorD3D11& descriptor = *(DescriptorD3D11*)attachmentsDesc.shadingRate;
            shadingRateImage = descriptor;

            // Program shading rate lookup table
            if (!m_IsShadingRateLookupTableSet) {
                std::array<NV_D3D11_VIEWPORT_SHADING_RATE_DESC_V1, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> shadingRates = {};
                for (i = 0; i < shadingRates.size(); i++) {
                    shadingRates[i].enableVariablePixelShadingRate = true;

                    for (size_t j = 0; j < GetCountOf(shadingRates[i].shadingRateTable); j++)
                        shadingRates[i].shadingRateTable[j] = NV_PIXEL_X1_PER_RASTER_PIXEL; // be on the safe side, avoid culling

                    shadingRates[i].shadingRateTable[NRI_SHADING_RATE(0, 0)] = NV_PIXEL_X1_PER_RASTER_PIXEL;
                    shadingRates[i].shadingRateTable[NRI_SHADING_RATE(0, 1)] = NV_PIXEL_X1_PER_1X2_RASTER_PIXELS;
                    shadingRates[i].shadingRateTable[NRI_SHADING_RATE(1, 0)] = NV_PIXEL_X1_PER_2X1_RASTER_PIXELS;
                    shadingRates[i].shadingRateTable[NRI_SHADING_RATE(1, 1)] = NV_PIXEL_X1_PER_2X2_RASTER_PIXELS;
                    shadingRates[i].shadingRateTable[NRI_SHADING_RATE(1, 2)] = NV_PIXEL_X1_PER_2X4_RASTER_PIXELS;
                    shadingRates[i].shadingRateTable[NRI_SHADING_RATE(2, 1)] = NV_PIXEL_X1_PER_4X2_RASTER_PIXELS;
                    shadingRates[i].shadingRateTable[NRI_SHADING_RATE(2, 2)] = NV_PIXEL_X1_PER_4X4_RASTER_PIXELS;
                }

                NV_D3D11_VIEWPORTS_SHADING_RATE_DESC shadingRateDesc = {NV_D3D11_VIEWPORTS_SHADING_RATE_DESC_VER};
                shadingRateDesc.numViewports = (uint32_t)shadingRates.size();
                shadingRateDesc.pViewports = shadingRates.data();

                REPORT_ERROR_ON_BAD_NVAPI_STATUS(&m_Device, NvAPI_D3D11_RSSetViewportsPixelShadingRates(m_DeferredContext, &shadingRateDesc));

                m_IsShadingRateLookupTableSet = true;
            }
        }

        REPORT_ERROR_ON_BAD_NVAPI_STATUS(&m_Device, NvAPI_D3D11_RSSetShadingRateResourceView(m_DeferredContext, shadingRateImage));
    }
#endif

    // Multiview
#if NRI_ENABLE_AMDAGS
    if (m_Device.HasAmdExt() && m_Device.GetDesc().other.viewMaxNum > 1) {
        const AmdExtD3D11& amdExt = m_Device.GetAmdExt();
        AGSReturnCode res = amdExt.SetViewBroadcastMasks(amdExt.context, attachmentsDesc.viewMask, attachmentsDesc.viewMask ? 0x1 : 0x0, 0);
        RETURN_ON_FAILURE(&m_Device, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_SetViewBroadcastMasks() failed!");
    }
#endif
}

NRI_INLINE void CommandBufferD3D11::SetVertexBuffers(uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum) {
    Scratch<uint8_t> scratch = AllocateScratch(m_Device, uint8_t, vertexBufferNum * (sizeof(ID3D11Buffer*) + sizeof(uint32_t) * 2));
    uint8_t* ptr = scratch;

    ID3D11Buffer** buffers = (ID3D11Buffer**)ptr;
    ptr += vertexBufferNum * sizeof(ID3D11Buffer*);

    uint32_t* strides = (uint32_t*)ptr;
    ptr += vertexBufferNum * sizeof(uint32_t);

    uint32_t* offsets = (uint32_t*)ptr;

    for (uint32_t i = 0; i < vertexBufferNum; i++) {
        const VertexBufferDesc& vertexBufferDesc = vertexBufferDescs[i];

        const BufferD3D11* bufferD3D11 = (BufferD3D11*)vertexBufferDesc.buffer;
        if (bufferD3D11) {
            buffers[i] = *bufferD3D11;
            offsets[i] = (uint32_t)vertexBufferDesc.offset;
            strides[i] = vertexBufferDesc.stride;
        } else {
            buffers[i] = 0;
            offsets[i] = 0;
            strides[i] = 0;
        }
    }

    m_DeferredContext->IASetVertexBuffers(baseSlot, vertexBufferNum, buffers, strides, offsets);
}

NRI_INLINE void CommandBufferD3D11::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    const BufferD3D11& bufferD3D11 = (BufferD3D11&)buffer;
    const DXGI_FORMAT format = indexType == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

    m_DeferredContext->IASetIndexBuffer(bufferD3D11, format, (uint32_t)offset);
}

NRI_INLINE void CommandBufferD3D11::SetPipelineLayout(BindPoint bindPoint, const PipelineLayout& pipelineLayout) {
    PipelineLayoutD3D11* pipelineLayoutD3D11 = (PipelineLayoutD3D11*)&pipelineLayout;
    pipelineLayoutD3D11->Bind(m_DeferredContext);

    m_PipelineLayout = pipelineLayoutD3D11;
    m_PipelineBindPoint = bindPoint;
}

NRI_INLINE void CommandBufferD3D11::SetPipeline(const Pipeline& pipeline) {
    PipelineD3D11* pipelineD3D11 = (PipelineD3D11*)&pipeline;
    pipelineD3D11->Bind(m_DeferredContext, m_Pipeline, m_StencilRef, m_BlendFactor, m_SamplePositionsState);

    m_Pipeline = pipelineD3D11; // needed only for "SetStencilReference", "SetBlendConstants" and "SetSampleLocations"
}

NRI_INLINE void CommandBufferD3D11::SetDescriptorPool(const DescriptorPool&) {
}

NRI_INLINE void CommandBufferD3D11::SetDescriptorSet(const SetDescriptorSetDesc& setDescriptorSetDesc) {
    BindPoint bindPoint = setDescriptorSetDesc.bindPoint == BindPoint::INHERIT ? m_PipelineBindPoint : setDescriptorSetDesc.bindPoint;
    const DescriptorSetD3D11& descriptorSetD3D11 = *(DescriptorSetD3D11*)setDescriptorSetDesc.descriptorSet;

    m_PipelineLayout->SetDescriptorSet(bindPoint, m_BindingState, m_DeferredContext, setDescriptorSetDesc.setIndex, &descriptorSetD3D11, nullptr, 0);
}

NRI_INLINE void CommandBufferD3D11::SetRootConstants(const SetRootConstantsDesc& setRootConstantsDesc) {
    m_PipelineLayout->SetRootConstants(m_DeferredContext, setRootConstantsDesc);
}

NRI_INLINE void CommandBufferD3D11::SetRootDescriptor(const SetRootDescriptorDesc& setRootDescriptorDesc) {
    BindPoint bindPoint = setRootDescriptorDesc.bindPoint == BindPoint::INHERIT ? m_PipelineBindPoint : setRootDescriptorDesc.bindPoint;
    uint32_t setIndex = m_PipelineLayout->GetRootBindingIndex(setRootDescriptorDesc.rootDescriptorIndex);
    const DescriptorD3D11& descriptorD3D11 = *(DescriptorD3D11*)setRootDescriptorDesc.descriptor;

    m_PipelineLayout->SetDescriptorSet(bindPoint, m_BindingState, m_DeferredContext, setIndex, nullptr, &descriptorD3D11, setRootDescriptorDesc.offset);
}

NRI_INLINE void CommandBufferD3D11::Draw(const DrawDesc& drawDesc) {
    m_DeferredContext->DrawInstanced(drawDesc.vertexNum, drawDesc.instanceNum, drawDesc.baseVertex, drawDesc.baseInstance);
}

NRI_INLINE void CommandBufferD3D11::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    m_DeferredContext->DrawIndexedInstanced(drawIndexedDesc.indexNum, drawIndexedDesc.instanceNum, drawIndexedDesc.baseIndex, drawIndexedDesc.baseVertex, drawIndexedDesc.baseInstance);
}

NRI_INLINE void CommandBufferD3D11::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    MaybeUnused(countBuffer, countBufferOffset);

    const BufferD3D11& bufferD3D11 = (BufferD3D11&)buffer;
    if (countBuffer && m_Device.HasAmdExt()) {
#if NRI_ENABLE_AMDAGS
        const BufferD3D11* countBufferD3D11 = (BufferD3D11*)countBuffer;
        const AmdExtD3D11& amdExt = m_Device.GetAmdExt();
        AGSReturnCode res = amdExt.DrawIndirectCount(amdExt.context, m_DeferredContext, *countBufferD3D11, (uint32_t)countBufferOffset, bufferD3D11, (uint32_t)offset, stride);
        RETURN_ON_FAILURE(&m_Device, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_MultiDrawInstancedIndirectCountIndirect() failed!");
#endif
    } else if (m_Device.HasNvExt() && drawNum > 1) {
#if NRI_ENABLE_NVAPI
        NvAPI_Status status = NvAPI_D3D11_MultiDrawInstancedIndirect(m_DeferredContext, drawNum, bufferD3D11, (uint32_t)offset, stride);
        RETURN_ON_FAILURE(&m_Device, status == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_MultiDrawInstancedIndirect() failed!");
#endif
    } else if (m_Device.HasAmdExt() && drawNum > 1) {
#if NRI_ENABLE_AMDAGS
        const AmdExtD3D11& amdExt = m_Device.GetAmdExt();
        AGSReturnCode res = amdExt.DrawIndirect(amdExt.context, m_DeferredContext, drawNum, bufferD3D11, (uint32_t)offset, stride);
        RETURN_ON_FAILURE(&m_Device, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_MultiDrawInstancedIndirect() failed!");
#endif
    } else {
        CHECK(!countBuffer, "'countBuffer' is unsupported");

        for (uint32_t i = 0; i < drawNum; i++) {
            m_DeferredContext->DrawInstancedIndirect(bufferD3D11, (uint32_t)offset);
            offset += stride;
        }
    }
}

NRI_INLINE void CommandBufferD3D11::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    MaybeUnused(countBuffer, countBufferOffset);

    const BufferD3D11& bufferD3D11 = (BufferD3D11&)buffer;
    if (countBuffer && m_Device.HasAmdExt()) {
#if NRI_ENABLE_AMDAGS
        const BufferD3D11* countBufferD3D11 = (BufferD3D11*)countBuffer;
        const AmdExtD3D11& amdExt = m_Device.GetAmdExt();
        AGSReturnCode res = amdExt.DrawIndexedIndirectCount(amdExt.context, m_DeferredContext, *countBufferD3D11, (uint32_t)countBufferOffset, bufferD3D11, (uint32_t)offset, stride);
        RETURN_ON_FAILURE(&m_Device, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirectCountIndirect() failed!");
#endif
    } else if (m_Device.HasNvExt() && drawNum > 1) {
#if NRI_ENABLE_NVAPI
        NvAPI_Status status = NvAPI_D3D11_MultiDrawIndexedInstancedIndirect(m_DeferredContext, drawNum, bufferD3D11, (uint32_t)offset, stride);
        RETURN_ON_FAILURE(&m_Device, status == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_MultiDrawIndexedInstancedIndirect() failed!");
#endif
    } else if (m_Device.HasAmdExt() && drawNum > 1) {
#if NRI_ENABLE_AMDAGS
        const AmdExtD3D11& amdExt = m_Device.GetAmdExt();
        AGSReturnCode res = amdExt.DrawIndexedIndirect(amdExt.context, m_DeferredContext, drawNum, bufferD3D11, (uint32_t)offset, stride);
        RETURN_ON_FAILURE(&m_Device, res == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_MultiDrawIndexedInstancedIndirect() failed!");
#endif
    } else {
        CHECK(!countBuffer, "'countBuffer' is unsupported");

        for (uint32_t i = 0; i < drawNum; i++) {
            m_DeferredContext->DrawIndexedInstancedIndirect(bufferD3D11, (uint32_t)offset);
            offset += stride;
        }
    }
}

NRI_INLINE void CommandBufferD3D11::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    const BufferD3D11& dst = (BufferD3D11&)dstBuffer;
    const BufferD3D11& src = (BufferD3D11&)srcBuffer;

    if (size == WHOLE_SIZE)
        size = src.GetDesc().size;

    bool isWholeResource = (srcOffset == 0 && dstOffset == 0);
    isWholeResource &= src.GetDesc().size == size;
    isWholeResource &= dst.GetDesc().size == size;

    if (isWholeResource)
        m_DeferredContext->CopyResource(dst, src);
    else {
        D3D11_BOX box = {};
        box.left = uint32_t(srcOffset);
        box.right = uint32_t(srcOffset + size);
        box.bottom = 1;
        box.back = 1;

        m_DeferredContext->CopySubresourceRegion(dst, 0, (uint32_t)dstOffset, 0, 0, src, 0, &box);
    }
}

NRI_INLINE void CommandBufferD3D11::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    const TextureD3D11& dst = (TextureD3D11&)dstTexture;
    const TextureD3D11& src = (TextureD3D11&)srcTexture;

    bool isWholeResource = (!dstRegion || dstRegion->mipOffset == NULL_TEXTURE_REGION_DESC) && (!srcRegion || srcRegion->mipOffset == NULL_TEXTURE_REGION_DESC);
    if (isWholeResource)
        m_DeferredContext->CopyResource(dst, src);
    else {
        TextureRegionDesc wholeResource = {};
        if (!srcRegion || srcRegion->mipOffset == NULL_TEXTURE_REGION_DESC)
            srcRegion = &wholeResource;
        if (!dstRegion || dstRegion->mipOffset == NULL_TEXTURE_REGION_DESC)
            dstRegion = &wholeResource;

        D3D11_BOX srcBox = {};
        srcBox.left = srcRegion->x;
        srcBox.top = srcRegion->y;
        srcBox.front = srcRegion->z;
        srcBox.right = srcRegion->width == WHOLE_SIZE ? src.GetSize(0, srcRegion->mipOffset) : srcRegion->width;
        srcBox.bottom = srcRegion->height == WHOLE_SIZE ? src.GetSize(1, srcRegion->mipOffset) : srcRegion->height;
        srcBox.back = srcRegion->depth == WHOLE_SIZE ? src.GetSize(2, srcRegion->mipOffset) : srcRegion->depth;
        srcBox.right += srcBox.left;
        srcBox.bottom += srcBox.top;
        srcBox.back += srcBox.front;

        uint32_t dstSubresource = dst.GetSubresourceIndex(dstRegion->layerOffset, dstRegion->mipOffset);
        uint32_t srcSubresource = src.GetSubresourceIndex(srcRegion->layerOffset, srcRegion->mipOffset);

        m_DeferredContext->CopySubresourceRegion(dst, dstSubresource, dstRegion->x, dstRegion->y, dstRegion->z, src, srcSubresource, &srcBox);
    }
}

NRI_INLINE void CommandBufferD3D11::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegion, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayout) {
    BufferD3D11& src = (BufferD3D11&)srcBuffer;
    const TextureD3D11& dst = (TextureD3D11&)dstTexture;

    D3D11_BOX dstBox = {};
    dstBox.left = dstRegion.x;
    dstBox.top = dstRegion.y;
    dstBox.front = dstRegion.z;
    dstBox.right = dstRegion.width == WHOLE_SIZE ? dst.GetSize(0, dstRegion.mipOffset) : dstRegion.width;
    dstBox.bottom = dstRegion.height == WHOLE_SIZE ? dst.GetSize(1, dstRegion.mipOffset) : dstRegion.height;
    dstBox.back = dstRegion.depth == WHOLE_SIZE ? dst.GetSize(2, dstRegion.mipOffset) : dstRegion.depth;
    dstBox.right += dstBox.left;
    dstBox.bottom += dstBox.top;
    dstBox.back += dstBox.front;

    uint32_t dstSubresource = dst.GetSubresourceIndex(dstRegion.layerOffset, dstRegion.mipOffset);

    uint32_t flags = D3D11_COPY_NO_OVERWRITE;
    if (dstRegion.x == 0 && dstRegion.width == WHOLE_SIZE && dstRegion.y == 0 && dstRegion.height == WHOLE_SIZE && dstRegion.z == 0 && dstRegion.depth == WHOLE_SIZE)
        flags = D3D11_COPY_DISCARD;

    uint8_t* data = (uint8_t*)src.Map(srcDataLayout.offset);
    m_DeferredContext->UpdateSubresource1(dst, dstSubresource, &dstBox, data, srcDataLayout.rowPitch, srcDataLayout.slicePitch, flags);
    src.Unmap();
}

NRI_INLINE void CommandBufferD3D11::ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayout, const Texture& srcTexture, const TextureRegionDesc& srcRegion) {
    CHECK(dstDataLayout.offset == 0, "D3D11 implementation currently supports copying a texture region to a buffer only with offset = 0!");

    BufferD3D11& dst = (BufferD3D11&)dstBuffer;
    const TextureD3D11& src = (TextureD3D11&)srcTexture;

    TextureD3D11& dstTemp = dst.RecreateReadbackTexture(src, srcRegion, dstDataLayout);

    TextureRegionDesc dstRegion = {};
    dstRegion.mipOffset = srcRegion.mipOffset;
    dstRegion.layerOffset = srcRegion.layerOffset;
    dstRegion.width = srcRegion.width == WHOLE_SIZE ? src.GetSize(0, srcRegion.mipOffset) : srcRegion.width;
    dstRegion.height = srcRegion.height == WHOLE_SIZE ? src.GetSize(1, srcRegion.mipOffset) : srcRegion.height;
    dstRegion.depth = srcRegion.depth == WHOLE_SIZE ? src.GetSize(2, srcRegion.mipOffset) : srcRegion.depth;

    CopyTexture((Texture&)dstTemp, &dstRegion, srcTexture, &srcRegion);
}

NRI_INLINE void CommandBufferD3D11::ZeroBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    const BufferD3D11& dst = (BufferD3D11&)buffer;
    ID3D11Buffer* zeroBuffer = m_Device.GetZeroBuffer();

    D3D11_BUFFER_DESC zeroBufferDesc = {};
    zeroBuffer->GetDesc(&zeroBufferDesc);

    if (size == WHOLE_SIZE)
        size = dst.GetDesc().size;

    D3D11_BOX box = {};
    box.left = 0;
    box.right = (uint32_t)size;
    box.bottom = 1;
    box.back = 1;

    while (box.right) {
        uint32_t blockSize = box.right < zeroBufferDesc.ByteWidth ? box.right : zeroBufferDesc.ByteWidth;

        m_DeferredContext->CopySubresourceRegion(dst, 0, (uint32_t)offset, 0, 0, m_Device.GetZeroBuffer(), 0, &box);

        offset += blockSize;
        box.right -= blockSize;
    }
}

NRI_INLINE void CommandBufferD3D11::ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    const TextureD3D11& dst = (TextureD3D11&)dstTexture;
    const TextureD3D11& src = (TextureD3D11&)srcTexture;
    const TextureDesc& dstDesc = dst.GetDesc();
    const DxgiFormat& dstFormat = GetDxgiFormat(dstDesc.format);

    bool isWholeResource = (!dstRegion || dstRegion->mipOffset == NULL_TEXTURE_REGION_DESC) && (!srcRegion || srcRegion->mipOffset == NULL_TEXTURE_REGION_DESC);
    if (isWholeResource) {
        for (Dim_t layer = 0; layer < dstDesc.layerNum; layer++) {
            for (Dim_t mip = 0; mip < dstDesc.mipNum; mip++) {
                uint32_t subresource = dst.GetSubresourceIndex(layer, mip);
                m_DeferredContext->ResolveSubresource(dst, subresource, src, subresource, dstFormat.typed);
            }
        }
    } else {
        TextureRegionDesc wholeResource = {};
        if (!srcRegion || srcRegion->mipOffset == NULL_TEXTURE_REGION_DESC)
            srcRegion = &wholeResource;
        if (!dstRegion || dstRegion->mipOffset == NULL_TEXTURE_REGION_DESC)
            dstRegion = &wholeResource;

        uint32_t dstSubresource = dst.GetSubresourceIndex(dstRegion->layerOffset, dstRegion->mipOffset);
        uint32_t srcSubresource = src.GetSubresourceIndex(srcRegion->layerOffset, srcRegion->mipOffset);

        m_DeferredContext->ResolveSubresource(dst, dstSubresource, src, srcSubresource, dstFormat.typed);
    }
}

NRI_INLINE void CommandBufferD3D11::Dispatch(const DispatchDesc& dispatchDesc) {
    m_DeferredContext->Dispatch(dispatchDesc.x, dispatchDesc.y, dispatchDesc.z);
}

NRI_INLINE void CommandBufferD3D11::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    m_DeferredContext->DispatchIndirect((BufferD3D11&)buffer, (uint32_t)offset);
}

NRI_INLINE void CommandBufferD3D11::Barrier(const BarrierDesc& barrierDesc) {
    if (barrierDesc.textureNum == 0 && barrierDesc.bufferNum == 0)
        return;

    uint32_t flags = 0;

    for (uint32_t i = 0; i < barrierDesc.globalNum; i++) {
        const GlobalBarrierDesc& barrier = barrierDesc.globals[i];
        if ((barrier.before.access & AccessBits::SHADER_RESOURCE_STORAGE) && (barrier.after.access & AccessBits::SHADER_RESOURCE_STORAGE)) {
            bool isGraphics = barrier.before.stages == StageBits::ALL || (barrier.before.stages & (StageBits::GRAPHICS));
            bool isCompute = barrier.before.stages == StageBits::ALL || (barrier.before.stages & StageBits::COMPUTE_SHADER);

#if NRI_ENABLE_NVAPI
            if (isGraphics)
                flags |= NVAPI_D3D_BEGIN_UAV_OVERLAP_GFX_WFI;
            if (isCompute)
                flags |= NVAPI_D3D_BEGIN_UAV_OVERLAP_COMP_WFI;
#else
            if (isGraphics || isCompute)
                flags |= 1;
#endif
        }
    }

    for (uint32_t i = 0; i < barrierDesc.bufferNum; i++) {
        const BufferBarrierDesc& barrier = barrierDesc.buffers[i];
        if ((barrier.before.access & AccessBits::SHADER_RESOURCE_STORAGE) && (barrier.after.access & AccessBits::SHADER_RESOURCE_STORAGE)) {
            bool isGraphics = barrier.before.stages == StageBits::ALL || (barrier.before.stages & (StageBits::GRAPHICS));
            bool isCompute = barrier.before.stages == StageBits::ALL || (barrier.before.stages & StageBits::COMPUTE_SHADER);

#if NRI_ENABLE_NVAPI
            if (isGraphics)
                flags |= NVAPI_D3D_BEGIN_UAV_OVERLAP_GFX_WFI;
            if (isCompute)
                flags |= NVAPI_D3D_BEGIN_UAV_OVERLAP_COMP_WFI;
#else
            if (isGraphics || isCompute)
                flags |= 1;
#endif
        }
    }

    for (uint32_t i = 0; i < barrierDesc.textureNum; i++) {
        const TextureBarrierDesc& barrier = barrierDesc.textures[i];
        if ((barrier.before.access & AccessBits::SHADER_RESOURCE_STORAGE) && (barrier.after.access & AccessBits::SHADER_RESOURCE_STORAGE)) {
            bool isGraphics = barrier.before.stages == StageBits::ALL || (barrier.before.stages & (StageBits::GRAPHICS));
            bool isCompute = barrier.before.stages == StageBits::ALL || (barrier.before.stages & StageBits::COMPUTE_SHADER);

#if NRI_ENABLE_NVAPI
            if (isGraphics)
                flags |= NVAPI_D3D_BEGIN_UAV_OVERLAP_GFX_WFI;
            if (isCompute)
                flags |= NVAPI_D3D_BEGIN_UAV_OVERLAP_COMP_WFI;
#else
            if (isGraphics || isCompute)
                flags |= 1;
#endif
        }
    }

    if (flags) {
        if (m_Device.HasNvExt()) {
#if NRI_ENABLE_NVAPI
            NvAPI_Status res = NvAPI_D3D11_BeginUAVOverlapEx(m_DeferredContext, flags);
            RETURN_ON_FAILURE(&m_Device, res == NVAPI_OK, ReturnVoid(), "NvAPI_D3D11_BeginUAVOverlap() failed!");
#endif
        } else if (m_Device.HasAmdExt()) {
#if NRI_ENABLE_AMDAGS
            // TODO: verify that this code actually works on AMD!
            const AmdExtD3D11& amdExt = m_Device.GetAmdExt();
            AGSReturnCode res1 = amdExt.EndUAVOverlap(amdExt.context, m_DeferredContext);
            RETURN_ON_FAILURE(&m_Device, res1 == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_EndUAVOverlap() failed!");
            AGSReturnCode res2 = amdExt.BeginUAVOverlap(amdExt.context, m_DeferredContext);
            RETURN_ON_FAILURE(&m_Device, res2 == AGS_SUCCESS, ReturnVoid(), "agsDriverExtensionsDX11_BeginUAVOverlap() failed!");
#endif
        }
    }
}

NRI_INLINE void CommandBufferD3D11::BeginQuery(QueryPool& queryPool, uint32_t offset) {
    ((QueryPoolD3D11&)queryPool).BeginQuery(m_DeferredContext, offset);
}

NRI_INLINE void CommandBufferD3D11::EndQuery(QueryPool& queryPool, uint32_t offset) {
    ((QueryPoolD3D11&)queryPool).EndQuery(m_DeferredContext, offset);
}

NRI_INLINE void CommandBufferD3D11::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    ((BufferD3D11&)dstBuffer).AssignQueryPoolRange((QueryPoolD3D11*)&queryPool, offset, num, dstOffset);
}

NRI_INLINE void CommandBufferD3D11::BeginAnnotation(const char* name, uint32_t bgra) {
#if USE_ANNOTATION_INT
    if (m_Version >= 2)
        PIXBeginEvent(m_DeferredContext, bgra, name);
    else
#endif
        PIXBeginEvent(m_Annotation, bgra, name);
}

NRI_INLINE void CommandBufferD3D11::EndAnnotation() {
#if USE_ANNOTATION_INT
    if (m_Version >= 2)
        PIXEndEvent(m_DeferredContext);
    else
#endif
        PIXEndEvent(m_Annotation);
}

NRI_INLINE void CommandBufferD3D11::Annotation(const char* name, uint32_t bgra) {
#if USE_ANNOTATION_INT
    if (m_Version >= 2)
        PIXSetMarker(m_DeferredContext, bgra, name);
    else
#endif
        PIXSetMarker(m_Annotation, bgra, name);
}
