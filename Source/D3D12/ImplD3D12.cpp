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
#include "PipelineD3D12.h"
#include "PipelineLayoutD3D12.h"
#include "QueryPoolD3D12.h"
#include "QueueD3D12.h"
#include "SwapChainD3D12.h"
#include "TextureD3D12.h"

#include "HelperInterface.h"
#include "ImguiInterface.h"
#include "StreamerInterface.h"
#include "UpscalerInterface.h"

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

static uint32_t NRI_CALL GetQuerySize(const QueryPool& queryPool) {
    return ((QueryPoolD3D12&)queryPool).GetQuerySize();
}

static uint64_t NRI_CALL GetFenceValue(Fence& fence) {
    return ((FenceD3D12&)fence).GetFenceValue();
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

static Result NRI_CALL CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D12& device = ((TextureD3D12*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D12& device = ((TextureD3D12*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
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

    D3D12_RESOURCE_DESC desc = {};
    deviceD3D12.GetResourceDesc(bufferD3D12.GetDesc(), desc);

    deviceD3D12.GetMemoryDesc(memoryLocation, desc, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc(const Texture& texture, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const TextureD3D12& textureD3D12 = (TextureD3D12&)texture;
    const DeviceD3D12& deviceD3D12 = textureD3D12.GetDevice();

    D3D12_RESOURCE_DESC desc = {};
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

    D3D12_RESOURCE_DESC desc = {};
    deviceD3D12.GetResourceDesc(bufferDesc, desc);

    deviceD3D12.GetMemoryDesc(memoryLocation, desc, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc2(const Device& device, const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    D3D12_RESOURCE_DESC desc = {};
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

static void NRI_CALL CmdBeginRendering(CommandBuffer& commandBuffer, const AttachmentsDesc& attachmentsDesc) {
    ((CommandBufferD3D12&)commandBuffer).BeginRendering(attachmentsDesc);
}

static void NRI_CALL CmdClearAttachments(CommandBuffer& commandBuffer, const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    ((CommandBufferD3D12&)commandBuffer).ClearAttachments(clearDescs, clearDescNum, rects, rectNum);
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
    ((CommandBufferD3D12&)commandBuffer).ResetAttachments();
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

static void NRI_CALL CmdResolveTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    ((CommandBufferD3D12&)commandBuffer).ResolveTexture(dstTexture, dstRegion, srcTexture, srcRegion);
}

static void NRI_CALL CmdClearStorage(CommandBuffer& commandBuffer, const ClearStorageDesc& clearDesc) {
    ((CommandBufferD3D12&)commandBuffer).ClearStorage(clearDesc);
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

static void NRI_CALL ResetCommandAllocator(CommandAllocator& commandAllocator) {
    ((CommandAllocatorD3D12&)commandAllocator).Reset();
}

static void* NRI_CALL MapBuffer(Buffer& buffer, uint64_t offset, uint64_t) {
    return ((BufferD3D12&)buffer).Map(offset);
}

static void NRI_CALL UnmapBuffer(Buffer&) {
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

    return (ID3D12GraphicsCommandList*)(*(CommandBufferD3D12*)commandBuffer);
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
    table.GetQueue = ::GetQueue;
    table.CreateCommandAllocator = ::CreateCommandAllocator;
    table.CreateCommandBuffer = ::CreateCommandBuffer;
    table.CreateDescriptorPool = ::CreateDescriptorPool;
    table.CreateBufferView = ::CreateBufferView;
    table.CreateTexture1DView = ::CreateTexture1DView;
    table.CreateTexture2DView = ::CreateTexture2DView;
    table.CreateTexture3DView = ::CreateTexture3DView;
    table.CreateSampler = ::CreateSampler;
    table.CreatePipelineLayout = ::CreatePipelineLayout;
    table.CreateGraphicsPipeline = ::CreateGraphicsPipeline;
    table.CreateComputePipeline = ::CreateComputePipeline;
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

    D3D12_RESOURCE_DESC resourceDesc = {};
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

    D3D12_RESOURCE_DESC resourceDesc = {};
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
    if (!m_Desc.features.rayTracing)
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

Result DeviceD3D12::FillFunctionTable(WrapperD3D12Interface& table) const {
    table.CreateCommandBufferD3D12 = ::CreateCommandBufferD3D12;
    table.CreateDescriptorPoolD3D12 = ::CreateDescriptorPoolD3D12;
    table.CreateBufferD3D12 = ::CreateBufferD3D12;
    table.CreateTextureD3D12 = ::CreateTextureD3D12;
    table.CreateMemoryD3D12 = ::CreateMemoryD3D12;
    table.CreateFenceD3D12 = ::CreateFenceD3D12;
    table.CreateAccelerationStructureD3D12 = ::CreateAccelerationStructureD3D12;

    return Result::SUCCESS;
}

#pragma endregion
