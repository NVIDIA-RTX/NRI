// © 2021 NVIDIA Corporation

#pragma once

struct ID3D12CommandAllocator;
struct ID3D12Resource;

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
struct ID3D12GraphicsCommandList10;
typedef ID3D12GraphicsCommandList10 ID3D12GraphicsCommandListBest;
#else
struct ID3D12GraphicsCommandList6;
typedef ID3D12GraphicsCommandList6 ID3D12GraphicsCommandListBest;
#endif

namespace nri {

struct PipelineD3D12;
struct PipelineLayoutD3D12;
struct DescriptorSetD3D12;

struct CommandBufferD3D12 final : public DebugNameBase {
    inline CommandBufferD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline ~CommandBufferD3D12() {
    }

    inline operator ID3D12GraphicsCommandList*() const {
        return m_GraphicsCommandList.GetInterface();
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline uint8_t GetVersion() const {
        return m_Version;
    }

    inline void ResetAttachments() {
        m_RenderTargetNum = 0;
        for (size_t i = 0; i < m_RenderTargets.size(); i++)
            m_RenderTargets[i].ptr = NULL;

        m_DepthStencil.ptr = NULL;
    }

    Result Create(D3D12_COMMAND_LIST_TYPE commandListType, ID3D12CommandAllocator* commandAllocator);
    Result Create(const CommandBufferD3D12Desc& commandBufferD3D12Desc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_GraphicsCommandList, name);
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
    void ResetQueries(QueryPool& queryPool, uint32_t offset, uint32_t num);
    void BeginQuery(QueryPool& queryPool, uint32_t offset);
    void EndQuery(QueryPool& queryPool, uint32_t offset);
    void CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& buffer, uint64_t alignedBufferOffset);
    void BeginAnnotation(const char* name, uint32_t bgra);
    void EndAnnotation();
    void Annotation(const char* name, uint32_t bgra);
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
    DeviceD3D12& m_Device;
    ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    ComPtr<ID3D12GraphicsCommandListBest> m_GraphicsCommandList;
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT> m_RenderTargets = {};
    std::array<DescriptorSetD3D12*, ROOT_SIGNATURE_DWORD_NUM> m_DescriptorSets = {}; // TODO: needed only for "ClearStorage"
    D3D12_CPU_DESCRIPTOR_HANDLE m_DepthStencil = {};
    const PipelineLayoutD3D12* m_PipelineLayout = nullptr;
    uint32_t m_RenderTargetNum = 0;
    BindPoint m_PipelineBindPoint = BindPoint::INHERIT;
    uint8_t m_Version = 0;
};

} // namespace nri
