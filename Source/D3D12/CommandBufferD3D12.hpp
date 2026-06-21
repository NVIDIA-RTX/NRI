// © 2021 NVIDIA Corporation

static uint8_t QueryLatestInterface(ComPtr<ID3D12GraphicsCommandList>& in, ComPtr<ID3D12GraphicsCommandListBest>& out) {
    static const IID versions[] = {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        __uuidof(ID3D12GraphicsCommandList10),
        __uuidof(ID3D12GraphicsCommandList9),
        __uuidof(ID3D12GraphicsCommandList8),
        __uuidof(ID3D12GraphicsCommandList7),
#endif
        // D3D12 Ultimate initial release
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

    NRI_CHECK(n > i, "Unexpected");

    return n - i - 1;
}

static uint8_t QueryLatestInterface(ComPtr<ID3D12VideoDecodeCommandList>& in, ComPtr<ID3D12VideoDecodeCommandListBest>& out) {
    static const IID versions[] = {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        __uuidof(ID3D12VideoDecodeCommandList3),
#endif
        // D3D12 Ultimate initial release
        __uuidof(ID3D12VideoDecodeCommandList2),
        __uuidof(ID3D12VideoDecodeCommandList1),
        __uuidof(ID3D12VideoDecodeCommandList),
    };
    const uint8_t n = (uint8_t)GetCountOf(versions);

    uint8_t i = 0;
    for (; i < n; i++) {
        HRESULT hr = in->QueryInterface(versions[i], (void**)&out);
        if (SUCCEEDED(hr))
            break;
    }

    NRI_CHECK(n > i, "Unexpected");

    return n - i - 1;
}

static uint8_t QueryLatestInterface(ComPtr<ID3D12VideoEncodeCommandList>& in, ComPtr<ID3D12VideoEncodeCommandListBest>& out) {
    static const IID versions[] = {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        __uuidof(ID3D12VideoEncodeCommandList4),
        __uuidof(ID3D12VideoEncodeCommandList3),
        __uuidof(ID3D12VideoEncodeCommandList2),
#endif
        // D3D12 Ultimate initial release
        __uuidof(ID3D12VideoEncodeCommandList1),
        __uuidof(ID3D12VideoEncodeCommandList),
    };
    const uint8_t n = (uint8_t)GetCountOf(versions);

    uint8_t i = 0;
    for (; i < n; i++) {
        HRESULT hr = in->QueryInterface(versions[i], (void**)&out);
        if (SUCCEEDED(hr))
            break;
    }

    NRI_CHECK(n > i, "Unexpected");

    return n - i - 1;
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

    if (stageBits & (StageBits::FRAGMENT_SHADER | StageBits::SHADING_RATE_ATTACHMENT))
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

    if (stageBits & StageBits::VIDEO_DECODE)
        flags |= D3D12_BARRIER_SYNC_VIDEO_DECODE;

    if (stageBits & StageBits::VIDEO_ENCODE)
        flags |= D3D12_BARRIER_SYNC_VIDEO_ENCODE;

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

    if (accessBits & (AccessBits::SHADER_RESOURCE | AccessBits::SHADER_BINDING_TABLE | AccessBits::INPUT_ATTACHMENT))
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

    if (accessBits & AccessBits::VIDEO_DECODE_READ)
        flags |= D3D12_BARRIER_ACCESS_VIDEO_DECODE_READ;

    if (accessBits & AccessBits::VIDEO_DECODE_WRITE)
        flags |= D3D12_BARRIER_ACCESS_VIDEO_DECODE_WRITE;

    if (accessBits & AccessBits::VIDEO_ENCODE_READ)
        flags |= D3D12_BARRIER_ACCESS_VIDEO_ENCODE_READ;

    if (accessBits & AccessBits::VIDEO_ENCODE_WRITE)
        flags |= D3D12_BARRIER_ACCESS_VIDEO_ENCODE_WRITE;

    return flags;
}

constexpr std::array<D3D12_BARRIER_LAYOUT, (size_t)Layout::MAX_NUM> g_BarrierLayouts = {
    D3D12_BARRIER_LAYOUT_UNDEFINED,           // UNDEFINED
    D3D12_BARRIER_LAYOUT_COMMON,              // GENERAL
    D3D12_BARRIER_LAYOUT_PRESENT,             // PRESENT
    D3D12_BARRIER_LAYOUT_RENDER_TARGET,       // COLOR_ATTACHMENT
    D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE, // DEPTH_STENCIL_ATTACHMENT
    D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE, // DEPTH_READONLY_STENCIL_ATTACHMENT
    D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE, // DEPTH_ATTACHMENT_STENCIL_READONLY
    D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ,  // DEPTH_STENCIL_READONLY
    D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE, // SHADING_RATE_ATTACHMENT
    D3D12_BARRIER_LAYOUT_RENDER_TARGET,       // INPUT_ATTACHMENT
    D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,     // SHADER_RESOURCE
    D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,    // SHADER_RESOURCE_STORAGE
    D3D12_BARRIER_LAYOUT_COPY_SOURCE,         // COPY_SOURCE
    D3D12_BARRIER_LAYOUT_COPY_DEST,           // COPY_DESTINATION
    D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE,      // RESOLVE_SOURCE
    D3D12_BARRIER_LAYOUT_RESOLVE_DEST,        // RESOLVE_DESTINATION
    D3D12_BARRIER_LAYOUT_VIDEO_DECODE_WRITE,  // VIDEO_DECODE_DST
    D3D12_BARRIER_LAYOUT_VIDEO_DECODE_READ,   // VIDEO_DECODE_DPB
    D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_READ,   // VIDEO_ENCODE_SRC
    D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_WRITE,  // VIDEO_ENCODE_DPB
};
NRI_VALIDATE_ARRAY(g_BarrierLayouts);

constexpr D3D12_BARRIER_LAYOUT GetBarrierLayout(Layout layout, AccessBits accessBits) {
    // Special case
    if (layout == Layout::INPUT_ATTACHMENT && (accessBits & AccessBits::INPUT_ATTACHMENT) != 0)
        return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;

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

    if (accessBits & AccessBits::SHADER_RESOURCE) {
        resourceStates |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        if (commandListType == D3D12_COMMAND_LIST_TYPE_DIRECT)
            resourceStates |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    if (accessBits & AccessBits::INPUT_ATTACHMENT)
        resourceStates |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    if (accessBits & AccessBits::SHADER_BINDING_TABLE)
        resourceStates |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    if (accessBits & (AccessBits::SHADER_RESOURCE_STORAGE | AccessBits::ACCELERATION_STRUCTURE | AccessBits::MICROMAP | AccessBits::SCRATCH_BUFFER | AccessBits::CLEAR_STORAGE))
        resourceStates |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    if (accessBits & AccessBits::COPY_SOURCE)
        resourceStates |= D3D12_RESOURCE_STATE_COPY_SOURCE;

    if (accessBits & AccessBits::COPY_DESTINATION)
        resourceStates |= D3D12_RESOURCE_STATE_COPY_DEST;

    if (accessBits & AccessBits::RESOLVE_SOURCE)
        resourceStates |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;

    if (accessBits & AccessBits::RESOLVE_DESTINATION)
        resourceStates |= D3D12_RESOURCE_STATE_RESOLVE_DEST;

    if (accessBits & AccessBits::VIDEO_DECODE_READ)
        resourceStates |= D3D12_RESOURCE_STATE_VIDEO_DECODE_READ;

    if (accessBits & AccessBits::VIDEO_DECODE_WRITE)
        resourceStates |= D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE;

    if (accessBits & AccessBits::VIDEO_ENCODE_READ)
        resourceStates |= D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ;

    if (accessBits & AccessBits::VIDEO_ENCODE_WRITE)
        resourceStates |= D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE;

    return resourceStates;
}

static inline bool HasVideoResourceState(D3D12_RESOURCE_STATES resourceStates) {
    return (resourceStates & (D3D12_RESOURCE_STATE_VIDEO_DECODE_READ | D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE | D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ | D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE)) != 0;
}

static inline void SetTransitionResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, D3D12_RESOURCE_BARRIER& resourceBarrier, uint32_t subresource) {
    resourceBarrier = {};
    resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resourceBarrier.Transition.pResource = resource;
    resourceBarrier.Transition.StateBefore = before;
    resourceBarrier.Transition.StateAfter = after;
    resourceBarrier.Transition.Subresource = subresource;
}

static uint32_t AddResourceBarrier(D3D12_COMMAND_LIST_TYPE commandListType, ID3D12Resource* resource, AccessBits before, AccessBits after, D3D12_RESOURCE_BARRIER& resourceBarrier, uint32_t subresource) {
    const D3D12_RESOURCE_STATES resourceStateBefore = GetResourceStates(before, commandListType);
    const D3D12_RESOURCE_STATES resourceStateAfter = GetResourceStates(after, commandListType);

    if (resourceStateBefore == resourceStateAfter) {
        if (resourceStateBefore != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            return 0;

        resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        resourceBarrier.UAV.pResource = resource;
    } else {
        SetTransitionResourceBarrier(resource, resourceStateBefore, resourceStateAfter, resourceBarrier, subresource);
    }

    return 1;
}

static uint32_t AddVideoBufferResourceBarriers(D3D12_COMMAND_LIST_TYPE commandListType, ID3D12Resource* resource, AccessBits before, AccessBits after, D3D12_RESOURCE_BARRIER* resourceBarriers, uint32_t subresource) {
    const D3D12_RESOURCE_STATES resourceStateBefore = GetResourceStates(before, commandListType);
    const D3D12_RESOURCE_STATES resourceStateAfter = GetResourceStates(after, commandListType);

    if (resourceStateBefore == resourceStateAfter && HasVideoResourceState(resourceStateBefore)) {
        SetTransitionResourceBarrier(resource, resourceStateBefore, D3D12_RESOURCE_STATE_COMMON, resourceBarriers[0], subresource);
        SetTransitionResourceBarrier(resource, D3D12_RESOURCE_STATE_COMMON, resourceStateAfter, resourceBarriers[1], subresource);

        return 2;
    }

    return AddResourceBarrier(commandListType, resource, before, after, resourceBarriers[0], subresource);
}

static inline bool HasVideoBufferUsage(BufferUsageBits usage) {
    return usage & (BufferUsageBits::VIDEO_DECODE | BufferUsageBits::VIDEO_ENCODE);
}

static inline void ConvertRects(const Rect* in, uint32_t rectNum, D3D12_RECT* out) {
    for (uint32_t i = 0; i < rectNum; i++) {
        out[i].left = in[i].x;
        out[i].top = in[i].y;
        out[i].right = in[i].x + in[i].width;
        out[i].bottom = in[i].y + in[i].height;
    }
}

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
static inline void FillResolveBarrier(bool isSource, DescriptorD3D12& descriptor, PlaneBits planeBits, D3D12_TEXTURE_BARRIER& textureBarrier) {
    const TexViewDesc& srcDesc = descriptor.GetTexViewDesc();

    textureBarrier = {};
    textureBarrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
    textureBarrier.SyncAfter = D3D12_BARRIER_SYNC_RESOLVE;
    textureBarrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
    textureBarrier.AccessAfter = isSource ? D3D12_BARRIER_ACCESS_RESOLVE_SOURCE : D3D12_BARRIER_ACCESS_RESOLVE_DEST;
    textureBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
    textureBarrier.LayoutAfter = isSource ? D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE : D3D12_BARRIER_LAYOUT_RESOLVE_DEST;
    textureBarrier.pResource = descriptor.GetResource();
    textureBarrier.Subresources.IndexOrFirstMipLevel = srcDesc.mipOffset;
    textureBarrier.Subresources.NumMipLevels = 1;
    textureBarrier.Subresources.FirstArraySlice = srcDesc.layerOffset;
    textureBarrier.Subresources.NumArraySlices = srcDesc.layerNum;
    textureBarrier.Subresources.NumPlanes = 1; // TODO: can be optimized for depth-stencil resolve
    textureBarrier.Subresources.FirstPlane = (planeBits & PlaneBits::STENCIL) ? 1 : 0;
}
#endif

static inline void FillLegacyResolveBarrier(bool isSource, DescriptorD3D12& descriptor, uint32_t subresource, D3D12_RESOURCE_BARRIER& resourceBarrier) {
    resourceBarrier = {};
    resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resourceBarrier.Transition.pResource = descriptor.GetResource();
    resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    resourceBarrier.Transition.StateAfter = isSource ? D3D12_RESOURCE_STATE_RESOLVE_SOURCE : D3D12_RESOURCE_STATE_RESOLVE_DEST;
    resourceBarrier.Transition.Subresource = subresource;
}

constexpr std::array<D3D12_RESOLVE_MODE, (size_t)ResolveOp::MAX_NUM> g_ResolveOps = {
    D3D12_RESOLVE_MODE_AVERAGE, // AVERAGE
    D3D12_RESOLVE_MODE_MIN,     // MIN
    D3D12_RESOLVE_MODE_MAX,     // MAX
};
NRI_VALIDATE_ARRAY(g_ResolveOps);

constexpr D3D12_RESOLVE_MODE GetResolveOp(ResolveOp resolveOp) {
    return g_ResolveOps[(size_t)resolveOp];
}

void CommandBufferD3D12::SetDebugName(const char* name) {
    NRI_SET_D3D_DEBUG_OBJECT_NAME(m_CommandList.GetInterface(), name);
}

Result CommandBufferD3D12::Create(D3D12_COMMAND_LIST_TYPE commandListType, ID3D12CommandAllocator* commandAllocator) {
    if (commandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE) {
        ComPtr<ID3D12VideoDecodeCommandList> commandList;
        HRESULT hr = m_Device->CreateCommandList(NODE_MASK, commandListType, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommandList");

        ComPtr<ID3D12VideoDecodeCommandListBest> commandListBest;
        m_Version = QueryLatestInterface(commandList, commandListBest);

        hr = commandListBest->Close();
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDecodeCommandList::Close");

        m_CommandList = commandListBest;
    } else if (commandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE) {
        ComPtr<ID3D12VideoEncodeCommandList> commandList;
        HRESULT hr = m_Device->CreateCommandList(NODE_MASK, commandListType, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommandList");

        ComPtr<ID3D12VideoEncodeCommandListBest> commandListBest;
        m_Version = QueryLatestInterface(commandList, commandListBest);

        hr = commandListBest->Close();
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoEncodeCommandList::Close");

        m_CommandList = commandListBest;
    } else {
        ComPtr<ID3D12GraphicsCommandList> commandList;
        HRESULT hr = m_Device->CreateCommandList(NODE_MASK, commandListType, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::CreateCommandList");

        ComPtr<ID3D12GraphicsCommandListBest> commandListBest;
        m_Version = QueryLatestInterface(commandList, commandListBest);

        hr = commandListBest->Close();
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12GraphicsCommandList::Close");

        m_CommandList = commandListBest;
    }

    m_CommandListType = commandListType;
    m_CommandAllocator = commandAllocator;

    return Result::SUCCESS;
}

Result CommandBufferD3D12::Create(const CommandBufferD3D12Desc& commandBufferD3D12Desc) {
    D3D12_COMMAND_LIST_TYPE commandListType = ((ID3D12CommandList*)commandBufferD3D12Desc.d3d12CommandList)->GetType();

    if (commandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE) {
        ComPtr<ID3D12VideoDecodeCommandList> commandList = (ID3D12VideoDecodeCommandList*)commandBufferD3D12Desc.d3d12CommandList;

        ComPtr<ID3D12VideoDecodeCommandListBest> commandListBest;
        m_Version = QueryLatestInterface(commandList, commandListBest);

        m_CommandList = commandListBest;
    } else if (commandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE) {
        ComPtr<ID3D12VideoEncodeCommandList> commandList = (ID3D12VideoEncodeCommandList*)commandBufferD3D12Desc.d3d12CommandList;

        ComPtr<ID3D12VideoEncodeCommandListBest> commandListBest;
        m_Version = QueryLatestInterface(commandList, commandListBest);

        m_CommandList = commandListBest;
    } else {
        ComPtr<ID3D12GraphicsCommandList> commandList = (ID3D12GraphicsCommandList*)commandBufferD3D12Desc.d3d12CommandList;

        ComPtr<ID3D12GraphicsCommandListBest> commandListBest;
        m_Version = QueryLatestInterface(commandList, commandListBest);

        m_CommandList = commandListBest;
    }

    // TODO: what if opened?

    m_CommandAllocator = commandBufferD3D12Desc.d3d12CommandAllocator;
    m_CommandListType = commandListType;

    return Result::SUCCESS;
}

NRI_INLINE Result CommandBufferD3D12::Begin(const DescriptorPool* descriptorPool) {
    if (m_CommandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE) {
        HRESULT hr = GetVideoDecodeCommandList()->Reset(m_CommandAllocator);
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDecodeCommandList::Reset");

        return Result::SUCCESS;
    }

    if (m_CommandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE) {
        HRESULT hr = GetVideoEncodeCommandList()->Reset(m_CommandAllocator);
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoEncodeCommandList::Reset");

        return Result::SUCCESS;
    }

    HRESULT hr = GetGraphicsCommandList()->Reset(m_CommandAllocator, nullptr);
    NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12GraphicsCommandList::Reset");

    if (descriptorPool)
        SetDescriptorPool(*descriptorPool);

    m_PipelineLayout = nullptr;
    m_PipelineBindPoint = BindPoint::INHERIT;

    ResetAttachments();

    return Result::SUCCESS;
}

NRI_INLINE Result CommandBufferD3D12::End() {
    HRESULT hr = S_OK;
    if (m_CommandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE) {
        hr = GetVideoDecodeCommandList()->Close();
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDecodeCommandList::Close");
    } else if (m_CommandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE) {
        hr = GetVideoEncodeCommandList()->Close();
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoEncodeCommandList::Close");
    } else {
        hr = GetGraphicsCommandList()->Close();
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12GraphicsCommandList::Close");
    }

    return Result::SUCCESS;
}

static uint32_t GetVideoDecodeAV1ReferenceNameIndexD3D12(VideoAV1ReferenceName name) {
    switch (name) {
        case VideoAV1ReferenceName::NONE:
            return 7;
        case VideoAV1ReferenceName::LAST:
            return 0;
        case VideoAV1ReferenceName::LAST2:
            return 1;
        case VideoAV1ReferenceName::LAST3:
            return 2;
        case VideoAV1ReferenceName::GOLDEN:
            return 3;
        case VideoAV1ReferenceName::BWDREF:
            return 4;
        case VideoAV1ReferenceName::ALTREF2:
            return 5;
        case VideoAV1ReferenceName::ALTREF:
            return 6;
        default:
            return 7;
    }
}

static uint8_t GetVideoDecodeAV1FrameTypeD3D12(VideoEncodeFrameType frameType) {
    return frameType == VideoEncodeFrameType::IDR || frameType == VideoEncodeFrameType::I ? 0 : 1;
}

struct VideoDecodeReferenceLayoutD3D12 {
    uint32_t slotCount = 0;
    uint32_t failingReference = 0;
    bool duplicateSlot = false;
};

inline bool GetVideoDecodeReferenceLayoutD3D12(const VideoReference* references, uint32_t referenceNum, VideoDecodeReferenceLayoutD3D12& layout) {
    layout = {};
    std::array<bool, VIDEO_DECODE_MAX_PIC_ENTRY_SLOT + 1> usedSlots = {};

    for (uint32_t i = 0; i < referenceNum; i++) {
        const uint32_t slot = references[i].slot;
        if (slot > VIDEO_DECODE_MAX_PIC_ENTRY_SLOT) {
            layout.failingReference = i;
            return false;
        }

        if (usedSlots[slot]) {
            layout.failingReference = i;
            layout.duplicateSlot = true;
            return false;
        }

        usedSlots[slot] = true;
        layout.slotCount = std::max(layout.slotCount, slot + 1);
    }

    return true;
}

inline const VideoH264SequenceParameterSetDesc* FindVideoH264SequenceParameterSetD3D12(const VideoH264SessionParametersDesc& parameters, uint8_t id) {
    if (!parameters.sequenceParameterSets)
        return nullptr;

    for (uint32_t i = 0; i < parameters.sequenceParameterSetNum; i++) {
        if (parameters.sequenceParameterSets[i].sequenceParameterSetId == id)
            return &parameters.sequenceParameterSets[i];
    }

    return nullptr;
}

inline const VideoH264PictureParameterSetDesc* FindVideoH264PictureParameterSetD3D12(const VideoH264SessionParametersDesc& parameters, uint8_t id) {
    if (!parameters.pictureParameterSets)
        return nullptr;

    for (uint32_t i = 0; i < parameters.pictureParameterSetNum; i++) {
        if (parameters.pictureParameterSets[i].pictureParameterSetId == id)
            return &parameters.pictureParameterSets[i];
    }

    return nullptr;
}

inline bool BuildVideoDecodeH264ArgumentsD3D12(const VideoH264SessionParametersDesc& parameters, const VideoH264DecodePictureDesc& pictureDesc, uint64_t bitstreamSize,
    uint32_t dstSlot, DXVA_PicParams_H264& pictureParameters, DXVA_Qmatrix_H264& inverseQuantizationMatrix, DXVA_Slice_H264_Short* slices, uint32_t sliceNum) {
    if (sliceNum == 0 || sliceNum != pictureDesc.sliceOffsetNum || !pictureDesc.sliceOffsets || !slices)
        return false;

    if (pictureDesc.referenceNum > 16 || (pictureDesc.referenceNum && !pictureDesc.references))
        return false;

    uint16_t usedReferenceSlots = 0;
    for (uint32_t i = 0; i < pictureDesc.referenceNum; i++) {
        const VideoH264DecodeReferenceDesc& reference = pictureDesc.references[i];
        if (reference.slot >= 16 || (usedReferenceSlots & (1u << reference.slot)))
            return false;
        usedReferenceSlots |= 1u << reference.slot;
    }

    const VideoH264PictureParameterSetDesc* pps = FindVideoH264PictureParameterSetD3D12(parameters, pictureDesc.pictureParameterSetId);
    if (!pps)
        return false;

    const VideoH264SequenceParameterSetDesc* sps = FindVideoH264SequenceParameterSetD3D12(parameters, pictureDesc.sequenceParameterSetId);
    if (!sps || pps->sequenceParameterSetId != sps->sequenceParameterSetId)
        return false;

    if (bitstreamSize > UINT32_MAX)
        return false;

    const bool frameMbsOnly = !!(sps->flags & VideoH264SequenceParameterSetBits::FRAME_MBS_ONLY);
    const uint32_t frameHeightInMbs = (uint32_t)(sps->pictureHeightInMapUnitsMinus1 + 1u) * (frameMbsOnly ? 1u : 2u);
    if (frameHeightInMbs == 0 || frameHeightInMbs > UINT16_MAX)
        return false;

    pictureParameters = {};
    pictureParameters.wFrameWidthInMbsMinus1 = sps->pictureWidthInMbsMinus1;
    pictureParameters.wFrameHeightInMbsMinus1 = (uint16_t)(frameHeightInMbs - 1u);
    if (dstSlot > 0x7F)
        return false;

    pictureParameters.CurrPic.bPicEntry = (uint8_t)dstSlot;
    pictureParameters.CurrPic.AssociatedFlag = !!(pictureDesc.flags & VideoH264DecodePictureBits::BOTTOM_FIELD);
    pictureParameters.num_ref_frames = sps->referenceFrameNum;
    pictureParameters.field_pic_flag = !!(pictureDesc.flags & VideoH264DecodePictureBits::FIELD_PICTURE);
    pictureParameters.MbaffFrameFlag = !!(sps->flags & VideoH264SequenceParameterSetBits::MB_ADAPTIVE_FRAME_FIELD) && !pictureParameters.field_pic_flag;
    pictureParameters.chroma_format_idc = sps->chromaFormatIdc;
    pictureParameters.RefPicFlag = !!(pictureDesc.flags & VideoH264DecodePictureBits::REFERENCE);
    pictureParameters.constrained_intra_pred_flag = !!(pps->flags & VideoH264PictureParameterSetBits::CONSTRAINED_INTRA_PRED);
    pictureParameters.weighted_pred_flag = !!(pps->flags & VideoH264PictureParameterSetBits::WEIGHTED_PRED);
    pictureParameters.weighted_bipred_idc = pps->weightedBipredIdc;
    pictureParameters.MbsConsecutiveFlag = 1;
    pictureParameters.frame_mbs_only_flag = frameMbsOnly;
    pictureParameters.transform_8x8_mode_flag = !!(pps->flags & VideoH264PictureParameterSetBits::TRANSFORM_8X8_MODE);
    pictureParameters.MinLumaBipredSize8x8Flag = sps->levelIdc >= 31;
    pictureParameters.IntraPicFlag = !!(pictureDesc.flags & VideoH264DecodePictureBits::INTRA);
    pictureParameters.bit_depth_luma_minus8 = sps->bitDepthLumaMinus8;
    pictureParameters.bit_depth_chroma_minus8 = sps->bitDepthChromaMinus8;
    pictureParameters.Reserved16Bits = 3;
    pictureParameters.StatusReportFeedbackNumber = 1;
    for (uint32_t i = 0; i < 16; i++)
        pictureParameters.RefFrameList[i].bPicEntry = 0xff;
    for (uint32_t i = 0; i < pictureDesc.referenceNum; i++) {
        const VideoH264DecodeReferenceDesc& reference = pictureDesc.references[i];
        pictureParameters.RefFrameList[reference.slot].Index7Bits = (UCHAR)reference.slot;
        pictureParameters.RefFrameList[reference.slot].AssociatedFlag = !!(reference.flags & VideoH264DecodeReferenceBits::LONG_TERM);
        pictureParameters.FieldOrderCntList[reference.slot][0] = reference.topFieldOrderCount;
        pictureParameters.FieldOrderCntList[reference.slot][1] = reference.bottomFieldOrderCount;
        pictureParameters.FrameNumList[reference.slot] = (USHORT)reference.frameNum;
        if (reference.flags & VideoH264DecodeReferenceBits::TOP_FIELD)
            pictureParameters.UsedForReferenceFlags |= 1u << (reference.slot * 2);
        if (reference.flags & VideoH264DecodeReferenceBits::BOTTOM_FIELD)
            pictureParameters.UsedForReferenceFlags |= 2u << (reference.slot * 2);
        if (reference.flags & VideoH264DecodeReferenceBits::NON_EXISTING)
            pictureParameters.NonExistingFrameFlags |= 1u << reference.slot;
    }
    pictureParameters.CurrFieldOrderCnt[0] = pictureDesc.topFieldOrderCount;
    pictureParameters.CurrFieldOrderCnt[1] = pictureDesc.bottomFieldOrderCount;
    pictureParameters.pic_init_qs_minus26 = pps->pictureInitQsMinus26;
    pictureParameters.chroma_qp_index_offset = pps->chromaQpIndexOffset;
    pictureParameters.second_chroma_qp_index_offset = pps->secondChromaQpIndexOffset;
    pictureParameters.ContinuationFlag = 1;
    pictureParameters.pic_init_qp_minus26 = pps->pictureInitQpMinus26;
    pictureParameters.num_ref_idx_l0_active_minus1 = pps->refIndexL0DefaultActiveMinus1;
    pictureParameters.num_ref_idx_l1_active_minus1 = pps->refIndexL1DefaultActiveMinus1;
    pictureParameters.frame_num = pictureDesc.frameNum;
    pictureParameters.log2_max_frame_num_minus4 = sps->log2MaxFrameNumMinus4;
    pictureParameters.pic_order_cnt_type = sps->pictureOrderCountType;
    pictureParameters.log2_max_pic_order_cnt_lsb_minus4 = sps->log2MaxPictureOrderCountLsbMinus4;
    pictureParameters.delta_pic_order_always_zero_flag = !!(sps->flags & VideoH264SequenceParameterSetBits::DELTA_PIC_ORDER_ALWAYS_ZERO);
    pictureParameters.direct_8x8_inference_flag = !!(sps->flags & VideoH264SequenceParameterSetBits::DIRECT_8X8_INFERENCE);
    pictureParameters.entropy_coding_mode_flag = !!(pps->flags & VideoH264PictureParameterSetBits::ENTROPY_CODING_MODE);
    pictureParameters.pic_order_present_flag = !!(pps->flags & VideoH264PictureParameterSetBits::BOTTOM_FIELD_PIC_ORDER_IN_FRAME);
    pictureParameters.deblocking_filter_control_present_flag = !!(pps->flags & VideoH264PictureParameterSetBits::DEBLOCKING_FILTER_CONTROL_PRESENT);
    pictureParameters.redundant_pic_cnt_present_flag = !!(pps->flags & VideoH264PictureParameterSetBits::REDUNDANT_PIC_CNT_PRESENT);

    inverseQuantizationMatrix = {};
    std::memset(inverseQuantizationMatrix.bScalingLists4x4, 16, sizeof(inverseQuantizationMatrix.bScalingLists4x4));
    std::memset(inverseQuantizationMatrix.bScalingLists8x8, 16, sizeof(inverseQuantizationMatrix.bScalingLists8x8));

    for (uint32_t i = 0; i < sliceNum; i++) {
        const uint32_t offset = pictureDesc.sliceOffsets[i];
        if (offset >= bitstreamSize || (i + 1 < sliceNum && pictureDesc.sliceOffsets[i + 1] <= offset))
            return false;

        const uint64_t nextOffset = i + 1 < sliceNum ? pictureDesc.sliceOffsets[i + 1] : bitstreamSize;
        const uint64_t size = nextOffset - offset;
        if (size > UINT32_MAX)
            return false;

        slices[i] = {};
        slices[i].BSNALunitDataLocation = offset;
        slices[i].SliceBytesInBuffer = (UINT)size;
    }

    return true;
}

inline const VideoH265SequenceParameterSetDesc* FindVideoH265SequenceParameterSetD3D12(const VideoH265SessionParametersDesc& parameters, uint8_t id) {
    if (!parameters.sequenceParameterSets)
        return nullptr;

    for (uint32_t i = 0; i < parameters.sequenceParameterSetNum; i++) {
        if (parameters.sequenceParameterSets[i].sequenceParameterSetId == id)
            return &parameters.sequenceParameterSets[i];
    }

    return nullptr;
}

inline const VideoH265PictureParameterSetDesc* FindVideoH265PictureParameterSetD3D12(const VideoH265SessionParametersDesc& parameters, uint8_t id) {
    if (!parameters.pictureParameterSets)
        return nullptr;

    for (uint32_t i = 0; i < parameters.pictureParameterSetNum; i++) {
        if (parameters.pictureParameterSets[i].pictureParameterSetId == id)
            return &parameters.pictureParameterSets[i];
    }

    return nullptr;
}

inline void FillVideoH265ScalingListsD3D12(DXVA_Qmatrix_HEVC& matrix, const VideoH265ScalingListsDesc* scalingLists) {
    matrix = {};
    if (scalingLists) {
        std::memcpy(matrix.ucScalingLists0, scalingLists->scalingList4x4, sizeof(matrix.ucScalingLists0));
        std::memcpy(matrix.ucScalingLists1, scalingLists->scalingList8x8, sizeof(matrix.ucScalingLists1));
        std::memcpy(matrix.ucScalingLists2, scalingLists->scalingList16x16, sizeof(matrix.ucScalingLists2));
        std::memcpy(matrix.ucScalingLists3, scalingLists->scalingList32x32, sizeof(matrix.ucScalingLists3));
        std::memcpy(matrix.ucScalingListDCCoefSizeID2, scalingLists->scalingListDCCoef16x16, sizeof(matrix.ucScalingListDCCoefSizeID2));
        std::memcpy(matrix.ucScalingListDCCoefSizeID3, scalingLists->scalingListDCCoef32x32, sizeof(matrix.ucScalingListDCCoefSizeID3));

        return;
    }

    std::memset(matrix.ucScalingLists0, 16, sizeof(matrix.ucScalingLists0));
    std::memset(matrix.ucScalingLists1, 16, sizeof(matrix.ucScalingLists1));
    std::memset(matrix.ucScalingLists2, 16, sizeof(matrix.ucScalingLists2));
    std::memset(matrix.ucScalingLists3, 16, sizeof(matrix.ucScalingLists3));
    std::memset(matrix.ucScalingListDCCoefSizeID2, 16, sizeof(matrix.ucScalingListDCCoefSizeID2));
    std::memset(matrix.ucScalingListDCCoefSizeID3, 16, sizeof(matrix.ucScalingListDCCoefSizeID3));
}

inline bool BuildVideoDecodeH265ArgumentsD3D12(const VideoH265SessionParametersDesc& parameters, const VideoH265DecodePictureDesc& pictureDesc, uint64_t bitstreamSize,
    uint32_t dstSlot, DXVA_PicParams_HEVC& pictureParameters, DXVA_Qmatrix_HEVC& inverseQuantizationMatrix, DXVA_Slice_HEVC_Short* slices, uint32_t sliceNum) {
    if (sliceNum == 0 || sliceNum != pictureDesc.sliceSegmentOffsetNum || !pictureDesc.sliceSegmentOffsets || !slices)
        return false;

    if (pictureDesc.referenceNum > VIDEO_HEVC_MAX_REFERENCE_NUM || (pictureDesc.referenceNum && !pictureDesc.references))
        return false;

    if (dstSlot > VIDEO_DECODE_MAX_PIC_ENTRY_SLOT || bitstreamSize > UINT32_MAX)
        return false;

    const VideoH265PictureParameterSetDesc* pps = FindVideoH265PictureParameterSetD3D12(parameters, pictureDesc.pictureParameterSetId);
    if (!pps)
        return false;

    const VideoH265SequenceParameterSetDesc* sps = FindVideoH265SequenceParameterSetD3D12(parameters, pictureDesc.sequenceParameterSetId);
    if (!sps || pps->sequenceParameterSetId != sps->sequenceParameterSetId || pps->videoParameterSetId != sps->videoParameterSetId)
        return false;

    const uint32_t log2MinCbSize = sps->log2MinLumaCodingBlockSizeMinus3 + 3u;
    if (log2MinCbSize >= 16 || sps->pictureWidthInLumaSamples == 0 || sps->pictureHeightInLumaSamples == 0)
        return false;

    pictureParameters = {};
    pictureParameters.PicWidthInMinCbsY = (USHORT)((sps->pictureWidthInLumaSamples + (1u << log2MinCbSize) - 1u) >> log2MinCbSize);
    pictureParameters.PicHeightInMinCbsY = (USHORT)((sps->pictureHeightInLumaSamples + (1u << log2MinCbSize) - 1u) >> log2MinCbSize);
    pictureParameters.CurrPic.Index7Bits = (UCHAR)dstSlot;
    pictureParameters.CurrPic.AssociatedFlag = 0;
    pictureParameters.chroma_format_idc = sps->chromaFormatIdc;
    pictureParameters.separate_colour_plane_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::SEPARATE_COLOUR_PLANE);
    pictureParameters.bit_depth_luma_minus8 = sps->bitDepthLumaMinus8;
    pictureParameters.bit_depth_chroma_minus8 = sps->bitDepthChromaMinus8;
    pictureParameters.log2_max_pic_order_cnt_lsb_minus4 = sps->log2MaxPictureOrderCountLsbMinus4;
    pictureParameters.NoPicReorderingFlag = sps->decPicBufMgr.maxNumReorderPics[0] == 0;
    pictureParameters.sps_max_dec_pic_buffering_minus1 = sps->decPicBufMgr.maxDecPicBufferingMinus1[0];
    pictureParameters.log2_min_luma_coding_block_size_minus3 = sps->log2MinLumaCodingBlockSizeMinus3;
    pictureParameters.log2_diff_max_min_luma_coding_block_size = sps->log2DiffMaxMinLumaCodingBlockSize;
    pictureParameters.log2_min_transform_block_size_minus2 = sps->log2MinLumaTransformBlockSizeMinus2;
    pictureParameters.log2_diff_max_min_transform_block_size = sps->log2DiffMaxMinLumaTransformBlockSize;
    pictureParameters.max_transform_hierarchy_depth_inter = sps->maxTransformHierarchyDepthInter;
    pictureParameters.max_transform_hierarchy_depth_intra = sps->maxTransformHierarchyDepthIntra;
    pictureParameters.num_short_term_ref_pic_sets = sps->numShortTermRefPicSets;
    pictureParameters.num_long_term_ref_pics_sps = sps->numLongTermRefPicsSps;
    pictureParameters.num_ref_idx_l0_default_active_minus1 = pps->refIndexL0DefaultActiveMinus1;
    pictureParameters.num_ref_idx_l1_default_active_minus1 = pps->refIndexL1DefaultActiveMinus1;
    pictureParameters.init_qp_minus26 = pps->initQpMinus26;
    pictureParameters.ucNumDeltaPocsOfRefRpsIdx = pictureDesc.numDeltaPocsOfRefRpsIdx;
    pictureParameters.wNumBitsForShortTermRPSInSlice = pictureDesc.numBitsForShortTermRefPicSetInSlice;
    pictureParameters.scaling_list_enabled_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::SCALING_LIST_ENABLED);
    pictureParameters.amp_enabled_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::AMP_ENABLED);
    pictureParameters.sample_adaptive_offset_enabled_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::SAMPLE_ADAPTIVE_OFFSET_ENABLED);
    pictureParameters.pcm_enabled_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::PCM_ENABLED);
    pictureParameters.pcm_sample_bit_depth_luma_minus1 = sps->pcmSampleBitDepthLumaMinus1;
    pictureParameters.pcm_sample_bit_depth_chroma_minus1 = sps->pcmSampleBitDepthChromaMinus1;
    pictureParameters.log2_min_pcm_luma_coding_block_size_minus3 = sps->log2MinPcmLumaCodingBlockSizeMinus3;
    pictureParameters.log2_diff_max_min_pcm_luma_coding_block_size = sps->log2DiffMaxMinPcmLumaCodingBlockSize;
    pictureParameters.pcm_loop_filter_disabled_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::PCM_LOOP_FILTER_DISABLED);
    pictureParameters.long_term_ref_pics_present_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::LONG_TERM_REF_PICS_PRESENT);
    pictureParameters.sps_temporal_mvp_enabled_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::TEMPORAL_MVP_ENABLED);
    pictureParameters.strong_intra_smoothing_enabled_flag = !!(sps->flags & VideoH265SequenceParameterSetBits::STRONG_INTRA_SMOOTHING_ENABLED);
    pictureParameters.dependent_slice_segments_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::DEPENDENT_SLICE_SEGMENTS_ENABLED);
    pictureParameters.output_flag_present_flag = !!(pps->flags & VideoH265PictureParameterSetBits::OUTPUT_FLAG_PRESENT);
    pictureParameters.num_extra_slice_header_bits = pps->numExtraSliceHeaderBits;
    pictureParameters.sign_data_hiding_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::SIGN_DATA_HIDING_ENABLED);
    pictureParameters.cabac_init_present_flag = !!(pps->flags & VideoH265PictureParameterSetBits::CABAC_INIT_PRESENT);
    pictureParameters.constrained_intra_pred_flag = !!(pps->flags & VideoH265PictureParameterSetBits::CONSTRAINED_INTRA_PRED);
    pictureParameters.transform_skip_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::TRANSFORM_SKIP_ENABLED);
    pictureParameters.cu_qp_delta_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::CU_QP_DELTA_ENABLED);
    pictureParameters.pps_slice_chroma_qp_offsets_present_flag = !!(pps->flags & VideoH265PictureParameterSetBits::SLICE_CHROMA_QP_OFFSETS_PRESENT);
    pictureParameters.weighted_pred_flag = !!(pps->flags & VideoH265PictureParameterSetBits::WEIGHTED_PRED);
    pictureParameters.weighted_bipred_flag = !!(pps->flags & VideoH265PictureParameterSetBits::WEIGHTED_BIPRED);
    pictureParameters.transquant_bypass_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::TRANSQUANT_BYPASS_ENABLED);
    pictureParameters.tiles_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::TILES_ENABLED);
    pictureParameters.entropy_coding_sync_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::ENTROPY_CODING_SYNC_ENABLED);
    pictureParameters.uniform_spacing_flag = !!(pps->flags & VideoH265PictureParameterSetBits::UNIFORM_SPACING);
    pictureParameters.loop_filter_across_tiles_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::LOOP_FILTER_ACROSS_TILES_ENABLED);
    pictureParameters.pps_loop_filter_across_slices_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::LOOP_FILTER_ACROSS_SLICES_ENABLED);
    pictureParameters.deblocking_filter_override_enabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::DEBLOCKING_FILTER_OVERRIDE_ENABLED);
    pictureParameters.pps_deblocking_filter_disabled_flag = !!(pps->flags & VideoH265PictureParameterSetBits::DEBLOCKING_FILTER_DISABLED);
    pictureParameters.lists_modification_present_flag = !!(pps->flags & VideoH265PictureParameterSetBits::LISTS_MODIFICATION_PRESENT);
    pictureParameters.slice_segment_header_extension_present_flag = !!(pps->flags & VideoH265PictureParameterSetBits::SLICE_SEGMENT_HEADER_EXTENSION_PRESENT);
    pictureParameters.IrapPicFlag = !!(pictureDesc.flags & VideoH265DecodePictureBits::IRAP);
    pictureParameters.IdrPicFlag = !!(pictureDesc.flags & VideoH265DecodePictureBits::IDR);
    pictureParameters.IntraPicFlag = pictureDesc.referenceNum == 0;
    pictureParameters.pps_cb_qp_offset = pps->cbQpOffset;
    pictureParameters.pps_cr_qp_offset = pps->crQpOffset;
    pictureParameters.num_tile_columns_minus1 = pps->tileColumnNumMinus1;
    pictureParameters.num_tile_rows_minus1 = pps->tileRowNumMinus1;
    std::memcpy(pictureParameters.column_width_minus1, pps->columnWidthMinus1, sizeof(pictureParameters.column_width_minus1));
    std::memcpy(pictureParameters.row_height_minus1, pps->rowHeightMinus1, sizeof(pictureParameters.row_height_minus1));
    pictureParameters.diff_cu_qp_delta_depth = pps->diffCuQpDeltaDepth;
    pictureParameters.pps_beta_offset_div2 = pps->betaOffsetDiv2;
    pictureParameters.pps_tc_offset_div2 = pps->tcOffsetDiv2;
    pictureParameters.log2_parallel_merge_level_minus2 = pps->log2ParallelMergeLevelMinus2;
    pictureParameters.CurrPicOrderCntVal = pictureDesc.pictureOrderCount;
    pictureParameters.StatusReportFeedbackNumber = 1;

    for (DXVA_PicEntry_HEVC& entry : pictureParameters.RefPicList)
        entry.bPicEntry = 0xFF;
    std::memset(pictureParameters.RefPicSetStCurrBefore, 0xFF, sizeof(pictureParameters.RefPicSetStCurrBefore));
    std::memset(pictureParameters.RefPicSetStCurrAfter, 0xFF, sizeof(pictureParameters.RefPicSetStCurrAfter));
    std::memset(pictureParameters.RefPicSetLtCurr, 0xFF, sizeof(pictureParameters.RefPicSetLtCurr));

    uint32_t beforeNum = 0;
    uint32_t afterNum = 0;
    uint32_t longTermNum = 0;
    for (uint32_t i = 0; i < pictureDesc.referenceNum; i++) {
        const VideoH265ReferenceDesc& reference = pictureDesc.references[i];
        if (reference.slot > VIDEO_DECODE_MAX_PIC_ENTRY_SLOT)
            return false;

        pictureParameters.RefPicList[i].Index7Bits = (UCHAR)reference.slot;
        pictureParameters.RefPicList[i].AssociatedFlag = reference.longTerm != 0;
        pictureParameters.PicOrderCntValList[i] = reference.pictureOrderCount;

        if (reference.longTerm) {
            if (longTermNum >= sizeof(pictureParameters.RefPicSetLtCurr))
                return false;
            pictureParameters.RefPicSetLtCurr[longTermNum++] = (UCHAR)i;
        } else if (reference.pictureOrderCount < pictureDesc.pictureOrderCount) {
            if (beforeNum >= sizeof(pictureParameters.RefPicSetStCurrBefore))
                return false;
            pictureParameters.RefPicSetStCurrBefore[beforeNum++] = (UCHAR)i;
        } else {
            if (afterNum >= sizeof(pictureParameters.RefPicSetStCurrAfter))
                return false;
            pictureParameters.RefPicSetStCurrAfter[afterNum++] = (UCHAR)i;
        }
    }

    FillVideoH265ScalingListsD3D12(inverseQuantizationMatrix, pps->scalingLists ? pps->scalingLists : sps->scalingLists);

    for (uint32_t i = 0; i < sliceNum; i++) {
        const uint32_t offset = pictureDesc.sliceSegmentOffsets[i];
        if (offset >= bitstreamSize || (i + 1 < sliceNum && pictureDesc.sliceSegmentOffsets[i + 1] <= offset))
            return false;

        const uint64_t nextOffset = i + 1 < sliceNum ? pictureDesc.sliceSegmentOffsets[i + 1] : bitstreamSize;
        const uint64_t size = nextOffset - offset;
        if (size > UINT32_MAX)
            return false;

        slices[i] = {};
        slices[i].BSNALunitDataLocation = offset;
        slices[i].SliceBytesInBuffer = (UINT)size;
    }

    return true;
}

