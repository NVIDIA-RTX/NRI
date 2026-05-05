// © 2021 NVIDIA Corporation

#include "MemoryAllocatorD3D12.h"

#include "SharedD3D12.h"

#include "AccelerationStructureD3D12.h"
#include "BufferD3D12.h"
#include "CommandAllocatorD3D12.h"
#include "CommandBufferD3D12.h"
#include "DescriptorD3D12.h"
#include "DescriptorPoolD3D12.h"
#include "DescriptorSetD3D12.h"
#include "FenceD3D12.h"
#include "MemoryD3D12.h"
#include "MicromapD3D12.h"
#include "PipelineCacheD3D12.h"
#include "PipelineD3D12.h"
#include "PipelineLayoutD3D12.h"
#include "QueryPoolD3D12.h"
#include "QueueD3D12.h"
#include "SwapChainD3D12.h"
#include "TextureD3D12.h"
#include "VideoHelpersD3D12.h"

#include "../Shared/VideoAV1.h"
#include "../Shared/VideoAnnexB.h"

#include "HelperInterface.h"
#include "ImguiInterface.h"
#include "StreamerInterface.h"
#include "UpscalerInterface.h"

#include <limits>

using namespace nri;

#include "AccelerationStructureD3D12.hpp"
#include "BufferD3D12.hpp"
#include "CommandAllocatorD3D12.hpp"
#include "CommandBufferD3D12.hpp"
#include "DescriptorD3D12.hpp"
#include "DescriptorPoolD3D12.hpp"
#include "DescriptorSetD3D12.hpp"
#include "DeviceD3D12.hpp"
#include "FenceD3D12.hpp"
#include "MemoryD3D12.hpp"
#include "MicromapD3D12.hpp"
#include "PipelineCacheD3D12.hpp"
#include "PipelineD3D12.hpp"
#include "PipelineLayoutD3D12.hpp"
#include "QueryPoolD3D12.hpp"
#include "QueueD3D12.hpp"
#include "SharedD3D12.hpp"
#include "SwapChainD3D12.hpp"
#include "TextureD3D12.hpp"

Result CreateDeviceD3D12(const DeviceCreationDesc& desc, const DeviceCreationD3D12Desc& descD3D12, DeviceBase*& device) {
    DeviceD3D12* impl = Allocate<DeviceD3D12>(desc.allocationCallbacks, desc.callbackInterface, desc.allocationCallbacks);
    Result result = impl->Create(desc, descD3D12);

    if (result != Result::SUCCESS) {
        Destroy(desc.allocationCallbacks, impl);
        device = nullptr;
    } else
        device = (DeviceBase*)impl;

    return result;
}

//============================================================================================================================================================================================
#pragma region[  Core  ]

static const DeviceDesc& NRI_CALL GetDeviceDesc(const Device& device) {
    return ((DeviceD3D12&)device).GetDesc();
}

static const BufferDesc& NRI_CALL GetBufferDesc(const Buffer& buffer) {
    return ((BufferD3D12&)buffer).GetDesc();
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture& texture) {
    return ((TextureD3D12&)texture).GetDesc();
}

static FormatSupportBits NRI_CALL GetFormatSupport(const Device& device, Format format) {
    return ((DeviceD3D12&)device).GetFormatSupport(format);
}

static Result NRI_CALL GetQueue(Device& device, QueueType queueType, uint32_t queueIndex, Queue*& queue) {
    return ((DeviceD3D12&)device).GetQueue(queueType, queueIndex, queue);
}

static Result NRI_CALL CreateCommandAllocator(Queue& queue, CommandAllocator*& commandAllocator) {
    DeviceD3D12& device = ((QueueD3D12&)queue).GetDevice();
    return device.CreateImplementation<CommandAllocatorD3D12>(commandAllocator, queue);
}

static Result NRI_CALL CreateCommandBuffer(CommandAllocator& commandAllocator, CommandBuffer*& commandBuffer) {
    return ((CommandAllocatorD3D12&)commandAllocator).CreateCommandBuffer(commandBuffer);
}

static Result NRI_CALL CreateFence(Device& device, uint64_t initialValue, Fence*& fence) {
    return ((DeviceD3D12&)device).CreateImplementation<FenceD3D12>(fence, initialValue);
}

static Result NRI_CALL CreateDescriptorPool(Device& device, const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceD3D12&)device).CreateImplementation<DescriptorPoolD3D12>(descriptorPool, descriptorPoolDesc);
}

static Result NRI_CALL CreatePipelineLayout(Device& device, const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    return ((DeviceD3D12&)device).CreateImplementation<PipelineLayoutD3D12>(pipelineLayout, pipelineLayoutDesc);
}

static Result NRI_CALL CreateGraphicsPipeline(Device& device, const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    return ((DeviceD3D12&)device).CreateImplementation<PipelineD3D12>(pipeline, graphicsPipelineDesc);
}

static Result NRI_CALL CreateComputePipeline(Device& device, const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    return ((DeviceD3D12&)device).CreateImplementation<PipelineD3D12>(pipeline, computePipelineDesc);
}

static Result NRI_CALL CreatePipelineCache(Device& device, const PipelineCacheDesc& pipelineCacheDesc, PipelineCache*& pipelineCache) {
    return ((DeviceD3D12&)device).CreateImplementation<PipelineCacheD3D12>(pipelineCache, pipelineCacheDesc);
}

static Result NRI_CALL CreateQueryPool(Device& device, const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    return ((DeviceD3D12&)device).CreateImplementation<QueryPoolD3D12>(queryPool, queryPoolDesc);
}

static Result NRI_CALL CreateSampler(Device& device, const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    return ((DeviceD3D12&)device).CreateImplementation<DescriptorD3D12>(sampler, samplerDesc);
}

static Result NRI_CALL CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    DeviceD3D12& device = ((BufferD3D12*)bufferViewDesc.buffer)->GetDevice();
    return device.CreateImplementation<DescriptorD3D12>(bufferView, bufferViewDesc);
}

static Result NRI_CALL CreateTextureView(const TextureViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D12& device = ((TextureD3D12*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator* commandAllocator) {
    Destroy((CommandAllocatorD3D12*)commandAllocator);
}

static void NRI_CALL DestroyCommandBuffer(CommandBuffer* commandBuffer) {
    Destroy((CommandBufferD3D12*)commandBuffer);
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool* descriptorPool) {
    Destroy((DescriptorPoolD3D12*)descriptorPool);
}

static void NRI_CALL DestroyBuffer(Buffer* buffer) {
    Destroy((BufferD3D12*)buffer);
}

static void NRI_CALL DestroyTexture(Texture* texture) {
    Destroy((TextureD3D12*)texture);
}

static void NRI_CALL DestroyDescriptor(Descriptor* descriptor) {
    Destroy((DescriptorD3D12*)descriptor);
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout* pipelineLayout) {
    Destroy((PipelineLayoutD3D12*)pipelineLayout);
}

static void NRI_CALL DestroyPipeline(Pipeline* pipeline) {
    Destroy((PipelineD3D12*)pipeline);
}

static void NRI_CALL DestroyPipelineCache(PipelineCache* pipelineCache) {
    Destroy((PipelineCacheD3D12*)pipelineCache);
}

static Result NRI_CALL GetPipelineCacheData(PipelineCache& pipelineCache, void* dst, uint64_t& size) {
    return ((PipelineCacheD3D12&)pipelineCache).GetData(dst, size);
}

static void NRI_CALL DestroyQueryPool(QueryPool* queryPool) {
    Destroy((QueryPoolD3D12*)queryPool);
}

static void NRI_CALL DestroyFence(Fence* fence) {
    Destroy((FenceD3D12*)fence);
}

static Result NRI_CALL AllocateMemory(Device& device, const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    return ((DeviceD3D12&)device).CreateImplementation<MemoryD3D12>(memory, allocateMemoryDesc);
}

static void NRI_CALL FreeMemory(Memory* memory) {
    Destroy((MemoryD3D12*)memory);
}

static Result NRI_CALL CreateBuffer(Device& device, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceD3D12&)device).CreateImplementation<BufferD3D12>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTexture(Device& device, const TextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceD3D12&)device).CreateImplementation<TextureD3D12>(texture, textureDesc);
}

static void NRI_CALL GetBufferMemoryDesc(const Buffer& buffer, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const BufferD3D12& bufferD3D12 = (BufferD3D12&)buffer;
    const DeviceD3D12& deviceD3D12 = bufferD3D12.GetDevice();

    D3D12_RESOURCE_DESC1 desc = {};
    deviceD3D12.GetResourceDesc(bufferD3D12.GetDesc(), desc);

    deviceD3D12.GetMemoryDesc(memoryLocation, desc, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc(const Texture& texture, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const TextureD3D12& textureD3D12 = (TextureD3D12&)texture;
    const DeviceD3D12& deviceD3D12 = textureD3D12.GetDevice();

    D3D12_RESOURCE_DESC1 desc = {};
    deviceD3D12.GetResourceDesc(textureD3D12.GetDesc(), desc);

    deviceD3D12.GetMemoryDesc(memoryLocation, desc, memoryDesc);
}

static Result NRI_CALL BindBufferMemory(const BindBufferMemoryDesc* bindBufferMemoryDescs, uint32_t bindBufferMemoryDescNum) {
    if (!bindBufferMemoryDescNum)
        return Result::SUCCESS;

    DeviceD3D12& deviceD3D12 = ((BufferD3D12*)bindBufferMemoryDescs->buffer)->GetDevice();
    return deviceD3D12.BindBufferMemory(bindBufferMemoryDescs, bindBufferMemoryDescNum);
}

static Result NRI_CALL BindTextureMemory(const BindTextureMemoryDesc* bindTextureMemoryDescs, uint32_t bindTextureMemoryDescNum) {
    if (!bindTextureMemoryDescNum)
        return Result::SUCCESS;

    DeviceD3D12& deviceD3D12 = ((TextureD3D12*)bindTextureMemoryDescs->texture)->GetDevice();
    return deviceD3D12.BindTextureMemory(bindTextureMemoryDescs, bindTextureMemoryDescNum);
}

static void NRI_CALL GetBufferMemoryDesc2(const Device& device, const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    D3D12_RESOURCE_DESC1 desc = {};
    deviceD3D12.GetResourceDesc(bufferDesc, desc);

    deviceD3D12.GetMemoryDesc(memoryLocation, desc, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc2(const Device& device, const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    D3D12_RESOURCE_DESC1 desc = {};
    deviceD3D12.GetResourceDesc(textureDesc, desc);

    deviceD3D12.GetMemoryDesc(memoryLocation, desc, memoryDesc);
}

static Result NRI_CALL CreateCommittedBuffer(Device& device, MemoryLocation memoryLocation, float priority, const BufferDesc& bufferDesc, Buffer*& buffer) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    Result result = deviceD3D12.CreateImplementation<BufferD3D12>(buffer, bufferDesc);
    if (result != Result::SUCCESS)
        return result;

    return ((BufferD3D12*)buffer)->Allocate(memoryLocation, priority, true);
}

static Result NRI_CALL CreateCommittedTexture(Device& device, MemoryLocation memoryLocation, float priority, const TextureDesc& textureDesc, Texture*& texture) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    Result result = deviceD3D12.CreateImplementation<TextureD3D12>(texture, textureDesc);
    if (result != Result::SUCCESS)
        return result;

    return ((TextureD3D12*)texture)->Allocate(memoryLocation, priority, true);
}

static Result NRI_CALL CreateCommittedVideoTexture(Device& device, MemoryLocation memoryLocation, float priority, const VideoTextureDesc& videoTextureDesc, Texture*& texture) {
    return CreateCommittedTexture(device, memoryLocation, priority, videoTextureDesc.textureDesc, texture);
}

static Result NRI_CALL CreateCommittedVideoBitstreamBuffer(Device& device, float priority, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return CreateCommittedBuffer(device, MemoryLocation::DEVICE, priority, bufferDesc, buffer);
}

static Result NRI_CALL GetVideoQueue(Device& device, const VideoSessionDesc& videoSessionDesc, Queue*& queue) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;
    if (videoSessionDesc.usage == VideoUsage::DECODE)
        return deviceD3D12.GetQueue(QueueType::VIDEO_DECODE, 0, queue);
    if (videoSessionDesc.usage == VideoUsage::ENCODE)
        return deviceD3D12.GetQueue(QueueType::VIDEO_ENCODE, 0, queue);
    return Result::INVALID_ARGUMENT;
}

static Result NRI_CALL CreatePlacedBuffer(Device& device, Memory* memory, uint64_t offset, const BufferDesc& bufferDesc, Buffer*& buffer) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    Result result = deviceD3D12.CreateImplementation<BufferD3D12>(buffer, bufferDesc);
    if (result != Result::SUCCESS)
        return result;

    if (memory)
        result = ((BufferD3D12*)buffer)->BindMemory(*(MemoryD3D12*)memory, offset);
    else
        result = ((BufferD3D12*)buffer)->Allocate((MemoryLocation)offset, 0.0f, false);

    return result;
}

static Result NRI_CALL CreatePlacedTexture(Device& device, Memory* memory, uint64_t offset, const TextureDesc& textureDesc, Texture*& texture) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    Result result = deviceD3D12.CreateImplementation<TextureD3D12>(texture, textureDesc);
    if (result != Result::SUCCESS)
        return result;

    if (memory)
        result = ((TextureD3D12*)texture)->BindMemory(*(MemoryD3D12*)memory, offset);
    else
        result = ((TextureD3D12*)texture)->Allocate((MemoryLocation)offset, 0.0f, false);

    return result;
}

static Result NRI_CALL AllocateDescriptorSets(DescriptorPool& descriptorPool, const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    return ((DescriptorPoolD3D12&)descriptorPool).AllocateDescriptorSets(pipelineLayout, setIndex, descriptorSets, instanceNum, variableDescriptorNum);
}

static void NRI_CALL UpdateDescriptorRanges(const UpdateDescriptorRangeDesc* updateDescriptorRangeDescs, uint32_t updateDescriptorRangeDescNum) {
    DescriptorSetD3D12::UpdateDescriptorRanges(updateDescriptorRangeDescs, updateDescriptorRangeDescNum);
}

static void NRI_CALL CopyDescriptorRanges(const CopyDescriptorRangeDesc* copyDescriptorRangeDescs, uint32_t copyDescriptorRangeDescNum) {
    DescriptorSetD3D12::Copy(copyDescriptorRangeDescs, copyDescriptorRangeDescNum);
}

static void NRI_CALL ResetDescriptorPool(DescriptorPool& descriptorPool) {
    ((DescriptorPoolD3D12&)descriptorPool).Reset();
}

static void NRI_CALL GetDescriptorSetOffsets(const DescriptorSet& descriptorSet, uint32_t& resourceHeapOffset, uint32_t& samplerHeapOffset) {
    ((DescriptorSetD3D12&)descriptorSet).GetOffsets(resourceHeapOffset, samplerHeapOffset);
}

static Result NRI_CALL BeginCommandBuffer(CommandBuffer& commandBuffer, const DescriptorPool* descriptorPool) {
    return ((CommandBufferD3D12&)commandBuffer).Begin(descriptorPool);
}

static void NRI_CALL CmdSetDescriptorPool(CommandBuffer& commandBuffer, const DescriptorPool& descriptorPool) {
    ((CommandBufferD3D12&)commandBuffer).SetDescriptorPool(descriptorPool);
}

static void NRI_CALL CmdSetPipelineLayout(CommandBuffer& commandBuffer, BindPoint bindPoint, const PipelineLayout& pipelineLayout) {
    ((CommandBufferD3D12&)commandBuffer).SetPipelineLayout(bindPoint, pipelineLayout);
}

static void NRI_CALL CmdSetDescriptorSet(CommandBuffer& commandBuffer, const SetDescriptorSetDesc& setDescriptorSetDesc) {
    ((CommandBufferD3D12&)commandBuffer).SetDescriptorSet(setDescriptorSetDesc);
}

static void NRI_CALL CmdSetRootConstants(CommandBuffer& commandBuffer, const SetRootConstantsDesc& setRootConstantsDesc) {
    ((CommandBufferD3D12&)commandBuffer).SetRootConstants(setRootConstantsDesc);
}

static void NRI_CALL CmdSetRootDescriptor(CommandBuffer& commandBuffer, const SetRootDescriptorDesc& setRootDescriptorDesc) {
    ((CommandBufferD3D12&)commandBuffer).SetRootDescriptor(setRootDescriptorDesc);
}

static void NRI_CALL CmdSetPipeline(CommandBuffer& commandBuffer, const Pipeline& pipeline) {
    ((CommandBufferD3D12&)commandBuffer).SetPipeline(pipeline);
}

static void NRI_CALL CmdBarrier(CommandBuffer& commandBuffer, const BarrierDesc& barrierDesc) {
    ((CommandBufferD3D12&)commandBuffer).Barrier(barrierDesc);
}

static void NRI_CALL CmdSetIndexBuffer(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, IndexType indexType) {
    ((CommandBufferD3D12&)commandBuffer).SetIndexBuffer(buffer, offset, indexType);
}

static void NRI_CALL CmdSetVertexBuffers(CommandBuffer& commandBuffer, uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum) {
    ((CommandBufferD3D12&)commandBuffer).SetVertexBuffers(baseSlot, vertexBufferDescs, vertexBufferNum);
}

static void NRI_CALL CmdSetViewports(CommandBuffer& commandBuffer, const Viewport* viewports, uint32_t viewportNum) {
    ((CommandBufferD3D12&)commandBuffer).SetViewports(viewports, viewportNum);
}

static void NRI_CALL CmdSetScissors(CommandBuffer& commandBuffer, const Rect* rects, uint32_t rectNum) {
    ((CommandBufferD3D12&)commandBuffer).SetScissors(rects, rectNum);
}

static void NRI_CALL CmdSetStencilReference(CommandBuffer& commandBuffer, uint8_t frontRef, uint8_t backRef) {
    ((CommandBufferD3D12&)commandBuffer).SetStencilReference(frontRef, backRef);
}

static void NRI_CALL CmdSetDepthBounds(CommandBuffer& commandBuffer, float boundsMin, float boundsMax) {
    ((CommandBufferD3D12&)commandBuffer).SetDepthBounds(boundsMin, boundsMax);
}

static void NRI_CALL CmdSetBlendConstants(CommandBuffer& commandBuffer, const Color32f& color) {
    ((CommandBufferD3D12&)commandBuffer).SetBlendConstants(color);
}

static void NRI_CALL CmdSetSampleLocations(CommandBuffer& commandBuffer, const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    ((CommandBufferD3D12&)commandBuffer).SetSampleLocations(locations, locationNum, sampleNum);
}

static void NRI_CALL CmdSetShadingRate(CommandBuffer& commandBuffer, const ShadingRateDesc& shadingRateDesc) {
    ((CommandBufferD3D12&)commandBuffer).SetShadingRate(shadingRateDesc);
}

static void NRI_CALL CmdSetDepthBias(CommandBuffer& commandBuffer, const DepthBiasDesc& depthBiasDesc) {
    ((CommandBufferD3D12&)commandBuffer).SetDepthBias(depthBiasDesc);
}

static void NRI_CALL CmdBeginRendering(CommandBuffer& commandBuffer, const RenderingDesc& renderingDesc) {
    ((CommandBufferD3D12&)commandBuffer).BeginRendering(renderingDesc);
}

static void NRI_CALL CmdClearAttachments(CommandBuffer& commandBuffer, const ClearAttachmentDesc* clearAttachmentDescs, uint32_t clearAttachmentDescNum, const Rect* rects, uint32_t rectNum) {
    ((CommandBufferD3D12&)commandBuffer).ClearAttachments(clearAttachmentDescs, clearAttachmentDescNum, rects, rectNum);
}

static void NRI_CALL CmdDraw(CommandBuffer& commandBuffer, const DrawDesc& drawDesc) {
    ((CommandBufferD3D12&)commandBuffer).Draw(drawDesc);
}

static void NRI_CALL CmdDrawIndexed(CommandBuffer& commandBuffer, const DrawIndexedDesc& drawIndexedDesc) {
    ((CommandBufferD3D12&)commandBuffer).DrawIndexed(drawIndexedDesc);
}

static void NRI_CALL CmdDrawIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferD3D12&)commandBuffer).DrawIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL CmdDrawIndexedIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferD3D12&)commandBuffer).DrawIndexedIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL CmdEndRendering(CommandBuffer& commandBuffer) {
    ((CommandBufferD3D12&)commandBuffer).EndRendering();
}

static void NRI_CALL CmdDispatch(CommandBuffer& commandBuffer, const DispatchDesc& dispatchDesc) {
    ((CommandBufferD3D12&)commandBuffer).Dispatch(dispatchDesc);
}

static void NRI_CALL CmdDispatchIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset) {
    ((CommandBufferD3D12&)commandBuffer).DispatchIndirect(buffer, offset);
}

static void NRI_CALL CmdCopyBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    ((CommandBufferD3D12&)commandBuffer).CopyBuffer(dstBuffer, dstOffset, srcBuffer, srcOffset, size);
}

static void NRI_CALL CmdCopyTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    ((CommandBufferD3D12&)commandBuffer).CopyTexture(dstTexture, dstRegion, srcTexture, srcRegion);
}

static void NRI_CALL CmdUploadBufferToTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc& dstRegion, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayout) {
    ((CommandBufferD3D12&)commandBuffer).UploadBufferToTexture(dstTexture, dstRegion, srcBuffer, srcDataLayout);
}

static void NRI_CALL CmdReadbackTextureToBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayout, const Texture& srcTexture, const TextureRegionDesc& srcRegion) {
    ((CommandBufferD3D12&)commandBuffer).ReadbackTextureToBuffer(dstBuffer, dstDataLayout, srcTexture, srcRegion);
}

static void NRI_CALL CmdZeroBuffer(CommandBuffer& commandBuffer, Buffer& buffer, uint64_t offset, uint64_t size) {
    ((CommandBufferD3D12&)commandBuffer).ZeroBuffer(buffer, offset, size);
}

static void NRI_CALL CmdResolveTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion, ResolveOp resolveOp) {
    ((CommandBufferD3D12&)commandBuffer).ResolveTexture(dstTexture, dstRegion, srcTexture, srcRegion, resolveOp);
}

static void NRI_CALL CmdClearStorage(CommandBuffer& commandBuffer, const ClearStorageDesc& clearStorageDesc) {
    ((CommandBufferD3D12&)commandBuffer).ClearStorage(clearStorageDesc);
}

static void NRI_CALL CmdResetQueries(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset, uint32_t num) {
    ((CommandBufferD3D12&)commandBuffer).ResetQueries(queryPool, offset, num);
}

static void NRI_CALL CmdBeginQuery(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferD3D12&)commandBuffer).BeginQuery(queryPool, offset);
}

