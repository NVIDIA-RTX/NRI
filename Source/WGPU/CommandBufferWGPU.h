// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

constexpr uint32_t COLOR_ATTACHMENT_MAX_NUM_WGPU = 8;

struct ClearPipelineWGPU {
    std::array<Format, COLOR_ATTACHMENT_MAX_NUM_WGPU> colorFormats = {};
    Format depthStencilFormat = Format::UNKNOWN;
    uint32_t colorNum = 0;
    uint32_t colorAttachmentIndex = 0;
    PlaneBits planes = PlaneBits::NONE;
    Sample_t sampleNum = 1;
    WGPUPipelineLayout layout = nullptr;
    WGPURenderPipeline pipeline = nullptr;
};

struct ClearStorageBufferPipelineWGPU {
    WGPUBindGroupLayout bindGroupLayout = nullptr;
    WGPUPipelineLayout pipelineLayout = nullptr;
    WGPUComputePipeline pipeline = nullptr;
};

struct ClearStorageTexturePipelineWGPU {
    Format format = Format::UNKNOWN;
    WGPUTextureViewDimension dimension = WGPUTextureViewDimension_Undefined;
    WGPUBindGroupLayout bindGroupLayout = nullptr;
    WGPUPipelineLayout pipelineLayout = nullptr;
    WGPUComputePipeline pipeline = nullptr;
};

struct RootConstantStateWGPU {
    inline RootConstantStateWGPU(const StdAllocator<uint8_t>& allocator)
        : data(allocator)
        , mask(allocator) {
    }

    Vector<uint8_t> data;
    Vector<uint8_t> mask;
};

struct RootDescriptorBindingWGPU {
    const DescriptorWGPU* descriptor = nullptr;
    uint64_t offset = 0;
};

enum class AnnotationScopeWGPU : uint8_t {
    COMMAND_ENCODER,
    RENDER_PASS,
    COMPUTE_PASS
};

struct CommandBufferWGPU final : public DebugNameBase {
    inline CommandBufferWGPU(DeviceWGPU& device)
        : m_Device(device)
        , m_GraphicsDescriptorSets(device.GetStdAllocator())
        , m_ComputeDescriptorSets(device.GetStdAllocator())
        , m_GraphicsDescriptorSetDirty(device.GetStdAllocator())
        , m_ComputeDescriptorSetDirty(device.GetStdAllocator())
        , m_ClearPipelines(device.GetStdAllocator())
        , m_ClearStorageTexturePipelines(device.GetStdAllocator())
        , m_RootDescriptorBindings(device.GetStdAllocator())
        , m_RootDynamicOffsets(device.GetStdAllocator())
        , m_GraphicsRootConstants(device.GetStdAllocator())
        , m_ComputeRootConstants(device.GetStdAllocator())
        , m_AnnotationScopes(device.GetStdAllocator())
        , m_TemporaryBuffers(device.GetStdAllocator()) {
    }

    ~CommandBufferWGPU();

    inline WGPUCommandBuffer GetCommandBuffer() const {
        return m_CommandBuffer;
    }

    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    Result Create(const CommandAllocator& commandAllocator);
    Result Begin(const DescriptorPool* descriptorPool);
    Result End();

    void SetViewports(const Viewport* viewports, uint32_t viewportNum);
    void SetScissors(const Rect* rects, uint32_t rectNum);
    void SetPipelineLayout(BindPoint bindPoint, const PipelineLayout& pipelineLayout);
    void SetPipeline(const Pipeline& pipeline);
    void SetDescriptorSet(const SetDescriptorSetDesc& setDescriptorSetDesc);
    void SetRootConstants(const SetRootConstantsDesc& setRootConstantsDesc);
    void SetRootDescriptor(const SetRootDescriptorDesc& setRootDescriptorDesc);
    void SetVertexBuffers(uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum);
    void SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType);
    void SetStencilReference(uint8_t frontRef, uint8_t backRef);
    void SetBlendConstants(const Color32f& color);
    void BeginRendering(const RenderingDesc& renderingDesc);
    void EndRendering();
    void ClearAttachments(const ClearAttachmentDesc* clearAttachmentDescs, uint32_t clearAttachmentDescNum, const Rect* rects, uint32_t rectNum);
    void Draw(const DrawDesc& drawDesc);
    void DrawIndexed(const DrawIndexedDesc& drawIndexedDesc);
    void DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset);
    void DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset);
    void Dispatch(const DispatchDesc& dispatchDesc);
    void DispatchIndirect(const Buffer& buffer, uint64_t offset);
    void CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size);
    void CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion);
    void UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegion, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayout);
    void ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayout, const Texture& srcTexture, const TextureRegionDesc& srcRegion);
    void ZeroBuffer(Buffer& buffer, uint64_t offset, uint64_t size);
    void ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion, ResolveOp resolveOp);
    void ClearStorage(const ClearStorageDesc& clearStorageDesc);
    void Barrier(const BarrierDesc& barrierDesc);
    void ResetQueries(QueryPool& queryPool, uint32_t offset, uint32_t num);
    void BeginQuery(QueryPool& queryPool, uint32_t offset);
    void EndQuery(QueryPool& queryPool, uint32_t offset);
    void CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset);
    void BeginAnnotation(const char* name, uint32_t bgra);
    void EndAnnotation();
    void Annotation(const char* name, uint32_t bgra);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        MaybeUnused(name);
    }