inline bool CanBuildVideoDecodeH264ArgumentsD3D12(const VideoDecodeDesc& desc) {
    return desc.h264PictureDesc && !desc.h265PictureDesc && desc.argumentNum == 0 && (desc.referenceNum == 0 || (desc.h264PictureDesc->references && desc.h264PictureDesc->referenceNum == desc.referenceNum));
}

inline const VideoH264DecodeReferenceDesc* FindVideoH264DecodeReferenceDescD3D12(const VideoH264DecodeReferenceDesc* references, uint32_t referenceNum, uint32_t slot) {
    if (!references)
        return nullptr;

    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i].slot == slot)
            return &references[i];
    }

    return nullptr;
}

NRI_INLINE void CommandBufferD3D12::DecodeVideo(const VideoDecodeDesc& videoDecodeDesc) {
    const VideoSessionD3D12& session = *(VideoSessionD3D12*)videoDecodeDesc.session;
    const VideoSessionDesc& sessionDesc = session.GetDesc();
    const VideoSessionParametersD3D12* parameters = (VideoSessionParametersD3D12*)videoDecodeDesc.parameters;

    if (parameters->m_Session != &session) {
        NRI_REPORT_ERROR(&m_Device, "'parameters' must belong to 'session'");
        return;
    }

    BufferD3D12& bitstream = *(BufferD3D12*)videoDecodeDesc.bitstream.buffer;
    if (videoDecodeDesc.bitstream.offset >= bitstream.GetDesc().size || videoDecodeDesc.bitstream.size > bitstream.GetDesc().size - videoDecodeDesc.bitstream.offset) {
        NRI_REPORT_ERROR(&m_Device, "'bitstream' range is outside of 'bitstream.buffer'");
        return;
    }

    D3D12_VIDEO_DECODE_INPUT_STREAM_ARGUMENTS input = {};
    DXVA_PicParams_H264 h264PictureParameters = {};
    DXVA_Qmatrix_H264 h264InverseQuantizationMatrix = {};
    Scratch<DXVA_Slice_H264_Short> h264Slices = NRI_ALLOCATE_SCRATCH(m_Device, DXVA_Slice_H264_Short, videoDecodeDesc.h264PictureDesc ? std::max(videoDecodeDesc.h264PictureDesc->sliceOffsetNum, 1u) : 1u);
    DXVA_PicParams_HEVC h265PictureParameters = {};
    DXVA_Qmatrix_HEVC h265InverseQuantizationMatrix = {};
    Scratch<DXVA_Slice_HEVC_Short> h265Slices = NRI_ALLOCATE_SCRATCH(m_Device, DXVA_Slice_HEVC_Short, videoDecodeDesc.h265PictureDesc ? std::max(videoDecodeDesc.h265PictureDesc->sliceSegmentOffsetNum, 1u) : 1u);
    DXVA_PicParams_AV1 av1PictureParameters = {};
    Scratch<DXVA_Tile_AV1> av1Tiles = NRI_ALLOCATE_SCRATCH(m_Device, DXVA_Tile_AV1, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum, 1u) : 1u);
    if (videoDecodeDesc.h264PictureDesc) {
        if (sessionDesc.codec != VideoCodec::H264) {
            NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc' can only be used with H.264 decode sessions");
            return;
        }

        if (!parameters || !parameters->m_H264Parameters) {
            NRI_REPORT_ERROR(&m_Device, "'parameters' with H.264 SPS/PPS data must be valid for neutral H.264 D3D12 decode");
            return;
        }

        if (!CanBuildVideoDecodeH264ArgumentsD3D12(videoDecodeDesc)) {
            NRI_REPORT_ERROR(&m_Device, "D3D12 neutral H.264 decode requires matching H.264 reference descriptors for inter pictures");
            return;
        }

        for (uint32_t i = 0; i < videoDecodeDesc.referenceNum; i++) {
            if (!FindVideoH264DecodeReferenceDescD3D12(videoDecodeDesc.h264PictureDesc->references, videoDecodeDesc.h264PictureDesc->referenceNum, videoDecodeDesc.references[i].slot)) {
                NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc->references' must include metadata for each H.264 reference");
                return;
            }
        }

        const uint32_t h264DstSlot = videoDecodeDesc.h264PictureDesc->referenceSlot ? videoDecodeDesc.h264PictureDesc->referenceSlot : videoDecodeDesc.dstSlot;
        if (!BuildVideoDecodeH264ArgumentsD3D12(*parameters->m_H264Parameters, *videoDecodeDesc.h264PictureDesc, videoDecodeDesc.bitstream.size, h264DstSlot,
                h264PictureParameters, h264InverseQuantizationMatrix, h264Slices, videoDecodeDesc.h264PictureDesc->sliceOffsetNum)) {
            NRI_REPORT_ERROR(&m_Device, "Failed to build D3D12 H.264 decode arguments from neutral descriptors");
            return;
        }

        input.NumFrameArguments = 3;
        input.FrameArguments[0].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_PICTURE_PARAMETERS;
        input.FrameArguments[0].Size = sizeof(h264PictureParameters);
        input.FrameArguments[0].pData = &h264PictureParameters;
        input.FrameArguments[1].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_INVERSE_QUANTIZATION_MATRIX;
        input.FrameArguments[1].Size = sizeof(h264InverseQuantizationMatrix);
        input.FrameArguments[1].pData = &h264InverseQuantizationMatrix;
        input.FrameArguments[2].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_SLICE_CONTROL;
        input.FrameArguments[2].Size = sizeof(DXVA_Slice_H264_Short) * videoDecodeDesc.h264PictureDesc->sliceOffsetNum;
        input.FrameArguments[2].pData = h264Slices;
    } else if (videoDecodeDesc.h265PictureDesc) {
        if (sessionDesc.codec != VideoCodec::H265) {
            NRI_REPORT_ERROR(&m_Device, "'h265PictureDesc' can only be used with H.265 decode sessions");
            return;
        }

        if (!parameters || !parameters->m_H265Parameters) {
            NRI_REPORT_ERROR(&m_Device, "'parameters' with H.265 VPS/SPS/PPS data must be valid for neutral H.265 D3D12 decode");
            return;
        }

        const VideoH265DecodePictureDesc& desc = *videoDecodeDesc.h265PictureDesc;
        if (desc.referenceNum != 0 && !desc.references) {
            NRI_REPORT_ERROR(&m_Device, "'h265PictureDesc->references' is NULL");
            return;
        }

        for (uint32_t i = 0; i < videoDecodeDesc.referenceNum; i++) {
            if (!FindVideoH265ReferenceDescD3D12(desc.references, desc.referenceNum, videoDecodeDesc.references[i].slot)) {
                NRI_REPORT_ERROR(&m_Device, "'h265PictureDesc->references' must include metadata for each H.265 reference");
                return;
            }
        }

        if (!BuildVideoDecodeH265ArgumentsD3D12(*parameters->m_H265Parameters, desc, videoDecodeDesc.bitstream.size, videoDecodeDesc.dstSlot,
                h265PictureParameters, h265InverseQuantizationMatrix, h265Slices, desc.sliceSegmentOffsetNum)) {
            NRI_REPORT_ERROR(&m_Device, "Failed to build D3D12 H.265 decode arguments from neutral descriptors");
            return;
        }

        input.NumFrameArguments = 3;
        input.FrameArguments[0].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_PICTURE_PARAMETERS;
        input.FrameArguments[0].Size = sizeof(h265PictureParameters);
        input.FrameArguments[0].pData = &h265PictureParameters;
        input.FrameArguments[1].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_INVERSE_QUANTIZATION_MATRIX;
        input.FrameArguments[1].Size = sizeof(h265InverseQuantizationMatrix);
        input.FrameArguments[1].pData = &h265InverseQuantizationMatrix;
        input.FrameArguments[2].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_SLICE_CONTROL;
        input.FrameArguments[2].Size = sizeof(DXVA_Slice_HEVC_Short) * desc.sliceSegmentOffsetNum;
        input.FrameArguments[2].pData = h265Slices;
    } else if (videoDecodeDesc.av1PictureDesc) {
        if (sessionDesc.codec != VideoCodec::AV1) {
            NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc' can only be used with AV1 decode sessions");
            return;
        }

        const VideoAV1DecodePictureDesc& desc = *videoDecodeDesc.av1PictureDesc;
        if ((desc.tileNum != 0 && !desc.tiles) || desc.tileNum > 256 || desc.referenceNum > 8 || (desc.referenceNum != 0 && !desc.references)) {
            NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc' contains invalid tile or reference data");
            return;
        }
        if (desc.tileLayout && (!desc.tileLayout->columnNum || !desc.tileLayout->rowNum || desc.tileLayout->columnNum > 64 || desc.tileLayout->rowNum > 64 || !desc.tileLayout->miColumnStarts || !desc.tileLayout->miRowStarts || !desc.tileLayout->widthInSuperblocksMinus1 || !desc.tileLayout->heightInSuperblocksMinus1)) {
            NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->tileLayout' is invalid");
            return;
        }

        const VideoAV1SessionParametersDesc defaultAV1Parameters = {GetDefaultVideoAV1SequenceDescD3D12(sessionDesc.width, sessionDesc.height, sessionDesc.format)};
        const VideoAV1SessionParametersDesc& av1Parameters = parameters && parameters->m_AV1Parameters ? *parameters->m_AV1Parameters : defaultAV1Parameters;
        const VideoAV1SequenceDesc& sequence = av1Parameters.sequence;
        const VideoAV1PictureBits pictureFlags = desc.flags == VideoAV1PictureBits::NONE ? GetDefaultVideoAV1PictureFlags() : desc.flags;
        av1PictureParameters.width = sessionDesc.width;
        av1PictureParameters.height = sessionDesc.height;
        av1PictureParameters.max_width = sequence.maxFrameWidthMinus1 + 1;
        av1PictureParameters.max_height = sequence.maxFrameHeightMinus1 + 1;
        av1PictureParameters.CurrPicTextureIndex = (UCHAR)videoDecodeDesc.dstSlot;
        av1PictureParameters.superres_denom = desc.superresDenom ? desc.superresDenom : 8;
        av1PictureParameters.bitdepth = sequence.bitDepth;
        av1PictureParameters.seq_profile = sequence.seqProfile;
        av1PictureParameters.tiles.cols = 1;
        av1PictureParameters.tiles.rows = 1;
        av1PictureParameters.tiles.widths[0] = (USHORT)((sessionDesc.width + 63) / 64);
        av1PictureParameters.tiles.heights[0] = (USHORT)((sessionDesc.height + 63) / 64);
        if (desc.tileLayout) {
            av1PictureParameters.tiles.cols = desc.tileLayout->columnNum;
            av1PictureParameters.tiles.rows = desc.tileLayout->rowNum;
            av1PictureParameters.tiles.context_update_id = desc.tileLayout->contextUpdateTileId;
            for (uint32_t i = 0; i < desc.tileLayout->columnNum; i++)
                av1PictureParameters.tiles.widths[i] = desc.tileLayout->widthInSuperblocksMinus1[i] + 1;
            for (uint32_t i = 0; i < desc.tileLayout->rowNum; i++)
                av1PictureParameters.tiles.heights[i] = desc.tileLayout->heightInSuperblocksMinus1[i] + 1;
        }
        av1PictureParameters.coding.use_128x128_superblock = !!(sequence.flags & VideoAV1SequenceBits::USE_128X128_SUPERBLOCK);
        av1PictureParameters.coding.intra_edge_filter = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_INTRA_EDGE_FILTER);
        av1PictureParameters.coding.interintra_compound = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_INTERINTRA_COMPOUND);
        av1PictureParameters.coding.masked_compound = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_MASKED_COMPOUND);
        av1PictureParameters.coding.warped_motion = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_WARPED_MOTION);
        av1PictureParameters.coding.dual_filter = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_DUAL_FILTER);
        av1PictureParameters.coding.jnt_comp = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_JNT_COMP);
        av1PictureParameters.coding.enable_ref_frame_mvs = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_REF_FRAME_MVS);
        av1PictureParameters.coding.screen_content_tools = !!(pictureFlags & VideoAV1PictureBits::ALLOW_SCREEN_CONTENT_TOOLS);
        av1PictureParameters.coding.integer_mv = !!(pictureFlags & VideoAV1PictureBits::FORCE_INTEGER_MV);
        av1PictureParameters.coding.cdef = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_CDEF);
        av1PictureParameters.coding.restoration = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_RESTORATION);
        av1PictureParameters.coding.film_grain = !!(sequence.flags & VideoAV1SequenceBits::FILM_GRAIN_PARAMS_PRESENT);
        av1PictureParameters.coding.intrabc = !!(pictureFlags & VideoAV1PictureBits::ALLOW_INTRABC);
        av1PictureParameters.coding.high_precision_mv = !!(pictureFlags & VideoAV1PictureBits::ALLOW_HIGH_PRECISION_MV);
        av1PictureParameters.coding.switchable_motion_mode = !!(pictureFlags & VideoAV1PictureBits::IS_MOTION_MODE_SWITCHABLE);
        av1PictureParameters.coding.disable_frame_end_update_cdf = !!(pictureFlags & VideoAV1PictureBits::DISABLE_FRAME_END_UPDATE_CDF);
        av1PictureParameters.coding.disable_cdf_update = !!(pictureFlags & VideoAV1PictureBits::DISABLE_CDF_UPDATE);
        av1PictureParameters.coding.reference_mode = !!(pictureFlags & VideoAV1PictureBits::REFERENCE_SELECT);
        av1PictureParameters.coding.skip_mode = !!(pictureFlags & VideoAV1PictureBits::SKIP_MODE_PRESENT);
        av1PictureParameters.coding.reduced_tx_set = !!(pictureFlags & VideoAV1PictureBits::REDUCED_TX_SET);
        av1PictureParameters.coding.superres = !!(pictureFlags & VideoAV1PictureBits::USE_SUPERRES);
        av1PictureParameters.coding.tx_mode = desc.txMode ? desc.txMode : 2;
        av1PictureParameters.coding.use_ref_frame_mvs = !!(pictureFlags & VideoAV1PictureBits::USE_REF_FRAME_MVS);
        av1PictureParameters.coding.reference_frame_update = desc.refreshFrameFlags != 0;
        av1PictureParameters.format.frame_type = GetVideoDecodeAV1FrameTypeD3D12(desc.frameType);
        av1PictureParameters.format.show_frame = !!(pictureFlags & VideoAV1PictureBits::SHOW_FRAME);
        av1PictureParameters.format.showable_frame = !!(pictureFlags & VideoAV1PictureBits::SHOWABLE_FRAME);
        av1PictureParameters.format.subsampling_x = sequence.subsamplingX;
        av1PictureParameters.format.subsampling_y = sequence.subsamplingY;
        av1PictureParameters.format.mono_chrome = !!(sequence.flags & VideoAV1SequenceBits::MONO_CHROME);
        av1PictureParameters.primary_ref_frame = (UCHAR)GetVideoDecodeAV1ReferenceNameIndexD3D12(desc.primaryReferenceName);
        av1PictureParameters.order_hint = desc.orderHint;
        av1PictureParameters.order_hint_bits = (UCHAR)(sequence.orderHintBitsMinus1 + 1);
        std::memset(av1PictureParameters.RefFrameMapTextureIndex, 0xFF, sizeof(av1PictureParameters.RefFrameMapTextureIndex));
        for (uint32_t i = 0; i < 7; i++)
            av1PictureParameters.frame_refs[i].Index = 0xFF;
        for (uint32_t i = 0; i < desc.referenceNum; i++) {
            const VideoAV1ReferenceDesc& reference = desc.references[i];
            if (reference.refFrameIndex >= 8 || reference.slot > 0xFE) {
                NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->references[%u]' is invalid", i);
                return;
            }

            av1PictureParameters.RefFrameMapTextureIndex[reference.refFrameIndex] = (UCHAR)reference.slot;
            const uint32_t referenceNameIndex = GetVideoDecodeAV1ReferenceNameIndexD3D12(reference.name);
            if (referenceNameIndex < 7) {
                av1PictureParameters.frame_refs[referenceNameIndex].Index = reference.refFrameIndex;
                av1PictureParameters.frame_refs[referenceNameIndex].width = sessionDesc.width;
                av1PictureParameters.frame_refs[referenceNameIndex].height = sessionDesc.height;
                if (desc.globalMotion) {
                    const uint32_t gmIndex = referenceNameIndex + 1;
                    av1PictureParameters.frame_refs[referenceNameIndex].wminvalid = desc.globalMotion->invalid[gmIndex] != 0;
                    av1PictureParameters.frame_refs[referenceNameIndex].wmtype = desc.globalMotion->type[gmIndex];
                    std::memcpy(av1PictureParameters.frame_refs[referenceNameIndex].wmmat, desc.globalMotion->params[gmIndex], sizeof(av1PictureParameters.frame_refs[referenceNameIndex].wmmat));
                }
            }
        }
        av1PictureParameters.quantization.delta_q_present = !!(pictureFlags & VideoAV1PictureBits::DELTA_Q_PRESENT);
        av1PictureParameters.quantization.delta_q_res = desc.deltaQRes;
        av1PictureParameters.quantization.base_qindex = desc.baseQIndex;
        if (desc.quantization) {
            av1PictureParameters.quantization.y_dc_delta_q = desc.quantization->deltaQYDc;
            av1PictureParameters.quantization.u_dc_delta_q = desc.quantization->deltaQUDc;
            av1PictureParameters.quantization.u_ac_delta_q = desc.quantization->deltaQUAc;
            av1PictureParameters.quantization.v_dc_delta_q = desc.quantization->deltaQVDc;
            av1PictureParameters.quantization.v_ac_delta_q = desc.quantization->deltaQVAc;
            av1PictureParameters.quantization.qm_y = desc.quantization->usingQmatrix ? desc.quantization->qmY : 0xFF;
            av1PictureParameters.quantization.qm_u = desc.quantization->usingQmatrix ? desc.quantization->qmU : 0xFF;
            av1PictureParameters.quantization.qm_v = desc.quantization->usingQmatrix ? desc.quantization->qmV : 0xFF;
        } else {
            av1PictureParameters.quantization.qm_y = 0xFF;
            av1PictureParameters.quantization.qm_u = 0xFF;
            av1PictureParameters.quantization.qm_v = 0xFF;
        }
        av1PictureParameters.cdef.damping = desc.cdefDampingMinus3;
        av1PictureParameters.cdef.bits = desc.cdefBits;
        if (desc.cdef) {
            for (uint32_t i = 0; i < 8; i++) {
                av1PictureParameters.cdef.y_strengths[i].primary = desc.cdef->yPrimaryStrength[i];
                av1PictureParameters.cdef.y_strengths[i].secondary = desc.cdef->ySecondaryStrength[i];
                av1PictureParameters.cdef.uv_strengths[i].primary = desc.cdef->uvPrimaryStrength[i];
                av1PictureParameters.cdef.uv_strengths[i].secondary = desc.cdef->uvSecondaryStrength[i];
            }
        }
        av1PictureParameters.interp_filter = desc.interpolationFilter ? desc.interpolationFilter : 4;
        av1PictureParameters.loop_filter.delta_lf_present = !!(pictureFlags & VideoAV1PictureBits::DELTA_LF_PRESENT);
        av1PictureParameters.loop_filter.delta_lf_multi = !!(pictureFlags & VideoAV1PictureBits::DELTA_LF_MULTI);
        av1PictureParameters.loop_filter.delta_lf_res = desc.deltaLfRes;
        if (desc.loopFilter) {
            av1PictureParameters.loop_filter.filter_level[0] = desc.loopFilter->level[0];
            av1PictureParameters.loop_filter.filter_level[1] = desc.loopFilter->level[1];
            av1PictureParameters.loop_filter.filter_level_u = desc.loopFilter->level[2];
            av1PictureParameters.loop_filter.filter_level_v = desc.loopFilter->level[3];
            av1PictureParameters.loop_filter.sharpness_level = desc.loopFilter->sharpness;
            av1PictureParameters.loop_filter.mode_ref_delta_enabled = desc.loopFilter->deltaEnabled;
            av1PictureParameters.loop_filter.mode_ref_delta_update = desc.loopFilter->deltaUpdate;
            std::memcpy(av1PictureParameters.loop_filter.ref_deltas, desc.loopFilter->refDeltas, sizeof(av1PictureParameters.loop_filter.ref_deltas));
            std::memcpy(av1PictureParameters.loop_filter.mode_deltas, desc.loopFilter->modeDeltas, sizeof(av1PictureParameters.loop_filter.mode_deltas));
        } else {
            av1PictureParameters.loop_filter.ref_deltas[0] = 1;
            av1PictureParameters.loop_filter.ref_deltas[4] = -1;
            av1PictureParameters.loop_filter.ref_deltas[6] = -1;
            av1PictureParameters.loop_filter.ref_deltas[7] = -1;
        }
        if (desc.loopRestoration) {
            av1PictureParameters.loop_filter.frame_restoration_type[0] = desc.loopRestoration->frameRestorationType[0];
            av1PictureParameters.loop_filter.frame_restoration_type[1] = desc.loopRestoration->frameRestorationType[1];
            av1PictureParameters.loop_filter.frame_restoration_type[2] = desc.loopRestoration->frameRestorationType[2];
            const bool usesLr = desc.loopRestoration->frameRestorationType[0] || desc.loopRestoration->frameRestorationType[1] || desc.loopRestoration->frameRestorationType[2];
            if (usesLr) {
                av1PictureParameters.loop_filter.log2_restoration_unit_size[0] = 6 + desc.loopRestoration->lrUnitShift;
                av1PictureParameters.loop_filter.log2_restoration_unit_size[1] = 6 + desc.loopRestoration->lrUnitShift - desc.loopRestoration->lrUvShift;
                av1PictureParameters.loop_filter.log2_restoration_unit_size[2] = 6 + desc.loopRestoration->lrUnitShift - desc.loopRestoration->lrUvShift;
            }
        }
        if (!desc.loopRestoration || (!av1PictureParameters.loop_filter.log2_restoration_unit_size[0] && !av1PictureParameters.loop_filter.log2_restoration_unit_size[1] && !av1PictureParameters.loop_filter.log2_restoration_unit_size[2])) {
            av1PictureParameters.loop_filter.log2_restoration_unit_size[0] = 8;
            av1PictureParameters.loop_filter.log2_restoration_unit_size[1] = 8;
            av1PictureParameters.loop_filter.log2_restoration_unit_size[2] = 8;
        }
        av1PictureParameters.segmentation.enabled = !!(pictureFlags & VideoAV1PictureBits::SEGMENTATION_ENABLED);
        av1PictureParameters.segmentation.update_map = !!(pictureFlags & VideoAV1PictureBits::SEGMENTATION_UPDATE_MAP);
        av1PictureParameters.segmentation.update_data = !!(pictureFlags & VideoAV1PictureBits::SEGMENTATION_UPDATE_DATA);
        av1PictureParameters.segmentation.temporal_update = !!(pictureFlags & VideoAV1PictureBits::SEGMENTATION_TEMPORAL_UPDATE);
        if (desc.segmentation) {
            for (uint32_t i = 0; i < 8; i++) {
                av1PictureParameters.segmentation.feature_mask[i].mask = desc.segmentation->featureEnabled[i];
                for (uint32_t j = 0; j < 8; j++)
                    av1PictureParameters.segmentation.feature_data[i][j] = desc.segmentation->featureData[i][j];
            }
        }
        if ((pictureFlags & VideoAV1PictureBits::APPLY_GRAIN) && desc.filmGrain) {
            av1PictureParameters.film_grain.apply_grain = 1;
            av1PictureParameters.film_grain.scaling_shift_minus8 = desc.filmGrain->grainScalingMinus8;
            av1PictureParameters.film_grain.chroma_scaling_from_luma = desc.filmGrain->chromaScalingFromLuma;
            av1PictureParameters.film_grain.ar_coeff_lag = desc.filmGrain->arCoeffLag;
            av1PictureParameters.film_grain.ar_coeff_shift_minus6 = desc.filmGrain->arCoeffShiftMinus6;
            av1PictureParameters.film_grain.grain_scale_shift = desc.filmGrain->grainScaleShift;
            av1PictureParameters.film_grain.overlap_flag = desc.filmGrain->overlapFlag;
            av1PictureParameters.film_grain.clip_to_restricted_range = desc.filmGrain->clipToRestrictedRange;
            av1PictureParameters.film_grain.matrix_coeff_is_identity = desc.filmGrain->matrixCoeffIsIdentity;
            av1PictureParameters.film_grain.grain_seed = desc.filmGrain->grainSeed;
            av1PictureParameters.film_grain.num_y_points = desc.filmGrain->numYPoints;
            av1PictureParameters.film_grain.num_cb_points = desc.filmGrain->numCbPoints;
            av1PictureParameters.film_grain.num_cr_points = desc.filmGrain->numCrPoints;
            for (uint32_t i = 0; i < 14; i++) {
                av1PictureParameters.film_grain.scaling_points_y[i][0] = desc.filmGrain->pointYValue[i];
                av1PictureParameters.film_grain.scaling_points_y[i][1] = desc.filmGrain->pointYScaling[i];
            }
            for (uint32_t i = 0; i < 10; i++) {
                av1PictureParameters.film_grain.scaling_points_cb[i][0] = desc.filmGrain->pointCbValue[i];
                av1PictureParameters.film_grain.scaling_points_cb[i][1] = desc.filmGrain->pointCbScaling[i];
                av1PictureParameters.film_grain.scaling_points_cr[i][0] = desc.filmGrain->pointCrValue[i];
                av1PictureParameters.film_grain.scaling_points_cr[i][1] = desc.filmGrain->pointCrScaling[i];
            }
            std::memcpy(av1PictureParameters.film_grain.ar_coeffs_y, desc.filmGrain->arCoeffsYPlus128, sizeof(av1PictureParameters.film_grain.ar_coeffs_y));
            std::memcpy(av1PictureParameters.film_grain.ar_coeffs_cb, desc.filmGrain->arCoeffsCbPlus128, sizeof(av1PictureParameters.film_grain.ar_coeffs_cb));
            std::memcpy(av1PictureParameters.film_grain.ar_coeffs_cr, desc.filmGrain->arCoeffsCrPlus128, sizeof(av1PictureParameters.film_grain.ar_coeffs_cr));
            av1PictureParameters.film_grain.cb_mult = desc.filmGrain->cbMult;
            av1PictureParameters.film_grain.cb_luma_mult = desc.filmGrain->cbLumaMult;
            av1PictureParameters.film_grain.cr_mult = desc.filmGrain->crMult;
            av1PictureParameters.film_grain.cr_luma_mult = desc.filmGrain->crLumaMult;
            av1PictureParameters.film_grain.cb_offset = desc.filmGrain->cbOffset;
            av1PictureParameters.film_grain.cr_offset = desc.filmGrain->crOffset;
        }
        av1PictureParameters.StatusReportFeedbackNumber = 1;

        for (uint32_t i = 0; i < desc.tileNum; i++) {
            av1Tiles[i] = {};
            av1Tiles[i].DataOffset = desc.tiles[i].offset;
            av1Tiles[i].DataSize = desc.tiles[i].size;
            av1Tiles[i].row = desc.tiles[i].row;
            av1Tiles[i].column = desc.tiles[i].column;
            av1Tiles[i].anchor_frame = desc.tiles[i].anchorFrame ? desc.tiles[i].anchorFrame : 0xFF;
            av1PictureParameters.tiles.cols = std::max<UCHAR>(av1PictureParameters.tiles.cols, (UCHAR)(desc.tiles[i].column + 1));
            av1PictureParameters.tiles.rows = std::max<UCHAR>(av1PictureParameters.tiles.rows, (UCHAR)(desc.tiles[i].row + 1));
        }

        input.NumFrameArguments = 2;
        input.FrameArguments[0].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_PICTURE_PARAMETERS;
        input.FrameArguments[0].Size = sizeof(av1PictureParameters);
        input.FrameArguments[0].pData = &av1PictureParameters;
        input.FrameArguments[1].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_SLICE_CONTROL;
        input.FrameArguments[1].Size = sizeof(DXVA_Tile_AV1) * desc.tileNum;
        input.FrameArguments[1].pData = av1Tiles;
    } else {
        input.NumFrameArguments = videoDecodeDesc.argumentNum;
        for (uint32_t i = 0; i < videoDecodeDesc.argumentNum; i++) {
            if (!videoDecodeDesc.arguments[i].data || videoDecodeDesc.arguments[i].size == 0) {
                NRI_REPORT_ERROR(&m_Device, "'arguments[%u]' has invalid data or size", i);
                return;
            }

            input.FrameArguments[i].Type = (D3D12_VIDEO_DECODE_ARGUMENT_TYPE)videoDecodeDesc.arguments[i].type;
            input.FrameArguments[i].Size = videoDecodeDesc.arguments[i].size;
            input.FrameArguments[i].pData = (void*)videoDecodeDesc.arguments[i].data;
        }
    }

    VideoPictureD3D12& dstPicture = *(VideoPictureD3D12*)videoDecodeDesc.dstPicture;
    VideoPictureD3D12& setupPicture = videoDecodeDesc.setupPicture ? *(VideoPictureD3D12*)videoDecodeDesc.setupPicture : dstPicture;
    const bool h264NeutralDecode = videoDecodeDesc.h264PictureDesc != nullptr;
    const bool h265NeutralDecode = videoDecodeDesc.h265PictureDesc != nullptr;
    const bool av1NeutralDecode = videoDecodeDesc.av1PictureDesc != nullptr;

    VideoDecodeReferenceLayoutD3D12 referenceLayout = {};
    if (!GetVideoDecodeReferenceLayoutD3D12(videoDecodeDesc.references, videoDecodeDesc.referenceNum, referenceLayout)) {
        if (referenceLayout.duplicateSlot)
            NRI_REPORT_ERROR(&m_Device, "'references[%u].slot' duplicates an earlier D3D12 decode reference slot", referenceLayout.failingReference);
        else
            NRI_REPORT_ERROR(&m_Device, "'references[%u].slot' exceeds the D3D12 decode PicEntry index range", referenceLayout.failingReference);
        return;
    }
    if (h264NeutralDecode)
        referenceLayout.slotCount = std::max(referenceLayout.slotCount, videoDecodeDesc.dstSlot + 1);
    if (h265NeutralDecode)
        referenceLayout.slotCount = std::max(referenceLayout.slotCount, videoDecodeDesc.dstSlot + 1);
    if (av1NeutralDecode)
        referenceLayout.slotCount = std::max(referenceLayout.slotCount, videoDecodeDesc.dstSlot + 1);

    Scratch<ID3D12Resource*> referenceResources = NRI_ALLOCATE_SCRATCH(m_Device, ID3D12Resource*, referenceLayout.slotCount);
    Scratch<uint32_t> referenceSubresources = NRI_ALLOCATE_SCRATCH(m_Device, uint32_t, referenceLayout.slotCount);
    for (uint32_t i = 0; i < referenceLayout.slotCount; i++) {
        referenceResources[i] = nullptr;
        referenceSubresources[i] = 0;
    }

    for (uint32_t i = 0; i < videoDecodeDesc.referenceNum; i++) {
        if (!videoDecodeDesc.references[i].picture) {
            NRI_REPORT_ERROR(&m_Device, "'references[%u].picture' is NULL", i);
            return;
        }

        VideoPictureD3D12& reference = *(VideoPictureD3D12*)videoDecodeDesc.references[i].picture;
        const uint32_t slot = videoDecodeDesc.references[i].slot;
        referenceResources[slot] = (ID3D12Resource*)(*reference.m_Texture);
        referenceSubresources[slot] = reference.m_Subresource;
    }
    if (h264NeutralDecode) {
        referenceResources[videoDecodeDesc.dstSlot] = (ID3D12Resource*)(*setupPicture.m_Texture);
        referenceSubresources[videoDecodeDesc.dstSlot] = setupPicture.m_Subresource;
    }
    if (h265NeutralDecode) {
        referenceResources[videoDecodeDesc.dstSlot] = (ID3D12Resource*)(*setupPicture.m_Texture);
        referenceSubresources[videoDecodeDesc.dstSlot] = setupPicture.m_Subresource;
    }
    if (av1NeutralDecode) {
        referenceResources[videoDecodeDesc.dstSlot] = (ID3D12Resource*)(*setupPicture.m_Texture);
        referenceSubresources[videoDecodeDesc.dstSlot] = setupPicture.m_Subresource;
    }

    input.ReferenceFrames.NumTexture2Ds = referenceLayout.slotCount;
    input.ReferenceFrames.ppTexture2Ds = referenceLayout.slotCount ? (ID3D12Resource**)referenceResources : nullptr;
    input.ReferenceFrames.pSubresources = referenceLayout.slotCount ? (uint32_t*)referenceSubresources : nullptr;
    input.CompressedBitstream.pBuffer = (ID3D12Resource*)bitstream;
    input.CompressedBitstream.Offset = videoDecodeDesc.bitstream.offset;
    input.CompressedBitstream.Size = videoDecodeDesc.bitstream.size;
    input.pHeap = session.GetDecoderHeap();

    D3D12_VIDEO_DECODE_OUTPUT_STREAM_ARGUMENTS output = {};
    output.pOutputTexture2D = (ID3D12Resource*)(*dstPicture.m_Texture);
    output.OutputSubresource = dstPicture.m_Subresource;

    GetVideoDecodeCommandList()->DecodeFrame(session.GetDecoder(), &output, &input);
}

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
inline bool IsVideoEncodeFrameTypeSupportedByD3D12(VideoCodec codec, VideoEncodeFrameType frameType, bool isBFrameSupported) {
    return frameType != VideoEncodeFrameType::B || ((codec == VideoCodec::H264 || codec == VideoCodec::H265) && isBFrameSupported);
}

