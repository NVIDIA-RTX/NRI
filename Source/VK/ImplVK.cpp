// © 2021 NVIDIA Corporation

#include "MemoryAllocatorVK.h"

#include "SharedVK.h"

#include "AccelerationStructureVK.h"
#include "BufferVK.h"
#include "CommandAllocatorVK.h"
#include "CommandBufferVK.h"
#include "ConversionVK.h"
#include "DescriptorPoolVK.h"
#include "DescriptorSetVK.h"
#include "DescriptorVK.h"
#include "FenceVK.h"
#include "MemoryVK.h"
#include "MicromapVK.h"
#include "PipelineLayoutVK.h"
#include "PipelineVK.h"
#include "QueryPoolVK.h"
#include "QueueVK.h"
#include "SwapChainVK.h"
#include "TextureVK.h"

#include "HelperInterface.h"
#include "ImguiInterface.h"
#include "StreamerInterface.h"
#include "UpscalerInterface.h"

using namespace nri;

#include "AccelerationStructureVK.hpp"
#include "BufferVK.hpp"
#include "CommandAllocatorVK.hpp"
#include "CommandBufferVK.hpp"
#include "ConversionVK.hpp"
#include "DescriptorPoolVK.hpp"
#include "DescriptorSetVK.hpp"
#include "DescriptorVK.hpp"
#include "DeviceVK.hpp"
#include "FenceVK.hpp"
#include "MemoryVK.hpp"
#include "MicromapVK.hpp"
#include "PipelineLayoutVK.hpp"
#include "PipelineVK.hpp"
#include "QueryPoolVK.hpp"
#include "QueueVK.hpp"
#include "SwapChainVK.hpp"
#include "TextureVK.hpp"

Result CreateDeviceVK(const DeviceCreationDesc& desc, const DeviceCreationVKDesc& descVK, DeviceBase*& device) {
    DeviceVK* impl = Allocate<DeviceVK>(desc.allocationCallbacks, desc.callbackInterface, desc.allocationCallbacks);
    Result result = impl->Create(desc, descVK);

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
    return ((DeviceVK&)device).GetDesc();
}

static const BufferDesc& NRI_CALL GetBufferDesc(const Buffer& buffer) {
    return ((BufferVK&)buffer).GetDesc();
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture& texture) {
    return ((TextureVK&)texture).GetDesc();
}

static FormatSupportBits NRI_CALL GetFormatSupport(const Device& device, Format format) {
    return ((DeviceVK&)device).GetFormatSupport(format);
}

static Result NRI_CALL GetQueue(Device& device, QueueType queueType, uint32_t queueIndex, Queue*& queue) {
    return ((DeviceVK&)device).GetQueue(queueType, queueIndex, queue);
}

static Result NRI_CALL CreateCommandAllocator(Queue& queue, CommandAllocator*& commandAllocator) {
    DeviceVK& device = ((QueueVK&)queue).GetDevice();
    return device.CreateImplementation<CommandAllocatorVK>(commandAllocator, queue);
}

static Result NRI_CALL CreateCommandBuffer(CommandAllocator& commandAllocator, CommandBuffer*& commandBuffer) {
    return ((CommandAllocatorVK&)commandAllocator).CreateCommandBuffer(commandBuffer);
}

static Result NRI_CALL CreateFence(Device& device, uint64_t initialValue, Fence*& fence) {
    return ((DeviceVK&)device).CreateImplementation<FenceVK>(fence, initialValue);
}

static Result NRI_CALL CreateDescriptorPool(Device& device, const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceVK&)device).CreateImplementation<DescriptorPoolVK>(descriptorPool, descriptorPoolDesc);
}

static Result NRI_CALL CreatePipelineLayout(Device& device, const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    return ((DeviceVK&)device).CreateImplementation<PipelineLayoutVK>(pipelineLayout, pipelineLayoutDesc);
}

static Result NRI_CALL CreateGraphicsPipeline(Device& device, const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    return ((DeviceVK&)device).CreateImplementation<PipelineVK>(pipeline, graphicsPipelineDesc);
}

static Result NRI_CALL CreateComputePipeline(Device& device, const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    return ((DeviceVK&)device).CreateImplementation<PipelineVK>(pipeline, computePipelineDesc);
}

static Result NRI_CALL CreateQueryPool(Device& device, const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    return ((DeviceVK&)device).CreateImplementation<QueryPoolVK>(queryPool, queryPoolDesc);
}

static Result NRI_CALL CreateSampler(Device& device, const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    return ((DeviceVK&)device).CreateImplementation<DescriptorVK>(sampler, samplerDesc);
}

static Result NRI_CALL CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    DeviceVK& device = ((BufferVK*)bufferViewDesc.buffer)->GetDevice();
    return device.CreateImplementation<DescriptorVK>(bufferView, bufferViewDesc);
}

static Result NRI_CALL CreateTextureView(const TextureViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceVK& device = ((TextureVK*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorVK>(textureView, textureViewDesc);
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator* commandAllocator) {
    Destroy((CommandAllocatorVK*)commandAllocator);
}

static void NRI_CALL DestroyCommandBuffer(CommandBuffer* commandBuffer) {
    Destroy((CommandBufferVK*)commandBuffer);
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool* descriptorPool) {
    Destroy((DescriptorPoolVK*)descriptorPool);
}

static void NRI_CALL DestroyBuffer(Buffer* buffer) {
    Destroy((BufferVK*)buffer);
}

static void NRI_CALL DestroyTexture(Texture* texture) {
    Destroy((TextureVK*)texture);
}

static void NRI_CALL DestroyDescriptor(Descriptor* descriptor) {
    Destroy((DescriptorVK*)descriptor);
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout* pipelineLayout) {
    Destroy((PipelineLayoutVK*)pipelineLayout);
}

static void NRI_CALL DestroyPipeline(Pipeline* pipeline) {
    Destroy((PipelineVK*)pipeline);
}

static void NRI_CALL DestroyQueryPool(QueryPool* queryPool) {
    Destroy((QueryPoolVK*)queryPool);
}

static void NRI_CALL DestroyFence(Fence* fence) {
    Destroy((FenceVK*)fence);
}

static Result NRI_CALL AllocateMemory(Device& device, const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    return ((DeviceVK&)device).CreateImplementation<MemoryVK>(memory, allocateMemoryDesc);
}

static void NRI_CALL FreeMemory(Memory* memory) {
    Destroy((MemoryVK*)memory);
}

static Result NRI_CALL CreateBuffer(Device& device, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceVK&)device).CreateImplementation<BufferVK>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTexture(Device& device, const TextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceVK&)device).CreateImplementation<TextureVK>(texture, textureDesc);
}