static void NRI_CALL CmdEndQuery(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferD3D12&)commandBuffer).EndQuery(queryPool, offset);
}

static void NRI_CALL CmdCopyQueries(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    ((CommandBufferD3D12&)commandBuffer).CopyQueries(queryPool, offset, num, dstBuffer, dstOffset);
}

static void NRI_CALL CmdBeginAnnotation(CommandBuffer& commandBuffer, const char* name, uint32_t bgra) {
    MaybeUnused(commandBuffer, name, bgra);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferD3D12&)commandBuffer).BeginAnnotation(name, bgra);
#endif
}

static void NRI_CALL CmdEndAnnotation(CommandBuffer& commandBuffer) {
    MaybeUnused(commandBuffer);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferD3D12&)commandBuffer).EndAnnotation();
#endif
}

static void NRI_CALL CmdAnnotation(CommandBuffer& commandBuffer, const char* name, uint32_t bgra) {
    MaybeUnused(commandBuffer, name, bgra);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferD3D12&)commandBuffer).Annotation(name, bgra);
#endif
}

static Result NRI_CALL EndCommandBuffer(CommandBuffer& commandBuffer) {
    return ((CommandBufferD3D12&)commandBuffer).End();
}

static void NRI_CALL QueueBeginAnnotation(Queue& queue, const char* name, uint32_t bgra) {
    MaybeUnused(queue, name, bgra);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((QueueD3D12&)queue).BeginAnnotation(name, bgra);
#endif
}

static void NRI_CALL QueueEndAnnotation(Queue& queue) {
    MaybeUnused(queue);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((QueueD3D12&)queue).EndAnnotation();
#endif
}

static void NRI_CALL QueueAnnotation(Queue& queue, const char* name, uint32_t bgra) {
    MaybeUnused(queue, name, bgra);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((QueueD3D12&)queue).Annotation(name, bgra);
#endif
}

static void NRI_CALL ResetQueries(QueryPool&, uint32_t, uint32_t) {
}

static uint32_t NRI_CALL GetQuerySize(const QueryPool& queryPool) {
    return ((QueryPoolD3D12&)queryPool).GetQuerySize();
}

static Result NRI_CALL QueueSubmit(Queue& queue, const QueueSubmitDesc& queueSubmitDesc) {
    return ((QueueD3D12&)queue).Submit(queueSubmitDesc);
}

static Result NRI_CALL QueueWaitIdle(Queue* queue) {
    if (!queue)
        return Result::SUCCESS;

    return ((QueueD3D12*)queue)->WaitIdle();
}

static Result NRI_CALL DeviceWaitIdle(Device* device) {
    if (!device)
        return Result::SUCCESS;

    return ((DeviceD3D12*)device)->WaitIdle();
}

static void NRI_CALL Wait(Fence& fence, uint64_t value) {
    ((FenceD3D12&)fence).Wait(value);
}

static uint64_t NRI_CALL GetFenceValue(Fence& fence) {
    return ((FenceD3D12&)fence).GetFenceValue();
}

static void NRI_CALL ResetCommandAllocator(CommandAllocator& commandAllocator) {
    ((CommandAllocatorD3D12&)commandAllocator).Reset();
}

static void* NRI_CALL MapBuffer(Buffer& buffer, uint64_t offset, uint64_t) {
    return ((BufferD3D12&)buffer).Map(offset);
}

static void NRI_CALL UnmapBuffer(Buffer&) {
}

static uint64_t NRI_CALL GetBufferDeviceAddress(const Buffer& buffer) {
    return ((BufferD3D12&)buffer).GetDeviceAddress();
}

static void NRI_CALL SetDebugName(Object* object, const char* name) {
    MaybeUnused(object, name);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    if (object)
        ((DebugNameBase*)object)->SetDebugName(name);
#endif
}

static void* NRI_CALL GetDeviceNativeObject(const Device* device) {
    if (!device)
        return nullptr;

    return ((DeviceD3D12*)device)->GetNativeObject();
}

static void* NRI_CALL GetQueueNativeObject(const Queue* queue) {
    if (!queue)
        return nullptr;

    return (ID3D12CommandQueue*)(*(QueueD3D12*)queue);
}

static void* NRI_CALL GetCommandBufferNativeObject(const CommandBuffer* commandBuffer) {
    if (!commandBuffer)
        return nullptr;

    return (ID3D12CommandList*)(*(CommandBufferD3D12*)commandBuffer);
}

static uint64_t NRI_CALL GetBufferNativeObject(const Buffer* buffer) {
    if (!buffer)
        return 0;

    return uint64_t((ID3D12Resource*)(*(BufferD3D12*)buffer));
}

static uint64_t NRI_CALL GetTextureNativeObject(const Texture* texture) {
    if (!texture)
        return 0;

    return uint64_t((ID3D12Resource*)(*(TextureD3D12*)texture));
}

static uint64_t NRI_CALL GetDescriptorNativeObject(const Descriptor* descriptor) {
    if (!descriptor)
        return 0;

    return uint64_t(((DescriptorD3D12*)descriptor)->GetDescriptorHandleCPU());
}

Result DeviceD3D12::FillFunctionTable(CoreInterface& table) const {
    table.GetDeviceDesc = ::GetDeviceDesc;
    table.GetBufferDesc = ::GetBufferDesc;
    table.GetTextureDesc = ::GetTextureDesc;
    table.GetFormatSupport = ::GetFormatSupport;
    table.GetQuerySize = ::GetQuerySize;
    table.GetFenceValue = ::GetFenceValue;
    table.GetDescriptorSetOffsets = ::GetDescriptorSetOffsets;
    table.GetQueue = ::GetQueue;
    table.CreateCommandAllocator = ::CreateCommandAllocator;
    table.CreateCommandBuffer = ::CreateCommandBuffer;
    table.CreateDescriptorPool = ::CreateDescriptorPool;
    table.CreateBufferView = ::CreateBufferView;
    table.CreateTextureView = ::CreateTextureView;
    table.CreateSampler = ::CreateSampler;
    table.CreatePipelineLayout = ::CreatePipelineLayout;
    table.CreateGraphicsPipeline = ::CreateGraphicsPipeline;
    table.CreateComputePipeline = ::CreateComputePipeline;
    table.CreatePipelineCache = ::CreatePipelineCache;
    table.CreateQueryPool = ::CreateQueryPool;
    table.CreateFence = ::CreateFence;
    table.DestroyCommandAllocator = ::DestroyCommandAllocator;
    table.DestroyCommandBuffer = ::DestroyCommandBuffer;
    table.DestroyDescriptorPool = ::DestroyDescriptorPool;
    table.DestroyBuffer = ::DestroyBuffer;
    table.DestroyTexture = ::DestroyTexture;
    table.DestroyDescriptor = ::DestroyDescriptor;
    table.DestroyPipelineLayout = ::DestroyPipelineLayout;
    table.DestroyPipeline = ::DestroyPipeline;
    table.DestroyPipelineCache = ::DestroyPipelineCache;
    table.GetPipelineCacheData = ::GetPipelineCacheData;
    table.DestroyQueryPool = ::DestroyQueryPool;
    table.DestroyFence = ::DestroyFence;
    table.AllocateMemory = ::AllocateMemory;
    table.FreeMemory = ::FreeMemory;
    table.CreateBuffer = ::CreateBuffer;
    table.CreateTexture = ::CreateTexture;
    table.GetBufferMemoryDesc = ::GetBufferMemoryDesc;
    table.GetTextureMemoryDesc = ::GetTextureMemoryDesc;
    table.BindBufferMemory = ::BindBufferMemory;
    table.BindTextureMemory = ::BindTextureMemory;
    table.GetBufferMemoryDesc2 = ::GetBufferMemoryDesc2;
    table.GetTextureMemoryDesc2 = ::GetTextureMemoryDesc2;
    table.CreateCommittedBuffer = ::CreateCommittedBuffer;
    table.CreateCommittedTexture = ::CreateCommittedTexture;
    table.CreatePlacedBuffer = ::CreatePlacedBuffer;
    table.CreatePlacedTexture = ::CreatePlacedTexture;
    table.AllocateDescriptorSets = ::AllocateDescriptorSets;
    table.UpdateDescriptorRanges = ::UpdateDescriptorRanges;
    table.CopyDescriptorRanges = ::CopyDescriptorRanges;
    table.ResetDescriptorPool = ::ResetDescriptorPool;
    table.BeginCommandBuffer = ::BeginCommandBuffer;
    table.CmdSetDescriptorPool = ::CmdSetDescriptorPool;
    table.CmdSetDescriptorSet = ::CmdSetDescriptorSet;
    table.CmdSetPipelineLayout = ::CmdSetPipelineLayout;
    table.CmdSetPipeline = ::CmdSetPipeline;
    table.CmdSetRootConstants = ::CmdSetRootConstants;
    table.CmdSetRootDescriptor = ::CmdSetRootDescriptor;
    table.CmdBarrier = ::CmdBarrier;
    table.CmdSetIndexBuffer = ::CmdSetIndexBuffer;
    table.CmdSetVertexBuffers = ::CmdSetVertexBuffers;
    table.CmdSetViewports = ::CmdSetViewports;
    table.CmdSetScissors = ::CmdSetScissors;
    table.CmdSetStencilReference = ::CmdSetStencilReference;
    table.CmdSetDepthBounds = ::CmdSetDepthBounds;
    table.CmdSetBlendConstants = ::CmdSetBlendConstants;
    table.CmdSetSampleLocations = ::CmdSetSampleLocations;
    table.CmdSetShadingRate = ::CmdSetShadingRate;
    table.CmdSetDepthBias = ::CmdSetDepthBias;
    table.CmdBeginRendering = ::CmdBeginRendering;
    table.CmdClearAttachments = ::CmdClearAttachments;
    table.CmdDraw = ::CmdDraw;
    table.CmdDrawIndexed = ::CmdDrawIndexed;
    table.CmdDrawIndirect = ::CmdDrawIndirect;
    table.CmdDrawIndexedIndirect = ::CmdDrawIndexedIndirect;
    table.CmdEndRendering = ::CmdEndRendering;
    table.CmdDispatch = ::CmdDispatch;
    table.CmdDispatchIndirect = ::CmdDispatchIndirect;
    table.CmdCopyBuffer = ::CmdCopyBuffer;
    table.CmdCopyTexture = ::CmdCopyTexture;
    table.CmdUploadBufferToTexture = ::CmdUploadBufferToTexture;
    table.CmdReadbackTextureToBuffer = ::CmdReadbackTextureToBuffer;
    table.CmdZeroBuffer = ::CmdZeroBuffer;
    table.CmdResolveTexture = ::CmdResolveTexture;
    table.CmdClearStorage = ::CmdClearStorage;
    table.CmdResetQueries = ::CmdResetQueries;
    table.CmdBeginQuery = ::CmdBeginQuery;
    table.CmdEndQuery = ::CmdEndQuery;
    table.CmdCopyQueries = ::CmdCopyQueries;
    table.CmdBeginAnnotation = ::CmdBeginAnnotation;
    table.CmdEndAnnotation = ::CmdEndAnnotation;
    table.CmdAnnotation = ::CmdAnnotation;
    table.EndCommandBuffer = ::EndCommandBuffer;
    table.QueueBeginAnnotation = ::QueueBeginAnnotation;
    table.QueueEndAnnotation = ::QueueEndAnnotation;
    table.QueueAnnotation = ::QueueAnnotation;
    table.ResetQueries = ::ResetQueries;
    table.QueueSubmit = ::QueueSubmit;
    table.QueueWaitIdle = ::QueueWaitIdle;
    table.DeviceWaitIdle = ::DeviceWaitIdle;
    table.Wait = ::Wait;
    table.ResetCommandAllocator = ::ResetCommandAllocator;
    table.MapBuffer = ::MapBuffer;
    table.UnmapBuffer = ::UnmapBuffer;
    table.GetBufferDeviceAddress = ::GetBufferDeviceAddress;
    table.SetDebugName = ::SetDebugName;
    table.GetDeviceNativeObject = ::GetDeviceNativeObject;
    table.GetQueueNativeObject = ::GetQueueNativeObject;
    table.GetCommandBufferNativeObject = ::GetCommandBufferNativeObject;
    table.GetBufferNativeObject = ::GetBufferNativeObject;
    table.GetTextureNativeObject = ::GetTextureNativeObject;
    table.GetDescriptorNativeObject = ::GetDescriptorNativeObject;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Helper  ]

static Result NRI_CALL UploadData(Queue& queue, const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    QueueD3D12& queueD3D12 = (QueueD3D12&)queue;
    DeviceD3D12& deviceD3D12 = queueD3D12.GetDevice();
    HelperDataUpload helperDataUpload(deviceD3D12.GetCoreInterface(), (Device&)deviceD3D12, queue);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

static uint32_t NRI_CALL CalculateAllocationNumber(const Device& device, const ResourceGroupDesc& resourceGroupDesc) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;
    HelperDeviceMemoryAllocator allocator(deviceD3D12.GetCoreInterface(), (Device&)device);

    return allocator.CalculateAllocationNumber(resourceGroupDesc);
}

static Result NRI_CALL AllocateAndBindMemory(Device& device, const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;
    HelperDeviceMemoryAllocator allocator(deviceD3D12.GetCoreInterface(), device);

    return allocator.AllocateAndBindMemory(resourceGroupDesc, allocations);
}

static Result NRI_CALL QueryVideoMemoryInfo(const Device& device, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) {
    uint64_t luid = ((DeviceD3D12&)device).GetDesc().adapterDesc.uid.low;

    return QueryVideoMemoryInfoDXGI(luid, memoryLocation, videoMemoryInfo);
}

Result DeviceD3D12::FillFunctionTable(HelperInterface& table) const {
    table.CalculateAllocationNumber = ::CalculateAllocationNumber;
    table.AllocateAndBindMemory = ::AllocateAndBindMemory;
    table.UploadData = ::UploadData;
    table.QueryVideoMemoryInfo = ::QueryVideoMemoryInfo;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Imgui  ]

#if NRI_ENABLE_IMGUI_EXTENSION

static Result NRI_CALL CreateImgui(Device& device, const ImguiDesc& imguiDesc, Imgui*& imgui) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;
    ImguiImpl* impl = Allocate<ImguiImpl>(deviceD3D12.GetAllocationCallbacks(), device, deviceD3D12.GetCoreInterface());
    Result result = impl->Create(imguiDesc);

    if (result != Result::SUCCESS) {
        Destroy(impl);
        imgui = nullptr;
    } else
        imgui = (Imgui*)impl;

    return result;
}

static void NRI_CALL DestroyImgui(Imgui* imgui) {
    Destroy((ImguiImpl*)imgui);
}

static void NRI_CALL CmdCopyImguiData(CommandBuffer& commandBuffer, Streamer& streamer, Imgui& imgui, const CopyImguiDataDesc& copyImguiDataDesc) {
    ImguiImpl& imguiImpl = (ImguiImpl&)imgui;

    return imguiImpl.CmdCopyData(commandBuffer, streamer, copyImguiDataDesc);
}

static void NRI_CALL CmdDrawImgui(CommandBuffer& commandBuffer, Imgui& imgui, const DrawImguiDesc& drawImguiDesc) {
    ImguiImpl& imguiImpl = (ImguiImpl&)imgui;

    return imguiImpl.CmdDraw(commandBuffer, drawImguiDesc);
}

Result DeviceD3D12::FillFunctionTable(ImguiInterface& table) const {
    table.CreateImgui = ::CreateImgui;
    table.DestroyImgui = ::DestroyImgui;
    table.CmdCopyImguiData = ::CmdCopyImguiData;
    table.CmdDrawImgui = ::CmdDrawImgui;

    return Result::SUCCESS;
}

#endif

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Low latency  ]

static Result NRI_CALL SetLatencySleepMode(SwapChain& swapChain, const LatencySleepMode& latencySleepMode) {
    return ((SwapChainD3D12&)swapChain).SetLatencySleepMode(latencySleepMode);
}

static Result NRI_CALL SetLatencyMarker(SwapChain& swapChain, LatencyMarker latencyMarker) {
    return ((SwapChainD3D12&)swapChain).SetLatencyMarker(latencyMarker);
}

static Result NRI_CALL LatencySleep(SwapChain& swapChain) {
    return ((SwapChainD3D12&)swapChain).LatencySleep();
}

static Result NRI_CALL GetLatencyReport(const SwapChain& swapChain, LatencyReport& latencyReport) {
    return ((SwapChainD3D12&)swapChain).GetLatencyReport(latencyReport);
}

Result DeviceD3D12::FillFunctionTable(LowLatencyInterface& table) const {
    if (!m_Desc.features.lowLatency)
        return Result::UNSUPPORTED;

    table.SetLatencySleepMode = ::SetLatencySleepMode;
    table.SetLatencyMarker = ::SetLatencyMarker;
    table.LatencySleep = ::LatencySleep;
    table.GetLatencyReport = ::GetLatencyReport;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  MeshShader  ]

static void NRI_CALL CmdDrawMeshTasks(CommandBuffer& commandBuffer, const DrawMeshTasksDesc& drawMeshTasksDesc) {
    ((CommandBufferD3D12&)commandBuffer).DrawMeshTasks(drawMeshTasksDesc);
}

static void NRI_CALL CmdDrawMeshTasksIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferD3D12&)commandBuffer).DrawMeshTasksIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

Result DeviceD3D12::FillFunctionTable(MeshShaderInterface& table) const {
    if (!m_Desc.features.meshShader)
        return Result::UNSUPPORTED;

    table.CmdDrawMeshTasks = ::CmdDrawMeshTasks;
    table.CmdDrawMeshTasksIndirect = ::CmdDrawMeshTasksIndirect;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  RayTracing  ]

static Result NRI_CALL CreateRayTracingPipeline(Device& device, const RayTracingPipelineDesc& rayTracingPipelineDesc, Pipeline*& pipeline) {
    return ((DeviceD3D12&)device).CreateImplementation<PipelineD3D12>(pipeline, rayTracingPipelineDesc);
}

static Result NRI_CALL CreateAccelerationStructureDescriptor(const AccelerationStructure& accelerationStructure, Descriptor*& descriptor) {
    return ((AccelerationStructureD3D12&)accelerationStructure).CreateDescriptor(descriptor);
}

static uint64_t NRI_CALL GetAccelerationStructureHandle(const AccelerationStructure& accelerationStructure) {
    return ((AccelerationStructureD3D12&)accelerationStructure).GetHandle();
}

static uint64_t NRI_CALL GetAccelerationStructureUpdateScratchBufferSize(const AccelerationStructure& accelerationStructure) {
    return ((AccelerationStructureD3D12&)accelerationStructure).GetUpdateScratchBufferSize();
}

static uint64_t NRI_CALL GetAccelerationStructureBuildScratchBufferSize(const AccelerationStructure& accelerationStructure) {
    return ((AccelerationStructureD3D12&)accelerationStructure).GetBuildScratchBufferSize();
}

static uint64_t NRI_CALL GetMicromapBuildScratchBufferSize(const Micromap& micromap) {
    return ((MicromapD3D12&)micromap).GetBuildScratchBufferSize();
}

static Buffer* NRI_CALL GetAccelerationStructureBuffer(const AccelerationStructure& accelerationStructure) {
    return (Buffer*)((AccelerationStructureD3D12&)accelerationStructure).GetBuffer();
}

static Buffer* NRI_CALL GetMicromapBuffer(const Micromap& micromap) {
    return (Buffer*)((MicromapD3D12&)micromap).GetBuffer();
}

static void NRI_CALL DestroyAccelerationStructure(AccelerationStructure* accelerationStructure) {
    Destroy((AccelerationStructureD3D12*)accelerationStructure);
}

static void NRI_CALL DestroyMicromap(Micromap* micromap) {
    Destroy((MicromapD3D12*)micromap);
}

static Result NRI_CALL CreateAccelerationStructure(Device& device, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceD3D12&)device).CreateImplementation<AccelerationStructureD3D12>(accelerationStructure, accelerationStructureDesc);
}

static Result NRI_CALL CreateMicromap(Device& device, const MicromapDesc& micromapDesc, Micromap*& micromap) {
    return ((DeviceD3D12&)device).CreateImplementation<MicromapD3D12>(micromap, micromapDesc);
}

static void NRI_CALL GetAccelerationStructureMemoryDesc(const AccelerationStructure& accelerationStructure, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((AccelerationStructureD3D12&)accelerationStructure).GetMemoryDesc(memoryLocation, memoryDesc);
}

static void NRI_CALL GetMicromapMemoryDesc(const Micromap& micromap, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((MicromapD3D12&)micromap).GetMemoryDesc(memoryLocation, memoryDesc);
}

static Result NRI_CALL BindAccelerationStructureMemory(const BindAccelerationStructureMemoryDesc* bindAccelerationStructureMemoryDescs, uint32_t bindAccelerationStructureMemoryDescNum) {
    if (!bindAccelerationStructureMemoryDescNum)
        return Result::SUCCESS;

    DeviceD3D12& deviceD3D12 = ((AccelerationStructureD3D12*)bindAccelerationStructureMemoryDescs->accelerationStructure)->GetDevice();
    return deviceD3D12.BindAccelerationStructureMemory(bindAccelerationStructureMemoryDescs, bindAccelerationStructureMemoryDescNum);
}

static Result NRI_CALL BindMicromapMemory(const BindMicromapMemoryDesc* bindMicromapMemoryDescs, uint32_t bindMicromapMemoryDescNum) {
    if (!bindMicromapMemoryDescNum)
        return Result::SUCCESS;

    DeviceD3D12& deviceD3D12 = ((MicromapD3D12*)bindMicromapMemoryDescs->micromap)->GetDevice();
    return deviceD3D12.BindMicromapMemory(bindMicromapMemoryDescs, bindMicromapMemoryDescNum);
}

static void NRI_CALL GetAccelerationStructureMemoryDesc2(const Device& device, const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    deviceD3D12.GetAccelerationStructurePrebuildInfo(accelerationStructureDesc, prebuildInfo);

    BufferDesc bufferDesc = {};
    bufferDesc.size = prebuildInfo.ResultDataMaxSizeInBytes;
    bufferDesc.usage = BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE;

    D3D12_RESOURCE_DESC1 resourceDesc = {};
    deviceD3D12.GetResourceDesc(bufferDesc, resourceDesc);
    deviceD3D12.GetMemoryDesc(memoryLocation, resourceDesc, memoryDesc);
}