inline bool IsVideoEncodePictureUsedAsReferenceD3D12(VideoCodec codec, VideoEncodeFrameType frameType, uint32_t maxReferenceNum, bool hasReconstructedPicture, uint8_t av1RefreshFrameFlags) {
    if (!maxReferenceNum || !hasReconstructedPicture)
        return false;

    if ((codec == VideoCodec::H264 || codec == VideoCodec::H265) && frameType == VideoEncodeFrameType::B)
        return false;

    return codec != VideoCodec::AV1 || av1RefreshFrameFlags != 0;
}

inline uint8_t GetVideoEncodeQPByFrameTypeD3D12(const VideoEncodeRateControlDesc& rateControlDesc, VideoEncodeFrameType frameType) {
    return frameType == VideoEncodeFrameType::B ? rateControlDesc.qpB : (frameType == VideoEncodeFrameType::P ? rateControlDesc.qpP : rateControlDesc.qpI);
}

static const VideoH264ReferenceDesc* FindVideoEncodeH264ReferenceDesc(const VideoH264PictureDesc* h264PictureDesc, uint32_t slot) {
    if (!h264PictureDesc)
        return nullptr;

    for (uint32_t i = 0; i < h264PictureDesc->referenceNum; i++) {
        if (h264PictureDesc->references[i].slot == slot)
            return &h264PictureDesc->references[i];
    }

    return nullptr;
}

