// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct AttachmentResolveMetal {
    MTL::Texture* src = nullptr;
    MTL::Texture* dst = nullptr;
    ResolveOp op = ResolveOp::AVERAGE;
    Format format = Format::UNKNOWN; // "src" view format
    uint16_t srcMip = 0, dstMip = 0;
    uint16_t srcLayer = 0, dstLayer = 0;
    uint16_t layerNum = 0;
};

struct CommandBufferMetal final : public DebugNameBase {
    inline CommandBufferMetal(DeviceMetal& device)
        : m_Device(device)
        , m_Annotations(device.GetStdAllocator()) {
    }

    ~CommandBufferMetal();
    Result Create(const CommandAllocator& allocator);
    Result Begin(const DescriptorPool* descriptorPool);
    Result End();

    inline DeviceMetal& GetDevice() const {
        return m_Device;
    }

    inline MTL4::CommandBuffer* GetNativeObject() const {
        return m_CommandBuffer;
    }

    inline void ReleaseOnReset(NS::Object* object) {
        m_Allocator->ReleaseOnReset(object);
    }

    static CommandBufferMetal& FromNativeObject(NS::Object* commandBuffer);
    void BeginNativeEncoding();
    void RecordFailure(Result result);
    void CmdSetDescriptorPool(const DescriptorPool& descriptorPool);
    void CmdSetPipelineLayout(BindPoint bindPoint, const PipelineLayout& pipelineLayout);
    void CmdSetDescriptorSet(const SetDescriptorSetDesc& desc);
    void CmdSetRootConstants(const SetRootConstantsDesc& desc);
    void CmdSetRootDescriptor(const SetRootDescriptorDesc& desc);
    void CmdSetPipeline(const Pipeline& pipeline);
    void CmdBarrier(const BarrierDesc& desc);
    void CmdSetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType);
    void CmdSetVertexBuffers(uint32_t baseSlot, const VertexBufferDesc* descs, uint32_t num);
    void CmdSetViewports(const Viewport* viewports, uint32_t num);
    void CmdSetScissors(const Rect* rects, uint32_t num);
    void CmdSetStencilReference(uint8_t front, uint8_t back);
    void CmdSetDepthBounds(float min, float max);
    void CmdSetBlendConstants(const Color32f& color);
    void CmdSetSampleLocations(const SampleLocation*, Sample_t, Sample_t);
    void CmdSetShadingRate(const ShadingRateDesc&);
    void CmdSetDepthBias(const DepthBiasDesc& desc);
    void CmdBeginRendering(const RenderingDesc& desc);
    void CmdClearAttachments(const ClearAttachmentDesc*, uint32_t, const Rect*, uint32_t);
    void CmdDraw(const DrawDesc& desc);
    void CmdDrawIndexed(const DrawIndexedDesc& desc);
    void CmdDrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countOffset);
    void CmdDrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countOffset);
    void CmdDrawMeshTasks(const DrawMeshTasksDesc& desc);
    void CmdDrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countOffset);
    void CmdBuildTopLevelAccelerationStructures(const BuildTopLevelAccelerationStructureDesc* descs, uint32_t num);
    void CmdBuildBottomLevelAccelerationStructures(const BuildBottomLevelAccelerationStructureDesc* descs, uint32_t num);
    void CmdCopyAccelerationStructure(AccelerationStructure& dst, const AccelerationStructure& src, CopyMode mode);
    void CmdWriteAccelerationStructureSizes(const AccelerationStructure* const* structures, uint32_t num, QueryPool& pool, uint32_t offset);
    void CmdDispatchRays(const DispatchRaysDesc& desc);
    void CmdDispatchRaysIndirect(const Buffer& buffer, uint64_t offset);
    void CmdEndRendering();
    void CmdDispatch(const DispatchDesc& desc);
    void CmdDispatchIndirect(const Buffer& buffer, uint64_t offset);
    void CmdCopyBuffer(Buffer& dst, uint64_t dstOffset, const Buffer& src, uint64_t srcOffset, uint64_t size);
    void CmdCopyTexture(Texture& dst, const TextureRegionDesc* dstRegion, const Texture& src, const TextureRegionDesc* srcRegion);
    void CmdUploadBufferToTexture(Texture& dst, const TextureRegionDesc& region, const Buffer& src, const TextureDataLayoutDesc& layout);
    void CmdReadbackTextureToBuffer(Buffer& dst, const TextureDataLayoutDesc& layout, const Texture& src, const TextureRegionDesc& region);
    void CmdZeroBuffer(Buffer& buffer, uint64_t offset, uint64_t size);
    void CmdResolveTexture(Texture&, const TextureRegionDesc*, const Texture&, const TextureRegionDesc*, ResolveOp);
    void CmdClearStorage(const ClearStorageDesc&);
    void CmdResetQueries(QueryPool& pool, uint32_t offset, uint32_t num);
    void CmdBeginQuery(QueryPool& pool, uint32_t offset);
    void CmdEndQuery(QueryPool& pool, uint32_t offset);
    void CmdCopyQueries(const QueryPool& pool, uint32_t offset, uint32_t num, Buffer& dst, uint64_t dstOffset);
    void CmdBeginAnnotation(const char* name, uint32_t);
    void CmdEndAnnotation();
    void CmdAnnotation(const char* name, uint32_t);
    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    struct State {
        const PipelineLayoutMetal* layout = nullptr;
        Vector<uint8_t> root;
        MTL::GPUAddress rootAddress = 0; // uploaded "root", "0" if changed since the last upload

        State(StdAllocator<uint8_t>& a) : root(a) {
        }
    };

    enum class AnnotationLocation : uint8_t {
        COMMAND_BUFFER,
        RENDER_ENCODER,
        SUSPENDED, // the render encoder has been split, the group gets re-pushed on resume
        CLOSED     // the render encoder has ended, the group has already been popped
    };

    struct Annotation {
        NS::String* name;
        AnnotationLocation location;
    };

    MTL4::ComputeCommandEncoder* BeginCompute();
    void EndCompute();
    MTL4::RenderCommandEncoder* BeginRenderEncoder(MTL4::RenderPassDescriptor* pass);
    MTL4::RenderCommandEncoder* GetRenderEncoder(); // lazily (re)opens the render encoder of the current pass
    void EndRenderEncoder(bool isSuspended);
    bool SuspendRendering();
    void FlushBarriers(MTL4::CommandEncoder* encoder, MTL::Stages stages);
    void SetArgumentAddress(uint32_t slot, MTL::GPUAddress address);
    void SetComputeState(MTL4::ArgumentTable* arguments, MTL::ComputePipelineState* pipeline);
    bool DispatchInternal(InternalKernelMetal kernel, const void* constants, uint32_t constantsSize, uint32_t threadNum);
    MTL::GPUAddress PrepareIndirectArguments(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t& stride, uint32_t argumentSize, const Buffer* countBuffer, uint64_t countOffset);
    void BindArguments(BindPoint bindPoint);
    State& GetState(BindPoint bindPoint);
    void ApplyRasterState();
    void SetDrawArguments(const void* data, uint64_t size, bool indexed);
    bool PrepareIndirectDrawRoots(MTL::GPUAddress arguments, uint32_t drawNum, uint32_t stride, MTL::GPUAddress& roots); // "roots = 0" if not needed
    void DrawIndirect(MTL::GPUAddress arguments, MTL::GPUAddress roots, uint32_t drawNum, uint32_t stride, bool indexed);
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    IRRuntimeDrawInfo PrepareEmulationDraw(bool indexed, MTL::Size& objectThreads, MTL::Size& meshThreads);
    void DrawEmulated(const void* arguments, uint64_t size, bool indexed, uint32_t vertexNum, uint32_t instanceNum);
    void DrawEmulatedIndirect(MTL::GPUAddress arguments, MTL::GPUAddress roots, uint32_t drawNum, uint32_t stride, bool indexed);
