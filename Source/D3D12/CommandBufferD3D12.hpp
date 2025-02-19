// © 2021 NVIDIA Corporation

static uint8_t QueryLatestGraphicsCommandList(ComPtr<ID3D12GraphicsCommandListBest>& in, ComPtr<ID3D12GraphicsCommandListBest>& out) {
    static const IID versions[] = {
#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
        __uuidof(ID3D12GraphicsCommandList9),
        __uuidof(ID3D12GraphicsCommandList8),
        __uuidof(ID3D12GraphicsCommandList7),
#endif
        __uuidof(ID3D12GraphicsCommandList6),
        __uuidof(ID3D12GraphicsCommandList5),
        __uuidof(ID3D12GraphicsCommandList4),
        __uuidof(ID3D12GraphicsCommandList3),
        __uuidof(ID3D12GraphicsCommandList2),
        __uuidof(ID3D12GraphicsCommandList1),
        __uuidof(ID3D12GraphicsCommandList),
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

#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
static inline D3D12_BARRIER_SYNC GetBarrierSyncFlags(StageBits stageBits) {
    // Check non-mask values first
    if (stageBits == StageBits::ALL)
        return D3D12_BARRIER_SYNC_ALL;

    if (stageBits == StageBits::NONE)
        return D3D12_BARRIER_SYNC_NONE;

    // Gather bits
    D3D12_BARRIER_SYNC flags = D3D12_BARRIER_SYNC_NONE;

    if (stageBits & StageBits::INDEX_INPUT)
        flags |= D3D12_BARRIER_SYNC_INDEX_INPUT;

    if (stageBits & (StageBits::VERTEX_SHADER | StageBits::TESSELLATION_SHADERS | StageBits::GEOMETRY_SHADER | StageBits::MESH_SHADERS))
        flags |= D3D12_BARRIER_SYNC_VERTEX_SHADING;

    if (stageBits & StageBits::FRAGMENT_SHADER)
        flags |= D3D12_BARRIER_SYNC_PIXEL_SHADING;

    if (stageBits & StageBits::DEPTH_STENCIL_ATTACHMENT)
        flags |= D3D12_BARRIER_SYNC_DEPTH_STENCIL;

    if (stageBits & StageBits::COLOR_ATTACHMENT)
        flags |= D3D12_BARRIER_SYNC_RENDER_TARGET;

    if (stageBits & StageBits::COMPUTE_SHADER)
        flags |= D3D12_BARRIER_SYNC_COMPUTE_SHADING;

    if (stageBits & StageBits::RAY_TRACING_SHADERS)
        flags |= D3D12_BARRIER_SYNC_RAYTRACING;

    if (stageBits & StageBits::INDIRECT)
        flags |= D3D12_BARRIER_SYNC_EXECUTE_INDIRECT;

    if (stageBits & StageBits::COPY)
        flags |= D3D12_BARRIER_SYNC_COPY;

    if (stageBits & StageBits::RESOLVE)
        flags |= D3D12_BARRIER_SYNC_RESOLVE;

    if (stageBits & StageBits::CLEAR_STORAGE)
        flags |= D3D12_BARRIER_SYNC_CLEAR_UNORDERED_ACCESS_VIEW;

    if (stageBits & StageBits::ACCELERATION_STRUCTURE)
        flags |= D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE | D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE;

    // TODO: D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO?

    return flags;
}

static inline D3D12_BARRIER_ACCESS GetBarrierAccessFlags(AccessBits accessBits) {
    if (accessBits == AccessBits::UNKNOWN)
        return D3D12_BARRIER_ACCESS_NO_ACCESS;

    D3D12_BARRIER_ACCESS flags = D3D12_BARRIER_ACCESS_COMMON; // = 0

    if (accessBits & AccessBits::VERTEX_BUFFER)
        flags |= D3D12_BARRIER_ACCESS_VERTEX_BUFFER;

    if (accessBits & AccessBits::INDEX_BUFFER)
        flags |= D3D12_BARRIER_ACCESS_INDEX_BUFFER;

    if (accessBits & AccessBits::CONSTANT_BUFFER)
        flags |= D3D12_BARRIER_ACCESS_CONSTANT_BUFFER;

    if (accessBits & AccessBits::ARGUMENT_BUFFER)
        flags |= D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;

    if (accessBits & AccessBits::SHADER_RESOURCE)
        flags |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;

    if (accessBits & AccessBits::SHADER_RESOURCE_STORAGE)
        flags |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;

    if (accessBits & AccessBits::COLOR_ATTACHMENT)
        flags |= D3D12_BARRIER_ACCESS_RENDER_TARGET;

    if (accessBits & AccessBits::DEPTH_STENCIL_ATTACHMENT_WRITE)
        flags |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;

    if (accessBits & AccessBits::DEPTH_STENCIL_ATTACHMENT_READ)
        flags |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;

    if (accessBits & AccessBits::COPY_SOURCE)
        flags |= D3D12_BARRIER_ACCESS_COPY_SOURCE;

    if (accessBits & AccessBits::COPY_DESTINATION)
        flags |= D3D12_BARRIER_ACCESS_COPY_DEST;

    if (accessBits & AccessBits::RESOLVE_SOURCE)
        flags |= D3D12_BARRIER_ACCESS_RESOLVE_SOURCE;

    if (accessBits & AccessBits::RESOLVE_DESTINATION)
        flags |= D3D12_BARRIER_ACCESS_RESOLVE_DEST;

    if (accessBits & AccessBits::ACCELERATION_STRUCTURE_READ)
        flags |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;

    if (accessBits & AccessBits::ACCELERATION_STRUCTURE_WRITE)
        flags |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;

    if (accessBits & AccessBits::SHADING_RATE_ATTACHMENT)
        flags |= D3D12_BARRIER_ACCESS_SHADING_RATE_SOURCE;

    return flags;
}

constexpr std::array<D3D12_BARRIER_LAYOUT, (size_t)Layout::MAX_NUM> LAYOUTS = {
    D3D12_BARRIER_LAYOUT_UNDEFINED,           // UNKNOWN
    D3D12_BARRIER_LAYOUT_RENDER_TARGET,       // COLOR_ATTACHMENT
    D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE, // DEPTH_STENCIL_ATTACHMENT
    D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ,  // DEPTH_STENCIL_READONLY
    D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,     // SHADER_RESOURCE
    D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,    // SHADER_RESOURCE_STORAGE
    D3D12_BARRIER_LAYOUT_COPY_SOURCE,         // COPY_SOURCE
    D3D12_BARRIER_LAYOUT_COPY_DEST,           // COPY_DESTINATION
    D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE,      // RESOLVE_SOURCE
    D3D12_BARRIER_LAYOUT_RESOLVE_DEST,        // RESOLVE_DESTINATION
    D3D12_BARRIER_LAYOUT_PRESENT,             // PRESENT
    D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE, // SHADING_RATE_ATTACHMENT
};

static inline D3D12_BARRIER_LAYOUT GetBarrierLayout(Layout layout) {
    return LAYOUTS[(uint32_t)layout];
}
#endif

static inline D3D12_RESOURCE_STATES GetResourceStates(AccessBits accessMask, D3D12_COMMAND_LIST_TYPE commandListType) {
    D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_COMMON;

    if (accessMask & (AccessBits::CONSTANT_BUFFER | AccessBits::VERTEX_BUFFER))
        resourceStates |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    if (accessMask & AccessBits::INDEX_BUFFER)
        resourceStates |= D3D12_RESOURCE_STATE_INDEX_BUFFER;

    if (accessMask & AccessBits::ARGUMENT_BUFFER)
        resourceStates |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;

    if (accessMask & AccessBits::SHADER_RESOURCE_STORAGE)
        resourceStates |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    if (accessMask & AccessBits::COLOR_ATTACHMENT)
        resourceStates |= D3D12_RESOURCE_STATE_RENDER_TARGET;

    if (accessMask & AccessBits::DEPTH_STENCIL_ATTACHMENT_READ)
        resourceStates |= D3D12_RESOURCE_STATE_DEPTH_READ;

    if (accessMask & AccessBits::DEPTH_STENCIL_ATTACHMENT_WRITE)
        resourceStates |= D3D12_RESOURCE_STATE_DEPTH_WRITE;

    if (accessMask & AccessBits::COPY_SOURCE)
        resourceStates |= D3D12_RESOURCE_STATE_COPY_SOURCE;

    if (accessMask & AccessBits::COPY_DESTINATION)
        resourceStates |= D3D12_RESOURCE_STATE_COPY_DEST;

    if (accessMask & AccessBits::RESOLVE_SOURCE)
        resourceStates |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;

    if (accessMask & AccessBits::RESOLVE_DESTINATION)
        resourceStates |= D3D12_RESOURCE_STATE_RESOLVE_DEST;

    if (accessMask & AccessBits::SHADER_RESOURCE) {
        resourceStates |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if (commandListType == D3D12_COMMAND_LIST_TYPE_DIRECT)
            resourceStates |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    if (accessMask & AccessBits::ACCELERATION_STRUCTURE_READ)
        resourceStates |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

    if (accessMask & AccessBits::ACCELERATION_STRUCTURE_WRITE)
        resourceStates |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    if (accessMask & AccessBits::SHADING_RATE_ATTACHMENT)
        resourceStates |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;

    return resourceStates;
}

static void AddResourceBarrier(D3D12_COMMAND_LIST_TYPE commandListType, ID3D12Resource* resource, AccessBits before, AccessBits after, D3D12_RESOURCE_BARRIER& resourceBarrier, uint32_t subresource) {
    D3D12_RESOURCE_STATES resourceStateBefore = GetResourceStates(before, commandListType);
    D3D12_RESOURCE_STATES resourceStateAfter = GetResourceStates(after, commandListType);

    if (resourceStateBefore == resourceStateAfter && resourceStateBefore == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
        resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        resourceBarrier.UAV.pResource = resource;
    } else {
        resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resourceBarrier.Transition.pResource = resource;
        resourceBarrier.Transition.StateBefore = resourceStateBefore;
        resourceBarrier.Transition.StateAfter = resourceStateAfter;
        resourceBarrier.Transition.Subresource = subresource;
    }
}

static inline void ConvertRects(const Rect* in, uint32_t rectNum, D3D12_RECT* out) {
    for (uint32_t i = 0; i < rectNum; i++) {
        out[i].left = in[i].x;
        out[i].top = in[i].y;
        out[i].right = in[i].x + in[i].width;
        out[i].bottom = in[i].y + in[i].height;
    }
}

Result CommandBufferD3D12::Create(D3D12_COMMAND_LIST_TYPE commandListType, ID3D12CommandAllocator* commandAllocator) {
    ComPtr<ID3D12GraphicsCommandListBest> graphicsCommandList;
    HRESULT hr = m_Device->CreateCommandList(NRI_NODE_MASK, commandListType, commandAllocator, nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&graphicsCommandList);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommandList()");

    m_Version = QueryLatestGraphicsCommandList(graphicsCommandList, m_GraphicsCommandList);

    hr = m_GraphicsCommandList->Close();
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12GraphicsCommandList::Close()");

    m_CommandAllocator = commandAllocator;

    return Result::SUCCESS;
}

Result CommandBufferD3D12::Create(const CommandBufferD3D12Desc& commandBufferDesc) {
    ComPtr<ID3D12GraphicsCommandListBest> graphicsCommandList = (ID3D12GraphicsCommandListBest*)commandBufferDesc.d3d12CommandList;
    m_Version = QueryLatestGraphicsCommandList(graphicsCommandList, m_GraphicsCommandList);

    // TODO: what if opened?

    m_CommandAllocator = commandBufferDesc.d3d12CommandAllocator;

    return Result::SUCCESS;
}

NRI_INLINE Result CommandBufferD3D12::Begin(const DescriptorPool* descriptorPool) {
    HRESULT hr = m_GraphicsCommandList->Reset(m_CommandAllocator, nullptr);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12GraphicsCommandList::Reset()");

    if (descriptorPool)
        SetDescriptorPool(*descriptorPool);

    m_PipelineLayout = nullptr;
    m_IsGraphicsPipelineLayout = false;
    m_Pipeline = nullptr;
    m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

    ResetAttachments();

    return Result::SUCCESS;
}

NRI_INLINE Result CommandBufferD3D12::End() {
    if (FAILED(m_GraphicsCommandList->Close()))
        return Result::FAILURE;

    return Result::SUCCESS;
}

NRI_INLINE void CommandBufferD3D12::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    Scratch<D3D12_VIEWPORT> d3dViewports = AllocateScratch(m_Device, D3D12_VIEWPORT, viewportNum);
    for (uint32_t i = 0; i < viewportNum; i++) {
        const Viewport& in = viewports[i];
        D3D12_VIEWPORT& out = d3dViewports[i];
        out.TopLeftX = in.x;
        out.TopLeftY = in.y;
        out.Width = in.width;
        out.Height = in.height;
        out.MinDepth = in.depthMin;
        out.MaxDepth = in.depthMax;

        // Origin bottom-left requires flipping
        if (in.originBottomLeft) {
            out.TopLeftY += in.height;
            out.Height = -in.height;
        }
    }

    m_GraphicsCommandList->RSSetViewports(viewportNum, d3dViewports);
}

NRI_INLINE void CommandBufferD3D12::SetScissors(const Rect* rects, uint32_t rectNum) {
    Scratch<D3D12_RECT> d3dRects = AllocateScratch(m_Device, D3D12_RECT, rectNum);
    ConvertRects(rects, rectNum, d3dRects);

    m_GraphicsCommandList->RSSetScissorRects(rectNum, d3dRects);
}

NRI_INLINE void CommandBufferD3D12::SetDepthBounds(float boundsMin, float boundsMax) {
    if (m_Version >= 1)
        m_GraphicsCommandList->OMSetDepthBounds(boundsMin, boundsMax);
}

NRI_INLINE void CommandBufferD3D12::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    MaybeUnused(backRef);
#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
    if (m_Device.GetDesc().isIndependentFrontAndBackStencilReferenceAndMasksSupported)
        m_GraphicsCommandList->OMSetFrontAndBackStencilRef(frontRef, backRef);
    else
#endif
        m_GraphicsCommandList->OMSetStencilRef(frontRef);
}

NRI_INLINE void CommandBufferD3D12::SetSampleLocations(const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    static_assert(sizeof(D3D12_SAMPLE_POSITION) == sizeof(SampleLocation));

    uint32_t pixelNum = locationNum / sampleNum;
    if (m_Version >= 1)
        m_GraphicsCommandList->SetSamplePositions(sampleNum, pixelNum, (D3D12_SAMPLE_POSITION*)locations);
}

NRI_INLINE void CommandBufferD3D12::SetBlendConstants(const Color32f& color) {
    m_GraphicsCommandList->OMSetBlendFactor(&color.x);
}

NRI_INLINE void CommandBufferD3D12::SetShadingRate(const ShadingRateDesc& shadingRateDesc) {
    D3D12_SHADING_RATE shadingRate = GetShadingRate(shadingRateDesc.shadingRate);
    D3D12_SHADING_RATE_COMBINER shadingRateCombiners[2] = {
        GetShadingRateCombiner(shadingRateDesc.primitiveCombiner),
        GetShadingRateCombiner(shadingRateDesc.attachmentCombiner),
    };

    m_GraphicsCommandList->RSSetShadingRate(shadingRate, shadingRateCombiners);
}

NRI_INLINE void CommandBufferD3D12::SetDepthBias(const DepthBiasDesc& depthBiasDesc) {
    MaybeUnused(depthBiasDesc);
#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
    m_GraphicsCommandList->RSSetDepthBias(depthBiasDesc.constant, depthBiasDesc.clamp, depthBiasDesc.slope);
#endif
}

NRI_INLINE void CommandBufferD3D12::ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    if (!clearDescNum)
        return;

    Scratch<D3D12_RECT> d3dRects = AllocateScratch(m_Device, D3D12_RECT, rectNum);
    ConvertRects(rects, rectNum, d3dRects);

    for (uint32_t i = 0; i < clearDescNum; i++) {
        if (clearDescs[i].planes & PlaneBits::COLOR)
            m_GraphicsCommandList->ClearRenderTargetView(m_RenderTargets[clearDescs[i].colorAttachmentIndex], &clearDescs[i].value.color.f.x, rectNum, d3dRects);
        else {
            D3D12_CLEAR_FLAGS clearFlags = (D3D12_CLEAR_FLAGS)0;
            if (clearDescs[i].planes & PlaneBits::DEPTH)
                clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
            if (clearDescs[i].planes & PlaneBits::STENCIL)
                clearFlags |= D3D12_CLEAR_FLAG_STENCIL;

            m_GraphicsCommandList->ClearDepthStencilView(m_DepthStencil, clearFlags, clearDescs[i].value.depthStencil.depth, clearDescs[i].value.depthStencil.stencil, rectNum, d3dRects);
        }
    }
}