static D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE GetVideoEncodeAV1FrameTypeD3D12(VideoEncodeFrameType frameType) {
    switch (frameType) {
        case VideoEncodeFrameType::IDR:
        case VideoEncodeFrameType::I:
            return D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_KEY_FRAME;
        case VideoEncodeFrameType::P:
        case VideoEncodeFrameType::B:
            return D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_INTER_FRAME;
        default:
            return (D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE)-1;
    }
}

static uint32_t GetVideoEncodeAV1ReferenceNameIndexD3D12(VideoAV1ReferenceName name) {
    switch (name) {
        case VideoAV1ReferenceName::NONE:
            return 7;
        case VideoAV1ReferenceName::LAST:
            return 0;
        case VideoAV1ReferenceName::LAST2:
            return 1;
        case VideoAV1ReferenceName::LAST3:
            return 2;
        case VideoAV1ReferenceName::GOLDEN:
            return 3;
        case VideoAV1ReferenceName::BWDREF:
            return 4;
        case VideoAV1ReferenceName::ALTREF2:
            return 5;
        case VideoAV1ReferenceName::ALTREF:
            return 6;
        default:
            return 7;
    }
}

static bool HasVideoEncodeAV1DPBSlotResourceD3D12(const uint32_t* dpbSlotResourceIndices, uint32_t resourceIndex) {
    for (uint32_t i = 0; i < 8; i++) {
        if (dpbSlotResourceIndices[i] == resourceIndex)
            return true;
    }

    return false;
}

static_assert(offsetof(VideoEncodeFeedback, errorFlags) == offsetof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA, EncodeErrorFlags));
static_assert(offsetof(VideoEncodeFeedback, encodedBitstreamWrittenBytes) == offsetof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA, EncodedBitstreamWrittenBytesCount));
static_assert(offsetof(VideoEncodeFeedback, writtenSubregionNum) == offsetof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA, WrittenSubregionsCount));

#endif

