// © 2026 NVIDIA Corporation

// The native command buffer points to its owner (not retained)
static char g_CommandBufferOwnerKey;

static inline CommandBufferMetal& GetCommandBuffer(NS::Object* commandBuffer) {
    return *(CommandBufferMetal*)objc_getAssociatedObject((id)commandBuffer, &g_CommandBufferOwnerKey);
}

#if NRI_ENABLE_METAL_SHADER_CONVERTER
// Object stage threadgroup memory of Converter's tessellation emulation (as in "metal_irconverter_runtime.h" draw helpers)
constexpr NS::UInteger TESSELLATION_OBJECT_THREADGROUP_MEMORY_SIZE = 15360;

// Converter's ray dispatch kernel ("RaygenIndirection") is dispatched with 8x8x1 threadgroups and an exact grid
constexpr uint32_t RAY_DISPATCH_GROUP_SIZE = 8;
#endif

static inline MTL::Stages GetBarrierStages(StageBits stages) {
    if (stages == StageBits::ALL)
        return MTL::StageAll;

    if (stages == StageBits::NONE)
        return 0;

    MTL::Stages result = 0;

    if (stages & (StageBits::INDEX_INPUT | StageBits::VERTEX_SHADER | StageBits::TESSELLATION_SHADERS | StageBits::GEOMETRY_SHADER))
        result |= MTL::StageVertex | MTL::StageObject | MTL::StageMesh;

    if (stages & StageBits::TASK_SHADER)
        result |= MTL::StageObject;

    if (stages & StageBits::MESH_SHADER)
        result |= MTL::StageMesh;

    if (stages & (StageBits::FRAGMENT_SHADER | StageBits::DEPTH_STENCIL_ATTACHMENT | StageBits::COLOR_ATTACHMENT | StageBits::SHADING_RATE_ATTACHMENT | StageBits::RESOLVE))
        result |= MTL::StageFragment;

    if (stages & (StageBits::COMPUTE_SHADER | StageBits::RAY_TRACING_SHADERS))
        result |= MTL::StageDispatch;

    // "CmdClearStorage" dispatches, "CmdZeroBuffer" uses "fillBuffer"
    if (stages & StageBits::CLEAR_STORAGE)
        result |= MTL::StageDispatch | MTL::StageBlit;

    // Instance conversion and TLAS header writes are dispatches
    if (stages & StageBits::ACCELERATION_STRUCTURE)
        result |= MTL::StageAccelerationStructure | MTL::StageDispatch;

    if (stages & (StageBits::COPY | StageBits::RESOLVE))
        result |= MTL::StageBlit;

    if (stages & StageBits::INDIRECT)
        result |= MTL::StageVertex | MTL::StageObject | MTL::StageMesh | MTL::StageDispatch;

    return result;
}

static inline bool HasWriteAccess(AccessBits access) {
    constexpr uint32_t writeAccess = (uint32_t)AccessBits::SCRATCH_BUFFER | (uint32_t)AccessBits::COLOR_ATTACHMENT_WRITE | (uint32_t)AccessBits::DEPTH_STENCIL_ATTACHMENT_WRITE
        | (uint32_t)AccessBits::ACCELERATION_STRUCTURE_WRITE | (uint32_t)AccessBits::MICROMAP_WRITE | (uint32_t)AccessBits::SHADER_RESOURCE_STORAGE
        | (uint32_t)AccessBits::COPY_DESTINATION | (uint32_t)AccessBits::RESOLVE_DESTINATION | (uint32_t)AccessBits::CLEAR_STORAGE | (uint32_t)AccessBits::HOST_WRITE
        | (uint32_t)AccessBits::VIDEO_DECODE_WRITE | (uint32_t)AccessBits::VIDEO_ENCODE_WRITE;

    // "NONE" is "COMMON" (any access)
    return access == AccessBits::NONE || ((uint32_t)access & writeAccess) != 0;
}

static inline MTL::ScissorRect GetScissorRect(const nri::Rect& rect) {
    const int32_t x = std::max<int32_t>(0, rect.x);
    const int32_t y = std::max<int32_t>(0, rect.y);

    return {(NS::UInteger)x, (NS::UInteger)y, (NS::UInteger)std::max<int32_t>(0, int32_t(rect.x) + rect.width - x), (NS::UInteger)std::max<int32_t>(0, int32_t(rect.y) + rect.height - y)};
}

static inline uint32_t GetAttachmentLayerNum(const DescriptorMetal& descriptor) {
    const TextureViewDesc& viewDesc = descriptor.GetTextureViewDesc();

    return viewDesc.layerNum == REMAINING ? uint32_t(descriptor.GetTexture()->arrayLength()) - viewDesc.layerOffset : uint32_t(viewDesc.layerNum);
}

static inline void SetAttachmentResolve(MTL::RenderPassAttachmentDescriptor* attachment, const AttachmentDesc& attachmentDesc) {
    const DescriptorMetal& resolve = *(DescriptorMetal*)attachmentDesc.resolveDst;
    const TextureViewDesc& resolveViewDesc = resolve.GetTextureViewDesc();

    attachment->setResolveTexture(resolve.GetTexture());
    attachment->setResolveLevel(resolveViewDesc.mipOffset);
    attachment->setResolveSlice(resolveViewDesc.layerOffset);
    attachment->setStoreAction(attachmentDesc.storeOp == StoreOp::STORE ? MTL::StoreActionStoreAndMultisampleResolve : MTL::StoreActionMultisampleResolve);
}

static inline MTL::Size GetRegionSize(const TextureMetal& t, const TextureRegionDesc& r) {
    const TextureDesc& d = t.GetDesc();

    return MTL::Size(r.width ? r.width : std::max(1u, uint32_t(d.width) >> r.mipOffset), r.height ? r.height : std::max(1u, uint32_t(d.height) >> r.mipOffset), r.depth ? r.depth : std::max(1u, uint32_t(d.depth) >> r.mipOffset));
}

static inline MTL::BlitOption GetTextureCopyOptions(const TextureMetal& texture, PlaneBits planes) {
    const FormatProps& props = GetFormatProps(texture.GetDesc().format);

    if (props.isDepth && props.isStencil) {
        if (planes == PlaneBits::DEPTH)
            return MTL::BlitOptionDepthFromDepthStencil;

        if (planes == PlaneBits::STENCIL)
            return MTL::BlitOptionStencilFromDepthStencil;
    }

    return MTL::BlitOptionNone;
}

static inline InternalKernelMetal GetClearStorageKernel(MTL::Texture* texture, Format format) {
    if (!texture)
        return InternalKernelMetal::CLEAR_STORAGE_BUFFER;

    // Matches "nri_clear_storage_<dimension>_<type>" in "InternalMetal.metal"
    uint32_t dimension = 0;

    switch (texture->textureType()) {
        case MTL::TextureType1D:
            dimension = 1;
            break;
        case MTL::TextureType1DArray:
            dimension = 2;
            break;
        case MTL::TextureType2D:
            dimension = 3;
            break;
        case MTL::TextureType2DArray:
            dimension = 4;
            break;
        case MTL::TextureType3D:
            dimension = 5;
            break;
        default:
            break;
    }

    const uint32_t type = (uint32_t)GetColorType(format);

    return (InternalKernelMetal)((uint32_t)InternalKernelMetal::CLEAR_STORAGE_TEXTURE + dimension * 3 + type);
}

CommandBufferMetal::~CommandBufferMetal() {
    for (Annotation& annotation : m_Annotations)
        annotation.name->release();

    // Destroyed while recording
    if (m_RenderEncoder) {
        m_RenderEncoder->endEncoding();
        m_RenderEncoder->release();
    }

    EndCompute();

    if (m_RenderPass)
        m_RenderPass->release();

    if (m_CounterFence)
        m_CounterFence->release();

    if (m_Arguments)
        m_Arguments->release();

    if (m_InternalArguments)
        m_InternalArguments->release();

    if (m_CommandBuffer)
        m_CommandBuffer->release();
}

Result CommandBufferMetal::Create(const CommandAllocator& allocator) {
    m_Allocator = (CommandAllocatorMetal*)&allocator;
    m_CommandBuffer = m_Device.GetNativeObject()->newCommandBuffer();

    if (m_CommandBuffer)
        objc_setAssociatedObject((id)m_CommandBuffer, &g_CommandBufferOwnerKey, (id)this, OBJC_ASSOCIATION_ASSIGN);

    MTL4::ArgumentTableDescriptor* desc = MTL4::ArgumentTableDescriptor::alloc()->init();
    desc->setMaxBufferBindCount(31);
    desc->setMaxTextureBindCount(1);
    desc->setInitializeBindings(true);
    desc->setSupportAttributeStrides(true);
    m_Arguments = m_Device.GetNativeObject()->newArgumentTable(desc, nullptr);
    m_InternalArguments = m_Device.GetNativeObject()->newArgumentTable(desc, nullptr);
    desc->release();

    return (m_CommandBuffer && m_Arguments && m_InternalArguments) ? Result::SUCCESS : Result::OUT_OF_MEMORY;
}

NRI_INLINE Result CommandBufferMetal::Begin(const DescriptorPool* pool) {
    NRI_CHECK(!m_RenderEncoder && !m_ComputeEncoder && !m_RenderPass, "The previous recording has not been ended");

    m_Result = Result::SUCCESS;
    m_DrawRootAddress = 0;
    m_Pipeline = nullptr;
    m_DescriptorPool = (DescriptorPoolMetal*)pool;
    m_AreHeapsDirty = true;
    m_Graphics.layout = nullptr;
    m_Graphics.rootAddress = 0;
    m_Compute.layout = nullptr;
    m_Compute.rootAddress = 0;
    m_BindPoint = BindPoint::GRAPHICS;
    m_ViewportNum = m_ScissorNum = 0;
    m_HasDepthBias = false;
    m_DepthMin = 0.0f;
    m_DepthMax = 1.0f;
    m_SamplePositionNum = 0;
    m_AttachmentResolveNum = 0;
    m_DrawArgumentsSize = 0;
    m_DrawArgumentsAddress = 0;
    m_PendingBefore = m_PendingAfter = 0;
    m_PendingVisibility = MTL4::VisibilityOptionNone;
    m_ResumeStages = 0;
    m_VisibilityBuffer = nullptr;
    m_IsCounterFenceWaitPending = false;
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    memset(m_EmulationVertexBuffers, 0, sizeof(m_EmulationVertexBuffers));
#endif

    // The device residency set is attached to all queues
    m_CommandBuffer->beginCommandBuffer(m_Allocator->GetNativeObject());

    return Result::SUCCESS;
}

void CommandBufferMetal::FlushBarriers(MTL4::CommandEncoder* encoder, MTL::Stages stages) {
    // "resolveCounterHeap" is not encoded into an encoder, the next encoder waits for its fence
    if (m_IsCounterFenceWaitPending) {
        encoder->waitForFence(m_CounterFence, stages);
        m_IsCounterFenceWaitPending = false;
    }

    if (!m_PendingBefore)
        return;

    // Previous encoders => this encoder, and previous encoders => next encoders
    encoder->barrierAfterQueueStages(m_PendingBefore, m_PendingAfter, m_PendingVisibility);
    encoder->barrierAfterStages(m_PendingBefore, m_PendingAfter, m_PendingVisibility);

    m_PendingBefore = m_PendingAfter = 0;
    m_PendingVisibility = MTL4::VisibilityOptionNone;
}

void CommandBufferMetal::EndCompute() {
    if (m_ComputeEncoder) {
        m_ComputeEncoder->endEncoding();
        m_ComputeEncoder->release();
        m_ComputeEncoder = nullptr;
    }
}

MTL4::ComputeCommandEncoder* CommandBufferMetal::BeginCompute() {
    if (!m_ComputeEncoder) {
        NRI_CHECK(!m_RenderEncoder, "Compute, copy, clear storage and query commands can't be recorded inside rendering");

        // The encoder is autoreleased and not retained by the command buffer
        AutoreleasePoolMetal autoreleasePool;
        m_ComputeEncoder = m_CommandBuffer->computeCommandEncoder();
        m_ComputeEncoder->retain();
        m_ComputeTable = nullptr;
        m_ComputePipeline = nullptr;

        FlushBarriers(m_ComputeEncoder, MTL::StageDispatch | MTL::StageBlit | MTL::StageAccelerationStructure);
    }

    return m_ComputeEncoder;
}

void CommandBufferMetal::SetComputeState(MTL4::ArgumentTable* arguments, MTL::ComputePipelineState* pipeline) {
    MTL4::ComputeCommandEncoder* encoder = BeginCompute();

    if (m_ComputeTable != arguments) {
        encoder->setArgumentTable(arguments);
        m_ComputeTable = arguments;
    }

    if (pipeline && m_ComputePipeline != pipeline) {
        encoder->setComputePipelineState(pipeline);
        m_ComputePipeline = pipeline;
    }
}