static void NRI_CALL GetBufferMemoryDesc(const Buffer& buffer, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((BufferVK&)buffer).GetMemoryDesc(memoryLocation, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc(const Texture& texture, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((TextureVK&)texture).GetMemoryDesc(memoryLocation, memoryDesc);
}

static Result NRI_CALL BindBufferMemory(const BindBufferMemoryDesc* bindBufferMemoryDescs, uint32_t bindBufferMemoryDescNum) {
    if (!bindBufferMemoryDescNum)
        return Result::SUCCESS;

    DeviceVK& deviceVK = ((BufferVK*)bindBufferMemoryDescs->buffer)->GetDevice();
    return deviceVK.BindBufferMemory(bindBufferMemoryDescs, bindBufferMemoryDescNum);
}

static Result NRI_CALL BindTextureMemory(const BindTextureMemoryDesc* bindTextureMemoryDescs, uint32_t bindTextureMemoryDescNum) {
    if (!bindTextureMemoryDescNum)
        return Result::SUCCESS;

    DeviceVK& deviceVK = ((TextureVK*)bindTextureMemoryDescs->texture)->GetDevice();
    return deviceVK.BindTextureMemory(bindTextureMemoryDescs, bindTextureMemoryDescNum);
}

static void NRI_CALL GetBufferMemoryDesc2(const Device& device, const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((DeviceVK&)device).GetMemoryDesc2(bufferDesc, memoryLocation, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc2(const Device& device, const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((DeviceVK&)device).GetMemoryDesc2(textureDesc, memoryLocation, memoryDesc);
}

static Result NRI_CALL CreateCommittedBuffer(Device& device, MemoryLocation memoryLocation, float priority, const BufferDesc& bufferDesc, Buffer*& buffer) {
    DeviceVK& deviceVK = (DeviceVK&)device;

    Result result = deviceVK.CreateImplementation<BufferVK>(buffer, bufferDesc);
    if (result != Result::SUCCESS)
        return result;

    return ((BufferVK*)buffer)->AllocateAndBindMemory(memoryLocation, priority, true);
}

static Result NRI_CALL CreateCommittedTexture(Device& device, MemoryLocation memoryLocation, float priority, const TextureDesc& textureDesc, Texture*& texture) {
    DeviceVK& deviceVK = (DeviceVK&)device;

    Result result = deviceVK.CreateImplementation<TextureVK>(texture, textureDesc);
    if (result != Result::SUCCESS)
        return result;

    return ((TextureVK*)texture)->AllocateAndBindMemory(memoryLocation, priority, true);
}

static Result NRI_CALL CreatePlacedBuffer(Device& device, Memory* memory, uint64_t offset, const BufferDesc& bufferDesc, Buffer*& buffer) {
    DeviceVK& deviceVK = (DeviceVK&)device;

    Result result = deviceVK.CreateImplementation<BufferVK>(buffer, bufferDesc);
    if (result != Result::SUCCESS)
        return result;

    if (memory)
        result = ((BufferVK*)buffer)->BindMemory(*(MemoryVK*)memory, offset, true);
    else
        result = ((BufferVK*)buffer)->AllocateAndBindMemory((MemoryLocation)offset, 0.0f, false);

    return result;
}

static Result NRI_CALL CreatePlacedTexture(Device& device, Memory* memory, uint64_t offset, const TextureDesc& textureDesc, Texture*& texture) {
    DeviceVK& deviceVK = (DeviceVK&)device;

    Result result = deviceVK.CreateImplementation<TextureVK>(texture, textureDesc);
    if (result != Result::SUCCESS)
        return result;

    if (memory)
        result = ((TextureVK*)texture)->BindMemory(*(MemoryVK*)memory, offset);
    else
        result = ((TextureVK*)texture)->AllocateAndBindMemory((MemoryLocation)offset, 0.0f, false);

    return result;
}

static Result NRI_CALL AllocateDescriptorSets(DescriptorPool& descriptorPool, const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    return ((DescriptorPoolVK&)descriptorPool).AllocateDescriptorSets(pipelineLayout, setIndex, descriptorSets, instanceNum, variableDescriptorNum);
}

static void NRI_CALL UpdateDescriptorRanges(const UpdateDescriptorRangeDesc* updateDescriptorRangeDescs, uint32_t updateDescriptorRangeDescNum) {
    if (!updateDescriptorRangeDescNum)
        return;

    DeviceVK& deviceVK = ((DescriptorSetVK*)updateDescriptorRangeDescs->descriptorSet)->GetDevice();
    deviceVK.UpdateDescriptorRanges(updateDescriptorRangeDescs, updateDescriptorRangeDescNum);
}

static void NRI_CALL CopyDescriptorRanges(const CopyDescriptorRangeDesc* copyDescriptorRangeDescs, uint32_t copyDescriptorRangeDescNum) {
    if (!copyDescriptorRangeDescNum)
        return;

    DeviceVK& deviceVK = ((DescriptorSetVK*)copyDescriptorRangeDescs->dstDescriptorSet)->GetDevice();
    deviceVK.CopyDescriptorRanges(copyDescriptorRangeDescs, copyDescriptorRangeDescNum);
}

static void NRI_CALL GetDescriptorSetOffsets(const DescriptorSet&, uint32_t& resourceHeapOffset, uint32_t& samplerHeapOffset) {
    resourceHeapOffset = 0;
    samplerHeapOffset = 0;
}

static void NRI_CALL ResetDescriptorPool(DescriptorPool& descriptorPool) {
    ((DescriptorPoolVK&)descriptorPool).Reset();
}

static Result NRI_CALL BeginCommandBuffer(CommandBuffer& commandBuffer, const DescriptorPool* descriptorPool) {
    return ((CommandBufferVK&)commandBuffer).Begin(descriptorPool);
}

static void NRI_CALL CmdSetDescriptorPool(CommandBuffer&, const DescriptorPool&) {
}

static void NRI_CALL CmdSetPipelineLayout(CommandBuffer& commandBuffer, BindPoint bindPoint, const PipelineLayout& pipelineLayout) {
    ((CommandBufferVK&)commandBuffer).SetPipelineLayout(bindPoint, pipelineLayout);
}

static void NRI_CALL CmdSetDescriptorSet(CommandBuffer& commandBuffer, const SetDescriptorSetDesc& setDescriptorSetDesc) {
    ((CommandBufferVK&)commandBuffer).SetDescriptorSet(setDescriptorSetDesc);
}

static void NRI_CALL CmdSetRootConstants(CommandBuffer& commandBuffer, const SetRootConstantsDesc& setRootConstantsDesc) {
    ((CommandBufferVK&)commandBuffer).SetRootConstants(setRootConstantsDesc);
}

static void NRI_CALL CmdSetRootDescriptor(CommandBuffer& commandBuffer, const SetRootDescriptorDesc& setRootDescriptorDesc) {
    ((CommandBufferVK&)commandBuffer).SetRootDescriptor(setRootDescriptorDesc);
}

static void NRI_CALL CmdSetPipeline(CommandBuffer& commandBuffer, const Pipeline& pipeline) {
    ((CommandBufferVK&)commandBuffer).SetPipeline(pipeline);
}

static void NRI_CALL CmdBarrier(CommandBuffer& commandBuffer, const BarrierDesc& barrierDesc) {
    ((CommandBufferVK&)commandBuffer).Barrier(barrierDesc);
}

static void NRI_CALL CmdSetIndexBuffer(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, IndexType indexType) {
    ((CommandBufferVK&)commandBuffer).SetIndexBuffer(buffer, offset, indexType);
}

static void NRI_CALL CmdSetVertexBuffers(CommandBuffer& commandBuffer, uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum) {
    ((CommandBufferVK&)commandBuffer).SetVertexBuffers(baseSlot, vertexBufferDescs, vertexBufferNum);
}

static void NRI_CALL CmdSetViewports(CommandBuffer& commandBuffer, const Viewport* viewports, uint32_t viewportNum) {
    ((CommandBufferVK&)commandBuffer).SetViewports(viewports, viewportNum);
}

static void NRI_CALL CmdSetScissors(CommandBuffer& commandBuffer, const Rect* rects, uint32_t rectNum) {
    ((CommandBufferVK&)commandBuffer).SetScissors(rects, rectNum);
}

static void NRI_CALL CmdSetStencilReference(CommandBuffer& commandBuffer, uint8_t frontRef, uint8_t backRef) {
    ((CommandBufferVK&)commandBuffer).SetStencilReference(frontRef, backRef);
}

static void NRI_CALL CmdSetDepthBounds(CommandBuffer& commandBuffer, float boundsMin, float boundsMax) {
    ((CommandBufferVK&)commandBuffer).SetDepthBounds(boundsMin, boundsMax);
}

static void NRI_CALL CmdSetBlendConstants(CommandBuffer& commandBuffer, const Color32f& color) {
    ((CommandBufferVK&)commandBuffer).SetBlendConstants(color);
}

static void NRI_CALL CmdSetSampleLocations(CommandBuffer& commandBuffer, const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    ((CommandBufferVK&)commandBuffer).SetSampleLocations(locations, locationNum, sampleNum);
}

static void NRI_CALL CmdSetShadingRate(CommandBuffer& commandBuffer, const ShadingRateDesc& shadingRateDesc) {
    ((CommandBufferVK&)commandBuffer).SetShadingRate(shadingRateDesc);
}

static void NRI_CALL CmdSetDepthBias(CommandBuffer& commandBuffer, const DepthBiasDesc& depthBiasDesc) {
    ((CommandBufferVK&)commandBuffer).SetDepthBias(depthBiasDesc);
}

static void NRI_CALL CmdBeginRendering(CommandBuffer& commandBuffer, const RenderingDesc& renderingDesc) {
    ((CommandBufferVK&)commandBuffer).BeginRendering(renderingDesc);
}

static void NRI_CALL CmdClearAttachments(CommandBuffer& commandBuffer, const ClearAttachmentDesc* clearAttachmentDescs, uint32_t clearAttachmentDescNum, const Rect* rects, uint32_t rectNum) {
    ((CommandBufferVK&)commandBuffer).ClearAttachments(clearAttachmentDescs, clearAttachmentDescNum, rects, rectNum);
}

static void NRI_CALL CmdDraw(CommandBuffer& commandBuffer, const DrawDesc& drawDesc) {
    ((CommandBufferVK&)commandBuffer).Draw(drawDesc);
}

static void NRI_CALL CmdDrawIndexed(CommandBuffer& commandBuffer, const DrawIndexedDesc& drawIndexedDesc) {
    ((CommandBufferVK&)commandBuffer).DrawIndexed(drawIndexedDesc);
}

static void NRI_CALL CmdDrawIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferVK&)commandBuffer).DrawIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL CmdDrawIndexedIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferVK&)commandBuffer).DrawIndexedIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL CmdEndRendering(CommandBuffer& commandBuffer) {
    ((CommandBufferVK&)commandBuffer).EndRendering();
}