NRI_INLINE void CommandBufferD3D12::ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc) {
    DescriptorSetD3D12* descriptorSet = m_DescriptorSets[clearDesc.setIndex];
    DescriptorD3D12* resourceView = (DescriptorD3D12*)clearDesc.storageBuffer;
    const UINT clearValues[4] = {clearDesc.value, clearDesc.value, clearDesc.value, clearDesc.value};

    m_GraphicsCommandList->ClearUnorderedAccessViewUint({descriptorSet->GetPointerGPU(clearDesc.rangeIndex, clearDesc.descriptorIndex)}, {resourceView->GetPointerCPU()}, *resourceView, clearValues, 0, nullptr);
}

NRI_INLINE void CommandBufferD3D12::ClearStorageTexture(const ClearStorageTextureDesc& clearDesc) {
    DescriptorSetD3D12* descriptorSet = m_DescriptorSets[clearDesc.setIndex];
    DescriptorD3D12* resourceView = (DescriptorD3D12*)clearDesc.storageTexture;

    if (resourceView->IsIntegerFormat())
        m_GraphicsCommandList->ClearUnorderedAccessViewUint({descriptorSet->GetPointerGPU(clearDesc.rangeIndex, clearDesc.descriptorIndex)}, {resourceView->GetPointerCPU()}, *resourceView, &clearDesc.value.color.ui.x, 0, nullptr);
    else
        m_GraphicsCommandList->ClearUnorderedAccessViewFloat({descriptorSet->GetPointerGPU(clearDesc.rangeIndex, clearDesc.descriptorIndex)}, {resourceView->GetPointerCPU()}, *resourceView, &clearDesc.value.color.f.x, 0, nullptr);
}