static void NRI_CALL GetMicromapMemoryDesc2(const Device& device, const MicromapDesc& micromapDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    deviceD3D12.GetMicromapPrebuildInfo(micromapDesc, prebuildInfo);

    BufferDesc bufferDesc = {};
    bufferDesc.size = prebuildInfo.ResultDataMaxSizeInBytes;
    bufferDesc.usage = BufferUsageBits::MICROMAP_STORAGE;

    D3D12_RESOURCE_DESC1 resourceDesc = {};
    deviceD3D12.GetResourceDesc(bufferDesc, resourceDesc);
    deviceD3D12.GetMemoryDesc(memoryLocation, resourceDesc, memoryDesc);
}

static Result NRI_CALL CreateCommittedAccelerationStructure(Device& device, MemoryLocation memoryLocation, float priority, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    Result result = deviceD3D12.CreateImplementation<AccelerationStructureD3D12>(accelerationStructure, accelerationStructureDesc);
    if (result != Result::SUCCESS)
        return result;

    return ((AccelerationStructureD3D12*)accelerationStructure)->Allocate(memoryLocation, priority, true);
}

static Result NRI_CALL CreateCommittedMicromap(Device& device, MemoryLocation memoryLocation, float priority, const MicromapDesc& micromapDesc, Micromap*& micromap) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    Result result = deviceD3D12.CreateImplementation<MicromapD3D12>(micromap, micromapDesc);
    if (result != Result::SUCCESS)
        return result;

    return ((MicromapD3D12*)micromap)->Allocate(memoryLocation, priority, true);
}

static Result NRI_CALL CreatePlacedAccelerationStructure(Device& device, Memory* memory, uint64_t offset, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    Result result = deviceD3D12.CreateImplementation<AccelerationStructureD3D12>(accelerationStructure, accelerationStructureDesc);
    if (result != Result::SUCCESS)
        return result;

    if (memory)
        result = ((AccelerationStructureD3D12*)accelerationStructure)->BindMemory(*(MemoryD3D12*)memory, offset);
    else
        result = ((AccelerationStructureD3D12*)accelerationStructure)->Allocate((MemoryLocation)offset, 0.0f, false);

    return result;
}

static Result NRI_CALL CreatePlacedMicromap(Device& device, Memory* memory, uint64_t offset, const MicromapDesc& micromapDesc, Micromap*& micromap) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    Result result = deviceD3D12.CreateImplementation<MicromapD3D12>(micromap, micromapDesc);
    if (result != Result::SUCCESS)
        return result;

    if (memory)
        result = ((MicromapD3D12*)micromap)->BindMemory(*(MemoryD3D12*)memory, offset);
    else
        result = ((MicromapD3D12*)micromap)->Allocate((MemoryLocation)offset, 0.0f, false);

    return result;
}

static Result NRI_CALL WriteShaderGroupIdentifiers(const Pipeline& pipeline, uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* dst) {
    return ((PipelineD3D12&)pipeline).WriteShaderGroupIdentifiers(baseShaderGroupIndex, shaderGroupNum, dst);
}

static void NRI_CALL CmdBuildTopLevelAccelerationStructures(CommandBuffer& commandBuffer, const BuildTopLevelAccelerationStructureDesc* buildTopLevelAccelerationStructureDescs, uint32_t buildTopLevelAccelerationStructureDescNum) {
    ((CommandBufferD3D12&)commandBuffer).BuildTopLevelAccelerationStructures(buildTopLevelAccelerationStructureDescs, buildTopLevelAccelerationStructureDescNum);
}

static void NRI_CALL CmdBuildBottomLevelAccelerationStructures(CommandBuffer& commandBuffer, const BuildBottomLevelAccelerationStructureDesc* buildBottomLevelAccelerationStructureDescs, uint32_t buildBottomLevelAccelerationStructureDescNum) {
    ((CommandBufferD3D12&)commandBuffer).BuildBottomLevelAccelerationStructures(buildBottomLevelAccelerationStructureDescs, buildBottomLevelAccelerationStructureDescNum);
}

static void NRI_CALL CmdBuildMicromaps(CommandBuffer& commandBuffer, const BuildMicromapDesc* buildMicromapDescs, uint32_t buildMicromapDescNum) {
    ((CommandBufferD3D12&)commandBuffer).BuildMicromaps(buildMicromapDescs, buildMicromapDescNum);
}

static void NRI_CALL CmdDispatchRays(CommandBuffer& commandBuffer, const DispatchRaysDesc& dispatchRaysDesc) {
    ((CommandBufferD3D12&)commandBuffer).DispatchRays(dispatchRaysDesc);
}

static void NRI_CALL CmdDispatchRaysIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset) {
    ((CommandBufferD3D12&)commandBuffer).DispatchRaysIndirect(buffer, offset);
}

static void NRI_CALL CmdWriteAccelerationStructuresSizes(CommandBuffer& commandBuffer, const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    ((CommandBufferD3D12&)commandBuffer).WriteAccelerationStructuresSizes(accelerationStructures, accelerationStructureNum, queryPool, queryPoolOffset);
}

static void NRI_CALL CmdWriteMicromapsSizes(CommandBuffer& commandBuffer, const Micromap* const* micromaps, uint32_t micromapNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    ((CommandBufferD3D12&)commandBuffer).WriteMicromapsSizes(micromaps, micromapNum, queryPool, queryPoolOffset);
}

static void NRI_CALL CmdCopyAccelerationStructure(CommandBuffer& commandBuffer, AccelerationStructure& dst, const AccelerationStructure& src, CopyMode copyMode) {
    ((CommandBufferD3D12&)commandBuffer).CopyAccelerationStructure(dst, src, copyMode);
}

static void NRI_CALL CmdCopyMicromap(CommandBuffer& commandBuffer, Micromap& dst, const Micromap& src, CopyMode copyMode) {
    ((CommandBufferD3D12&)commandBuffer).CopyMicromap(dst, src, copyMode);
}

static uint64_t NRI_CALL GetAccelerationStructureNativeObject(const AccelerationStructure* accelerationStructure) {
    if (!accelerationStructure)
        return 0;

    return uint64_t((ID3D12Resource*)(*(AccelerationStructureD3D12*)accelerationStructure));
}

static uint64_t NRI_CALL GetMicromapNativeObject(const Micromap* micromap) {
    if (!micromap)
        return 0;

    return uint64_t((ID3D12Resource*)(*(MicromapD3D12*)micromap));
}

Result DeviceD3D12::FillFunctionTable(RayTracingInterface& table) const {
    if (m_Desc.tiers.rayTracing == 0)
        return Result::UNSUPPORTED;

    table.CreateAccelerationStructureDescriptor = ::CreateAccelerationStructureDescriptor;
    table.CreateRayTracingPipeline = ::CreateRayTracingPipeline;
    table.GetAccelerationStructureHandle = ::GetAccelerationStructureHandle;
    table.GetAccelerationStructureUpdateScratchBufferSize = ::GetAccelerationStructureUpdateScratchBufferSize;
    table.GetAccelerationStructureBuildScratchBufferSize = ::GetAccelerationStructureBuildScratchBufferSize;
    table.GetAccelerationStructureBuffer = ::GetAccelerationStructureBuffer;
    table.GetMicromapBuildScratchBufferSize = ::GetMicromapBuildScratchBufferSize;
    table.GetMicromapBuffer = ::GetMicromapBuffer;
    table.DestroyAccelerationStructure = ::DestroyAccelerationStructure;
    table.DestroyMicromap = ::DestroyMicromap;
    table.CreateAccelerationStructure = ::CreateAccelerationStructure;
    table.CreateMicromap = ::CreateMicromap;
    table.GetAccelerationStructureMemoryDesc = ::GetAccelerationStructureMemoryDesc;
    table.GetMicromapMemoryDesc = ::GetMicromapMemoryDesc;
    table.BindAccelerationStructureMemory = ::BindAccelerationStructureMemory;
    table.BindMicromapMemory = ::BindMicromapMemory;
    table.GetAccelerationStructureMemoryDesc2 = ::GetAccelerationStructureMemoryDesc2;
    table.GetMicromapMemoryDesc2 = ::GetMicromapMemoryDesc2;
    table.CreateCommittedAccelerationStructure = ::CreateCommittedAccelerationStructure;
    table.CreateCommittedMicromap = ::CreateCommittedMicromap;
    table.CreatePlacedAccelerationStructure = ::CreatePlacedAccelerationStructure;
    table.CreatePlacedMicromap = ::CreatePlacedMicromap;
    table.WriteShaderGroupIdentifiers = ::WriteShaderGroupIdentifiers;
    table.CmdBuildTopLevelAccelerationStructures = ::CmdBuildTopLevelAccelerationStructures;
    table.CmdBuildBottomLevelAccelerationStructures = ::CmdBuildBottomLevelAccelerationStructures;
    table.CmdBuildMicromaps = ::CmdBuildMicromaps;
    table.CmdDispatchRays = ::CmdDispatchRays;
    table.CmdDispatchRaysIndirect = ::CmdDispatchRaysIndirect;
    table.CmdWriteAccelerationStructuresSizes = ::CmdWriteAccelerationStructuresSizes;
    table.CmdWriteMicromapsSizes = ::CmdWriteMicromapsSizes;
    table.CmdCopyAccelerationStructure = ::CmdCopyAccelerationStructure;
    table.CmdCopyMicromap = ::CmdCopyMicromap;
    table.GetAccelerationStructureNativeObject = ::GetAccelerationStructureNativeObject;
    table.GetMicromapNativeObject = ::GetMicromapNativeObject;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Video  ]

struct VideoSessionD3D12 final : public DebugNameBase {
    inline VideoSessionD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    Result Create(const VideoSessionDesc& videoSessionDesc);

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_Decoder, name);
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_DecoderHeap, name);
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_Encoder, name);
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_EncoderHeap, name);
    }

    DeviceD3D12& m_Device;
    ComPtr<ID3D12VideoDecoder> m_Decoder;
    ComPtr<ID3D12VideoDecoderHeap> m_DecoderHeap;
    ComPtr<ID3D12VideoEncoder> m_Encoder;
    ComPtr<ID3D12VideoEncoderHeap> m_EncoderHeap;
    ComPtr<ID3D12VideoEncoderHeap1> m_EncoderHeap1;
    D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAGS m_AV1FeatureFlags = D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_NONE;
    VideoSessionDesc m_Desc = {};
};

struct VideoSessionParametersD3D12 final {
    inline VideoSessionParametersD3D12(DeviceD3D12& device)
        : m_Device(device)
        , m_H264SequenceParameterSets(device.GetStdAllocator())
        , m_H264PictureParameterSets(device.GetStdAllocator())
        , m_H265VideoParameterSets(device.GetStdAllocator())
        , m_H265SequenceParameterSets(device.GetStdAllocator())
        , m_H265PictureParameterSets(device.GetStdAllocator())
        , m_H265SequenceScalingLists(device.GetStdAllocator())
        , m_H265PictureScalingLists(device.GetStdAllocator()) {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    Result Create(const VideoSessionParametersDesc& videoSessionParametersDesc) {
        if (!videoSessionParametersDesc.session)
            return Result::INVALID_ARGUMENT;

        m_Session = (VideoSessionD3D12*)videoSessionParametersDesc.session;
        m_H264Parameters = videoSessionParametersDesc.h264Parameters;
        m_H265Parameters = videoSessionParametersDesc.h265Parameters;
        m_AV1Parameters = videoSessionParametersDesc.av1Parameters;
        if (m_H264Parameters) {
            if ((m_H264Parameters->sequenceParameterSetNum && !m_H264Parameters->sequenceParameterSets) || (m_H264Parameters->pictureParameterSetNum && !m_H264Parameters->pictureParameterSets))
                return Result::INVALID_ARGUMENT;

            m_H264SequenceParameterSets.clear();
            m_H264PictureParameterSets.clear();
            if (m_H264Parameters->sequenceParameterSetNum)
                m_H264SequenceParameterSets.assign(m_H264Parameters->sequenceParameterSets, m_H264Parameters->sequenceParameterSets + m_H264Parameters->sequenceParameterSetNum);
            if (m_H264Parameters->pictureParameterSetNum)
                m_H264PictureParameterSets.assign(m_H264Parameters->pictureParameterSets, m_H264Parameters->pictureParameterSets + m_H264Parameters->pictureParameterSetNum);
            m_H264ParametersStorage = *m_H264Parameters;
            m_H264ParametersStorage.sequenceParameterSets = m_H264SequenceParameterSets.data();
            m_H264ParametersStorage.pictureParameterSets = m_H264PictureParameterSets.data();
            m_H264Parameters = &m_H264ParametersStorage;
        }
        if (m_H265Parameters) {
            if ((m_H265Parameters->videoParameterSetNum && !m_H265Parameters->videoParameterSets) || (m_H265Parameters->sequenceParameterSetNum && !m_H265Parameters->sequenceParameterSets) || (m_H265Parameters->pictureParameterSetNum && !m_H265Parameters->pictureParameterSets))
                return Result::INVALID_ARGUMENT;

            m_H265VideoParameterSets.clear();
            m_H265SequenceParameterSets.clear();
            m_H265PictureParameterSets.clear();
            m_H265SequenceScalingLists.clear();
            m_H265PictureScalingLists.clear();
            if (m_H265Parameters->videoParameterSetNum)
                m_H265VideoParameterSets.assign(m_H265Parameters->videoParameterSets, m_H265Parameters->videoParameterSets + m_H265Parameters->videoParameterSetNum);
            if (m_H265Parameters->sequenceParameterSetNum)
                m_H265SequenceParameterSets.assign(m_H265Parameters->sequenceParameterSets, m_H265Parameters->sequenceParameterSets + m_H265Parameters->sequenceParameterSetNum);
            if (m_H265Parameters->pictureParameterSetNum)
                m_H265PictureParameterSets.assign(m_H265Parameters->pictureParameterSets, m_H265Parameters->pictureParameterSets + m_H265Parameters->pictureParameterSetNum);
            m_H265SequenceScalingLists.resize(m_H265SequenceParameterSets.size());
            for (size_t i = 0; i < m_H265SequenceParameterSets.size(); i++) {
                if (m_H265SequenceParameterSets[i].scalingLists) {
                    m_H265SequenceScalingLists[i] = *m_H265SequenceParameterSets[i].scalingLists;
                    m_H265SequenceParameterSets[i].scalingLists = &m_H265SequenceScalingLists[i];
                }
            }
            m_H265PictureScalingLists.resize(m_H265PictureParameterSets.size());
            for (size_t i = 0; i < m_H265PictureParameterSets.size(); i++) {
                if (m_H265PictureParameterSets[i].scalingLists) {
                    m_H265PictureScalingLists[i] = *m_H265PictureParameterSets[i].scalingLists;
                    m_H265PictureParameterSets[i].scalingLists = &m_H265PictureScalingLists[i];
                }
            }
            m_H265ParametersStorage = *m_H265Parameters;
            m_H265ParametersStorage.videoParameterSets = m_H265VideoParameterSets.data();
            m_H265ParametersStorage.sequenceParameterSets = m_H265SequenceParameterSets.data();
            m_H265ParametersStorage.pictureParameterSets = m_H265PictureParameterSets.data();
            m_H265Parameters = &m_H265ParametersStorage;
        }
        if (m_AV1Parameters) {
            m_AV1ParametersStorage = *m_AV1Parameters;
            m_AV1Parameters = &m_AV1ParametersStorage;
        }
        return Result::SUCCESS;
    }

    DeviceD3D12& m_Device;
    VideoSessionD3D12* m_Session = nullptr;
    VideoH264SessionParametersDesc m_H264ParametersStorage = {};
    Vector<VideoH264SequenceParameterSetDesc> m_H264SequenceParameterSets;
    Vector<VideoH264PictureParameterSetDesc> m_H264PictureParameterSets;
    const VideoH264SessionParametersDesc* m_H264Parameters = nullptr;
    VideoH265SessionParametersDesc m_H265ParametersStorage = {};
    Vector<VideoH265VideoParameterSetDesc> m_H265VideoParameterSets;
    Vector<VideoH265SequenceParameterSetDesc> m_H265SequenceParameterSets;
    Vector<VideoH265PictureParameterSetDesc> m_H265PictureParameterSets;
    Vector<VideoH265ScalingListsDesc> m_H265SequenceScalingLists;
    Vector<VideoH265ScalingListsDesc> m_H265PictureScalingLists;
    const VideoH265SessionParametersDesc* m_H265Parameters = nullptr;
    VideoAV1SessionParametersDesc m_AV1ParametersStorage = {};
    const VideoAV1SessionParametersDesc* m_AV1Parameters = nullptr;
};

// Older Windows SDK headers used by some builds do not name this newer support bit yet.
static constexpr D3D12_VIDEO_ENCODER_SUPPORT_FLAGS D3D12_VIDEO_ENCODER_SUPPORT_FLAG_READABLE_RECONSTRUCTED_PICTURE_LAYOUT_AVAILABLE_COMPAT = (D3D12_VIDEO_ENCODER_SUPPORT_FLAGS)0x8000;

struct VideoPictureD3D12 final : public DebugNameBase {
    inline VideoPictureD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    Result Create(const VideoPictureDesc& videoPictureDesc) {
        if (!videoPictureDesc.texture)
            return Result::INVALID_ARGUMENT;

        m_Texture = (TextureD3D12*)videoPictureDesc.texture;
        m_Subresource = videoPictureDesc.subresource;
        return Result::SUCCESS;
    }

    void SetDebugName(const char*) NRI_DEBUG_NAME_OVERRIDE {
    }

    DeviceD3D12& m_Device;
    TextureD3D12* m_Texture = nullptr;
    uint32_t m_Subresource = 0;
};

static GUID GetVideoDecodeProfileD3D12(const VideoSessionDesc& videoSessionDesc) {
    switch (videoSessionDesc.codec) {
        case VideoCodec::H264:
            return D3D12_VIDEO_DECODE_PROFILE_H264;
        case VideoCodec::H265:
            return videoSessionDesc.format == Format::P010_UNORM || videoSessionDesc.format == Format::P016_UNORM ? D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN10 : D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN;
        case VideoCodec::AV1:
            return D3D12_VIDEO_DECODE_PROFILE_AV1_PROFILE0;
        case VideoCodec::MAX_NUM:
            return {};
    }

    return {};
}

static D3D12_VIDEO_ENCODER_CODEC GetVideoEncodeCodecD3D12(VideoCodec codec) {
    switch (codec) {
        case VideoCodec::H264:
            return D3D12_VIDEO_ENCODER_CODEC_H264;
        case VideoCodec::H265:
            return D3D12_VIDEO_ENCODER_CODEC_HEVC;
        case VideoCodec::AV1:
            return D3D12_VIDEO_ENCODER_CODEC_AV1;
        case VideoCodec::MAX_NUM:
            return (D3D12_VIDEO_ENCODER_CODEC)-1;
    }

    return (D3D12_VIDEO_ENCODER_CODEC)-1;
}

Result VideoSessionD3D12::Create(const VideoSessionDesc& videoSessionDesc) {
    if (videoSessionDesc.width == 0 || videoSessionDesc.height == 0 || videoSessionDesc.format == Format::UNKNOWN)
        return Result::INVALID_ARGUMENT;

    if (videoSessionDesc.usage == VideoUsage::DECODE) {
        ComPtr<ID3D12VideoDevice> videoDevice;
        HRESULT hr = m_Device->QueryInterface(IID_PPV_ARGS(&videoDevice));
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::QueryInterface(ID3D12VideoDevice)");

        D3D12_VIDEO_DECODE_CONFIGURATION configuration = {};
        configuration.DecodeProfile = GetVideoDecodeProfileD3D12(videoSessionDesc);
        configuration.BitstreamEncryption = D3D12_BITSTREAM_ENCRYPTION_TYPE_NONE;
        configuration.InterlaceType = D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE;
        if (configuration.DecodeProfile == GUID{})
            return Result::UNSUPPORTED;

        D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT decodeSupport = {};
        decodeSupport.Configuration = configuration;
        decodeSupport.Width = videoSessionDesc.width;
        decodeSupport.Height = videoSessionDesc.height;
        decodeSupport.DecodeFormat = GetDxgiFormat(videoSessionDesc.format).typed;
        decodeSupport.FrameRate = {30, 1};
        hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_SUPPORT, &decodeSupport, sizeof(decodeSupport));
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice::CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_SUPPORT)");
        if ((decodeSupport.SupportFlags & D3D12_VIDEO_DECODE_SUPPORT_FLAG_SUPPORTED) == 0) {
            NRI_REPORT_WARNING(&m_Device, "D3D12 video decode support rejected: supportFlags=0x%X configurationFlags=0x%X decodeTier=0x%X", decodeSupport.SupportFlags,
                decodeSupport.ConfigurationFlags, decodeSupport.DecodeTier);
            return Result::UNSUPPORTED;
        }
        if (decodeSupport.ConfigurationFlags & D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_REFERENCE_ONLY_ALLOCATIONS_REQUIRED) {
            NRI_REPORT_WARNING(&m_Device, "D3D12 video decode support requires reference-only allocations, which are not exposed by the current NRIVideo texture usage flags");
            return Result::UNSUPPORTED;
        }

        D3D12_VIDEO_DECODER_DESC decoderDesc = {};
        decoderDesc.Configuration = configuration;

        hr = videoDevice->CreateVideoDecoder(&decoderDesc, IID_PPV_ARGS(&m_Decoder));
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice::CreateVideoDecoder");

        D3D12_VIDEO_DECODER_HEAP_DESC heapDesc = {};
        heapDesc.Configuration = configuration;
        heapDesc.DecodeWidth = videoSessionDesc.width;
        heapDesc.DecodeHeight = videoSessionDesc.height;
        heapDesc.Format = GetDxgiFormat(videoSessionDesc.format).typed;
        heapDesc.MaxDecodePictureBufferCount = videoSessionDesc.maxReferenceNum ? videoSessionDesc.maxReferenceNum : 1;