void CommandBufferMetal::SetArgumentAddress(uint32_t slot, MTL::GPUAddress address) {
    if (slot < GetCountOf(m_ArgumentAddresses)) {
        if (m_ArgumentAddresses[slot] == address)
            return;

        m_ArgumentAddresses[slot] = address;
    }

    m_Arguments->setAddress(address, slot);
}

MTL4::RenderCommandEncoder* CommandBufferMetal::BeginRenderEncoder(MTL4::RenderPassDescriptor* pass) {
    EndCompute();

    // The encoder is autoreleased and not retained by the command buffer
    AutoreleasePoolMetal autoreleasePool;
    MTL4::RenderCommandEncoder* encoder = m_CommandBuffer->renderCommandEncoder(pass);
    encoder->retain();

    FlushBarriers(encoder, MTL::StageVertex | MTL::StageObject | MTL::StageMesh | MTL::StageFragment);

    return encoder;
}

MTL4::RenderCommandEncoder* CommandBufferMetal::GetRenderEncoder() {
    if (m_RenderEncoder)
        return m_RenderEncoder;

    NRI_CHECK(m_RenderPass, "Rendering commands must be recorded inside rendering");

    m_RenderEncoder = BeginRenderEncoder(m_RenderPass);

    for (Annotation& annotation : m_Annotations) {
        if (annotation.location == AnnotationLocation::SUSPENDED) {
            m_RenderEncoder->pushDebugGroup(annotation.name);
            annotation.location = AnnotationLocation::RENDER_ENCODER;
        }
    }

    // Attachments stored by previous render passes or by the previous render encoder of this pass
    if (m_ResumeStages & MTL::StageFragment)
        m_RenderEncoder->barrierAfterQueueStages(MTL::StageFragment, MTL::StageFragment, MTL4::VisibilityOptionDevice);

    // Draw arguments and roots produced by internal kernels
    if (m_ResumeStages & MTL::StageDispatch)
        m_RenderEncoder->barrierAfterQueueStages(MTL::StageDispatch, MTL::StageVertex | MTL::StageObject | MTL::StageMesh | MTL::StageFragment, MTL4::VisibilityOptionDevice);

    m_ResumeStages = 0;
    m_RenderPipelineDirty = true;
    m_IsRenderTableBound = false;

    ApplyRasterState();

    if (m_VisibilityMode != MTL::VisibilityResultModeDisabled)
        m_RenderEncoder->setVisibilityResultMode(m_VisibilityMode, m_VisibilityOffset);

    return m_RenderEncoder;
}

void CommandBufferMetal::EndRenderEncoder(bool isSuspended) {
    // Debug groups can't span encoders
    for (size_t i = m_Annotations.size(); i > 0; i--) {
        Annotation& annotation = m_Annotations[i - 1];

        if (annotation.location == AnnotationLocation::RENDER_ENCODER) {
            m_RenderEncoder->popDebugGroup();
            annotation.location = isSuspended ? AnnotationLocation::SUSPENDED : AnnotationLocation::CLOSED;
        }
    }

    m_RenderEncoder->endEncoding();
    m_RenderEncoder->release();
    m_RenderEncoder = nullptr;
    m_IsRenderTableBound = false;
}

NRI_INLINE Result CommandBufferMetal::End() {
    NRI_CHECK(!m_RenderPass, "'CmdEndRendering' is missing");

    // A trailing barrier orders work in the next command buffers
    if (m_PendingBefore)
        BeginCompute();

    EndCompute();

    // Unbalanced annotations
    for (size_t i = m_Annotations.size(); i > 0; i--) {
        Annotation& annotation = m_Annotations[i - 1];

        if (annotation.location == AnnotationLocation::COMMAND_BUFFER)
            m_CommandBuffer->popDebugGroup();

        annotation.name->release();
    }
    m_Annotations.clear();

    m_CommandBuffer->endCommandBuffer();

    return m_Result;
}

void CommandBufferMetal::RecordFailure(Result result) {
    if (m_Result == Result::SUCCESS)
        m_Result = result;
}

MTL::GPUAddress CommandBufferMetal::Upload(const void* data, uint64_t size) {
    const MTL::GPUAddress address = m_Allocator->Upload(data, size);

    if (!address && size)
        RecordFailure(Result::OUT_OF_MEMORY);

    return address;
}

NRI_INLINE void CommandBufferMetal::CmdSetDescriptorPool(const DescriptorPool& pool) {
    m_DescriptorPool = (DescriptorPoolMetal*)&pool;
    m_AreHeapsDirty = true;
}

NRI_INLINE void CommandBufferMetal::CmdSetPipelineLayout(BindPoint bindPoint, const PipelineLayout& layout) {
    m_BindPoint = bindPoint;
    State& state = bindPoint == BindPoint::GRAPHICS ? m_Graphics : m_Compute;
    state.layout = (const PipelineLayoutMetal*)&layout;
    state.root.resize(state.layout->GetRootDataSize());
    state.layout->InitRootData(state.root.data());
    state.rootAddress = 0;
}

NRI_INLINE void CommandBufferMetal::CmdSetDescriptorSet(const SetDescriptorSetDesc& desc) {
    State& s = GetState(desc.bindPoint);

    s.layout->WriteSetPointers(s.root.data(), desc.setIndex, *(DescriptorSetMetal*)desc.descriptorSet);
    s.rootAddress = 0;
}

NRI_INLINE void CommandBufferMetal::CmdSetRootConstants(const SetRootConstantsDesc& desc) {
    State& s = GetState(desc.bindPoint);

    memcpy(s.root.data() + s.layout->GetRootConstantOffset(desc.rootConstantIndex) + desc.offset, desc.data, desc.size);
    s.rootAddress = 0;
}

NRI_INLINE void CommandBufferMetal::CmdSetRootDescriptor(const SetRootDescriptorDesc& desc) {
    State& s = GetState(desc.bindPoint);
    const DescriptorMetal& descriptor = *(DescriptorMetal*)desc.descriptor;

    const uint64_t address = descriptor.GetBuffer()->gpuAddress() + descriptor.GetBufferOffset() + desc.offset;
    memcpy(s.root.data() + s.layout->GetRootDescriptorOffset(desc.rootDescriptorIndex), &address, sizeof(address));
    s.rootAddress = 0;
}

CommandBufferMetal::State& CommandBufferMetal::GetState(BindPoint bindPoint) {
    if (bindPoint == BindPoint::INHERIT)
        bindPoint = m_BindPoint;

    return bindPoint == BindPoint::GRAPHICS ? m_Graphics : m_Compute;
}

NRI_INLINE void CommandBufferMetal::CmdSetPipeline(const Pipeline& pipeline) {
    const bool hadSampleLocations = m_Pipeline && m_Pipeline->HasSampleLocations();
    m_Pipeline = (const PipelineMetal*)&pipeline;
    m_RenderPipelineDirty = true;

    // Like in VK, binding a pipeline with depth bias restores its static depth bias
    m_HasDepthBias = false;

    if (m_RenderPass && hadSampleLocations != m_Pipeline->HasSampleLocations() && SuspendRendering())
        m_RenderPass->setSamplePositions(m_SamplePositions, m_Pipeline->HasSampleLocations() ? m_SamplePositionNum : 0);
}

NRI_INLINE void CommandBufferMetal::CmdBarrier(const BarrierDesc& desc) {
    MTL::Stages before = 0;
    MTL::Stages after = 0;
    bool hasWrites = false;

    for (uint32_t i = 0; i < desc.globalNum; i++) {
        const GlobalBarrierDesc& barrier = desc.globals[i];
        before |= GetBarrierStages(barrier.before.stages);
        after |= GetBarrierStages(barrier.after.stages);
        hasWrites = hasWrites || HasWriteAccess(barrier.before.access);
    }

    for (uint32_t i = 0; i < desc.bufferNum; i++) {
        const BufferBarrierDesc& barrier = desc.buffers[i];
        before |= GetBarrierStages(barrier.before.stages);
        after |= GetBarrierStages(barrier.after.stages);
        hasWrites = hasWrites || HasWriteAccess(barrier.before.access);
    }

    for (uint32_t i = 0; i < desc.textureNum; i++) {
        const TextureBarrierDesc& barrier = desc.textures[i];
        before |= GetBarrierStages(barrier.before.stages);
        after |= GetBarrierStages(barrier.after.stages);
        hasWrites = hasWrites || HasWriteAccess(barrier.before.access);
    }

    if (!before || !after)
        return;

    // Read-to-write (WAR) dependencies need only an execution barrier
    const MTL4::VisibilityOptions visibility = hasWrites ? MTL4::VisibilityOptionDevice : MTL4::VisibilityOptionNone;

    // Without an open encoder (also inside rendering before the first draw) the barrier is emitted at the beginning of the next encoder,
    // saving an encoder which would contain only barriers
    if (!m_RenderEncoder && !m_ComputeEncoder) {
        m_PendingBefore |= before;
        m_PendingAfter |= after;
        m_PendingVisibility |= visibility;

        return;
    }

    if (m_RenderEncoder) {
        const MTL::Stages preRasterStages = MTL::StageVertex | MTL::StageObject | MTL::StageMesh;
        const MTL::Stages renderStages = preRasterStages | MTL::StageFragment;

        // Work in this pass: only pre-rasterization stages can be waited for (TBDR). Framebuffer-local fragment to fragment dependencies
        // (including input attachments) are implicitly ordered per pixel in primitive order
        NRI_CHECK(before == MTL::StageAll || after == MTL::StageAll || !(before & MTL::StageFragment) || !(after & preRasterStages), "Pre-rasterization stages can't wait for fragment work inside rendering");

        if ((before & preRasterStages) && (after & renderStages))
            m_RenderEncoder->barrierAfterEncoderStages(before & preRasterStages, after & renderStages, visibility);

        // Work from previous encoders, including fragment work of previous render encoders
        if (after & renderStages)
            m_RenderEncoder->barrierAfterQueueStages(before, after & renderStages, visibility);

        return;
    }

    // Separate barriers for "previous encoders => this encoder", "this encoder => this encoder" and "this and previous encoders => next encoders"
    m_ComputeEncoder->barrierAfterQueueStages(before, after, visibility);

    const MTL::Stages computeStages = MTL::StageDispatch | MTL::StageBlit | MTL::StageAccelerationStructure;

    if ((before & computeStages) && (after & computeStages))
        m_ComputeEncoder->barrierAfterEncoderStages(before & computeStages, after & computeStages, visibility);

    m_ComputeEncoder->barrierAfterStages(before, after, visibility);
}

NRI_INLINE void CommandBufferMetal::CmdSetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType type) {
    const BufferMetal& b = (const BufferMetal&)buffer;
    m_IndexAddress = b.GetGpuAddress() + offset;
    m_IndexLength = b.GetDesc().size - offset;
    m_IndexType = type == IndexType::UINT16 ? MTL::IndexTypeUInt16 : MTL::IndexTypeUInt32;
}

NRI_INLINE void CommandBufferMetal::CmdSetVertexBuffers(uint32_t base, const VertexBufferDesc* descs, uint32_t num) {
    for (uint32_t i = 0; i < num; i++) {
        const BufferMetal& buffer = *(BufferMetal*)descs[i].buffer;
        m_Arguments->setAddress(buffer.GetGpuAddress() + descs[i].offset, descs[i].stride, 6 + base + i);
#if NRI_ENABLE_METAL_SHADER_CONVERTER
        m_EmulationVertexBuffers[base + i] = {buffer.GetGpuAddress() + descs[i].offset, (uint32_t)std::min<uint64_t>(buffer.GetDesc().size - descs[i].offset, UINT32_MAX), descs[i].stride};
#endif
    }
}

NRI_INLINE void CommandBufferMetal::CmdSetViewports(const Viewport* v, uint32_t n) {
    m_ViewportNum = n;

    for (uint32_t i = 0; i < n; i++)
        m_Viewports[i] = {v[i].x, v[i].y, v[i].width, v[i].height, v[i].depthMin, v[i].depthMax};

    if (m_RenderEncoder)
        m_RenderEncoder->setViewports(m_Viewports, n);
}

NRI_INLINE void CommandBufferMetal::CmdSetScissors(const Rect* r, uint32_t n) {
    m_ScissorNum = n;

    for (uint32_t i = 0; i < n; i++)
        m_Scissors[i] = GetScissorRect(r[i]);

    if (m_RenderEncoder)
        m_RenderEncoder->setScissorRects(m_Scissors, n);
}

NRI_INLINE void CommandBufferMetal::CmdSetStencilReference(uint8_t f, uint8_t b) {
    m_FrontStencil = f;
    m_BackStencil = b;

    if (m_RenderEncoder)
        m_RenderEncoder->setStencilReferenceValues(f, b);
}

NRI_INLINE void CommandBufferMetal::CmdSetDepthBounds(float min, float max) {
    m_DepthMin = min;
    m_DepthMax = max;

    if (m_RenderEncoder && m_Pipeline && m_Pipeline->IsDepthBoundsEnabled())
        m_RenderEncoder->setDepthTestBounds(min, max);
}

NRI_INLINE void CommandBufferMetal::CmdSetBlendConstants(const Color32f& c) {
    m_BlendColor = c;

    if (m_RenderEncoder)
        m_RenderEncoder->setBlendColor(c.x, c.y, c.z, c.w);
}

