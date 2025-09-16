// © 2021 NVIDIA Corporation

static HRESULT QueryLatestInterface(ComPtr<ID3D12GraphicsCommandListBest>& in, ComPtr<ID3D12GraphicsCommandListBest>& out, uint8_t& version) {
    static const IID versions[] = {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        __uuidof(ID3D12GraphicsCommandList10),
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

    version = n - i - 1;

    return i == 0 ? S_OK : D3D12_ERROR_INVALID_REDIST;
}

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
static inline D3D12_BARRIER_SYNC GetBarrierSyncFlags(StageBits stageBits, AccessBits accessBits) {
    // Check non-mask values first
    if (stageBits == StageBits::ALL)
        return D3D12_BARRIER_SYNC_ALL;

    if (stageBits == StageBits::NONE)
        return D3D12_BARRIER_SYNC_NONE;

    // Gather bits
    D3D12_BARRIER_SYNC flags = D3D12_BARRIER_SYNC_NONE; // = 0

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

    if (stageBits & (StageBits::ACCELERATION_STRUCTURE | StageBits::MICROMAP)) {
        flags |= D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE | D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE;

        // There is no "EMIT_POSTBUILD_INFO" flag in VK, moreover "VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR" already includes "ACCELERATION_STRUCTURE_COPY".
        // "EMIT_POSTBUILD_INFO" can't be set if "write" access is expected.
        if (!(accessBits & AccessBits::ACCELERATION_STRUCTURE_WRITE))
            flags |= D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO;
    }

    return flags;
}

static inline D3D12_BARRIER_ACCESS GetBarrierAccessFlags(AccessBits accessBits) {
    // Check non-mask values first
    if (accessBits == AccessBits::NONE)
        return D3D12_BARRIER_ACCESS_NO_ACCESS;

    // Gather bits
    D3D12_BARRIER_ACCESS flags = D3D12_BARRIER_ACCESS_COMMON; // = 0

    if (accessBits & AccessBits::INDEX_BUFFER)
        flags |= D3D12_BARRIER_ACCESS_INDEX_BUFFER;

    if (accessBits & AccessBits::VERTEX_BUFFER)
        flags |= D3D12_BARRIER_ACCESS_VERTEX_BUFFER;

    if (accessBits & AccessBits::CONSTANT_BUFFER)
        flags |= D3D12_BARRIER_ACCESS_CONSTANT_BUFFER;

    if (accessBits & AccessBits::ARGUMENT_BUFFER)
        flags |= D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;

    if (accessBits & AccessBits::COLOR_ATTACHMENT)
        flags |= D3D12_BARRIER_ACCESS_RENDER_TARGET;

    if (accessBits & AccessBits::SHADING_RATE_ATTACHMENT)
        flags |= D3D12_BARRIER_ACCESS_SHADING_RATE_SOURCE;

    if (accessBits & AccessBits::DEPTH_STENCIL_ATTACHMENT_WRITE)
        flags |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;

    if (accessBits & AccessBits::DEPTH_STENCIL_ATTACHMENT_READ)
        flags |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;

    if (accessBits & (AccessBits::ACCELERATION_STRUCTURE_READ | AccessBits::MICROMAP_READ))
        flags |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;

    if (accessBits & (AccessBits::ACCELERATION_STRUCTURE_WRITE | AccessBits::MICROMAP_WRITE))
        flags |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;

    if (accessBits & (AccessBits::SHADER_RESOURCE | AccessBits::SHADER_BINDING_TABLE))
        flags |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;

    if (accessBits & (AccessBits::SHADER_RESOURCE_STORAGE | AccessBits::SCRATCH_BUFFER | AccessBits::CLEAR_STORAGE))
        flags |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;

    if (accessBits & AccessBits::COPY_SOURCE)
        flags |= D3D12_BARRIER_ACCESS_COPY_SOURCE;

    if (accessBits & AccessBits::COPY_DESTINATION)
        flags |= D3D12_BARRIER_ACCESS_COPY_DEST;

    if (accessBits & AccessBits::RESOLVE_SOURCE)
        flags |= D3D12_BARRIER_ACCESS_RESOLVE_SOURCE;

    if (accessBits & AccessBits::RESOLVE_DESTINATION)
        flags |= D3D12_BARRIER_ACCESS_RESOLVE_DEST;

    return flags;
}

constexpr std::array<D3D12_BARRIER_LAYOUT, (size_t)Layout::MAX_NUM> g_BarrierLayouts = {
    D3D12_BARRIER_LAYOUT_UNDEFINED,           // UNDEFINED
    D3D12_BARRIER_LAYOUT_COMMON,              // GENERAL
    D3D12_BARRIER_LAYOUT_PRESENT,             // PRESENT
    D3D12_BARRIER_LAYOUT_RENDER_TARGET,       // COLOR_ATTACHMENT
    D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE, // SHADING_RATE_ATTACHMENT
    D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE, // DEPTH_STENCIL_ATTACHMENT
    D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ,  // DEPTH_STENCIL_READONLY
    D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,     // SHADER_RESOURCE
    D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,    // SHADER_RESOURCE_STORAGE
    D3D12_BARRIER_LAYOUT_COPY_SOURCE,         // COPY_SOURCE
    D3D12_BARRIER_LAYOUT_COPY_DEST,           // COPY_DESTINATION
    D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE,      // RESOLVE_SOURCE
    D3D12_BARRIER_LAYOUT_RESOLVE_DEST,        // RESOLVE_DESTINATION
};
VALIDATE_ARRAY(g_BarrierLayouts);

static inline D3D12_BARRIER_LAYOUT GetBarrierLayout(Layout layout) {
    return g_BarrierLayouts[(uint32_t)layout];
}
#endif

static inline D3D12_RESOURCE_STATES GetResourceStates(AccessBits accessBits, D3D12_COMMAND_LIST_TYPE commandListType) {
    D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_COMMON;

    if (accessBits & AccessBits::INDEX_BUFFER)
        resourceStates |= D3D12_RESOURCE_STATE_INDEX_BUFFER;

    if (accessBits & (AccessBits::CONSTANT_BUFFER | AccessBits::VERTEX_BUFFER))
        resourceStates |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    if (accessBits & AccessBits::ARGUMENT_BUFFER)
        resourceStates |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;

    if (accessBits & AccessBits::COLOR_ATTACHMENT)
        resourceStates |= D3D12_RESOURCE_STATE_RENDER_TARGET;

    if (accessBits & AccessBits::SHADING_RATE_ATTACHMENT)
        resourceStates |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;

    if (accessBits & AccessBits::DEPTH_STENCIL_ATTACHMENT_READ)
        resourceStates |= D3D12_RESOURCE_STATE_DEPTH_READ;

    if (accessBits & AccessBits::DEPTH_STENCIL_ATTACHMENT_WRITE)
        resourceStates |= D3D12_RESOURCE_STATE_DEPTH_WRITE;

    if (accessBits & (AccessBits::ACCELERATION_STRUCTURE_READ | AccessBits::MICROMAP_READ))
        resourceStates |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    if (accessBits & (AccessBits::ACCELERATION_STRUCTURE_WRITE | AccessBits::MICROMAP_WRITE))
        resourceStates |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    if (accessBits & AccessBits::SHADER_RESOURCE) {
        resourceStates |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        if (commandListType == D3D12_COMMAND_LIST_TYPE_DIRECT)
            resourceStates |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    if (accessBits & AccessBits::SHADER_BINDING_TABLE)
        resourceStates |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    if (accessBits & (AccessBits::SHADER_RESOURCE_STORAGE | AccessBits::SCRATCH_BUFFER | AccessBits::CLEAR_STORAGE))
        resourceStates |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    if (accessBits & AccessBits::COPY_SOURCE)
        resourceStates |= D3D12_RESOURCE_STATE_COPY_SOURCE;

    if (accessBits & AccessBits::COPY_DESTINATION)
        resourceStates |= D3D12_RESOURCE_STATE_COPY_DEST;

    if (accessBits & AccessBits::RESOLVE_SOURCE)
        resourceStates |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;

    if (accessBits & AccessBits::RESOLVE_DESTINATION)
        resourceStates |= D3D12_RESOURCE_STATE_RESOLVE_DEST;

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
    HRESULT hr = m_Device->CreateCommandList(NODE_MASK, commandListType, commandAllocator, nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&graphicsCommandList);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommandList");

    hr = QueryLatestInterface(graphicsCommandList, m_GraphicsCommandList, m_Version);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12GraphicsCommandList::QueryLatestInterface");

    hr = m_GraphicsCommandList->Close();
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12GraphicsCommandList::Close");

    m_CommandAllocator = commandAllocator;

    return Result::SUCCESS;
}

Result CommandBufferD3D12::Create(const CommandBufferD3D12Desc& commandBufferD3D12Desc) {
    ComPtr<ID3D12GraphicsCommandListBest> graphicsCommandList = (ID3D12GraphicsCommandListBest*)commandBufferD3D12Desc.d3d12CommandList;

    HRESULT hr = QueryLatestInterface(graphicsCommandList, m_GraphicsCommandList, m_Version);
    if (hr == D3D12_ERROR_INVALID_REDIST)
        REPORT_WARNING(&m_Device, "ID3D12GraphicsCommandList version is lower than expected, some functionality may be not available...");

    // TODO: what if opened?

    m_CommandAllocator = commandBufferD3D12Desc.d3d12CommandAllocator;

    return Result::SUCCESS;
}

NRI_INLINE Result CommandBufferD3D12::Begin(const DescriptorPool* descriptorPool) {
    HRESULT hr = m_GraphicsCommandList->Reset(m_CommandAllocator, nullptr);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12GraphicsCommandList::Reset");

    if (descriptorPool)
        SetDescriptorPool(*descriptorPool);

    m_PipelineLayout = nullptr;
    m_PipelineBindPoint = BindPoint::INHERIT;

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
    m_GraphicsCommandList->OMSetDepthBounds(boundsMin, boundsMax);
}

NRI_INLINE void CommandBufferD3D12::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    MaybeUnused(backRef);
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    if (m_Device.GetDesc().features.independentFrontAndBackStencilReferenceAndMasks)
        m_GraphicsCommandList->OMSetFrontAndBackStencilRef(frontRef, backRef);
    else
#endif
        m_GraphicsCommandList->OMSetStencilRef(frontRef);
}

NRI_INLINE void CommandBufferD3D12::SetSampleLocations(const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    static_assert(sizeof(D3D12_SAMPLE_POSITION) == sizeof(SampleLocation));

    uint32_t pixelNum = locationNum / sampleNum;
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
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
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

NRI_INLINE void CommandBufferD3D12::ClearStorage(const ClearStorageDesc& clearDesc) {
    DescriptorSetD3D12* descriptorSet = m_DescriptorSets[clearDesc.setIndex];
    DescriptorD3D12* storage = (DescriptorD3D12*)clearDesc.storage;

    // TODO: typed buffers are currently cleared according to the format, it seems to be more reliable than using integers for all buffers
    if (storage->IsIntegerFormat())
        m_GraphicsCommandList->ClearUnorderedAccessViewUint({descriptorSet->GetDescriptorHandleGPU(clearDesc.rangeIndex, clearDesc.descriptorIndex)}, {storage->GetDescriptorHandleCPU()}, *storage, &clearDesc.value.ui.x, 0, nullptr);
    else
        m_GraphicsCommandList->ClearUnorderedAccessViewFloat({descriptorSet->GetDescriptorHandleGPU(clearDesc.rangeIndex, clearDesc.descriptorIndex)}, {storage->GetDescriptorHandleCPU()}, *storage, &clearDesc.value.f.x, 0, nullptr);
}

NRI_INLINE void CommandBufferD3D12::BeginRendering(const AttachmentsDesc& attachmentsDesc) {
    // Render targets
    m_RenderTargetNum = attachmentsDesc.colors ? attachmentsDesc.colorNum : 0;

    uint32_t i = 0;
    for (; i < m_RenderTargetNum; i++) {
        const DescriptorD3D12& descriptor = *(DescriptorD3D12*)attachmentsDesc.colors[i];
        m_RenderTargets[i].ptr = descriptor.GetDescriptorHandleCPU();
    }
    for (; i < (uint32_t)m_RenderTargets.size(); i++)
        m_RenderTargets[i].ptr = NULL;

    if (attachmentsDesc.depthStencil) {
        const DescriptorD3D12& descriptor = *(DescriptorD3D12*)attachmentsDesc.depthStencil;
        m_DepthStencil.ptr = descriptor.GetDescriptorHandleCPU();
    } else
        m_DepthStencil.ptr = NULL;

    m_GraphicsCommandList->OMSetRenderTargets(m_RenderTargetNum, m_RenderTargets.data(), FALSE, m_DepthStencil.ptr ? &m_DepthStencil : nullptr);

    // Shading rate
    if (m_Device.GetDesc().tiers.shadingRate >= 2) {
        ID3D12Resource* shadingRateImage = nullptr;
        if (attachmentsDesc.shadingRate)
            shadingRateImage = *(DescriptorD3D12*)attachmentsDesc.shadingRate;

        m_GraphicsCommandList->RSSetShadingRateImage(shadingRateImage);
    }

    // Multiview
    if (m_Device.GetDesc().other.viewMaxNum > 1 && attachmentsDesc.viewMask)
        m_GraphicsCommandList->SetViewInstanceMask(attachmentsDesc.viewMask);
}

NRI_INLINE void CommandBufferD3D12::SetVertexBuffers(uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum) {
    Scratch<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews = AllocateScratch(m_Device, D3D12_VERTEX_BUFFER_VIEW, vertexBufferNum);
    for (uint32_t i = 0; i < vertexBufferNum; i++) {
        const VertexBufferDesc& vertexBufferDesc = vertexBufferDescs[i];

        const BufferD3D12* bufferD3D12 = (BufferD3D12*)vertexBufferDesc.buffer;
        if (bufferD3D12) {
            vertexBufferViews[i].BufferLocation = bufferD3D12->GetGPUVA() + vertexBufferDesc.offset;
            vertexBufferViews[i].SizeInBytes = (uint32_t)(bufferD3D12->GetDesc().size - vertexBufferDesc.offset);
            vertexBufferViews[i].StrideInBytes = vertexBufferDesc.stride;
        } else {
            vertexBufferViews[i].BufferLocation = 0;
            vertexBufferViews[i].SizeInBytes = 0;
            vertexBufferViews[i].StrideInBytes = 0;
        }
    }

    m_GraphicsCommandList->IASetVertexBuffers(baseSlot, vertexBufferNum, vertexBufferViews);
}

NRI_INLINE void CommandBufferD3D12::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    const BufferD3D12& bufferD3D12 = (BufferD3D12&)buffer;

    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    indexBufferView.BufferLocation = bufferD3D12.GetGPUVA() + offset;
    indexBufferView.SizeInBytes = (uint32_t)(bufferD3D12.GetDesc().size - offset);
    indexBufferView.Format = indexType == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

    m_GraphicsCommandList->IASetIndexBuffer(&indexBufferView);
}

NRI_INLINE void CommandBufferD3D12::SetPipelineLayout(BindPoint bindPoint, const PipelineLayout& pipelineLayout) {
    const PipelineLayoutD3D12& pipelineLayoutD3D12 = (PipelineLayoutD3D12&)pipelineLayout;
    if (bindPoint == BindPoint::GRAPHICS)
        m_GraphicsCommandList->SetGraphicsRootSignature(pipelineLayoutD3D12);
    else
        m_GraphicsCommandList->SetComputeRootSignature(pipelineLayoutD3D12);

    m_PipelineLayout = &pipelineLayoutD3D12;
    m_PipelineBindPoint = bindPoint;
}

NRI_INLINE void CommandBufferD3D12::SetPipeline(const Pipeline& pipeline) {
    PipelineD3D12* pipelineD3D12 = (PipelineD3D12*)&pipeline;
    pipelineD3D12->Bind(m_GraphicsCommandList);
}

NRI_INLINE void CommandBufferD3D12::SetDescriptorPool(const DescriptorPool& descriptorPool) {
    ((DescriptorPoolD3D12&)descriptorPool).Bind(m_GraphicsCommandList);
}

NRI_INLINE void CommandBufferD3D12::SetDescriptorSet(const SetDescriptorSetDesc& setDescriptorSetDesc) {
    BindPoint bindPoint = setDescriptorSetDesc.bindPoint == BindPoint::INHERIT ? m_PipelineBindPoint : setDescriptorSetDesc.bindPoint;
    m_PipelineLayout->SetDescriptorSet(m_GraphicsCommandList, bindPoint, setDescriptorSetDesc);

    m_DescriptorSets[setDescriptorSetDesc.setIndex] = (DescriptorSetD3D12*)setDescriptorSetDesc.descriptorSet;
}

NRI_INLINE void CommandBufferD3D12::SetRootConstants(const SetRootConstantsDesc& setRootConstantsDesc) {
    BindPoint bindPoint = setRootConstantsDesc.bindPoint == BindPoint::INHERIT ? m_PipelineBindPoint : setRootConstantsDesc.bindPoint;
    m_PipelineLayout->SetRootConstants(m_GraphicsCommandList, bindPoint, setRootConstantsDesc);
}

NRI_INLINE void CommandBufferD3D12::SetRootDescriptor(const SetRootDescriptorDesc& setRootDescriptorDesc) {
    BindPoint bindPoint = setRootDescriptorDesc.bindPoint == BindPoint::INHERIT ? m_PipelineBindPoint : setRootDescriptorDesc.bindPoint;
    m_PipelineLayout->SetRootDescriptor(m_GraphicsCommandList, bindPoint, setRootDescriptorDesc);
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

NRI_INLINE void CommandBufferD3D12::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    const TextureD3D12& dst = (TextureD3D12&)dstTexture;
    const TextureD3D12& src = (TextureD3D12&)srcTexture;

    bool isWholeResource = !dstRegion && !srcRegion;
    if (isWholeResource)
        m_GraphicsCommandList->CopyResource(dst, src);
    else {
        TextureRegionDesc wholeResource = {};
        if (!srcRegion)
            srcRegion = &wholeResource;
        if (!dstRegion)
            dstRegion = &wholeResource;

        D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation = {dst, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        dstTextureCopyLocation.SubresourceIndex = dst.GetSubresourceIndex(dstRegion->layerOffset, dstRegion->mipOffset, dstRegion->planes);

        D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation = {src, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        srcTextureCopyLocation.SubresourceIndex = src.GetSubresourceIndex(srcRegion->layerOffset, srcRegion->mipOffset, srcRegion->planes);

        uint32_t w = srcRegion->width == WHOLE_SIZE ? src.GetSize(0, srcRegion->mipOffset) : srcRegion->width;
        uint32_t h = srcRegion->height == WHOLE_SIZE ? src.GetSize(1, srcRegion->mipOffset) : srcRegion->height;
        uint32_t d = srcRegion->depth == WHOLE_SIZE ? src.GetSize(2, srcRegion->mipOffset) : srcRegion->depth;

        D3D12_BOX srcBox = {
            srcRegion->x,
            srcRegion->y,
            srcRegion->z,
            srcRegion->x + w,
            srcRegion->y + h,
            srcRegion->z + d,
        };

        m_GraphicsCommandList->CopyTextureRegion(&dstTextureCopyLocation, dstRegion->x, dstRegion->y, dstRegion->z, &srcTextureCopyLocation, &srcBox);
    }
}

NRI_INLINE void CommandBufferD3D12::ZeroBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    const BufferD3D12& dst = (BufferD3D12&)buffer;
    ID3D12Resource* zeroBuffer = m_Device.GetZeroBuffer();
    D3D12_RESOURCE_DESC zeroBufferDesc = zeroBuffer->GetDesc();

    if (size == WHOLE_SIZE)
        size = dst.GetDesc().size;

    // "Self" copies require COMMON to COMMON barrier in-between, making the implementation 2x slower
    while (size) {
        uint64_t blockSize = std::min(size, zeroBufferDesc.Width);

        m_GraphicsCommandList->CopyBufferRegion(dst, offset, zeroBuffer, 0, blockSize);

        offset += blockSize;
        size -= blockSize;
    }
}

NRI_INLINE void CommandBufferD3D12::ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    const TextureD3D12& dst = (TextureD3D12&)dstTexture;
    const TextureD3D12& src = (TextureD3D12&)srcTexture;
    const TextureDesc& dstDesc = dst.GetDesc();
    const DxgiFormat& dstFormat = GetDxgiFormat(dstDesc.format);

    bool isWholeResource = !dstRegion && !srcRegion;
    if (isWholeResource) {
        for (Dim_t layer = 0; layer < dstDesc.layerNum; layer++) {
            for (Dim_t mip = 0; mip < dstDesc.mipNum; mip++) {
                uint32_t subresource = dst.GetSubresourceIndex(layer, mip, PlaneBits::ALL);
                m_GraphicsCommandList->ResolveSubresource(dst, subresource, src, subresource, dstFormat.typed);
            }
        }
    } else {
        TextureRegionDesc wholeResource = {};
        if (!srcRegion)
            srcRegion = &wholeResource;
        if (!dstRegion)
            dstRegion = &wholeResource;

        uint32_t dstSubresource = dst.GetSubresourceIndex(dstRegion->layerOffset, dstRegion->mipOffset, dstRegion->planes);
        uint32_t srcSubresource = src.GetSubresourceIndex(srcRegion->layerOffset, srcRegion->mipOffset, srcRegion->planes);

        D3D12_RECT srcRect = {
            srcRegion->x,
            srcRegion->y,
            srcRegion->width,
            srcRegion->height,
        };

        m_GraphicsCommandList->ResolveSubresourceRegion(dst, dstSubresource, dstRegion->x, dstRegion->y, src, srcSubresource, &srcRect, dstFormat.typed, D3D12_RESOLVE_MODE_AVERAGE);
    }
}

NRI_INLINE void CommandBufferD3D12::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegion, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayout) {
    const TextureD3D12& dst = (TextureD3D12&)dstTexture;
    const TextureDesc& dstDesc = dst.GetDesc();

    D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation = {dst, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    dstTextureCopyLocation.SubresourceIndex = dst.GetSubresourceIndex(dstRegion.layerOffset, dstRegion.mipOffset, dstRegion.planes);

    const uint32_t size[3] = {
        dstRegion.width == WHOLE_SIZE ? dst.GetSize(0, dstRegion.mipOffset) : dstRegion.width,
        dstRegion.height == WHOLE_SIZE ? dst.GetSize(1, dstRegion.mipOffset) : dstRegion.height,
        dstRegion.depth == WHOLE_SIZE ? dst.GetSize(2, dstRegion.mipOffset) : dstRegion.depth,
    };

    D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation = {};
    srcTextureCopyLocation.pResource = (BufferD3D12&)srcBuffer;
    srcTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcTextureCopyLocation.PlacedFootprint.Offset = srcDataLayout.offset;
    srcTextureCopyLocation.PlacedFootprint.Footprint.Format = GetDxgiFormat(dstDesc.format).typeless;
    srcTextureCopyLocation.PlacedFootprint.Footprint.Width = size[0];
    srcTextureCopyLocation.PlacedFootprint.Footprint.Height = size[1];
    srcTextureCopyLocation.PlacedFootprint.Footprint.Depth = size[2];
    srcTextureCopyLocation.PlacedFootprint.Footprint.RowPitch = srcDataLayout.rowPitch;

    m_GraphicsCommandList->CopyTextureRegion(&dstTextureCopyLocation, dstRegion.x, dstRegion.y, dstRegion.z, &srcTextureCopyLocation, nullptr);
}

NRI_INLINE void CommandBufferD3D12::ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayout, const Texture& srcTexture, const TextureRegionDesc& srcRegion) {
    const TextureD3D12& src = (TextureD3D12&)srcTexture;
    const TextureDesc& srcDesc = src.GetDesc();

    D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation = {src, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    srcTextureCopyLocation.SubresourceIndex = src.GetSubresourceIndex(srcRegion.layerOffset, srcRegion.mipOffset, srcRegion.planes);

    D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation = {};
    dstTextureCopyLocation.pResource = (BufferD3D12&)dstBuffer;
    dstTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstTextureCopyLocation.PlacedFootprint.Offset = dstDataLayout.offset;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Format = GetDxgiFormat(srcDesc.format).typeless;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Width = srcRegion.width;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Height = srcRegion.height;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Depth = srcRegion.depth;
    dstTextureCopyLocation.PlacedFootprint.Footprint.RowPitch = dstDataLayout.rowPitch;

    uint32_t w = srcRegion.width == WHOLE_SIZE ? src.GetSize(0, srcRegion.mipOffset) : srcRegion.width;
    uint32_t h = srcRegion.height == WHOLE_SIZE ? src.GetSize(1, srcRegion.mipOffset) : srcRegion.height;
    uint32_t d = srcRegion.depth == WHOLE_SIZE ? src.GetSize(2, srcRegion.mipOffset) : srcRegion.depth;

    D3D12_BOX srcBox = {
        srcRegion.x,
        srcRegion.y,
        srcRegion.z,
        srcRegion.x + w,
        srcRegion.y + h,
        srcRegion.z + d,
    };

    m_GraphicsCommandList->CopyTextureRegion(&dstTextureCopyLocation, 0, 0, 0, &srcTextureCopyLocation, &srcBox);
}

NRI_INLINE void CommandBufferD3D12::Dispatch(const DispatchDesc& dispatchDesc) {
    m_GraphicsCommandList->Dispatch(dispatchDesc.x, dispatchDesc.y, dispatchDesc.z);
}

NRI_INLINE void CommandBufferD3D12::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    static_assert(sizeof(DispatchDesc) == sizeof(D3D12_DISPATCH_ARGUMENTS));

    m_GraphicsCommandList->ExecuteIndirect(m_Device.GetDispatchCommandSignature(), 1, (BufferD3D12&)buffer, offset, nullptr, 0);
}