NRI_INLINE void CommandBufferD3D12::EncodeVideo(const VideoEncodeDesc& videoEncodeDesc) {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    const VideoSessionD3D12& session = *(VideoSessionD3D12*)videoEncodeDesc.session;
    const VideoSessionDesc& sessionDesc = session.GetDesc();

    if (videoEncodeDesc.h264PictureDesc && sessionDesc.codec != VideoCodec::H264) {
        NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc' can only be used with H.264 sessions");
        return;
    }
    if (videoEncodeDesc.av1PictureDesc && sessionDesc.codec != VideoCodec::AV1) {
        NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc' can only be used with AV1 sessions");
        return;
    }

    VideoSessionParametersD3D12& parameters = *(VideoSessionParametersD3D12*)videoEncodeDesc.parameters;
    if (parameters.m_Session != &session) {
        NRI_REPORT_ERROR(&m_Device, "'parameters' must belong to 'session'");
        return;
    }

    BufferD3D12& dstBitstream = *(BufferD3D12*)videoEncodeDesc.dstBitstream.buffer;
    if (videoEncodeDesc.dstBitstream.offset >= dstBitstream.GetDesc().size || videoEncodeDesc.dstBitstream.size > dstBitstream.GetDesc().size - videoEncodeDesc.dstBitstream.offset) {
        NRI_REPORT_ERROR(&m_Device, "'dstBitstream' range is outside of 'dstBitstream.buffer'");
        return;
    }

    if (videoEncodeDesc.h265ReferenceDescs && sessionDesc.codec != VideoCodec::H265) {
        NRI_REPORT_ERROR(&m_Device, "'h265ReferenceDescs' can only be used with H.265 sessions");
        return;
    }

    if (sessionDesc.codec == VideoCodec::H264 && videoEncodeDesc.referenceNum) {
        if (!videoEncodeDesc.h264PictureDesc) {
            NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc' must be valid when H.264 encode uses references");
            return;
        }
        if (videoEncodeDesc.h264PictureDesc->referenceNum != videoEncodeDesc.referenceNum) {
            NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc->referenceNum' must match 'referenceNum'");
            return;
        }
    }

    Scratch<ID3D12Resource*> referenceResources = NRI_ALLOCATE_SCRATCH(m_Device, ID3D12Resource*, videoEncodeDesc.referenceNum);
    Scratch<uint32_t> referenceSubresources = NRI_ALLOCATE_SCRATCH(m_Device, uint32_t, videoEncodeDesc.referenceNum);
    Scratch<UINT> h264List0References = NRI_ALLOCATE_SCRATCH(m_Device, UINT, videoEncodeDesc.referenceNum);
    Scratch<UINT> h264List1References = NRI_ALLOCATE_SCRATCH(m_Device, UINT, videoEncodeDesc.referenceNum);
    Scratch<D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_H264> h264ReferenceDescriptors = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_H264, videoEncodeDesc.referenceNum);
    uint32_t h264List0ReferenceNum = 0;
    uint32_t h264List1ReferenceNum = 0;
    for (uint32_t i = 0; i < videoEncodeDesc.referenceNum; i++) {
        if (!videoEncodeDesc.references[i].picture) {
            NRI_REPORT_ERROR(&m_Device, "'references[%u].picture' is NULL", i);
            return;
        }

        VideoPictureD3D12& reference = *(VideoPictureD3D12*)videoEncodeDesc.references[i].picture;
        referenceResources[i] = (ID3D12Resource*)(*reference.m_Texture);
        referenceSubresources[i] = reference.m_Subresource;

        if (sessionDesc.codec == VideoCodec::H264) {
            const VideoH264ReferenceDesc* referenceDesc = FindVideoEncodeH264ReferenceDesc(videoEncodeDesc.h264PictureDesc, videoEncodeDesc.references[i].slot);
            if (!referenceDesc) {
                NRI_REPORT_ERROR(&m_Device, "'references[%u].slot' is not described by 'h264PictureDesc'", i);
                return;
            }

            if (referenceDesc->listIndex == 0)
                h264List0References[h264List0ReferenceNum++] = i;
            else if (referenceDesc->listIndex == 1)
                h264List1References[h264List1ReferenceNum++] = i;
            else {
                NRI_REPORT_ERROR(&m_Device, "'h264PictureDesc->references' listIndex must be 0 or 1");
                return;
            }

            h264ReferenceDescriptors[i] = {};
            h264ReferenceDescriptors[i].ReconstructedPictureResourceIndex = i;
            h264ReferenceDescriptors[i].IsLongTermReference = referenceDesc->longTermReference != 0;
            h264ReferenceDescriptors[i].LongTermPictureIdx = referenceDesc->longTermPictureIndex;
            h264ReferenceDescriptors[i].PictureOrderCountNumber = referenceDesc->pictureOrderCount;
            h264ReferenceDescriptors[i].FrameDecodingOrderNumber = referenceDesc->frameNum;
            h264ReferenceDescriptors[i].TemporalLayerIndex = referenceDesc->temporalLayer;
        }
    }

    const VideoEncodeRateControlDesc defaultRateControl = {VideoEncodeRateControlMode::CQP, 26, 28, 30, 0, 51, 30, 1, 0, 0, 0, 0, 0};
    const VideoEncodeRateControlDesc& rateControlDesc = videoEncodeDesc.rateControlDesc ? *videoEncodeDesc.rateControlDesc : defaultRateControl;
    if ((uint32_t)rateControlDesc.mode >= (uint32_t)VideoEncodeRateControlMode::MAX_NUM || (rateControlDesc.mode != VideoEncodeRateControlMode::CQP && !rateControlDesc.targetBitrate)
        || (rateControlDesc.qpMax && rateControlDesc.qpMin > rateControlDesc.qpMax)) {
        NRI_REPORT_ERROR(&m_Device, "'rateControlDesc' is invalid");
        return;
    }
    if ((session.m_RateControlModes & GetVideoEncodeRateControlModeMask(rateControlDesc.mode)) == 0) {
        NRI_REPORT_ERROR(&m_Device, "Unsupported D3D12 video encode rate control mode");
        return;
    }

    VideoEncodeRateControlStateD3D12 rateControlState;
    FillVideoEncodeRateControlD3D12(rateControlDesc, rateControlState);

    D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_H264 h264Gop = {};
    h264Gop.GOPLength = videoEncodeDesc.referenceNum ? 60 : 1;
    h264Gop.PPicturePeriod = sessionDesc.maxReferenceNum > 1 ? 2 : 1;

    D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_HEVC hevcGop = {};
    hevcGop.GOPLength = sessionDesc.maxReferenceNum ? 60 : 1;
    hevcGop.PPicturePeriod = sessionDesc.maxReferenceNum > 1 ? 2 : 1;

    D3D12_VIDEO_ENCODER_ENCODEFRAME_INPUT_ARGUMENTS1 input = {};
    D3D12_VIDEO_ENCODER_ENCODEFRAME_OUTPUT_ARGUMENTS1 output = {};
    D3D12_VIDEO_ENCODER_COMPRESSED_BITSTREAM& bitstream = output.Bitstream.FrameOutputBuffer;

    D3D12_VIDEO_ENCODER_PICTURE_CONTROL_DESC1 pictureControl = {};
    D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA1 pictureCodecData = {};
    D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA_HEVC2 hevcPicture = {};

    D3D12_VIDEO_ENCODER_RESOLVE_METADATA_INPUT_ARGUMENTS1 resolveInput = {};
    D3D12_VIDEO_ENCODER_RESOLVE_METADATA_OUTPUT_ARGUMENTS1 resolveOutput = {};

    D3D12_VIDEO_ENCODER_AV1_PROFILE resolveAv1Profile = D3D12_VIDEO_ENCODER_AV1_PROFILE_MAIN;
    D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_TILES av1Tiles = {};
    D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_CODEC_DATA av1Picture = {};

    D3D12_VIDEO_ENCODER_AV1_SEQUENCE_STRUCTURE av1Sequence = {};
    av1Sequence.IntraDistance = sessionDesc.maxReferenceNum ? 60 : 1;
    av1Sequence.InterFramePeriod = sessionDesc.maxReferenceNum ? 1 : 0;

    D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE gop = {};
    if (sessionDesc.codec == VideoCodec::H264) {
        gop.DataSize = sizeof(h264Gop);
        gop.pH264GroupOfPictures = &h264Gop;
    } else if (sessionDesc.codec == VideoCodec::H265) {
        gop.DataSize = sizeof(hevcGop);
        gop.pHEVCGroupOfPictures = &hevcGop;
    } else if (sessionDesc.codec == VideoCodec::AV1) {
        gop.DataSize = sizeof(av1Sequence);
        gop.pAV1SequenceStructure = &av1Sequence;
    } else {
        NRI_REPORT_ERROR(&m_Device, "Unsupported video encode codec");
        return;
    }

    D3D12_VIDEO_ENCODER_SEQUENCE_CONTROL_DESC sequenceControl = {};
    sequenceControl.Flags = D3D12_VIDEO_ENCODER_SEQUENCE_CONTROL_FLAG_NONE;
    sequenceControl.RateControl = rateControlState.rateControl;
    sequenceControl.PictureTargetResolution = {sessionDesc.width, sessionDesc.height};
    sequenceControl.SelectedLayoutMode = D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_FULL_FRAME;
    sequenceControl.CodecGopSequence = gop;
    if (sessionDesc.codec == VideoCodec::AV1) {
        const VideoAV1TileLayoutDesc* tileLayout = videoEncodeDesc.av1PictureDesc ? videoEncodeDesc.av1PictureDesc->tileLayout : nullptr;
        if (tileLayout && (!tileLayout->columnNum || !tileLayout->rowNum || tileLayout->columnNum > 64 || tileLayout->rowNum > 64)) {
            NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->tileLayout' is invalid");
            return;
        }

        if (tileLayout && (tileLayout->columnNum != 1 || tileLayout->rowNum != 1)) {
            NRI_REPORT_ERROR(&m_Device, "D3D12 AV1 encode supports only a single tile");
            return;
        }

        av1Tiles.RowCount = tileLayout ? tileLayout->rowNum : 1;
        av1Tiles.ColCount = tileLayout ? tileLayout->columnNum : 1;
        sequenceControl.FrameSubregionsLayoutData.DataSize = sizeof(av1Tiles);
        sequenceControl.FrameSubregionsLayoutData.pTilesPartition_AV1 = &av1Tiles;
    }

    const VideoEncodePictureDesc defaultPicture = {VideoEncodeFrameType::IDR, 0, 0, 0, 0};
    VideoEncodePictureDesc pictureDesc = videoEncodeDesc.pictureDesc ? *videoEncodeDesc.pictureDesc : defaultPicture;
    if (videoEncodeDesc.flags & VideoEncodeBits::FORCE_KEY_FRAME)
        pictureDesc.frameType = VideoEncodeFrameType::IDR;
    if (!IsVideoEncodeFrameTypeSupportedByD3D12(sessionDesc.codec, pictureDesc.frameType, session.m_BFrameSupported)) {
        NRI_REPORT_ERROR(&m_Device, "D3D12 video encode session does not support the requested frame type");
        return;
    }

    D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA_H264 h264Picture = {};
    switch (pictureDesc.frameType) {
        case VideoEncodeFrameType::IDR:
            h264Picture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_H264_IDR_FRAME;
            break;
        case VideoEncodeFrameType::I:
            h264Picture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_H264_I_FRAME;
            break;
        case VideoEncodeFrameType::P:
            h264Picture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_H264_P_FRAME;
            break;
        case VideoEncodeFrameType::B:
            h264Picture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_H264_B_FRAME;
            break;
        default:
            NRI_REPORT_ERROR(&m_Device, "Unsupported video encode frame type");
            return;
    }
    h264Picture.pic_parameter_set_id = videoEncodeDesc.h264PictureDesc ? videoEncodeDesc.h264PictureDesc->pictureParameterSetId : 0;
    h264Picture.idr_pic_id = pictureDesc.idrPictureId;
    h264Picture.PictureOrderCountNumber = pictureDesc.pictureOrderCount;
    h264Picture.FrameDecodingOrderNumber = pictureDesc.frameIndex;
    h264Picture.TemporalLayerIndex = pictureDesc.temporalLayer;
    h264Picture.List0ReferenceFramesCount = h264List0ReferenceNum;
    h264Picture.pList0ReferenceFrames = h264List0ReferenceNum ? (UINT*)h264List0References : nullptr;
    h264Picture.List1ReferenceFramesCount = h264List1ReferenceNum;
    h264Picture.pList1ReferenceFrames = h264List1ReferenceNum ? (UINT*)h264List1References : nullptr;
    h264Picture.ReferenceFramesReconPictureDescriptorsCount = videoEncodeDesc.referenceNum;
    h264Picture.pReferenceFramesReconPictureDescriptors = videoEncodeDesc.referenceNum ? (D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_H264*)h264ReferenceDescriptors : nullptr;

    switch (pictureDesc.frameType) {
        case VideoEncodeFrameType::IDR:
            hevcPicture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_IDR_FRAME;
            break;
        case VideoEncodeFrameType::I:
            hevcPicture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_I_FRAME;
            break;
        case VideoEncodeFrameType::P:
            hevcPicture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_P_FRAME;
            break;
        case VideoEncodeFrameType::B:
            hevcPicture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_B_FRAME;
            break;
        default:
            NRI_REPORT_ERROR(&m_Device, "Unsupported video encode frame type");
            return;
    }
    if (sessionDesc.codec == VideoCodec::H265 && videoEncodeDesc.referenceNum > 15) {
        NRI_REPORT_ERROR(&m_Device, "'referenceNum' exceeds the H.265 reference list size");
        return;
    }

    Scratch<UINT> hevcList0References = NRI_ALLOCATE_SCRATCH(m_Device, UINT, videoEncodeDesc.referenceNum ? videoEncodeDesc.referenceNum : 1);
    Scratch<UINT> hevcList1References = NRI_ALLOCATE_SCRATCH(m_Device, UINT, videoEncodeDesc.referenceNum ? videoEncodeDesc.referenceNum : 1);
    Scratch<D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_HEVC> hevcReferenceDescriptors = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_HEVC, videoEncodeDesc.referenceNum ? videoEncodeDesc.referenceNum : 1);
    if (sessionDesc.codec == VideoCodec::H265) {
        VideoEncodeHEVCReferenceListsD3D12 hevcReferenceLists = {};
        if (!BuildVideoEncodeHEVCReferenceListsD3D12(videoEncodeDesc.references, videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, pictureDesc.frameType,
                pictureDesc.pictureOrderCount, hevcReferenceLists)) {
            if (hevcReferenceLists.missingDescriptor)
                NRI_REPORT_ERROR(&m_Device, "'h265ReferenceDescs' must describe every H.265 reference");
            else if (hevcReferenceLists.invalidPictureOrderCount)
                NRI_REPORT_ERROR(&m_Device, "'h265ReferenceDescs[%u].pictureOrderCount' is invalid for the current H.265 frame type", hevcReferenceLists.failingReference);
            else
                NRI_REPORT_ERROR(&m_Device, "'referenceNum' exceeds the H.265 reference list size");
            return;
        }
        for (uint32_t i = 0; i < hevcReferenceLists.list0Num; i++)
            hevcList0References[i] = hevcReferenceLists.list0[i];
        for (uint32_t i = 0; i < hevcReferenceLists.list1Num; i++)
            hevcList1References[i] = hevcReferenceLists.list1[i];

        for (uint32_t i = 0; i < videoEncodeDesc.referenceNum; i++) {
            const VideoH265ReferenceDesc* referenceDesc = FindVideoH265ReferenceDescD3D12(videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, videoEncodeDesc.references[i].slot);
            hevcReferenceDescriptors[i] = {};
            hevcReferenceDescriptors[i].ReconstructedPictureResourceIndex = i;
            hevcReferenceDescriptors[i].IsRefUsedByCurrentPic = TRUE;
            hevcReferenceDescriptors[i].IsLongTermReference = referenceDesc && referenceDesc->longTerm;
            hevcReferenceDescriptors[i].PictureOrderCountNumber = referenceDesc ? (UINT)referenceDesc->pictureOrderCount : videoEncodeDesc.references[i].slot;
            hevcReferenceDescriptors[i].TemporalLayerIndex = referenceDesc ? referenceDesc->temporalLayer : 0;
        }

        hevcPicture.slice_pic_parameter_set_id = 0;
        hevcPicture.PictureOrderCountNumber = (UINT)pictureDesc.pictureOrderCount;
        hevcPicture.TemporalLayerIndex = pictureDesc.temporalLayer;
        hevcPicture.List0ReferenceFramesCount = hevcReferenceLists.list0Num;
        hevcPicture.pList0ReferenceFrames = hevcReferenceLists.list0Num ? (UINT*)hevcList0References : nullptr;
        hevcPicture.List1ReferenceFramesCount = hevcReferenceLists.list1Num;
        hevcPicture.pList1ReferenceFrames = hevcReferenceLists.list1Num ? (UINT*)hevcList1References : nullptr;
        hevcPicture.ReferenceFramesReconPictureDescriptorsCount = videoEncodeDesc.referenceNum;
        hevcPicture.pReferenceFramesReconPictureDescriptors = videoEncodeDesc.referenceNum ? (D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_HEVC*)hevcReferenceDescriptors : nullptr;
    }

    uint8_t av1RefreshFrameFlags = 0;
    if (sessionDesc.codec == VideoCodec::AV1) {
        D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE frameType = GetVideoEncodeAV1FrameTypeD3D12(pictureDesc.frameType);
        if (frameType == (D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE)-1) {
            NRI_REPORT_ERROR(&m_Device, "Unsupported AV1 video encode frame type");
            return;
        }

        for (auto& referenceDescriptor : av1Picture.ReferenceFramesReconPictureDescriptors)
            referenceDescriptor.ReconstructedPictureResourceIndex = 0xFF;
        std::array<bool, 7> activeReferenceNames = {};
        std::array<uint32_t, 7> referenceNameSlots = {};
        for (uint32_t& slot : referenceNameSlots)
            slot = UINT32_MAX;
        std::array<bool, 7> av1ReferenceNameSpecified = {};
        std::array<uint32_t, 8> av1DPBSlotResourceIndices = {};
        for (uint32_t& resourceIndex : av1DPBSlotResourceIndices)
            resourceIndex = UINT32_MAX;

        const VideoAV1PictureBits pictureFlags = (videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->flags != VideoAV1PictureBits::NONE)
            ? videoEncodeDesc.av1PictureDesc->flags
            : GetDefaultVideoAV1PictureFlags();
        if (pictureFlags & VideoAV1PictureBits::ERROR_RESILIENT_MODE)
            av1Picture.Flags |= D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_FLAG_ENABLE_ERROR_RESILIENT_MODE;
        if (session.m_AV1FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_RESTORATION_FILTER) {
            for (auto& type : av1Picture.FrameRestorationConfig.FrameRestorationType)
                type = D3D12_VIDEO_ENCODER_AV1_RESTORATION_TYPE_DISABLED;
            for (auto& tileSize : av1Picture.FrameRestorationConfig.LoopRestorationPixelSize)
                tileSize = D3D12_VIDEO_ENCODER_AV1_RESTORATION_TILESIZE_DISABLED;
        }
        if ((session.m_AV1FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FORCED_INTEGER_MOTION_VECTORS) && (pictureFlags & VideoAV1PictureBits::FORCE_INTEGER_MV))
            av1Picture.Flags |= D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_FLAG_FORCE_INTEGER_MOTION_VECTORS;
        if (videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->segmentation) {
            NRI_REPORT_ERROR(&m_Device, "D3D12 AV1 encode does not support explicit segmentation");
            return;
        }
        if ((session.m_AV1FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_AUTO_SEGMENTATION) && (pictureFlags & VideoAV1PictureBits::SEGMENTATION_ENABLED))
            av1Picture.Flags |= D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_FLAG_ENABLE_FRAME_SEGMENTATION_AUTO;
        av1Picture.FrameType = frameType;
        av1Picture.CompoundPredictionType = D3D12_VIDEO_ENCODER_AV1_COMP_PREDICTION_TYPE_SINGLE_REFERENCE;
        av1Picture.InterpolationFilter = videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->interpolationFilter
            ? (D3D12_VIDEO_ENCODER_AV1_INTERPOLATION_FILTERS)videoEncodeDesc.av1PictureDesc->interpolationFilter
            : D3D12_VIDEO_ENCODER_AV1_INTERPOLATION_FILTERS_SWITCHABLE;
        av1Picture.TxMode = videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->txMode
            ? (D3D12_VIDEO_ENCODER_AV1_TX_MODE)videoEncodeDesc.av1PictureDesc->txMode
            : (pictureDesc.frameType == VideoEncodeFrameType::P ? D3D12_VIDEO_ENCODER_AV1_TX_MODE_SELECT : D3D12_VIDEO_ENCODER_AV1_TX_MODE_LARGEST);
        av1Picture.OrderHint = videoEncodeDesc.av1PictureDesc ? videoEncodeDesc.av1PictureDesc->orderHint : (UINT)pictureDesc.pictureOrderCount;
        av1Picture.PictureIndex = videoEncodeDesc.av1PictureDesc ? videoEncodeDesc.av1PictureDesc->currentFrameId : pictureDesc.frameIndex;
        av1Picture.TemporalLayerIndexPlus1 = pictureDesc.temporalLayer + 1;
        av1Picture.SpatialLayerIndexPlus1 = 1;
        av1Picture.PrimaryRefFrame = 7;
        av1Picture.RefreshFrameFlags = videoEncodeDesc.av1PictureDesc ? videoEncodeDesc.av1PictureDesc->refreshFrameFlags : (pictureDesc.frameType == VideoEncodeFrameType::IDR ? 0xFF : 0);
        if (frameType == D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_KEY_FRAME) {
            av1Picture.PrimaryRefFrame = 7;
            av1Picture.RefreshFrameFlags = 0xFF;
            if (videoEncodeDesc.referenceNum) {
                NRI_REPORT_ERROR(&m_Device, "AV1 key frames must not reference previous pictures");
                return;
            }
        }
        av1Picture.Quantization.BaseQIndex = videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->baseQIndex
            ? videoEncodeDesc.av1PictureDesc->baseQIndex
            : GetVideoEncodeQPByFrameTypeD3D12(rateControlDesc, pictureDesc.frameType);
        if (videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->quantization) {
            const VideoAV1QuantizationDesc& quantization = *videoEncodeDesc.av1PictureDesc->quantization;
            av1Picture.Quantization.YDCDeltaQ = quantization.deltaQYDc;
            av1Picture.Quantization.UDCDeltaQ = quantization.deltaQUDc;
            av1Picture.Quantization.UACDeltaQ = quantization.deltaQUAc;
            av1Picture.Quantization.VDCDeltaQ = quantization.deltaQVDc;
            av1Picture.Quantization.VACDeltaQ = quantization.deltaQVAc;
            av1Picture.Quantization.UsingQMatrix = quantization.usingQmatrix;
            av1Picture.Quantization.QMY = quantization.qmY;
            av1Picture.Quantization.QMU = quantization.qmU;
            av1Picture.Quantization.QMV = quantization.qmV;
        }
        if (session.m_AV1FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_QUANTIZATION_DELTAS) {
            av1Picture.QuantizationDelta.DeltaQPresent = !!(pictureFlags & VideoAV1PictureBits::DELTA_Q_PRESENT);
            av1Picture.QuantizationDelta.DeltaQRes = videoEncodeDesc.av1PictureDesc ? videoEncodeDesc.av1PictureDesc->deltaQRes : 0;
        }
        if (session.m_AV1FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_FILTER_DELTAS) {
            av1Picture.LoopFilter.LoopFilterDeltaEnabled = 1;
            av1Picture.LoopFilter.UpdateRefDelta = 1;
            av1Picture.LoopFilter.RefDeltas[0] = 1;
            av1Picture.LoopFilter.RefDeltas[4] = -1;
            av1Picture.LoopFilter.RefDeltas[6] = -1;
            av1Picture.LoopFilter.RefDeltas[7] = -1;
            av1Picture.LoopFilterDelta.DeltaLFPresent = !!(pictureFlags & VideoAV1PictureBits::DELTA_LF_PRESENT);
            av1Picture.LoopFilterDelta.DeltaLFMulti = !!(pictureFlags & VideoAV1PictureBits::DELTA_LF_MULTI);
            av1Picture.LoopFilterDelta.DeltaLFRes = videoEncodeDesc.av1PictureDesc ? videoEncodeDesc.av1PictureDesc->deltaLfRes : 0;
        }
        if (videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->loopFilter) {
            const VideoAV1LoopFilterDesc& loopFilter = *videoEncodeDesc.av1PictureDesc->loopFilter;
            av1Picture.LoopFilter.LoopFilterLevel[0] = loopFilter.level[0];
            av1Picture.LoopFilter.LoopFilterLevel[1] = loopFilter.level[1];
            av1Picture.LoopFilter.LoopFilterLevelU = loopFilter.level[2];
            av1Picture.LoopFilter.LoopFilterLevelV = loopFilter.level[3];
            av1Picture.LoopFilter.LoopFilterSharpnessLevel = loopFilter.sharpness;
            av1Picture.LoopFilter.LoopFilterDeltaEnabled = loopFilter.deltaEnabled;
            av1Picture.LoopFilter.UpdateRefDelta = loopFilter.deltaUpdate;
            av1Picture.LoopFilter.UpdateModeDelta = loopFilter.updateModeDelta;
            for (uint32_t i = 0; i < 8; i++)
                av1Picture.LoopFilter.RefDeltas[i] = loopFilter.refDeltas[i];
            for (uint32_t i = 0; i < 2; i++)
                av1Picture.LoopFilter.ModeDeltas[i] = loopFilter.modeDeltas[i];
        }
        if (session.m_AV1FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_CDEF_FILTERING) {
            av1Picture.CDEF.CdefDampingMinus3 = videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->cdefDampingMinus3 ? videoEncodeDesc.av1PictureDesc->cdefDampingMinus3 : 3;
            av1Picture.CDEF.CdefBits = videoEncodeDesc.av1PictureDesc ? videoEncodeDesc.av1PictureDesc->cdefBits : 0;
            if (videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->cdef) {
                const VideoAV1CdefDesc& cdef = *videoEncodeDesc.av1PictureDesc->cdef;
                for (uint32_t i = 0; i < 8; i++) {
                    av1Picture.CDEF.CdefYPriStrength[i] = cdef.yPrimaryStrength[i];
                    av1Picture.CDEF.CdefYSecStrength[i] = cdef.ySecondaryStrength[i];
                    av1Picture.CDEF.CdefUVPriStrength[i] = cdef.uvPrimaryStrength[i];
                    av1Picture.CDEF.CdefUVSecStrength[i] = cdef.uvSecondaryStrength[i];
                }
            }
        }

        if (videoEncodeDesc.av1PictureDesc) {
            if (videoEncodeDesc.av1PictureDesc->referenceNum > 8) {
                NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->referenceNum' exceeds AV1 DPB slot count");
                return;
            }

            for (uint32_t i = 0; i < videoEncodeDesc.av1PictureDesc->referenceNum; i++) {
                const VideoAV1ReferenceDesc& reference = videoEncodeDesc.av1PictureDesc->references[i];
                const uint32_t referenceNameIndex = GetVideoEncodeAV1ReferenceNameIndexD3D12(reference.name);

                uint32_t resourceIndex = UINT32_MAX;
                for (uint32_t j = 0; j < videoEncodeDesc.referenceNum; j++) {
                    if (videoEncodeDesc.references[j].slot == reference.slot) {
                        resourceIndex = j;
                        break;
                    }
                }
                if (resourceIndex == UINT32_MAX) {
                    NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->references[%u].slot' is not present in 'references'", i);
                    return;
                }
                if (reference.refFrameIndex >= 8) {
                    NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->references[%u].refFrameIndex' is invalid", i);
                    return;
                }

                av1Picture.ReferenceFramesReconPictureDescriptors[reference.refFrameIndex] = {};
                av1Picture.ReferenceFramesReconPictureDescriptors[reference.refFrameIndex].ReconstructedPictureResourceIndex = resourceIndex;
                av1Picture.ReferenceFramesReconPictureDescriptors[reference.refFrameIndex].TemporalLayerIndexPlus1 = reference.frameType == VideoEncodeFrameType::MAX_NUM ? 0 : 1;
                av1Picture.ReferenceFramesReconPictureDescriptors[reference.refFrameIndex].SpatialLayerIndexPlus1 = 1;
                av1Picture.ReferenceFramesReconPictureDescriptors[reference.refFrameIndex].FrameType = GetVideoEncodeAV1FrameTypeD3D12(reference.frameType);
                av1Picture.ReferenceFramesReconPictureDescriptors[reference.refFrameIndex].OrderHint = reference.orderHint;
                av1Picture.ReferenceFramesReconPictureDescriptors[reference.refFrameIndex].PictureIndex = reference.frameId;
                av1DPBSlotResourceIndices[reference.refFrameIndex] = resourceIndex;
                if (reference.name != VideoAV1ReferenceName::NONE) {
                    if (referenceNameIndex >= 7) {
                        NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->references[%u].name' is invalid", i);
                        return;
                    }

                    av1Picture.ReferenceIndices[referenceNameIndex] = reference.refFrameIndex;
                    activeReferenceNames[referenceNameIndex] = true;
                    av1ReferenceNameSpecified[referenceNameIndex] = true;
                    referenceNameSlots[referenceNameIndex] = reference.slot;
                }
            }

            // Unspecified AV1 reference names must resolve to an invalid DPB descriptor, otherwise D3D12 treats them as active references.
            uint32_t invalidReferenceIndex = UINT32_MAX;
            for (uint32_t i = 0; i < 8; i++) {
                if (av1Picture.ReferenceFramesReconPictureDescriptors[i].ReconstructedPictureResourceIndex == 0xFF) {
                    invalidReferenceIndex = i;
                    break;
                }
            }
            const uint32_t primaryReferenceNameIndex = GetVideoEncodeAV1ReferenceNameIndexD3D12(videoEncodeDesc.av1PictureDesc->primaryReferenceName);
            if (primaryReferenceNameIndex < 7 && !activeReferenceNames[primaryReferenceNameIndex]) {
                NRI_REPORT_ERROR(&m_Device, "'av1PictureDesc->primaryReferenceName' does not name an active reference");
                return;
            }
            const uint32_t unusedReferenceIndex = primaryReferenceNameIndex < 7 ? av1Picture.ReferenceIndices[primaryReferenceNameIndex] : invalidReferenceIndex;
            av1Picture.PrimaryRefFrame = primaryReferenceNameIndex < 7 ? (UCHAR)primaryReferenceNameIndex : 7;
            for (uint32_t i = 0; i < 7; i++) {
                if (av1ReferenceNameSpecified[i])
                    continue;

                if (unusedReferenceIndex == UINT32_MAX) {
                    NRI_REPORT_ERROR(&m_Device, "AV1 DPB snapshot has no DPB slot for unused reference names");
                    return;
                }

                av1Picture.ReferenceIndices[i] = unusedReferenceIndex;
            }

            for (uint32_t i = 0; i < videoEncodeDesc.referenceNum; i++) {
                if (!HasVideoEncodeAV1DPBSlotResourceD3D12(av1DPBSlotResourceIndices.data(), i)) {
                    NRI_REPORT_ERROR(&m_Device, "'references[%u].slot' is not present in the AV1 DPB snapshot", i);
                    return;
                }
            }

        } else if (videoEncodeDesc.referenceNum) {
            av1Picture.ReferenceFramesReconPictureDescriptors[0] = {};
            av1Picture.ReferenceFramesReconPictureDescriptors[0].ReconstructedPictureResourceIndex = 0;
            av1Picture.ReferenceFramesReconPictureDescriptors[0].TemporalLayerIndexPlus1 = 1;
            av1Picture.ReferenceFramesReconPictureDescriptors[0].SpatialLayerIndexPlus1 = 1;
            av1Picture.ReferenceFramesReconPictureDescriptors[0].FrameType = D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_KEY_FRAME;
            av1Picture.ReferenceIndices[0] = 0;
            av1Picture.PrimaryRefFrame = 0;
        }
        av1RefreshFrameFlags = (uint8_t)av1Picture.RefreshFrameFlags;
    }

    if (sessionDesc.codec == VideoCodec::H264) {
        pictureCodecData.DataSize = sizeof(h264Picture);
        pictureCodecData.pH264PicData = &h264Picture;
    } else if (sessionDesc.codec == VideoCodec::H265) {
        pictureCodecData.DataSize = sizeof(hevcPicture);
        pictureCodecData.pHEVCPicData = &hevcPicture;
    } else {
        pictureCodecData.DataSize = sizeof(av1Picture);
        pictureCodecData.pAV1PicData = &av1Picture;
    }

    const bool isAV1NonReferencePicture = sessionDesc.codec == VideoCodec::AV1 && av1RefreshFrameFlags == 0;
    if (sessionDesc.codec == VideoCodec::AV1 && !isAV1NonReferencePicture && !videoEncodeDesc.reconstructedPicture) {
        NRI_REPORT_ERROR(&m_Device, "AV1 frames that refresh DPB slots require 'reconstructedPicture'");
        return;
    }

    const bool isUsedAsReferencePicture = IsVideoEncodePictureUsedAsReferenceD3D12(sessionDesc.codec, pictureDesc.frameType, sessionDesc.maxReferenceNum,
        videoEncodeDesc.reconstructedPicture != nullptr, av1RefreshFrameFlags);

    if (isUsedAsReferencePicture)
        pictureControl.Flags |= D3D12_VIDEO_ENCODER_PICTURE_CONTROL_FLAG_USED_AS_REFERENCE_PICTURE;
    pictureControl.PictureControlCodecData = pictureCodecData;
    pictureControl.ReferenceFrames.NumTexture2Ds = videoEncodeDesc.referenceNum;
    pictureControl.ReferenceFrames.ppTexture2Ds = referenceResources;
    pictureControl.ReferenceFrames.pSubresources = referenceSubresources;

    VideoPictureD3D12& srcPicture = *(VideoPictureD3D12*)videoEncodeDesc.srcPicture;

    input.SequenceControlDesc = sequenceControl;
    input.PictureControlDesc = pictureControl;
    input.CurrentFrameBitstreamMetadataSize = (UINT)videoEncodeDesc.bitstreamMetadataSize;
    input.pInputFrame = (ID3D12Resource*)(*srcPicture.m_Texture);
    input.InputFrameSubresource = srcPicture.m_Subresource;

    bitstream.pBuffer = (ID3D12Resource*)dstBitstream;
    bitstream.FrameStartOffset = videoEncodeDesc.dstBitstream.offset;
    if (isUsedAsReferencePicture) {
        VideoPictureD3D12& reconstructedPicture = *(VideoPictureD3D12*)videoEncodeDesc.reconstructedPicture;
        output.ReconstructedPicture.pReconstructedPicture = (ID3D12Resource*)(*reconstructedPicture.m_Texture);
        output.ReconstructedPicture.ReconstructedPictureSubresource = reconstructedPicture.m_Subresource;
    }
    output.EncoderOutputMetadata.pBuffer = (ID3D12Resource*)(*(BufferD3D12*)videoEncodeDesc.metadata);
    output.EncoderOutputMetadata.Offset = videoEncodeDesc.metadataOffset;

    D3D12_VIDEO_ENCODER_PROFILE_H264 resolveH264Profile = D3D12_VIDEO_ENCODER_PROFILE_H264_HIGH;
    D3D12_VIDEO_ENCODER_PROFILE_HEVC resolveHevcProfile = (sessionDesc.format == Format::P010_UNORM || sessionDesc.format == Format::P016_UNORM)
        ? D3D12_VIDEO_ENCODER_PROFILE_HEVC_MAIN10
        : D3D12_VIDEO_ENCODER_PROFILE_HEVC_MAIN;

    D3D12_VIDEO_ENCODER_PROFILE_DESC resolveProfile = {};
    if (sessionDesc.codec == VideoCodec::H264) {
        resolveProfile.DataSize = sizeof(resolveH264Profile);
        resolveProfile.pH264Profile = &resolveH264Profile;
    } else if (sessionDesc.codec == VideoCodec::H265) {
        resolveProfile.DataSize = sizeof(resolveHevcProfile);
        resolveProfile.pHEVCProfile = &resolveHevcProfile;
    } else {
        resolveProfile.DataSize = sizeof(resolveAv1Profile);
        resolveProfile.pAV1Profile = &resolveAv1Profile;
    }

    if (videoEncodeDesc.resolvedMetadata) {
        resolveInput.EncoderCodec = GetVideoEncodeCodecD3D12(sessionDesc.codec);
        resolveInput.EncoderProfile = resolveProfile;
        resolveInput.EncoderInputFormat = GetDxgiFormat(sessionDesc.format).typed;
        resolveInput.EncodedPictureEffectiveResolution = {sessionDesc.width, sessionDesc.height};
        resolveInput.HWLayoutMetadata = output.EncoderOutputMetadata;
        resolveOutput.ResolvedLayoutMetadata.pBuffer = (ID3D12Resource*)(*(BufferD3D12*)videoEncodeDesc.resolvedMetadata);
        resolveOutput.ResolvedLayoutMetadata.Offset = videoEncodeDesc.resolvedMetadataOffset;
    }

    ID3D12VideoEncodeCommandListBest* commandList = GetVideoEncodeCommandList();

    commandList->EncodeFrame1(session.GetEncoder(), session.GetEncoderHeap(), &input, &output);

    if (videoEncodeDesc.resolvedMetadata) {
        D3D12_RESOURCE_BARRIER metadataReady = {};
        metadataReady.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        metadataReady.Transition.pResource = resolveInput.HWLayoutMetadata.pBuffer;
        metadataReady.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        metadataReady.Transition.StateBefore = D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE;
        metadataReady.Transition.StateAfter = D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ;
        commandList->ResourceBarrier(1, &metadataReady); // TODO-VIDEO: support enhanced barriers too

        commandList->ResolveEncoderOutputMetadata1(&resolveInput, &resolveOutput);
    }