private:
    void EndPass();
    bool SetScissorRect(const Rect& rect);
    void BindDescriptorSets(BindPoint bindPoint);
    void ReleaseTransientObjects();
    void ReleaseRootBindGroups();
    void MarkDescriptorSetDirty(BindPoint bindPoint, uint32_t bindGroupIndex);
    void MarkDescriptorSetsDirty(BindPoint bindPoint);
    void BindRootGroup(BindPoint bindPoint);
    void RestoreRootConstants(BindPoint bindPoint);
    void ReleaseRenderPassTransientObjects();
    void PopPassAnnotations(AnnotationScopeWGPU scope);
    void FlushDeferredEncoderAnnotationPops();
    WGPUBindGroup CreateRootBindGroup(BindPoint bindPoint);
    RootConstantStateWGPU& GetRootConstantState(BindPoint bindPoint);
    WGPURenderPipeline GetClearPipeline(uint32_t colorAttachmentIndex, PlaneBits planes, WGPUPipelineLayout& pipelineLayout);
    WGPUComputePipeline GetClearStorageBufferPipeline(WGPUBindGroupLayout& bindGroupLayout);
    WGPUComputePipeline GetClearStorageTexturePipeline(Format format, WGPUTextureViewDimension dimension, WGPUBindGroupLayout& bindGroupLayout);

private:
    DeviceWGPU& m_Device;
    Vector<const DescriptorSetWGPU*> m_GraphicsDescriptorSets;
    Vector<const DescriptorSetWGPU*> m_ComputeDescriptorSets;
    Vector<uint8_t> m_GraphicsDescriptorSetDirty;
    Vector<uint8_t> m_ComputeDescriptorSetDirty;
    Vector<ClearPipelineWGPU> m_ClearPipelines;
    Vector<ClearStorageTexturePipelineWGPU> m_ClearStorageTexturePipelines;
    Vector<RootDescriptorBindingWGPU> m_RootDescriptorBindings;
    Vector<uint32_t> m_RootDynamicOffsets;
    RootConstantStateWGPU m_GraphicsRootConstants;
    RootConstantStateWGPU m_ComputeRootConstants;
    Vector<AnnotationScopeWGPU> m_AnnotationScopes;
    Vector<WGPUBuffer> m_TemporaryBuffers;
    WGPUCommandEncoder m_CommandEncoder = nullptr;
    WGPUCommandBuffer m_CommandBuffer = nullptr;
    WGPURenderPassEncoder m_RenderPass = nullptr;
    WGPUComputePassEncoder m_ComputePass = nullptr;
    WGPUTextureView m_RenderDepthStencilView = nullptr;
    ClearStorageBufferPipelineWGPU m_ClearStorageBufferPipeline = {};
    WGPUBindGroup m_GraphicsRootBindGroup = nullptr;
    WGPUBindGroup m_ComputeRootBindGroup = nullptr;
    const PipelineLayoutWGPU* m_PipelineLayout = nullptr;
    WGPURenderPipeline m_RenderPipeline = nullptr;
    WGPUComputePipeline m_ComputePipeline = nullptr;
    WGPUComputePipeline m_BoundComputePipeline = nullptr;
    BindPoint m_BindPoint = BindPoint::GRAPHICS;
    std::array<Format, COLOR_ATTACHMENT_MAX_NUM_WGPU> m_RenderColorFormats = {};
    Format m_RenderDepthStencilFormat = Format::UNKNOWN;
    uint32_t m_RenderColorNum = 0;
    Sample_t m_RenderSampleNum = 1;
    Dim_t m_RenderWidth = 0;
    Dim_t m_RenderHeight = 0;
    Viewport m_Viewport = {};
    Rect m_Scissor = {};
    uint8_t m_StencilReference = 0;
    uint32_t m_DeferredEncoderAnnotationPopNum = 0;
    uint32_t m_GraphicsDirtyDescriptorSetMin = uint32_t(-1);
    uint32_t m_GraphicsDirtyDescriptorSetMax = 0;
    uint32_t m_ComputeDirtyDescriptorSetMin = uint32_t(-1);
    uint32_t m_ComputeDirtyDescriptorSetMax = 0;
    bool m_GraphicsRootGroupDirty = true;
    bool m_ComputeRootGroupDirty = true;
    bool m_HasViewport = false;
    bool m_HasScissor = false;
};

} // namespace nri