NRI_INLINE void CommandBufferMetal::CmdSetSampleLocations(const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    NRI_CHECK(locationNum == sampleNum && locationNum <= 16, "Metal supports one sample-location pattern per pixel");
    MaybeUnused(sampleNum);

    m_SamplePositionNum = locationNum;

    for (uint32_t i = 0; i < locationNum; i++)
        m_SamplePositions[i] = {(locations[i].x + 8) / 16.0f, (locations[i].y + 8) / 16.0f};

    if (m_RenderPass && m_Pipeline && m_Pipeline->HasSampleLocations() && SuspendRendering())
        m_RenderPass->setSamplePositions(m_SamplePositions, m_SamplePositionNum);
}

NRI_INLINE void CommandBufferMetal::CmdSetShadingRate(const ShadingRateDesc&) {
    RecordFailure(Result::UNSUPPORTED);
}

NRI_INLINE void CommandBufferMetal::CmdSetDepthBias(const DepthBiasDesc& d) {
    // Like in VK, dynamic depth bias applies only to pipelines with enabled depth bias
    if (!m_Pipeline || !IsDepthBiasEnabled(m_Pipeline->GetDepthBias()))
        return;

    m_DepthBias = d;
    m_HasDepthBias = true;

    if (m_RenderEncoder)
        m_RenderEncoder->setDepthBias(d.constant, d.slope, d.clamp);
}

NRI_INLINE void CommandBufferMetal::CmdBeginRendering(const RenderingDesc& desc) {
    EndCompute();

    MTL4::RenderPassDescriptor* pass = MTL4::RenderPassDescriptor::alloc()->init();
    m_RenderColorNum = (uint8_t)desc.colorNum;
    m_ViewMask = desc.viewMask;
    uint32_t layerNum = UINT32_MAX;
    m_RenderDepth = m_RenderStencil = MTL::PixelFormatInvalid;
    m_RenderWidth = m_RenderHeight = UINT32_MAX;
    m_RenderSampleNum = 1;
    m_HasMemorylessAttachment = false;

    auto updateExtent = [&](const DescriptorMetal& descriptor) {
        MTL::Texture* texture = descriptor.GetTexture();
        const uint32_t mipOffset = descriptor.GetTextureViewDesc().mipOffset;

        m_HasMemorylessAttachment = m_HasMemorylessAttachment || texture->storageMode() == MTL::StorageModeMemoryless;

        m_RenderWidth = std::min(m_RenderWidth, std::max(1u, (uint32_t)texture->width() >> mipOffset));
        m_RenderHeight = std::min(m_RenderHeight, std::max(1u, (uint32_t)texture->height() >> mipOffset));
        m_RenderSampleNum = (uint8_t)texture->sampleCount();
        layerNum = std::min(layerNum, GetAttachmentLayerNum(descriptor));
    };

    for (uint32_t i = 0; i < desc.colorNum; i++) {
        const AttachmentDesc& a = desc.colors[i];
        const DescriptorMetal& d = *(DescriptorMetal*)a.descriptor;
        const TextureViewDesc& viewDesc = d.GetTextureViewDesc();

        auto* n = pass->colorAttachments()->object(i);
        n->setTexture(d.GetTexture());
        n->setLevel(viewDesc.mipOffset);
        n->setSlice(viewDesc.layerOffset);
        n->setDepthPlane(viewDesc.sliceOffset);
        n->setLoadAction(a.loadOp == LoadOp::CLEAR ? MTL::LoadActionClear : MTL::LoadActionLoad);
        n->setStoreAction(a.storeOp == StoreOp::STORE ? MTL::StoreActionStore : MTL::StoreActionDontCare);

        m_RenderColors[i] = d.GetTexture()->pixelFormat();
        m_RenderColorFormats[i] = viewDesc.format;
        updateExtent(d);

        const FormatProps& props = GetFormatProps(viewDesc.format);

        if (props.isInteger && props.isSigned)
            n->setClearColor(MTL::ClearColor(a.clearValue.color.i.x, a.clearValue.color.i.y, a.clearValue.color.i.z, a.clearValue.color.i.w));
        else if (props.isInteger)
            n->setClearColor(MTL::ClearColor(a.clearValue.color.ui.x, a.clearValue.color.ui.y, a.clearValue.color.ui.z, a.clearValue.color.ui.w));
        else
            n->setClearColor(MTL::ClearColor(a.clearValue.color.f.x, a.clearValue.color.f.y, a.clearValue.color.f.z, a.clearValue.color.f.w));

        if (!a.resolveDst)
            continue;

        if (a.resolveOp == ResolveOp::AVERAGE && m_Device.IsNativeAverageResolveSupported(viewDesc.format))
            SetAttachmentResolve(n, a);
        else {
            // Other color resolves are performed by a shader after the pass, which needs the multisampled contents
            const DescriptorMetal& resolve = *(DescriptorMetal*)a.resolveDst;
            const TextureViewDesc& resolveViewDesc = resolve.GetTextureViewDesc();
            m_AttachmentResolves[m_AttachmentResolveNum++] = {d.GetTexture(), resolve.GetTexture(), a.resolveOp, viewDesc.format, viewDesc.mipOffset, resolveViewDesc.mipOffset, viewDesc.layerOffset, resolveViewDesc.layerOffset, (uint16_t)GetAttachmentLayerNum(d)};
            n->setStoreAction(MTL::StoreActionStore);
        }
    }

    // A combined depth-stencil view in "depth" covers the stencil plane too (like in VK and D3D12)
    const AttachmentDesc* depthAttachmentDesc = desc.depth.descriptor ? &desc.depth : nullptr;
    const AttachmentDesc* stencilAttachmentDesc = desc.stencil.descriptor ? &desc.stencil : nullptr;

    if (!stencilAttachmentDesc && depthAttachmentDesc && GetFormatProps(((DescriptorMetal*)depthAttachmentDesc->descriptor)->GetTextureViewDesc().format).isStencil)
        stencilAttachmentDesc = depthAttachmentDesc;

    if (depthAttachmentDesc) {
        const AttachmentDesc& a = *depthAttachmentDesc;
        const DescriptorMetal& d = *(DescriptorMetal*)a.descriptor;
        const TextureViewDesc& viewDesc = d.GetTextureViewDesc();

        MTL::RenderPassDepthAttachmentDescriptor* n = pass->depthAttachment();
        n->setTexture(d.GetTexture());
        n->setLevel(viewDesc.mipOffset);
        n->setSlice(viewDesc.layerOffset);
        n->setLoadAction(a.loadOp == LoadOp::CLEAR ? MTL::LoadActionClear : MTL::LoadActionLoad);
        n->setStoreAction(a.storeOp == StoreOp::STORE ? MTL::StoreActionStore : MTL::StoreActionDontCare);
        n->setClearDepth(a.clearValue.depthStencil.depth);

        // Metal depth resolve filters are "sample 0", "min" and "max" ("AVERAGE" is rejected by validation)
        if (a.resolveDst) {
            SetAttachmentResolve(n, a);

            if (a.resolveOp == ResolveOp::SAMPLE_ZERO)
                n->setDepthResolveFilter(MTL::MultisampleDepthResolveFilterSample0);
            else
                n->setDepthResolveFilter(a.resolveOp == ResolveOp::MIN ? MTL::MultisampleDepthResolveFilterMin : MTL::MultisampleDepthResolveFilterMax);
        }

        m_RenderDepth = d.GetTexture()->pixelFormat();
        updateExtent(d);
    }

    if (stencilAttachmentDesc) {
        const AttachmentDesc& a = *stencilAttachmentDesc;
        const DescriptorMetal& d = *(DescriptorMetal*)a.descriptor;
        const TextureViewDesc& viewDesc = d.GetTextureViewDesc();

        MTL::RenderPassStencilAttachmentDescriptor* n = pass->stencilAttachment();
        n->setTexture(d.GetTexture());
        n->setLevel(viewDesc.mipOffset);
        n->setSlice(viewDesc.layerOffset);
        n->setLoadAction(a.loadOp == LoadOp::CLEAR ? MTL::LoadActionClear : MTL::LoadActionLoad);
        n->setStoreAction(a.storeOp == StoreOp::STORE ? MTL::StoreActionStore : MTL::StoreActionDontCare);
        n->setClearStencil(a.clearValue.depthStencil.stencil);

        // Metal stencil resolve filters are "sample 0" and "the sample selected by the depth filter", only "SAMPLE_ZERO" matches an NRI
        // resolve op ("MIN" and "MAX" are rejected by validation). A combined "depth" attachment resolves stencil with "depth.resolveOp"
        if (a.resolveDst && a.resolveOp == ResolveOp::SAMPLE_ZERO) {
            SetAttachmentResolve(n, a);
            n->setStencilResolveFilter(MTL::MultisampleStencilResolveFilterSample0);
        }

        m_RenderStencil = d.GetTexture()->pixelFormat();
        updateExtent(d);
    }

    pass->setRenderTargetArrayLength(layerNum == UINT32_MAX ? 1 : layerNum);

    // Store actions are deferred to "CmdEndRendering", since hidden render pass splits need to store and load attachments
    for (uint32_t i = 0; i < m_RenderColorNum; i++) {
        auto* attachment = pass->colorAttachments()->object(i);
        m_RenderStore[i] = attachment->storeAction();
        attachment->setStoreAction(MTL::StoreActionUnknown);
    }

    m_DepthStore = pass->depthAttachment()->storeAction();
    m_StencilStore = pass->stencilAttachment()->storeAction();

    if (m_RenderDepth != MTL::PixelFormatInvalid)
        pass->depthAttachment()->setStoreAction(MTL::StoreActionUnknown);

    if (m_RenderStencil != MTL::PixelFormatInvalid)
        pass->stencilAttachment()->setStoreAction(MTL::StoreActionUnknown);

    // Attach the last used visibility buffer upfront to avoid a render pass split on the first query
    if (m_VisibilityBuffer) {
        pass->setVisibilityResultBuffer(m_VisibilityBuffer);
        pass->setVisibilityResultType(MTL::VisibilityResultTypeAccumulate);
    }

    m_RenderPass = pass;
    m_VisibilityMode = MTL::VisibilityResultModeDisabled;
    m_VisibilityOffset = 0;
    m_ResumeStages = MTL::StageFragment; // attachments written by previous render passes (implicitly ordered in VK and D3D12)
    pass->setSamplePositions(m_SamplePositions, (m_Pipeline && m_Pipeline->HasSampleLocations()) ? m_SamplePositionNum : 0);

    // The render encoder is opened by the first command which needs it. Until then, commands which require a render pass split
    // (queries, sample locations, indirect draw preparation) can modify the pass or record compute work without splitting
}

void CommandBufferMetal::ApplyRasterState() {
    // Metal resets the state for a new encoder, but "CmdClearAttachments" overrides it. Without user state the defaults cover the render area
    const bool hasRenderArea = m_RenderWidth != UINT32_MAX && m_RenderHeight != UINT32_MAX;

    if (m_ViewportNum)
        m_RenderEncoder->setViewports(m_Viewports, m_ViewportNum);
    else if (hasRenderArea)
        m_RenderEncoder->setViewport(MTL::Viewport{0.0, 0.0, (double)m_RenderWidth, (double)m_RenderHeight, 0.0, 1.0});

    if (m_ScissorNum)
        m_RenderEncoder->setScissorRects(m_Scissors, m_ScissorNum);
    else if (hasRenderArea)
        m_RenderEncoder->setScissorRect(MTL::ScissorRect{0, 0, m_RenderWidth, m_RenderHeight});

    m_RenderEncoder->setStencilReferenceValues(m_FrontStencil, m_BackStencil);
    m_RenderEncoder->setBlendColor(m_BlendColor.x, m_BlendColor.y, m_BlendColor.z, m_BlendColor.w);
}