        hr = videoDevice->CreateVideoDecoderHeap(&heapDesc, IID_PPV_ARGS(&m_DecoderHeap));
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice::CreateVideoDecoderHeap");
    } else if (videoSessionDesc.usage == VideoUsage::ENCODE) {
        ComPtr<ID3D12VideoDevice3> videoDevice;
        HRESULT hr = m_Device->QueryInterface(IID_PPV_ARGS(&videoDevice));
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::QueryInterface(ID3D12VideoDevice3)");

        D3D12_VIDEO_ENCODER_CODEC codec = GetVideoEncodeCodecD3D12(videoSessionDesc.codec);
        if (codec == (D3D12_VIDEO_ENCODER_CODEC)-1)
            return Result::UNSUPPORTED;

        D3D12_VIDEO_ENCODER_PROFILE_H264 h264Profile = D3D12_VIDEO_ENCODER_PROFILE_H264_HIGH;
        D3D12_VIDEO_ENCODER_PROFILE_HEVC hevcProfile = videoSessionDesc.format == Format::P010_UNORM || videoSessionDesc.format == Format::P016_UNORM ? D3D12_VIDEO_ENCODER_PROFILE_HEVC_MAIN10 : D3D12_VIDEO_ENCODER_PROFILE_HEVC_MAIN;
        D3D12_VIDEO_ENCODER_AV1_PROFILE av1Profile = D3D12_VIDEO_ENCODER_AV1_PROFILE_MAIN;
        D3D12_VIDEO_ENCODER_PROFILE_DESC profile = {};
        if (videoSessionDesc.codec == VideoCodec::H264) {
            profile.DataSize = sizeof(h264Profile);
            profile.pH264Profile = &h264Profile;
        } else if (videoSessionDesc.codec == VideoCodec::H265) {
            profile.DataSize = sizeof(hevcProfile);
            profile.pHEVCProfile = &hevcProfile;
        } else {
            profile.DataSize = sizeof(av1Profile);
            profile.pAV1Profile = &av1Profile;
        }

        D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264 h264Config = {};
        h264Config.DirectModeConfig = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264_DIRECT_MODES_DISABLED;
        h264Config.DisableDeblockingFilterConfig = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264_SLICES_DEBLOCKING_MODE_0_ALL_LUMA_CHROMA_SLICE_BLOCK_EDGES_ALWAYS_FILTERED;

        D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC hevcConfig = {};
        hevcConfig.MinLumaCodingUnitSize = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_8x8;
        hevcConfig.MaxLumaCodingUnitSize = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_32x32;
        hevcConfig.MinLumaTransformUnitSize = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_4x4;
        hevcConfig.MaxLumaTransformUnitSize = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_32x32;
        hevcConfig.max_transform_hierarchy_depth_inter = 3;
        hevcConfig.max_transform_hierarchy_depth_intra = 3;
        if (videoSessionDesc.codec == VideoCodec::H265) {
            D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC hevcCaps = {};
            hevcCaps.MinLumaCodingUnitSize = hevcConfig.MinLumaCodingUnitSize;
            hevcCaps.MaxLumaCodingUnitSize = hevcConfig.MaxLumaCodingUnitSize;
            hevcCaps.MinLumaTransformUnitSize = hevcConfig.MinLumaTransformUnitSize;
            hevcCaps.MaxLumaTransformUnitSize = hevcConfig.MaxLumaTransformUnitSize;
            hevcCaps.max_transform_hierarchy_depth_inter = hevcConfig.max_transform_hierarchy_depth_inter;
            hevcCaps.max_transform_hierarchy_depth_intra = hevcConfig.max_transform_hierarchy_depth_intra;

            D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT hevcConfigSupport = {};
            hevcConfigSupport.Codec = codec;
            hevcConfigSupport.Profile = profile;
            hevcConfigSupport.CodecSupportLimits.DataSize = sizeof(hevcCaps);
            hevcConfigSupport.CodecSupportLimits.pHEVCSupport = &hevcCaps;
            hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &hevcConfigSupport, sizeof(hevcConfigSupport));
            NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice3::CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT)");
            if (!hevcConfigSupport.IsSupported)
                return Result::UNSUPPORTED;

            if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_ASYMETRIC_MOTION_PARTITION_SUPPORT || hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_ASYMETRIC_MOTION_PARTITION_REQUIRED)
                hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_USE_ASYMETRIC_MOTION_PARTITION;
            if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_SAO_FILTER_SUPPORT)
                hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ENABLE_SAO_FILTER;
            if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_DISABLING_LOOP_FILTER_ACROSS_SLICES_SUPPORT)
                hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_DISABLE_LOOP_FILTER_ACROSS_SLICES;
            if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_TRANSFORM_SKIP_SUPPORT)
                hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ENABLE_TRANSFORM_SKIPPING;
        }

        D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION av1Config = {};
        av1Config.FeatureFlags = D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_NONE;
        av1Config.OrderHintBitsMinus1 = 7;
        if (videoSessionDesc.codec == VideoCodec::AV1) {
            D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION_SUPPORT av1Caps = {};
            D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT av1ConfigSupport = {};
            av1ConfigSupport.Codec = codec;
            av1ConfigSupport.Profile = profile;
            av1ConfigSupport.CodecSupportLimits.DataSize = sizeof(av1Caps);
            av1ConfigSupport.CodecSupportLimits.pAV1Support = &av1Caps;
            hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &av1ConfigSupport, sizeof(av1ConfigSupport));
            NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice3::CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT)");
            if (!av1ConfigSupport.IsSupported)
                return Result::UNSUPPORTED;

            if (!IsVideoEncodeAV1RequiredFeatureSetSupportedD3D12(av1Caps.RequiredFeatureFlags)) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 AV1 encoder requires unsupported feature flags: required=0x%X", av1Caps.RequiredFeatureFlags);
                return Result::UNSUPPORTED;
            }

            av1Config.FeatureFlags = av1Caps.RequiredFeatureFlags;
            m_AV1FeatureFlags = av1Config.FeatureFlags;
        }

        D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION codecConfig = {};
        if (videoSessionDesc.codec == VideoCodec::H264) {
            codecConfig.DataSize = sizeof(h264Config);
            codecConfig.pH264Config = &h264Config;
        } else if (videoSessionDesc.codec == VideoCodec::H265) {
            codecConfig.DataSize = sizeof(hevcConfig);
            codecConfig.pHEVCConfig = &hevcConfig;
        } else {
            codecConfig.DataSize = sizeof(av1Config);
            codecConfig.pAV1Config = &av1Config;
        }

        D3D12_FEATURE_DATA_VIDEO_ENCODER_RATE_CONTROL_MODE rateControlMode = {};
        rateControlMode.Codec = codec;
        rateControlMode.RateControlMode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CQP;
        hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_RATE_CONTROL_MODE, &rateControlMode, sizeof(rateControlMode));
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice3::CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_RATE_CONTROL_MODE)");
        if (!rateControlMode.IsSupported)
            return Result::UNSUPPORTED;

        D3D12_VIDEO_ENCODER_RATE_CONTROL_CQP cqp = {26, 28, 30};
        D3D12_VIDEO_ENCODER_RATE_CONTROL rateControl = {};
        rateControl.Mode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CQP;
        rateControl.ConfigParams.DataSize = sizeof(cqp);
        rateControl.ConfigParams.pConfiguration_CQP = &cqp;
        rateControl.TargetFrameRate = {30, 1};

        D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_H264 h264Gop = {};
        h264Gop.GOPLength = videoSessionDesc.maxReferenceNum ? 60 : 1;
        h264Gop.PPicturePeriod = 1;

        D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_HEVC hevcGop = {};
        hevcGop.GOPLength = videoSessionDesc.maxReferenceNum ? 60 : 1;
        hevcGop.PPicturePeriod = videoSessionDesc.maxReferenceNum > 1 ? 2 : 1;

        D3D12_VIDEO_ENCODER_AV1_SEQUENCE_STRUCTURE av1Sequence = {};
        av1Sequence.IntraDistance = videoSessionDesc.maxReferenceNum ? 60 : 1;
        av1Sequence.InterFramePeriod = videoSessionDesc.maxReferenceNum ? 1 : 0;

        D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE gop = {};
        if (videoSessionDesc.codec == VideoCodec::H264) {
            gop.DataSize = sizeof(h264Gop);
            gop.pH264GroupOfPictures = &h264Gop;
        } else if (videoSessionDesc.codec == VideoCodec::H265) {
            gop.DataSize = sizeof(hevcGop);
            gop.pHEVCGroupOfPictures = &hevcGop;
        } else {
            gop.DataSize = sizeof(av1Sequence);
            gop.pAV1SequenceStructure = &av1Sequence;
        }

        D3D12_VIDEO_ENCODER_LEVELS_H264 suggestedH264Level = D3D12_VIDEO_ENCODER_LEVELS_H264_41;
        D3D12_VIDEO_ENCODER_LEVEL_TIER_CONSTRAINTS_HEVC suggestedHevcLevel = {D3D12_VIDEO_ENCODER_LEVELS_HEVC_41, D3D12_VIDEO_ENCODER_TIER_HEVC_MAIN};
        D3D12_VIDEO_ENCODER_AV1_LEVEL_TIER_CONSTRAINTS suggestedAv1Level = {D3D12_VIDEO_ENCODER_AV1_LEVELS_4_1, D3D12_VIDEO_ENCODER_AV1_TIER_MAIN};
        D3D12_VIDEO_ENCODER_LEVEL_SETTING suggestedLevel = {};
        if (videoSessionDesc.codec == VideoCodec::H264) {
            suggestedLevel.DataSize = sizeof(suggestedH264Level);
            suggestedLevel.pH264LevelSetting = &suggestedH264Level;
        } else if (videoSessionDesc.codec == VideoCodec::H265) {
            suggestedLevel.DataSize = sizeof(suggestedHevcLevel);
            suggestedLevel.pHEVCLevelSetting = &suggestedHevcLevel;
        } else {
            suggestedLevel.DataSize = sizeof(suggestedAv1Level);
            suggestedLevel.pAV1LevelSetting = &suggestedAv1Level;
        }

        D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC resolution = {videoSessionDesc.width, videoSessionDesc.height};
        D3D12_FEATURE_DATA_VIDEO_ENCODER_RESOLUTION_SUPPORT_LIMITS resolutionLimits = {};
        if (videoSessionDesc.codec == VideoCodec::AV1) {
            D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_TILES tiles = {};
            tiles.RowCount = 1;
            tiles.ColCount = 1;

            D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1 encoderSupport = {};
            encoderSupport.Codec = codec;
            encoderSupport.InputFormat = GetDxgiFormat(videoSessionDesc.format).typed;
            encoderSupport.CodecConfiguration = codecConfig;
            encoderSupport.CodecGopSequence = gop;
            encoderSupport.RateControl = rateControl;
            encoderSupport.IntraRefresh = D3D12_VIDEO_ENCODER_INTRA_REFRESH_MODE_NONE;
            encoderSupport.SubregionFrameEncoding = D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_FULL_FRAME;
            encoderSupport.ResolutionsListCount = 1;
            encoderSupport.pResolutionList = &resolution;
            encoderSupport.MaxReferenceFramesInDPB = 8;
            encoderSupport.SuggestedProfile = profile;
            encoderSupport.SuggestedLevel = suggestedLevel;
            encoderSupport.pResolutionDependentSupport = &resolutionLimits;
            encoderSupport.SubregionFrameEncodingData.DataSize = sizeof(tiles);
            encoderSupport.SubregionFrameEncodingData.pTilesPartition_AV1 = &tiles;
            hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1, &encoderSupport, sizeof(encoderSupport));
            NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice3::CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1)");
            if ((encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_GENERAL_SUPPORT_OK) == 0) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 video encoder support rejected: validationFlags=0x%X supportFlags=0x%X", encoderSupport.ValidationFlags, encoderSupport.SupportFlags);
                return Result::UNSUPPORTED;
            }
            if ((encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_READABLE_RECONSTRUCTED_PICTURE_LAYOUT_AVAILABLE_COMPAT) == 0) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 video encoder support requires reference-only reconstructed pictures, which are not exposed by the current NRIVideo texture usage flags");
                return Result::UNSUPPORTED;
            }
        } else {
            D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT encoderSupport = {};
            encoderSupport.Codec = codec;
            encoderSupport.InputFormat = GetDxgiFormat(videoSessionDesc.format).typed;
            encoderSupport.CodecConfiguration = codecConfig;
            encoderSupport.CodecGopSequence = gop;
            encoderSupport.RateControl = rateControl;
            encoderSupport.IntraRefresh = D3D12_VIDEO_ENCODER_INTRA_REFRESH_MODE_NONE;
            encoderSupport.SubregionFrameEncoding = D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_FULL_FRAME;
            encoderSupport.ResolutionsListCount = 1;
            encoderSupport.pResolutionList = &resolution;
            encoderSupport.MaxReferenceFramesInDPB = videoSessionDesc.maxReferenceNum;
            encoderSupport.SuggestedProfile = profile;
            encoderSupport.SuggestedLevel = suggestedLevel;
            encoderSupport.pResolutionDependentSupport = &resolutionLimits;
            hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT, &encoderSupport, sizeof(encoderSupport));
            NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice3::CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT)");
            if ((encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_GENERAL_SUPPORT_OK) == 0) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 video encoder support rejected: validationFlags=0x%X supportFlags=0x%X", encoderSupport.ValidationFlags, encoderSupport.SupportFlags);
                return Result::UNSUPPORTED;
            }
            if ((encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_READABLE_RECONSTRUCTED_PICTURE_LAYOUT_AVAILABLE_COMPAT) == 0) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 video encoder support requires reference-only reconstructed pictures, which are not exposed by the current NRIVideo texture usage flags");
                return Result::UNSUPPORTED;
            }
        }

        D3D12_VIDEO_ENCODER_DESC encoderDesc = {};
        encoderDesc.EncodeCodec = codec;
        encoderDesc.EncodeProfile = profile;
        encoderDesc.InputFormat = GetDxgiFormat(videoSessionDesc.format).typed;
        encoderDesc.CodecConfiguration = codecConfig;
        encoderDesc.MaxMotionEstimationPrecision = D3D12_VIDEO_ENCODER_MOTION_ESTIMATION_PRECISION_MODE_MAXIMUM;

        hr = videoDevice->CreateVideoEncoder(&encoderDesc, IID_PPV_ARGS(&m_Encoder));
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice3::CreateVideoEncoder");

        D3D12_VIDEO_ENCODER_HEAP_DESC heapDesc = {};
        heapDesc.EncodeCodec = codec;
        heapDesc.EncodeProfile = profile;
        heapDesc.EncodeLevel = suggestedLevel;
        heapDesc.ResolutionsListCount = 1;
        heapDesc.pResolutionList = &resolution;

        hr = videoDevice->CreateVideoEncoderHeap(&heapDesc, IID_PPV_ARGS(&m_EncoderHeap));
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice3::CreateVideoEncoderHeap");
    } else
        return Result::UNSUPPORTED;

    m_Desc = videoSessionDesc;

    return Result::SUCCESS;
}

static Result NRI_CALL CreateVideoSession(Device& device, const VideoSessionDesc& videoSessionDesc, VideoSession*& videoSession) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;
    VideoSessionD3D12* impl = Allocate<VideoSessionD3D12>(deviceD3D12.GetAllocationCallbacks(), deviceD3D12);
    Result result = impl->Create(videoSessionDesc);

    if (result != Result::SUCCESS) {
        Destroy(deviceD3D12.GetAllocationCallbacks(), impl);
        videoSession = nullptr;
    } else
        videoSession = (VideoSession*)impl;

    return result;
}

static void NRI_CALL DestroyVideoSession(VideoSession& videoSession) {
    Destroy((VideoSessionD3D12*)&videoSession);
}

static Result NRI_CALL CreateVideoSessionParameters(Device& device, const VideoSessionParametersDesc& videoSessionParametersDesc, VideoSessionParameters*& videoSessionParameters) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;
    VideoSessionParametersD3D12* impl = Allocate<VideoSessionParametersD3D12>(deviceD3D12.GetAllocationCallbacks(), deviceD3D12);
    Result result = impl->Create(videoSessionParametersDesc);

    if (result != Result::SUCCESS) {
        Destroy(impl);
        videoSessionParameters = nullptr;
    } else
        videoSessionParameters = (VideoSessionParameters*)impl;

    return result;
}

static void NRI_CALL DestroyVideoSessionParameters(VideoSessionParameters& videoSessionParameters) {
    Destroy((VideoSessionParametersD3D12*)&videoSessionParameters);
}

static Result NRI_CALL CreateVideoPicture(Device& device, const VideoPictureDesc& videoPictureDesc, VideoPicture*& videoPicture) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;
    VideoPictureD3D12* impl = Allocate<VideoPictureD3D12>(deviceD3D12.GetAllocationCallbacks(), deviceD3D12);
    Result result = impl->Create(videoPictureDesc);

    if (result != Result::SUCCESS) {
        Destroy(impl);
        videoPicture = nullptr;
    } else
        videoPicture = (VideoPicture*)impl;

    return result;
}

static void NRI_CALL DestroyVideoPicture(VideoPicture& videoPicture) {
    Destroy((VideoPictureD3D12*)&videoPicture);
}

static Result NRI_CALL GetVideoDecodePictureStates(const VideoPicture&, VideoDecodePictureStates& states) {
    states = {};
    states.decodeWrite = {AccessBits::VIDEO_DECODE_WRITE, Layout::GENERAL, StageBits::VIDEO_DECODE};
    states.afterDecode = {AccessBits::NONE, Layout::GENERAL, StageBits::NONE};
    states.graphicsBefore = states.afterDecode;
    states.releaseAfterDecode = true;
    return Result::SUCCESS;
}

static Result NRI_CALL WriteVideoAnnexBParameterSets(VideoAnnexBParameterSetsDesc& annexBParameterSetsDesc) {
    return WriteVideoAnnexBParameterSetsShared(annexBParameterSetsDesc);
}

static uint32_t GetVideoDecodeAV1ReferenceNameIndexD3D12(VideoAV1ReferenceName name) {
    switch (name) {
        case VideoAV1ReferenceName::NONE:
            return 7;
        case VideoAV1ReferenceName::LAST:
            return 0;
        case VideoAV1ReferenceName::LAST2:
            return 1;
        case VideoAV1ReferenceName::LAST3:
            return 2;
        case VideoAV1ReferenceName::GOLDEN:
            return 3;
        case VideoAV1ReferenceName::BWDREF:
            return 4;
        case VideoAV1ReferenceName::ALTREF2:
            return 5;
        case VideoAV1ReferenceName::ALTREF:
            return 6;
        case VideoAV1ReferenceName::MAX_NUM:
            return 7;
    }

    return 7;
}

static uint8_t GetVideoDecodeAV1FrameTypeD3D12(VideoEncodeFrameType frameType) {
    return frameType == VideoEncodeFrameType::IDR || frameType == VideoEncodeFrameType::I ? 0 : 1;
}