static void NRI_CALL CmdDispatch(CommandBuffer& commandBuffer, const DispatchDesc& dispatchDesc) {
    ((CommandBufferVK&)commandBuffer).Dispatch(dispatchDesc);
}

static void NRI_CALL CmdDispatchIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset) {
    ((CommandBufferVK&)commandBuffer).DispatchIndirect(buffer, offset);
}

static void NRI_CALL CmdCopyBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    ((CommandBufferVK&)commandBuffer).CopyBuffer(dstBuffer, dstOffset, srcBuffer, srcOffset, size);
}

static void NRI_CALL CmdCopyTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    ((CommandBufferVK&)commandBuffer).CopyTexture(dstTexture, dstRegion, srcTexture, srcRegion);
}

static void NRI_CALL CmdUploadBufferToTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc& dstRegion, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayout) {
    ((CommandBufferVK&)commandBuffer).UploadBufferToTexture(dstTexture, dstRegion, srcBuffer, srcDataLayout);
}

static void NRI_CALL CmdReadbackTextureToBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayout, const Texture& srcTexture, const TextureRegionDesc& srcRegion) {
    ((CommandBufferVK&)commandBuffer).ReadbackTextureToBuffer(dstBuffer, dstDataLayout, srcTexture, srcRegion);
}

static void NRI_CALL CmdZeroBuffer(CommandBuffer& commandBuffer, Buffer& buffer, uint64_t offset, uint64_t size) {
    ((CommandBufferVK&)commandBuffer).ZeroBuffer(buffer, offset, size);
}

static void NRI_CALL CmdResolveTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion, ResolveOp resolveOp) {
    ((CommandBufferVK&)commandBuffer).ResolveTexture(dstTexture, dstRegion, srcTexture, srcRegion, resolveOp);
}

static void NRI_CALL CmdClearStorage(CommandBuffer& commandBuffer, const ClearStorageDesc& clearStorageDesc) {
    ((CommandBufferVK&)commandBuffer).ClearStorage(clearStorageDesc);
}

static void NRI_CALL CmdResetQueries(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset, uint32_t num) {
    ((CommandBufferVK&)commandBuffer).ResetQueries(queryPool, offset, num);
}

static void NRI_CALL CmdBeginQuery(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferVK&)commandBuffer).BeginQuery(queryPool, offset);
}

static void NRI_CALL CmdEndQuery(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferVK&)commandBuffer).EndQuery(queryPool, offset);
}

static void NRI_CALL CmdCopyQueries(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    ((CommandBufferVK&)commandBuffer).CopyQueries(queryPool, offset, num, dstBuffer, dstOffset);
}

static void NRI_CALL CmdBeginAnnotation(CommandBuffer& commandBuffer, const char* name, uint32_t bgra) {
    MaybeUnused(commandBuffer, name, bgra);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferVK&)commandBuffer).BeginAnnotation(name, bgra);
#endif
}

static void NRI_CALL CmdEndAnnotation(CommandBuffer& commandBuffer) {
    MaybeUnused(commandBuffer);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferVK&)commandBuffer).EndAnnotation();
#endif
}

static void NRI_CALL CmdAnnotation(CommandBuffer& commandBuffer, const char* name, uint32_t bgra) {
    MaybeUnused(commandBuffer, name, bgra);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferVK&)commandBuffer).Annotation(name, bgra);
#endif
}

static Result NRI_CALL EndCommandBuffer(CommandBuffer& commandBuffer) {
    return ((CommandBufferVK&)commandBuffer).End();
}

static void NRI_CALL QueueBeginAnnotation(Queue& queue, const char* name, uint32_t bgra) {
    MaybeUnused(queue, name, bgra);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((QueueVK&)queue).BeginAnnotation(name, bgra);
#endif
}

static void NRI_CALL QueueEndAnnotation(Queue& queue) {
    MaybeUnused(queue);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((QueueVK&)queue).EndAnnotation();
#endif
}

static void NRI_CALL QueueAnnotation(Queue& queue, const char* name, uint32_t bgra) {
    MaybeUnused(queue, name, bgra);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((QueueVK&)queue).Annotation(name, bgra);
#endif
}

static void NRI_CALL ResetQueries(QueryPool& queryPool, uint32_t offset, uint32_t num) {
    ((QueryPoolVK&)queryPool).Reset(offset, num);
}

static uint32_t NRI_CALL GetQuerySize(const QueryPool& queryPool) {
    return ((QueryPoolVK&)queryPool).GetQuerySize();
}

static Result NRI_CALL QueueSubmit(Queue& queue, const QueueSubmitDesc& workSubmissionDesc) {
    return ((QueueVK&)queue).Submit(workSubmissionDesc);
}

static Result NRI_CALL QueueWaitIdle(Queue* queue) {
    if (!queue)
        return Result::SUCCESS;

    return ((QueueVK*)queue)->WaitIdle();
}

static Result NRI_CALL DeviceWaitIdle(Device* device) {
    if (!device)
        return Result::SUCCESS;

    return ((DeviceVK*)device)->WaitIdle();
}

static void NRI_CALL Wait(Fence& fence, uint64_t value) {
    ((FenceVK&)fence).Wait(value);
}

static uint64_t NRI_CALL GetFenceValue(Fence& fence) {
    return ((FenceVK&)fence).GetFenceValue();
}

static void NRI_CALL ResetCommandAllocator(CommandAllocator& commandAllocator) {
    ((CommandAllocatorVK&)commandAllocator).Reset();
}

static void* NRI_CALL MapBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    return ((BufferVK&)buffer).Map(offset, size);
}

static void NRI_CALL UnmapBuffer(Buffer& buffer) {
    ((BufferVK&)buffer).Unmap();
}

static uint64_t NRI_CALL GetBufferDeviceAddress(const Buffer& buffer) {
    return ((BufferVK&)buffer).GetDeviceAddress();
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

    return (VkDevice)(*(DeviceVK*)device);
}