NRI_INLINE void CommandBufferD3D12::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    // Render targets
    m_RenderTargetNum = attachmentsDesc.colors ? attachmentsDesc.colorNum : 0;

    uint32_t i = 0;
    for (; i < m_RenderTargetNum; i++) {
        const DescriptorD3D12& descriptor = *(DescriptorD3D12*)attachmentsDesc.colors[i];
        m_RenderTargets[i].ptr = descriptor.GetPointerCPU();
    }
    for (; i < (uint32_t)m_RenderTargets.size(); i++)
        m_RenderTargets[i].ptr = NULL;

    if (attachmentsDesc.depthStencil) {
        const DescriptorD3D12& descriptor = *(DescriptorD3D12*)attachmentsDesc.depthStencil;
        m_DepthStencil.ptr = descriptor.GetPointerCPU();
    } else
        m_DepthStencil.ptr = NULL;

    m_GraphicsCommandList->OMSetRenderTargets(m_RenderTargetNum, m_RenderTargets.data(), FALSE, m_DepthStencil.ptr ? &m_DepthStencil : nullptr);

    // Shading rate
    if (m_Device.GetDesc().shadingRateTier >= 2) {
        ID3D12Resource* shadingRateImage = nullptr;
        if (attachmentsDesc.shadingRate)
            shadingRateImage = *(DescriptorD3D12*)attachmentsDesc.shadingRate;

        m_GraphicsCommandList->RSSetShadingRateImage(shadingRateImage);
    }

    // Multiview
    if (m_Device.GetDesc().viewMaxNum > 1 && attachmentsDesc.viewMask)
        m_GraphicsCommandList->SetViewInstanceMask(attachmentsDesc.viewMask);
}

NRI_INLINE void CommandBufferD3D12::SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    Scratch<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews = AllocateScratch(m_Device, D3D12_VERTEX_BUFFER_VIEW, bufferNum);
    for (uint32_t i = 0; i < bufferNum; i++) {
        if (buffers[i]) {
            const BufferD3D12* buffer = (BufferD3D12*)buffers[i];
            uint64_t offset = offsets ? offsets[i] : 0;
            vertexBufferViews[i].BufferLocation = buffer->GetPointerGPU() + offset;
            vertexBufferViews[i].SizeInBytes = (UINT)(buffer->GetDesc().size - offset);
            vertexBufferViews[i].StrideInBytes = m_Pipeline->GetVertexStreamStride(baseSlot + i);
        } else {
            vertexBufferViews[i].BufferLocation = 0;
            vertexBufferViews[i].SizeInBytes = 0;
            vertexBufferViews[i].StrideInBytes = 0;
        }
    }

    m_GraphicsCommandList->IASetVertexBuffers(baseSlot, bufferNum, vertexBufferViews);
}