#else
    MaybeUnused(videoEncodeDesc);
#endif
}

NRI_INLINE void CommandBufferD3D12::SetViewports(const Viewport* viewports, uint32_t viewportNum) {
    Scratch<D3D12_VIEWPORT> d3dViewports = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_VIEWPORT, viewportNum);
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

    GetGraphicsCommandList()->RSSetViewports(viewportNum, d3dViewports);
}

NRI_INLINE void CommandBufferD3D12::SetScissors(const Rect* rects, uint32_t rectNum) {
    Scratch<D3D12_RECT> d3dRects = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_RECT, rectNum);
    ConvertRects(rects, rectNum, d3dRects);

    GetGraphicsCommandList()->RSSetScissorRects(rectNum, d3dRects);
}

NRI_INLINE void CommandBufferD3D12::SetDepthBounds(float boundsMin, float boundsMax) {
    GetGraphicsCommandList()->OMSetDepthBounds(boundsMin, boundsMax);
}

NRI_INLINE void CommandBufferD3D12::SetStencilReference(uint8_t frontRef, uint8_t backRef) {
    MaybeUnused(backRef);
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    if (m_Device.GetDesc().features.independentFrontAndBackStencilReferenceAndMasks && m_Device.GetVersion() >= 8)
        GetGraphicsCommandList()->OMSetFrontAndBackStencilRef(frontRef, backRef);
    else
#endif
        GetGraphicsCommandList()->OMSetStencilRef(frontRef);
}

NRI_INLINE void CommandBufferD3D12::SetSampleLocations(const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    static_assert(sizeof(D3D12_SAMPLE_POSITION) == sizeof(SampleLocation));

    uint32_t pixelNum = locationNum / sampleNum;
    GetGraphicsCommandList()->SetSamplePositions(sampleNum, pixelNum, (D3D12_SAMPLE_POSITION*)locations);
}

NRI_INLINE void CommandBufferD3D12::SetBlendConstants(const Color32f& color) {
    GetGraphicsCommandList()->OMSetBlendFactor(&color.x);
}

NRI_INLINE void CommandBufferD3D12::SetShadingRate(const ShadingRateDesc& shadingRateDesc) {
    D3D12_SHADING_RATE shadingRate = GetShadingRate(shadingRateDesc.shadingRate);
    D3D12_SHADING_RATE_COMBINER shadingRateCombiners[2] = {
        GetShadingRateCombiner(shadingRateDesc.primitiveCombiner),
        GetShadingRateCombiner(shadingRateDesc.attachmentCombiner),
    };

    GetGraphicsCommandList()->RSSetShadingRate(shadingRate, shadingRateCombiners);
}

NRI_INLINE void CommandBufferD3D12::SetDepthBias(const DepthBiasDesc& depthBiasDesc) {
    MaybeUnused(depthBiasDesc);
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    if (m_Device.GetVersion() >= 9)
        GetGraphicsCommandList()->RSSetDepthBias(depthBiasDesc.constant, depthBiasDesc.clamp, depthBiasDesc.slope);
#endif
}

NRI_INLINE void CommandBufferD3D12::ClearAttachments(const ClearAttachmentDesc* clearAttachmentDescs, uint32_t clearAttachmentDescNum, const Rect* rects, uint32_t rectNum) {
    Scratch<D3D12_RECT> d3dRects = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_RECT, rectNum);
    ConvertRects(rects, rectNum, d3dRects);

    for (uint32_t i = 0; i < clearAttachmentDescNum; i++) {
        const ClearAttachmentDesc& clearAttachmentDesc = clearAttachmentDescs[i];

        if (clearAttachmentDescs[i].planes & PlaneBits::COLOR) {
            D3D12_CPU_DESCRIPTOR_HANDLE handle = {m_RenderTargets[clearAttachmentDesc.colorAttachmentIndex].attachment->GetDescriptorHandleCPU()};
            GetGraphicsCommandList()->ClearRenderTargetView(handle, &clearAttachmentDesc.value.color.f.x, rectNum, d3dRects);
        } else {
            D3D12_CLEAR_FLAGS clearFlags = (D3D12_CLEAR_FLAGS)0;
            if (clearAttachmentDesc.planes & PlaneBits::DEPTH)
                clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
            if (clearAttachmentDesc.planes & PlaneBits::STENCIL)
                clearFlags |= D3D12_CLEAR_FLAG_STENCIL;

            D3D12_CPU_DESCRIPTOR_HANDLE handle = {m_Depth.attachment->GetDescriptorHandleCPU()};
            GetGraphicsCommandList()->ClearDepthStencilView(handle, clearFlags, clearAttachmentDesc.value.depthStencil.depth, clearAttachmentDesc.value.depthStencil.stencil, rectNum, d3dRects);
        }
    }
}

NRI_INLINE void CommandBufferD3D12::ClearStorage(const ClearStorageDesc& clearStorageDesc) {
    DescriptorSetD3D12* descriptorSet = m_DescriptorSets[clearStorageDesc.setIndex];
    const DescriptorD3D12& descriptorD3D12 = *(DescriptorD3D12*)clearStorageDesc.descriptor;

    // TODO: typed buffers are currently cleared according to the format, it seems to be more reliable than using integers for all buffers
    const FormatProps& formatProps = GetFormatProps(descriptorD3D12.GetFormat());
    DescriptorHandleGPU handleGPU = descriptorSet->GetDescriptorHandleGPU(clearStorageDesc.rangeIndex, clearStorageDesc.descriptorIndex);
    DescriptorHandleCPU handleCPU = descriptorD3D12.GetDescriptorHandleCPU();

    if (formatProps.isInteger)
        GetGraphicsCommandList()->ClearUnorderedAccessViewUint({handleGPU}, {handleCPU}, descriptorD3D12.GetResource(), &clearStorageDesc.value.ui.x, 0, nullptr);
    else
        GetGraphicsCommandList()->ClearUnorderedAccessViewFloat({handleGPU}, {handleCPU}, descriptorD3D12.GetResource(), &clearStorageDesc.value.f.x, 0, nullptr);
}