void CommandBufferMetal::BindArguments(BindPoint point) {
    if (point == BindPoint::GRAPHICS) {
        MTL4::RenderCommandEncoder* encoder = GetRenderEncoder();

        // A previous pass's pipeline may not match the new attachments. Bind only when drawing, after the caller has selected the pipeline for this pass
        if (m_RenderPipelineDirty && m_Pipeline) {
            MTL::VertexAmplificationViewMapping mappings[32] = {};
            uint32_t viewIndices[32] = {};
            uint32_t count = 0;
            const Multiview multiview = m_Pipeline->GetMultiview();
            uint32_t mask = multiview == Multiview::LAYER_BASED ? m_Pipeline->GetViewMask() : m_ViewMask;

            // Amplification IDs are dense and map to the set bits of the view mask
            for (uint32_t view = 0; mask; view++, mask >>= 1) {
                if (mask & 1) {
                    viewIndices[count] = view;
                    mappings[count].renderTargetArrayIndexOffset = multiview == Multiview::LAYER_BASED ? view : 0;
                    mappings[count].viewportArrayIndexOffset = multiview == Multiview::VIEWPORT_BASED ? view : 0;
                    count++;
                }
            }

            encoder->setVertexAmplificationCount(std::max(1u, count), mappings);

            if (count && !m_Pipeline->IsConverted())
                SetArgumentAddress(ARGUMENT_SLOT_MULTIVIEW, Upload(viewIndices, sizeof(viewIndices)));

            encoder->setRenderPipelineState(m_Pipeline->GetRenderPipeline());
            encoder->setDepthStencilState(m_Pipeline->GetDepthStencilState());
            encoder->setCullMode(m_Pipeline->GetCullMode());
            encoder->setFrontFacingWinding(m_Pipeline->GetWinding());
            encoder->setTriangleFillMode(m_Pipeline->GetFillMode());
            encoder->setDepthClipMode(m_Pipeline->GetDepthClipMode());

            if (m_Device.GetDesc().features.depthBoundsTest)
                encoder->setDepthTestBounds(m_Pipeline->IsDepthBoundsEnabled() ? m_DepthMin : 0.0f, m_Pipeline->IsDepthBoundsEnabled() ? m_DepthMax : 1.0f);

            const auto& bias = m_HasDepthBias ? m_DepthBias : m_Pipeline->GetDepthBias();
            encoder->setDepthBias(bias.constant, bias.slope, bias.clamp);
            m_RenderPipelineDirty = false;
        }
    }

    // Root data is uploaded only if it has changed since the last upload
    State& s = point == BindPoint::GRAPHICS ? m_Graphics : m_Compute;
    MTL::GPUAddress root = m_DrawRootAddress;
    m_DrawRootAddress = 0;

    if (!root && s.layout && !s.root.empty()) {
        if (!s.rootAddress)
            s.rootAddress = Upload(s.root.data(), s.root.size());

        root = s.rootAddress;
    }

    if (m_DescriptorPool && m_AreHeapsDirty) {
        SetArgumentAddress(ARGUMENT_SLOT_RESOURCE_HEAP, m_DescriptorPool->GetResourceHeapAddress());
        SetArgumentAddress(ARGUMENT_SLOT_SAMPLER_HEAP, m_DescriptorPool->GetSamplerHeapAddress());
        m_AreHeapsDirty = false;
    }

#if NRI_ENABLE_METAL_SHADER_CONVERTER
    if (point == BindPoint::GRAPHICS && m_Pipeline && m_Pipeline->IsTessellationEmulation()) {
        if (!root)
            root = m_Device.GetConstantsAddress(); // zeros

        SetArgumentAddress(kIRArgumentBufferHullDomainBindPoint, root);

        if (!m_DescriptorPool || !m_DescriptorPool->GetResourceHeapAddress()) {
            SetArgumentAddress(ARGUMENT_SLOT_RESOURCE_HEAP, root);
            m_AreHeapsDirty = true;
        }

        if (!m_DescriptorPool || !m_DescriptorPool->GetSamplerHeapAddress()) {
            SetArgumentAddress(ARGUMENT_SLOT_SAMPLER_HEAP, root);
            m_AreHeapsDirty = true;
        }
    }
#endif

    if (root)
        SetArgumentAddress(ARGUMENT_SLOT_ROOT, root);

    // Argument tables are snapshotted at draw and dispatch time, i.e. later "setAddress" calls don't require rebinding
    if (point == BindPoint::GRAPHICS) {
        if (!m_IsRenderTableBound) {
            m_RenderEncoder->setArgumentTable(m_Arguments, MTL::RenderStageVertex | MTL::RenderStageObject | MTL::RenderStageMesh | MTL::RenderStageFragment);
            m_IsRenderTableBound = true;
        }
    } else
        SetComputeState(m_Arguments, nullptr);
}

NRI_INLINE void CommandBufferMetal::CmdClearAttachments(const ClearAttachmentDesc* clears, uint32_t clearNum, const Rect* rects, uint32_t rectNum) {
    MTL4::RenderCommandEncoder* encoder = GetRenderEncoder();

    if (!m_IsRenderTableBound) {
        encoder->setArgumentTable(m_Arguments, MTL::RenderStageVertex | MTL::RenderStageObject | MTL::RenderStageMesh | MTL::RenderStageFragment);
        m_IsRenderTableBound = true;
    }

    // Clears must not count toward an active occlusion query. A visibility offset can't be enabled twice in one encoder, i.e. the query
    // continues in the next render encoder (impossible with memoryless attachments)
    const bool isQueryPaused = m_VisibilityMode != MTL::VisibilityResultModeDisabled && !m_HasMemorylessAttachment;

    if (isQueryPaused)
        encoder->setVisibilityResultMode(MTL::VisibilityResultModeDisabled, 0);

    for (uint32_t i = 0; i < clearNum; i++) {
        const ClearAttachmentDesc& desc = clears[i];

        ClearPipelineKeyMetal key = {};
        memcpy(key.colors, m_RenderColors, sizeof(key.colors));
        key.colorNum = m_RenderColorNum;
        key.colorIndex = (uint8_t)desc.colorAttachmentIndex;
        key.sampleNum = m_RenderSampleNum;
        key.planes = desc.planes;

        if (key.planes == PlaneBits::ALL) {
            if (desc.colorAttachmentIndex < m_RenderColorNum)
                key.planes = PlaneBits::COLOR;
            else
                key.planes = (PlaneBits)((m_RenderDepth != MTL::PixelFormatInvalid ? (uint8_t)PlaneBits::DEPTH : 0) | (m_RenderStencil != MTL::PixelFormatInvalid ? (uint8_t)PlaneBits::STENCIL : 0));
        }

        if (key.planes & PlaneBits::COLOR)
            key.colorType = GetColorType(m_RenderColorFormats[desc.colorAttachmentIndex]);

        const ClearPipelineMetal clear = m_Device.GetInternalShaders().GetClearPipeline(key);

        if (!clear.pipeline) {
            RecordFailure(Result::FAILURE);
            continue;
        }

        struct ClearConstants {
            Color32f color;
            float depth;
        } constants = {};

        memcpy(&constants.color, &desc.value.color, sizeof(constants.color));
        constants.depth = (key.planes & PlaneBits::DEPTH) ? desc.value.depthStencil.depth : 0.0f;

        SetArgumentAddress(INTERNAL_SLOT_CONSTANTS, Upload(&constants, sizeof(constants)));
        encoder->setRenderPipelineState(clear.pipeline);
        encoder->setVertexAmplificationCount(1, nullptr);
        encoder->setDepthStencilState(clear.depthStencil);
        encoder->setCullMode(MTL::CullModeNone);
        encoder->setTriangleFillMode(MTL::TriangleFillModeFill);
        encoder->setDepthBias(0.0f, 0.0f, 0.0f);
        encoder->setDepthClipMode(MTL::DepthClipModeClamp);

        if (m_Device.GetDesc().features.depthBoundsTest)
            encoder->setDepthTestBounds(0.0f, 1.0f);

        encoder->setStencilReferenceValue(desc.value.depthStencil.stencil);
        encoder->setViewport(MTL::Viewport{0.0, 0.0, (double)m_RenderWidth, (double)m_RenderHeight, 0.0, 1.0});

        if (rectNum) {
            for (uint32_t j = 0; j < rectNum; j++) {
                encoder->setScissorRect(GetScissorRect(rects[j]));
                encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, 0, 3, m_RenderPass->renderTargetArrayLength());
            }
        } else {
            encoder->setScissorRect(MTL::ScissorRect{0, 0, m_RenderWidth, m_RenderHeight});
            encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, 0, 3, m_RenderPass->renderTargetArrayLength());
        }
    }

    m_RenderPipelineDirty = true;

    if (isQueryPaused)
        SuspendRendering();
    else
        ApplyRasterState();
}

NRI_INLINE void CommandBufferMetal::CmdDraw(const DrawDesc& d) {
    SetDrawArguments(&d, sizeof(d), false);
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    if (m_Pipeline->IsGeometryEmulation() || m_Pipeline->IsTessellationEmulation()) {
        DrawEmulated(&d, sizeof(d), false, d.vertexNum, d.instanceNum);

        return;
    }
#endif
    BindArguments(BindPoint::GRAPHICS);
    m_RenderEncoder->drawPrimitives(m_Pipeline->GetPrimitiveType(), d.baseVertex, d.vertexNum, d.instanceNum, d.baseInstance);
}

NRI_INLINE void CommandBufferMetal::CmdDrawIndexed(const DrawIndexedDesc& d) {
    SetDrawArguments(&d, sizeof(d), true);
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    if (m_Pipeline->IsGeometryEmulation() || m_Pipeline->IsTessellationEmulation()) {
        DrawEmulated(&d, sizeof(d), true, d.indexNum, d.instanceNum);

        return;
    }
#endif
    BindArguments(BindPoint::GRAPHICS);
    uint64_t o = uint64_t(d.baseIndex) * (m_IndexType == MTL::IndexTypeUInt16 ? 2 : 4);
    m_RenderEncoder->drawIndexedPrimitives(m_Pipeline->GetPrimitiveType(), d.indexNum, m_IndexType, m_IndexAddress + o, m_IndexLength - o, d.instanceNum, d.baseVertex, d.baseInstance);
}

NRI_INLINE void CommandBufferMetal::CmdDrawIndirect(const Buffer& b, uint64_t o, uint32_t n, uint32_t s, const Buffer* c, uint64_t co) {
    const bool emulatedParameters = m_Graphics.layout->IsDrawParametersEmulationEnabled();
    const uint32_t argumentSize = emulatedParameters ? sizeof(DrawBaseDesc) : sizeof(DrawDesc);
    MTL::GPUAddress address = PrepareIndirectArguments(b, o, n, s, argumentSize, c, co);

    if (!address)
        return;

    MTL::GPUAddress roots = 0;

    if (!PrepareIndirectDrawRoots(address, n, s, roots))
        return;

    address += emulatedParameters ? 8 : 0;

#if NRI_ENABLE_METAL_SHADER_CONVERTER
    if (m_Pipeline->IsGeometryEmulation() || m_Pipeline->IsTessellationEmulation()) {
        DrawEmulatedIndirect(address, roots, n, s, false);

        return;
    }
#endif
    // Converted shaders read the index type
    if (m_Pipeline->IsConverted())
        SetArgumentAddress(ARGUMENT_SLOT_DRAW_UNIFORMS, m_Device.GetConstantsAddress(0));

    DrawIndirect(address, roots, n, s, false);
}

NRI_INLINE void CommandBufferMetal::CmdDrawIndexedIndirect(const Buffer& b, uint64_t o, uint32_t n, uint32_t s, const Buffer* c, uint64_t co) {
    const bool emulatedParameters = m_Graphics.layout->IsDrawParametersEmulationEnabled();
    const uint32_t argumentSize = emulatedParameters ? sizeof(DrawIndexedBaseDesc) : sizeof(DrawIndexedDesc);
    MTL::GPUAddress address = PrepareIndirectArguments(b, o, n, s, argumentSize, c, co);

    if (!address)
        return;

    MTL::GPUAddress roots = 0;

    if (!PrepareIndirectDrawRoots(address, n, s, roots))
        return;

    address += emulatedParameters ? 8 : 0;

#if NRI_ENABLE_METAL_SHADER_CONVERTER
    if (m_Pipeline->IsGeometryEmulation() || m_Pipeline->IsTessellationEmulation()) {
        DrawEmulatedIndirect(address, roots, n, s, true);

        return;
    }
#endif
    // Converted shaders read the index type
    if (m_Pipeline->IsConverted())
        SetArgumentAddress(ARGUMENT_SLOT_DRAW_UNIFORMS, m_Device.GetConstantsAddress(m_IndexType == MTL::IndexTypeUInt16 ? 1 : 2));

    DrawIndirect(address, roots, n, s, true);
}

void CommandBufferMetal::DrawIndirect(MTL::GPUAddress arguments, MTL::GPUAddress roots, uint32_t drawNum, uint32_t stride, bool indexed) {
    m_DrawRootAddress = roots;
    BindArguments(BindPoint::GRAPHICS);

    // Only argument table slots change per draw (tables are snapshotted at draw time). Metal has no multi-draw indirect, so the CPU cost is
    // O(drawNum), draws after "count" are empty. GPU-encoded indirect command buffers can't replace the loop: an ICB draw with a pipeline
    // created without "supportIndirectCommandBuffers" silently draws nothing, and per-draw roots and draw arguments require
    // "inheritBuffers = false", which drops all other argument table bindings
    const MTL::PrimitiveType primitiveType = m_Pipeline->GetPrimitiveType();
    const uint64_t rootSize = m_Graphics.layout->GetRootDataSize();
    const bool isConverted = m_Pipeline->IsConverted();

    for (uint32_t i = 0; i < drawNum; i++) {
        const MTL::GPUAddress drawArguments = arguments + uint64_t(i) * stride;

        if (roots)
            SetArgumentAddress(ARGUMENT_SLOT_ROOT, roots + i * rootSize);

        if (isConverted)
            SetArgumentAddress(ARGUMENT_SLOT_DRAW_ARGUMENTS, drawArguments);

        if (indexed)
            m_RenderEncoder->drawIndexedPrimitives(primitiveType, m_IndexType, m_IndexAddress, m_IndexLength, drawArguments);
        else
            m_RenderEncoder->drawPrimitives(primitiveType, drawArguments);
    }
}

