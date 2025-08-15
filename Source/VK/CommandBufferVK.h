// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineVK;
struct PipelineLayoutVK;
struct DescriptorVK;

struct CommandBufferVK final : public DebugNameBase {
    inline CommandBufferVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline operator VkCommandBuffer() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~CommandBufferVK();

    void Create(VkCommandPool commandPool, VkCommandBuffer commandBuffer, QueueType type);
    Result Create(const CommandBufferVKDesc& commandBufferVKDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result Begin(const DescriptorPool* descriptorPool);
    Result End();
    void SetPipeline(const Pipeline& pipeline);
    void SetPipelineLayout(BindPoint bindPoint, const PipelineLayout& pipelineLayout);
    void SetDescriptorSet(const SetDescriptorSetDesc& setDescriptorSetDesc);
    void SetRootConstants(const SetRootConstantsDesc& setRootConstantsDesc);
    void SetRootDescriptor(const SetRootDescriptorDesc& setRootDescriptorDesc);
    void Barrier(const BarrierDesc& barrierDesc);
    void BeginRendering(const AttachmentsDesc& attachmentsDesc);
    void EndRendering();
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
    void SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType);
    void SetVertexBuffers(uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum);
    void Draw(const DrawDesc& drawDesc);
    void DrawIndexed(const DrawIndexedDesc& drawIndexedDesc);
    void DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset);
    void DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset);
    void Dispatch(const DispatchDesc& dispatchDesc);
    void DispatchIndirect(const Buffer& buffer, uint64_t offset);
    void BeginQuery(QueryPool& queryPool, uint32_t offset);
    void EndQuery(QueryPool& queryPool, uint32_t offset);
    void BeginAnnotation(const char* name, uint32_t bgra);
    void EndAnnotation();
    void Annotation(const char* name, uint32_t bgra);
    void CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size);
    void CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion);
    void UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegion, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayout);
    void ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayout, const Texture& srcTexture, const TextureRegionDesc& srcRegion);
    void ZeroBuffer(Buffer& buffer, uint64_t offset, uint64_t size);
    void ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion);
    void CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset);
    void ResetQueries(QueryPool& queryPool, uint32_t offset, uint32_t num);
    void BuildTopLevelAccelerationStructures(const BuildTopLevelAccelerationStructureDesc* buildTopLevelAccelerationStructureDescs, uint32_t buildTopLevelAccelerationStructureDescNum);
    void BuildBottomLevelAccelerationStructures(const BuildBottomLevelAccelerationStructureDesc* buildBottomLevelAccelerationStructureDescs, uint32_t buildBottomLevelAccelerationStructureDescNum);
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
    DeviceVK& m_Device;
    const PipelineLayoutVK* m_PipelineLayout = nullptr;
    const DescriptorVK* m_DepthStencil = nullptr;
    VkCommandBuffer m_Handle = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    QueueType m_Type = (QueueType)0;
    uint32_t m_ViewMask = 0;
    BindPoint m_PipelineBindPoint = BindPoint::INHERIT;
    Dim_t m_RenderLayerNum = 0;
    Dim_t m_RenderWidth = 0;
    Dim_t m_RenderHeight = 0;
};

} // namespace nri
