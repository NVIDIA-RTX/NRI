// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineD3D11;
typedef Vector<uint32_t> PushBuffer;

struct CommandBufferEmuD3D11 final : public CommandBufferBase {
    inline CommandBufferEmuD3D11(DeviceD3D11& device)
        : m_Device(device)
        , m_PushBuffer(device.GetStdAllocator()) {
    }

    inline ~CommandBufferEmuD3D11() {
    }

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    //================================================================================================================
    // CommandBufferBase
    //================================================================================================================

    inline ID3D11DeviceContextBest* GetNativeObject() const override {
        return m_Device.GetImmediateContext();
    }

    inline const AllocationCallbacks& GetAllocationCallbacks() const override {
        return m_Device.GetAllocationCallbacks();
    }

    Result Create(ID3D11DeviceContext* precreatedContext) override;
    void Submit() override;

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
    void ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum);
    void ClearStorage(const ClearStorageDesc& clearDesc);
    void BeginRendering(const AttachmentsDesc& attachmentsDesc);
    void EndRendering();
    void SetVertexBuffers(uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum);
    void SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType);
    void SetPipelineLayout(BindPoint bindPoint, const PipelineLayout& pipelineLayout);
    void SetPipeline(const Pipeline& pipeline);
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
    void BeginAnnotation(const char* name, uint32_t bgra);
    void EndAnnotation();
    void Annotation(const char* name, uint32_t bgra);

private:
    DeviceD3D11& m_Device;
    PushBuffer m_PushBuffer;
};

} // namespace nri