static void* NRI_CALL GetQueueNativeObject(const Queue* queue) {
    if (!queue)
        return nullptr;

    return (VkQueue)(*(QueueVK*)queue);
}

static void* NRI_CALL GetCommandBufferNativeObject(const CommandBuffer* commandBuffer) {
    if (!commandBuffer)
        return nullptr;

    return (VkCommandBuffer)(*(CommandBufferVK*)commandBuffer);
}

static uint64_t NRI_CALL GetBufferNativeObject(const Buffer* buffer) {
    if (!buffer)
        return 0;

    return uint64_t(((BufferVK*)buffer)->GetHandle());
}

static uint64_t NRI_CALL GetTextureNativeObject(const Texture* texture) {
    if (!texture)
        return 0;

    return uint64_t(((TextureVK*)texture)->GetHandle());
}

static uint64_t NRI_CALL GetDescriptorNativeObject(const Descriptor* descriptor) {
    if (!descriptor)
        return 0;

    const DescriptorVK& d = *(DescriptorVK*)descriptor;
    DescriptorType descriptorType = d.GetType();

    uint64_t handle = 0;
    switch (descriptorType) {
        case DescriptorType::SAMPLER:
            handle = (uint64_t)d.GetSampler();
            break;
        case DescriptorType::BUFFER:
        case DescriptorType::STORAGE_BUFFER:
        case DescriptorType::CONSTANT_BUFFER:
        case DescriptorType::STRUCTURED_BUFFER:
        case DescriptorType::STORAGE_STRUCTURED_BUFFER:
            handle = (uint64_t)d.GetBufferView(); // 0 for non-typed views
            break;
        case DescriptorType::ACCELERATION_STRUCTURE:
            handle = (uint64_t)d.GetAccelerationStructure();
            break;
        default: // all textures (including HOST only)
            handle = (uint64_t)d.GetImageView();
            break;
    }

    return handle;
}

Result DeviceVK::FillFunctionTable(CoreInterface& table) const {
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
    QueueVK& queueVK = (QueueVK&)queue;
    DeviceVK& deviceVK = queueVK.GetDevice();
    HelperDataUpload helperDataUpload(deviceVK.GetCoreInterface(), (Device&)deviceVK, queue);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

static uint32_t NRI_CALL CalculateAllocationNumber(const Device& device, const ResourceGroupDesc& resourceGroupDesc) {
    DeviceVK& deviceVK = (DeviceVK&)device;
    HelperDeviceMemoryAllocator allocator(deviceVK.GetCoreInterface(), (Device&)device);

    return allocator.CalculateAllocationNumber(resourceGroupDesc);
}

static Result NRI_CALL AllocateAndBindMemory(Device& device, const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    DeviceVK& deviceVK = (DeviceVK&)device;
    HelperDeviceMemoryAllocator allocator(deviceVK.GetCoreInterface(), device);

    return allocator.AllocateAndBindMemory(resourceGroupDesc, allocations);
}

static Result NRI_CALL QueryVideoMemoryInfo(const Device& device, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) {
    return ((DeviceVK&)device).QueryVideoMemoryInfo(memoryLocation, videoMemoryInfo);
}

Result DeviceVK::FillFunctionTable(HelperInterface& table) const {
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
    DeviceVK& deviceVK = (DeviceVK&)device;
    ImguiImpl* impl = Allocate<ImguiImpl>(deviceVK.GetAllocationCallbacks(), device, deviceVK.GetCoreInterface());
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

Result DeviceVK::FillFunctionTable(ImguiInterface& table) const {
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
    return ((SwapChainVK&)swapChain).SetLatencySleepMode(latencySleepMode);
}

static Result NRI_CALL SetLatencyMarker(SwapChain& swapChain, LatencyMarker latencyMarker) {
    return ((SwapChainVK&)swapChain).SetLatencyMarker(latencyMarker);
}

static Result NRI_CALL LatencySleep(SwapChain& swapChain) {
    return ((SwapChainVK&)swapChain).LatencySleep();
}

static Result NRI_CALL GetLatencyReport(const SwapChain& swapChain, LatencyReport& latencyReport) {
    return ((SwapChainVK&)swapChain).GetLatencyReport(latencyReport);
}

Result DeviceVK::FillFunctionTable(LowLatencyInterface& table) const {
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
    ((CommandBufferVK&)commandBuffer).DrawMeshTasks(drawMeshTasksDesc);
}

static void NRI_CALL CmdDrawMeshTasksIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferVK&)commandBuffer).DrawMeshTasksIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

Result DeviceVK::FillFunctionTable(MeshShaderInterface& table) const {
    if (!m_Desc.features.meshShader)
        return Result::UNSUPPORTED;

    table.CmdDrawMeshTasks = ::CmdDrawMeshTasks;
    table.CmdDrawMeshTasksIndirect = ::CmdDrawMeshTasksIndirect;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  RayTracing  ]

static Result NRI_CALL CreateRayTracingPipeline(Device& device, const RayTracingPipelineDesc& pipelineDesc, Pipeline*& pipeline) {
    return ((DeviceVK&)device).CreateImplementation<PipelineVK>(pipeline, pipelineDesc);
}

static Result NRI_CALL CreateAccelerationStructureDescriptor(const AccelerationStructure& accelerationStructure, Descriptor*& descriptor) {
    return ((AccelerationStructureVK&)accelerationStructure).CreateDescriptor(descriptor);
}

static uint64_t NRI_CALL GetAccelerationStructureHandle(const AccelerationStructure& accelerationStructure) {
    static_assert(sizeof(uint64_t) == sizeof(VkDeviceAddress), "type mismatch");
    return (uint64_t)((AccelerationStructureVK&)accelerationStructure).GetDeviceAddress();
}

static uint64_t NRI_CALL GetAccelerationStructureUpdateScratchBufferSize(const AccelerationStructure& accelerationStructure) {
    return ((AccelerationStructureVK&)accelerationStructure).GetUpdateScratchBufferSize();
}

static uint64_t NRI_CALL GetAccelerationStructureBuildScratchBufferSize(const AccelerationStructure& accelerationStructure) {
    return ((AccelerationStructureVK&)accelerationStructure).GetBuildScratchBufferSize();
}

static uint64_t NRI_CALL GetMicromapBuildScratchBufferSize(const Micromap& micromap) {
    return ((MicromapVK&)micromap).GetBuildScratchBufferSize();
}

static Buffer* NRI_CALL GetAccelerationStructureBuffer(const AccelerationStructure& accelerationStructure) {
    return (Buffer*)((AccelerationStructureVK&)accelerationStructure).GetBuffer();
}

static Buffer* NRI_CALL GetMicromapBuffer(const Micromap& micromap) {
    return (Buffer*)((MicromapVK&)micromap).GetBuffer();
}

static void NRI_CALL DestroyAccelerationStructure(AccelerationStructure* accelerationStructure) {
    Destroy((AccelerationStructureVK*)accelerationStructure);
}

static void NRI_CALL DestroyMicromap(Micromap* micromap) {
    Destroy((MicromapVK*)micromap);
}

static Result NRI_CALL CreateAccelerationStructure(Device& device, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVK&)device).CreateImplementation<AccelerationStructureVK>(accelerationStructure, accelerationStructureDesc);
}

static Result NRI_CALL CreateMicromap(Device& device, const MicromapDesc& micromapDesc, Micromap*& micromap) {
    return ((DeviceVK&)device).CreateImplementation<MicromapVK>(micromap, micromapDesc);
}

static void NRI_CALL GetAccelerationStructureMemoryDesc(const AccelerationStructure& accelerationStructure, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((AccelerationStructureVK&)accelerationStructure).GetBuffer()->GetMemoryDesc(memoryLocation, memoryDesc);
}

static void NRI_CALL GetMicromapMemoryDesc(const Micromap& micromap, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((MicromapVK&)micromap).GetBuffer()->GetMemoryDesc(memoryLocation, memoryDesc);
}