NRI_INLINE void CommandBufferMetal::CmdDrawMeshTasks(const DrawMeshTasksDesc& d) {
    BindArguments(BindPoint::GRAPHICS);
    m_RenderEncoder->drawMeshThreadgroups(MTL::Size(d.x, d.y, d.z), m_Pipeline->GetTaskThreadGroupSize(), m_Pipeline->GetMeshThreadGroupSize());
}

NRI_INLINE void CommandBufferMetal::CmdDrawMeshTasksIndirect(const Buffer& b, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countOffset) {
    const MTL::GPUAddress address = PrepareIndirectArguments(b, offset, drawNum, stride, sizeof(DrawMeshTasksDesc), countBuffer, countOffset);

    if (!address)
        return;

    BindArguments(BindPoint::GRAPHICS);

    for (uint32_t i = 0; i < drawNum; i++)
        m_RenderEncoder->drawMeshThreadgroups(address + uint64_t(i) * stride, m_Pipeline->GetTaskThreadGroupSize(), m_Pipeline->GetMeshThreadGroupSize());
}

bool CommandBufferMetal::DispatchInternal(InternalKernelMetal kernel, const void* constants, uint32_t constantsSize, uint32_t threadNum) {
    MTL::ComputePipelineState* pipeline = GetInternalKernel(kernel);

    if (!pipeline)
        return false;

    const MTL::GPUAddress constantsAddress = Upload(constants, constantsSize);

    if (!constantsAddress) {
        RecordFailure(Result::OUT_OF_MEMORY);

        return false;
    }

    m_InternalArguments->setAddress(constantsAddress, INTERNAL_SLOT_KERNEL_CONSTANTS);
    SetComputeState(m_InternalArguments, pipeline);

    // Internal dispatches of a draw share the compute encoder and can consume the output of the previous one (i.e. filtered draw arguments)
    if (m_ResumeStages & MTL::StageDispatch)
        m_ComputeEncoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);

    m_ComputeEncoder->dispatchThreads(MTL::Size(threadNum, 1, 1), MTL::Size(64, 1, 1));

    return true;
}

bool CommandBufferMetal::SuspendRendering() {
    // Nothing has been recorded since the pass has begun or since the previous suspension
    if (!m_RenderEncoder)
        return true;

    // Suspension stores and reloads attachments
    NRI_CHECK(!m_HasMemorylessAttachment, "This command requires a render pass split, which is impossible with memoryless attachments");

    if (m_HasMemorylessAttachment) {
        RecordFailure(Result::UNSUPPORTED);

        return false;
    }

    for (uint32_t i = 0; i < m_RenderColorNum; i++) {
        m_RenderEncoder->setColorStoreAction(MTL::StoreActionStore, i);
        m_RenderPass->colorAttachments()->object(i)->setLoadAction(MTL::LoadActionLoad);
    }

    if (m_RenderDepth != MTL::PixelFormatInvalid) {
        m_RenderEncoder->setDepthStoreAction(MTL::StoreActionStore);
        m_RenderPass->depthAttachment()->setLoadAction(MTL::LoadActionLoad);
    }

    if (m_RenderStencil != MTL::PixelFormatInvalid) {
        m_RenderEncoder->setStencilStoreAction(MTL::StoreActionStore);
        m_RenderPass->stencilAttachment()->setLoadAction(MTL::LoadActionLoad);
    }

    EndRenderEncoder(true);
    m_ResumeStages |= MTL::StageFragment;

    return true;
}

MTL::GPUAddress CommandBufferMetal::PrepareIndirectArguments(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t& stride, uint32_t argumentSize, const Buffer* countBuffer, uint64_t countOffset) {
    const MTL::GPUAddress source = ((const BufferMetal&)buffer).GetGpuAddress() + offset;

    if (!drawNum || !countBuffer)
        return drawNum ? source : 0;

    const MTL::GPUAddress destination = Upload(nullptr, uint64_t(drawNum) * argumentSize);

    if (!destination) {
        RecordFailure(Result::OUT_OF_MEMORY);

        return 0;
    }

    struct Arguments {
        MTL::GPUAddress source, count, destination;
        uint32_t drawNum, stride, words;
    } arguments = {source, ((const BufferMetal*)countBuffer)->GetGpuAddress() + countOffset, destination, drawNum, stride / 4, argumentSize / 4};

    // A render encoder can't dispatch. Inputs are ordered by the app's "INDIRECT" barrier (it includes dispatches). Preparation is needed
    // only for count buffers, draw parameters (index) emulation and geometry (tessellation) emulation. Dispatches of a call share one
    // compute encoder, but a call inside a started pass splits it: its draws must follow its dispatches and precede those of the next call
    if (!SuspendRendering())
        return 0;

    if (!DispatchInternal(InternalKernelMetal::FILTER_DRAWS, &arguments, sizeof(arguments), drawNum))
        return 0;

    m_ResumeStages |= MTL::StageDispatch;
    stride = argumentSize;

    return destination;
}

NRI_INLINE void CommandBufferMetal::CmdEndRendering() {
    // Load and store actions need an encoder, even if nothing has been recorded
    MTL4::RenderCommandEncoder* encoder = GetRenderEncoder();

    for (uint32_t i = 0; i < m_RenderColorNum; i++)
        encoder->setColorStoreAction(m_RenderStore[i], i);

    if (m_RenderDepth != MTL::PixelFormatInvalid)
        encoder->setDepthStoreAction(m_DepthStore);

    if (m_RenderStencil != MTL::PixelFormatInvalid)
        encoder->setStencilStoreAction(m_StencilStore);

    EndRenderEncoder(false);
    m_RenderPass->release();
    m_RenderPass = nullptr;

    for (uint32_t i = 0; i < m_AttachmentResolveNum; i++) {
        const AttachmentResolveMetal& resolve = m_AttachmentResolves[i];

        for (uint32_t layer = 0; layer < resolve.layerNum; layer++) {
            TextureRegionDesc srcRegion = {}, dstRegion = {};
            srcRegion.mipOffset = resolve.srcMip;
            srcRegion.layerOffset = resolve.srcLayer + layer;
            dstRegion.mipOffset = resolve.dstMip;
            dstRegion.layerOffset = resolve.dstLayer + layer;
            ResolveColor(resolve.dst, dstRegion, resolve.src, srcRegion, resolve.op, resolve.format, true);
        }
    }
    m_AttachmentResolveNum = 0;
}

void CommandBufferMetal::SetDrawArguments(const void* data, uint64_t size, bool indexed) {
    const PipelineLayoutMetal& layout = *m_Graphics.layout;

    // Root data changes only if the emulated values change
    if (layout.IsDrawParametersEmulationEnabled()) {
        uint8_t* dst = m_Graphics.root.data() + layout.GetDrawParametersOffset();
        const uint8_t* src = (const uint8_t*)data + (indexed ? 12 : 8);

        if (memcmp(dst, src, 8)) {
            memcpy(dst, src, 8);
            m_Graphics.rootAddress = 0;
        }
    }

    if (layout.IsDrawIndexEmulationEnabled()) {
        uint8_t* dst = m_Graphics.root.data() + layout.GetDrawIndexOffset();
        const uint32_t drawIndex = 0;

        if (memcmp(dst, &drawIndex, sizeof(drawIndex))) {
            memcpy(dst, &drawIndex, sizeof(drawIndex));
            m_Graphics.rootAddress = 0;
        }
    }

    if (!m_Pipeline->IsConverted())
        return;

    // Converted shaders read draw arguments and the index type ("kIRArgumentBufferDrawArgumentsBindPoint", "kIRArgumentBufferUniformsBindPoint").
    // Draw arguments are uploaded only if they change, index types are static
    if (size != m_DrawArgumentsSize || memcmp(m_DrawArguments, data, (size_t)size)) {
        m_DrawArgumentsAddress = Upload(data, size);
        m_DrawArgumentsSize = m_DrawArgumentsAddress ? (uint32_t)size : 0;
        memcpy(m_DrawArguments, data, (size_t)size);
    }

    SetArgumentAddress(ARGUMENT_SLOT_DRAW_ARGUMENTS, m_DrawArgumentsAddress);
    SetArgumentAddress(ARGUMENT_SLOT_DRAW_UNIFORMS, m_Device.GetConstantsAddress(indexed ? (m_IndexType == MTL::IndexTypeUInt16 ? 1 : 2) : 0));
}

bool CommandBufferMetal::PrepareIndirectDrawRoots(MTL::GPUAddress arguments, uint32_t drawNum, uint32_t stride, MTL::GPUAddress& roots) {
    const bool parameters = m_Graphics.layout->IsDrawParametersEmulationEnabled();
    const bool index = m_Graphics.layout->IsDrawIndexEmulationEnabled();

    roots = 0;

    if (!parameters && !index)
        return true;

    const uint32_t rootSize = m_Graphics.layout->GetRootDataSize();

    if (!m_Graphics.rootAddress)
        m_Graphics.rootAddress = Upload(m_Graphics.root.data(), rootSize);

    const MTL::GPUAddress destination = Upload(nullptr, uint64_t(drawNum) * rootSize);

    if (!m_Graphics.rootAddress || !destination) {
        RecordFailure(Result::OUT_OF_MEMORY);

        return false;
    }

    struct Arguments {
        MTL::GPUAddress source, root, roots;
        uint32_t drawNum, stride, rootWords, parameterOffset, indexOffset;
    } constants = {arguments, m_Graphics.rootAddress, destination, drawNum, stride / 4, rootSize / 4, parameters ? m_Graphics.layout->GetDrawParametersOffset() / 4 : UINT32_MAX, index ? m_Graphics.layout->GetDrawIndexOffset() / 4 : UINT32_MAX};

    // A render encoder can't dispatch. Inputs are ordered by the app's "INDIRECT" barrier (it includes dispatches)
    if (!SuspendRendering())
        return false;

    if (!DispatchInternal(InternalKernelMetal::PREPARE_DRAW_ROOTS, &constants, sizeof(constants), drawNum))
        return false;

    m_ResumeStages |= MTL::StageDispatch;
    roots = destination;

    return true;
}

#if NRI_ENABLE_METAL_SHADER_CONVERTER
IRRuntimeDrawInfo CommandBufferMetal::PrepareEmulationDraw(bool indexed, MTL::Size& objectGroup, MTL::Size& meshGroup) {
    const IRRuntimePrimitiveType primitive = m_Pipeline->GetEmulationPrimitive();
    IRRuntimeDrawInfo info = {};
    uint32_t objectThreads = 0, meshThreads = 0;

    if (m_Pipeline->IsTessellationEmulation()) {
        const auto& config = m_Pipeline->GetTessellationConfig();
        info = IRRuntimeCalculateDrawInfoForGSTSEmulation(primitive, m_IndexType, config.outputPrimitiveType, config.gsMaxInputPrimitivesPerMeshThreadgroup, config.hsMaxPatchesPerObjectThreadgroup, config.hsInputControlPointCount, config.hsMaxObjectThreadsPerThreadgroup, config.gsInstanceCount);
        IRRuntimeCalculateThreadgroupSizeForTessellationAndGeometry(config.hsMaxPatchesPerObjectThreadgroup, config.hsMaxObjectThreadsPerThreadgroup, config.gsMaxInputPrimitivesPerMeshThreadgroup, &objectThreads, &meshThreads);
        const MTL::GPUAddress tables = m_Device.GetTessellatorTables();

        if (!tables) {
            RecordFailure(Result::OUT_OF_MEMORY);

            return {};
        }

        m_Arguments->setAddress(tables, kIRRuntimeTessellatorTablesBindPoint);
    } else {
        const auto& config = m_Pipeline->GetGeometryConfig();
        info = IRRuntimeCalculateDrawInfoForGSEmulation(primitive, m_IndexType, config.gsVertexSizeInBytes, config.gsMaxInputPrimitivesPerMeshThreadgroup, m_Pipeline->GetTessellationConfig().gsInstanceCount);
        IRRuntimeCalculateThreadgroupSizeForGeometry(primitive, config.gsMaxInputPrimitivesPerMeshThreadgroup, info.objectThreadgroupVertexStride, &objectThreads, &meshThreads);
    }
    info.indexType = indexed ? uint16_t(m_IndexType + 1) : kIRNonIndexedDraw;
    info.indexBuffer = indexed ? m_IndexAddress : 0;
    objectGroup = MTL::Size(objectThreads, 1, 1);
    meshGroup = MTL::Size(meshThreads, 1, 1);
    SetArgumentAddress(kIRArgumentBufferUniformsBindPoint, Upload(&info, sizeof(info)));

    m_Arguments->setAddress(Upload(m_EmulationVertexBuffers, sizeof(m_EmulationVertexBuffers)), kIRVertexBufferBindPoint);

    return info;
}

