// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorVal;
struct PipelineVal;
struct PipelineLayoutVal;

struct CommandBufferVal final : public ObjectVal {
    CommandBufferVal(DeviceVal& device, CommandBuffer* commandBuffer, bool isWrapped)
        : ObjectVal(device, commandBuffer)
        , m_IsRecordingStarted(isWrapped)
        , m_IsWrapped(isWrapped) {
    }

    inline CommandBuffer* GetImpl() const {
        return (CommandBuffer*)m_Impl;
    }

    inline void* GetNativeObject() const {
        return GetCoreInterfaceImpl().GetCommandBufferNativeObject(GetImpl());
    }

    inline void ResetAttachments() {
        m_RenderTargetNum = 0;
        for (size_t i = 0; i < m_RenderTargets.size(); i++)
            m_RenderTargets[i] = nullptr;

        m_DepthStencil = nullptr;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result Begin(const DescriptorPool* descriptorPool);
    Result End();
    void SetViewports(const Viewport* viewports, uint32_t viewportNum);
    void SetScissors(const Rect* rects, uint32_t rectNum);
    void SetDepthBounds(float boundsMin, float boundsMax);
    void SetStencilReference(uint8_t frontRef, uint8_t backRef);
    void SetSampleLocations(const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum);
    void SetBlendConstants(const Color32f& color);
    void SetShadingRate(const ShadingRateDesc& shadingRateDesc);
    void SetDepthBias(const DepthBiasDesc& depthBiasDesc);
    void ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum);
    void ClearStorage(const ClearStorageDesc& clearDesc);
    void BeginRendering(const AttachmentsDesc& attachmentsDesc);
    void EndRendering();
    void SetVertexBuffers(uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum);
    void SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType);
    void SetPipelineLayout(BindPoint bindPoint, const PipelineLayout& pipelineLayout);
    void SetPipeline(const Pipeline& pipeline);
    void SetDescriptorPool(const DescriptorPool& descriptorPool);
    void SetDescriptorSet(const SetDescriptorSetDesc& setDescriptorSetDesc);
    void SetRootConstants(const SetRootConstantsDesc& setRootConstantsDesc);
    void SetRootDescriptor(const SetRootDescriptorDesc& setRootDescriptorDesc);
    void Draw(const DrawDesc& drawDesc);
    void DrawIndexed(const DrawIndexedDesc& drawIndexedDesc);
    void DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset);
    void DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset);
    void CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size);
    void CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion);
    void UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegion, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayout);
    void ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayout, const Texture& srcTexture, const TextureRegionDesc& srcRegion);
    void ZeroBuffer(Buffer& buffer, uint64_t offset, uint64_t size);
    void ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion);
    void Dispatch(const DispatchDesc& dispatchDesc);
    void DispatchIndirect(const Buffer& buffer, uint64_t offset);
    void Barrier(const BarrierDesc& barrierDesc);
    void BeginQuery(QueryPool& queryPool, uint32_t offset);
    void EndQuery(QueryPool& queryPool, uint32_t offset);
    void CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset);
    void ResetQueries(QueryPool& queryPool, uint32_t offset, uint32_t num);
    void BeginAnnotation(const char* name, uint32_t bgra);
    void EndAnnotation();
    void Annotation(const char* name, uint32_t bgra);
    void BuildTopLevelAccelerationStructure(const BuildTopLevelAccelerationStructureDesc* buildTopLevelAccelerationStructureDescs, uint32_t buildTopLevelAccelerationStructureDescNum);
    void BuildBottomLevelAccelerationStructure(const BuildBottomLevelAccelerationStructureDesc* buildBottomLevelAccelerationStructureDescs, uint32_t buildBottomLevelAccelerationStructureDescNum);
    void BuildMicromaps(const BuildMicromapDesc* buildMicromapDescs, uint32_t buildMicromapDescNum);
    void CopyAccelerationStructure(AccelerationStructure& dst, const AccelerationStructure& src, CopyMode copyMode);
    void CopyMicromap(Micromap& dst, const Micromap& src, CopyMode copyMode);
    void WriteAccelerationStructuresSizes(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset);
    void WriteMicromapsSizes(const Micromap* const* micromaps, uint32_t micromapNum, QueryPool& queryPool, uint32_t queryPoolOffset);
    void DispatchRays(const DispatchRaysDesc& dispatchRaysDesc);
    void DispatchRaysIndirect(const Buffer& buffer, uint64_t offset);
    void DrawMeshTasks(const DrawMeshTasksDesc& drawMeshTasksDesc);
    void DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset);

private:
    void ValidateReadonlyDepthStencil();

    std::array<DescriptorVal*, 16> m_RenderTargets = {};
    DescriptorVal* m_DepthStencil = nullptr;
    PipelineLayoutVal* m_PipelineLayout = nullptr;
    PipelineVal* m_Pipeline = nullptr;
    uint32_t m_RenderTargetNum = 0;
    int32_t m_AnnotationStack = 0;
    bool m_IsRecordingStarted = false;
    bool m_IsWrapped = false;
    bool m_IsRenderPass = false;
};

} // namespace nri