#endif
    MTL::ComputePipelineState* GetInternalKernel(InternalKernelMetal kernel); // records a failure if unavailable
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    MTL::GPUAddress SetRayDispatchArguments(const IRDispatchRaysDescriptor& desc);
#endif
    void ResolveColor(MTL::Texture* dst, const TextureRegionDesc& dstRegion, MTL::Texture* src, const TextureRegionDesc& srcRegion, ResolveOp op, Format format, bool attachmentResolve = false);
    static MTL::Size GetRegionSize(const TextureMetal& texture, const TextureRegionDesc& region);

    DeviceMetal& m_Device;
    CommandAllocatorMetal* m_Allocator = nullptr;
    MTL4::CommandBuffer* m_CommandBuffer = nullptr;
    MTL4::ArgumentTable* m_Arguments = nullptr;
    MTL4::ArgumentTable* m_InternalArguments = nullptr;
    MTL4::ComputeCommandEncoder* m_ComputeEncoder = nullptr;
    MTL4::RenderCommandEncoder* m_RenderEncoder = nullptr;
    MTL4::RenderPassDescriptor* m_RenderPass = nullptr; // not "nullptr" inside rendering, even if the render encoder is not open
    MTL::Fence* m_CounterFence = nullptr;
    DescriptorPoolMetal* m_DescriptorPool = nullptr;
    const PipelineMetal* m_Pipeline = nullptr;
    bool m_RenderPipelineDirty = true;
    State m_Graphics{m_Device.GetStdAllocator()}, m_Compute{m_Device.GetStdAllocator()};
    BindPoint m_BindPoint = BindPoint::GRAPHICS;

    // Bound state (redundant calls are skipped). Argument tables are snapshotted at draw/dispatch time, so a table is set once per encoder
    MTL::GPUAddress m_ArgumentAddresses[6] = {}; // "m_Arguments" slots [0; 5]
    bool m_IsRenderTableBound = false;
    bool m_AreHeapsDirty = true;
    MTL4::ArgumentTable* m_ComputeTable = nullptr;
    MTL::ComputePipelineState* m_ComputePipeline = nullptr;
    uint8_t m_DrawArguments[sizeof(DrawIndexedDesc)] = {}; // last uploaded draw arguments
    uint32_t m_DrawArgumentsSize = 0;
    MTL::GPUAddress m_DrawArgumentsAddress = 0;

    // Barriers recorded without an open encoder are emitted at the beginning of the next encoder
    MTL::Stages m_PendingBefore = 0, m_PendingAfter = 0;
    MTL4::VisibilityOptions m_PendingVisibility = MTL4::VisibilityOptionNone;
    MTL::Stages m_ResumeStages = 0; // work between render encoders of a pass, which the next render encoder must wait for
    bool m_IsCounterFenceWaitPending = false;

    MTL::Viewport m_Viewports[16] = {};
    MTL::ScissorRect m_Scissors[16] = {};
    uint32_t m_ViewportNum = 0, m_ScissorNum = 0;
    uint8_t m_FrontStencil = 0, m_BackStencil = 0;
    Color32f m_BlendColor = {};
    DepthBiasDesc m_DepthBias = {};
    bool m_HasDepthBias = false;
    float m_DepthMin = 0.0f, m_DepthMax = 1.0f;
    MTL::SamplePosition m_SamplePositions[16] = {};
    uint8_t m_SamplePositionNum = 0;
    MTL::GPUAddress m_IndexAddress = 0;
    MTL::GPUAddress m_DrawRootAddress = 0;
    uint64_t m_IndexLength = 0;
    MTL::IndexType m_IndexType = MTL::IndexTypeUInt16;