void CommandBufferMetal::DrawEmulated(const void* arguments, uint64_t size, bool indexed, uint32_t vertexNum, uint32_t instanceNum) {
    const IRRuntimePrimitiveType primitive = m_Pipeline->GetEmulationPrimitive();

    if (!instanceNum || vertexNum <= IRRuntimePrimitiveTypeVertexOverlap(primitive))
        return;

    MTL::Size objectThreads, meshThreads;
    const IRRuntimeDrawInfo info = PrepareEmulationDraw(indexed, objectThreads, meshThreads);

    if (!info.objectThreadgroupVertexStride)
        return;

    const MTL::Size groups = IRRuntimeCalculateObjectTgCountForTessellationAndGeometryEmulation(vertexNum, info.objectThreadgroupVertexStride, primitive, instanceNum);
    SetArgumentAddress(kIRArgumentBufferDrawArgumentsBindPoint, Upload(arguments, size));
    BindArguments(BindPoint::GRAPHICS);

    if (m_Pipeline->IsTessellationEmulation())
        m_RenderEncoder->setObjectThreadgroupMemoryLength(TESSELLATION_OBJECT_THREADGROUP_MEMORY_SIZE, 0);

    m_RenderEncoder->drawMeshThreadgroups(groups, objectThreads, meshThreads);
}

void CommandBufferMetal::DrawEmulatedIndirect(MTL::GPUAddress arguments, MTL::GPUAddress roots, uint32_t drawNum, uint32_t stride, bool indexed) {
    MTL::Size objectThreads, meshThreads;
    const IRRuntimeDrawInfo info = PrepareEmulationDraw(indexed, objectThreads, meshThreads);

    if (!info.objectThreadgroupVertexStride)
        return;

    const MTL::GPUAddress grids = Upload(nullptr, uint64_t(drawNum) * sizeof(DrawMeshTasksDesc));

    if (!grids) {
        RecordFailure(Result::OUT_OF_MEMORY);

        return;
    }

    struct Arguments {
        MTL::GPUAddress source, grids;
        uint32_t drawNum, stride, verticesPerGroup, overlap;
    } constants = {arguments, grids, drawNum, stride / 4, info.objectThreadgroupVertexStride, IRRuntimePrimitiveTypeVertexOverlap(m_Pipeline->GetEmulationPrimitive())};

    // A render encoder can't dispatch. Inputs are ordered by the app's "INDIRECT" barrier (it includes dispatches)
    if (!SuspendRendering() || !DispatchInternal(InternalKernelMetal::EMULATE_DRAWS, &constants, sizeof(constants), drawNum))
        return;

    m_ResumeStages |= MTL::StageDispatch;

    for (uint32_t i = 0; i < drawNum; i++) {
        if (roots)
            m_DrawRootAddress = roots + uint64_t(i) * m_Graphics.layout->GetRootDataSize();

        SetArgumentAddress(kIRArgumentBufferDrawArgumentsBindPoint, arguments + uint64_t(i) * stride);
        BindArguments(BindPoint::GRAPHICS);

        if (m_Pipeline->IsTessellationEmulation())
            m_RenderEncoder->setObjectThreadgroupMemoryLength(TESSELLATION_OBJECT_THREADGROUP_MEMORY_SIZE, 0);

        m_RenderEncoder->drawMeshThreadgroups(grids + uint64_t(i) * sizeof(DrawMeshTasksDesc), objectThreads, meshThreads);
    }
}
#endif

NRI_INLINE void CommandBufferMetal::CmdDispatch(const DispatchDesc& d) {
    BindArguments(BindPoint::COMPUTE);
    SetComputeState(m_Arguments, m_Pipeline->GetComputePipeline());
    m_ComputeEncoder->dispatchThreadgroups(MTL::Size(d.workGroupNumX, d.workGroupNumY, d.workGroupNumZ), m_Pipeline->GetThreadGroupSize());
}

NRI_INLINE void CommandBufferMetal::CmdDispatchIndirect(const Buffer& b, uint64_t o) {
    BindArguments(BindPoint::COMPUTE);
    SetComputeState(m_Arguments, m_Pipeline->GetComputePipeline());
    m_ComputeEncoder->dispatchThreadgroups(((BufferMetal&)b).GetGpuAddress() + o, m_Pipeline->GetThreadGroupSize());
}

NRI_INLINE void CommandBufferMetal::CmdCopyBuffer(Buffer& d, uint64_t dof, const Buffer& s, uint64_t sof, uint64_t z) {
    if (z == WHOLE_SIZE)
        z = ((const BufferMetal&)s).GetDesc().size;

    BeginCompute()->copyFromBuffer(((BufferMetal&)s).GetNativeObject(), sof, ((BufferMetal&)d).GetNativeObject(), dof, z);
}

NRI_INLINE void CommandBufferMetal::CmdCopyTexture(Texture& d, const TextureRegionDesc* dr, const Texture& s, const TextureRegionDesc* sr) {
    const FormatProps& srcProps = GetFormatProps(((TextureMetal&)s).GetDesc().format);
    const FormatProps& dstProps = GetFormatProps(((TextureMetal&)d).GetDesc().format);
    const bool selectiveSrcPlane = sr && srcProps.isDepth && srcProps.isStencil && (sr->planes == PlaneBits::DEPTH || sr->planes == PlaneBits::STENCIL);
    const bool selectiveDstPlane = dr && dstProps.isDepth && dstProps.isStencil && (dr->planes == PlaneBits::DEPTH || dr->planes == PlaneBits::STENCIL);

    if (selectiveSrcPlane || selectiveDstPlane) {
        const TextureDesc& source = ((TextureMetal&)s).GetDesc();
        const TextureDesc& destination = ((TextureMetal&)d).GetDesc();

        // Metal copies a single plane only through a buffer, which requires identical non-multisampled depth-stencil formats
        if (!selectiveSrcPlane || !selectiveDstPlane || source.format != destination.format || source.sampleNum > 1 || destination.sampleNum > 1) {
            NRI_REPORT_WARNING(&m_Device, "a single-plane depth-stencil copy requires identical non-multisampled formats");
            RecordFailure(Result::UNSUPPORTED);

            return;
        }

        NRI_CHECK(sr->planes == dr->planes, "Planes must match"); // validated

        const MTL::Size size = GetRegionSize((TextureMetal&)s, *sr);
        const bool stencil = sr->planes == PlaneBits::STENCIL;
        const uint64_t rowPitch = Align(size.width * (stencil ? 1 : 4), m_Device.GetDesc().memoryAlignment.uploadBufferTextureRow);
        const uint64_t slicePitch = Align(rowPitch * size.height, m_Device.GetDesc().memoryAlignment.uploadBufferTextureSlice);
        MTL::Buffer* staging = m_Allocator->CreateTransientBuffer(slicePitch * size.depth);

        if (!staging) {
            RecordFailure(Result::OUT_OF_MEMORY);

            return;
        }

        const MTL::BlitOption option = stencil ? MTL::BlitOptionStencilFromDepthStencil : MTL::BlitOptionDepthFromDepthStencil;
        MTL4::ComputeCommandEncoder* encoder = BeginCompute();
        encoder->copyFromTexture(((TextureMetal&)s).GetNativeObject(), sr->layerOffset, sr->mipOffset, MTL::Origin(sr->x, sr->y, sr->z), size, staging, 0, rowPitch, slicePitch, option);
        encoder->barrierAfterEncoderStages(MTL::StageBlit, MTL::StageBlit, MTL4::VisibilityOptionDevice);
        encoder->copyFromBuffer(staging, 0, rowPitch, slicePitch, size, ((TextureMetal&)d).GetNativeObject(), dr->layerOffset, dr->mipOffset, MTL::Origin(dr->x, dr->y, dr->z), option);

        return;
    }

    if (!dr && !sr) {
        BeginCompute()->copyFromTexture(((TextureMetal&)s).GetNativeObject(), ((TextureMetal&)d).GetNativeObject());

        return;
    }
    TextureRegionDesc a = sr ? *sr : TextureRegionDesc{}, b = dr ? *dr : TextureRegionDesc{};
    MTL::Size size = GetRegionSize((TextureMetal&)s, a);

    // Metal API validation requires compressed copy sizes to be multiples of the block size, even for mips smaller than a block
    const FormatProps& props = GetFormatProps(((TextureMetal&)s).GetDesc().format);
    size.width = (size.width + props.blockWidth - 1) / props.blockWidth * props.blockWidth;
    size.height = (size.height + props.blockHeight - 1) / props.blockHeight * props.blockHeight;
    BeginCompute()->copyFromTexture(((TextureMetal&)s).GetNativeObject(), a.layerOffset, a.mipOffset, MTL::Origin(a.x, a.y, a.z), size, ((TextureMetal&)d).GetNativeObject(), b.layerOffset, b.mipOffset, MTL::Origin(b.x, b.y, b.z));
}

NRI_INLINE void CommandBufferMetal::CmdUploadBufferToTexture(Texture& d, const TextureRegionDesc& r, const Buffer& s, const TextureDataLayoutDesc& l) {
    BeginCompute()->copyFromBuffer(((BufferMetal&)s).GetNativeObject(), l.offset, l.rowPitch, l.slicePitch, GetRegionSize((TextureMetal&)d, r), ((TextureMetal&)d).GetNativeObject(), r.layerOffset, r.mipOffset, MTL::Origin(r.x, r.y, r.z), GetTextureCopyOptions((TextureMetal&)d, r.planes));
}

NRI_INLINE void CommandBufferMetal::CmdReadbackTextureToBuffer(Buffer& d, const TextureDataLayoutDesc& l, const Texture& s, const TextureRegionDesc& r) {
    BeginCompute()->copyFromTexture(((TextureMetal&)s).GetNativeObject(), r.layerOffset, r.mipOffset, MTL::Origin(r.x, r.y, r.z), GetRegionSize((TextureMetal&)s, r), ((BufferMetal&)d).GetNativeObject(), l.offset, l.rowPitch, l.slicePitch, GetTextureCopyOptions((TextureMetal&)s, r.planes));
}

NRI_INLINE void CommandBufferMetal::CmdZeroBuffer(Buffer& b, uint64_t o, uint64_t z) {
    if (z == WHOLE_SIZE)
        z = ((BufferMetal&)b).GetDesc().size - o;

    BeginCompute()->fillBuffer(((BufferMetal&)b).GetNativeObject(), NS::Range(o, z), 0);
}

void CommandBufferMetal::ResolveColor(MTL::Texture* dst, const TextureRegionDesc& dstRegion, MTL::Texture* src, const TextureRegionDesc& srcRegion, ResolveOp op, Format format, bool attachmentResolve) {
    const uint32_t srcWidth = std::max(1u, (uint32_t)src->width() >> srcRegion.mipOffset);
    const uint32_t srcHeight = std::max(1u, (uint32_t)src->height() >> srcRegion.mipOffset);
    const uint32_t width = srcRegion.width ? srcRegion.width : srcWidth;
    const uint32_t height = srcRegion.height ? srcRegion.height : srcHeight;
    const bool isNative = op == ResolveOp::AVERAGE && m_Device.IsNativeAverageResolveSupported(format);
    MTL::Texture* transient = attachmentResolve ? dst : m_Allocator->CreateTransientTexture(src->pixelFormat(), isNative ? srcWidth : width, isNative ? srcHeight : height);

    if (!transient) {
        RecordFailure(Result::OUT_OF_MEMORY);

        return;
    }

    MTL4::RenderPassDescriptor* pass = MTL4::RenderPassDescriptor::alloc()->init();
    auto* attachment = pass->colorAttachments()->object(0);
    attachment->setLoadAction(MTL::LoadActionDontCare);

    if (isNative) {
        attachment->setLoadAction(MTL::LoadActionLoad);
        attachment->setTexture(src);
        attachment->setLevel(srcRegion.mipOffset);
        attachment->setSlice(srcRegion.layerOffset);
        attachment->setStoreAction(MTL::StoreActionStoreAndMultisampleResolve);
        attachment->setResolveTexture(transient);
    } else {
        attachment->setTexture(transient);
        attachment->setStoreAction(MTL::StoreActionStore);

        if (attachmentResolve) {
            attachment->setLevel(dstRegion.mipOffset);
            attachment->setSlice(dstRegion.layerOffset);
        }
    }
    MTL4::RenderCommandEncoder* encoder = BeginRenderEncoder(pass);

    if (attachmentResolve)
        encoder->barrierAfterQueueStages(MTL::StageFragment, MTL::StageFragment, MTL4::VisibilityOptionDevice);

    if (!isNative) {
        const bool isArray = src->textureType() == MTL::TextureType2DMultisampleArray;
        MTL::RenderPipelineState* pipeline = m_Device.GetInternalShaders().GetResolvePipeline(src->pixelFormat(), GetColorType(format), isArray);

        if (!pipeline) {
            encoder->endEncoding();
            encoder->release();
            pass->release();
            RecordFailure(Result::FAILURE);

            return;
        }

        struct Constants {
            uint32_t origin[2];
            uint32_t layer;
            uint32_t samples;
            uint32_t op;
        } constants = {{srcRegion.x, srcRegion.y}, srcRegion.layerOffset, op == ResolveOp::SAMPLE_ZERO ? 1u : (uint32_t)src->sampleCount(), (uint32_t)op}; // "SAMPLE_ZERO": only the sample 0 is read

        m_InternalArguments->setAddress(Upload(&constants, sizeof(constants)), INTERNAL_SLOT_CONSTANTS);
        m_InternalArguments->setTexture(src->gpuResourceID(), INTERNAL_SLOT_TEXTURE);
        encoder->setArgumentTable(m_InternalArguments, MTL::RenderStageFragment);
        encoder->setRenderPipelineState(pipeline);
        encoder->setViewport(MTL::Viewport{0.0, 0.0, (double)width, (double)height, 0.0, 1.0});
        encoder->setScissorRect(MTL::ScissorRect{0, 0, width, height});
        encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, 0, 3);
    }
    encoder->endEncoding();
    encoder->release();
    pass->release();

    if (attachmentResolve)
        return;

    MTL4::ComputeCommandEncoder* blit = BeginCompute();
    blit->barrierAfterQueueStages(MTL::StageFragment, MTL::StageBlit, MTL4::VisibilityOptionDevice);
    const MTL::Origin transientOrigin = isNative ? MTL::Origin(srcRegion.x, srcRegion.y, 0) : MTL::Origin(0, 0, 0);
    blit->copyFromTexture(transient, 0, 0, transientOrigin, MTL::Size(width, height, 1), dst, dstRegion.layerOffset, dstRegion.mipOffset, MTL::Origin(dstRegion.x, dstRegion.y, dstRegion.z));
}