NRI_INLINE void CommandBufferD3D12::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    const BufferD3D12& bufferD3D12 = (BufferD3D12&)buffer;

    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    indexBufferView.BufferLocation = bufferD3D12.GetPointerGPU() + offset;
    indexBufferView.SizeInBytes = (UINT)(bufferD3D12.GetDesc().size - offset);
    indexBufferView.Format = indexType == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

    m_GraphicsCommandList->IASetIndexBuffer(&indexBufferView);
}

NRI_INLINE void CommandBufferD3D12::SetPipelineLayout(const PipelineLayout& pipelineLayout) {
    const PipelineLayoutD3D12& pipelineLayoutD3D12 = (const PipelineLayoutD3D12&)pipelineLayout;

    m_PipelineLayout = &pipelineLayoutD3D12;
    m_IsGraphicsPipelineLayout = pipelineLayoutD3D12.IsGraphicsPipelineLayout();

    if (m_IsGraphicsPipelineLayout)
        m_GraphicsCommandList->SetGraphicsRootSignature(pipelineLayoutD3D12);
    else
        m_GraphicsCommandList->SetComputeRootSignature(pipelineLayoutD3D12);
}

NRI_INLINE void CommandBufferD3D12::SetPipeline(const Pipeline& pipeline) {
    PipelineD3D12* pipelineD3D12 = (PipelineD3D12*)&pipeline;
    pipelineD3D12->Bind(m_GraphicsCommandList, m_PrimitiveTopology);

    m_Pipeline = pipelineD3D12;
}

NRI_INLINE void CommandBufferD3D12::SetDescriptorPool(const DescriptorPool& descriptorPool) {
    ((DescriptorPoolD3D12&)descriptorPool).Bind(m_GraphicsCommandList);
}

NRI_INLINE void CommandBufferD3D12::SetDescriptorSet(uint32_t setIndex, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    m_PipelineLayout->SetDescriptorSet(*m_GraphicsCommandList, m_IsGraphicsPipelineLayout, setIndex, descriptorSet, dynamicConstantBufferOffsets);
    m_DescriptorSets[setIndex] = (DescriptorSetD3D12*)&descriptorSet;
}

NRI_INLINE void CommandBufferD3D12::SetRootConstants(uint32_t rootConstantIndex, const void* data, uint32_t size) {
    uint32_t rootParameterIndex = m_PipelineLayout->GetBaseRootConstant() + rootConstantIndex;
    uint32_t rootConstantNum = size / 4;

    if (m_IsGraphicsPipelineLayout)
        m_GraphicsCommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, rootConstantNum, data, 0);
    else
        m_GraphicsCommandList->SetComputeRoot32BitConstants(rootParameterIndex, rootConstantNum, data, 0);
}

NRI_INLINE void CommandBufferD3D12::SetRootDescriptor(uint32_t rootDescriptorIndex, Descriptor& descriptor) {
    uint32_t rootParameterIndex = m_PipelineLayout->GetBaseRootDescriptor() + rootDescriptorIndex;
    DescriptorD3D12& descriptorD3D12 = (DescriptorD3D12&)descriptor;
    D3D12_GPU_VIRTUAL_ADDRESS bufferLocation = descriptorD3D12.GetPointerGPU();

    switch (descriptorD3D12.GetBufferViewType()) {
        case BufferViewType::SHADER_RESOURCE:
            if (m_IsGraphicsPipelineLayout)
                m_GraphicsCommandList->SetGraphicsRootShaderResourceView(rootParameterIndex, bufferLocation);
            else
                m_GraphicsCommandList->SetComputeRootShaderResourceView(rootParameterIndex, bufferLocation);
            break;
        case BufferViewType::SHADER_RESOURCE_STORAGE:
            if (m_IsGraphicsPipelineLayout)
                m_GraphicsCommandList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, bufferLocation);
            else
                m_GraphicsCommandList->SetComputeRootUnorderedAccessView(rootParameterIndex, bufferLocation);
            break;
        case BufferViewType::CONSTANT:
            if (m_IsGraphicsPipelineLayout)
                m_GraphicsCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, bufferLocation);
            else
                m_GraphicsCommandList->SetComputeRootConstantBufferView(rootParameterIndex, bufferLocation);
            break;
    }
}

NRI_INLINE void CommandBufferD3D12::Draw(const DrawDesc& drawDesc) {
    if (m_PipelineLayout && m_PipelineLayout->IsDrawParametersEmulationEnabled()) {
        struct BaseVertexInstance {
            uint32_t baseVertex;
            uint32_t baseInstance;
        } baseVertexInstance = {drawDesc.baseVertex, drawDesc.baseInstance};

        m_GraphicsCommandList->SetGraphicsRoot32BitConstants(0, 2, &baseVertexInstance, 0);
    }

    m_GraphicsCommandList->DrawInstanced(drawDesc.vertexNum, drawDesc.instanceNum, drawDesc.baseVertex, drawDesc.baseInstance);
}

NRI_INLINE void CommandBufferD3D12::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    if (m_PipelineLayout && m_PipelineLayout->IsDrawParametersEmulationEnabled()) {
        struct BaseVertexInstance {
            int32_t baseVertex;
            uint32_t baseInstance;
        } baseVertexInstance = {drawIndexedDesc.baseVertex, drawIndexedDesc.baseInstance};

        m_GraphicsCommandList->SetGraphicsRoot32BitConstants(0, 2, &baseVertexInstance, 0);
    }

    m_GraphicsCommandList->DrawIndexedInstanced(drawIndexedDesc.indexNum, drawIndexedDesc.instanceNum, drawIndexedDesc.baseIndex, drawIndexedDesc.baseVertex, drawIndexedDesc.baseInstance);
}

NRI_INLINE void CommandBufferD3D12::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ID3D12Resource* pCountBuffer = nullptr;
    if (countBuffer)
        pCountBuffer = *(BufferD3D12*)countBuffer;

    m_GraphicsCommandList->ExecuteIndirect(m_Device.GetDrawCommandSignature(stride, *m_PipelineLayout), drawNum, (BufferD3D12&)buffer, offset, pCountBuffer, countBufferOffset);
}

NRI_INLINE void CommandBufferD3D12::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ID3D12Resource* pCountBuffer = nullptr;
    if (countBuffer)
        pCountBuffer = *(BufferD3D12*)countBuffer;

    m_GraphicsCommandList->ExecuteIndirect(m_Device.GetDrawIndexedCommandSignature(stride, *m_PipelineLayout), drawNum, (BufferD3D12&)buffer, offset, pCountBuffer, countBufferOffset);
}