NRI_INLINE void CommandBufferD3D12::BeginRendering(const RenderingDesc& renderingDesc) {
    ResetAttachments();

    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT> renderTargets = {};
    for (uint32_t i = 0; i < renderingDesc.colorNum; i++) {
        const AttachmentDesc& attachmentDesc = renderingDesc.colors[i];

        m_RenderTargets[i].attachment = (DescriptorD3D12*)attachmentDesc.descriptor;
        m_RenderTargets[i].resolveDst = (DescriptorD3D12*)attachmentDesc.resolveDst;
        m_RenderTargets[i].resolveOp = attachmentDesc.resolveOp;

        renderTargets[i].ptr = m_RenderTargets[i].attachment->GetDescriptorHandleCPU();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE depthStencil = {};
    if (renderingDesc.depth.descriptor) {
        m_Depth.attachment = (DescriptorD3D12*)renderingDesc.depth.descriptor;
        m_Depth.resolveDst = (DescriptorD3D12*)renderingDesc.depth.resolveDst;
        m_Depth.resolveOp = renderingDesc.depth.resolveOp;

        depthStencil.ptr = m_Depth.attachment->GetDescriptorHandleCPU();

        const FormatProps& formatProps = GetFormatProps(m_Depth.attachment->GetFormat());
        if (formatProps.isStencil)
            m_Stencil = m_Depth;
    }

    if (renderingDesc.stencil.descriptor) { // it's safe to do it this way, since there are no "stencil-only" formats
        m_Stencil.attachment = (DescriptorD3D12*)renderingDesc.stencil.descriptor;
        m_Stencil.resolveDst = (DescriptorD3D12*)renderingDesc.stencil.resolveDst;
        m_Stencil.resolveOp = renderingDesc.stencil.resolveOp;

        depthStencil.ptr = m_Stencil.attachment->GetDescriptorHandleCPU();
    }

    // Bind
    GetGraphicsCommandList()->OMSetRenderTargets(renderingDesc.colorNum, renderTargets.data(), FALSE, depthStencil.ptr ? &depthStencil : nullptr);

    // Clear
    for (uint32_t i = 0; i < renderingDesc.colorNum; i++) {
        const AttachmentDesc& attachmentDesc = renderingDesc.colors[i];

        if (attachmentDesc.loadOp == LoadOp::CLEAR)
            GetGraphicsCommandList()->ClearRenderTargetView(renderTargets[i], &attachmentDesc.clearValue.color.f.x, 0, nullptr);
    }

    D3D12_CLEAR_FLAGS clearFlags = (D3D12_CLEAR_FLAGS)0;
    if (renderingDesc.depth.loadOp == LoadOp::CLEAR)
        clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
    if (renderingDesc.stencil.loadOp == LoadOp::CLEAR)
        clearFlags |= D3D12_CLEAR_FLAG_STENCIL;

    if (clearFlags)
        GetGraphicsCommandList()->ClearDepthStencilView(depthStencil, clearFlags, renderingDesc.depth.clearValue.depthStencil.depth, renderingDesc.stencil.clearValue.depthStencil.stencil, 0, nullptr);

    // Shading rate
    if (m_Device.GetDesc().tiers.shadingRate >= 2) {
        ID3D12Resource* shadingRateImage = nullptr;
        if (renderingDesc.shadingRate) {
            const DescriptorD3D12& descriptorD3D12 = *(DescriptorD3D12*)renderingDesc.shadingRate;
            shadingRateImage = descriptorD3D12.GetResource();
        }

        GetGraphicsCommandList()->RSSetShadingRateImage(shadingRateImage);
    }

    // Multiview
    if (m_Device.GetDesc().other.viewMaxNum > 1 && renderingDesc.viewMask)
        GetGraphicsCommandList()->SetViewInstanceMask(renderingDesc.viewMask);

    m_RenderPass = true;
}

NRI_INLINE void CommandBufferD3D12::EndRendering() {
    uint32_t resourceBarrierNum = 0;
    if (!m_Device.GetDesc().features.enhancedBarriers) {
        for (const AttachmentDescD3D12& attachmentDesc : m_RenderTargets) {
            if (attachmentDesc.resolveDst) {
                const TexViewDesc& srcDesc = attachmentDesc.attachment->GetTexViewDesc();
                resourceBarrierNum += srcDesc.layerNum;
            }
        }

        if (m_Depth.resolveDst) {
            const TexViewDesc& srcDesc = m_Depth.attachment->GetTexViewDesc();
            resourceBarrierNum += srcDesc.layerNum;
        }

        if (m_Stencil.resolveDst) {
            const TexViewDesc& srcDesc = m_Stencil.attachment->GetTexViewDesc();
            resourceBarrierNum += srcDesc.layerNum;
        }
    }
    Scratch<D3D12_RESOURCE_BARRIER> resourceBarriers = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_RESOURCE_BARRIER, resourceBarrierNum * 2);
    uint32_t barrierNum = 0;

    constexpr uint32_t attachmentNum = (D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT + 2) * 2; // (colors, depth and stencil) x (src and dst)
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    std::array<D3D12_TEXTURE_BARRIER, attachmentNum> textureBarriers = {};

    D3D12_BARRIER_GROUP barrierGroup = {};
    barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
    barrierGroup.pTextureBarriers = textureBarriers.data();
#endif

    for (size_t i = 0; i < attachmentNum; i++) {
        const AttachmentDescD3D12* attachmentDesc = &m_Stencil;
        PlaneBits planeBits = PlaneBits::STENCIL;
        if (i < m_RenderTargets.size()) {
            attachmentDesc = &m_RenderTargets[i];
            planeBits = PlaneBits::COLOR;
        } else if (i == m_RenderTargets.size()) {
            attachmentDesc = &m_Depth;
            planeBits = PlaneBits::DEPTH;
        }

        if (!attachmentDesc->resolveDst)
            continue;

        DescriptorD3D12* resolveSrc = attachmentDesc->attachment;
        DescriptorD3D12* resolveDst = attachmentDesc->resolveDst;
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        if (m_Device.GetDesc().features.enhancedBarriers) {
            FillResolveBarrier(true, *resolveSrc, planeBits, textureBarriers[barrierNum++]);
            FillResolveBarrier(false, *resolveDst, planeBits, textureBarriers[barrierNum++]);
        } else
#endif
        {
            const TexViewDesc& srcDesc = resolveSrc->GetTexViewDesc();
            D3D12_RESOURCE_DESC srcResourceDesc = resolveSrc->GetResource()->GetDesc();

            const TexViewDesc& dstDesc = resolveDst->GetTexViewDesc();
            D3D12_RESOURCE_DESC dstResourceDesc = resolveDst->GetResource()->GetDesc();

            for (uint32_t layer = 0; layer < srcDesc.layerNum; layer++) {
                uint32_t srcSubresource = GetSubresourceIndex(srcDesc.layerOffset + layer, srcResourceDesc.DepthOrArraySize, srcDesc.mipOffset, srcResourceDesc.MipLevels, planeBits);
                FillLegacyResolveBarrier(true, *resolveSrc, srcSubresource, resourceBarriers[barrierNum++]);

                uint32_t dstSubresource = GetSubresourceIndex(dstDesc.layerOffset + layer, dstResourceDesc.DepthOrArraySize, dstDesc.mipOffset, dstResourceDesc.MipLevels, planeBits);
                FillLegacyResolveBarrier(false, *resolveDst, dstSubresource, resourceBarriers[barrierNum++]);
            }
        }
    }

    if (barrierNum) {
        // Barriers to "RESOLVE" state
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        barrierGroup.NumBarriers = barrierNum;
        if (m_Device.GetDesc().features.enhancedBarriers)
            GetGraphicsCommandList()->Barrier(1, &barrierGroup);
        else
#endif
            GetGraphicsCommandList()->ResourceBarrier(barrierNum, resourceBarriers);

        // Resolve
        for (size_t i = 0; i < attachmentNum; i++) {
            const AttachmentDescD3D12* attachmentDesc = &m_Stencil;
            PlaneBits planeBits = PlaneBits::STENCIL;
            if (i < m_RenderTargets.size()) {
                attachmentDesc = &m_RenderTargets[i];
                planeBits = PlaneBits::COLOR;
            } else if (i == m_RenderTargets.size()) {
                attachmentDesc = &m_Depth;
                planeBits = PlaneBits::DEPTH;
            }

            if (!attachmentDesc->resolveDst)
                continue;

            D3D12_RESOLVE_MODE resolveMode = GetResolveOp(attachmentDesc->resolveOp);
            const DxgiFormat& format = GetDxgiFormat(attachmentDesc->resolveDst->GetFormat());

            ID3D12Resource* srcResource = attachmentDesc->attachment->GetResource();
            D3D12_RESOURCE_DESC srcResourceDesc = srcResource->GetDesc();
            const TexViewDesc& srcDesc = attachmentDesc->attachment->GetTexViewDesc();
            uint32_t srcSubresource = GetSubresourceIndex(srcDesc.layerOffset, srcResourceDesc.DepthOrArraySize, srcDesc.mipOffset, srcResourceDesc.MipLevels, planeBits);

            ID3D12Resource* dstResource = attachmentDesc->resolveDst->GetResource();
            D3D12_RESOURCE_DESC dstResourceDesc = dstResource->GetDesc();
            const TexViewDesc& dstDesc = attachmentDesc->resolveDst->GetTexViewDesc();
            uint32_t dstSubresource = GetSubresourceIndex(dstDesc.layerOffset, dstResourceDesc.DepthOrArraySize, dstDesc.mipOffset, dstResourceDesc.DepthOrArraySize, planeBits);

            GetGraphicsCommandList()->ResolveSubresourceRegion(dstResource, dstSubresource, 0, 0, srcResource, srcSubresource, nullptr, format.typed, resolveMode);
        }

        // Restore state
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        if (m_Device.GetDesc().features.enhancedBarriers) {
            for (uint32_t i = 0; i < barrierNum; i++) {
                D3D12_TEXTURE_BARRIER& textureBarrier = textureBarriers[i];

                std::swap(textureBarrier.SyncAfter, textureBarrier.SyncBefore);
                std::swap(textureBarrier.AccessAfter, textureBarrier.AccessBefore);
                std::swap(textureBarrier.LayoutAfter, textureBarrier.LayoutBefore);
            }

            GetGraphicsCommandList()->Barrier(1, &barrierGroup);
        } else
#endif
        {
            for (uint32_t i = 0; i < barrierNum; i++) {
                D3D12_RESOURCE_BARRIER& resourceBarrier = resourceBarriers[i];

                std::swap(resourceBarrier.Transition.StateAfter, resourceBarrier.Transition.StateBefore);
            }

            GetGraphicsCommandList()->ResourceBarrier(barrierNum, resourceBarriers);
        }
    }

    ResetAttachments();

    m_RenderPass = false;
}

NRI_INLINE void CommandBufferD3D12::SetVertexBuffers(uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum) {
    Scratch<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_VERTEX_BUFFER_VIEW, vertexBufferNum);
    for (uint32_t i = 0; i < vertexBufferNum; i++) {
        const VertexBufferDesc& vertexBufferDesc = vertexBufferDescs[i];

        const BufferD3D12* bufferD3D12 = (BufferD3D12*)vertexBufferDesc.buffer;
        if (bufferD3D12) {
            vertexBufferViews[i].BufferLocation = bufferD3D12->GetDeviceAddress() + vertexBufferDesc.offset;
            vertexBufferViews[i].SizeInBytes = (uint32_t)(bufferD3D12->GetDesc().size - vertexBufferDesc.offset);
            vertexBufferViews[i].StrideInBytes = vertexBufferDesc.stride;
        } else {
            vertexBufferViews[i].BufferLocation = 0;
            vertexBufferViews[i].SizeInBytes = 0;
            vertexBufferViews[i].StrideInBytes = 0;
        }
    }

    GetGraphicsCommandList()->IASetVertexBuffers(baseSlot, vertexBufferNum, vertexBufferViews);
}

NRI_INLINE void CommandBufferD3D12::SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType) {
    const BufferD3D12& bufferD3D12 = (BufferD3D12&)buffer;

    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    indexBufferView.BufferLocation = bufferD3D12.GetDeviceAddress() + offset;
    indexBufferView.SizeInBytes = (uint32_t)(bufferD3D12.GetDesc().size - offset);
    indexBufferView.Format = indexType == IndexType::UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

    GetGraphicsCommandList()->IASetIndexBuffer(&indexBufferView);
}

NRI_INLINE void CommandBufferD3D12::SetPipelineLayout(BindPoint bindPoint, const PipelineLayout& pipelineLayout) {
    const PipelineLayoutD3D12& pipelineLayoutD3D12 = (PipelineLayoutD3D12&)pipelineLayout;
    if (bindPoint == BindPoint::GRAPHICS)
        GetGraphicsCommandList()->SetGraphicsRootSignature(pipelineLayoutD3D12);
    else
        GetGraphicsCommandList()->SetComputeRootSignature(pipelineLayoutD3D12);

    m_PipelineLayout = &pipelineLayoutD3D12;
    m_PipelineBindPoint = bindPoint;
}

NRI_INLINE void CommandBufferD3D12::SetPipeline(const Pipeline& pipeline) {
    PipelineD3D12* pipelineD3D12 = (PipelineD3D12*)&pipeline;
    pipelineD3D12->Bind(GetGraphicsCommandList());
}

NRI_INLINE void CommandBufferD3D12::SetDescriptorPool(const DescriptorPool& descriptorPool) {
    ((DescriptorPoolD3D12&)descriptorPool).Bind(GetGraphicsCommandList());
}

NRI_INLINE void CommandBufferD3D12::SetDescriptorSet(const SetDescriptorSetDesc& setDescriptorSetDesc) {
    BindPoint bindPoint = setDescriptorSetDesc.bindPoint == BindPoint::INHERIT ? m_PipelineBindPoint : setDescriptorSetDesc.bindPoint;
    m_PipelineLayout->SetDescriptorSet(GetGraphicsCommandList(), bindPoint, setDescriptorSetDesc);

    m_DescriptorSets[setDescriptorSetDesc.setIndex] = (DescriptorSetD3D12*)setDescriptorSetDesc.descriptorSet;
}

NRI_INLINE void CommandBufferD3D12::SetRootConstants(const SetRootConstantsDesc& setRootConstantsDesc) {
    BindPoint bindPoint = setRootConstantsDesc.bindPoint == BindPoint::INHERIT ? m_PipelineBindPoint : setRootConstantsDesc.bindPoint;
    m_PipelineLayout->SetRootConstants(GetGraphicsCommandList(), bindPoint, setRootConstantsDesc);
}

NRI_INLINE void CommandBufferD3D12::SetRootDescriptor(const SetRootDescriptorDesc& setRootDescriptorDesc) {
    BindPoint bindPoint = setRootDescriptorDesc.bindPoint == BindPoint::INHERIT ? m_PipelineBindPoint : setRootDescriptorDesc.bindPoint;
    m_PipelineLayout->SetRootDescriptor(GetGraphicsCommandList(), bindPoint, setRootDescriptorDesc);
}

NRI_INLINE void CommandBufferD3D12::Draw(const DrawDesc& drawDesc) {
    if (m_PipelineLayout && m_PipelineLayout->IsDrawParametersEmulationEnabled()) {
        struct BaseVertexInstance {
            uint32_t baseVertex;
            uint32_t baseInstance;
        } baseVertexInstance = {drawDesc.baseVertex, drawDesc.baseInstance};

        GetGraphicsCommandList()->SetGraphicsRoot32BitConstants(m_PipelineLayout->GetDrawParametersRootConstantIndex(), 2, &baseVertexInstance, 0);
    }

    if (m_PipelineLayout && m_PipelineLayout->IsDrawIndexEmulationEnabled()) {
        uint32_t drawIndex = 0;
        GetGraphicsCommandList()->SetGraphicsRoot32BitConstants(m_PipelineLayout->GetDrawIndexRootConstantIndex(), 1, &drawIndex, 0);
    }

    GetGraphicsCommandList()->DrawInstanced(drawDesc.vertexNum, drawDesc.instanceNum, drawDesc.baseVertex, drawDesc.baseInstance);
}

NRI_INLINE void CommandBufferD3D12::DrawIndexed(const DrawIndexedDesc& drawIndexedDesc) {
    if (m_PipelineLayout && m_PipelineLayout->IsDrawParametersEmulationEnabled()) {
        struct BaseVertexInstance {
            int32_t baseVertex;
            uint32_t baseInstance;
        } baseVertexInstance = {drawIndexedDesc.baseVertex, drawIndexedDesc.baseInstance};

        GetGraphicsCommandList()->SetGraphicsRoot32BitConstants(m_PipelineLayout->GetDrawParametersRootConstantIndex(), 2, &baseVertexInstance, 0);
    }

    if (m_PipelineLayout && m_PipelineLayout->IsDrawIndexEmulationEnabled()) {
        uint32_t drawIndex = 0;
        GetGraphicsCommandList()->SetGraphicsRoot32BitConstants(m_PipelineLayout->GetDrawIndexRootConstantIndex(), 1, &drawIndex, 0);
    }

    GetGraphicsCommandList()->DrawIndexedInstanced(drawIndexedDesc.indexNum, drawIndexedDesc.instanceNum, drawIndexedDesc.baseIndex, drawIndexedDesc.baseVertex, drawIndexedDesc.baseInstance);
}

NRI_INLINE void CommandBufferD3D12::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ID3D12Resource* pCountBuffer = nullptr;
    if (countBuffer)
        pCountBuffer = *(BufferD3D12*)countBuffer;

    GetGraphicsCommandList()->ExecuteIndirect(m_Device.GetDrawCommandSignature(m_PipelineLayout, stride), drawNum, (BufferD3D12&)buffer, offset, pCountBuffer, countBufferOffset);
}

NRI_INLINE void CommandBufferD3D12::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ID3D12Resource* pCountBuffer = nullptr;
    if (countBuffer)
        pCountBuffer = *(BufferD3D12*)countBuffer;

    GetGraphicsCommandList()->ExecuteIndirect(m_Device.GetDrawIndexedCommandSignature(m_PipelineLayout, stride), drawNum, (BufferD3D12&)buffer, offset, pCountBuffer, countBufferOffset);
}

NRI_INLINE void CommandBufferD3D12::CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    if (size == WHOLE_SIZE)
        size = ((BufferD3D12&)srcBuffer).GetDesc().size;

    GetGraphicsCommandList()->CopyBufferRegion((BufferD3D12&)dstBuffer, dstOffset, (BufferD3D12&)srcBuffer, srcOffset, size);
}

NRI_INLINE void CommandBufferD3D12::CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    const TextureD3D12& dst = (TextureD3D12&)dstTexture;
    const TextureD3D12& src = (TextureD3D12&)srcTexture;

    bool isWholeResource = !dstRegion && !srcRegion;
    if (isWholeResource)
        GetGraphicsCommandList()->CopyResource(dst, src);
    else {
        TextureRegionDesc wholeResource = {};
        if (!srcRegion)
            srcRegion = &wholeResource;
        if (!dstRegion)
            dstRegion = &wholeResource;

        const TextureDesc& dstDesc = dst.GetDesc();
        D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation = {dst, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        dstTextureCopyLocation.SubresourceIndex = GetSubresourceIndex(dstRegion->layerOffset, dstDesc.layerNum, dstRegion->mipOffset, dstDesc.mipNum, dstRegion->planes);

        const TextureDesc& srcDesc = src.GetDesc();
        D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation = {src, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        srcTextureCopyLocation.SubresourceIndex = GetSubresourceIndex(srcRegion->layerOffset, srcDesc.layerNum, srcRegion->mipOffset, srcDesc.mipNum, srcRegion->planes);

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

        GetGraphicsCommandList()->CopyTextureRegion(&dstTextureCopyLocation, dstRegion->x, dstRegion->y, dstRegion->z, &srcTextureCopyLocation, &srcBox);
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

        GetGraphicsCommandList()->CopyBufferRegion(dst, offset, zeroBuffer, 0, blockSize);

        offset += blockSize;
        size -= blockSize;
    }
}

NRI_INLINE void CommandBufferD3D12::ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion, ResolveOp resolveOp) {
    const TextureD3D12& dst = (TextureD3D12&)dstTexture;
    const TextureD3D12& src = (TextureD3D12&)srcTexture;
    const TextureDesc& dstDesc = dst.GetDesc();
    const TextureDesc& srcDesc = src.GetDesc();
    const DxgiFormat& dstFormat = GetDxgiFormat(dstDesc.format);

    bool isWholeResource = !dstRegion && !srcRegion && resolveOp == ResolveOp::AVERAGE; // old API supports only AVERAGE
    if (isWholeResource) {
        for (Dim_t layer = 0; layer < dstDesc.layerNum; layer++) {
            for (Dim_t mip = 0; mip < dstDesc.mipNum; mip++) {
                uint32_t subresource = GetSubresourceIndex(layer, dstDesc.layerNum, mip, dstDesc.mipNum, PlaneBits::ALL);
                GetGraphicsCommandList()->ResolveSubresource(dst, subresource, src, subresource, dstFormat.typed);
            }
        }
    } else {
        TextureRegionDesc wholeResource = {};
        if (!srcRegion)
            srcRegion = &wholeResource;
        if (!dstRegion)
            dstRegion = &wholeResource;

        uint32_t dstSubresource = GetSubresourceIndex(dstRegion->layerOffset, dstDesc.layerNum, dstRegion->mipOffset, dstDesc.mipNum, dstRegion->planes);
        uint32_t srcSubresource = GetSubresourceIndex(srcRegion->layerOffset, srcDesc.layerNum, srcRegion->mipOffset, srcDesc.mipNum, srcRegion->planes);

        Dim_t w = srcRegion->width == WHOLE_SIZE ? src.GetSize(0, srcRegion->mipOffset) : srcRegion->width;
        Dim_t h = srcRegion->height == WHOLE_SIZE ? src.GetSize(1, srcRegion->mipOffset) : srcRegion->height;

        D3D12_RECT srcRect = {
            srcRegion->x,
            srcRegion->y,
            srcRegion->x + w,
            srcRegion->y + h,
        };

        D3D12_RESOLVE_MODE resolveMode = GetResolveOp(resolveOp);

        GetGraphicsCommandList()->ResolveSubresourceRegion(dst, dstSubresource, dstRegion->x, dstRegion->y, src, srcSubresource, &srcRect, dstFormat.typed, resolveMode);
    }
}

