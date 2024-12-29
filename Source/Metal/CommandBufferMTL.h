// © 2021 NVIDIA Corporation

#pragma once

#import <MetalKit/MetalKit.h>

#include "SharedMTL.h"

namespace nri {

struct DeviceMTL;
struct PipelineMTL;
struct BufferMTL;
struct PipelineLayoutMTL;
struct TextureMTL;
struct DescriptorMTL;

struct CommandBufferMTL {
    
    CommandBufferMTL(DeviceMTL& device);
    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }
    
    inline id<MTLCommandBuffer> GetHandle() const {
        return m_Handle;
    }
    
    void SetDebugName(const char* name);
    Result Begin(const DescriptorPool* descriptorPool);
    Result End();
    void SetPipeline(const Pipeline& pipeline);
    void SetPipelineLayout(const PipelineLayout& pipelineLayout);
    void SetDescriptorSet(uint32_t setIndexInPipelineLayout, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets);
    void SetConstants(uint32_t pushConstantIndex, const void* data, uint32_t size);
    void SetDescriptorPool(const DescriptorPool& descriptorPool);
    void Barrier(const BarrierGroupDesc& barrierGroupDesc);
    void BeginRendering(const AttachmentsDesc& attachmentsDesc);
    void EndRendering();
    void SetViewports(const Viewport* viewports, uint32_t viewportNum);
    void SetScissors(const Rect* rects, uint32_t rectNum);
    void SetDepthBounds(float boundsMin, float boundsMax);
    void SetStencilReference(uint8_t frontRef, uint8_t backRef);
    // void SetSamplePositions(const SamplePosition* positions, Sample_t positionNum, Sample_t sampleNum);
    void SetBlendConstants(const Color32f& color);
    void SetShadingRate(const ShadingRateDesc& shadingRateDesc);
    void ClearAttachments(const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum);
    void SetIndexBuffer(const Buffer& buffer, uint64_t offset, IndexType indexType);
    void SetVertexBuffers(uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets);
    void Draw(const DrawDesc& drawDesc);
    void DrawIndexed(const DrawIndexedDesc& drawIndexedDesc);
    void DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset);
    void DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset);
    void Dispatch(const DispatchDesc& dispatchDesc);
    void DispatchIndirect(const Buffer& buffer, uint64_t offset);
    void BeginQuery(const QueryPool& queryPool, uint32_t offset);
    void EndQuery(const QueryPool& queryPool, uint32_t offset);
    void BeginAnnotation(const char* name);
    void EndAnnotation();
    void ClearStorageBuffer(const ClearStorageBufferDesc& clearDesc);
    void ClearStorageTexture(const ClearStorageTextureDesc& clearDesc);
    void CopyBuffer(Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size);
    void CopyTexture(Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc);
    void UploadBufferToTexture(Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc);
    void ReadbackTextureToBuffer(Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc);
    void CopyQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset);
    void ResetQueries(const QueryPool& queryPool, uint32_t offset, uint32_t num);
    void BuildTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset);
    void BuildBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset);
    void UpdateTopLevelAccelerationStructure(uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset);
    void UpdateBottomLevelAccelerationStructure(uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset);
    void CopyAccelerationStructure(AccelerationStructure& dst, AccelerationStructure& src, CopyMode copyMode);
    void WriteAccelerationStructureSize(const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset);
    void DispatchRays(const DispatchRaysDesc& dispatchRaysDesc);
    void DispatchRaysIndirect(const Buffer& buffer, uint64_t offset);
    void DrawMeshTasks(const DrawMeshTasksDesc& drawMeshTasksDesc);
    void DrawMeshTasksIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride);
    
    ~CommandBufferMTL();
    
    void Create(const struct CommandQueueMTL* queue);
    
    struct CmdIndexBuffer {
        size_t m_Offset;
        MTLIndexType m_Type;
        struct BufferMTL* m_Buffer;
    };
    struct CmdVertexBuffer {
        size_t m_Offset;
        struct BufferMTL* m_Buffer;
    };
    
private:
    
    void updateCommandBufferState();
    void EndCurrentEncoders();
    void InsertBarriers();
    
    DeviceMTL& m_Device;
    struct PipelineMTL* m_CurrentPipeline = nullptr;
    PipelineLayoutMTL* m_CurrentPipelineLayout = nullptr;
    id<MTLCommandBuffer> m_Handle;
    id<MTLRenderCommandEncoder> m_RendererEncoder = nil;
    id<MTLComputeCommandEncoder> m_ComputeEncoder = nil;
    id<MTLBlitCommandEncoder> m_BlitEncoder = nil;
    const struct CommandQueueMTL* m_CommandQueue = nullptr;
   
    BarrierBits m_barrierFlags = BarrierBits::NONE;
    
    
    uint16_t m_numViewports = 0;
    uint16_t m_numScissors = 0;
    CommandBufferDirtyBits m_DirtyBits = CommandBufferDirtyBits::NONE;
    
    MTLRenderPassDescriptor* m_renderPassDescriptor = nil;
    struct CmdIndexBuffer m_indexBuffer;
    uint32_t m_dirtyVertexBufferBits = 0;
    struct CmdVertexBuffer m_vertexBuffers[32];
    struct ShadingRateDesc m_shadingRateDesc;
    MTLViewport m_viewports[16];
    MTLScissorRect m_Scissors[16];
    Color32f m_BlendColor;
    uint8_t m_StencilFront;
    uint8_t m_StencilBack;
    
    NSMutableArray* m_Annotations;

};
};