NRI_INLINE void CommandBufferD3D12::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    if (size == WHOLE_SIZE)
        size = ((BufferD3D12&)srcBuffer).GetDesc().size;

    m_GraphicsCommandList->CopyBufferRegion((BufferD3D12&)dstBuffer, dstOffset, (BufferD3D12&)srcBuffer, srcOffset, size);
}

NRI_INLINE void CommandBufferD3D12::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    const TextureD3D12& dst = (TextureD3D12&)dstTexture;
    const TextureD3D12& src = (TextureD3D12&)srcTexture;

    bool isWholeResource = !dstRegionDesc && !srcRegionDesc;
    if (isWholeResource)
        m_GraphicsCommandList->CopyResource(dst, src);
    else {
        TextureRegionDesc wholeResource = {};
        if (!srcRegionDesc)
            srcRegionDesc = &wholeResource;
        if (!dstRegionDesc)
            dstRegionDesc = &wholeResource;

        D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation = {
            dst,
            D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            dst.GetSubresourceIndex(dstRegionDesc->layerOffset, dstRegionDesc->mipOffset),
        };

        D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation = {
            src,
            D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            src.GetSubresourceIndex(srcRegionDesc->layerOffset, srcRegionDesc->mipOffset),
        };

        const uint32_t size[3] = {
            srcRegionDesc->width == WHOLE_SIZE ? src.GetSize(0, srcRegionDesc->mipOffset) : srcRegionDesc->width,
            srcRegionDesc->height == WHOLE_SIZE ? src.GetSize(1, srcRegionDesc->mipOffset) : srcRegionDesc->height,
            srcRegionDesc->depth == WHOLE_SIZE ? src.GetSize(2, srcRegionDesc->mipOffset) : srcRegionDesc->depth,
        };

        D3D12_BOX box = {
            srcRegionDesc->x,
            srcRegionDesc->y,
            srcRegionDesc->z,
            srcRegionDesc->x + size[0],
            srcRegionDesc->y + size[1],
            srcRegionDesc->z + size[2],
        };

        m_GraphicsCommandList->CopyTextureRegion(&dstTextureCopyLocation, dstRegionDesc->x, dstRegionDesc->y, dstRegionDesc->z, &srcTextureCopyLocation, &box);
    }
}

NRI_INLINE void CommandBufferD3D12::ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    const TextureD3D12& dst = (TextureD3D12&)dstTexture;
    const TextureD3D12& src = (TextureD3D12&)srcTexture;
    const TextureDesc& dstDesc = dst.GetDesc();
    const DxgiFormat& dstFormat = GetDxgiFormat(dstDesc.format);

    bool isWholeResource = !dstRegionDesc && !srcRegionDesc;
    if (isWholeResource || m_Version < 1) {
        for (Dim_t layer = 0; layer < dstDesc.layerNum; layer++) {
            for (Mip_t mip = 0; mip < dstDesc.mipNum; mip++) {
                uint32_t subresource = dst.GetSubresourceIndex(layer, mip);
                m_GraphicsCommandList->ResolveSubresource(dst, subresource, src, subresource, dstFormat.typed);
            }
        }
    } else {
        TextureRegionDesc wholeResource = {};
        if (!srcRegionDesc)
            srcRegionDesc = &wholeResource;
        if (!dstRegionDesc)
            dstRegionDesc = &wholeResource;

        uint32_t dstSubresource = dst.GetSubresourceIndex(dstRegionDesc->layerOffset, dstRegionDesc->mipOffset);
        uint32_t srcSubresource = src.GetSubresourceIndex(srcRegionDesc->layerOffset, srcRegionDesc->mipOffset);

        D3D12_RECT srcRect = {
            srcRegionDesc->x,
            srcRegionDesc->y,
            srcRegionDesc->width,
            srcRegionDesc->height,
        };

        m_GraphicsCommandList->ResolveSubresourceRegion(dst, dstSubresource, dstRegionDesc->x, dstRegionDesc->y, src, srcSubresource, &srcRect, dstFormat.typed, D3D12_RESOLVE_MODE_AVERAGE);
    }
}

NRI_INLINE void CommandBufferD3D12::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    const TextureD3D12& dst = (TextureD3D12&)dstTexture;
    const TextureDesc& dstDesc = dst.GetDesc();

    D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation = {
        dst,
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        dst.GetSubresourceIndex(dstRegionDesc.layerOffset, dstRegionDesc.mipOffset),
    };

    const uint32_t size[3] = {
        dstRegionDesc.width == WHOLE_SIZE ? dst.GetSize(0, dstRegionDesc.mipOffset) : dstRegionDesc.width,
        dstRegionDesc.height == WHOLE_SIZE ? dst.GetSize(1, dstRegionDesc.mipOffset) : dstRegionDesc.height,
        dstRegionDesc.depth == WHOLE_SIZE ? dst.GetSize(2, dstRegionDesc.mipOffset) : dstRegionDesc.depth,
    };

    D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation = {};
    srcTextureCopyLocation.pResource = (BufferD3D12&)srcBuffer;
    srcTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcTextureCopyLocation.PlacedFootprint.Offset = srcDataLayoutDesc.offset;
    srcTextureCopyLocation.PlacedFootprint.Footprint.Format = GetDxgiFormat(dstDesc.format).typeless;
    srcTextureCopyLocation.PlacedFootprint.Footprint.Width = size[0];
    srcTextureCopyLocation.PlacedFootprint.Footprint.Height = size[1];
    srcTextureCopyLocation.PlacedFootprint.Footprint.Depth = size[2];
    srcTextureCopyLocation.PlacedFootprint.Footprint.RowPitch = srcDataLayoutDesc.rowPitch;

    D3D12_BOX box = {
        dstRegionDesc.x,
        dstRegionDesc.y,
        dstRegionDesc.z,
        dstRegionDesc.x + size[0],
        dstRegionDesc.y + size[1],
        dstRegionDesc.z + size[2],
    };

    m_GraphicsCommandList->CopyTextureRegion(&dstTextureCopyLocation, dstRegionDesc.x, dstRegionDesc.y, dstRegionDesc.z, &srcTextureCopyLocation, &box);
}

NRI_INLINE void CommandBufferD3D12::ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    const TextureD3D12& src = (TextureD3D12&)srcTexture;
    const TextureDesc& srcDesc = src.GetDesc();

    D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation = {
        src,
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        src.GetSubresourceIndex(srcRegionDesc.layerOffset, srcRegionDesc.mipOffset),
    };

    D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation = {};
    dstTextureCopyLocation.pResource = (BufferD3D12&)dstBuffer;
    dstTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstTextureCopyLocation.PlacedFootprint.Offset = dstDataLayoutDesc.offset;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Format = GetDxgiFormat(srcDesc.format).typeless;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Width = srcRegionDesc.width;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Height = srcRegionDesc.height;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Depth = srcRegionDesc.depth;
    dstTextureCopyLocation.PlacedFootprint.Footprint.RowPitch = dstDataLayoutDesc.rowPitch;

    const uint32_t size[3] = {
        srcRegionDesc.width == WHOLE_SIZE ? src.GetSize(0, srcRegionDesc.mipOffset) : srcRegionDesc.width,
        srcRegionDesc.height == WHOLE_SIZE ? src.GetSize(1, srcRegionDesc.mipOffset) : srcRegionDesc.height,
        srcRegionDesc.depth == WHOLE_SIZE ? src.GetSize(2, srcRegionDesc.mipOffset) : srcRegionDesc.depth,
    };

    D3D12_BOX box = {
        srcRegionDesc.x,
        srcRegionDesc.y,
        srcRegionDesc.z,
        srcRegionDesc.x + size[0],
        srcRegionDesc.y + size[1],
        srcRegionDesc.z + size[2],
    };

    m_GraphicsCommandList->CopyTextureRegion(&dstTextureCopyLocation, 0, 0, 0, &srcTextureCopyLocation, &box);
}