NRI_INLINE void CommandBufferMetal::CmdResolveTexture(Texture& dst, const TextureRegionDesc* dstRegion, const Texture& src, const TextureRegionDesc* srcRegion, ResolveOp op) {
    const TextureMetal& source = (const TextureMetal&)src;
    const TextureMetal& destination = (const TextureMetal&)dst;

    // Depth-stencil resolves are only supported for attachments (rejected by validation)
    NRI_CHECK(!GetFormatProps(source.GetDesc().format).isDepth && !GetFormatProps(source.GetDesc().format).isStencil, "Depth-stencil textures can't be resolved by 'CmdResolveTexture'");

    if (!dstRegion && !srcRegion && op == ResolveOp::AVERAGE && m_Device.IsNativeAverageResolveSupported(source.GetDesc().format) && (destination.GetNativeObject()->usage() & MTL::TextureUsageRenderTarget)) {
        MTL4::RenderPassDescriptor* pass = MTL4::RenderPassDescriptor::alloc()->init();

        for (uint32_t layer = 0; layer < source.GetDesc().layerNum; layer++) {
            auto* attachment = pass->colorAttachments()->object(0);
            attachment->setTexture(source.GetNativeObject());
            attachment->setSlice(layer);
            attachment->setLoadAction(MTL::LoadActionLoad);
            attachment->setStoreAction(MTL::StoreActionStoreAndMultisampleResolve);
            attachment->setResolveTexture(destination.GetNativeObject());
            attachment->setResolveSlice(layer);
            MTL4::RenderCommandEncoder* encoder = BeginRenderEncoder(pass);
            encoder->endEncoding();
            encoder->release();
        }
        pass->release();

        return;
    }

    if (!dstRegion && !srcRegion) {
        for (uint32_t layer = 0; layer < source.GetDesc().layerNum; layer++) {
            TextureRegionDesc region = {};
            region.layerOffset = layer;
            ResolveColor(destination.GetNativeObject(), region, source.GetNativeObject(), region, op, source.GetDesc().format);
        }

        return;
    }

    TextureRegionDesc sourceRegion = srcRegion ? *srcRegion : TextureRegionDesc{};
    TextureRegionDesc destinationRegion = dstRegion ? *dstRegion : TextureRegionDesc{};
    ResolveColor(destination.GetNativeObject(), destinationRegion, source.GetNativeObject(), sourceRegion, op, source.GetDesc().format);
}

NRI_INLINE void CommandBufferMetal::CmdClearStorage(const ClearStorageDesc& desc) {
    const DescriptorMetal& descriptor = *(const DescriptorMetal*)desc.descriptor;
    MTL::Texture* texture = descriptor.GetTexture();
    MTL::TextureType textureType = texture ? texture->textureType() : MTL::TextureTypeTextureBuffer;

    MTL::ComputePipelineState* pipeline = m_Device.GetInternalShaders().GetKernel(GetClearStorageKernel(texture, descriptor.GetFormat()));

    if (!pipeline) {
        RecordFailure(Result::FAILURE);

        return;
    }

    MTL::Size grid = {};

    if (texture) {
        grid = MTL::Size(texture->width(), texture->height(), texture->depth());

        if (textureType == MTL::TextureType1DArray)
            grid.height = texture->arrayLength();
        else if (textureType == MTL::TextureType2DArray)
            grid.depth = texture->arrayLength();

        // 3D: the first slice. Typed buffers: the first element, since the view texture starts at an aligned offset before the view
        const TextureViewDesc& view = descriptor.GetTextureViewDesc();
        uint32_t offset = 0;

        if (textureType == MTL::TextureType3D) {
            offset = view.sliceOffset;
            grid.depth = view.sliceNum == REMAINING ? grid.depth - offset : view.sliceNum;
        } else if (textureType == MTL::TextureTypeTextureBuffer) {
            const uint32_t stride = GetFormatProps(descriptor.GetFormat()).stride;
            offset = uint32_t((descriptor.GetBufferOffset() - texture->bufferOffset()) / stride);
            grid.width = descriptor.GetBufferSize() / stride;
        }

        m_InternalArguments->setTexture(texture->gpuResourceID(), INTERNAL_SLOT_TEXTURE);
        m_InternalArguments->setAddress(Upload(&offset, sizeof(offset)), INTERNAL_SLOT_CLEAR_OFFSET);
    } else {
        grid = MTL::Size(descriptor.GetBufferSize() / sizeof(uint32_t), 1, 1);
        m_InternalArguments->setAddress(descriptor.GetBuffer()->gpuAddress() + descriptor.GetBufferOffset(), INTERNAL_SLOT_CONSTANTS);
    }

    if (!grid.width || !grid.height || !grid.depth)
        return;

    m_InternalArguments->setAddress(Upload(&desc.value, sizeof(desc.value)), INTERNAL_SLOT_CLEAR_VALUE);
    SetComputeState(m_InternalArguments, pipeline);
    m_ComputeEncoder->dispatchThreads(grid, MTL::Size(8, std::min<NS::UInteger>(grid.height, 8), 1));
}

NRI_INLINE void CommandBufferMetal::CmdResetQueries(QueryPool& p, uint32_t o, uint32_t n) {
    QueryPoolMetal& q = (QueryPoolMetal&)p;

    // Only occlusion results accumulate, other queries are overwritten
    if (q.GetType() != QueryType::OCCLUSION)
        return;

    // Queries are implicitly ordered after resets (visibility results are written by fragment work of next render encoders)
    MTL4::ComputeCommandEncoder* encoder = BeginCompute();
    encoder->fillBuffer(q.GetVisibilityBuffer(), NS::Range(uint64_t(o) * 8, uint64_t(n) * 8), 0);
    encoder->barrierAfterStages(MTL::StageBlit, MTL::StageFragment, MTL4::VisibilityOptionDevice);
}

NRI_INLINE void CommandBufferMetal::CmdBeginQuery(QueryPool& p, uint32_t o) {
    QueryPoolMetal& q = (QueryPoolMetal&)p;
    MTL::Buffer* buffer = q.GetVisibilityBuffer();

    // The visibility buffer belongs to the pass. No split if nothing has been recorded yet or if the buffer has been attached upfront
    if (m_RenderPass->visibilityResultBuffer() != buffer) {
        if (!SuspendRendering())
            return;

        m_RenderPass->setVisibilityResultBuffer(buffer);
        m_RenderPass->setVisibilityResultType(MTL::VisibilityResultTypeAccumulate);
    }

    m_VisibilityBuffer = buffer;
    m_VisibilityMode = MTL::VisibilityResultModeCounting;
    m_VisibilityOffset = uint64_t(o) * 8;

    if (m_RenderEncoder)
        m_RenderEncoder->setVisibilityResultMode(m_VisibilityMode, m_VisibilityOffset);
}

NRI_INLINE void CommandBufferMetal::CmdEndQuery(QueryPool& p, uint32_t o) {
    QueryPoolMetal& q = (QueryPoolMetal&)p;

    if (q.GetCounterHeap()) {
        if (m_RenderPass)
            GetRenderEncoder()->writeTimestamp(MTL4::TimestampGranularityPrecise, MTL::RenderStageFragment, q.GetCounterHeap(), o);
        else
            BeginCompute()->writeTimestamp(MTL4::TimestampGranularityPrecise, q.GetCounterHeap(), o);
    } else {
        m_VisibilityMode = MTL::VisibilityResultModeDisabled;
        m_VisibilityOffset = 0;

        if (m_RenderEncoder)
            m_RenderEncoder->setVisibilityResultMode(MTL::VisibilityResultModeDisabled, 0);
    }
}

NRI_INLINE void CommandBufferMetal::CmdCopyQueries(const QueryPool& p, uint32_t o, uint32_t n, Buffer& d, uint64_t x) {
    const QueryPoolMetal& q = (const QueryPoolMetal&)p;
    auto* encoder = BeginCompute();

    // Query results are written by internal work without app barriers
    if (q.GetType() == QueryType::OCCLUSION)
        encoder->barrierAfterQueueStages(MTL::StageFragment, MTL::StageBlit, MTL4::VisibilityOptionDevice); // visibility results are written by previous render encoders
    else {
        encoder->barrierAfterQueueStages(MTL::StageFragment | MTL::StageDispatch | MTL::StageAccelerationStructure, MTL::StageBlit, MTL4::VisibilityOptionDevice);
        encoder->barrierAfterEncoderStages(MTL::StageDispatch | MTL::StageAccelerationStructure, MTL::StageBlit, MTL4::VisibilityOptionDevice);
    }

    if (q.GetCounterHeap()) {
        if (!m_CounterFence)
            m_CounterFence = m_Device.GetNativeObject()->newFence();

        if (!m_CounterFence) {
            RecordFailure(Result::OUT_OF_MEMORY);

            return;
        }

        // The resolve waits for the timestamps and gets ordered before the next encoder, i.e. before accesses to "dst" after a barrier
        encoder->updateFence(m_CounterFence, MTL::StageDispatch | MTL::StageBlit);
        EndCompute();
        m_CommandBuffer->resolveCounterHeap(q.GetCounterHeap(), NS::Range(o, n), MTL4::BufferRange(((BufferMetal&)d).GetGpuAddress() + x, uint64_t(n) * q.GetQuerySize()), m_CounterFence, m_CounterFence);
        m_IsCounterFenceWaitPending = true;
    } else
        encoder->copyFromBuffer(q.GetVisibilityBuffer(), uint64_t(o) * 8, ((BufferMetal&)d).GetNativeObject(), x, uint64_t(n) * 8);
}

NRI_INLINE void CommandBufferMetal::CmdBeginAnnotation(const char* name, uint32_t bgra) {
    MaybeUnused(bgra); // Metal debug groups have no color

    NS::String* string = NS::String::alloc()->init(name, NS::UTF8StringEncoding);

    // Inside rendering groups go to the render encoder, otherwise to the command buffer, since compute encoders are implicit.
    // The current compute encoder gets ended to make the group boundaries match encoder boundaries
    AnnotationLocation location = AnnotationLocation::RENDER_ENCODER;

    if (m_RenderEncoder)
        m_RenderEncoder->pushDebugGroup(string);
    else if (m_RenderPass)
        location = AnnotationLocation::SUSPENDED; // pushed when the render encoder opens
    else {
        EndCompute();
        m_CommandBuffer->pushDebugGroup(string);
        location = AnnotationLocation::COMMAND_BUFFER;
    }

    m_Annotations.push_back({string, location});
}

NRI_INLINE void CommandBufferMetal::CmdEndAnnotation() {
    NRI_CHECK(!m_Annotations.empty(), "Unbalanced 'CmdEndAnnotation'");

    if (m_Annotations.empty())
        return;

    const Annotation& annotation = m_Annotations.back();

    if (annotation.location == AnnotationLocation::RENDER_ENCODER)
        m_RenderEncoder->popDebugGroup();
    else if (annotation.location == AnnotationLocation::COMMAND_BUFFER) {
        EndCompute();
        m_CommandBuffer->popDebugGroup();
    }

    annotation.name->release();
    m_Annotations.pop_back();
}

NRI_INLINE void CommandBufferMetal::CmdAnnotation(const char* name, uint32_t bgra) {
    MaybeUnused(bgra); // Metal signposts have no color

    NS::String* string = NS::String::alloc()->init(name, NS::UTF8StringEncoding);

    if (m_RenderEncoder)
        m_RenderEncoder->insertDebugSignpost(string);
    else if (m_ComputeEncoder)
        m_ComputeEncoder->insertDebugSignpost(string);
    else {
        // Command buffers have no signposts
        m_CommandBuffer->pushDebugGroup(string);
        m_CommandBuffer->popDebugGroup();
    }

    string->release();
}