static void NRI_CALL CmdDecodeVideo(CommandBuffer& commandBuffer, const VideoDecodeDesc& videoDecodeDesc) {
    CommandBufferD3D12& commandBufferD3D12 = (CommandBufferD3D12&)commandBuffer;
    DeviceD3D12& device = commandBufferD3D12.GetDevice();

    if (!videoDecodeDesc.session || !videoDecodeDesc.parameters || !videoDecodeDesc.bitstream.buffer || !videoDecodeDesc.bitstream.size || !videoDecodeDesc.dstPicture) {
        NRI_REPORT_ERROR(&device, "'session', 'parameters', 'bitstream.buffer', 'bitstream.size' and 'dstPicture' must be valid");
        return;
    }

    if (videoDecodeDesc.argumentNum > 10) {
        NRI_REPORT_ERROR(&device, "'argumentNum' must be <= 10");
        return;
    }

    if (videoDecodeDesc.referenceNum != 0 && !videoDecodeDesc.references) {
        NRI_REPORT_ERROR(&device, "'references' is NULL");
        return;
    }

    if (videoDecodeDesc.argumentNum != 0 && !videoDecodeDesc.arguments) {
        NRI_REPORT_ERROR(&device, "'arguments' is NULL");
        return;
    }

    VideoSessionD3D12& session = *(VideoSessionD3D12*)videoDecodeDesc.session;
    VideoSessionParametersD3D12* parameters = (VideoSessionParametersD3D12*)videoDecodeDesc.parameters;
    if (parameters->m_Session != &session) {
        NRI_REPORT_ERROR(&device, "'parameters' must belong to 'session'");
        return;
    }

    BufferD3D12& bitstream = *(BufferD3D12*)videoDecodeDesc.bitstream.buffer;
    if (videoDecodeDesc.bitstream.offset >= bitstream.GetDesc().size || videoDecodeDesc.bitstream.size > bitstream.GetDesc().size - videoDecodeDesc.bitstream.offset) {
        NRI_REPORT_ERROR(&device, "'bitstream' range is outside of 'bitstream.buffer'");
        return;
    }

    D3D12_VIDEO_DECODE_INPUT_STREAM_ARGUMENTS input = {};
    DXVA_PicParams_H264 h264PictureParameters = {};
    DXVA_Qmatrix_H264 h264InverseQuantizationMatrix = {};
    Scratch<DXVA_Slice_H264_Short> h264Slices = NRI_ALLOCATE_SCRATCH(device, DXVA_Slice_H264_Short, videoDecodeDesc.h264PictureDesc ? std::max(videoDecodeDesc.h264PictureDesc->sliceOffsetNum, 1u) : 1u);
    DXVA_PicParams_HEVC h265PictureParameters = {};
    DXVA_Qmatrix_HEVC h265InverseQuantizationMatrix = {};
    Scratch<DXVA_Slice_HEVC_Short> h265Slices = NRI_ALLOCATE_SCRATCH(device, DXVA_Slice_HEVC_Short, videoDecodeDesc.h265PictureDesc ? std::max(videoDecodeDesc.h265PictureDesc->sliceSegmentOffsetNum, 1u) : 1u);
    DXVA_PicParams_AV1 av1PictureParameters = {};
    Scratch<DXVA_Tile_AV1> av1Tiles = NRI_ALLOCATE_SCRATCH(device, DXVA_Tile_AV1, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum, 1u) : 1u);
    if (videoDecodeDesc.h264PictureDesc) {
        if (session.m_Desc.codec != VideoCodec::H264) {
            NRI_REPORT_ERROR(&device, "'h264PictureDesc' can only be used with H.264 decode sessions");
            return;
        }

        if (!parameters || !parameters->m_H264Parameters) {
            NRI_REPORT_ERROR(&device, "'parameters' with H.264 SPS/PPS data must be valid for neutral H.264 D3D12 decode");
            return;
        }

        if (!CanBuildVideoDecodeH264ArgumentsD3D12(videoDecodeDesc)) {
            NRI_REPORT_ERROR(&device, "D3D12 neutral H.264 decode requires matching H.264 reference descriptors for inter pictures");
            return;
        }

        for (uint32_t i = 0; i < videoDecodeDesc.referenceNum; i++) {
            if (!FindVideoH264DecodeReferenceDescD3D12(videoDecodeDesc.h264PictureDesc->references, videoDecodeDesc.h264PictureDesc->referenceNum, videoDecodeDesc.references[i].slot)) {
                NRI_REPORT_ERROR(&device, "'h264PictureDesc->references' must include metadata for each H.264 reference");
                return;
            }
        }

        const uint32_t h264DstSlot = videoDecodeDesc.h264PictureDesc->referenceSlot ? videoDecodeDesc.h264PictureDesc->referenceSlot : videoDecodeDesc.dstSlot;
        if (!BuildVideoDecodeH264ArgumentsD3D12(*parameters->m_H264Parameters, *videoDecodeDesc.h264PictureDesc, videoDecodeDesc.bitstream.size, h264DstSlot,
                h264PictureParameters, h264InverseQuantizationMatrix, h264Slices, videoDecodeDesc.h264PictureDesc->sliceOffsetNum)) {
            NRI_REPORT_ERROR(&device, "Failed to build D3D12 H.264 decode arguments from neutral descriptors");
            return;
        }

        input.NumFrameArguments = 3;
        input.FrameArguments[0].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_PICTURE_PARAMETERS;
        input.FrameArguments[0].Size = sizeof(h264PictureParameters);
        input.FrameArguments[0].pData = &h264PictureParameters;
        input.FrameArguments[1].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_INVERSE_QUANTIZATION_MATRIX;
        input.FrameArguments[1].Size = sizeof(h264InverseQuantizationMatrix);
        input.FrameArguments[1].pData = &h264InverseQuantizationMatrix;
        input.FrameArguments[2].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_SLICE_CONTROL;
        input.FrameArguments[2].Size = sizeof(DXVA_Slice_H264_Short) * videoDecodeDesc.h264PictureDesc->sliceOffsetNum;
        input.FrameArguments[2].pData = h264Slices;
    } else if (videoDecodeDesc.h265PictureDesc) {
        if (session.m_Desc.codec != VideoCodec::H265) {
            NRI_REPORT_ERROR(&device, "'h265PictureDesc' can only be used with H.265 decode sessions");
            return;
        }

        if (!parameters || !parameters->m_H265Parameters) {
            NRI_REPORT_ERROR(&device, "'parameters' with H.265 VPS/SPS/PPS data must be valid for neutral H.265 D3D12 decode");
            return;
        }

        const VideoH265DecodePictureDesc& desc = *videoDecodeDesc.h265PictureDesc;
        if (desc.referenceNum != 0 && !desc.references) {
            NRI_REPORT_ERROR(&device, "'h265PictureDesc->references' is NULL");
            return;
        }

        for (uint32_t i = 0; i < videoDecodeDesc.referenceNum; i++) {
            if (!FindVideoH265ReferenceDescD3D12(desc.references, desc.referenceNum, videoDecodeDesc.references[i].slot)) {
                NRI_REPORT_ERROR(&device, "'h265PictureDesc->references' must include metadata for each H.265 reference");
                return;
            }
        }

        if (!BuildVideoDecodeH265ArgumentsD3D12(*parameters->m_H265Parameters, desc, videoDecodeDesc.bitstream.size, videoDecodeDesc.dstSlot,
                h265PictureParameters, h265InverseQuantizationMatrix, h265Slices, desc.sliceSegmentOffsetNum)) {
            NRI_REPORT_ERROR(&device, "Failed to build D3D12 H.265 decode arguments from neutral descriptors");
            return;
        }

        input.NumFrameArguments = 3;
        input.FrameArguments[0].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_PICTURE_PARAMETERS;
        input.FrameArguments[0].Size = sizeof(h265PictureParameters);
        input.FrameArguments[0].pData = &h265PictureParameters;
        input.FrameArguments[1].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_INVERSE_QUANTIZATION_MATRIX;
        input.FrameArguments[1].Size = sizeof(h265InverseQuantizationMatrix);
        input.FrameArguments[1].pData = &h265InverseQuantizationMatrix;
        input.FrameArguments[2].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_SLICE_CONTROL;
        input.FrameArguments[2].Size = sizeof(DXVA_Slice_HEVC_Short) * desc.sliceSegmentOffsetNum;
        input.FrameArguments[2].pData = h265Slices;
    } else if (videoDecodeDesc.av1PictureDesc) {
        if (session.m_Desc.codec != VideoCodec::AV1) {
            NRI_REPORT_ERROR(&device, "'av1PictureDesc' can only be used with AV1 decode sessions");
            return;
        }

        const VideoAV1DecodePictureDesc& desc = *videoDecodeDesc.av1PictureDesc;
        if ((desc.tileNum != 0 && !desc.tiles) || desc.tileNum > 256 || desc.referenceNum > 8 || (desc.referenceNum != 0 && !desc.references)) {
            NRI_REPORT_ERROR(&device, "'av1PictureDesc' contains invalid tile or reference data");
            return;
        }
        if (desc.tileLayout && (!desc.tileLayout->columnNum || !desc.tileLayout->rowNum || desc.tileLayout->columnNum > 64 || desc.tileLayout->rowNum > 64 || !desc.tileLayout->miColumnStarts || !desc.tileLayout->miRowStarts || !desc.tileLayout->widthInSuperblocksMinus1 || !desc.tileLayout->heightInSuperblocksMinus1)) {
            NRI_REPORT_ERROR(&device, "'av1PictureDesc->tileLayout' is invalid");
            return;
        }

        const VideoAV1SessionParametersDesc defaultAV1Parameters = {GetDefaultVideoAV1SequenceDescD3D12(session.m_Desc.width, session.m_Desc.height, session.m_Desc.format)};
        const VideoAV1SessionParametersDesc& av1Parameters = parameters && parameters->m_AV1Parameters ? *parameters->m_AV1Parameters : defaultAV1Parameters;
        const VideoAV1SequenceDesc& sequence = av1Parameters.sequence;
        const VideoAV1PictureBits pictureFlags = desc.flags == VideoAV1PictureBits::NONE ? GetDefaultVideoAV1PictureFlags(false) : desc.flags;
        av1PictureParameters.width = session.m_Desc.width;
        av1PictureParameters.height = session.m_Desc.height;
        av1PictureParameters.max_width = sequence.maxFrameWidthMinus1 + 1;
        av1PictureParameters.max_height = sequence.maxFrameHeightMinus1 + 1;
        av1PictureParameters.CurrPicTextureIndex = (UCHAR)videoDecodeDesc.dstSlot;
        av1PictureParameters.superres_denom = desc.superresDenom ? desc.superresDenom : 8;
        av1PictureParameters.bitdepth = sequence.bitDepth;
        av1PictureParameters.seq_profile = sequence.seqProfile;
        av1PictureParameters.tiles.cols = 1;
        av1PictureParameters.tiles.rows = 1;
        av1PictureParameters.tiles.widths[0] = (USHORT)((session.m_Desc.width + 63) / 64);
        av1PictureParameters.tiles.heights[0] = (USHORT)((session.m_Desc.height + 63) / 64);
        if (desc.tileLayout) {
            av1PictureParameters.tiles.cols = desc.tileLayout->columnNum;
            av1PictureParameters.tiles.rows = desc.tileLayout->rowNum;
            av1PictureParameters.tiles.context_update_id = desc.tileLayout->contextUpdateTileId;
            for (uint32_t i = 0; i < desc.tileLayout->columnNum; i++)
                av1PictureParameters.tiles.widths[i] = desc.tileLayout->widthInSuperblocksMinus1[i] + 1;
            for (uint32_t i = 0; i < desc.tileLayout->rowNum; i++)
                av1PictureParameters.tiles.heights[i] = desc.tileLayout->heightInSuperblocksMinus1[i] + 1;
        }
        av1PictureParameters.coding.use_128x128_superblock = !!(sequence.flags & VideoAV1SequenceBits::USE_128X128_SUPERBLOCK);
        av1PictureParameters.coding.intra_edge_filter = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_INTRA_EDGE_FILTER);
        av1PictureParameters.coding.interintra_compound = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_INTERINTRA_COMPOUND);
        av1PictureParameters.coding.masked_compound = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_MASKED_COMPOUND);
        av1PictureParameters.coding.warped_motion = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_WARPED_MOTION);
        av1PictureParameters.coding.dual_filter = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_DUAL_FILTER);
        av1PictureParameters.coding.jnt_comp = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_JNT_COMP);
        av1PictureParameters.coding.enable_ref_frame_mvs = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_REF_FRAME_MVS);
        av1PictureParameters.coding.screen_content_tools = !!(pictureFlags & VideoAV1PictureBits::ALLOW_SCREEN_CONTENT_TOOLS);
        av1PictureParameters.coding.integer_mv = !!(pictureFlags & VideoAV1PictureBits::FORCE_INTEGER_MV);
        av1PictureParameters.coding.cdef = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_CDEF);
        av1PictureParameters.coding.restoration = !!(sequence.flags & VideoAV1SequenceBits::ENABLE_RESTORATION);
        av1PictureParameters.coding.film_grain = !!(sequence.flags & VideoAV1SequenceBits::FILM_GRAIN_PARAMS_PRESENT);
        av1PictureParameters.coding.intrabc = !!(pictureFlags & VideoAV1PictureBits::ALLOW_INTRABC);
        av1PictureParameters.coding.high_precision_mv = !!(pictureFlags & VideoAV1PictureBits::ALLOW_HIGH_PRECISION_MV);
        av1PictureParameters.coding.switchable_motion_mode = !!(pictureFlags & VideoAV1PictureBits::IS_MOTION_MODE_SWITCHABLE);
        av1PictureParameters.coding.disable_frame_end_update_cdf = !!(pictureFlags & VideoAV1PictureBits::DISABLE_FRAME_END_UPDATE_CDF);
        av1PictureParameters.coding.disable_cdf_update = !!(pictureFlags & VideoAV1PictureBits::DISABLE_CDF_UPDATE);
        av1PictureParameters.coding.reference_mode = !!(pictureFlags & VideoAV1PictureBits::REFERENCE_SELECT);
        av1PictureParameters.coding.skip_mode = !!(pictureFlags & VideoAV1PictureBits::SKIP_MODE_PRESENT);
        av1PictureParameters.coding.reduced_tx_set = !!(pictureFlags & VideoAV1PictureBits::REDUCED_TX_SET);
        av1PictureParameters.coding.superres = !!(pictureFlags & VideoAV1PictureBits::USE_SUPERRES);
        av1PictureParameters.coding.tx_mode = desc.txMode ? desc.txMode : 2;
        av1PictureParameters.coding.use_ref_frame_mvs = !!(pictureFlags & VideoAV1PictureBits::USE_REF_FRAME_MVS);
        av1PictureParameters.coding.reference_frame_update = desc.refreshFrameFlags != 0;
        av1PictureParameters.format.frame_type = GetVideoDecodeAV1FrameTypeD3D12(desc.frameType);
        av1PictureParameters.format.show_frame = !!(pictureFlags & VideoAV1PictureBits::SHOW_FRAME);
        av1PictureParameters.format.showable_frame = !!(pictureFlags & VideoAV1PictureBits::SHOWABLE_FRAME);
        av1PictureParameters.format.subsampling_x = sequence.subsamplingX;
        av1PictureParameters.format.subsampling_y = sequence.subsamplingY;
        av1PictureParameters.format.mono_chrome = !!(sequence.flags & VideoAV1SequenceBits::MONO_CHROME);
        av1PictureParameters.primary_ref_frame = (UCHAR)GetVideoDecodeAV1ReferenceNameIndexD3D12(desc.primaryReferenceName);
        av1PictureParameters.order_hint = desc.orderHint;
        av1PictureParameters.order_hint_bits = (UCHAR)(sequence.orderHintBitsMinus1 + 1);
        std::memset(av1PictureParameters.RefFrameMapTextureIndex, 0xFF, sizeof(av1PictureParameters.RefFrameMapTextureIndex));
        for (uint32_t i = 0; i < 7; i++)
            av1PictureParameters.frame_refs[i].Index = 0xFF;
        for (uint32_t i = 0; i < desc.referenceNum; i++) {
            const VideoAV1ReferenceDesc& reference = desc.references[i];
            if (reference.refFrameIndex >= 8 || reference.slot > 0xFE) {
                NRI_REPORT_ERROR(&device, "'av1PictureDesc->references[%u]' is invalid", i);
                return;
            }

            av1PictureParameters.RefFrameMapTextureIndex[reference.refFrameIndex] = (UCHAR)reference.slot;
            const uint32_t referenceNameIndex = GetVideoDecodeAV1ReferenceNameIndexD3D12(reference.name);
            if (referenceNameIndex < 7) {
                av1PictureParameters.frame_refs[referenceNameIndex].Index = reference.refFrameIndex;
                av1PictureParameters.frame_refs[referenceNameIndex].width = session.m_Desc.width;
                av1PictureParameters.frame_refs[referenceNameIndex].height = session.m_Desc.height;
                if (desc.globalMotion) {
                    const uint32_t gmIndex = referenceNameIndex + 1;
                    av1PictureParameters.frame_refs[referenceNameIndex].wminvalid = desc.globalMotion->invalid[gmIndex] != 0;
                    av1PictureParameters.frame_refs[referenceNameIndex].wmtype = desc.globalMotion->type[gmIndex];
                    std::memcpy(av1PictureParameters.frame_refs[referenceNameIndex].wmmat, desc.globalMotion->params[gmIndex], sizeof(av1PictureParameters.frame_refs[referenceNameIndex].wmmat));
                }
            }
        }
        av1PictureParameters.quantization.delta_q_present = !!(pictureFlags & VideoAV1PictureBits::DELTA_Q_PRESENT);
        av1PictureParameters.quantization.delta_q_res = desc.deltaQRes;
        av1PictureParameters.quantization.base_qindex = desc.baseQIndex;
        if (desc.quantization) {
            av1PictureParameters.quantization.y_dc_delta_q = desc.quantization->deltaQYDc;
            av1PictureParameters.quantization.u_dc_delta_q = desc.quantization->deltaQUDc;
            av1PictureParameters.quantization.u_ac_delta_q = desc.quantization->deltaQUAc;
            av1PictureParameters.quantization.v_dc_delta_q = desc.quantization->deltaQVDc;
            av1PictureParameters.quantization.v_ac_delta_q = desc.quantization->deltaQVAc;
            av1PictureParameters.quantization.qm_y = desc.quantization->usingQmatrix ? desc.quantization->qmY : 0xFF;
            av1PictureParameters.quantization.qm_u = desc.quantization->usingQmatrix ? desc.quantization->qmU : 0xFF;
            av1PictureParameters.quantization.qm_v = desc.quantization->usingQmatrix ? desc.quantization->qmV : 0xFF;
        } else {
            av1PictureParameters.quantization.qm_y = 0xFF;
            av1PictureParameters.quantization.qm_u = 0xFF;
            av1PictureParameters.quantization.qm_v = 0xFF;
        }
        av1PictureParameters.cdef.damping = desc.cdefDampingMinus3;
        av1PictureParameters.cdef.bits = desc.cdefBits;
        if (desc.cdef) {
            for (uint32_t i = 0; i < 8; i++) {
                av1PictureParameters.cdef.y_strengths[i].primary = desc.cdef->yPrimaryStrength[i];
                av1PictureParameters.cdef.y_strengths[i].secondary = desc.cdef->ySecondaryStrength[i];
                av1PictureParameters.cdef.uv_strengths[i].primary = desc.cdef->uvPrimaryStrength[i];
                av1PictureParameters.cdef.uv_strengths[i].secondary = desc.cdef->uvSecondaryStrength[i];
            }
        }
        av1PictureParameters.interp_filter = desc.interpolationFilter ? desc.interpolationFilter : 4;
        av1PictureParameters.loop_filter.delta_lf_present = !!(pictureFlags & VideoAV1PictureBits::DELTA_LF_PRESENT);
        av1PictureParameters.loop_filter.delta_lf_multi = !!(pictureFlags & VideoAV1PictureBits::DELTA_LF_MULTI);
        av1PictureParameters.loop_filter.delta_lf_res = desc.deltaLfRes;
        if (desc.loopFilter) {
            av1PictureParameters.loop_filter.filter_level[0] = desc.loopFilter->level[0];
            av1PictureParameters.loop_filter.filter_level[1] = desc.loopFilter->level[1];
            av1PictureParameters.loop_filter.filter_level_u = desc.loopFilter->level[2];
            av1PictureParameters.loop_filter.filter_level_v = desc.loopFilter->level[3];
            av1PictureParameters.loop_filter.sharpness_level = desc.loopFilter->sharpness;
            av1PictureParameters.loop_filter.mode_ref_delta_enabled = desc.loopFilter->deltaEnabled;
            av1PictureParameters.loop_filter.mode_ref_delta_update = desc.loopFilter->deltaUpdate;
            std::memcpy(av1PictureParameters.loop_filter.ref_deltas, desc.loopFilter->refDeltas, sizeof(av1PictureParameters.loop_filter.ref_deltas));
            std::memcpy(av1PictureParameters.loop_filter.mode_deltas, desc.loopFilter->modeDeltas, sizeof(av1PictureParameters.loop_filter.mode_deltas));
        } else {
            av1PictureParameters.loop_filter.ref_deltas[0] = 1;
            av1PictureParameters.loop_filter.ref_deltas[4] = -1;
            av1PictureParameters.loop_filter.ref_deltas[6] = -1;
            av1PictureParameters.loop_filter.ref_deltas[7] = -1;
        }
        if (desc.loopRestoration) {
            av1PictureParameters.loop_filter.frame_restoration_type[0] = desc.loopRestoration->frameRestorationType[0];
            av1PictureParameters.loop_filter.frame_restoration_type[1] = desc.loopRestoration->frameRestorationType[1];
            av1PictureParameters.loop_filter.frame_restoration_type[2] = desc.loopRestoration->frameRestorationType[2];
            const bool usesLr = desc.loopRestoration->frameRestorationType[0] || desc.loopRestoration->frameRestorationType[1] || desc.loopRestoration->frameRestorationType[2];
            if (usesLr) {
                av1PictureParameters.loop_filter.log2_restoration_unit_size[0] = 6 + desc.loopRestoration->lrUnitShift;
                av1PictureParameters.loop_filter.log2_restoration_unit_size[1] = 6 + desc.loopRestoration->lrUnitShift - desc.loopRestoration->lrUvShift;
                av1PictureParameters.loop_filter.log2_restoration_unit_size[2] = 6 + desc.loopRestoration->lrUnitShift - desc.loopRestoration->lrUvShift;
            }
        }
        if (!desc.loopRestoration || (!av1PictureParameters.loop_filter.log2_restoration_unit_size[0] && !av1PictureParameters.loop_filter.log2_restoration_unit_size[1] && !av1PictureParameters.loop_filter.log2_restoration_unit_size[2])) {
            av1PictureParameters.loop_filter.log2_restoration_unit_size[0] = 8;
            av1PictureParameters.loop_filter.log2_restoration_unit_size[1] = 8;
            av1PictureParameters.loop_filter.log2_restoration_unit_size[2] = 8;
        }
        av1PictureParameters.segmentation.enabled = !!(pictureFlags & VideoAV1PictureBits::SEGMENTATION_ENABLED);
        av1PictureParameters.segmentation.update_map = !!(pictureFlags & VideoAV1PictureBits::SEGMENTATION_UPDATE_MAP);
        av1PictureParameters.segmentation.update_data = !!(pictureFlags & VideoAV1PictureBits::SEGMENTATION_UPDATE_DATA);
        av1PictureParameters.segmentation.temporal_update = !!(pictureFlags & VideoAV1PictureBits::SEGMENTATION_TEMPORAL_UPDATE);
        if (desc.segmentation) {
            for (uint32_t i = 0; i < 8; i++) {
                av1PictureParameters.segmentation.feature_mask[i].mask = desc.segmentation->featureEnabled[i];
                for (uint32_t j = 0; j < 8; j++)
                    av1PictureParameters.segmentation.feature_data[i][j] = desc.segmentation->featureData[i][j];
            }
        }
        if ((pictureFlags & VideoAV1PictureBits::APPLY_GRAIN) && desc.filmGrain) {
            av1PictureParameters.film_grain.apply_grain = 1;
            av1PictureParameters.film_grain.scaling_shift_minus8 = desc.filmGrain->grainScalingMinus8;
            av1PictureParameters.film_grain.chroma_scaling_from_luma = desc.filmGrain->chromaScalingFromLuma;
            av1PictureParameters.film_grain.ar_coeff_lag = desc.filmGrain->arCoeffLag;
            av1PictureParameters.film_grain.ar_coeff_shift_minus6 = desc.filmGrain->arCoeffShiftMinus6;
            av1PictureParameters.film_grain.grain_scale_shift = desc.filmGrain->grainScaleShift;
            av1PictureParameters.film_grain.overlap_flag = desc.filmGrain->overlapFlag;
            av1PictureParameters.film_grain.clip_to_restricted_range = desc.filmGrain->clipToRestrictedRange;
            av1PictureParameters.film_grain.matrix_coeff_is_identity = desc.filmGrain->matrixCoeffIsIdentity;
            av1PictureParameters.film_grain.grain_seed = desc.filmGrain->grainSeed;
            av1PictureParameters.film_grain.num_y_points = desc.filmGrain->numYPoints;
            av1PictureParameters.film_grain.num_cb_points = desc.filmGrain->numCbPoints;
            av1PictureParameters.film_grain.num_cr_points = desc.filmGrain->numCrPoints;
            for (uint32_t i = 0; i < 14; i++) {
                av1PictureParameters.film_grain.scaling_points_y[i][0] = desc.filmGrain->pointYValue[i];
                av1PictureParameters.film_grain.scaling_points_y[i][1] = desc.filmGrain->pointYScaling[i];
            }
            for (uint32_t i = 0; i < 10; i++) {
                av1PictureParameters.film_grain.scaling_points_cb[i][0] = desc.filmGrain->pointCbValue[i];
                av1PictureParameters.film_grain.scaling_points_cb[i][1] = desc.filmGrain->pointCbScaling[i];
                av1PictureParameters.film_grain.scaling_points_cr[i][0] = desc.filmGrain->pointCrValue[i];
                av1PictureParameters.film_grain.scaling_points_cr[i][1] = desc.filmGrain->pointCrScaling[i];
            }
            std::memcpy(av1PictureParameters.film_grain.ar_coeffs_y, desc.filmGrain->arCoeffsYPlus128, sizeof(av1PictureParameters.film_grain.ar_coeffs_y));
            std::memcpy(av1PictureParameters.film_grain.ar_coeffs_cb, desc.filmGrain->arCoeffsCbPlus128, sizeof(av1PictureParameters.film_grain.ar_coeffs_cb));
            std::memcpy(av1PictureParameters.film_grain.ar_coeffs_cr, desc.filmGrain->arCoeffsCrPlus128, sizeof(av1PictureParameters.film_grain.ar_coeffs_cr));
            av1PictureParameters.film_grain.cb_mult = desc.filmGrain->cbMult;
            av1PictureParameters.film_grain.cb_luma_mult = desc.filmGrain->cbLumaMult;
            av1PictureParameters.film_grain.cr_mult = desc.filmGrain->crMult;
            av1PictureParameters.film_grain.cr_luma_mult = desc.filmGrain->crLumaMult;
            av1PictureParameters.film_grain.cb_offset = desc.filmGrain->cbOffset;
            av1PictureParameters.film_grain.cr_offset = desc.filmGrain->crOffset;
        }
        av1PictureParameters.StatusReportFeedbackNumber = 1;

        for (uint32_t i = 0; i < desc.tileNum; i++) {
            av1Tiles[i] = {};
            av1Tiles[i].DataOffset = desc.tiles[i].offset;
            av1Tiles[i].DataSize = desc.tiles[i].size;
            av1Tiles[i].row = desc.tiles[i].row;
            av1Tiles[i].column = desc.tiles[i].column;
            av1Tiles[i].anchor_frame = desc.tiles[i].anchorFrame ? desc.tiles[i].anchorFrame : 0xFF;
            av1PictureParameters.tiles.cols = std::max<UCHAR>(av1PictureParameters.tiles.cols, (UCHAR)(desc.tiles[i].column + 1));
            av1PictureParameters.tiles.rows = std::max<UCHAR>(av1PictureParameters.tiles.rows, (UCHAR)(desc.tiles[i].row + 1));
        }

        input.NumFrameArguments = 2;
        input.FrameArguments[0].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_PICTURE_PARAMETERS;
        input.FrameArguments[0].Size = sizeof(av1PictureParameters);
        input.FrameArguments[0].pData = &av1PictureParameters;
        input.FrameArguments[1].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_SLICE_CONTROL;
        input.FrameArguments[1].Size = sizeof(DXVA_Tile_AV1) * desc.tileNum;
        input.FrameArguments[1].pData = av1Tiles;
    } else {
        input.NumFrameArguments = videoDecodeDesc.argumentNum;
        for (uint32_t i = 0; i < videoDecodeDesc.argumentNum; i++) {
            if (!videoDecodeDesc.arguments[i].data || videoDecodeDesc.arguments[i].size == 0) {
                NRI_REPORT_ERROR(&device, "'arguments[%u]' has invalid data or size", i);
                return;
            }

            input.FrameArguments[i].Type = (D3D12_VIDEO_DECODE_ARGUMENT_TYPE)videoDecodeDesc.arguments[i].type;
            input.FrameArguments[i].Size = videoDecodeDesc.arguments[i].size;
            input.FrameArguments[i].pData = (void*)videoDecodeDesc.arguments[i].data;
        }
    }

    VideoPictureD3D12& dstPicture = *(VideoPictureD3D12*)videoDecodeDesc.dstPicture;
    VideoPictureD3D12& setupPicture = videoDecodeDesc.setupPicture ? *(VideoPictureD3D12*)videoDecodeDesc.setupPicture : dstPicture;
    const bool h264NeutralDecode = videoDecodeDesc.h264PictureDesc != nullptr;
    const bool h265NeutralDecode = videoDecodeDesc.h265PictureDesc != nullptr;
    const bool av1NeutralDecode = videoDecodeDesc.av1PictureDesc != nullptr;

    VideoDecodeReferenceLayoutD3D12 referenceLayout = {};
    if (!GetVideoDecodeReferenceLayoutD3D12(videoDecodeDesc.references, videoDecodeDesc.referenceNum, referenceLayout)) {
        if (referenceLayout.duplicateSlot)
            NRI_REPORT_ERROR(&device, "'references[%u].slot' duplicates an earlier D3D12 decode reference slot", referenceLayout.failingReference);
        else
            NRI_REPORT_ERROR(&device, "'references[%u].slot' exceeds the D3D12 decode PicEntry index range", referenceLayout.failingReference);
        return;
    }
    if (h264NeutralDecode)
        referenceLayout.slotCount = std::max(referenceLayout.slotCount, videoDecodeDesc.dstSlot + 1);
    if (h265NeutralDecode)
        referenceLayout.slotCount = std::max(referenceLayout.slotCount, videoDecodeDesc.dstSlot + 1);
    if (av1NeutralDecode)
        referenceLayout.slotCount = std::max(referenceLayout.slotCount, videoDecodeDesc.dstSlot + 1);

    Scratch<ID3D12Resource*> referenceResources = NRI_ALLOCATE_SCRATCH(device, ID3D12Resource*, referenceLayout.slotCount);
    Scratch<uint32_t> referenceSubresources = NRI_ALLOCATE_SCRATCH(device, uint32_t, referenceLayout.slotCount);
    for (uint32_t i = 0; i < referenceLayout.slotCount; i++) {
        referenceResources[i] = nullptr;
        referenceSubresources[i] = 0;
    }

    for (uint32_t i = 0; i < videoDecodeDesc.referenceNum; i++) {
        if (!videoDecodeDesc.references[i].picture) {
            NRI_REPORT_ERROR(&device, "'references[%u].picture' is NULL", i);
            return;
        }

        VideoPictureD3D12& reference = *(VideoPictureD3D12*)videoDecodeDesc.references[i].picture;
        const uint32_t slot = videoDecodeDesc.references[i].slot;
        referenceResources[slot] = (ID3D12Resource*)(*reference.m_Texture);
        referenceSubresources[slot] = reference.m_Subresource;
    }
    if (h264NeutralDecode) {
        referenceResources[videoDecodeDesc.dstSlot] = (ID3D12Resource*)(*setupPicture.m_Texture);
        referenceSubresources[videoDecodeDesc.dstSlot] = setupPicture.m_Subresource;
    }
    if (h265NeutralDecode) {
        referenceResources[videoDecodeDesc.dstSlot] = (ID3D12Resource*)(*setupPicture.m_Texture);
        referenceSubresources[videoDecodeDesc.dstSlot] = setupPicture.m_Subresource;
    }
    if (av1NeutralDecode) {
        referenceResources[videoDecodeDesc.dstSlot] = (ID3D12Resource*)(*setupPicture.m_Texture);
        referenceSubresources[videoDecodeDesc.dstSlot] = setupPicture.m_Subresource;
    }

    input.ReferenceFrames.NumTexture2Ds = referenceLayout.slotCount;
    input.ReferenceFrames.ppTexture2Ds = referenceLayout.slotCount ? (ID3D12Resource**)referenceResources : nullptr;
    input.ReferenceFrames.pSubresources = referenceLayout.slotCount ? (uint32_t*)referenceSubresources : nullptr;
    input.CompressedBitstream.pBuffer = (ID3D12Resource*)bitstream;
    input.CompressedBitstream.Offset = videoDecodeDesc.bitstream.offset;
    input.CompressedBitstream.Size = videoDecodeDesc.bitstream.size;
    input.pHeap = session.m_DecoderHeap;

    D3D12_VIDEO_DECODE_OUTPUT_STREAM_ARGUMENTS output = {};
    output.pOutputTexture2D = (ID3D12Resource*)(*dstPicture.m_Texture);
    output.OutputSubresource = dstPicture.m_Subresource;

    VideoDecodeD3D12Desc desc = {};
    desc.d3d12Decoder = session.m_Decoder;
    desc.d3d12OutputArguments = &output;
    desc.d3d12InputArguments = &input;
    commandBufferD3D12.DecodeVideo(desc);
}

