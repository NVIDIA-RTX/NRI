// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineLayoutD3D11;
struct PipelineD3D11;

struct CommandBufferD3D11 final : public CommandBufferBase {
    CommandBufferD3D11(DeviceD3D11& device);
    ~CommandBufferD3D11();

    inline DeviceD3D11& GetDevice() const {
        return m_Device;
    }

    inline void ResetAttachments() {
        m_RenderTargetNum = 0;
        for (size_t i = 0; i < m_RenderTargets.size(); i++)
            m_RenderTargets[i] = nullptr;

        m_DepthStencil = nullptr;
    }

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_DeferredContext, name);
        SET_D3D_DEBUG_OBJECT_NAME(m_CommandList, name);
    }

    //================================================================================================================
    // CommandBufferBase
    //================================================================================================================

    inline ID3D11DeviceContextBest* GetNativeObject() const override {
        return m_DeferredContext;
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
    void BeginAnnotation(const char* name, uint32_t bgra);
    void EndAnnotation();
    void Annotation(const char* name, uint32_t bgra);

private:
    DeviceD3D11& m_Device;
    ComPtr<ID3D11DeviceContextBest> m_DeferredContext; // can be immediate to redirect data from emulation
    ComPtr<ID3D11CommandList> m_CommandList;
    ComPtr<ID3DUserDefinedAnnotation> m_Annotation;
    std::array<ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> m_RenderTargets = {};
    ID3D11DepthStencilView* m_DepthStencil = nullptr;
    PipelineLayoutD3D11* m_PipelineLayout = nullptr;
    PipelineD3D11* m_Pipeline = nullptr;
    BindingState m_BindingState;
    SamplePositionsState m_SamplePositionsState = {};
    Color32f m_BlendFactor = {};
    float m_DepthBounds[2] = {0.0f, 1.0f};
    uint32_t m_RenderTargetNum = 0;
    BindPoint m_PipelineBindPoint = BindPoint::INHERIT;
    uint8_t m_StencilRef = 0;
    uint8_t m_Version = 0;
    bool m_IsShadingRateLookupTableSet = false;
};

} // namespace nri