static Result NRI_CALL BindAccelerationStructureMemory(const BindAccelerationStructureMemoryDesc* bindAccelerationStructureMemoryDescs, uint32_t bindAccelerationStructureMemoryDescNum) {
    if (!bindAccelerationStructureMemoryDescNum)
        return Result::SUCCESS;

    DeviceVK& deviceVK = ((AccelerationStructureVK*)bindAccelerationStructureMemoryDescs->accelerationStructure)->GetDevice();
    return deviceVK.BindAccelerationStructureMemory(bindAccelerationStructureMemoryDescs, bindAccelerationStructureMemoryDescNum);
}

static Result NRI_CALL BindMicromapMemory(const BindMicromapMemoryDesc* bindMicromapMemoryDescs, uint32_t bindMicromapMemoryDescNum) {
    if (!bindMicromapMemoryDescNum)
        return Result::SUCCESS;

    DeviceVK& deviceVK = ((MicromapVK*)bindMicromapMemoryDescs->micromap)->GetDevice();
    return deviceVK.BindMicromapMemory(bindMicromapMemoryDescs, bindMicromapMemoryDescNum);
}

static void NRI_CALL GetAccelerationStructureMemoryDesc2(const Device& device, const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((DeviceVK&)device).GetMemoryDesc2(accelerationStructureDesc, memoryLocation, memoryDesc);
}

static void NRI_CALL GetMicromapMemoryDesc2(const Device& device, const MicromapDesc& micromapDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((DeviceVK&)device).GetMemoryDesc2(micromapDesc, memoryLocation, memoryDesc);
}

static Result NRI_CALL CreateCommittedAccelerationStructure(Device& device, MemoryLocation memoryLocation, float priority, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    DeviceVK& deviceVK = (DeviceVK&)device;

    Result result = deviceVK.CreateImplementation<AccelerationStructureVK>(accelerationStructure, accelerationStructureDesc);
    if (result != Result::SUCCESS)
        return result;

    return ((AccelerationStructureVK*)accelerationStructure)->AllocateAndBindMemory(memoryLocation, priority, true);
}

static Result NRI_CALL CreateCommittedMicromap(Device& device, MemoryLocation memoryLocation, float priority, const MicromapDesc& micromapDesc, Micromap*& micromap) {
    DeviceVK& deviceVK = (DeviceVK&)device;

    Result result = deviceVK.CreateImplementation<MicromapVK>(micromap, micromapDesc);
    if (result != Result::SUCCESS)
        return result;

    return ((MicromapVK*)micromap)->AllocateAndBindMemory(memoryLocation, priority, true);
}

static Result NRI_CALL CreatePlacedAccelerationStructure(Device& device, Memory* memory, uint64_t offset, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    DeviceVK& deviceVK = (DeviceVK&)device;

    Result result = deviceVK.CreateImplementation<AccelerationStructureVK>(accelerationStructure, accelerationStructureDesc);
    if (result != Result::SUCCESS)
        return result;

    if (memory)
        result = ((AccelerationStructureVK*)accelerationStructure)->BindMemory((MemoryVK*)memory, offset);
    else
        result = ((AccelerationStructureVK*)accelerationStructure)->AllocateAndBindMemory((MemoryLocation)offset, 0.0f, false);

    return result;
}

static Result NRI_CALL CreatePlacedMicromap(Device& device, Memory* memory, uint64_t offset, const MicromapDesc& micromapDesc, Micromap*& micromap) {
    DeviceVK& deviceVK = (DeviceVK&)device;

    Result result = deviceVK.CreateImplementation<MicromapVK>(micromap, micromapDesc);
    if (result != Result::SUCCESS)
        return result;

    if (memory)
        result = ((MicromapVK*)micromap)->BindMemory((MemoryVK*)memory, offset);
    else
        result = ((MicromapVK*)micromap)->AllocateAndBindMemory((MemoryLocation)offset, 0.0f, false);

    return result;
}

static Result NRI_CALL WriteShaderGroupIdentifiers(const Pipeline& pipeline, uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* dst) {
    return ((PipelineVK&)pipeline).WriteShaderGroupIdentifiers(baseShaderGroupIndex, shaderGroupNum, dst);
}

static void NRI_CALL CmdBuildTopLevelAccelerationStructures(CommandBuffer& commandBuffer, const BuildTopLevelAccelerationStructureDesc* buildTopLevelAccelerationStructureDescs, uint32_t buildTopLevelAccelerationStructureDescNum) {
    ((CommandBufferVK&)commandBuffer).BuildTopLevelAccelerationStructures(buildTopLevelAccelerationStructureDescs, buildTopLevelAccelerationStructureDescNum);
}

static void NRI_CALL CmdBuildBottomLevelAccelerationStructures(CommandBuffer& commandBuffer, const BuildBottomLevelAccelerationStructureDesc* buildBottomLevelAccelerationStructureDescs, uint32_t buildBottomLevelAccelerationStructureDescNum) {
    ((CommandBufferVK&)commandBuffer).BuildBottomLevelAccelerationStructures(buildBottomLevelAccelerationStructureDescs, buildBottomLevelAccelerationStructureDescNum);
}

static void NRI_CALL CmdBuildMicromaps(CommandBuffer& commandBuffer, const BuildMicromapDesc* buildMicromapDescs, uint32_t buildMicromapDescNum) {
    ((CommandBufferVK&)commandBuffer).BuildMicromaps(buildMicromapDescs, buildMicromapDescNum);
}

static void NRI_CALL CmdDispatchRays(CommandBuffer& commandBuffer, const DispatchRaysDesc& dispatchRaysDesc) {
    ((CommandBufferVK&)commandBuffer).DispatchRays(dispatchRaysDesc);
}

static void NRI_CALL CmdDispatchRaysIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset) {
    ((CommandBufferVK&)commandBuffer).DispatchRaysIndirect(buffer, offset);
}

static void NRI_CALL CmdWriteAccelerationStructuresSizes(CommandBuffer& commandBuffer, const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    ((CommandBufferVK&)commandBuffer).WriteAccelerationStructuresSizes(accelerationStructures, accelerationStructureNum, queryPool, queryPoolOffset);
}

static void NRI_CALL CmdWriteMicromapsSizes(CommandBuffer& commandBuffer, const Micromap* const* micromaps, uint32_t micromapNum, QueryPool& queryPool, uint32_t queryPoolOffset) {
    ((CommandBufferVK&)commandBuffer).WriteMicromapsSizes(micromaps, micromapNum, queryPool, queryPoolOffset);
}

static void NRI_CALL CmdCopyAccelerationStructure(CommandBuffer& commandBuffer, AccelerationStructure& dst, const AccelerationStructure& src, CopyMode mode) {
    ((CommandBufferVK&)commandBuffer).CopyAccelerationStructure(dst, src, mode);
}

static void NRI_CALL CmdCopyMicromap(CommandBuffer& commandBuffer, Micromap& dst, const Micromap& src, CopyMode copyMode) {
    ((CommandBufferVK&)commandBuffer).CopyMicromap(dst, src, copyMode);
}

static uint64_t NRI_CALL GetAccelerationStructureNativeObject(const AccelerationStructure* accelerationStructure) {
    if (!accelerationStructure)
        return 0;

    return uint64_t(((AccelerationStructureVK*)accelerationStructure)->GetHandle());
}

static uint64_t NRI_CALL GetMicromapNativeObject(const Micromap* micromap) {
    if (!micromap)
        return 0;

    return uint64_t(((MicromapVK*)micromap)->GetHandle());
}