NRI_INLINE void CommandBufferD3D12::Dispatch(const DispatchDesc& dispatchDesc) {
    m_GraphicsCommandList->Dispatch(dispatchDesc.x, dispatchDesc.y, dispatchDesc.z);
}

NRI_INLINE void CommandBufferD3D12::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    static_assert(sizeof(DispatchDesc) == sizeof(D3D12_DISPATCH_ARGUMENTS));

    m_GraphicsCommandList->ExecuteIndirect(m_Device.GetDispatchCommandSignature(), 1, (BufferD3D12&)buffer, offset, nullptr, 0);
}

NRI_INLINE void CommandBufferD3D12::Barrier(const BarrierGroupDesc& barrierGroupDesc) {
#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
    if (m_Device.GetDesc().isEnchancedBarrierSupported) { // Enhanced barriers
        // Count
        uint32_t barrierNum = barrierGroupDesc.globalNum + barrierGroupDesc.bufferNum + barrierGroupDesc.textureNum;
        if (!barrierNum)
            return;

        D3D12_BARRIER_GROUP barrierGroups[3] = {};
        uint32_t barriersGroupsNum = 0;

        // Global
        Scratch<D3D12_GLOBAL_BARRIER> globalBarriers = AllocateScratch(m_Device, D3D12_GLOBAL_BARRIER, barrierGroupDesc.globalNum);
        if (barrierGroupDesc.globalNum) {
            D3D12_BARRIER_GROUP* barrierGroup = &barrierGroups[barriersGroupsNum++];
            barrierGroup->Type = D3D12_BARRIER_TYPE_GLOBAL;
            barrierGroup->NumBarriers = barrierGroupDesc.globalNum;
            barrierGroup->pGlobalBarriers = globalBarriers;

            for (uint32_t i = 0; i < barrierGroupDesc.globalNum; i++) {
                const GlobalBarrierDesc& in = barrierGroupDesc.globals[i];

                D3D12_GLOBAL_BARRIER& out = globalBarriers[i];
                out = {};
                out.SyncBefore = GetBarrierSyncFlags(in.before.stages);
                out.SyncAfter = GetBarrierSyncFlags(in.after.stages);
                out.AccessBefore = GetBarrierAccessFlags(in.before.access);
                out.AccessAfter = GetBarrierAccessFlags(in.after.access);
            }
        }

        // Buffer
        Scratch<D3D12_BUFFER_BARRIER> bufferBarriers = AllocateScratch(m_Device, D3D12_BUFFER_BARRIER, barrierGroupDesc.bufferNum);
        if (barrierGroupDesc.bufferNum) {
            D3D12_BARRIER_GROUP* barrierGroup = &barrierGroups[barriersGroupsNum++];
            barrierGroup->Type = D3D12_BARRIER_TYPE_BUFFER;
            barrierGroup->NumBarriers = barrierGroupDesc.bufferNum;
            barrierGroup->pBufferBarriers = bufferBarriers;

            for (uint32_t i = 0; i < barrierGroupDesc.bufferNum; i++) {
                const BufferBarrierDesc& in = barrierGroupDesc.buffers[i];
                const BufferD3D12& buffer = *(BufferD3D12*)in.buffer;

                D3D12_BUFFER_BARRIER& out = bufferBarriers[i];
                out = {};
                out.SyncBefore = GetBarrierSyncFlags(in.before.stages);
                out.SyncAfter = GetBarrierSyncFlags(in.after.stages);
                out.AccessBefore = GetBarrierAccessFlags(in.before.access);
                out.AccessAfter = GetBarrierAccessFlags(in.after.access);
                out.pResource = buffer;
                out.Offset = 0;
                out.Size = UINT64_MAX;
            }
        }

        // Texture
        Scratch<D3D12_TEXTURE_BARRIER> textureBarriers = AllocateScratch(m_Device, D3D12_TEXTURE_BARRIER, barrierGroupDesc.textureNum);
        if (barrierGroupDesc.textureNum) {
            D3D12_BARRIER_GROUP* barrierGroup = &barrierGroups[barriersGroupsNum++];
            barrierGroup->Type = D3D12_BARRIER_TYPE_TEXTURE;
            barrierGroup->NumBarriers = barrierGroupDesc.textureNum;
            barrierGroup->pTextureBarriers = textureBarriers;

            for (uint32_t i = 0; i < barrierGroupDesc.textureNum; i++) {
                const TextureBarrierDesc& in = barrierGroupDesc.textures[i];
                const TextureD3D12& texture = *(TextureD3D12*)in.texture;
                const TextureDesc& desc = texture.GetDesc();

                D3D12_TEXTURE_BARRIER& out = textureBarriers[i];
                out = {};
                out.SyncBefore = GetBarrierSyncFlags(in.before.stages);
                out.SyncAfter = GetBarrierSyncFlags(in.after.stages);
                out.AccessBefore = in.before.layout == Layout::PRESENT ? D3D12_BARRIER_ACCESS_COMMON : GetBarrierAccessFlags(in.before.access);
                out.AccessAfter = in.after.layout == Layout::PRESENT ? D3D12_BARRIER_ACCESS_COMMON : GetBarrierAccessFlags(in.after.access);
                out.LayoutBefore = GetBarrierLayout(in.before.layout);
                out.LayoutAfter = GetBarrierLayout(in.after.layout);
                out.pResource = texture;
                out.Subresources.IndexOrFirstMipLevel = in.mipOffset;
                out.Subresources.NumMipLevels = in.mipNum == REMAINING_MIPS ? desc.mipNum : in.mipNum;
                out.Subresources.FirstArraySlice = in.layerOffset;
                out.Subresources.NumArraySlices = in.layerNum == REMAINING_LAYERS ? desc.layerNum : in.layerNum;
                out.Subresources.FirstPlane = 0;
                out.Subresources.NumPlanes = 1;

                // https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#d3d12_texture_barrier_flags
                out.Flags = in.before.layout == Layout::UNKNOWN ? D3D12_TEXTURE_BARRIER_FLAG_DISCARD : D3D12_TEXTURE_BARRIER_FLAG_NONE; // TODO: verify that it works
            }
        }

        // Submit
        m_GraphicsCommandList->Barrier(barriersGroupsNum, barrierGroups);
    } else
#endif
    { // Legacy barriers
        // Count
        uint32_t barrierNum = barrierGroupDesc.bufferNum;

        for (uint32_t i = 0; i < barrierGroupDesc.textureNum; i++) {
            const TextureBarrierDesc& barrierDesc = barrierGroupDesc.textures[i];
            const TextureD3D12& texture = *(TextureD3D12*)barrierDesc.texture;
            const TextureDesc& textureDesc = texture.GetDesc();
            const Dim_t layerNum = barrierDesc.layerNum == REMAINING_LAYERS ? textureDesc.layerNum : barrierDesc.layerNum;
            const Mip_t mipNum = barrierDesc.mipNum == REMAINING_MIPS ? textureDesc.mipNum : barrierDesc.mipNum;

            if (barrierDesc.layerOffset == 0 && layerNum == textureDesc.layerNum && barrierDesc.mipOffset == 0 && mipNum == textureDesc.mipNum)
                barrierNum++;
            else
                barrierNum += layerNum * mipNum;
        }

        bool isGlobalUavBarrierNeeded = false;
        for (uint32_t i = 0; i < barrierGroupDesc.globalNum && !isGlobalUavBarrierNeeded; i++) {
            const GlobalBarrierDesc& barrierDesc = barrierGroupDesc.globals[i];
            if ((barrierDesc.before.access & AccessBits::SHADER_RESOURCE_STORAGE) && (barrierDesc.after.access & AccessBits::SHADER_RESOURCE_STORAGE))
                isGlobalUavBarrierNeeded = true;
        }

        if (isGlobalUavBarrierNeeded)
            barrierNum++;

        if (!barrierNum)
            return;

        // Gather
        Scratch<D3D12_RESOURCE_BARRIER> barriers = AllocateScratch(m_Device, D3D12_RESOURCE_BARRIER, barrierNum);
        memset(barriers, 0, sizeof(D3D12_RESOURCE_BARRIER) * barrierNum);

        D3D12_RESOURCE_BARRIER* ptr = barriers;
        D3D12_COMMAND_LIST_TYPE commandListType = m_GraphicsCommandList->GetType();

        for (uint32_t i = 0; i < barrierGroupDesc.bufferNum; i++) {
            const BufferBarrierDesc& barrierDesc = barrierGroupDesc.buffers[i];
            AddResourceBarrier(commandListType, *((BufferD3D12*)barrierDesc.buffer), barrierDesc.before.access, barrierDesc.after.access, *ptr++, 0);
        }

        for (uint32_t i = 0; i < barrierGroupDesc.textureNum; i++) {
            const TextureBarrierDesc& barrierDesc = barrierGroupDesc.textures[i];
            const TextureD3D12& texture = *(TextureD3D12*)barrierDesc.texture;
            const TextureDesc& textureDesc = texture.GetDesc();
            const Dim_t layerNum = barrierDesc.layerNum == REMAINING_LAYERS ? textureDesc.layerNum : barrierDesc.layerNum;
            const Mip_t mipNum = barrierDesc.mipNum == REMAINING_MIPS ? textureDesc.mipNum : barrierDesc.mipNum;

            if (barrierDesc.layerOffset == 0 && layerNum == textureDesc.layerNum && barrierDesc.mipOffset == 0 && mipNum == textureDesc.mipNum && barrierDesc.planes == PlaneBits::ALL)
                AddResourceBarrier(commandListType, texture, barrierDesc.before.access, barrierDesc.after.access, *ptr++, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
            else {
                for (Dim_t layerOffset = barrierDesc.layerOffset; layerOffset < barrierDesc.layerOffset + layerNum; layerOffset++) {
                    for (Mip_t mipOffset = barrierDesc.mipOffset; mipOffset < barrierDesc.mipOffset + mipNum; mipOffset++) {
                        uint32_t subresource = texture.GetSubresourceIndex(layerOffset, mipOffset, barrierDesc.planes);
                        AddResourceBarrier(commandListType, texture, barrierDesc.before.access, barrierDesc.after.access, *ptr++, subresource);
                    }
                }
            }
        }

        if (isGlobalUavBarrierNeeded) {
            ptr->Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            ptr->UAV.pResource = nullptr;
        }

        // Submit
        m_GraphicsCommandList->ResourceBarrier(barrierNum, barriers);
    }
}

NRI_INLINE void CommandBufferD3D12::BeginQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    m_GraphicsCommandList->BeginQuery(queryPoolD3D12, queryPoolD3D12.GetType(), offset);
}

