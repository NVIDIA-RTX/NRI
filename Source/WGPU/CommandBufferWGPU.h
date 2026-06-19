// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct ClearPipelineWGPU {
    Format format = Format::UNKNOWN;
    Format depthStencilFormat = Format::UNKNOWN;
    PlaneBits planes = PlaneBits::NONE;
    WGPUPipelineLayout layout = nullptr;
    WGPURenderPipeline pipeline = nullptr;
};

struct RootDescriptorBindingWGPU {
    const DescriptorWGPU* descriptor = nullptr;
    uint64_t offset = 0;
};

struct CommandBufferWGPU final : public DebugNameBase {
    inline CommandBufferWGPU(DeviceWGPU& device)
        : m_Device(device)
        , m_GraphicsDescriptorSets(device.GetStdAllocator())
        , m_ComputeDescriptorSets(device.GetStdAllocator())
        , m_GraphicsDescriptorSetDirty(device.GetStdAllocator())
        , m_ComputeDescriptorSetDirty(device.GetStdAllocator())
        , m_ClearPipelines(device.GetStdAllocator())
        , m_RootDescriptorBindings(device.GetStdAllocator())
        , m_RootDynamicOffsets(device.GetStdAllocator())
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
    void BindDescriptorSet(BindPoint bindPoint, uint32_t bindGroupIndex);
    void BindDescriptorSets(BindPoint bindPoint);
    void ReleaseTransientObjects();
    void ReleaseRootBindGroups();
    void MarkDescriptorSetsDirty(BindPoint bindPoint);
    void BindRootGroup(BindPoint bindPoint);
    WGPUBindGroup CreateRootBindGroup(BindPoint bindPoint);
    WGPURenderPipeline GetClearPipeline(Format format, Format depthStencilFormat, PlaneBits planes, WGPUPipelineLayout& pipelineLayout);

private:
    DeviceWGPU& m_Device;
    Vector<const DescriptorSetWGPU*> m_GraphicsDescriptorSets;
    Vector<const DescriptorSetWGPU*> m_ComputeDescriptorSets;
    Vector<uint8_t> m_GraphicsDescriptorSetDirty;
    Vector<uint8_t> m_ComputeDescriptorSetDirty;
    Vector<ClearPipelineWGPU> m_ClearPipelines;
    Vector<RootDescriptorBindingWGPU> m_RootDescriptorBindings;
    Vector<uint32_t> m_RootDynamicOffsets;
    Vector<WGPUBuffer> m_TemporaryBuffers;
    WGPUCommandEncoder m_CommandEncoder = nullptr;
    WGPUCommandBuffer m_CommandBuffer = nullptr;
    WGPURenderPassEncoder m_RenderPass = nullptr;
    WGPUComputePassEncoder m_ComputePass = nullptr;
    WGPUBindGroup m_GraphicsRootBindGroup = nullptr;
    WGPUBindGroup m_ComputeRootBindGroup = nullptr;
    const PipelineLayoutWGPU* m_PipelineLayout = nullptr;
    WGPURenderPipeline m_RenderPipeline = nullptr;
    WGPUComputePipeline m_ComputePipeline = nullptr;
    WGPUComputePipeline m_BoundComputePipeline = nullptr;
    BindPoint m_BindPoint = BindPoint::GRAPHICS;
    Format m_RenderFormat = Format::UNKNOWN;
    Format m_RenderDepthStencilFormat = Format::UNKNOWN;
    Dim_t m_RenderWidth = 0;
    Dim_t m_RenderHeight = 0;
    bool m_GraphicsRootGroupDirty = true;
    bool m_ComputeRootGroupDirty = true;
};

} // namespace nri
