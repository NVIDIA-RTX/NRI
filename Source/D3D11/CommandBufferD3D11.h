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

    inline ID3D11DeviceContext* GetNativeObject() const override {
        return m_DeferredContext;
    }

    inline const AllocationCallbacks& CommandBufferD3D11::GetAllocationCallbacks() const override {
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
    void SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets);
    void SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType);
    void SetPipelineLayout(const PipelineLayout& pipelineLayout);
    void SetPipeline(const Pipeline& pipeline);
    void SetDescriptorPool(const DescriptorPool& descriptorPool);
    void SetDescriptorSet(uint32_t setIndex, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets);
    void SetRootConstants(uint32_t rootConstantIndex, const void* data, uint32_t size);
    void SetRootDescriptor(uint32_t rootDescriptorIndex, Descriptor& descriptor);
    void Draw(const DrawDesc& drawDesc);
    void DrawIndexed(const DrawIndexedDesc& drawIndexedDesc);
    void DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset);
    void DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset);
    void CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size);
    void CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc);
    void UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc);
    void ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc);
    void ZeroBuffer(Buffer& buffer, uint64_t offset, uint64_t size);
    void ResolveTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc);
    void Dispatch(const DispatchDesc& dispatchDesc);
    void DispatchIndirect(const Buffer& buffer, uint64_t offset);
    void Barrier(const BarrierGroupDesc& barrierGroupDesc);
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
    const Buffer* m_IndexBuffer = nullptr;
    BindingState m_BindingState;
    SamplePositionsState m_SamplePositionsState = {};
    Color32f m_BlendFactor = {};
    uint64_t m_IndexBufferOffset = 0;
    uint32_t m_RenderTargetNum = 0;
    IndexType m_IndexType = IndexType::UINT32;
    float m_DepthBounds[2] = {0.0f, 1.0f};
    uint8_t m_StencilRef = 0;
    uint8_t m_Version = 0;
    bool m_IsShadingRateLookupTableSet = false;
};

} // namespace nri