NRI_INLINE void CommandBufferD3D12::EndQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    m_GraphicsCommandList->EndQuery(queryPoolD3D12, queryPoolD3D12.GetType(), offset);
}

NRI_INLINE void CommandBufferD3D12::CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& buffer, uint64_t alignedBufferOffset) {
    const QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    const BufferD3D12& bufferD3D12 = (BufferD3D12&)buffer;

    if (queryPoolD3D12.GetType() == QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE) {
        const uint64_t srcOffset = offset * queryPoolD3D12.GetQuerySize();
        const uint64_t size = num * queryPoolD3D12.GetQuerySize();
        m_GraphicsCommandList->CopyBufferRegion(bufferD3D12, alignedBufferOffset, queryPoolD3D12.GetReadbackBuffer(), srcOffset, size);
        return;
    }

    m_GraphicsCommandList->ResolveQueryData(queryPoolD3D12, queryPoolD3D12.GetType(), offset, num, bufferD3D12, alignedBufferOffset);
}

NRI_INLINE void CommandBufferD3D12::BeginAnnotation(const char* name, uint32_t bgra) {
    if (m_Device.HasPix())
        m_Device.GetPix().BeginEventOnCommandList(m_GraphicsCommandList, bgra, name);
    else
        PIXBeginEvent(m_GraphicsCommandList, bgra, name);
}

NRI_INLINE void CommandBufferD3D12::EndAnnotation() {
    if (m_Device.HasPix())
        m_Device.GetPix().EndEventOnCommandList(m_GraphicsCommandList);
    else
        PIXEndEvent(m_GraphicsCommandList);
}

NRI_INLINE void CommandBufferD3D12::Annotation(const char* name, uint32_t bgra) {
    if (m_Device.HasPix())
        m_Device.GetPix().SetMarkerOnCommandList(m_GraphicsCommandList, bgra, name);
    else
        PIXSetMarker(m_GraphicsCommandList, bgra, name);
}

NRI_INLINE void CommandBufferD3D12::BuildTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {
    static_assert(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) == sizeof(GeometryObjectInstance), "Mismatched sizeof");

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
    desc.DestAccelerationStructureData = ((AccelerationStructureD3D12&)dst).GetHandle();
    desc.ScratchAccelerationStructureData = ((BufferD3D12&)scratch).GetPointerGPU() + scratchOffset;
    desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    desc.Inputs.Flags = GetAccelerationStructureBuildFlags(flags);
    desc.Inputs.NumDescs = instanceNum;
    desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    desc.Inputs.InstanceDescs = ((BufferD3D12&)buffer).GetPointerGPU() + bufferOffset;

    if (m_Version >= 4)
        m_GraphicsCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
}