static const VideoH264ReferenceDesc* FindVideoEncodeH264ReferenceDesc(const VideoH264PictureDesc* h264PictureDesc, uint32_t slot) {
    if (!h264PictureDesc)
        return nullptr;

    for (uint32_t i = 0; i < h264PictureDesc->referenceNum; i++) {
        if (h264PictureDesc->references[i].slot == slot)
            return &h264PictureDesc->references[i];
    }

    return nullptr;
}

static D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE GetVideoEncodeAV1FrameTypeD3D12(VideoEncodeFrameType frameType) {
    switch (frameType) {
        case VideoEncodeFrameType::IDR:
        case VideoEncodeFrameType::I:
            return D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_KEY_FRAME;
        case VideoEncodeFrameType::P:
        case VideoEncodeFrameType::B:
            return D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_INTER_FRAME;
        case VideoEncodeFrameType::MAX_NUM:
            return (D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE)-1;
    }

    return (D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE)-1;
}

static uint32_t GetVideoEncodeAV1ReferenceNameIndexD3D12(VideoAV1ReferenceName name) {
    switch (name) {
        case VideoAV1ReferenceName::NONE:
            return 7;
        case VideoAV1ReferenceName::LAST:
            return 0;
        case VideoAV1ReferenceName::LAST2:
            return 1;
        case VideoAV1ReferenceName::LAST3:
            return 2;
        case VideoAV1ReferenceName::GOLDEN:
            return 3;
        case VideoAV1ReferenceName::BWDREF:
            return 4;
        case VideoAV1ReferenceName::ALTREF2:
            return 5;
        case VideoAV1ReferenceName::ALTREF:
            return 6;
        case VideoAV1ReferenceName::MAX_NUM:
            return 7;
    }

    return 7;
}

static bool HasVideoEncodeAV1DPBSlotResourceD3D12(const uint32_t* dpbSlotResourceIndices, uint32_t resourceIndex) {
    for (uint32_t i = 0; i < 8; i++) {
        if (dpbSlotResourceIndices[i] == resourceIndex)
            return true;
    }

    return false;
}

static_assert(offsetof(VideoEncodeFeedback, errorFlags) == offsetof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA, EncodeErrorFlags));
static_assert(offsetof(VideoEncodeFeedback, encodedBitstreamWrittenBytes) == offsetof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA, EncodedBitstreamWrittenBytesCount));
static_assert(offsetof(VideoEncodeFeedback, writtenSubregionNum) == offsetof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA, WrittenSubregionsCount));