Result DeviceVK::FillFunctionTable(RayTracingInterface& table) const {
    if (m_Desc.tiers.rayTracing == 0)
        return Result::UNSUPPORTED;

    table.CreateRayTracingPipeline = ::CreateRayTracingPipeline;
    table.CreateAccelerationStructureDescriptor = ::CreateAccelerationStructureDescriptor;
    table.GetAccelerationStructureHandle = ::GetAccelerationStructureHandle;
    table.GetAccelerationStructureUpdateScratchBufferSize = ::GetAccelerationStructureUpdateScratchBufferSize;
    table.GetAccelerationStructureBuildScratchBufferSize = ::GetAccelerationStructureBuildScratchBufferSize;
    table.GetMicromapBuildScratchBufferSize = ::GetMicromapBuildScratchBufferSize;
    table.GetAccelerationStructureBuffer = ::GetAccelerationStructureBuffer;
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

struct VideoSessionVK final : public DebugNameBase {
    inline VideoSessionVK(DeviceVK& device)
        : m_Device(device)
        , m_Memory(device.GetStdAllocator()) {
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~VideoSessionVK();

    Result Create(const VideoSessionDesc& videoSessionDesc);

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_VIDEO_SESSION_KHR, (uint64_t)m_Handle, name);
    }

    DeviceVK& m_Device;
    VkVideoSessionKHR m_Handle = VK_NULL_HANDLE;
    Vector<VkDeviceMemory> m_Memory;
    VideoSessionDesc m_Desc = {};
};

static VkVideoComponentBitDepthFlagsKHR GetVideoBitDepthVK(Format format) {
    return format == Format::P010_UNORM || format == Format::P016_UNORM ? VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR : VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
}

static VkVideoCodecOperationFlagBitsKHR GetVideoCodecOperationVK(const VideoSessionDesc& videoSessionDesc) {
    if (videoSessionDesc.usage == VideoUsage::DECODE) {
        switch (videoSessionDesc.codec) {
        case VideoCodec::H264:
            return VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR;
        case VideoCodec::H265:
            return VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR;
        case VideoCodec::AV1:
            return VK_VIDEO_CODEC_OPERATION_DECODE_AV1_BIT_KHR;
        case VideoCodec::MAX_NUM:
            return (VkVideoCodecOperationFlagBitsKHR)0;
        }
    } else if (videoSessionDesc.usage == VideoUsage::ENCODE) {
        switch (videoSessionDesc.codec) {
        case VideoCodec::H264:
            return VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_KHR;
        case VideoCodec::H265:
            return VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_KHR;
        case VideoCodec::AV1:
            return VK_VIDEO_CODEC_OPERATION_ENCODE_AV1_BIT_KHR;
        case VideoCodec::MAX_NUM:
            return (VkVideoCodecOperationFlagBitsKHR)0;
        }
    }

    return (VkVideoCodecOperationFlagBitsKHR)0;
}

static void* FillVideoProfileCodecInfoVK(const VideoSessionDesc& videoSessionDesc, void* storage) {
    if (videoSessionDesc.usage == VideoUsage::DECODE) {
        switch (videoSessionDesc.codec) {
        case VideoCodec::H264: {
            VkVideoDecodeH264ProfileInfoKHR& info = *(VkVideoDecodeH264ProfileInfoKHR*)storage;
            info = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR};
            info.stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH;
            info.pictureLayout = VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_PROGRESSIVE_KHR;
            return &info;
        }
        case VideoCodec::H265: {
            VkVideoDecodeH265ProfileInfoKHR& info = *(VkVideoDecodeH265ProfileInfoKHR*)storage;
            info = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PROFILE_INFO_KHR};
            info.stdProfileIdc = videoSessionDesc.format == Format::P010_UNORM || videoSessionDesc.format == Format::P016_UNORM ? STD_VIDEO_H265_PROFILE_IDC_MAIN_10 : STD_VIDEO_H265_PROFILE_IDC_MAIN;
            return &info;
        }
        case VideoCodec::AV1: {
            VkVideoDecodeAV1ProfileInfoKHR& info = *(VkVideoDecodeAV1ProfileInfoKHR*)storage;
            info = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_PROFILE_INFO_KHR};
            info.stdProfile = STD_VIDEO_AV1_PROFILE_MAIN;
            return &info;
        }
        case VideoCodec::MAX_NUM:
            return nullptr;
        }
    } else if (videoSessionDesc.usage == VideoUsage::ENCODE) {
        switch (videoSessionDesc.codec) {
        case VideoCodec::H264: {
            VkVideoEncodeH264ProfileInfoKHR& info = *(VkVideoEncodeH264ProfileInfoKHR*)storage;
            info = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_INFO_KHR};
            info.stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH;
            return &info;
        }
        case VideoCodec::H265: {
            VkVideoEncodeH265ProfileInfoKHR& info = *(VkVideoEncodeH265ProfileInfoKHR*)storage;
            info = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PROFILE_INFO_KHR};
            info.stdProfileIdc = videoSessionDesc.format == Format::P010_UNORM || videoSessionDesc.format == Format::P016_UNORM ? STD_VIDEO_H265_PROFILE_IDC_MAIN_10 : STD_VIDEO_H265_PROFILE_IDC_MAIN;
            return &info;
        }
        case VideoCodec::AV1: {
            VkVideoEncodeAV1ProfileInfoKHR& info = *(VkVideoEncodeAV1ProfileInfoKHR*)storage;
            info = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_PROFILE_INFO_KHR};
            info.stdProfile = STD_VIDEO_AV1_PROFILE_MAIN;
            return &info;
        }
        case VideoCodec::MAX_NUM:
            return nullptr;
        }
    }

    return nullptr;
}

static bool FindVideoSessionMemoryTypeVK(const DeviceVK& device, uint32_t memoryTypeBits, uint32_t& memoryTypeIndex) {
    uint32_t fallbackIndex = uint32_t(-1);
    for (uint32_t i = 0; i < 32; i++) {
        if ((memoryTypeBits & (1u << i)) == 0)
            continue;

        MemoryTypeInfo memoryTypeInfo = {};
        if (!device.GetMemoryTypeByIndex(i, memoryTypeInfo))
            continue;

        if (memoryTypeInfo.location == MemoryLocation::DEVICE) {
            memoryTypeIndex = i;
            return true;
        }

        if (fallbackIndex == uint32_t(-1))
            fallbackIndex = i;
    }

    if (fallbackIndex == uint32_t(-1))
        return false;

    memoryTypeIndex = fallbackIndex;
    return true;
}

VideoSessionVK::~VideoSessionVK() {
    const auto& vk = m_Device.GetDispatchTable();
    if (m_Handle)
        vk.DestroyVideoSessionKHR(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());

    for (VkDeviceMemory memory : m_Memory)
        vk.FreeMemory(m_Device, memory, m_Device.GetVkAllocationCallbacks());
}