#if NRI_ENABLE_METAL_SHADER_CONVERTER
    IRRuntimeVertexBuffers m_EmulationVertexBuffers = {};
#endif
    AttachmentResolveMetal m_AttachmentResolves[8] = {};
    uint8_t m_AttachmentResolveNum = 0;
    MTL::PixelFormat m_RenderColors[8] = {};
    Format m_RenderColorFormats[8] = {};
    MTL::StoreAction m_RenderStore[8] = {};
    MTL::StoreAction m_DepthStore = MTL::StoreActionDontCare, m_StencilStore = MTL::StoreActionDontCare;
    MTL::PixelFormat m_RenderDepth = MTL::PixelFormatInvalid, m_RenderStencil = MTL::PixelFormatInvalid;
    uint32_t m_RenderWidth = 0, m_RenderHeight = 0;
    uint32_t m_ViewMask = 0;
    uint8_t m_RenderColorNum = 0, m_RenderSampleNum = 1;
    bool m_HasMemorylessAttachment = false;
    MTL::VisibilityResultMode m_VisibilityMode = MTL::VisibilityResultModeDisabled;
    uint64_t m_VisibilityOffset = 0;
    MTL::Buffer* m_VisibilityBuffer = nullptr; // the last one in this recording, attached to new passes upfront
    Vector<Annotation> m_Annotations;
    Result m_Result = Result::SUCCESS;
};

} // namespace nri