void CommandBufferMetal::BeginNativeEncoding() {
    NRI_CHECK(!m_RenderPass, "Native encoding is impossible inside rendering");

    // Pending barriers must precede native encoders
    if (m_PendingBefore)
        BeginCompute();

    EndCompute();
}

NRI_INLINE void CommandBufferMetal::SetDebugName(const char* name) {
    NS::String* string = NS::String::alloc()->init(name, NS::UTF8StringEncoding);
    m_CommandBuffer->setLabel(string);
    string->release();
}

MTL::ComputePipelineState* CommandBufferMetal::GetInternalKernel(InternalKernelMetal kernel) {
    MTL::ComputePipelineState* pipeline = m_Device.GetInternalShaders().GetKernel(kernel);

    if (!pipeline)
        RecordFailure(Result::FAILURE);

    return pipeline;
}

NRI_INLINE void CommandBufferMetal::CmdBuildBottomLevelAccelerationStructures(const BuildBottomLevelAccelerationStructureDesc* descs, uint32_t num) {
    auto* encoder = BeginCompute();

    for (uint32_t i = 0; i < num; i++) {
        const auto& desc = descs[i];
        auto& dst = *(AccelerationStructureMetal*)desc.dst;
        auto* descriptor = dst.CreateBuildDescriptor(desc.geometries, desc.geometryNum);
        MTL4::BufferRange scratch(((BufferMetal*)desc.scratchBuffer)->GetGpuAddress() + desc.scratchOffset, desc.src ? dst.GetUpdateScratchBufferSize() : dst.GetBuildScratchBufferSize());

        if (desc.src)
            encoder->refitAccelerationStructure(((const AccelerationStructureMetal*)desc.src)->GetNativeObject(), descriptor, dst.GetNativeObject(), scratch);
        else
            encoder->buildAccelerationStructure(dst.GetNativeObject(), descriptor, scratch);

        descriptor->release();
    }
}

NRI_INLINE void CommandBufferMetal::CmdBuildTopLevelAccelerationStructures(const BuildTopLevelAccelerationStructureDesc* descs, uint32_t num) {
    MTL::ComputePipelineState* convertInstances = GetInternalKernel(InternalKernelMetal::CONVERT_INSTANCES);

    if (!convertInstances)
        return;

    // Convert NRI instances to Metal instances and write the TLAS header with instance contributions.
    // These dispatches are part of the "ACCELERATION_STRUCTURE" stage from the NRI perspective
    Scratch<MTL::GPUAddress> instances = NRI_ALLOCATE_SCRATCH(m_Device, MTL::GPUAddress, num);
    SetComputeState(m_InternalArguments, convertInstances);
    MTL4::ComputeCommandEncoder* encoder = m_ComputeEncoder;

    for (uint32_t i = 0; i < num; i++) {
        const auto& desc = descs[i];
        const auto& dst = *(const AccelerationStructureMetal*)desc.dst;
        const uint32_t threadNum = std::max(desc.instanceNum, 1u); // thread 0 writes the header
        instances[i] = Upload(nullptr, threadNum * sizeof(MTL::IndirectAccelerationStructureInstanceDescriptor));

        if (!instances[i])
            return;

        ConvertInstancesArgsMetal args = {};
        args.src = desc.instanceNum ? ((const BufferMetal*)desc.instanceBuffer)->GetGpuAddress() + desc.instanceOffset : 0;
        args.dst = instances[i];
        args.header = dst.GetShaderBindingHeaderAddress();
        args.accelerationStructure = dst.GetHandle();
        args.instanceNum = desc.instanceNum;

        m_InternalArguments->setAddress(Upload(&args, sizeof(args)), INTERNAL_SLOT_CONSTANTS);
        encoder->dispatchThreads(MTL::Size(threadNum, 1, 1), MTL::Size(std::min(threadNum, 64u), 1, 1));
    }

    encoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageAccelerationStructure, MTL4::VisibilityOptionDevice);

    for (uint32_t i = 0; i < num; i++) {
        const auto& desc = descs[i];
        auto& dst = *(AccelerationStructureMetal*)desc.dst;
        auto* descriptor = dst.CreateBuildDescriptor(nullptr, 0, instances[i], desc.instanceNum);
        MTL4::BufferRange scratch(((BufferMetal*)desc.scratchBuffer)->GetGpuAddress() + desc.scratchOffset, desc.src ? dst.GetUpdateScratchBufferSize() : dst.GetBuildScratchBufferSize());

        if (desc.src)
            encoder->refitAccelerationStructure(((const AccelerationStructureMetal*)desc.src)->GetNativeObject(), descriptor, dst.GetNativeObject(), scratch);
        else
            encoder->buildAccelerationStructure(dst.GetNativeObject(), descriptor, scratch);

        descriptor->release();
    }
}

NRI_INLINE void CommandBufferMetal::CmdCopyAccelerationStructure(AccelerationStructure& dst, const AccelerationStructure& src, CopyMode mode) {
    auto& destination = (AccelerationStructureMetal&)dst;
    const auto& source = (const AccelerationStructureMetal&)src;

    MTL::ComputePipelineState* copyHeader = source.GetShaderBindingHeaderBuffer() ? GetInternalKernel(InternalKernelMetal::COPY_TOP_LEVEL_HEADER) : nullptr;

    if (source.GetShaderBindingHeaderBuffer() && !copyHeader)
        return;

    auto* encoder = BeginCompute();

    if (mode == CopyMode::COMPACT)
        encoder->copyAndCompactAccelerationStructure(source.GetNativeObject(), destination.GetNativeObject());
    else
        encoder->copyAccelerationStructure(source.GetNativeObject(), destination.GetNativeObject());

    // TLAS: write the destination header and copy instance contributions (clamped to the smaller TLAS)
    if (copyHeader) {
        CopyTopLevelHeaderArgsMetal args = {};
        args.srcContributions = source.GetInstanceContributionAddress();
        args.dstHeader = destination.GetShaderBindingHeaderAddress();
        args.dstAccelerationStructure = destination.GetHandle();
        args.num = std::min(source.GetInstanceNum(), destination.GetInstanceNum());

        const uint32_t threadNum = std::max(args.num, 1u); // thread 0 writes the header
        m_InternalArguments->setAddress(Upload(&args, sizeof(args)), INTERNAL_SLOT_CONSTANTS);
        SetComputeState(m_InternalArguments, copyHeader);
        encoder->dispatchThreads(MTL::Size(threadNum, 1, 1), MTL::Size(std::min(threadNum, 64u), 1, 1));
    }
}

NRI_INLINE void CommandBufferMetal::CmdWriteAccelerationStructureSizes(const AccelerationStructure* const* structures, uint32_t num, QueryPool& pool, uint32_t offset) {
    auto& queries = (QueryPoolMetal&)pool;
    const MTL::GPUAddress dst = queries.GetVisibilityBuffer()->gpuAddress() + uint64_t(offset) * sizeof(uint64_t);

    // "writeCompactedAccelerationStructureSize" writes a 64-bit value
    if (queries.GetType() == QueryType::ACCELERATION_STRUCTURE_COMPACTED_SIZE) {
        auto* encoder = BeginCompute();

        for (uint32_t i = 0; i < num; i++) {
            const auto& structure = *(const AccelerationStructureMetal*)structures[i];
            encoder->writeCompactedAccelerationStructureSize(structure.GetNativeObject(), MTL4::BufferRange(dst + i * sizeof(uint64_t), sizeof(uint64_t)));
        }

        return;
    }

    // Sizes are known on the host, but the query storage is private
    MTL::ComputePipelineState* copyWords = GetInternalKernel(InternalKernelMetal::COPY_WORDS);

    if (!copyWords)
        return;

    Scratch<uint64_t> sizes = NRI_ALLOCATE_SCRATCH(m_Device, uint64_t, num);

    for (uint32_t i = 0; i < num; i++)
        sizes[i] = ((const AccelerationStructureMetal*)structures[i])->GetSize();

    CopyWordsArgsMetal args = {};
    args.src = Upload(sizes, num * sizeof(uint64_t));
    args.dst = dst;

    if (!args.src)
        return;

    const uint32_t threadNum = num * 2;
    auto* encoder = BeginCompute();
    m_InternalArguments->setAddress(Upload(&args, sizeof(args)), INTERNAL_SLOT_CONSTANTS);
    SetComputeState(m_InternalArguments, copyWords);
    encoder->dispatchThreads(MTL::Size(threadNum, 1, 1), MTL::Size(std::min(threadNum, 64u), 1, 1));
}

#if NRI_ENABLE_METAL_SHADER_CONVERTER

MTL::GPUAddress CommandBufferMetal::SetRayDispatchArguments(const IRDispatchRaysDescriptor& desc) {
    IRDispatchRaysArgument args = {};
    args.DispatchRaysDesc = desc;
    args.GRS = m_Compute.root.empty() ? 0 : Upload(m_Compute.root.data(), m_Compute.root.size());
    args.ResDescHeap = m_DescriptorPool ? m_DescriptorPool->GetResourceHeapAddress() : 0;
    args.SmpDescHeap = m_DescriptorPool ? m_DescriptorPool->GetSamplerHeapAddress() : 0;
    args.VisibleFunctionTable = m_Pipeline->GetVisibleFunctionTableResourceID();
    args.IntersectionFunctionTable = m_Pipeline->GetIntersectionFunctionTableResourceID();

    const MTL::GPUAddress address = Upload(&args, sizeof(args));
    SetArgumentAddress(kIRRayDispatchArgumentsBindPoint, address);

    return address;
}

NRI_INLINE void CommandBufferMetal::CmdDispatchRays(const DispatchRaysDesc& desc) {
    auto getAddress = [](const StridedBufferRegion& region) -> uint64_t {
        return region.buffer ? ((const BufferMetal*)region.buffer)->GetGpuAddress() + region.offset : 0;
    };

    auto getRange = [&](const StridedBufferRegion& region) -> IRVirtualAddressRangeAndStride {
        IRVirtualAddressRangeAndStride range = {};
        range.StartAddress = getAddress(region);
        range.SizeInBytes = region.size;
        range.StrideInBytes = region.stride;

        return range;
    };

    IRDispatchRaysDescriptor dispatch = {};
    dispatch.RayGenerationShaderRecord.StartAddress = getAddress(desc.raygenShaderRecord);
    dispatch.RayGenerationShaderRecord.SizeInBytes = desc.raygenShaderRecord.size;
    dispatch.MissShaderTable = getRange(desc.missShaderBindingTable);
    dispatch.HitGroupTable = getRange(desc.hitShaderBindingTable);
    dispatch.CallableShaderTable = getRange(desc.callableShaderBindingTable);
    dispatch.Width = desc.width;
    dispatch.Height = desc.height;
    dispatch.Depth = desc.depth;

    SetRayDispatchArguments(dispatch);

    SetComputeState(m_Arguments, m_Pipeline->GetComputePipeline());
    m_ComputeEncoder->dispatchThreads(MTL::Size(desc.width, desc.height, desc.depth), MTL::Size(RAY_DISPATCH_GROUP_SIZE, RAY_DISPATCH_GROUP_SIZE, 1));
}

NRI_INLINE void CommandBufferMetal::CmdDispatchRaysIndirect(const Buffer& buffer, uint64_t offset) {
    MTL::ComputePipelineState* prepareRays = GetInternalKernel(InternalKernelMetal::PREPARE_RAYS_INDIRECT);

    if (!prepareRays)
        return;

    // Copy "DispatchRaysIndirectDesc" into the ray dispatch arguments and produce "MTLDispatchThreadsIndirectArguments"
    PrepareRaysIndirectArgsMetal args = {};
    args.src = ((const BufferMetal&)buffer).GetGpuAddress() + offset;
    args.dst = SetRayDispatchArguments({});
    args.dispatch = Upload(nullptr, sizeof(MTL::DispatchThreadsIndirectArguments));

    if (!args.dst || !args.dispatch)
        return;

    auto* encoder = BeginCompute();
    m_InternalArguments->setAddress(Upload(&args, sizeof(args)), INTERNAL_SLOT_CONSTANTS);
    SetComputeState(m_InternalArguments, prepareRays);
    encoder->dispatchThreads(MTL::Size(1, 1, 1), MTL::Size(1, 1, 1));
    encoder->barrierAfterEncoderStages(MTL::StageDispatch, MTL::StageDispatch, MTL4::VisibilityOptionDevice);

    SetComputeState(m_Arguments, m_Pipeline->GetComputePipeline());
    encoder->dispatchThreads(args.dispatch);
}

#else

NRI_INLINE void CommandBufferMetal::CmdDispatchRays(const DispatchRaysDesc&) {
}

NRI_INLINE void CommandBufferMetal::CmdDispatchRaysIndirect(const Buffer&, uint64_t) {
}

#endif