Result VideoSessionVK::Create(const VideoSessionDesc& videoSessionDesc) {
    const auto& vk = m_Device.GetDispatchTable();
    if (!vk.CreateVideoSessionKHR || !vk.GetVideoSessionMemoryRequirementsKHR || !vk.BindVideoSessionMemoryKHR)
        return Result::UNSUPPORTED;

    if (videoSessionDesc.width == 0 || videoSessionDesc.height == 0 || videoSessionDesc.format == Format::UNKNOWN)
        return Result::INVALID_ARGUMENT;

    Queue* queue = nullptr;
    Result result = m_Device.GetQueue(videoSessionDesc.usage == VideoUsage::ENCODE ? QueueType::VIDEO_ENCODE : QueueType::VIDEO_DECODE, 0, queue);
    if (result != Result::SUCCESS)
        return result;

    VkVideoCodecOperationFlagBitsKHR operation = GetVideoCodecOperationVK(videoSessionDesc);
    if (!operation)
        return Result::UNSUPPORTED;

    std::aligned_storage_t<64, 8> codecProfileStorage = {};
    VkVideoProfileInfoKHR profile = {VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR};
    profile.pNext = FillVideoProfileCodecInfoVK(videoSessionDesc, &codecProfileStorage);
    profile.videoCodecOperation = operation;
    profile.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
    profile.lumaBitDepth = GetVideoBitDepthVK(videoSessionDesc.format);
    profile.chromaBitDepth = GetVideoBitDepthVK(videoSessionDesc.format);
    if (!profile.pNext)
        return Result::UNSUPPORTED;

    VkVideoCapabilitiesKHR capabilities = {VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR};
    VkResult vkResult = vk.GetPhysicalDeviceVideoCapabilitiesKHR(m_Device, &profile, &capabilities);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetPhysicalDeviceVideoCapabilitiesKHR");

    if (videoSessionDesc.width < capabilities.minCodedExtent.width || videoSessionDesc.height < capabilities.minCodedExtent.height || videoSessionDesc.width > capabilities.maxCodedExtent.width
        || videoSessionDesc.height > capabilities.maxCodedExtent.height)
        return Result::UNSUPPORTED;

    VkVideoSessionCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR};
    createInfo.queueFamilyIndex = ((QueueVK*)queue)->GetFamilyIndex();
    createInfo.pVideoProfile = &profile;
    createInfo.pictureFormat = GetVkFormat(videoSessionDesc.format);
    createInfo.maxCodedExtent = {videoSessionDesc.width, videoSessionDesc.height};
    createInfo.referencePictureFormat = videoSessionDesc.maxReferenceNum ? createInfo.pictureFormat : VK_FORMAT_UNDEFINED;
    createInfo.maxDpbSlots = videoSessionDesc.maxReferenceNum;
    createInfo.maxActiveReferencePictures = videoSessionDesc.maxReferenceNum;
    createInfo.pStdHeaderVersion = &capabilities.stdHeaderVersion;

    vkResult = vk.CreateVideoSessionKHR(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateVideoSessionKHR");

    uint32_t memoryRequirementNum = 0;
    vkResult = vk.GetVideoSessionMemoryRequirementsKHR(m_Device, m_Handle, &memoryRequirementNum, nullptr);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetVideoSessionMemoryRequirementsKHR");

    Scratch<VkVideoSessionMemoryRequirementsKHR> memoryRequirements = NRI_ALLOCATE_SCRATCH(m_Device, VkVideoSessionMemoryRequirementsKHR, memoryRequirementNum);
    for (uint32_t i = 0; i < memoryRequirementNum; i++)
        memoryRequirements[i] = {VK_STRUCTURE_TYPE_VIDEO_SESSION_MEMORY_REQUIREMENTS_KHR};

    vkResult = vk.GetVideoSessionMemoryRequirementsKHR(m_Device, m_Handle, &memoryRequirementNum, memoryRequirements);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetVideoSessionMemoryRequirementsKHR");

    m_Memory.resize(memoryRequirementNum);
    Scratch<VkBindVideoSessionMemoryInfoKHR> bindInfos = NRI_ALLOCATE_SCRATCH(m_Device, VkBindVideoSessionMemoryInfoKHR, memoryRequirementNum);
    for (uint32_t i = 0; i < memoryRequirementNum; i++) {
        uint32_t memoryTypeIndex = 0;
        if (!FindVideoSessionMemoryTypeVK(m_Device, memoryRequirements[i].memoryRequirements.memoryTypeBits, memoryTypeIndex))
            return Result::UNSUPPORTED;

        VkMemoryAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocateInfo.allocationSize = memoryRequirements[i].memoryRequirements.size;
        allocateInfo.memoryTypeIndex = memoryTypeIndex;

        vkResult = vk.AllocateMemory(m_Device, &allocateInfo, m_Device.GetVkAllocationCallbacks(), &m_Memory[i]);
        NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkAllocateMemory");

        bindInfos[i] = {VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR};
        bindInfos[i].memoryBindIndex = memoryRequirements[i].memoryBindIndex;
        bindInfos[i].memory = m_Memory[i];
        bindInfos[i].memorySize = memoryRequirements[i].memoryRequirements.size;
    }

    vkResult = vk.BindVideoSessionMemoryKHR(m_Device, m_Handle, memoryRequirementNum, bindInfos);
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkBindVideoSessionMemoryKHR");

    m_Desc = videoSessionDesc;
    return Result::SUCCESS;
}

static Result NRI_CALL CreateVideoSession(Device& device, const VideoSessionDesc& videoSessionDesc, VideoSession*& videoSession) {
    DeviceVK& deviceVK = (DeviceVK&)device;
    VideoSessionVK* impl = Allocate<VideoSessionVK>(deviceVK.GetAllocationCallbacks(), deviceVK);
    Result result = impl->Create(videoSessionDesc);

    if (result != Result::SUCCESS) {
        Destroy(impl);
        videoSession = nullptr;
    } else
        videoSession = (VideoSession*)impl;

    return result;
}

static void NRI_CALL DestroyVideoSession(VideoSession& videoSession) {
    Destroy((VideoSessionVK*)&videoSession);
}

static void NRI_CALL CmdDecodeVideo(CommandBuffer& commandBuffer, const VideoDecodeDesc& videoDecodeDesc) {
    MaybeUnused(videoDecodeDesc);
    NRI_REPORT_ERROR(&((CommandBufferVK&)commandBuffer).GetDevice(), "Backend-neutral video decode for Vulkan requires VkVideoSessionParametersKHR and VkImageView-backed picture resources; use WrapperVKInterface::CmdDecodeVideoVK.");
}

static void NRI_CALL CmdEncodeVideo(CommandBuffer& commandBuffer, const VideoEncodeDesc& videoEncodeDesc) {
    MaybeUnused(videoEncodeDesc);
    NRI_REPORT_ERROR(&((CommandBufferVK&)commandBuffer).GetDevice(), "Backend-neutral video encode for Vulkan requires codec-specific rate control and picture parameter abstraction; use WrapperVKInterface::CmdEncodeVideoVK.");
}

Result DeviceVK::FillFunctionTable(VideoInterface& table) const {
    if (m_Desc.adapterDesc.queueNum[(size_t)QueueType::VIDEO_DECODE] == 0 && m_Desc.adapterDesc.queueNum[(size_t)QueueType::VIDEO_ENCODE] == 0)
        return Result::UNSUPPORTED;

    table.CreateVideoSession = ::CreateVideoSession;
    table.DestroyVideoSession = ::DestroyVideoSession;
    table.CmdDecodeVideo = ::CmdDecodeVideo;
    table.CmdEncodeVideo = ::CmdEncodeVideo;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Streamer  ]

static Result NRI_CALL CreateStreamer(Device& device, const StreamerDesc& streamerDesc, Streamer*& streamer) {
    DeviceVK& deviceVK = (DeviceVK&)device;
    StreamerImpl* impl = Allocate<StreamerImpl>(deviceVK.GetAllocationCallbacks(), device, deviceVK.GetCoreInterface());
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
    return ((StreamerImpl&)streamer).EndFrame();
}

static void NRI_CALL CmdCopyStreamedData(CommandBuffer& commandBuffer, Streamer& streamer) {
    ((StreamerImpl&)streamer).CmdCopyStreamedData(commandBuffer);
}

Result DeviceVK::FillFunctionTable(StreamerInterface& table) const {
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
    return ((DeviceVK&)device).CreateImplementation<SwapChainVK>(swapChain, swapChainDesc);
}

static void NRI_CALL DestroySwapChain(SwapChain* swapChain) {
    Destroy((SwapChainVK*)swapChain);
}

static Texture* const* NRI_CALL GetSwapChainTextures(const SwapChain& swapChain, uint32_t& textureNum) {
    return ((SwapChainVK&)swapChain).GetTextures(textureNum);
}

static Result NRI_CALL GetDisplayDesc(SwapChain& swapChain, DisplayDesc& displayDesc) {
    return ((SwapChainVK&)swapChain).GetDisplayDesc(displayDesc);
}

static Result NRI_CALL AcquireNextTexture(SwapChain& swapChain, Fence& acquireSemaphore, uint32_t& textureIndex) {
    return ((SwapChainVK&)swapChain).AcquireNextTexture((FenceVK&)acquireSemaphore, textureIndex);
}