NRI_INLINE void CommandBufferD3D12::UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegion, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayout) {
    const TextureD3D12& dst = (TextureD3D12&)dstTexture;
    const TextureDesc& dstDesc = dst.GetDesc();
    auto getPlaneCompatibleFormat = [](Format format, PlaneBits planes) {
        if (format == Format::NV12_UNORM) {
            if (planes & PlaneBits::PLANE_0)
                return DXGI_FORMAT_R8_UNORM;
            if (planes & PlaneBits::PLANE_1)
                return DXGI_FORMAT_R8G8_UNORM;
        } else if (format == Format::P010_UNORM || format == Format::P016_UNORM) {
            if (planes & PlaneBits::PLANE_0)
                return DXGI_FORMAT_R16_UNORM;
            if (planes & PlaneBits::PLANE_1)
                return DXGI_FORMAT_R16G16_UNORM;
        }

        return GetDxgiFormat(format).typeless;
    };
    auto getPlaneDivisor = [](Format format, PlaneBits planes) {
        return (planes & PlaneBits::PLANE_1) && (format == Format::NV12_UNORM || format == Format::P010_UNORM || format == Format::P016_UNORM) ? 2u : 1u;
    };

    D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation = {dst, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    dstTextureCopyLocation.SubresourceIndex = GetSubresourceIndex(dstRegion.layerOffset, dstDesc.layerNum, dstRegion.mipOffset, dstDesc.mipNum, dstRegion.planes);

    const uint32_t planeDivisor = getPlaneDivisor(dstDesc.format, dstRegion.planes);
    const uint32_t size[3] = {
        (dstRegion.width == WHOLE_SIZE ? dst.GetSize(0, dstRegion.mipOffset) : dstRegion.width) / planeDivisor,
        (dstRegion.height == WHOLE_SIZE ? dst.GetSize(1, dstRegion.mipOffset) : dstRegion.height) / planeDivisor,
        dstRegion.depth == WHOLE_SIZE ? dst.GetSize(2, dstRegion.mipOffset) : dstRegion.depth,
    };
    const uint32_t x = dstRegion.x / planeDivisor;
    const uint32_t y = dstRegion.y / planeDivisor;

    D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation = {};
    srcTextureCopyLocation.pResource = (BufferD3D12&)srcBuffer;
    srcTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcTextureCopyLocation.PlacedFootprint.Offset = srcDataLayout.offset;
    srcTextureCopyLocation.PlacedFootprint.Footprint.Format = getPlaneCompatibleFormat(dstDesc.format, dstRegion.planes);
    srcTextureCopyLocation.PlacedFootprint.Footprint.Width = size[0];
    srcTextureCopyLocation.PlacedFootprint.Footprint.Height = size[1];
    srcTextureCopyLocation.PlacedFootprint.Footprint.Depth = size[2];
    srcTextureCopyLocation.PlacedFootprint.Footprint.RowPitch = srcDataLayout.rowPitch;

    GetGraphicsCommandList()->CopyTextureRegion(&dstTextureCopyLocation, x, y, dstRegion.z, &srcTextureCopyLocation, nullptr);
}

NRI_INLINE void CommandBufferD3D12::ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayout, const Texture& srcTexture, const TextureRegionDesc& srcRegion) {
    const TextureD3D12& src = (TextureD3D12&)srcTexture;
    const TextureDesc& srcDesc = src.GetDesc();
    auto getPlaneCompatibleFormat = [](Format format, PlaneBits planes) {
        if (format == Format::NV12_UNORM) {
            if (planes & PlaneBits::PLANE_0)
                return DXGI_FORMAT_R8_UNORM;
            if (planes & PlaneBits::PLANE_1)
                return DXGI_FORMAT_R8G8_UNORM;
        } else if (format == Format::P010_UNORM || format == Format::P016_UNORM) {
            if (planes & PlaneBits::PLANE_0)
                return DXGI_FORMAT_R16_UNORM;
            if (planes & PlaneBits::PLANE_1)
                return DXGI_FORMAT_R16G16_UNORM;
        }

        return GetDxgiFormat(format).typeless;
    };
    auto getPlaneDivisor = [](Format format, PlaneBits planes) {
        return (planes & PlaneBits::PLANE_1) && (format == Format::NV12_UNORM || format == Format::P010_UNORM || format == Format::P016_UNORM) ? 2u : 1u;
    };

    D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation = {src, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    srcTextureCopyLocation.SubresourceIndex = GetSubresourceIndex(srcRegion.layerOffset, srcDesc.layerNum, srcRegion.mipOffset, srcDesc.mipNum, srcRegion.planes);

    const uint32_t planeDivisor = getPlaneDivisor(srcDesc.format, srcRegion.planes);
    uint32_t w = (srcRegion.width == WHOLE_SIZE ? src.GetSize(0, srcRegion.mipOffset) : srcRegion.width) / planeDivisor;
    uint32_t h = (srcRegion.height == WHOLE_SIZE ? src.GetSize(1, srcRegion.mipOffset) : srcRegion.height) / planeDivisor;
    uint32_t d = srcRegion.depth == WHOLE_SIZE ? src.GetSize(2, srcRegion.mipOffset) : srcRegion.depth;
    const uint32_t x = srcRegion.x / planeDivisor;
    const uint32_t y = srcRegion.y / planeDivisor;

    D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation = {};
    dstTextureCopyLocation.pResource = (BufferD3D12&)dstBuffer;
    dstTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstTextureCopyLocation.PlacedFootprint.Offset = dstDataLayout.offset;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Format = getPlaneCompatibleFormat(srcDesc.format, srcRegion.planes);
    dstTextureCopyLocation.PlacedFootprint.Footprint.Width = w;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Height = h;
    dstTextureCopyLocation.PlacedFootprint.Footprint.Depth = d;
    dstTextureCopyLocation.PlacedFootprint.Footprint.RowPitch = dstDataLayout.rowPitch;

    D3D12_BOX srcBox = {
        x,
        y,
        srcRegion.z,
        x + w,
        y + h,
        srcRegion.z + d,
    };

    GetGraphicsCommandList()->CopyTextureRegion(&dstTextureCopyLocation, 0, 0, 0, &srcTextureCopyLocation, &srcBox);
}

NRI_INLINE void CommandBufferD3D12::Dispatch(const DispatchDesc& dispatchDesc) {
    GetGraphicsCommandList()->Dispatch(dispatchDesc.x, dispatchDesc.y, dispatchDesc.z);
}

NRI_INLINE void CommandBufferD3D12::DispatchIndirect(const Buffer& buffer, uint64_t offset) {
    static_assert(sizeof(DispatchDesc) == sizeof(D3D12_DISPATCH_ARGUMENTS));

    GetGraphicsCommandList()->ExecuteIndirect(m_Device.GetDispatchCommandSignature(), 1, (BufferD3D12&)buffer, offset, nullptr, 0);
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

        Scratch<D3D12_RESOURCE_BARRIER> videoBufferResourceBarriers = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_RESOURCE_BARRIER, barrierDesc.bufferNum * 2);
        uint32_t videoBufferResourceBarrierNum = 0;

        // Global
        Scratch<D3D12_GLOBAL_BARRIER> globalBarriers = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_GLOBAL_BARRIER, barrierDesc.globalNum);

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
        uint32_t bufferBarrierNum = 0;

        for (uint32_t i = 0; i < barrierDesc.bufferNum; i++) {
            const BufferBarrierDesc& in = barrierDesc.buffers[i];
            const BufferD3D12& buffer = *(BufferD3D12*)in.buffer;

            if (HasVideoBufferUsage(buffer.GetDesc().usage))
                continue;

            bufferBarrierNum++;
        }

        Scratch<D3D12_BUFFER_BARRIER> bufferBarriers = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_BUFFER_BARRIER, barrierDesc.bufferNum);
        if (bufferBarrierNum) {
            D3D12_BARRIER_GROUP* barrierGroup = &barrierGroups[barriersGroupsNum++];
            barrierGroup->Type = D3D12_BARRIER_TYPE_BUFFER;
            barrierGroup->NumBarriers = bufferBarrierNum;
            barrierGroup->pBufferBarriers = bufferBarriers;

            uint32_t bufferBarrierIndex = 0;

            for (uint32_t i = 0; i < barrierDesc.bufferNum; i++) {
                const BufferBarrierDesc& in = barrierDesc.buffers[i];
                const BufferD3D12& buffer = *(BufferD3D12*)in.buffer;

                if (HasVideoBufferUsage(buffer.GetDesc().usage))
                    continue;

                D3D12_BUFFER_BARRIER& out = bufferBarriers[bufferBarrierIndex++];
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

        // D3D12 validation rejects VIDEO_* access flags in D3D12_BUFFER_BARRIER.
        // Video buffers are created on the legacy state path, so route their barriers through ResourceBarrier as well.
        for (uint32_t i = 0; i < barrierDesc.bufferNum; i++) {
            const BufferBarrierDesc& in = barrierDesc.buffers[i];
            const BufferD3D12& buffer = *(BufferD3D12*)in.buffer;

            if (!HasVideoBufferUsage(buffer.GetDesc().usage))
                continue;

            videoBufferResourceBarriers[videoBufferResourceBarrierNum] = {};
            videoBufferResourceBarrierNum += AddVideoBufferResourceBarriers(
                m_CommandListType,
                (ID3D12Resource*)buffer,
                in.before.access,
                in.after.access,
                &videoBufferResourceBarriers[videoBufferResourceBarrierNum],
                0);
        }

        // Texture
        Scratch<D3D12_TEXTURE_BARRIER> textureBarriers = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_TEXTURE_BARRIER, barrierDesc.textureNum);
        if (barrierDesc.textureNum) {
            D3D12_BARRIER_GROUP* barrierGroup = &barrierGroups[barriersGroupsNum++];
            barrierGroup->Type = D3D12_BARRIER_TYPE_TEXTURE;
            barrierGroup->NumBarriers = barrierDesc.textureNum;
            barrierGroup->pTextureBarriers = textureBarriers;

            for (uint32_t i = 0; i < barrierDesc.textureNum; i++) {
                const TextureBarrierDesc& in = barrierDesc.textures[i];
                const TextureD3D12& texture = *(TextureD3D12*)in.texture;
                const TextureDesc& textureDesc = texture.GetDesc();

                Dim_t mipNum = in.mipNum == REMAINING ? (textureDesc.mipNum - in.mipOffset) : in.mipNum;
                Dim_t layerNum = in.layerNum == REMAINING ? (textureDesc.layerNum - in.layerOffset) : in.layerNum;

                D3D12_TEXTURE_BARRIER& out = textureBarriers[i];
                out = {};
                out.SyncBefore = GetBarrierSyncFlags(in.before.stages, in.before.access);
                out.SyncAfter = GetBarrierSyncFlags(in.after.stages, in.after.access);
                out.AccessBefore = GetBarrierAccessFlags(in.before.access);
                out.AccessAfter = GetBarrierAccessFlags(in.after.access);
                out.LayoutBefore = GetBarrierLayout(in.before.layout, in.before.access);
                out.LayoutAfter = GetBarrierLayout(in.after.layout, in.after.access);
                out.pResource = texture;
                out.Subresources.IndexOrFirstMipLevel = in.mipOffset;
                out.Subresources.NumMipLevels = mipNum;
                out.Subresources.FirstArraySlice = in.layerOffset;
                out.Subresources.NumArraySlices = layerNum;

                const FormatProps& formatProps = GetFormatProps(textureDesc.format);
                if (textureDesc.format == Format::NV12_UNORM || textureDesc.format == Format::P010_UNORM || textureDesc.format == Format::P016_UNORM) {
                    const bool plane0 = in.planes == PlaneBits::ALL || (in.planes & PlaneBits::PLANE_0);
                    const bool plane1 = in.planes == PlaneBits::ALL || (in.planes & PlaneBits::PLANE_1);
                    out.Subresources.FirstPlane = plane0 ? 0 : 1;
                    out.Subresources.NumPlanes = plane0 && plane1 ? 2 : 1;
                } else {
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
                }

                // https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#d3d12_texture_barrier_flags
                out.Flags = in.before.layout == Layout::UNDEFINED ? D3D12_TEXTURE_BARRIER_FLAG_DISCARD : D3D12_TEXTURE_BARRIER_FLAG_NONE; // TODO: verify that it works
            }
        }

        // Submit
        if (videoBufferResourceBarrierNum) {
            if (m_CommandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE)
                GetVideoDecodeCommandList()->ResourceBarrier(videoBufferResourceBarrierNum, videoBufferResourceBarriers);
            else if (m_CommandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE)
                GetVideoEncodeCommandList()->ResourceBarrier(videoBufferResourceBarrierNum, videoBufferResourceBarriers);
            else
                GetGraphicsCommandList()->ResourceBarrier(videoBufferResourceBarrierNum, videoBufferResourceBarriers);
        }

        if (!barriersGroupsNum) {
            return;
        }

        if (m_CommandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE)
            GetVideoDecodeCommandList()->Barrier(barriersGroupsNum, barrierGroups);
        else if (m_CommandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE)
            GetVideoEncodeCommandList()->Barrier(barriersGroupsNum, barrierGroups);
        else
            GetGraphicsCommandList()->Barrier(barriersGroupsNum, barrierGroups);
    } else
#endif
    { // Legacy barriers
        // Count
        uint32_t barrierNum = barrierDesc.bufferNum;

        for (uint32_t i = 0; i < barrierDesc.textureNum; i++) {
            const TextureBarrierDesc& barrier = barrierDesc.textures[i];
            const TextureD3D12& texture = *(TextureD3D12*)barrier.texture;
            const TextureDesc& textureDesc = texture.GetDesc();

            Dim_t mipNum = barrier.mipNum == REMAINING ? (textureDesc.mipNum - barrier.mipOffset) : barrier.mipNum;
            Dim_t layerNum = barrier.layerNum == REMAINING ? (textureDesc.layerNum - barrier.layerOffset) : barrier.layerNum;

            if (layerNum == textureDesc.layerNum && mipNum == textureDesc.mipNum && barrier.planes == PlaneBits::ALL)
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
        Scratch<D3D12_RESOURCE_BARRIER> barriers = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_RESOURCE_BARRIER, barrierNum);
        memset(barriers, 0, sizeof(D3D12_RESOURCE_BARRIER) * barrierNum);

        D3D12_RESOURCE_BARRIER* ptr = barriers;
        D3D12_COMMAND_LIST_TYPE commandListType = m_CommandListType;

        for (uint32_t i = 0; i < barrierDesc.bufferNum; i++) {
            const BufferBarrierDesc& barrier = barrierDesc.buffers[i];
            ptr += AddResourceBarrier(commandListType, *((BufferD3D12*)barrier.buffer), barrier.before.access, barrier.after.access, *ptr, 0);
        }

        for (uint32_t i = 0; i < barrierDesc.textureNum; i++) {
            const TextureBarrierDesc& barrier = barrierDesc.textures[i];
            const TextureD3D12& texture = *(TextureD3D12*)barrier.texture;
            const TextureDesc& textureDesc = texture.GetDesc();

            Dim_t mipNum = barrier.mipNum == REMAINING ? (textureDesc.mipNum - barrier.mipOffset) : barrier.mipNum;
            Dim_t layerNum = barrier.layerNum == REMAINING ? (textureDesc.layerNum - barrier.layerOffset) : barrier.layerNum;

            if (layerNum == textureDesc.layerNum && mipNum == textureDesc.mipNum && barrier.planes == PlaneBits::ALL)
                ptr += AddResourceBarrier(commandListType, texture, barrier.before.access, barrier.after.access, *ptr, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
            else {
                for (Dim_t layer = 0; layer < layerNum; layer++) {
                    for (Dim_t mip = 0; mip < mipNum; mip++) {
                        uint32_t subresource = GetSubresourceIndex(barrier.layerOffset + layer, textureDesc.layerNum, barrier.mipOffset + mip, textureDesc.mipNum, barrier.planes);
                        ptr += AddResourceBarrier(commandListType, texture, barrier.before.access, barrier.after.access, *ptr, subresource);
                    }
                }
            }
        }

        if (isGlobalUavBarrierNeeded) {
            ptr->Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            ptr->UAV.pResource = nullptr;
            ptr++;
        }

        barrierNum = (uint32_t)(ptr - (D3D12_RESOURCE_BARRIER*)barriers);
        if (!barrierNum)
            return;

        // Submit
        if (m_CommandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE)
            GetVideoDecodeCommandList()->ResourceBarrier(barrierNum, barriers);
        else if (m_CommandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE)
            GetVideoEncodeCommandList()->ResourceBarrier(barrierNum, barriers);
        else
            GetGraphicsCommandList()->ResourceBarrier(barrierNum, barriers);
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

            GetGraphicsCommandList()->Barrier(1, &barrierGroup);
        } else
#endif
        {
            D3D12_RESOURCE_BARRIER resourceBarrier = {};
            resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            resourceBarrier.Transition.pResource = queryPoolD3D12.GetBufferForAccelerationStructuresSizes();
            resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

            GetGraphicsCommandList()->ResourceBarrier(1, &resourceBarrier);
        }
    }
}

NRI_INLINE void CommandBufferD3D12::BeginQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    GetGraphicsCommandList()->BeginQuery(queryPoolD3D12, queryPoolD3D12.GetType(), offset);
}

NRI_INLINE void CommandBufferD3D12::EndQuery(QueryPool& queryPool, uint32_t offset) {
    QueryPoolD3D12& queryPoolD3D12 = (QueryPoolD3D12&)queryPool;
    GetGraphicsCommandList()->EndQuery(queryPoolD3D12, queryPoolD3D12.GetType(), offset);
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

            GetGraphicsCommandList()->Barrier(1, &barrierGroup);
        } else
#endif
        {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = bufferSrc;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

            GetGraphicsCommandList()->ResourceBarrier(1, &barrier);
        }

        GetGraphicsCommandList()->CopyBufferRegion(bufferD3D12, alignedBufferOffset, bufferSrc, srcOffset, size);
    } else
        GetGraphicsCommandList()->ResolveQueryData(queryPoolD3D12, queryPoolD3D12.GetType(), offset, num, bufferD3D12, alignedBufferOffset);
}

NRI_INLINE void CommandBufferD3D12::BeginAnnotation(const char* name, uint32_t bgra) {
    if (m_Device.HasPix())
        m_Device.GetPix().BeginEventOnCommandList(GetGraphicsCommandList(), bgra, name);
    else
        PIXBeginEvent(GetGraphicsCommandList(), bgra, name);
}

NRI_INLINE void CommandBufferD3D12::EndAnnotation() {
    if (m_Device.HasPix())
        m_Device.GetPix().EndEventOnCommandList(GetGraphicsCommandList());
    else
        PIXEndEvent(GetGraphicsCommandList());
}

NRI_INLINE void CommandBufferD3D12::Annotation(const char* name, uint32_t bgra) {
    if (m_Device.HasPix())
        m_Device.GetPix().SetMarkerOnCommandList(GetGraphicsCommandList(), bgra, name);
    else
        PIXSetMarker(GetGraphicsCommandList(), bgra, name);
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

        GetGraphicsCommandList()->BuildRaytracingAccelerationStructure(&out, 0, nullptr);
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

    Scratch<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_RAYTRACING_GEOMETRY_DESC, geometryMaxNum);
    Scratch<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC> trianglesDescs = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC, micromapMaxNum);
    Scratch<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC> ommDescs = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC, micromapMaxNum);

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

        GetGraphicsCommandList()->BuildRaytracingAccelerationStructure(&out, 0, nullptr);
    }
}

NRI_INLINE void CommandBufferD3D12::BuildMicromaps(const BuildMicromapDesc* buildMicromapDescs, uint32_t buildMicromapDescNum) {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    static_assert(sizeof(MicromapTriangle) == sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_DESC), "Type mismatch");

    uint32_t usageMaxNum = 0;
    for (uint32_t i = 0; i < buildMicromapDescNum; i++)
        usageMaxNum = std::max(usageMaxNum, ((MicromapD3D12*)buildMicromapDescs[i].dst)->GetUsageNum());

    Scratch<D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY> usages = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY, usageMaxNum);

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

        GetGraphicsCommandList()->BuildRaytracingAccelerationStructure(&out, 0, nullptr);
    }
#else
    MaybeUnused(buildMicromapDescs, buildMicromapDescNum);
#endif
}

NRI_INLINE void CommandBufferD3D12::CopyAccelerationStructure(AccelerationStructure& dst, const AccelerationStructure& src, CopyMode copyMode) {
    GetGraphicsCommandList()->CopyRaytracingAccelerationStructure(((AccelerationStructureD3D12&)dst).GetHandle(), ((AccelerationStructureD3D12&)src).GetHandle(), GetCopyMode(copyMode));
}

NRI_INLINE void CommandBufferD3D12::CopyMicromap(Micromap& dst, const Micromap& src, CopyMode copyMode) {
    GetGraphicsCommandList()->CopyRaytracingAccelerationStructure(((MicromapD3D12&)dst).GetHandle(), ((MicromapD3D12&)src).GetHandle(), GetCopyMode(copyMode));
}

NRI_INLINE void CommandBufferD3D12::WriteAccelerationStructuresSizes(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    Scratch<D3D12_GPU_VIRTUAL_ADDRESS> virtualAddresses = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_GPU_VIRTUAL_ADDRESS, accelerationStructureNum);
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

    GetGraphicsCommandList()->EmitRaytracingAccelerationStructurePostbuildInfo(&postbuildInfo, accelerationStructureNum, virtualAddresses);
}

NRI_INLINE void CommandBufferD3D12::WriteMicromapsSizes(const Micromap* const* micromaps, uint32_t micromapNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    Scratch<D3D12_GPU_VIRTUAL_ADDRESS> virtualAddresses = NRI_ALLOCATE_SCRATCH(m_Device, D3D12_GPU_VIRTUAL_ADDRESS, micromapNum);
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

    GetGraphicsCommandList()->EmitRaytracingAccelerationStructurePostbuildInfo(&postbuildInfo, micromapNum, virtualAddresses);
}

NRI_INLINE void CommandBufferD3D12::DispatchRays(const DispatchRaysDesc& dispatchRaysDesc) {
    D3D12_DISPATCH_RAYS_DESC desc = {};

    desc.RayGenerationShaderRecord.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.raygenShader.buffer).GetDeviceAddress() + dispatchRaysDesc.raygenShader.offset;
    desc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    if (dispatchRaysDesc.missShaders.buffer) {
        desc.MissShaderTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.missShaders.buffer).GetDeviceAddress() + dispatchRaysDesc.missShaders.offset;
        desc.MissShaderTable.SizeInBytes = dispatchRaysDesc.missShaders.size;
        desc.MissShaderTable.StrideInBytes = dispatchRaysDesc.missShaders.stride;
    }

    if (dispatchRaysDesc.hitShaderGroups.buffer) {
        desc.HitGroupTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.hitShaderGroups.buffer).GetDeviceAddress() + dispatchRaysDesc.hitShaderGroups.offset;
        desc.HitGroupTable.SizeInBytes = dispatchRaysDesc.hitShaderGroups.size;
        desc.HitGroupTable.StrideInBytes = dispatchRaysDesc.hitShaderGroups.stride;
    }

    if (dispatchRaysDesc.callableShaders.buffer) {
        desc.CallableShaderTable.StartAddress = (*(BufferD3D12*)dispatchRaysDesc.callableShaders.buffer).GetDeviceAddress() + dispatchRaysDesc.callableShaders.offset;
        desc.CallableShaderTable.SizeInBytes = dispatchRaysDesc.callableShaders.size;
        desc.CallableShaderTable.StrideInBytes = dispatchRaysDesc.callableShaders.stride;
    }

    desc.Width = dispatchRaysDesc.x;
    desc.Height = dispatchRaysDesc.y;
    desc.Depth = dispatchRaysDesc.z;

    GetGraphicsCommandList()->DispatchRays(&desc);
}

NRI_INLINE void CommandBufferD3D12::DispatchRaysIndirect(const Buffer& buffer, uint64_t offset) {
    static_assert(sizeof(DispatchRaysIndirectDesc) == sizeof(D3D12_DISPATCH_RAYS_DESC));

    GetGraphicsCommandList()->ExecuteIndirect(m_Device.GetDispatchRaysCommandSignature(), 1, (BufferD3D12&)buffer, offset, nullptr, 0);
}

NRI_INLINE void CommandBufferD3D12::DrawMeshTasks(const DrawMeshTasksDesc& drawMeshTasksDesc) {
    GetGraphicsCommandList()->DispatchMesh(drawMeshTasksDesc.x, drawMeshTasksDesc.y, drawMeshTasksDesc.z);
}

NRI_INLINE void CommandBufferD3D12::DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    static_assert(sizeof(DrawMeshTasksDesc) == sizeof(D3D12_DISPATCH_MESH_ARGUMENTS));

    ID3D12Resource* pCountBuffer = nullptr;
    if (countBuffer)
        pCountBuffer = *(BufferD3D12*)countBuffer;

    GetGraphicsCommandList()->ExecuteIndirect(m_Device.GetDrawMeshCommandSignature(stride), drawNum, (BufferD3D12&)buffer, offset, pCountBuffer, countBufferOffset);
}