static void NRI_CALL CmdEncodeVideo(CommandBuffer& commandBuffer, const VideoEncodeDesc& videoEncodeDesc) {
    CommandBufferD3D12& commandBufferD3D12 = (CommandBufferD3D12&)commandBuffer;
    DeviceD3D12& device = commandBufferD3D12.GetDevice();

    if (!videoEncodeDesc.session || !videoEncodeDesc.parameters || !videoEncodeDesc.srcPicture || !videoEncodeDesc.dstBitstream.buffer || !videoEncodeDesc.dstBitstream.size || !videoEncodeDesc.metadata) {
        NRI_REPORT_ERROR(&device, "'session', 'parameters', 'srcPicture', 'dstBitstream.buffer', 'dstBitstream.size' and 'metadata' must be valid");
        return;
    }

    if (videoEncodeDesc.referenceNum != 0 && !videoEncodeDesc.references) {
        NRI_REPORT_ERROR(&device, "'references' is NULL");
        return;
    }

    VideoSessionD3D12& session = *(VideoSessionD3D12*)videoEncodeDesc.session;
    if (videoEncodeDesc.h264PictureDesc && session.m_Desc.codec != VideoCodec::H264) {
        NRI_REPORT_ERROR(&device, "'h264PictureDesc' can only be used with H.264 sessions");
        return;
    }
    if (videoEncodeDesc.h264PictureDesc && videoEncodeDesc.h264PictureDesc->referenceNum != 0 && !videoEncodeDesc.h264PictureDesc->references) {
        NRI_REPORT_ERROR(&device, "'h264PictureDesc->references' is NULL");
        return;
    }
    if (videoEncodeDesc.av1PictureDesc && session.m_Desc.codec != VideoCodec::AV1) {
        NRI_REPORT_ERROR(&device, "'av1PictureDesc' can only be used with AV1 sessions");
        return;
    }
    if (videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->referenceNum != 0 && !videoEncodeDesc.av1PictureDesc->references) {
        NRI_REPORT_ERROR(&device, "'av1PictureDesc->references' is NULL");
        return;
    }
    if (videoEncodeDesc.av1PictureDesc && (videoEncodeDesc.av1PictureDesc->referenceNum != 0) != (videoEncodeDesc.referenceNum != 0)) {
        NRI_REPORT_ERROR(&device, "'av1PictureDesc->referenceNum' must match whether 'references' are provided");
        return;
    }

    VideoSessionParametersD3D12& parameters = *(VideoSessionParametersD3D12*)videoEncodeDesc.parameters;
    if (parameters.m_Session != &session) {
        NRI_REPORT_ERROR(&device, "'parameters' must belong to 'session'");
        return;
    }

    BufferD3D12& dstBitstream = *(BufferD3D12*)videoEncodeDesc.dstBitstream.buffer;
    if (videoEncodeDesc.dstBitstream.offset >= dstBitstream.GetDesc().size || videoEncodeDesc.dstBitstream.size > dstBitstream.GetDesc().size - videoEncodeDesc.dstBitstream.offset) {
        NRI_REPORT_ERROR(&device, "'dstBitstream' range is outside of 'dstBitstream.buffer'");
        return;
    }

    if (videoEncodeDesc.h265ReferenceDescs && session.m_Desc.codec != VideoCodec::H265) {
        NRI_REPORT_ERROR(&device, "'h265ReferenceDescs' can only be used with H.265 sessions");
        return;
    }

    if (!session.m_Encoder || !session.m_EncoderHeap) {
        NRI_REPORT_ERROR(&device, "'session' is not an encode session");
        return;
    }
    if (session.m_Desc.codec == VideoCodec::H264 && videoEncodeDesc.referenceNum) {
        if (!videoEncodeDesc.h264PictureDesc) {
            NRI_REPORT_ERROR(&device, "'h264PictureDesc' must be valid when H.264 encode uses references");
            return;
        }
        if (videoEncodeDesc.h264PictureDesc->referenceNum != videoEncodeDesc.referenceNum) {
            NRI_REPORT_ERROR(&device, "'h264PictureDesc->referenceNum' must match 'referenceNum'");
            return;
        }
    }

    Scratch<ID3D12Resource*> referenceResources = NRI_ALLOCATE_SCRATCH(device, ID3D12Resource*, videoEncodeDesc.referenceNum);
    Scratch<uint32_t> referenceSubresources = NRI_ALLOCATE_SCRATCH(device, uint32_t, videoEncodeDesc.referenceNum);
    Scratch<UINT> h264List0References = NRI_ALLOCATE_SCRATCH(device, UINT, videoEncodeDesc.referenceNum);
    Scratch<UINT> h264List1References = NRI_ALLOCATE_SCRATCH(device, UINT, videoEncodeDesc.referenceNum);
    Scratch<D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_H264> h264ReferenceDescriptors = NRI_ALLOCATE_SCRATCH(device, D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_H264, videoEncodeDesc.referenceNum);
    uint32_t h264List0ReferenceNum = 0;
    uint32_t h264List1ReferenceNum = 0;
    for (uint32_t i = 0; i < videoEncodeDesc.referenceNum; i++) {
        if (!videoEncodeDesc.references[i].picture) {
            NRI_REPORT_ERROR(&device, "'references[%u].picture' is NULL", i);
            return;
        }

        VideoPictureD3D12& reference = *(VideoPictureD3D12*)videoEncodeDesc.references[i].picture;
        referenceResources[i] = (ID3D12Resource*)(*reference.m_Texture);
        referenceSubresources[i] = reference.m_Subresource;

        if (session.m_Desc.codec == VideoCodec::H264) {
            const VideoH264ReferenceDesc* referenceDesc = FindVideoEncodeH264ReferenceDesc(videoEncodeDesc.h264PictureDesc, videoEncodeDesc.references[i].slot);
            if (!referenceDesc) {
                NRI_REPORT_ERROR(&device, "'references[%u].slot' is not described by 'h264PictureDesc'", i);
                return;
            }

            if (referenceDesc->listIndex == 0)
                h264List0References[h264List0ReferenceNum++] = i;
            else if (referenceDesc->listIndex == 1)
                h264List1References[h264List1ReferenceNum++] = i;
            else {
                NRI_REPORT_ERROR(&device, "'h264PictureDesc->references' listIndex must be 0 or 1");
                return;
            }

            h264ReferenceDescriptors[i] = {};
            h264ReferenceDescriptors[i].ReconstructedPictureResourceIndex = i;
            h264ReferenceDescriptors[i].IsLongTermReference = referenceDesc->longTermReference != 0;
            h264ReferenceDescriptors[i].LongTermPictureIdx = referenceDesc->longTermPictureIndex;
            h264ReferenceDescriptors[i].PictureOrderCountNumber = referenceDesc->pictureOrderCount;
            h264ReferenceDescriptors[i].FrameDecodingOrderNumber = referenceDesc->frameNum;
            h264ReferenceDescriptors[i].TemporalLayerIndex = referenceDesc->temporalLayer;
        }
    }

    const VideoEncodeRateControlDesc defaultRateControl = {VideoEncodeRateControlMode::CQP, 26, 28, 30, 30, 1};
    const VideoEncodeRateControlDesc& rateControlDesc = videoEncodeDesc.rateControlDesc ? *videoEncodeDesc.rateControlDesc : defaultRateControl;
    if (rateControlDesc.mode != VideoEncodeRateControlMode::CQP) {
        NRI_REPORT_ERROR(&device, "Unsupported video encode rate control mode");
        return;
    }

    D3D12_VIDEO_ENCODER_RATE_CONTROL_CQP cqp = {rateControlDesc.qpI, rateControlDesc.qpP, rateControlDesc.qpB};
    D3D12_VIDEO_ENCODER_RATE_CONTROL rateControl = {};
    rateControl.Mode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CQP;
    rateControl.ConfigParams.DataSize = sizeof(cqp);
    rateControl.ConfigParams.pConfiguration_CQP = &cqp;
    rateControl.TargetFrameRate = {rateControlDesc.frameRateNumerator ? rateControlDesc.frameRateNumerator : 30, rateControlDesc.frameRateDenominator ? rateControlDesc.frameRateDenominator : 1};

    D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_H264 h264Gop = {};
    h264Gop.GOPLength = videoEncodeDesc.referenceNum ? 60 : 1;
    h264Gop.PPicturePeriod = 1;

    D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_HEVC hevcGop = {};
    hevcGop.GOPLength = session.m_Desc.maxReferenceNum ? 60 : 1;
    hevcGop.PPicturePeriod = session.m_Desc.maxReferenceNum > 1 ? 2 : 1;

    D3D12_VIDEO_ENCODER_AV1_SEQUENCE_STRUCTURE av1Sequence = {};
    av1Sequence.IntraDistance = session.m_Desc.maxReferenceNum ? 60 : 1;
    av1Sequence.InterFramePeriod = session.m_Desc.maxReferenceNum ? 1 : 0;

    D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE gop = {};
    if (session.m_Desc.codec == VideoCodec::H264) {
        gop.DataSize = sizeof(h264Gop);
        gop.pH264GroupOfPictures = &h264Gop;
    } else if (session.m_Desc.codec == VideoCodec::H265) {
        gop.DataSize = sizeof(hevcGop);
        gop.pHEVCGroupOfPictures = &hevcGop;
    } else if (session.m_Desc.codec == VideoCodec::AV1) {
        gop.DataSize = sizeof(av1Sequence);
        gop.pAV1SequenceStructure = &av1Sequence;
    } else {
        NRI_REPORT_ERROR(&device, "Unsupported video encode codec");
        return;
    }

    D3D12_VIDEO_ENCODER_SEQUENCE_CONTROL_DESC sequenceControl = {};
    sequenceControl.Flags = D3D12_VIDEO_ENCODER_SEQUENCE_CONTROL_FLAG_NONE;
    sequenceControl.RateControl = rateControl;
    sequenceControl.PictureTargetResolution = {session.m_Desc.width, session.m_Desc.height};
    sequenceControl.SelectedLayoutMode = D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_FULL_FRAME;
    sequenceControl.CodecGopSequence = gop;
    D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_TILES av1Tiles = {};
    if (session.m_Desc.codec == VideoCodec::AV1) {
        const VideoAV1TileLayoutDesc* tileLayout = videoEncodeDesc.av1PictureDesc ? videoEncodeDesc.av1PictureDesc->tileLayout : nullptr;
        if (tileLayout && (!tileLayout->columnNum || !tileLayout->rowNum || tileLayout->columnNum > 64 || tileLayout->rowNum > 64)) {
            NRI_REPORT_ERROR(&device, "'av1PictureDesc->tileLayout' is invalid");
            return;
        }
        av1Tiles.RowCount = tileLayout ? tileLayout->rowNum : 1;
        av1Tiles.ColCount = tileLayout ? tileLayout->columnNum : 1;
        sequenceControl.FrameSubregionsLayoutData.DataSize = sizeof(av1Tiles);
        sequenceControl.FrameSubregionsLayoutData.pTilesPartition_AV1 = &av1Tiles;
    }

    const VideoEncodePictureDesc defaultPicture = {VideoEncodeFrameType::IDR, 0, 0, 0, 0};
    const VideoEncodePictureDesc& pictureDesc = videoEncodeDesc.pictureDesc ? *videoEncodeDesc.pictureDesc : defaultPicture;
    if (!IsVideoEncodeFrameTypeSupportedByD3D12NoBGop(session.m_Desc.codec, pictureDesc.frameType)) {
        NRI_REPORT_ERROR(&device, "D3D12 H.264 encode sessions are configured without B-frame GOP support");
        return;
    }

    D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA_H264 h264Picture = {};
    switch (pictureDesc.frameType) {
        case VideoEncodeFrameType::IDR:
            h264Picture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_H264_IDR_FRAME;
            break;
        case VideoEncodeFrameType::I:
            h264Picture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_H264_I_FRAME;
            break;
        case VideoEncodeFrameType::P:
            h264Picture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_H264_P_FRAME;
            break;
        case VideoEncodeFrameType::B:
            h264Picture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_H264_B_FRAME;
            break;
        case VideoEncodeFrameType::MAX_NUM:
            NRI_REPORT_ERROR(&device, "Unsupported video encode frame type");
            return;
    }
    h264Picture.pic_parameter_set_id = videoEncodeDesc.h264PictureDesc ? videoEncodeDesc.h264PictureDesc->pictureParameterSetId : 0;
    h264Picture.idr_pic_id = pictureDesc.idrPictureId;
    h264Picture.PictureOrderCountNumber = pictureDesc.pictureOrderCount;
    h264Picture.FrameDecodingOrderNumber = pictureDesc.frameIndex;
    h264Picture.TemporalLayerIndex = pictureDesc.temporalLayer;
    h264Picture.List0ReferenceFramesCount = h264List0ReferenceNum;
    h264Picture.pList0ReferenceFrames = h264List0ReferenceNum ? (UINT*)h264List0References : nullptr;
    h264Picture.List1ReferenceFramesCount = h264List1ReferenceNum;
    h264Picture.pList1ReferenceFrames = h264List1ReferenceNum ? (UINT*)h264List1References : nullptr;
    h264Picture.ReferenceFramesReconPictureDescriptorsCount = videoEncodeDesc.referenceNum;
    h264Picture.pReferenceFramesReconPictureDescriptors = videoEncodeDesc.referenceNum ? (D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_H264*)h264ReferenceDescriptors : nullptr;

    D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA_HEVC hevcPicture = {};
    switch (pictureDesc.frameType) {
        case VideoEncodeFrameType::IDR:
            hevcPicture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_IDR_FRAME;
            break;
        case VideoEncodeFrameType::I:
            hevcPicture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_I_FRAME;
            break;
        case VideoEncodeFrameType::P:
            hevcPicture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_P_FRAME;
            break;
        case VideoEncodeFrameType::B:
            hevcPicture.FrameType = D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_B_FRAME;
            break;
        case VideoEncodeFrameType::MAX_NUM:
            NRI_REPORT_ERROR(&device, "Unsupported video encode frame type");
            return;
    }
    if (session.m_Desc.codec == VideoCodec::H265 && videoEncodeDesc.referenceNum > 15) {
        NRI_REPORT_ERROR(&device, "'referenceNum' exceeds the H.265 reference list size");
        return;
    }

    Scratch<UINT> hevcList0References = NRI_ALLOCATE_SCRATCH(device, UINT, videoEncodeDesc.referenceNum ? videoEncodeDesc.referenceNum : 1);
    Scratch<UINT> hevcList1References = NRI_ALLOCATE_SCRATCH(device, UINT, videoEncodeDesc.referenceNum ? videoEncodeDesc.referenceNum : 1);
    Scratch<D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_HEVC> hevcReferenceDescriptors = NRI_ALLOCATE_SCRATCH(device, D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_HEVC, videoEncodeDesc.referenceNum ? videoEncodeDesc.referenceNum : 1);
    if (session.m_Desc.codec == VideoCodec::H265) {
        VideoEncodeHEVCReferenceListsD3D12 hevcReferenceLists = {};
        if (!BuildVideoEncodeHEVCReferenceListsD3D12(videoEncodeDesc.references, videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, pictureDesc.frameType,
                pictureDesc.pictureOrderCount, hevcReferenceLists)) {
            if (hevcReferenceLists.missingDescriptor)
                NRI_REPORT_ERROR(&device, "'h265ReferenceDescs' must describe every H.265 reference");
            else if (hevcReferenceLists.invalidPictureOrderCount)
                NRI_REPORT_ERROR(&device, "'h265ReferenceDescs[%u].pictureOrderCount' is invalid for the current H.265 frame type", hevcReferenceLists.failingReference);
            else
                NRI_REPORT_ERROR(&device, "'referenceNum' exceeds the H.265 reference list size");
            return;
        }
        for (uint32_t i = 0; i < hevcReferenceLists.list0Num; i++)
            hevcList0References[i] = hevcReferenceLists.list0[i];
        for (uint32_t i = 0; i < hevcReferenceLists.list1Num; i++)
            hevcList1References[i] = hevcReferenceLists.list1[i];

        for (uint32_t i = 0; i < videoEncodeDesc.referenceNum; i++) {
            const VideoH265ReferenceDesc* referenceDesc = FindVideoH265ReferenceDescD3D12(videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, videoEncodeDesc.references[i].slot);
            hevcReferenceDescriptors[i] = {};
            hevcReferenceDescriptors[i].ReconstructedPictureResourceIndex = i;
            hevcReferenceDescriptors[i].IsRefUsedByCurrentPic = TRUE;
            hevcReferenceDescriptors[i].IsLongTermReference = referenceDesc && referenceDesc->longTerm;
            hevcReferenceDescriptors[i].PictureOrderCountNumber = referenceDesc ? (UINT)referenceDesc->pictureOrderCount : videoEncodeDesc.references[i].slot;
            hevcReferenceDescriptors[i].TemporalLayerIndex = referenceDesc ? referenceDesc->temporalLayer : 0;
        }

        hevcPicture.slice_pic_parameter_set_id = 0;
        hevcPicture.PictureOrderCountNumber = (UINT)pictureDesc.pictureOrderCount;
        hevcPicture.TemporalLayerIndex = pictureDesc.temporalLayer;
        hevcPicture.List0ReferenceFramesCount = hevcReferenceLists.list0Num;
        hevcPicture.pList0ReferenceFrames = hevcReferenceLists.list0Num ? (UINT*)hevcList0References : nullptr;
        hevcPicture.List1ReferenceFramesCount = hevcReferenceLists.list1Num;
        hevcPicture.pList1ReferenceFrames = hevcReferenceLists.list1Num ? (UINT*)hevcList1References : nullptr;
        hevcPicture.ReferenceFramesReconPictureDescriptorsCount = videoEncodeDesc.referenceNum;
        hevcPicture.pReferenceFramesReconPictureDescriptors = videoEncodeDesc.referenceNum ? (D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_HEVC*)hevcReferenceDescriptors : nullptr;
    }

    D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_CODEC_DATA av1Picture = {};
    if (session.m_Desc.codec == VideoCodec::AV1) {
        D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE frameType = GetVideoEncodeAV1FrameTypeD3D12(pictureDesc.frameType);
        if (frameType == (D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE)-1) {
            NRI_REPORT_ERROR(&device, "Unsupported AV1 video encode frame type");
            return;
        }

        for (auto& referenceDescriptor : av1Picture.ReferenceFramesReconPictureDescriptors)
            referenceDescriptor.ReconstructedPictureResourceIndex = 0xFF;
        std::array<bool, 7> activeReferenceNames = {};
        std::array<uint32_t, 7> referenceNameSlots = {};
        for (uint32_t& slot : referenceNameSlots)
            slot = UINT32_MAX;
        std::array<bool, 7> av1ReferenceNameSpecified = {};
        std::array<uint32_t, 8> av1DPBSlotResourceIndices = {};
        for (uint32_t& resourceIndex : av1DPBSlotResourceIndices)
            resourceIndex = UINT32_MAX;

        const VideoAV1PictureBits pictureFlags = videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->flags != VideoAV1PictureBits::NONE
            ? videoEncodeDesc.av1PictureDesc->flags
            : GetDefaultVideoAV1PictureFlags(true);
        if (pictureFlags & VideoAV1PictureBits::ERROR_RESILIENT_MODE)
            av1Picture.Flags |= D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_FLAG_ENABLE_ERROR_RESILIENT_MODE;
        if (session.m_AV1FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_RESTORATION_FILTER) {
            for (auto& type : av1Picture.FrameRestorationConfig.FrameRestorationType)
                type = D3D12_VIDEO_ENCODER_AV1_RESTORATION_TYPE_DISABLED;
            for (auto& tileSize : av1Picture.FrameRestorationConfig.LoopRestorationPixelSize)
                tileSize = D3D12_VIDEO_ENCODER_AV1_RESTORATION_TILESIZE_DISABLED;
        }
        if ((session.m_AV1FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FORCED_INTEGER_MOTION_VECTORS) && (pictureFlags & VideoAV1PictureBits::FORCE_INTEGER_MV))
            av1Picture.Flags |= D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_FLAG_FORCE_INTEGER_MOTION_VECTORS;
        if (session.m_AV1FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_AUTO_SEGMENTATION)
            av1Picture.Flags |= D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_FLAG_ENABLE_FRAME_SEGMENTATION_AUTO;
        av1Picture.FrameType = frameType;
        av1Picture.CompoundPredictionType = D3D12_VIDEO_ENCODER_AV1_COMP_PREDICTION_TYPE_SINGLE_REFERENCE;
        av1Picture.InterpolationFilter = videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->interpolationFilter
            ? (D3D12_VIDEO_ENCODER_AV1_INTERPOLATION_FILTERS)videoEncodeDesc.av1PictureDesc->interpolationFilter
            : D3D12_VIDEO_ENCODER_AV1_INTERPOLATION_FILTERS_SWITCHABLE;
        av1Picture.TxMode = videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->txMode
            ? (D3D12_VIDEO_ENCODER_AV1_TX_MODE)videoEncodeDesc.av1PictureDesc->txMode
            : (pictureDesc.frameType == VideoEncodeFrameType::P ? D3D12_VIDEO_ENCODER_AV1_TX_MODE_SELECT : D3D12_VIDEO_ENCODER_AV1_TX_MODE_LARGEST);
        av1Picture.OrderHint = videoEncodeDesc.av1PictureDesc ? videoEncodeDesc.av1PictureDesc->orderHint : (UINT)pictureDesc.pictureOrderCount;
        av1Picture.PictureIndex = videoEncodeDesc.av1PictureDesc ? videoEncodeDesc.av1PictureDesc->currentFrameId : pictureDesc.frameIndex;
        av1Picture.TemporalLayerIndexPlus1 = pictureDesc.temporalLayer + 1;
        av1Picture.SpatialLayerIndexPlus1 = 1;
        av1Picture.PrimaryRefFrame = 7;
        av1Picture.RefreshFrameFlags = videoEncodeDesc.av1PictureDesc ? videoEncodeDesc.av1PictureDesc->refreshFrameFlags : (pictureDesc.frameType == VideoEncodeFrameType::IDR ? 0xFF : 0);
        if (frameType == D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_KEY_FRAME) {
            av1Picture.PrimaryRefFrame = 7;
            av1Picture.RefreshFrameFlags = 0xFF;
            if (videoEncodeDesc.referenceNum) {
                NRI_REPORT_ERROR(&device, "AV1 key frames must not reference previous pictures");
                return;
            }
        }
        av1Picture.Quantization.BaseQIndex = videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->baseQIndex
            ? videoEncodeDesc.av1PictureDesc->baseQIndex
            : GetVideoEncodeQPByFrameTypeD3D12(rateControlDesc, pictureDesc.frameType);
        if (session.m_AV1FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_QUANTIZATION_DELTAS)
            av1Picture.QuantizationDelta.DeltaQPresent = !!(pictureFlags & VideoAV1PictureBits::DELTA_Q_PRESENT);
        if (session.m_AV1FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_FILTER_DELTAS) {
            av1Picture.LoopFilter.LoopFilterDeltaEnabled = 1;
            av1Picture.LoopFilter.UpdateRefDelta = 1;
            av1Picture.LoopFilter.RefDeltas[0] = 1;
            av1Picture.LoopFilter.RefDeltas[4] = -1;
            av1Picture.LoopFilter.RefDeltas[6] = -1;
            av1Picture.LoopFilter.RefDeltas[7] = -1;
        }
        if (session.m_AV1FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_CDEF_FILTERING)
            av1Picture.CDEF.CdefDampingMinus3 = videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->cdefDampingMinus3 ? videoEncodeDesc.av1PictureDesc->cdefDampingMinus3 : 3;

        if (videoEncodeDesc.av1PictureDesc) {
            if (videoEncodeDesc.av1PictureDesc->referenceNum > 8) {
                NRI_REPORT_ERROR(&device, "'av1PictureDesc->referenceNum' exceeds AV1 DPB slot count");
                return;
            }

            for (uint32_t i = 0; i < videoEncodeDesc.av1PictureDesc->referenceNum; i++) {
                const VideoAV1ReferenceDesc& reference = videoEncodeDesc.av1PictureDesc->references[i];
                const uint32_t referenceNameIndex = GetVideoEncodeAV1ReferenceNameIndexD3D12(reference.name);

                uint32_t resourceIndex = UINT32_MAX;
                for (uint32_t j = 0; j < videoEncodeDesc.referenceNum; j++) {
                    if (videoEncodeDesc.references[j].slot == reference.slot) {
                        resourceIndex = j;
                        break;
                    }
                }
                if (resourceIndex == UINT32_MAX) {
                    NRI_REPORT_ERROR(&device, "'av1PictureDesc->references[%u].slot' is not present in 'references'", i);
                    return;
                }
                if (reference.refFrameIndex >= 8) {
                    NRI_REPORT_ERROR(&device, "'av1PictureDesc->references[%u].refFrameIndex' is invalid", i);
                    return;
                }

                av1Picture.ReferenceFramesReconPictureDescriptors[reference.refFrameIndex] = {};
                av1Picture.ReferenceFramesReconPictureDescriptors[reference.refFrameIndex].ReconstructedPictureResourceIndex = resourceIndex;
                av1Picture.ReferenceFramesReconPictureDescriptors[reference.refFrameIndex].TemporalLayerIndexPlus1 = reference.frameType == VideoEncodeFrameType::MAX_NUM ? 0 : 1;
                av1Picture.ReferenceFramesReconPictureDescriptors[reference.refFrameIndex].SpatialLayerIndexPlus1 = 1;
                av1Picture.ReferenceFramesReconPictureDescriptors[reference.refFrameIndex].FrameType = GetVideoEncodeAV1FrameTypeD3D12(reference.frameType);
                av1Picture.ReferenceFramesReconPictureDescriptors[reference.refFrameIndex].OrderHint = reference.orderHint;
                av1Picture.ReferenceFramesReconPictureDescriptors[reference.refFrameIndex].PictureIndex = reference.frameId;
                av1DPBSlotResourceIndices[reference.refFrameIndex] = resourceIndex;
                if (reference.name != VideoAV1ReferenceName::NONE) {
                    if (referenceNameIndex >= 7) {
                        NRI_REPORT_ERROR(&device, "'av1PictureDesc->references[%u].name' is invalid", i);
                        return;
                    }

                    av1Picture.ReferenceIndices[referenceNameIndex] = reference.refFrameIndex;
                    activeReferenceNames[referenceNameIndex] = true;
                    av1ReferenceNameSpecified[referenceNameIndex] = true;
                    referenceNameSlots[referenceNameIndex] = reference.slot;
                }
            }

            // Unspecified AV1 reference names must resolve to an invalid DPB descriptor, otherwise D3D12 treats them as active references.
            uint32_t invalidReferenceIndex = UINT32_MAX;
            for (uint32_t i = 0; i < 8; i++) {
                if (av1Picture.ReferenceFramesReconPictureDescriptors[i].ReconstructedPictureResourceIndex == 0xFF) {
                    invalidReferenceIndex = i;
                    break;
                }
            }
            for (uint32_t i = 0; i < 7; i++) {
                if (av1ReferenceNameSpecified[i])
                    continue;

                if (invalidReferenceIndex == UINT32_MAX) {
                    NRI_REPORT_ERROR(&device, "AV1 DPB snapshot has no invalid slot for unused reference names");
                    return;
                }

                av1Picture.ReferenceIndices[i] = invalidReferenceIndex;
            }

            for (uint32_t i = 0; i < videoEncodeDesc.referenceNum; i++) {
                if (!HasVideoEncodeAV1DPBSlotResourceD3D12(av1DPBSlotResourceIndices.data(), i)) {
                    NRI_REPORT_ERROR(&device, "'references[%u].slot' is not present in the AV1 DPB snapshot", i);
                    return;
                }
            }

            const uint32_t primaryReferenceNameIndex = GetVideoEncodeAV1ReferenceNameIndexD3D12(videoEncodeDesc.av1PictureDesc->primaryReferenceName);
            if (primaryReferenceNameIndex < 7 && !activeReferenceNames[primaryReferenceNameIndex]) {
                NRI_REPORT_ERROR(&device, "'av1PictureDesc->primaryReferenceName' does not name an active reference");
                return;
            }
            av1Picture.PrimaryRefFrame = primaryReferenceNameIndex;
        } else if (videoEncodeDesc.referenceNum) {
            av1Picture.ReferenceFramesReconPictureDescriptors[0] = {};
            av1Picture.ReferenceFramesReconPictureDescriptors[0].ReconstructedPictureResourceIndex = 0;
            av1Picture.ReferenceFramesReconPictureDescriptors[0].TemporalLayerIndexPlus1 = 1;
            av1Picture.ReferenceFramesReconPictureDescriptors[0].SpatialLayerIndexPlus1 = 1;
            av1Picture.ReferenceFramesReconPictureDescriptors[0].FrameType = D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_KEY_FRAME;
            av1Picture.ReferenceIndices[0] = 0;
            av1Picture.PrimaryRefFrame = 0;
        }
    }

    D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA pictureCodecData = {};
    if (session.m_Desc.codec == VideoCodec::H264) {
        pictureCodecData.DataSize = sizeof(h264Picture);
        pictureCodecData.pH264PicData = &h264Picture;
    } else if (session.m_Desc.codec == VideoCodec::H265) {
        pictureCodecData.DataSize = sizeof(hevcPicture);
        pictureCodecData.pHEVCPicData = &hevcPicture;
    } else {
        pictureCodecData.DataSize = sizeof(av1Picture);
        pictureCodecData.pAV1PicData = &av1Picture;
    }

    const bool isAV1NonReferencePicture = session.m_Desc.codec == VideoCodec::AV1 && av1Picture.RefreshFrameFlags == 0;
    if (session.m_Desc.codec == VideoCodec::AV1 && !isAV1NonReferencePicture && !videoEncodeDesc.reconstructedPicture) {
        NRI_REPORT_ERROR(&device, "AV1 frames that refresh DPB slots require 'reconstructedPicture'");
        return;
    }

    const bool isUsedAsReferencePicture = IsVideoEncodePictureUsedAsReferenceD3D12(session.m_Desc.codec, session.m_Desc.maxReferenceNum,
        videoEncodeDesc.reconstructedPicture != nullptr, (uint8_t)av1Picture.RefreshFrameFlags);

    D3D12_VIDEO_ENCODER_PICTURE_CONTROL_DESC pictureControl = {};
    if (isUsedAsReferencePicture)
        pictureControl.Flags |= D3D12_VIDEO_ENCODER_PICTURE_CONTROL_FLAG_USED_AS_REFERENCE_PICTURE;
    pictureControl.PictureControlCodecData = pictureCodecData;
    pictureControl.ReferenceFrames.NumTexture2Ds = videoEncodeDesc.referenceNum;
    pictureControl.ReferenceFrames.ppTexture2Ds = referenceResources;
    pictureControl.ReferenceFrames.pSubresources = referenceSubresources;

    D3D12_VIDEO_ENCODER_ENCODEFRAME_INPUT_ARGUMENTS input = {};
    input.SequenceControlDesc = sequenceControl;
    input.PictureControlDesc = pictureControl;
    input.CurrentFrameBitstreamMetadataSize = (UINT)videoEncodeDesc.bitstreamMetadataSize;
    VideoPictureD3D12& srcPicture = *(VideoPictureD3D12*)videoEncodeDesc.srcPicture;
    input.pInputFrame = (ID3D12Resource*)(*srcPicture.m_Texture);
    input.InputFrameSubresource = srcPicture.m_Subresource;

    D3D12_VIDEO_ENCODER_ENCODEFRAME_OUTPUT_ARGUMENTS output = {};
    output.Bitstream.pBuffer = (ID3D12Resource*)dstBitstream;
    output.Bitstream.FrameStartOffset = videoEncodeDesc.dstBitstream.offset;
    if (videoEncodeDesc.reconstructedPicture) {
        VideoPictureD3D12& reconstructedPicture = *(VideoPictureD3D12*)videoEncodeDesc.reconstructedPicture;
        output.ReconstructedPicture.pReconstructedPicture = (ID3D12Resource*)(*reconstructedPicture.m_Texture);
        output.ReconstructedPicture.ReconstructedPictureSubresource = reconstructedPicture.m_Subresource;
    }
    output.EncoderOutputMetadata.pBuffer = (ID3D12Resource*)(*(BufferD3D12*)videoEncodeDesc.metadata);
    output.EncoderOutputMetadata.Offset = videoEncodeDesc.metadataOffset;

    D3D12_VIDEO_ENCODER_PROFILE_H264 resolveH264Profile = D3D12_VIDEO_ENCODER_PROFILE_H264_HIGH;
    D3D12_VIDEO_ENCODER_PROFILE_HEVC resolveHevcProfile = session.m_Desc.format == Format::P010_UNORM || session.m_Desc.format == Format::P016_UNORM
        ? D3D12_VIDEO_ENCODER_PROFILE_HEVC_MAIN10
        : D3D12_VIDEO_ENCODER_PROFILE_HEVC_MAIN;
    D3D12_VIDEO_ENCODER_AV1_PROFILE resolveAv1Profile = D3D12_VIDEO_ENCODER_AV1_PROFILE_MAIN;
    D3D12_VIDEO_ENCODER_PROFILE_DESC resolveProfile = {};
    if (session.m_Desc.codec == VideoCodec::H264) {
        resolveProfile.DataSize = sizeof(resolveH264Profile);
        resolveProfile.pH264Profile = &resolveH264Profile;
    } else if (session.m_Desc.codec == VideoCodec::H265) {
        resolveProfile.DataSize = sizeof(resolveHevcProfile);
        resolveProfile.pHEVCProfile = &resolveHevcProfile;
    } else {
        resolveProfile.DataSize = sizeof(resolveAv1Profile);
        resolveProfile.pAV1Profile = &resolveAv1Profile;
    }

    D3D12_VIDEO_ENCODER_RESOLVE_METADATA_INPUT_ARGUMENTS resolveInput = {};
    D3D12_VIDEO_ENCODER_RESOLVE_METADATA_OUTPUT_ARGUMENTS resolveOutput = {};
    if (videoEncodeDesc.resolvedMetadata) {
        resolveInput.EncoderCodec = GetVideoEncodeCodecD3D12(session.m_Desc.codec);
        resolveInput.EncoderProfile = resolveProfile;
        resolveInput.EncoderInputFormat = GetDxgiFormat(session.m_Desc.format).typed;
        resolveInput.EncodedPictureEffectiveResolution = {session.m_Desc.width, session.m_Desc.height};
        resolveInput.HWLayoutMetadata = output.EncoderOutputMetadata;
        resolveOutput.ResolvedLayoutMetadata.pBuffer = (ID3D12Resource*)(*(BufferD3D12*)videoEncodeDesc.resolvedMetadata);
        resolveOutput.ResolvedLayoutMetadata.Offset = videoEncodeDesc.resolvedMetadataOffset;
    }

    VideoEncodeD3D12Desc desc = {};
    desc.d3d12Encoder = session.m_Encoder;
    desc.d3d12Heap = session.m_EncoderHeap;
    desc.d3d12InputArguments = &input;
    desc.d3d12OutputArguments = &output;
    desc.d3d12ResolveMetadataInputArguments = videoEncodeDesc.resolvedMetadata ? &resolveInput : nullptr;
    desc.d3d12ResolveMetadataOutputArguments = videoEncodeDesc.resolvedMetadata ? &resolveOutput : nullptr;
    commandBufferD3D12.EncodeVideo(desc);
}

static void NRI_CALL CmdResolveVideoEncodeFeedback(CommandBuffer&, VideoSession&, Buffer&, uint64_t) {
}

static Result NRI_CALL GetVideoEncodeFeedback(VideoSession&, Buffer& resolvedMetadataReadback, uint64_t resolvedMetadataOffset, VideoEncodeFeedback& feedback) {
    const void* metadata = ((BufferD3D12&)resolvedMetadataReadback).Map(resolvedMetadataOffset);
    if (!metadata)
        return Result::FAILURE;

    const auto& d3d12Feedback = *(const D3D12_VIDEO_ENCODER_OUTPUT_METADATA*)metadata;
    feedback = {};
    feedback.errorFlags = d3d12Feedback.EncodeErrorFlags;
    feedback.averageQP = d3d12Feedback.EncodeStats.AverageQP;
    feedback.intraCodingUnitNum = d3d12Feedback.EncodeStats.IntraCodingUnitsCount;
    feedback.interCodingUnitNum = d3d12Feedback.EncodeStats.InterCodingUnitsCount;
    feedback.skipCodingUnitNum = d3d12Feedback.EncodeStats.SkipCodingUnitsCount;
    feedback.averageMotionEstimationX = d3d12Feedback.EncodeStats.AverageMotionEstimationXDirection;
    feedback.averageMotionEstimationY = d3d12Feedback.EncodeStats.AverageMotionEstimationYDirection;
    feedback.encodedBitstreamWrittenBytes = d3d12Feedback.EncodedBitstreamWrittenBytesCount;
    feedback.writtenSubregionNum = d3d12Feedback.WrittenSubregionsCount;
    return Result::SUCCESS;
}