static Result NRI_CALL WaitForPresent(SwapChain& swapChain) {
    return ((SwapChainVK&)swapChain).WaitForPresent();
}

static Result NRI_CALL QueuePresent(SwapChain& swapChain, Fence& releaseSemaphore) {
    return ((SwapChainVK&)swapChain).Present((FenceVK&)releaseSemaphore);
}

Result DeviceVK::FillFunctionTable(SwapChainInterface& table) const {
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
    DeviceVK& deviceVK = (DeviceVK&)device;
    UpscalerImpl* impl = Allocate<UpscalerImpl>(deviceVK.GetAllocationCallbacks(), device, deviceVK.GetCoreInterface());
    Result result = impl->Create(upscalerDesc);

    if (result != Result::SUCCESS) {
        Destroy(impl);
        upscaler = nullptr;
    } else
        upscaler = (Upscaler*)impl;

    return result;
}

static void NRI_CALL DestroyUpscaler(Upscaler* upscaler) {
    Destroy((UpscalerImpl*)upscaler);
}

static bool NRI_CALL IsUpscalerSupported(const Device& device, UpscalerType upscalerType) {
    DeviceVK& deviceVK = (DeviceVK&)device;

    return IsUpscalerSupported(deviceVK.GetDesc(), upscalerType);
}

static void NRI_CALL GetUpscalerProps(const Upscaler& upscaler, UpscalerProps& upscalerProps) {
    UpscalerImpl& upscalerVK = (UpscalerImpl&)upscaler;

    return upscalerVK.GetUpscalerProps(upscalerProps);
}

static void NRI_CALL CmdDispatchUpscale(CommandBuffer& commandBuffer, Upscaler& upscaler, const DispatchUpscaleDesc& dispatchUpscalerDesc) {
    UpscalerImpl& upscalerVK = (UpscalerImpl&)upscaler;

    upscalerVK.CmdDispatchUpscale(commandBuffer, dispatchUpscalerDesc);
}

Result DeviceVK::FillFunctionTable(UpscalerInterface& table) const {
    table.CreateUpscaler = ::CreateUpscaler;
    table.DestroyUpscaler = ::DestroyUpscaler;
    table.IsUpscalerSupported = ::IsUpscalerSupported;
    table.GetUpscalerProps = ::GetUpscalerProps;
    table.CmdDispatchUpscale = ::CmdDispatchUpscale;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  WrapperVK  ]

static Result NRI_CALL CreateCommandAllocatorVK(Device& device, const CommandAllocatorVKDesc& commandAllocatorVKDesc, CommandAllocator*& commandAllocator) {
    return ((DeviceVK&)device).CreateImplementation<CommandAllocatorVK>(commandAllocator, commandAllocatorVKDesc);
}

static Result NRI_CALL CreateCommandBufferVK(Device& device, const CommandBufferVKDesc& commandBufferVKDesc, CommandBuffer*& commandBuffer) {
    return ((DeviceVK&)device).CreateImplementation<CommandBufferVK>(commandBuffer, commandBufferVKDesc);
}

static Result NRI_CALL CreateDescriptorPoolVK(Device& device, const DescriptorPoolVKDesc& descriptorPoolVKDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceVK&)device).CreateImplementation<DescriptorPoolVK>(descriptorPool, descriptorPoolVKDesc);
}

static Result NRI_CALL CreateBufferVK(Device& device, const BufferVKDesc& bufferVKDesc, Buffer*& buffer) {
    return ((DeviceVK&)device).CreateImplementation<BufferVK>(buffer, bufferVKDesc);
}

static Result NRI_CALL CreateTextureVK(Device& device, const TextureVKDesc& textureVKDesc, Texture*& texture) {
    return ((DeviceVK&)device).CreateImplementation<TextureVK>(texture, textureVKDesc);
}

static Result NRI_CALL CreateMemoryVK(Device& device, const MemoryVKDesc& memoryVKDesc, Memory*& memory) {
    return ((DeviceVK&)device).CreateImplementation<MemoryVK>(memory, memoryVKDesc);
}

static Result NRI_CALL CreatePipelineVK(Device& device, const PipelineVKDesc& pipelineVKDesc, Pipeline*& pipeline) {
    return ((DeviceVK&)device).CreateImplementation<PipelineVK>(pipeline, pipelineVKDesc);
}

static Result NRI_CALL CreateQueryPoolVK(Device& device, const QueryPoolVKDesc& queryPoolVKDesc, QueryPool*& queryPool) {
    return ((DeviceVK&)device).CreateImplementation<QueryPoolVK>(queryPool, queryPoolVKDesc);
}

static Result NRI_CALL CreateFenceVK(Device& device, const FenceVKDesc& fenceVKDesc, Fence*& fence) {
    return ((DeviceVK&)device).CreateImplementation<FenceVK>(fence, fenceVKDesc);
}

static Result NRI_CALL CreateAccelerationStructureVK(Device& device, const AccelerationStructureVKDesc& accelerationStructureVKDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVK&)device).CreateImplementation<AccelerationStructureVK>(accelerationStructure, accelerationStructureVKDesc);
}

static void NRI_CALL CmdDecodeVideoVK(CommandBuffer& commandBuffer, const VideoDecodeVKDesc& videoDecodeVKDesc) {
    ((CommandBufferVK&)commandBuffer).DecodeVideo(videoDecodeVKDesc);
}

static void NRI_CALL CmdEncodeVideoVK(CommandBuffer& commandBuffer, const VideoEncodeVKDesc& videoEncodeVKDesc) {
    ((CommandBufferVK&)commandBuffer).EncodeVideo(videoEncodeVKDesc);
}

static uint32_t NRI_CALL GetQueueFamilyIndexVK(const Queue& queue) {
    return ((QueueVK&)queue).GetFamilyIndex();
}

static VKHandle NRI_CALL GetPhysicalDeviceVK(const Device& device) {
    return (VkPhysicalDevice)((DeviceVK&)device);
}

static VKHandle NRI_CALL GetInstanceVK(const Device& device) {
    return (VkInstance)((DeviceVK&)device);
}

static void* NRI_CALL GetInstanceProcAddrVK(const Device& device) {
    return (void*)((DeviceVK&)device).GetDispatchTable().GetInstanceProcAddr;
}

static void* NRI_CALL GetDeviceProcAddrVK(const Device& device) {
    return (void*)((DeviceVK&)device).GetDispatchTable().GetDeviceProcAddr;
}

Result DeviceVK::FillFunctionTable(WrapperVKInterface& table) const {
    table.CreateCommandAllocatorVK = ::CreateCommandAllocatorVK;
    table.CreateCommandBufferVK = ::CreateCommandBufferVK;
    table.CreateDescriptorPoolVK = ::CreateDescriptorPoolVK;
    table.CreateBufferVK = ::CreateBufferVK;
    table.CreateTextureVK = ::CreateTextureVK;
    table.CreateMemoryVK = ::CreateMemoryVK;
    table.CreatePipelineVK = ::CreatePipelineVK;
    table.CreateQueryPoolVK = ::CreateQueryPoolVK;
    table.CreateFenceVK = ::CreateFenceVK;
    table.CreateAccelerationStructureVK = ::CreateAccelerationStructureVK;
    table.CmdDecodeVideoVK = ::CmdDecodeVideoVK;
    table.CmdEncodeVideoVK = ::CmdEncodeVideoVK;
    table.GetQueueFamilyIndexVK = ::GetQueueFamilyIndexVK;
    table.GetPhysicalDeviceVK = ::GetPhysicalDeviceVK;
    table.GetInstanceVK = ::GetInstanceVK;
    table.GetDeviceProcAddrVK = ::GetDeviceProcAddrVK;
    table.GetInstanceProcAddrVK = ::GetInstanceProcAddrVK;

    return Result::SUCCESS;
}

#pragma endregion