NRI_INLINE void CommandBufferD3D12::BuildBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
    desc.DestAccelerationStructureData = ((AccelerationStructureD3D12&)dst).GetHandle();
    desc.ScratchAccelerationStructureData = ((BufferD3D12&)scratch).GetPointerGPU() + scratchOffset;
    desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    desc.Inputs.Flags = GetAccelerationStructureBuildFlags(flags);
    desc.Inputs.NumDescs = geometryObjectNum;
    desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    Scratch<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs = AllocateScratch(m_Device, D3D12_RAYTRACING_GEOMETRY_DESC, geometryObjectNum);
    ConvertGeometryDescs(geometryDescs, geometryObjects, geometryObjectNum);
    desc.Inputs.pGeometryDescs = geometryDescs;

    if (m_Version >= 4)
        m_GraphicsCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
}

NRI_INLINE void CommandBufferD3D12::UpdateTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags,
    AccelerationStructure& dst, const AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
    desc.DestAccelerationStructureData = ((AccelerationStructureD3D12&)dst).GetHandle();
    desc.SourceAccelerationStructureData = ((AccelerationStructureD3D12&)src).GetHandle();
    desc.ScratchAccelerationStructureData = ((BufferD3D12&)scratch).GetPointerGPU() + scratchOffset;
    desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    desc.Inputs.Flags = GetAccelerationStructureBuildFlags(flags) | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    desc.Inputs.NumDescs = instanceNum;
    desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    desc.Inputs.InstanceDescs = ((BufferD3D12&)buffer).GetPointerGPU() + bufferOffset;

    if (m_Version >= 4)
        m_GraphicsCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
}

NRI_INLINE void CommandBufferD3D12::UpdateBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags,
    AccelerationStructure& dst, const AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
    desc.DestAccelerationStructureData = ((AccelerationStructureD3D12&)dst).GetHandle();
    desc.SourceAccelerationStructureData = ((AccelerationStructureD3D12&)src).GetHandle();
    desc.ScratchAccelerationStructureData = ((BufferD3D12&)scratch).GetPointerGPU() + scratchOffset;
    desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    desc.Inputs.Flags = GetAccelerationStructureBuildFlags(flags) | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    desc.Inputs.NumDescs = geometryObjectNum;
    desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    Scratch<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs = AllocateScratch(m_Device, D3D12_RAYTRACING_GEOMETRY_DESC, geometryObjectNum);
    ConvertGeometryDescs(geometryDescs, geometryObjects, geometryObjectNum);
    desc.Inputs.pGeometryDescs = geometryDescs;

    if (m_Version >= 4)
        m_GraphicsCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
}

NRI_INLINE void CommandBufferD3D12::CopyAccelerationStructure(AccelerationStructure& dst, const AccelerationStructure& src, CopyMode copyMode) {
    m_GraphicsCommandList->CopyRaytracingAccelerationStructure(((AccelerationStructureD3D12&)dst).GetHandle(), ((AccelerationStructureD3D12&)src).GetHandle(), GetCopyMode(copyMode));
}

NRI_INLINE void CommandBufferD3D12::WriteAccelerationStructureSize(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryOffset) {
    Scratch<D3D12_GPU_VIRTUAL_ADDRESS> virtualAddresses = AllocateScratch(m_Device, D3D12_GPU_VIRTUAL_ADDRESS, accelerationStructureNum);
    for (uint32_t i = 0; i < accelerationStructureNum; i++)
        virtualAddresses[i] = ((AccelerationStructureD3D12&)accelerationStructures[i]).GetHandle();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postbuildInfo = {};
    postbuildInfo.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;
    postbuildInfo.DestBuffer = ((QueryPoolD3D12&)queryPool).GetReadbackBuffer()->GetGPUVirtualAddress() + queryOffset;

    if (m_Version >= 4)
        m_GraphicsCommandList->EmitRaytracingAccelerationStructurePostbuildInfo(&postbuildInfo, accelerationStructureNum, virtualAddresses);
}

NRI_INLINE void CommandBufferD3D12::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc) {
    D3D12_DISPATCH_RAYS_DESC desc = {};

    desc.RayGenerationShaderRecord.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.raygenShader.buffer).GetPointerGPU() + dispatchRaysDesc.raygenShader.offset;
    desc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    if (dispatchRaysDesc.missShaders.buffer) {
        desc.MissShaderTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.missShaders.buffer).GetPointerGPU() + dispatchRaysDesc.missShaders.offset;
        desc.MissShaderTable.SizeInBytes = dispatchRaysDesc.missShaders.size;
        desc.MissShaderTable.StrideInBytes = dispatchRaysDesc.missShaders.stride;
    }

    if (dispatchRaysDesc.hitShaderGroups.buffer) {
        desc.HitGroupTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.hitShaderGroups.buffer).GetPointerGPU() + dispatchRaysDesc.hitShaderGroups.offset;
        desc.HitGroupTable.SizeInBytes = dispatchRaysDesc.hitShaderGroups.size;
        desc.HitGroupTable.StrideInBytes = dispatchRaysDesc.hitShaderGroups.stride;
    }

    if (dispatchRaysDesc.callableShaders.buffer) {
        desc.CallableShaderTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.callableShaders.buffer).GetPointerGPU() + dispatchRaysDesc.callableShaders.offset;
        desc.CallableShaderTable.SizeInBytes = dispatchRaysDesc.callableShaders.size;
        desc.CallableShaderTable.StrideInBytes = dispatchRaysDesc.callableShaders.stride;
    }

    desc.Width = dispatchRaysDesc.x;
    desc.Height = dispatchRaysDesc.y;
    desc.Depth = dispatchRaysDesc.z;

    if (m_Version >= 4)
        m_GraphicsCommandList->DispatchRays(&desc);
}

NRI_INLINE void CommandBufferD3D12::DispatchRaysIndirect(const Buffer& buffer, uint64_t offset) {
    static_assert(sizeof(DispatchRaysIndirectDesc) == sizeof(D3D12_DISPATCH_RAYS_DESC));

    if (m_Version >= 4)
        m_GraphicsCommandList->ExecuteIndirect(m_Device.GetDispatchRaysCommandSignature(), 1, (BufferD3D12&)buffer, offset, nullptr, 0);
}

NRI_INLINE void CommandBufferD3D12::DrawMeshTasks(const DrawMeshTasksDesc& drawMeshTasksDesc) {
    if (m_Version >= 6)
        m_GraphicsCommandList->DispatchMesh(drawMeshTasksDesc.x, drawMeshTasksDesc.y, drawMeshTasksDesc.z);
}

NRI_INLINE void CommandBufferD3D12::DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    static_assert(sizeof(DrawMeshTasksDesc) == sizeof(D3D12_DISPATCH_MESH_ARGUMENTS));

    ID3D12Resource* pCountBuffer = nullptr;
    if (countBuffer)
        pCountBuffer = *(BufferD3D12*)countBuffer;

    if (m_Version >= 6)
        m_GraphicsCommandList->ExecuteIndirect(m_Device.GetDrawMeshCommandSignature(stride), drawNum, (BufferD3D12&)buffer, offset, pCountBuffer, countBufferOffset);
}