struct VideoEncodeAV1TilesLayoutD3D12 {
    uint64_t rowCount;
    uint64_t colCount;
    std::array<uint64_t, 64> rowHeights;
    std::array<uint64_t, 64> colWidths;
    uint64_t contextUpdateTileId;
};

struct VideoEncodeAV1LoopFilterConfigD3D12 {
    std::array<uint64_t, 2> level;
    uint64_t levelU;
    uint64_t levelV;
    uint64_t sharpness;
    uint64_t deltaEnabled;
    uint64_t updateRefDelta;
    std::array<int64_t, 8> refDeltas;
    uint64_t updateModeDelta;
    std::array<int64_t, 2> modeDeltas;
};

struct VideoEncodeAV1LoopFilterDeltaConfigD3D12 {
    uint64_t deltaLfPresent;
    uint64_t deltaLfMulti;
    uint64_t deltaLfRes;
};

struct VideoEncodeAV1QuantizationConfigD3D12 {
    uint64_t baseQIndex;
    int64_t yDcDeltaQ;
    int64_t uDcDeltaQ;
    int64_t uAcDeltaQ;
    int64_t vDcDeltaQ;
    int64_t vAcDeltaQ;
    uint64_t usingQMatrix;
    uint64_t qmY;
    uint64_t qmU;
    uint64_t qmV;
};

struct VideoEncodeAV1QuantizationDeltaConfigD3D12 {
    uint64_t deltaQPresent;
    uint64_t deltaQRes;
};

struct VideoEncodeAV1CdefConfigD3D12 {
    uint64_t cdefBits;
    uint64_t cdefDampingMinus3;
    std::array<uint64_t, 8> yPrimaryStrength;
    std::array<uint64_t, 8> uvPrimaryStrength;
    std::array<uint64_t, 8> ySecondaryStrength;
    std::array<uint64_t, 8> uvSecondaryStrength;
};

struct VideoEncodeAV1SegmentationConfigD3D12 {
    uint64_t enabled;
    uint64_t updateMap;
    uint64_t updateData;
    uint64_t temporalUpdate;
    std::array<uint64_t, 8> enabledFeatures;
    std::array<std::array<int64_t, 8>, 8> featureValues;
};

struct VideoEncodeAV1PostEncodeValuesD3D12 {
    uint64_t compoundPredictionType;
    VideoEncodeAV1LoopFilterConfigD3D12 loopFilter;
    VideoEncodeAV1LoopFilterDeltaConfigD3D12 loopFilterDelta;
    VideoEncodeAV1QuantizationConfigD3D12 quantization;
    VideoEncodeAV1QuantizationDeltaConfigD3D12 quantizationDelta;
    VideoEncodeAV1CdefConfigD3D12 cdef;
    VideoEncodeAV1SegmentationConfigD3D12 segmentation;
    uint64_t primaryRefFrame;
    std::array<uint64_t, 7> referenceIndices;
};

static Result NRI_CALL GetVideoEncodeAV1DecodeInfo(VideoSession&, Buffer& resolvedMetadataReadback, uint64_t resolvedMetadataOffset,
    const VideoAV1EncodeDecodeInfoDesc& desc, VideoAV1EncodeDecodeInfo& info) {
    if (!desc.feedback || !desc.sequence)
        return Result::INVALID_ARGUMENT;
    if (desc.feedback->errorFlags || !desc.feedback->encodedBitstreamWrittenBytes || !desc.feedback->writtenSubregionNum)
        return Result::FAILURE;

    const void* metadata = ((BufferD3D12&)resolvedMetadataReadback).Map(resolvedMetadataOffset);
    if (!metadata)
        return Result::FAILURE;

    const auto* bytes = (const uint8_t*)metadata;
    const auto& output = *(const D3D12_VIDEO_ENCODER_OUTPUT_METADATA*)bytes;
    const auto& subregion = *(const D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA*)(bytes + sizeof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA));
    const auto& tilesLayout = *(const VideoEncodeAV1TilesLayoutD3D12*)(bytes + sizeof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA) + sizeof(D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA));
    const auto& post = *(const VideoEncodeAV1PostEncodeValuesD3D12*)(bytes + sizeof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA) + sizeof(D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA) + sizeof(VideoEncodeAV1TilesLayoutD3D12));

    if (output.EncodeErrorFlags || output.WrittenSubregionsCount != 1 || subregion.bSize <= subregion.bStartOffset)
        return Result::FAILURE;
    if (tilesLayout.colCount != 1 || tilesLayout.rowCount != 1)
        return Result::FAILURE;

    const uint64_t tilePayloadSize = subregion.bSize - subregion.bStartOffset;
    if (tilePayloadSize > std::numeric_limits<uint32_t>::max())
        return Result::FAILURE;

    info = {};
    info.sequence = *desc.sequence;
    info.sequence.flags |= VideoAV1SequenceBits::ENABLE_CDEF | VideoAV1SequenceBits::ENABLE_RESTORATION;
    const uint32_t width = info.sequence.maxFrameWidthMinus1 + 1;
    const uint32_t height = info.sequence.maxFrameHeightMinus1 + 1;
    video_av1::BindPointers(info);
    video_av1::FillSingleTileLayout(info, width, height);
    info.tileLayout.contextUpdateTileId = (uint16_t)tilesLayout.contextUpdateTileId;

    info.bitstreamOffset = subregion.bStartOffset;
    info.bitstreamSize = tilePayloadSize;
    info.tiles[0] = {0, (uint32_t)tilePayloadSize, 0, 0, 0xFF, {}};

    info.quantization.deltaQYDc = (int8_t)post.quantization.yDcDeltaQ;
    info.quantization.deltaQUDc = (int8_t)post.quantization.uDcDeltaQ;
    info.quantization.deltaQUAc = (int8_t)post.quantization.uAcDeltaQ;
    info.quantization.deltaQVDc = (int8_t)post.quantization.vDcDeltaQ;
    info.quantization.deltaQVAc = (int8_t)post.quantization.vAcDeltaQ;
    info.quantization.usingQmatrix = (uint8_t)post.quantization.usingQMatrix;
    info.quantization.qmY = (uint8_t)post.quantization.qmY;
    info.quantization.qmU = (uint8_t)post.quantization.qmU;
    info.quantization.qmV = (uint8_t)post.quantization.qmV;

    info.loopFilter.level[0] = (uint8_t)post.loopFilter.level[0];
    info.loopFilter.level[1] = (uint8_t)post.loopFilter.level[1];
    info.loopFilter.level[2] = (uint8_t)post.loopFilter.levelU;
    info.loopFilter.level[3] = (uint8_t)post.loopFilter.levelV;
    info.loopFilter.sharpness = (uint8_t)post.loopFilter.sharpness;
    info.loopFilter.deltaEnabled = (uint8_t)post.loopFilter.deltaEnabled;
    info.loopFilter.deltaUpdate = (uint8_t)post.loopFilter.updateRefDelta;
    info.loopFilter.updateModeDelta = (uint8_t)post.loopFilter.updateModeDelta;
    for (uint32_t i = 0; i < 8; i++)
        info.loopFilter.refDeltas[i] = (int8_t)post.loopFilter.refDeltas[i];
    for (uint32_t i = 0; i < 2; i++)
        info.loopFilter.modeDeltas[i] = (int8_t)post.loopFilter.modeDeltas[i];

    info.picture.frameType = VideoEncodeFrameType::IDR;
    info.picture.orderHint = 0;
    info.picture.refreshFrameFlags = 0xFF;
    info.picture.primaryReferenceName = VideoAV1ReferenceName::NONE;
    info.picture.currentFrameId = 0;
    info.picture.flags = VideoAV1PictureBits::ERROR_RESILIENT_MODE | VideoAV1PictureBits::FORCE_INTEGER_MV | VideoAV1PictureBits::SHOW_FRAME;
    if (post.quantizationDelta.deltaQPresent) {
        info.picture.flags |= VideoAV1PictureBits::DELTA_Q_PRESENT;
        info.picture.deltaQRes = (uint8_t)post.quantizationDelta.deltaQRes;
    }
    if (post.loopFilterDelta.deltaLfPresent) {
        info.picture.flags |= VideoAV1PictureBits::DELTA_LF_PRESENT;
        info.picture.deltaLfRes = (uint8_t)post.loopFilterDelta.deltaLfRes;
    }
    if (post.loopFilterDelta.deltaLfMulti)
        info.picture.flags |= VideoAV1PictureBits::DELTA_LF_MULTI;
    if (post.segmentation.enabled) {
        info.picture.flags |= VideoAV1PictureBits::SEGMENTATION_ENABLED;
        if (post.segmentation.updateMap)
            info.picture.flags |= VideoAV1PictureBits::SEGMENTATION_UPDATE_MAP;
        if (post.segmentation.updateData)
            info.picture.flags |= VideoAV1PictureBits::SEGMENTATION_UPDATE_DATA;
        if (post.segmentation.temporalUpdate)
            info.picture.flags |= VideoAV1PictureBits::SEGMENTATION_TEMPORAL_UPDATE;
        info.picture.segmentation = &info.segmentation;
        for (uint32_t i = 0; i < 8; i++) {
            info.segmentation.featureEnabled[i] = (uint8_t)post.segmentation.enabledFeatures[i];
            for (uint32_t j = 0; j < 8; j++)
                info.segmentation.featureData[i][j] = (int16_t)post.segmentation.featureValues[i][j];
        }
    }
    info.picture.renderWidthMinus1 = (uint16_t)(width - 1);
    info.picture.renderHeightMinus1 = (uint16_t)(height - 1);
    info.picture.baseQIndex = (uint8_t)post.quantization.baseQIndex;
    info.picture.interpolationFilter = video_av1::INTERPOLATION_FILTER_EIGHTTAP;
    info.picture.txMode = video_av1::TX_MODE_SELECT;
    info.picture.cdefDampingMinus3 = (uint8_t)post.cdef.cdefDampingMinus3;
    info.picture.cdefBits = (uint8_t)post.cdef.cdefBits;
    info.picture.tileNum = 1;
    for (uint32_t i = 0; i < 8; i++) {
        info.cdef.yPrimaryStrength[i] = (uint8_t)post.cdef.yPrimaryStrength[i];
        info.cdef.ySecondaryStrength[i] = (uint8_t)post.cdef.ySecondaryStrength[i];
        info.cdef.uvPrimaryStrength[i] = (uint8_t)post.cdef.uvPrimaryStrength[i];
        info.cdef.uvSecondaryStrength[i] = (uint8_t)post.cdef.uvSecondaryStrength[i];
    }
    video_av1::FillIdentityGlobalMotion(info.globalMotion);
    video_av1::BindPointers(info);
    return Result::SUCCESS;
}

Result DeviceD3D12::FillFunctionTable(VideoInterface& table) const {
    if (m_Desc.adapterDesc.queueNum[(size_t)QueueType::VIDEO_DECODE] == 0 && m_Desc.adapterDesc.queueNum[(size_t)QueueType::VIDEO_ENCODE] == 0)
        return Result::UNSUPPORTED;

    table.CreateCommittedVideoTexture = ::CreateCommittedVideoTexture;
    table.CreateCommittedVideoBitstreamBuffer = ::CreateCommittedVideoBitstreamBuffer;
    table.GetVideoQueue = ::GetVideoQueue;
    table.CreateVideoSession = ::CreateVideoSession;
    table.DestroyVideoSession = ::DestroyVideoSession;
    table.CreateVideoSessionParameters = ::CreateVideoSessionParameters;
    table.DestroyVideoSessionParameters = ::DestroyVideoSessionParameters;
    table.CreateVideoPicture = ::CreateVideoPicture;
    table.DestroyVideoPicture = ::DestroyVideoPicture;
    table.GetVideoDecodePictureStates = ::GetVideoDecodePictureStates;
    table.WriteVideoAnnexBParameterSets = ::WriteVideoAnnexBParameterSets;
    table.CmdDecodeVideo = ::CmdDecodeVideo;
    table.CmdEncodeVideo = ::CmdEncodeVideo;
    table.CmdResolveVideoEncodeFeedback = ::CmdResolveVideoEncodeFeedback;
    table.GetVideoEncodeFeedback = ::GetVideoEncodeFeedback;
    table.GetVideoEncodeAV1DecodeInfo = ::GetVideoEncodeAV1DecodeInfo;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Streamer  ]

static Result NRI_CALL CreateStreamer(Device& device, const StreamerDesc& streamerDesc, Streamer*& streamer) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;
    StreamerImpl* impl = Allocate<StreamerImpl>(deviceD3D12.GetAllocationCallbacks(), device, deviceD3D12.GetCoreInterface());
    Result result = impl->Create(streamerDesc);

    if (result != Result::SUCCESS) {
        Destroy(impl);
        streamer = nullptr;
    } else
        streamer = (Streamer*)impl;

    return result;
}

static void NRI_CALL DestroyStreamer(Streamer* streamer) {
    Destroy((StreamerImpl*)streamer);
}

static Buffer* NRI_CALL GetStreamerConstantBuffer(Streamer& streamer) {
    return ((StreamerImpl&)streamer).GetConstantBuffer();
}

static uint32_t NRI_CALL StreamConstantData(Streamer& streamer, const void* data, uint32_t dataSize) {
    return ((StreamerImpl&)streamer).StreamConstantData(data, dataSize);
}

static BufferOffset NRI_CALL StreamBufferData(Streamer& streamer, const StreamBufferDataDesc& streamBufferDataDesc) {
    return ((StreamerImpl&)streamer).StreamBufferData(streamBufferDataDesc);
}

static BufferOffset NRI_CALL StreamTextureData(Streamer& streamer, const StreamTextureDataDesc& streamTextureDataDesc) {
    return ((StreamerImpl&)streamer).StreamTextureData(streamTextureDataDesc);
}

static void NRI_CALL EndStreamerFrame(Streamer& streamer) {
    ((StreamerImpl&)streamer).EndFrame();
}

static void NRI_CALL CmdCopyStreamedData(CommandBuffer& commandBuffer, Streamer& streamer) {
    ((StreamerImpl&)streamer).CmdCopyStreamedData(commandBuffer);
}

Result DeviceD3D12::FillFunctionTable(StreamerInterface& table) const {
    table.CreateStreamer = ::CreateStreamer;
    table.DestroyStreamer = ::DestroyStreamer;
    table.GetStreamerConstantBuffer = ::GetStreamerConstantBuffer;
    table.StreamBufferData = ::StreamBufferData;
    table.StreamTextureData = ::StreamTextureData;
    table.StreamConstantData = ::StreamConstantData;
    table.EndStreamerFrame = ::EndStreamerFrame;
    table.CmdCopyStreamedData = ::CmdCopyStreamedData;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  SwapChain  ]

static Result NRI_CALL CreateSwapChain(Device& device, const SwapChainDesc& swapChainDesc, SwapChain*& swapChain) {
    return ((DeviceD3D12&)device).CreateImplementation<SwapChainD3D12>(swapChain, swapChainDesc);
}

static void NRI_CALL DestroySwapChain(SwapChain* swapChain) {
    Destroy((SwapChainD3D12*)swapChain);
}

static Texture* const* NRI_CALL GetSwapChainTextures(const SwapChain& swapChain, uint32_t& textureNum) {
    return ((SwapChainD3D12&)swapChain).GetTextures(textureNum);
}

static Result NRI_CALL GetDisplayDesc(SwapChain& swapChain, DisplayDesc& displayDesc) {
    return ((SwapChainD3D12&)swapChain).GetDisplayDesc(displayDesc);
}

static Result NRI_CALL AcquireNextTexture(SwapChain& swapChain, Fence&, uint32_t& textureIndex) {
    return ((SwapChainD3D12&)swapChain).AcquireNextTexture(textureIndex);
}

static Result NRI_CALL WaitForPresent(SwapChain& swapChain) {
    return ((SwapChainD3D12&)swapChain).WaitForPresent();
}

static Result NRI_CALL QueuePresent(SwapChain& swapChain, Fence&) {
    return ((SwapChainD3D12&)swapChain).Present();
}

Result DeviceD3D12::FillFunctionTable(SwapChainInterface& table) const {
    if (!m_Desc.features.swapChain)
        return Result::UNSUPPORTED;

    table.CreateSwapChain = ::CreateSwapChain;
    table.DestroySwapChain = ::DestroySwapChain;
    table.GetSwapChainTextures = ::GetSwapChainTextures;
    table.GetDisplayDesc = ::GetDisplayDesc;
    table.AcquireNextTexture = ::AcquireNextTexture;
    table.WaitForPresent = ::WaitForPresent;
    table.QueuePresent = ::QueuePresent;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Upscaler  ]

static Result NRI_CALL CreateUpscaler(Device& device, const UpscalerDesc& upscalerDesc, Upscaler*& upscaler) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;
    UpscalerImpl* impl = Allocate<UpscalerImpl>(deviceD3D12.GetAllocationCallbacks(), device, deviceD3D12.GetCoreInterface());
    Result result = impl->Create(upscalerDesc);

    if (result != Result::SUCCESS) {
        Destroy(deviceD3D12.GetAllocationCallbacks(), impl);
        upscaler = nullptr;
    } else
        upscaler = (Upscaler*)impl;

    return result;
}

static void NRI_CALL DestroyUpscaler(Upscaler* upscaler) {
    Destroy((UpscalerImpl*)upscaler);
}

static bool NRI_CALL IsUpscalerSupported(const Device& device, UpscalerType upscalerType) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    return IsUpscalerSupported(deviceD3D12.GetDesc(), upscalerType);
}

static void NRI_CALL GetUpscalerProps(const Upscaler& upscaler, UpscalerProps& upscalerProps) {
    UpscalerImpl& upscalerImpl = (UpscalerImpl&)upscaler;

    return upscalerImpl.GetUpscalerProps(upscalerProps);
}

static void NRI_CALL CmdDispatchUpscale(CommandBuffer& commandBuffer, Upscaler& upscaler, const DispatchUpscaleDesc& dispatchUpscalerDesc) {
    UpscalerImpl& upscalerImpl = (UpscalerImpl&)upscaler;

    upscalerImpl.CmdDispatchUpscale(commandBuffer, dispatchUpscalerDesc);
}

Result DeviceD3D12::FillFunctionTable(UpscalerInterface& table) const {
    table.CreateUpscaler = ::CreateUpscaler;
    table.DestroyUpscaler = ::DestroyUpscaler;
    table.IsUpscalerSupported = ::IsUpscalerSupported;
    table.GetUpscalerProps = ::GetUpscalerProps;
    table.CmdDispatchUpscale = ::CmdDispatchUpscale;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  WrapperD3D12  ]

static Result NRI_CALL CreateCommandBufferD3D12(Device& device, const CommandBufferD3D12Desc& commandBufferD3D12Desc, CommandBuffer*& commandBuffer) {
    return ((DeviceD3D12&)device).CreateImplementation<CommandBufferD3D12>(commandBuffer, commandBufferD3D12Desc);
}

static Result NRI_CALL CreateDescriptorPoolD3D12(Device& device, const DescriptorPoolD3D12Desc& descriptorPoolD3D12Desc, DescriptorPool*& descriptorPool) {
    return ((DeviceD3D12&)device).CreateImplementation<DescriptorPoolD3D12>(descriptorPool, descriptorPoolD3D12Desc);
}

static Result NRI_CALL CreateBufferD3D12(Device& device, const BufferD3D12Desc& bufferD3D12Desc, Buffer*& buffer) {
    return ((DeviceD3D12&)device).CreateImplementation<BufferD3D12>(buffer, bufferD3D12Desc);
}

static Result NRI_CALL CreateTextureD3D12(Device& device, const TextureD3D12Desc& textureD3D12Desc, Texture*& texture) {
    return ((DeviceD3D12&)device).CreateImplementation<TextureD3D12>(texture, textureD3D12Desc);
}

static Result NRI_CALL CreateMemoryD3D12(Device& device, const MemoryD3D12Desc& memoryD3D12Desc, Memory*& memory) {
    return ((DeviceD3D12&)device).CreateImplementation<MemoryD3D12>(memory, memoryD3D12Desc);
}

static Result NRI_CALL CreateFenceD3D12(Device& device, const FenceD3D12Desc& fenceD3D12Desc, Fence*& fence) {
    return ((DeviceD3D12&)device).CreateImplementation<FenceD3D12>(fence, fenceD3D12Desc);
}

static Result NRI_CALL CreateAccelerationStructureD3D12(Device& device, const AccelerationStructureD3D12Desc& accelerationStructureD3D12Desc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceD3D12&)device).CreateImplementation<AccelerationStructureD3D12>(accelerationStructure, accelerationStructureD3D12Desc);
}

static void NRI_CALL CmdDecodeVideoD3D12(CommandBuffer& commandBuffer, const VideoDecodeD3D12Desc& videoDecodeD3D12Desc) {
    ((CommandBufferD3D12&)commandBuffer).DecodeVideo(videoDecodeD3D12Desc);
}

static void NRI_CALL CmdEncodeVideoD3D12(CommandBuffer& commandBuffer, const VideoEncodeD3D12Desc& videoEncodeD3D12Desc) {
    ((CommandBufferD3D12&)commandBuffer).EncodeVideo(videoEncodeD3D12Desc);
}

Result DeviceD3D12::FillFunctionTable(WrapperD3D12Interface& table) const {
    table.CreateCommandBufferD3D12 = ::CreateCommandBufferD3D12;
    table.CreateDescriptorPoolD3D12 = ::CreateDescriptorPoolD3D12;
    table.CreateBufferD3D12 = ::CreateBufferD3D12;
    table.CreateTextureD3D12 = ::CreateTextureD3D12;
    table.CreateMemoryD3D12 = ::CreateMemoryD3D12;
    table.CreateFenceD3D12 = ::CreateFenceD3D12;
    table.CreateAccelerationStructureD3D12 = ::CreateAccelerationStructureD3D12;
    table.CmdDecodeVideoD3D12 = ::CmdDecodeVideoD3D12;
    table.CmdEncodeVideoD3D12 = ::CmdEncodeVideoD3D12;

    return Result::SUCCESS;
}

#pragma endregion