NRI_INLINE void CommandBufferD3D12::Barrier(const BarrierDesc& barrierDesc) {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    if (m_Device.GetDesc().features.enhancedBarriers) {
        // Count
        uint32_t barrierNum = barrierDesc.globalNum + barrierDesc.bufferNum + barrierDesc.textureNum;
        if (!barrierNum)
            return;

        D3D12_BARRIER_GROUP barrierGroups[3] = {};
        uint32_t barriersGroupsNum = 0;

        // Global
        Scratch<D3D12_GLOBAL_BARRIER> globalBarriers = AllocateScratch(m_Device, D3D12_GLOBAL_BARRIER, barrierDesc.globalNum);
        if (barrierDesc.globalNum) {
            D3D12_BARRIER_GROUP* barrierGroup = &barrierGroups[barriersGroupsNum++];
            barrierGroup->Type = D3D12_BARRIER_TYPE_GLOBAL;
            barrierGroup->NumBarriers = barrierDesc.globalNum;
            barrierGroup->pGlobalBarriers = globalBarriers;

            for (uint32_t i = 0; i < barrierDesc.globalNum; i++) {
                const GlobalBarrierDesc& in = barrierDesc.globals[i];

                D3D12_GLOBAL_BARRIER& out = globalBarriers[i];
                out = {};
                out.SyncBefore = GetBarrierSyncFlags(in.before.stages, in.before.access);
                out.SyncAfter = GetBarrierSyncFlags(in.after.stages, in.after.access);
                out.AccessBefore = GetBarrierAccessFlags(in.before.access);
                out.AccessAfter = GetBarrierAccessFlags(in.after.access);
            }
        }

        // Buffer
        Scratch<D3D12_BUFFER_BARRIER> bufferBarriers = AllocateScratch(m_Device, D3D12_BUFFER_BARRIER, barrierDesc.bufferNum);
        if (barrierDesc.bufferNum) {
            D3D12_BARRIER_GROUP* barrierGroup = &barrierGroups[barriersGroupsNum++];
            barrierGroup->Type = D3D12_BARRIER_TYPE_BUFFER;
            barrierGroup->NumBarriers = barrierDesc.bufferNum;
            barrierGroup->pBufferBarriers = bufferBarriers;

            for (uint32_t i = 0; i < barrierDesc.bufferNum; i++) {
                const BufferBarrierDesc& in = barrierDesc.buffers[i];
                const BufferD3D12& buffer = *(BufferD3D12*)in.buffer;

                D3D12_BUFFER_BARRIER& out = bufferBarriers[i];
                out = {};
                out.SyncBefore = GetBarrierSyncFlags(in.before.stages, in.before.access);
                out.SyncAfter = GetBarrierSyncFlags(in.after.stages, in.after.access);
                out.AccessBefore = GetBarrierAccessFlags(in.before.access);
                out.AccessAfter = GetBarrierAccessFlags(in.after.access);
                out.pResource = buffer;
                out.Offset = 0;
                out.Size = UINT64_MAX;
            }
        }

        // Texture
        Scratch<D3D12_TEXTURE_BARRIER> textureBarriers = AllocateScratch(m_Device, D3D12_TEXTURE_BARRIER, barrierDesc.textureNum);
        if (barrierDesc.textureNum) {
            D3D12_BARRIER_GROUP* barrierGroup = &barrierGroups[barriersGroupsNum++];
            barrierGroup->Type = D3D12_BARRIER_TYPE_TEXTURE;
            barrierGroup->NumBarriers = barrierDesc.textureNum;
            barrierGroup->pTextureBarriers = textureBarriers;

            for (uint32_t i = 0; i < barrierDesc.textureNum; i++) {
                const TextureBarrierDesc& in = barrierDesc.textures[i];
                const TextureD3D12& texture = *(TextureD3D12*)in.texture;
                const TextureDesc& desc = texture.GetDesc();

                D3D12_TEXTURE_BARRIER& out = textureBarriers[i];
                out = {};
                out.SyncBefore = GetBarrierSyncFlags(in.before.stages, in.before.access);
                out.SyncAfter = GetBarrierSyncFlags(in.after.stages, in.after.access);
                out.AccessBefore = in.before.layout == Layout::PRESENT ? D3D12_BARRIER_ACCESS_COMMON : GetBarrierAccessFlags(in.before.access);
                out.AccessAfter = in.after.layout == Layout::PRESENT ? D3D12_BARRIER_ACCESS_COMMON : GetBarrierAccessFlags(in.after.access);
                out.LayoutBefore = GetBarrierLayout(in.before.layout);
                out.LayoutAfter = GetBarrierLayout(in.after.layout);
                out.pResource = texture;
                out.Subresources.IndexOrFirstMipLevel = in.mipOffset;
                out.Subresources.NumMipLevels = in.mipNum == REMAINING ? desc.mipNum : in.mipNum;
                out.Subresources.FirstArraySlice = in.layerOffset;
                out.Subresources.NumArraySlices = in.layerNum == REMAINING ? desc.layerNum : in.layerNum;

                const FormatProps& formatProps = GetFormatProps(desc.format);
                if (in.planes == PlaneBits::ALL || (in.planes & PlaneBits::STENCIL)) { // fallthrough
                    out.Subresources.NumPlanes += formatProps.isStencil ? 1 : 0;
                    out.Subresources.FirstPlane = 1;
                }
                if (in.planes == PlaneBits::ALL || (in.planes & PlaneBits::DEPTH)) { // fallthrough
                    out.Subresources.NumPlanes += formatProps.isDepth ? 1 : 0;
                    out.Subresources.FirstPlane = 0;
                }
                if (in.planes == PlaneBits::ALL || (in.planes & PlaneBits::COLOR)) { // fallthrough
                    out.Subresources.NumPlanes += (!formatProps.isDepth && !formatProps.isStencil) ? 1 : 0;
                    out.Subresources.FirstPlane = 0;
                }

                // https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#d3d12_texture_barrier_flags
                out.Flags = in.before.layout == Layout::UNDEFINED ? D3D12_TEXTURE_BARRIER_FLAG_DISCARD : D3D12_TEXTURE_BARRIER_FLAG_NONE; // TODO: verify that it works
            }
        }

        // Submit
        m_GraphicsCommandList->Barrier(barriersGroupsNum, barrierGroups);
    } else
#endif
    { // Legacy barriers
        // Count
        uint32_t barrierNum = barrierDesc.bufferNum;

        for (uint32_t i = 0; i < barrierDesc.textureNum; i++) {
            const TextureBarrierDesc& barrier = barrierDesc.textures[i];
            const TextureD3D12& texture = *(TextureD3D12*)barrier.texture;
            const TextureDesc& textureDesc = texture.GetDesc();
            const Dim_t layerNum = barrier.layerNum == REMAINING ? textureDesc.layerNum : barrier.layerNum;
            const Dim_t mipNum = barrier.mipNum == REMAINING ? textureDesc.mipNum : barrier.mipNum;

            if (barrier.layerOffset == 0 && layerNum == textureDesc.layerNum && barrier.mipOffset == 0 && mipNum == textureDesc.mipNum)
                barrierNum++;
            else
                barrierNum += layerNum * mipNum;
        }

        bool isGlobalUavBarrierNeeded = false;
        for (uint32_t i = 0; i < barrierDesc.globalNum && !isGlobalUavBarrierNeeded; i++) {
            const GlobalBarrierDesc& barrier = barrierDesc.globals[i];
            if ((barrier.before.access & AccessBits::SHADER_RESOURCE_STORAGE) && (barrier.after.access & AccessBits::SHADER_RESOURCE_STORAGE))
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

        for (uint32_t i = 0; i < barrierDesc.bufferNum; i++) {
            const BufferBarrierDesc& barrier = barrierDesc.buffers[i];
            AddResourceBarrier(commandListType, *((BufferD3D12*)barrier.buffer), barrier.before.access, barrier.after.access, *ptr++, 0);
        }

        for (uint32_t i = 0; i < barrierDesc.textureNum; i++) {
            const TextureBarrierDesc& barrier = barrierDesc.textures[i];
            const TextureD3D12& texture = *(TextureD3D12*)barrier.texture;
            const TextureDesc& textureDesc = texture.GetDesc();
            const Dim_t layerNum = barrier.layerNum == REMAINING ? textureDesc.layerNum : barrier.layerNum;
            const Dim_t mipNum = barrier.mipNum == REMAINING ? textureDesc.mipNum : barrier.mipNum;

            if (barrier.layerOffset == 0 && layerNum == textureDesc.layerNum && barrier.mipOffset == 0 && mipNum == textureDesc.mipNum && barrier.planes == PlaneBits::ALL)
                AddResourceBarrier(commandListType, texture, barrier.before.access, barrier.after.access, *ptr++, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
            else {
                for (Dim_t layerOffset = barrier.layerOffset; layerOffset < barrier.layerOffset + layerNum; layerOffset++) {
                    for (Dim_t mipOffset = barrier.mipOffset; mipOffset < barrier.mipOffset + mipNum; mipOffset++) {
                        uint32_t subresource = texture.GetSubresourceIndex(layerOffset, mipOffset, barrier.planes);
                        AddResourceBarrier(commandListType, texture, barrier.before.access, barrier.after.access, *ptr++, subresource);
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

NRI_INLINE void CommandBufferD3D12::ResetQueries(QueryPool& queryPool, uint32_t, uint32_t) {
    QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    if (queryPoolD3D12.GetType() >= QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE) {
        // TODO: "bufferForAccelerationStructuresSizes" is completely hidden from a user, transition needs to be done under the hood.
        // "ResetQueries" is a good indicator that next call will be "CmdWrite*Sizes" where UAV state is needed
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        if (m_Device.GetDesc().features.enhancedBarriers) {
            D3D12_BUFFER_BARRIER barrier = {};
            barrier.SyncBefore = D3D12_BARRIER_SYNC_COPY;
            barrier.SyncAfter = D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO;
            barrier.AccessBefore = D3D12_BARRIER_ACCESS_COPY_SOURCE;
            barrier.AccessAfter = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
            barrier.pResource = queryPoolD3D12.GetBufferForAccelerationStructuresSizes();
            barrier.Offset = 0; // TODO: would be good to use "offset and "num", but API says "must be 0 and UINT64_MAX"
            barrier.Size = UINT64_MAX;

            D3D12_BARRIER_GROUP barrierGroup = {};
            barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
            barrierGroup.NumBarriers = 1;
            barrierGroup.pBufferBarriers = &barrier;

            m_GraphicsCommandList->Barrier(1, &barrierGroup);
        } else
#endif
        {
            D3D12_RESOURCE_BARRIER resourceBarrier = {};
            resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            resourceBarrier.Transition.pResource = queryPoolD3D12.GetBufferForAccelerationStructuresSizes();
            resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

            m_GraphicsCommandList->ResourceBarrier(1, &resourceBarrier);
        }
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
    QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    const BufferD3D12& bufferD3D12 = (BufferD3D12&)buffer;

    if (queryPoolD3D12.GetType() >= QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE) {
        const uint64_t srcOffset = offset * queryPoolD3D12.GetQuerySize();
        const uint64_t size = num * queryPoolD3D12.GetQuerySize();
        ID3D12Resource* bufferSrc = queryPoolD3D12.GetBufferForAccelerationStructuresSizes();

        // TODO: "bufferForAccelerationStructuresSizes" is completely hidden from a user, transition needs to be done under the hood.
        // Let's naively assume that "CopyQueries" can be called only once after potentially multiple "CmdWrite*Sizes"
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        if (m_Device.GetDesc().features.enhancedBarriers) {
            D3D12_BUFFER_BARRIER barrier = {};
            barrier.SyncBefore = D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO;
            barrier.SyncAfter = D3D12_BARRIER_SYNC_COPY;
            barrier.AccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
            barrier.AccessAfter = D3D12_BARRIER_ACCESS_COPY_SOURCE;
            barrier.pResource = queryPoolD3D12.GetBufferForAccelerationStructuresSizes();
            barrier.Offset = 0; // TODO: would be good to use "offset and "num", but API says "must be 0 and UINT64_MAX"
            barrier.Size = UINT64_MAX;

            D3D12_BARRIER_GROUP barrierGroup = {};
            barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
            barrierGroup.NumBarriers = 1;
            barrierGroup.pBufferBarriers = &barrier;

            m_GraphicsCommandList->Barrier(1, &barrierGroup);
        } else
#endif
        {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = bufferSrc;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

            m_GraphicsCommandList->ResourceBarrier(1, &barrier);
        }

        m_GraphicsCommandList->CopyBufferRegion(bufferD3D12, alignedBufferOffset, bufferSrc, srcOffset, size);
    } else
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

NRI_INLINE void CommandBufferD3D12::BuildTopLevelAccelerationStructures(const BuildTopLevelAccelerationStructureDesc* buildTopLevelAccelerationStructureDescs, uint32_t buildTopLevelAccelerationStructureDescNum) {
    static_assert(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) == sizeof(TopLevelInstance), "Mismatched sizeof");

    for (uint32_t i = 0; i < buildTopLevelAccelerationStructureDescNum; i++) {
        const BuildTopLevelAccelerationStructureDesc& in = buildTopLevelAccelerationStructureDescs[i];

        AccelerationStructureD3D12* dst = (AccelerationStructureD3D12*)in.dst;
        AccelerationStructureD3D12* src = (AccelerationStructureD3D12*)in.src;

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC out = {};
        out.DestAccelerationStructureData = dst->GetHandle();
        out.ScratchAccelerationStructureData = GetBufferAddress(in.scratchBuffer, in.scratchOffset);
        out.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        out.Inputs.Flags = GetAccelerationStructureFlags(dst->GetFlags());
        out.Inputs.NumDescs = in.instanceNum;
        out.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        out.Inputs.InstanceDescs = GetBufferAddress(in.instanceBuffer, in.instanceOffset);

        if (in.src) {
            out.SourceAccelerationStructureData = src->GetHandle();
            out.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        }

        m_GraphicsCommandList->BuildRaytracingAccelerationStructure(&out, 0, nullptr);
    }
}

NRI_INLINE void CommandBufferD3D12::BuildBottomLevelAccelerationStructures(const BuildBottomLevelAccelerationStructureDesc* buildBottomLevelAccelerationStructureDescs, uint32_t buildBottomLevelAccelerationStructureDescNum) {
    // Scratch memory
    uint32_t geometryMaxNum = 0;
    uint32_t micromapMaxNum = 0;

    for (uint32_t i = 0; i < buildBottomLevelAccelerationStructureDescNum; i++) {
        const BuildBottomLevelAccelerationStructureDesc& desc = buildBottomLevelAccelerationStructureDescs[i];

        uint32_t micromapNum = 0;
        for (uint32_t j = 0; j < desc.geometryNum; j++) {
            const BottomLevelGeometryDesc& geometryDesc = desc.geometries[i];
            if (geometryDesc.type == BottomLevelGeometryType::TRIANGLES && geometryDesc.triangles.micromap)
                micromapNum++;
        }

        geometryMaxNum = std::max(geometryMaxNum, desc.geometryNum);
        micromapMaxNum = std::max(micromapMaxNum, micromapNum);
    }

    Scratch<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs = AllocateScratch(m_Device, D3D12_RAYTRACING_GEOMETRY_DESC, geometryMaxNum);
    Scratch<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC> trianglesDescs = AllocateScratch(m_Device, D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC, micromapMaxNum);
    Scratch<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC> ommDescs = AllocateScratch(m_Device, D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC, micromapMaxNum);

    // 1 by 1
    for (uint32_t i = 0; i < buildBottomLevelAccelerationStructureDescNum; i++) {
        const BuildBottomLevelAccelerationStructureDesc& in = buildBottomLevelAccelerationStructureDescs[i];

        AccelerationStructureD3D12* dst = (AccelerationStructureD3D12*)in.dst;
        AccelerationStructureD3D12* src = (AccelerationStructureD3D12*)in.src;

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC out = {};
        out.DestAccelerationStructureData = dst->GetHandle();
        out.ScratchAccelerationStructureData = GetBufferAddress(in.scratchBuffer, in.scratchOffset);
        out.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        out.Inputs.Flags = GetAccelerationStructureFlags(dst->GetFlags());
        out.Inputs.NumDescs = in.geometryNum;
        out.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        out.Inputs.pGeometryDescs = geometryDescs;

        if (in.src) {
            out.SourceAccelerationStructureData = src->GetHandle();
            out.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        }

        ConvertBotomLevelGeometries(in.geometries, in.geometryNum, geometryDescs, trianglesDescs, ommDescs);

        m_GraphicsCommandList->BuildRaytracingAccelerationStructure(&out, 0, nullptr);
    }
}

NRI_INLINE void CommandBufferD3D12::BuildMicromaps(const BuildMicromapDesc* buildMicromapDescs, uint32_t buildMicromapDescNum) {
#ifdef NRI_D3D12_HAS_OPACITY_MICROMAP
    static_assert(sizeof(MicromapTriangle) == sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_DESC), "Type mismatch");

    uint32_t usageMaxNum = 0;
    for (uint32_t i = 0; i < buildMicromapDescNum; i++)
        usageMaxNum = std::max(usageMaxNum, ((MicromapD3D12*)buildMicromapDescs[i].dst)->GetUsageNum());

    Scratch<D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY> usages = AllocateScratch(m_Device, D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY, usageMaxNum);

    for (uint32_t i = 0; i < buildMicromapDescNum; i++) {
        const BuildMicromapDesc& in = buildMicromapDescs[i];

        MicromapD3D12* dst = (MicromapD3D12*)in.dst;

        for (uint32_t j = 0; j < dst->GetUsageNum(); j++)
            usages[j] = dst->GetUsages()[j];

        D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC opacityMicromapArrayDesc = {};
        opacityMicromapArrayDesc.NumOmmHistogramEntries = dst->GetUsageNum();
        opacityMicromapArrayDesc.pOmmHistogram = usages;
        opacityMicromapArrayDesc.InputBuffer = GetBufferAddress(in.dataBuffer, in.dataOffset);
        opacityMicromapArrayDesc.PerOmmDescs.StartAddress = GetBufferAddress(in.triangleBuffer, in.triangleOffset);
        opacityMicromapArrayDesc.PerOmmDescs.StrideInBytes = sizeof(MicromapTriangle);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC out = {};
        out.DestAccelerationStructureData = dst->GetHandle();
        out.ScratchAccelerationStructureData = GetBufferAddress(in.scratchBuffer, in.scratchOffset);
        out.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY;
        out.Inputs.Flags = GetMicromapFlags(dst->GetFlags());
        out.Inputs.NumDescs = 1;
        out.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY; // TODO: D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS support?
        out.Inputs.pOpacityMicromapArrayDesc = &opacityMicromapArrayDesc;

        m_GraphicsCommandList->BuildRaytracingAccelerationStructure(&out, 0, nullptr);
    }
#else
    MaybeUnused(buildMicromapDescs, buildMicromapDescNum);
#endif
}

NRI_INLINE void CommandBufferD3D12::CopyAccelerationStructure(AccelerationStructure& dst, const AccelerationStructure& src, CopyMode copyMode) {
    m_GraphicsCommandList->CopyRaytracingAccelerationStructure(((AccelerationStructureD3D12&)dst).GetHandle(), ((AccelerationStructureD3D12&)src).GetHandle(), GetCopyMode(copyMode));
}

NRI_INLINE void CommandBufferD3D12::CopyMicromap(Micromap& dst, const Micromap& src, CopyMode copyMode) {
    m_GraphicsCommandList->CopyRaytracingAccelerationStructure(((MicromapD3D12&)dst).GetHandle(), ((MicromapD3D12&)src).GetHandle(), GetCopyMode(copyMode));
}

NRI_INLINE void CommandBufferD3D12::WriteAccelerationStructuresSizes(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    Scratch<D3D12_GPU_VIRTUAL_ADDRESS> virtualAddresses = AllocateScratch(m_Device, D3D12_GPU_VIRTUAL_ADDRESS, accelerationStructureNum);
    for (uint32_t i = 0; i < accelerationStructureNum; i++)
        virtualAddresses[i] = ((AccelerationStructureD3D12*)accelerationStructures[i])->GetHandle();

    QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    ID3D12Resource* buffer = queryPoolD3D12.GetBufferForAccelerationStructuresSizes();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postbuildInfo = {};
    postbuildInfo.DestBuffer = buffer->GetGPUVirtualAddress() + queryPoolOffset;

    if (queryPoolD3D12.GetType() == QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE)
        postbuildInfo.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE;
    else
        postbuildInfo.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;

    m_GraphicsCommandList->EmitRaytracingAccelerationStructurePostbuildInfo(&postbuildInfo, accelerationStructureNum, virtualAddresses);
}

NRI_INLINE void CommandBufferD3D12::WriteMicromapsSizes(const Micromap* const* micromaps, uint32_t micromapNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    Scratch<D3D12_GPU_VIRTUAL_ADDRESS> virtualAddresses = AllocateScratch(m_Device, D3D12_GPU_VIRTUAL_ADDRESS, micromapNum);
    for (uint32_t i = 0; i < micromapNum; i++)
        virtualAddresses[i] = ((AccelerationStructureD3D12&)micromaps[i]).GetHandle();

    QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    ID3D12Resource* buffer = queryPoolD3D12.GetBufferForAccelerationStructuresSizes();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postbuildInfo = {};
    postbuildInfo.DestBuffer = buffer->GetGPUVirtualAddress() + queryPoolOffset;

    if (queryPoolD3D12.GetType() == QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE)
        postbuildInfo.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE;
    else
        postbuildInfo.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;

    m_GraphicsCommandList->EmitRaytracingAccelerationStructurePostbuildInfo(&postbuildInfo, micromapNum, virtualAddresses);
}

NRI_INLINE void CommandBufferD3D12::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc) {
    D3D12_DISPATCH_RAYS_DESC desc = {};

    desc.RayGenerationShaderRecord.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.raygenShader.buffer).GetGPUVA() + dispatchRaysDesc.raygenShader.offset;
    desc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    if (dispatchRaysDesc.missShaders.buffer) {
        desc.MissShaderTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.missShaders.buffer).GetGPUVA() + dispatchRaysDesc.missShaders.offset;
        desc.MissShaderTable.SizeInBytes = dispatchRaysDesc.missShaders.size;
        desc.MissShaderTable.StrideInBytes = dispatchRaysDesc.missShaders.stride;
    }

    if (dispatchRaysDesc.hitShaderGroups.buffer) {
        desc.HitGroupTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.hitShaderGroups.buffer).GetGPUVA() + dispatchRaysDesc.hitShaderGroups.offset;
        desc.HitGroupTable.SizeInBytes = dispatchRaysDesc.hitShaderGroups.size;
        desc.HitGroupTable.StrideInBytes = dispatchRaysDesc.hitShaderGroups.stride;
    }

    if (dispatchRaysDesc.callableShaders.buffer) {
        desc.CallableShaderTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.callableShaders.buffer).GetGPUVA() + dispatchRaysDesc.callableShaders.offset;
        desc.CallableShaderTable.SizeInBytes = dispatchRaysDesc.callableShaders.size;
        desc.CallableShaderTable.StrideInBytes = dispatchRaysDesc.callableShaders.stride;
    }

    desc.Width = dispatchRaysDesc.x;
    desc.Height = dispatchRaysDesc.y;
    desc.Depth = dispatchRaysDesc.z;

    m_GraphicsCommandList->DispatchRays(&desc);
}

NRI_INLINE void CommandBufferD3D12::DispatchRaysIndirect(const Buffer& buffer, uint64_t offset) {
    static_assert(sizeof(DispatchRaysIndirectDesc) == sizeof(D3D12_DISPATCH_RAYS_DESC));

    m_GraphicsCommandList->ExecuteIndirect(m_Device.GetDispatchRaysCommandSignature(), 1, (BufferD3D12&)buffer, offset, nullptr, 0);
}

NRI_INLINE void CommandBufferD3D12::DrawMeshTasks(const DrawMeshTasksDesc& drawMeshTasksDesc) {
    m_GraphicsCommandList->DispatchMesh(drawMeshTasksDesc.x, drawMeshTasksDesc.y, drawMeshTasksDesc.z);
}

NRI_INLINE void CommandBufferD3D12::DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    static_assert(sizeof(DrawMeshTasksDesc) == sizeof(D3D12_DISPATCH_MESH_ARGUMENTS));

    ID3D12Resource* pCountBuffer = nullptr;
    if (countBuffer)
        pCountBuffer = *(BufferD3D12*)countBuffer;

    m_GraphicsCommandList->ExecuteIndirect(m_Device.GetDrawMeshCommandSignature(stride), drawNum, (BufferD3D12&)buffer, offset, pCountBuffer, countBufferOffset);
}
