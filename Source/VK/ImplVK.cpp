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
#include "VideoHelpersVK.h"

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
    bool m_Initialized = false;
    bool m_UseInlineSessionParameters = false;
};

static StdVideoH264LevelIdc GetVideoH264LevelIdcVK(uint8_t levelIdc) {
    switch (levelIdc) {
    case 10:
        return STD_VIDEO_H264_LEVEL_IDC_1_0;
    case 11:
        return STD_VIDEO_H264_LEVEL_IDC_1_1;
    case 12:
        return STD_VIDEO_H264_LEVEL_IDC_1_2;
    case 13:
        return STD_VIDEO_H264_LEVEL_IDC_1_3;
    case 20:
        return STD_VIDEO_H264_LEVEL_IDC_2_0;
    case 21:
        return STD_VIDEO_H264_LEVEL_IDC_2_1;
    case 22:
        return STD_VIDEO_H264_LEVEL_IDC_2_2;
    case 30:
        return STD_VIDEO_H264_LEVEL_IDC_3_0;
    case 31:
        return STD_VIDEO_H264_LEVEL_IDC_3_1;
    case 32:
        return STD_VIDEO_H264_LEVEL_IDC_3_2;
    case 40:
        return STD_VIDEO_H264_LEVEL_IDC_4_0;
    case 41:
        return STD_VIDEO_H264_LEVEL_IDC_4_1;
    case 42:
        return STD_VIDEO_H264_LEVEL_IDC_4_2;
    case 50:
        return STD_VIDEO_H264_LEVEL_IDC_5_0;
    case 51:
        return STD_VIDEO_H264_LEVEL_IDC_5_1;
    case 52:
        return STD_VIDEO_H264_LEVEL_IDC_5_2;
    case 60:
        return STD_VIDEO_H264_LEVEL_IDC_6_0;
    case 61:
        return STD_VIDEO_H264_LEVEL_IDC_6_1;
    case 62:
        return STD_VIDEO_H264_LEVEL_IDC_6_2;
    }

    return STD_VIDEO_H264_LEVEL_IDC_INVALID;
}

static StdVideoH264SequenceParameterSet GetVideoH264SequenceParameterSetVK(const VideoH264SequenceParameterSetDesc& desc) {
    StdVideoH264SequenceParameterSet sps = {};
    sps.flags.constraint_set0_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET0);
    sps.flags.constraint_set1_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET1);
    sps.flags.constraint_set2_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET2);
    sps.flags.constraint_set3_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET3);
    sps.flags.constraint_set4_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET4);
    sps.flags.constraint_set5_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::CONSTRAINT_SET5);
    sps.flags.direct_8x8_inference_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::DIRECT_8X8_INFERENCE);
    sps.flags.mb_adaptive_frame_field_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::MB_ADAPTIVE_FRAME_FIELD);
    sps.flags.frame_mbs_only_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::FRAME_MBS_ONLY);
    sps.flags.delta_pic_order_always_zero_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::DELTA_PIC_ORDER_ALWAYS_ZERO);
    sps.flags.separate_colour_plane_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::SEPARATE_COLOUR_PLANE);
    sps.flags.gaps_in_frame_num_value_allowed_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::GAPS_IN_FRAME_NUM_ALLOWED);
    sps.flags.qpprime_y_zero_transform_bypass_flag = !!(desc.flags & VideoH264SequenceParameterSetBits::QPPRIME_Y_ZERO_TRANSFORM_BYPASS);
    sps.profile_idc = (StdVideoH264ProfileIdc)desc.profileIdc;
    sps.level_idc = GetVideoH264LevelIdcVK(desc.levelIdc);
    sps.chroma_format_idc = (StdVideoH264ChromaFormatIdc)desc.chromaFormatIdc;
    sps.seq_parameter_set_id = desc.sequenceParameterSetId;
    sps.bit_depth_luma_minus8 = desc.bitDepthLumaMinus8;
    sps.bit_depth_chroma_minus8 = desc.bitDepthChromaMinus8;
    sps.log2_max_frame_num_minus4 = desc.log2MaxFrameNumMinus4;
    sps.pic_order_cnt_type = (StdVideoH264PocType)desc.pictureOrderCountType;
    sps.offset_for_non_ref_pic = desc.offsetForNonReferencePicture;
    sps.offset_for_top_to_bottom_field = desc.offsetForTopToBottomField;
    sps.log2_max_pic_order_cnt_lsb_minus4 = desc.log2MaxPictureOrderCountLsbMinus4;
    sps.max_num_ref_frames = desc.referenceFrameNum;
    sps.pic_width_in_mbs_minus1 = desc.pictureWidthInMbsMinus1;
    sps.pic_height_in_map_units_minus1 = desc.pictureHeightInMapUnitsMinus1;
    return sps;
}

static StdVideoH264PictureParameterSet GetVideoH264PictureParameterSetVK(const VideoH264PictureParameterSetDesc& desc) {
    StdVideoH264PictureParameterSet pps = {};
    pps.flags.transform_8x8_mode_flag = !!(desc.flags & VideoH264PictureParameterSetBits::TRANSFORM_8X8_MODE);
    pps.flags.redundant_pic_cnt_present_flag = !!(desc.flags & VideoH264PictureParameterSetBits::REDUNDANT_PIC_CNT_PRESENT);
    pps.flags.constrained_intra_pred_flag = !!(desc.flags & VideoH264PictureParameterSetBits::CONSTRAINED_INTRA_PRED);
    pps.flags.deblocking_filter_control_present_flag = !!(desc.flags & VideoH264PictureParameterSetBits::DEBLOCKING_FILTER_CONTROL_PRESENT);
    pps.flags.weighted_pred_flag = !!(desc.flags & VideoH264PictureParameterSetBits::WEIGHTED_PRED);
    pps.flags.bottom_field_pic_order_in_frame_present_flag = !!(desc.flags & VideoH264PictureParameterSetBits::BOTTOM_FIELD_PIC_ORDER_IN_FRAME);
    pps.flags.entropy_coding_mode_flag = !!(desc.flags & VideoH264PictureParameterSetBits::ENTROPY_CODING_MODE);
    pps.seq_parameter_set_id = desc.sequenceParameterSetId;
    pps.pic_parameter_set_id = desc.pictureParameterSetId;
    pps.num_ref_idx_l0_default_active_minus1 = desc.refIndexL0DefaultActiveMinus1;
    pps.num_ref_idx_l1_default_active_minus1 = desc.refIndexL1DefaultActiveMinus1;
    pps.weighted_bipred_idc = (StdVideoH264WeightedBipredIdc)desc.weightedBipredIdc;
    pps.pic_init_qp_minus26 = desc.pictureInitQpMinus26;
    pps.pic_init_qs_minus26 = desc.pictureInitQsMinus26;
    pps.chroma_qp_index_offset = desc.chromaQpIndexOffset;
    pps.second_chroma_qp_index_offset = desc.secondChromaQpIndexOffset;
    return pps;
}

static StdVideoH265LevelIdc GetVideoH265LevelIdcVK(uint32_t width, uint32_t height) {
    const uint64_t samples = uint64_t(width) * height;
    if (samples <= 512ull * 512ull)
        return STD_VIDEO_H265_LEVEL_IDC_3_1;

    return STD_VIDEO_H265_LEVEL_IDC_4_1;
}

struct VideoSessionParametersVK final {
    inline VideoSessionParametersVK(DeviceVK& device)
        : m_Device(device)
        , m_H264Sps(device.GetStdAllocator())
        , m_H264Pps(device.GetStdAllocator())
        , m_H265VpsProfileTierLevels(device.GetStdAllocator())
        , m_H265SpsProfileTierLevels(device.GetStdAllocator())
        , m_H265VpsDecPicBufMgrs(device.GetStdAllocator())
        , m_H265SpsDecPicBufMgrs(device.GetStdAllocator())
        , m_H265Vps(device.GetStdAllocator())
        , m_H265Sps(device.GetStdAllocator())
        , m_H265Pps(device.GetStdAllocator())
        , m_H265SpsScalingLists(device.GetStdAllocator())
        , m_H265PpsScalingLists(device.GetStdAllocator())
        , m_H265ShortTermRefPicSets(device.GetStdAllocator())
        , m_H265LongTermRefPicsSps(device.GetStdAllocator()) {
        for (auto& list : m_H264DefaultScalingLists.ScalingList4x4) {
            for (uint8_t& entry : list)
                entry = 16;
        }

        for (auto& list : m_H264DefaultScalingLists.ScalingList8x8) {
            for (uint8_t& entry : list)
                entry = 16;
        }
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~VideoSessionParametersVK() {
        const auto& vk = m_Device.GetDispatchTable();
        if (m_Handle)
            vk.DestroyVideoSessionParametersKHR(m_Device, m_Handle, m_Device.GetVkAllocationCallbacks());
    }

    Result CreateNative(VideoSessionVK& session, const void* pNext) {
        const auto& vk = m_Device.GetDispatchTable();
        if (!vk.CreateVideoSessionParametersKHR)
            return Result::UNSUPPORTED;

        m_Session = &session;

        VkVideoSessionParametersCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR};
        createInfo.pNext = pNext;
        createInfo.videoSession = session.m_Handle;

        VkResult vkResult = vk.CreateVideoSessionParametersKHR(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
        NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateVideoSessionParametersKHR");

        return Result::SUCCESS;
    }

    Result Create(const VideoSessionParametersDesc& videoSessionParametersDesc) {
        if (!videoSessionParametersDesc.session)
            return Result::INVALID_ARGUMENT;

        const auto& vk = m_Device.GetDispatchTable();
        if (!vk.CreateVideoSessionParametersKHR)
            return Result::UNSUPPORTED;

        VideoSessionVK& session = *(VideoSessionVK*)videoSessionParametersDesc.session;
        m_Session = &session;
        if (session.m_Desc.codec == VideoCodec::H265) {
            m_H265Parameters = videoSessionParametersDesc.h265Parameters;
            return CreateH265(session);
        }
        if (session.m_Desc.codec == VideoCodec::AV1) {
            m_AV1Parameters = videoSessionParametersDesc.av1Parameters;
            return CreateAV1(session);
        }
        if (session.m_Desc.codec != VideoCodec::H264)
            return Result::UNSUPPORTED;

        const VideoH264SessionParametersDesc emptyH264Parameters = {};
        const VideoH264SessionParametersDesc& h264Parameters = videoSessionParametersDesc.h264Parameters ? *videoSessionParametersDesc.h264Parameters : emptyH264Parameters;

        m_H264Sps.resize(h264Parameters.sequenceParameterSetNum);
        for (uint32_t i = 0; i < h264Parameters.sequenceParameterSetNum; i++) {
            m_H264Sps[i] = GetVideoH264SequenceParameterSetVK(h264Parameters.sequenceParameterSets[i]);
            m_H264Sps[i].pScalingLists = &m_H264DefaultScalingLists;
        }

        m_H264Pps.resize(h264Parameters.pictureParameterSetNum);
        for (uint32_t i = 0; i < h264Parameters.pictureParameterSetNum; i++) {
            m_H264Pps[i] = GetVideoH264PictureParameterSetVK(h264Parameters.pictureParameterSets[i]);
            m_H264Pps[i].pScalingLists = &m_H264DefaultScalingLists;
        }

        VkVideoDecodeH264SessionParametersAddInfoKHR decodeAddInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR};
        decodeAddInfo.stdSPSCount = h264Parameters.sequenceParameterSetNum;
        decodeAddInfo.pStdSPSs = m_H264Sps.data();
        decodeAddInfo.stdPPSCount = h264Parameters.pictureParameterSetNum;
        decodeAddInfo.pStdPPSs = m_H264Pps.data();

        VkVideoDecodeH264SessionParametersCreateInfoKHR decodeInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR};
        decodeInfo.maxStdSPSCount = h264Parameters.maxSequenceParameterSetNum ? h264Parameters.maxSequenceParameterSetNum : h264Parameters.sequenceParameterSetNum;
        decodeInfo.maxStdPPSCount = h264Parameters.maxPictureParameterSetNum ? h264Parameters.maxPictureParameterSetNum : h264Parameters.pictureParameterSetNum;
        decodeInfo.pParametersAddInfo = h264Parameters.sequenceParameterSetNum || h264Parameters.pictureParameterSetNum ? &decodeAddInfo : nullptr;

        VkVideoEncodeH264SessionParametersAddInfoKHR encodeAddInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR};
        encodeAddInfo.stdSPSCount = h264Parameters.sequenceParameterSetNum;
        encodeAddInfo.pStdSPSs = m_H264Sps.data();
        encodeAddInfo.stdPPSCount = h264Parameters.pictureParameterSetNum;
        encodeAddInfo.pStdPPSs = m_H264Pps.data();

        VkVideoEncodeH264SessionParametersCreateInfoKHR encodeInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR};
        encodeInfo.maxStdSPSCount = h264Parameters.maxSequenceParameterSetNum ? h264Parameters.maxSequenceParameterSetNum : h264Parameters.sequenceParameterSetNum;
        encodeInfo.maxStdPPSCount = h264Parameters.maxPictureParameterSetNum ? h264Parameters.maxPictureParameterSetNum : h264Parameters.pictureParameterSetNum;
        encodeInfo.pParametersAddInfo = h264Parameters.sequenceParameterSetNum || h264Parameters.pictureParameterSetNum ? &encodeAddInfo : nullptr;

        return CreateNative(session, session.m_Desc.usage == VideoUsage::DECODE ? (const void*)&decodeInfo : (const void*)&encodeInfo);
    }

    Result CreateH265(VideoSessionVK& session) {
        VideoH265VideoParameterSetDesc defaultVps = {};
        VideoH265SequenceParameterSetDesc defaultSps = {};
        VideoH265PictureParameterSetDesc defaultPps = {};
        VideoH265SessionParametersDesc defaultParameters = {};
        if (!m_H265Parameters) {
            const uint8_t bitDepthMinus8 = session.m_Desc.format == Format::P010_UNORM || session.m_Desc.format == Format::P016_UNORM ? 2 : 0;
            defaultVps.flags = VideoH265VideoParameterSetBits::TEMPORAL_ID_NESTING;
            defaultVps.profileTierLevel.generalProfileIdc = (uint8_t)(session.m_Desc.format == Format::P010_UNORM || session.m_Desc.format == Format::P016_UNORM
                ? STD_VIDEO_H265_PROFILE_IDC_MAIN_10
                : STD_VIDEO_H265_PROFILE_IDC_MAIN);
            defaultVps.profileTierLevel.generalLevelIdc = (uint8_t)GetVideoH265LevelIdcVK(session.m_Desc.width, session.m_Desc.height);
            defaultVps.decPicBufMgr.maxDecPicBufferingMinus1[0] = (uint8_t)std::min(session.m_Desc.maxReferenceNum ? session.m_Desc.maxReferenceNum : 1u, 15u);

            defaultSps.flags = VideoH265SequenceParameterSetBits::TEMPORAL_ID_NESTING |
                VideoH265SequenceParameterSetBits::STRONG_INTRA_SMOOTHING_ENABLED;
            defaultSps.chromaFormatIdc = STD_VIDEO_H265_CHROMA_FORMAT_IDC_420;
            defaultSps.pictureWidthInLumaSamples = session.m_Desc.width;
            defaultSps.pictureHeightInLumaSamples = session.m_Desc.height;
            defaultSps.bitDepthLumaMinus8 = bitDepthMinus8;
            defaultSps.bitDepthChromaMinus8 = bitDepthMinus8;
            defaultSps.log2MaxPictureOrderCountLsbMinus4 = 4;
            defaultSps.log2MinLumaCodingBlockSizeMinus3 = 0;
            defaultSps.log2DiffMaxMinLumaCodingBlockSize = 2;
            defaultSps.log2MinLumaTransformBlockSizeMinus2 = 0;
            defaultSps.log2DiffMaxMinLumaTransformBlockSize = 2;
            defaultSps.maxTransformHierarchyDepthInter = 2;
            defaultSps.maxTransformHierarchyDepthIntra = 2;
            defaultSps.profileTierLevel = defaultVps.profileTierLevel;
            defaultSps.decPicBufMgr = defaultVps.decPicBufMgr;

            defaultPps.flags = VideoH265PictureParameterSetBits::LOOP_FILTER_ACROSS_SLICES_ENABLED;
            defaultPps.log2ParallelMergeLevelMinus2 = 2;

            defaultParameters.videoParameterSets = &defaultVps;
            defaultParameters.videoParameterSetNum = 1;
            defaultParameters.sequenceParameterSets = &defaultSps;
            defaultParameters.sequenceParameterSetNum = 1;
            defaultParameters.pictureParameterSets = &defaultPps;
            defaultParameters.pictureParameterSetNum = 1;
            m_H265Parameters = &defaultParameters;
        }

        if ((m_H265Parameters->videoParameterSetNum && !m_H265Parameters->videoParameterSets) ||
            (m_H265Parameters->sequenceParameterSetNum && !m_H265Parameters->sequenceParameterSets) ||
            (m_H265Parameters->pictureParameterSetNum && !m_H265Parameters->pictureParameterSets))
            return Result::INVALID_ARGUMENT;

        m_H265VpsProfileTierLevels.resize(m_H265Parameters->videoParameterSetNum);
        m_H265SpsProfileTierLevels.resize(m_H265Parameters->sequenceParameterSetNum);
        m_H265VpsDecPicBufMgrs.resize(m_H265Parameters->videoParameterSetNum);
        m_H265SpsDecPicBufMgrs.resize(m_H265Parameters->sequenceParameterSetNum);
        m_H265Vps.resize(m_H265Parameters->videoParameterSetNum);
        m_H265Sps.resize(m_H265Parameters->sequenceParameterSetNum);
        m_H265Pps.resize(m_H265Parameters->pictureParameterSetNum);
        m_H265SpsScalingLists.resize(m_H265Parameters->sequenceParameterSetNum);
        m_H265PpsScalingLists.resize(m_H265Parameters->pictureParameterSetNum);
        uint32_t shortTermRefPicSetNum = 0;
        for (uint32_t i = 0; i < m_H265Parameters->sequenceParameterSetNum; i++) {
            const VideoH265SequenceParameterSetDesc& sps = m_H265Parameters->sequenceParameterSets[i];
            if (sps.numShortTermRefPicSets && !sps.shortTermRefPicSets)
                return Result::INVALID_ARGUMENT;
            shortTermRefPicSetNum += sps.numShortTermRefPicSets;
        }
        m_H265ShortTermRefPicSets.resize(shortTermRefPicSetNum);
        m_H265LongTermRefPicsSps.resize(m_H265Parameters->sequenceParameterSetNum);

        for (uint32_t i = 0; i < m_H265Parameters->videoParameterSetNum; i++) {
            const VideoH265VideoParameterSetDesc& vps = m_H265Parameters->videoParameterSets[i];
            FillVideoH265ProfileTierLevelVK(m_H265VpsProfileTierLevels[i], vps.profileTierLevel);
            FillVideoH265DecPicBufMgrVK(m_H265VpsDecPicBufMgrs[i], vps.decPicBufMgr);
            m_H265Vps[i] = GetVideoH265VideoParameterSetVK(vps, m_H265VpsProfileTierLevels[i], m_H265VpsDecPicBufMgrs[i]);
        }

        uint32_t firstShortTermRefPicSet = 0;
        for (uint32_t i = 0; i < m_H265Parameters->sequenceParameterSetNum; i++) {
            const VideoH265SequenceParameterSetDesc& sps = m_H265Parameters->sequenceParameterSets[i];
            for (uint32_t j = 0; j < sps.numShortTermRefPicSets; j++)
                m_H265ShortTermRefPicSets[firstShortTermRefPicSet + j] = GetVideoH265ShortTermRefPicSetVK(sps.shortTermRefPicSets[j]);

            FillVideoH265ProfileTierLevelVK(m_H265SpsProfileTierLevels[i], sps.profileTierLevel);
            FillVideoH265DecPicBufMgrVK(m_H265SpsDecPicBufMgrs[i], sps.decPicBufMgr);
            const StdVideoH265ScalingLists* scalingLists = nullptr;
            if (sps.scalingLists) {
                m_H265SpsScalingLists[i] = GetVideoH265ScalingListsVK(*sps.scalingLists);
                scalingLists = &m_H265SpsScalingLists[i];
            }
            const StdVideoH265ShortTermRefPicSet* shortTermRefPicSets = sps.numShortTermRefPicSets ? &m_H265ShortTermRefPicSets[firstShortTermRefPicSet] : nullptr;
            const StdVideoH265LongTermRefPicsSps* longTermRefPicsSps = nullptr;
            if (sps.longTermRefPicsSps) {
                m_H265LongTermRefPicsSps[i] = GetVideoH265LongTermRefPicsSpsVK(*sps.longTermRefPicsSps);
                longTermRefPicsSps = &m_H265LongTermRefPicsSps[i];
            }
            m_H265Sps[i] = GetVideoH265SequenceParameterSetVK(sps, m_H265SpsProfileTierLevels[i], m_H265SpsDecPicBufMgrs[i], scalingLists, shortTermRefPicSets,
                longTermRefPicsSps);
            firstShortTermRefPicSet += sps.numShortTermRefPicSets;
        }

        for (uint32_t i = 0; i < m_H265Parameters->pictureParameterSetNum; i++) {
            const VideoH265PictureParameterSetDesc& pps = m_H265Parameters->pictureParameterSets[i];
            const StdVideoH265ScalingLists* scalingLists = nullptr;
            if (pps.scalingLists) {
                m_H265PpsScalingLists[i] = GetVideoH265ScalingListsVK(*pps.scalingLists);
                scalingLists = &m_H265PpsScalingLists[i];
            }
            m_H265Pps[i] = GetVideoH265PictureParameterSetVK(pps, scalingLists);
        }

        VkVideoDecodeH265SessionParametersAddInfoKHR decodeAddInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_ADD_INFO_KHR};
        decodeAddInfo.stdVPSCount = m_H265Parameters->videoParameterSetNum;
        decodeAddInfo.pStdVPSs = m_H265Vps.data();
        decodeAddInfo.stdSPSCount = m_H265Parameters->sequenceParameterSetNum;
        decodeAddInfo.pStdSPSs = m_H265Sps.data();
        decodeAddInfo.stdPPSCount = m_H265Parameters->pictureParameterSetNum;
        decodeAddInfo.pStdPPSs = m_H265Pps.data();

        VkVideoDecodeH265SessionParametersCreateInfoKHR decodeInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_CREATE_INFO_KHR};
        decodeInfo.maxStdVPSCount = m_H265Parameters->maxVideoParameterSetNum ? m_H265Parameters->maxVideoParameterSetNum : m_H265Parameters->videoParameterSetNum;
        decodeInfo.maxStdSPSCount = m_H265Parameters->maxSequenceParameterSetNum ? m_H265Parameters->maxSequenceParameterSetNum : m_H265Parameters->sequenceParameterSetNum;
        decodeInfo.maxStdPPSCount = m_H265Parameters->maxPictureParameterSetNum ? m_H265Parameters->maxPictureParameterSetNum : m_H265Parameters->pictureParameterSetNum;
        decodeInfo.pParametersAddInfo = m_H265Parameters->videoParameterSetNum || m_H265Parameters->sequenceParameterSetNum || m_H265Parameters->pictureParameterSetNum
            ? &decodeAddInfo
            : nullptr;

        VkVideoEncodeH265SessionParametersAddInfoKHR encodeAddInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_ADD_INFO_KHR};
        encodeAddInfo.stdVPSCount = m_H265Parameters->videoParameterSetNum;
        encodeAddInfo.pStdVPSs = m_H265Vps.data();
        encodeAddInfo.stdSPSCount = m_H265Parameters->sequenceParameterSetNum;
        encodeAddInfo.pStdSPSs = m_H265Sps.data();
        encodeAddInfo.stdPPSCount = m_H265Parameters->pictureParameterSetNum;
        encodeAddInfo.pStdPPSs = m_H265Pps.data();

        VkVideoEncodeH265SessionParametersCreateInfoKHR encodeInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_CREATE_INFO_KHR};
        encodeInfo.maxStdVPSCount = m_H265Parameters->maxVideoParameterSetNum ? m_H265Parameters->maxVideoParameterSetNum : m_H265Parameters->videoParameterSetNum;
        encodeInfo.maxStdSPSCount = m_H265Parameters->maxSequenceParameterSetNum ? m_H265Parameters->maxSequenceParameterSetNum : m_H265Parameters->sequenceParameterSetNum;
        encodeInfo.maxStdPPSCount = m_H265Parameters->maxPictureParameterSetNum ? m_H265Parameters->maxPictureParameterSetNum : m_H265Parameters->pictureParameterSetNum;
        encodeInfo.pParametersAddInfo = m_H265Parameters->videoParameterSetNum || m_H265Parameters->sequenceParameterSetNum || m_H265Parameters->pictureParameterSetNum
            ? &encodeAddInfo
            : nullptr;

        return CreateNative(session, session.m_Desc.usage == VideoUsage::DECODE ? (const void*)&decodeInfo : (const void*)&encodeInfo);
    }

    Result CreateAV1(VideoSessionVK& session) {
        if (m_AV1Parameters) {
            FillVideoAV1ColorConfigVK(m_AV1ColorConfig, m_AV1Parameters->sequence);
            m_AV1TimingInfo = {};
            const StdVideoAV1TimingInfo* timingInfo = nullptr;
            if (m_AV1Parameters->sequence.numUnitsInDisplayTick && m_AV1Parameters->sequence.timeScale) {
                m_AV1TimingInfo.num_units_in_display_tick = m_AV1Parameters->sequence.numUnitsInDisplayTick;
                m_AV1TimingInfo.time_scale = m_AV1Parameters->sequence.timeScale;
                m_AV1TimingInfo.num_ticks_per_picture_minus_1 = m_AV1Parameters->sequence.numTicksPerPictureMinus1;
                timingInfo = &m_AV1TimingInfo;
            }
            if (session.m_Desc.usage == VideoUsage::DECODE) {
                m_AV1ColorConfig.flags.color_description_present_flag = false;
                m_AV1ColorConfig.chroma_sample_position = {};
                timingInfo = &m_AV1TimingInfo;
            }
            FillVideoAV1SequenceHeaderVK(m_AV1SequenceHeader, m_AV1Parameters->sequence, m_AV1ColorConfig, timingInfo);
            m_AV1OperatingPoint.seq_level_idx = (uint8_t)GetVideoAV1LevelVK(m_AV1Parameters->sequence.level, session.m_Desc.width, session.m_Desc.height);
        } else {
            m_AV1ColorConfig.BitDepth = session.m_Desc.format == Format::P010_UNORM || session.m_Desc.format == Format::P016_UNORM ? 10 : 8;
            m_AV1ColorConfig.subsampling_x = 1;
            m_AV1ColorConfig.subsampling_y = 1;
            m_AV1ColorConfig.flags.color_description_present_flag = true;
            m_AV1ColorConfig.color_primaries = STD_VIDEO_AV1_COLOR_PRIMARIES_BT_709;
            m_AV1ColorConfig.transfer_characteristics = STD_VIDEO_AV1_TRANSFER_CHARACTERISTICS_BT_709;
            m_AV1ColorConfig.matrix_coefficients = STD_VIDEO_AV1_MATRIX_COEFFICIENTS_BT_709;
            m_AV1ColorConfig.chroma_sample_position = STD_VIDEO_AV1_CHROMA_SAMPLE_POSITION_VERTICAL;
            m_AV1SequenceHeader.seq_profile = STD_VIDEO_AV1_PROFILE_MAIN;
            m_AV1SequenceHeader.flags.enable_order_hint = true;
            m_AV1SequenceHeader.frame_width_bits_minus_1 = GetVideoAV1SizeBitsMinus1VK(session.m_Desc.width);
            m_AV1SequenceHeader.frame_height_bits_minus_1 = GetVideoAV1SizeBitsMinus1VK(session.m_Desc.height);
            m_AV1SequenceHeader.max_frame_width_minus_1 = (uint16_t)(session.m_Desc.width - 1);
            m_AV1SequenceHeader.max_frame_height_minus_1 = (uint16_t)(session.m_Desc.height - 1);
            m_AV1SequenceHeader.order_hint_bits_minus_1 = 7;
            m_AV1SequenceHeader.seq_force_integer_mv = STD_VIDEO_AV1_SELECT_INTEGER_MV;
            m_AV1SequenceHeader.seq_force_screen_content_tools = STD_VIDEO_AV1_SELECT_SCREEN_CONTENT_TOOLS;
            m_AV1SequenceHeader.pColorConfig = &m_AV1ColorConfig;
            m_AV1OperatingPoint.seq_level_idx = (uint8_t)GetVideoAV1LevelVK(session.m_Desc.width, session.m_Desc.height);
        }

        VkVideoDecodeAV1SessionParametersCreateInfoKHR decodeInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_SESSION_PARAMETERS_CREATE_INFO_KHR};
        decodeInfo.pStdSequenceHeader = &m_AV1SequenceHeader;

        VkVideoEncodeAV1SessionParametersCreateInfoKHR encodeInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_SESSION_PARAMETERS_CREATE_INFO_KHR};
        encodeInfo.pStdSequenceHeader = &m_AV1SequenceHeader;

        if (session.m_UseInlineSessionParameters && session.m_Desc.usage == VideoUsage::DECODE)
            return Result::SUCCESS;

        return CreateNative(session, session.m_Desc.usage == VideoUsage::DECODE ? (const void*)&decodeInfo : (const void*)&encodeInfo);
    }

    DeviceVK& m_Device;
    VideoSessionVK* m_Session = nullptr;
    VkVideoSessionParametersKHR m_Handle = VK_NULL_HANDLE;
    StdVideoH264ScalingLists m_H264DefaultScalingLists = {};
    Vector<StdVideoH264SequenceParameterSet> m_H264Sps;
    Vector<StdVideoH264PictureParameterSet> m_H264Pps;
    Vector<StdVideoH265ProfileTierLevel> m_H265VpsProfileTierLevels;
    Vector<StdVideoH265ProfileTierLevel> m_H265SpsProfileTierLevels;
    Vector<StdVideoH265DecPicBufMgr> m_H265VpsDecPicBufMgrs;
    Vector<StdVideoH265DecPicBufMgr> m_H265SpsDecPicBufMgrs;
    Vector<StdVideoH265VideoParameterSet> m_H265Vps;
    Vector<StdVideoH265SequenceParameterSet> m_H265Sps;
    Vector<StdVideoH265PictureParameterSet> m_H265Pps;
    Vector<StdVideoH265ScalingLists> m_H265SpsScalingLists;
    Vector<StdVideoH265ScalingLists> m_H265PpsScalingLists;
    Vector<StdVideoH265ShortTermRefPicSet> m_H265ShortTermRefPicSets;
    Vector<StdVideoH265LongTermRefPicsSps> m_H265LongTermRefPicsSps;
    StdVideoAV1ColorConfig m_AV1ColorConfig = {};
    StdVideoAV1TimingInfo m_AV1TimingInfo = {};
    StdVideoAV1SequenceHeader m_AV1SequenceHeader = {};
    StdVideoEncodeAV1OperatingPointInfo m_AV1OperatingPoint = {};
    const VideoH265SessionParametersDesc* m_H265Parameters = nullptr;
    const VideoAV1SessionParametersDesc* m_AV1Parameters = nullptr;
};

struct VideoPictureVK final : public DebugNameBase {
    inline VideoPictureVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~VideoPictureVK() {
        const auto& vk = m_Device.GetDispatchTable();
        if (m_ImageView)
            vk.DestroyImageView(m_Device, m_ImageView, m_Device.GetVkAllocationCallbacks());
    }

    Result Create(const VideoPictureDesc& videoPictureDesc) {
        if (!videoPictureDesc.texture)
            return Result::INVALID_ARGUMENT;

        TextureVK& texture = *(TextureVK*)videoPictureDesc.texture;
        const TextureDesc& textureDesc = texture.GetDesc();

        VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        VkImageViewUsageCreateInfo usageInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
        createInfo.image = texture.GetHandle();
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = GetVkFormat(videoPictureDesc.format == Format::UNKNOWN ? textureDesc.format : videoPictureDesc.format);
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = videoPictureDesc.layer;
        createInfo.subresourceRange.layerCount = 1;

        TextureUsageBits requiredTextureUsage = TextureUsageBits::NONE;
        switch (videoPictureDesc.usage) {
        case VideoPictureUsage::DECODE_OUTPUT:
            requiredTextureUsage = TextureUsageBits::VIDEO_DECODE;
            usageInfo.usage |= VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
            break;
        case VideoPictureUsage::DECODE_REFERENCE:
            requiredTextureUsage = TextureUsageBits::VIDEO_DECODE;
            usageInfo.usage |= VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR;
            break;
        case VideoPictureUsage::ENCODE_INPUT:
            requiredTextureUsage = TextureUsageBits::VIDEO_ENCODE;
            usageInfo.usage |= VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR;
            break;
        case VideoPictureUsage::ENCODE_REFERENCE:
            requiredTextureUsage = TextureUsageBits::VIDEO_ENCODE;
            usageInfo.usage |= VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR;
            break;
        case VideoPictureUsage::MAX_NUM:
            return Result::INVALID_ARGUMENT;
        }

        if ((textureDesc.usage & requiredTextureUsage) == 0)
            return Result::INVALID_ARGUMENT;

        if (usageInfo.usage)
            createInfo.pNext = &usageInfo;

        const auto& vk = m_Device.GetDispatchTable();
        VkResult vkResult = vk.CreateImageView(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_ImageView);
        NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkCreateImageView");

        m_Resource = {VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR};
        m_Resource.codedOffset = {};
        m_Resource.codedExtent = {videoPictureDesc.width ? videoPictureDesc.width : textureDesc.width, videoPictureDesc.height ? videoPictureDesc.height : textureDesc.height};
        m_Resource.baseArrayLayer = videoPictureDesc.layer;
        m_Resource.imageViewBinding = m_ImageView;

        return Result::SUCCESS;
    }

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)m_ImageView, name);
    }

    DeviceVK& m_Device;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkVideoPictureResourceInfoKHR m_Resource = {};
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
            info.filmGrainSupport = VK_TRUE;
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

    VkVideoCodecOperationFlagBitsKHR operation = GetVideoCodecOperationVK(videoSessionDesc);
    if (!operation) {
        NRI_REPORT_ERROR(&m_Device, "Unsupported Vulkan video codec operation");
        return Result::UNSUPPORTED;
    }

    Queue* queue = nullptr;
    Result result = m_Device.GetVideoQueue(operation, queue);
    if (result != Result::SUCCESS) {
        NRI_REPORT_ERROR(&m_Device, "Failed to get Vulkan video queue for codec operation 0x%X", operation);
        return result;
    }

    std::aligned_storage_t<64, 8> codecProfileStorage = {};
    VkVideoProfileInfoKHR profile = {VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR};
    void* codecProfileInfo = FillVideoProfileCodecInfoVK(videoSessionDesc, &codecProfileStorage);
    VkVideoDecodeUsageInfoKHR decodeUsage = {VK_STRUCTURE_TYPE_VIDEO_DECODE_USAGE_INFO_KHR};
    VkVideoEncodeUsageInfoKHR encodeUsage = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_USAGE_INFO_KHR};
    if (videoSessionDesc.usage == VideoUsage::DECODE) {
        decodeUsage.videoUsageHints = VK_VIDEO_DECODE_USAGE_DEFAULT_KHR;
        decodeUsage.pNext = codecProfileInfo;
        profile.pNext = &decodeUsage;
    } else {
        encodeUsage.videoUsageHints = VK_VIDEO_ENCODE_USAGE_DEFAULT_KHR;
        encodeUsage.pNext = codecProfileInfo;
        profile.pNext = &encodeUsage;
    }
    profile.videoCodecOperation = operation;
    profile.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
    profile.lumaBitDepth = GetVideoBitDepthVK(videoSessionDesc.format);
    profile.chromaBitDepth = GetVideoBitDepthVK(videoSessionDesc.format);
    if (!codecProfileInfo) {
        NRI_REPORT_ERROR(&m_Device, "Unsupported Vulkan video profile");
        return Result::UNSUPPORTED;
    }

    VkVideoCapabilitiesKHR capabilities = {VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR};
    VkVideoDecodeCapabilitiesKHR decodeCapabilities = {VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR};
    VkVideoDecodeH264CapabilitiesKHR decodeH264Capabilities = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_KHR};
    VkVideoDecodeH265CapabilitiesKHR decodeH265Capabilities = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_CAPABILITIES_KHR};
    VkVideoDecodeAV1CapabilitiesKHR decodeAV1Capabilities = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_CAPABILITIES_KHR};
    VkVideoEncodeCapabilitiesKHR encodeCapabilities = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_CAPABILITIES_KHR};
    VkVideoEncodeH264CapabilitiesKHR encodeH264Capabilities = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_CAPABILITIES_KHR};
    VkVideoEncodeH265CapabilitiesKHR encodeH265Capabilities = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_CAPABILITIES_KHR};
    VkVideoEncodeAV1CapabilitiesKHR encodeAV1Capabilities = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_CAPABILITIES_KHR};

    if (videoSessionDesc.usage == VideoUsage::DECODE) {
        capabilities.pNext = &decodeCapabilities;
        switch (videoSessionDesc.codec) {
        case VideoCodec::H264:
            decodeCapabilities.pNext = &decodeH264Capabilities;
            break;
        case VideoCodec::H265:
            decodeCapabilities.pNext = &decodeH265Capabilities;
            break;
        case VideoCodec::AV1:
            decodeCapabilities.pNext = &decodeAV1Capabilities;
            break;
        case VideoCodec::MAX_NUM:
            break;
        }
    } else {
        capabilities.pNext = &encodeCapabilities;
        switch (videoSessionDesc.codec) {
        case VideoCodec::H264:
            encodeCapabilities.pNext = &encodeH264Capabilities;
            break;
        case VideoCodec::H265:
            encodeCapabilities.pNext = &encodeH265Capabilities;
            break;
        case VideoCodec::AV1:
            encodeCapabilities.pNext = &encodeAV1Capabilities;
            break;
        case VideoCodec::MAX_NUM:
            break;
        }
    }

    VkResult vkResult = vk.GetPhysicalDeviceVideoCapabilitiesKHR(m_Device, &profile, &capabilities);
    if (vkResult != VK_SUCCESS) {
        NRI_REPORT_ERROR(&m_Device, "vkGetPhysicalDeviceVideoCapabilitiesKHR failed for operation 0x%X, format %u, result %d", operation, (uint32_t)videoSessionDesc.format, vkResult);
    }
    NRI_RETURN_ON_BAD_VKRESULT(&m_Device, vkResult, "vkGetPhysicalDeviceVideoCapabilitiesKHR");

    if (videoSessionDesc.width < capabilities.minCodedExtent.width || videoSessionDesc.height < capabilities.minCodedExtent.height || videoSessionDesc.width > capabilities.maxCodedExtent.width
        || videoSessionDesc.height > capabilities.maxCodedExtent.height) {
        NRI_REPORT_ERROR(&m_Device, "Vulkan video coded extent %ux%u is outside supported range %ux%u..%ux%u", videoSessionDesc.width, videoSessionDesc.height, capabilities.minCodedExtent.width,
            capabilities.minCodedExtent.height, capabilities.maxCodedExtent.width, capabilities.maxCodedExtent.height);
        return Result::UNSUPPORTED;
    }

    VkVideoSessionCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR};
    VkVideoEncodeH264SessionCreateInfoKHR encodeH264SessionCreateInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_CREATE_INFO_KHR};
    VkVideoEncodeH265SessionCreateInfoKHR encodeH265SessionCreateInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_CREATE_INFO_KHR};
    VkVideoEncodeAV1SessionCreateInfoKHR encodeAV1SessionCreateInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_SESSION_CREATE_INFO_KHR};
    const uint32_t maxActiveReferencePictures = std::min(videoSessionDesc.maxReferenceNum, capabilities.maxActiveReferencePictures);
    const uint32_t maxDpbSlots = videoSessionDesc.maxReferenceNum ? std::min(videoSessionDesc.maxReferenceNum + 1u, capabilities.maxDpbSlots) : 0;
    if (videoSessionDesc.usage == VideoUsage::ENCODE) {
        switch (videoSessionDesc.codec) {
        case VideoCodec::H264:
            encodeH264SessionCreateInfo.useMaxLevelIdc = true;
            encodeH264SessionCreateInfo.maxLevelIdc = STD_VIDEO_H264_LEVEL_IDC_4_2;
            createInfo.pNext = &encodeH264SessionCreateInfo;
            break;
        case VideoCodec::H265:
            encodeH265SessionCreateInfo.useMaxLevelIdc = true;
            encodeH265SessionCreateInfo.maxLevelIdc = GetVideoH265LevelIdcVK(videoSessionDesc.width, videoSessionDesc.height);
            createInfo.pNext = &encodeH265SessionCreateInfo;
            break;
        case VideoCodec::AV1:
            encodeAV1SessionCreateInfo.useMaxLevel = true;
            encodeAV1SessionCreateInfo.maxLevel = GetVideoAV1LevelVK(videoSessionDesc.width, videoSessionDesc.height);
            createInfo.pNext = &encodeAV1SessionCreateInfo;
            break;
        case VideoCodec::MAX_NUM:
            break;
        }
    }
    createInfo.queueFamilyIndex = ((QueueVK*)queue)->GetFamilyIndex();
    createInfo.pVideoProfile = &profile;
    createInfo.pictureFormat = GetVkFormat(videoSessionDesc.format);
    createInfo.maxCodedExtent = videoSessionDesc.usage == VideoUsage::DECODE ? capabilities.maxCodedExtent : VkExtent2D{videoSessionDesc.width, videoSessionDesc.height};
    createInfo.referencePictureFormat = maxDpbSlots ? createInfo.pictureFormat : VK_FORMAT_UNDEFINED;
    createInfo.maxDpbSlots = maxDpbSlots;
    createInfo.maxActiveReferencePictures = maxActiveReferencePictures;
    createInfo.pStdHeaderVersion = &capabilities.stdHeaderVersion;
#if defined(VK_VIDEO_SESSION_CREATE_INLINE_SESSION_PARAMETERS_BIT_KHR)
    if (videoSessionDesc.usage == VideoUsage::DECODE && videoSessionDesc.codec == VideoCodec::AV1 && m_Device.m_IsSupported.videoMaintenance2) {
        createInfo.flags |= VK_VIDEO_SESSION_CREATE_INLINE_SESSION_PARAMETERS_BIT_KHR;
        m_UseInlineSessionParameters = true;
    }
#endif
    vkResult = vk.CreateVideoSessionKHR(m_Device, &createInfo, m_Device.GetVkAllocationCallbacks(), &m_Handle);
    if (vkResult != VK_SUCCESS) {
        NRI_REPORT_ERROR(&m_Device, "vkCreateVideoSessionKHR failed for queue family %u, operation 0x%X, format %u, result %d", createInfo.queueFamilyIndex, operation, (uint32_t)videoSessionDesc.format, vkResult);
    }
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

static Result NRI_CALL CreateVideoSessionParameters(Device& device, const VideoSessionParametersDesc& videoSessionParametersDesc, VideoSessionParameters*& videoSessionParameters) {
    DeviceVK& deviceVK = (DeviceVK&)device;
    VideoSessionParametersVK* impl = Allocate<VideoSessionParametersVK>(deviceVK.GetAllocationCallbacks(), deviceVK);
    Result result = impl->Create(videoSessionParametersDesc);

    if (result != Result::SUCCESS) {
        Destroy(impl);
        videoSessionParameters = nullptr;
    } else
        videoSessionParameters = (VideoSessionParameters*)impl;

    return result;
}

static void NRI_CALL DestroyVideoSessionParameters(VideoSessionParameters& videoSessionParameters) {
    Destroy((VideoSessionParametersVK*)&videoSessionParameters);
}

static Result NRI_CALL CreateVideoPicture(Device& device, const VideoPictureDesc& videoPictureDesc, VideoPicture*& videoPicture) {
    DeviceVK& deviceVK = (DeviceVK&)device;
    VideoPictureVK* impl = Allocate<VideoPictureVK>(deviceVK.GetAllocationCallbacks(), deviceVK);
    Result result = impl->Create(videoPictureDesc);

    if (result != Result::SUCCESS) {
        Destroy(impl);
        videoPicture = nullptr;
    } else
        videoPicture = (VideoPicture*)impl;

    return result;
}

static void NRI_CALL DestroyVideoPicture(VideoPicture& videoPicture) {
    Destroy((VideoPictureVK*)&videoPicture);
}

static void NRI_CALL CmdDecodeVideo(CommandBuffer& commandBuffer, const VideoDecodeDesc& videoDecodeDesc) {
    CommandBufferVK& commandBufferVK = (CommandBufferVK&)commandBuffer;
    DeviceVK& device = commandBufferVK.GetDevice();
    const auto& vk = device.GetDispatchTable();

    if (!videoDecodeDesc.session || !videoDecodeDesc.parameters || !videoDecodeDesc.bitstream.buffer || !videoDecodeDesc.bitstream.size || !videoDecodeDesc.dstPicture) {
        NRI_REPORT_ERROR(&device, "'session', 'parameters', 'bitstream.buffer', 'bitstream.size' and 'dstPicture' must be valid");
        return;
    }

    if (!vk.CmdBeginVideoCodingKHR || !vk.CmdControlVideoCodingKHR || !vk.CmdDecodeVideoKHR || !vk.CmdEndVideoCodingKHR) {
        NRI_REPORT_ERROR(&device, "Vulkan video decode commands are not available");
        return;
    }

    if (videoDecodeDesc.referenceNum != 0 && !videoDecodeDesc.references) {
        NRI_REPORT_ERROR(&device, "'references' is NULL");
        return;
    }

    VideoSessionVK& session = *(VideoSessionVK*)videoDecodeDesc.session;
    VideoSessionParametersVK& parameters = *(VideoSessionParametersVK*)videoDecodeDesc.parameters;
    VideoPictureVK& dstPicture = *(VideoPictureVK*)videoDecodeDesc.dstPicture;
    VideoPictureVK& setupPicture = videoDecodeDesc.setupPicture ? *(VideoPictureVK*)videoDecodeDesc.setupPicture : dstPicture;
    if (parameters.m_Session != &session) {
        NRI_REPORT_ERROR(&device, "'parameters' must belong to 'session'");
        return;
    }

    BufferVK& bitstream = *(BufferVK*)videoDecodeDesc.bitstream.buffer;
    if (videoDecodeDesc.bitstream.offset >= bitstream.GetDesc().size || videoDecodeDesc.bitstream.size > bitstream.GetDesc().size - videoDecodeDesc.bitstream.offset) {
        NRI_REPORT_ERROR(&device, "'bitstream' range is outside of 'bitstream.buffer'");
        return;
    }

    const uint32_t referenceScratchNum = videoDecodeDesc.referenceNum ? videoDecodeDesc.referenceNum : 1;
    Scratch<VkVideoReferenceSlotInfoKHR> referenceSlots = NRI_ALLOCATE_SCRATCH(device, VkVideoReferenceSlotInfoKHR, videoDecodeDesc.referenceNum + 1);
    Scratch<StdVideoDecodeH264ReferenceInfo> h264StdReferences = NRI_ALLOCATE_SCRATCH(device, StdVideoDecodeH264ReferenceInfo, referenceScratchNum);
    Scratch<VkVideoDecodeH264DpbSlotInfoKHR> h264References = NRI_ALLOCATE_SCRATCH(device, VkVideoDecodeH264DpbSlotInfoKHR, referenceScratchNum);
    Scratch<StdVideoDecodeH265ReferenceInfo> h265StdReferences = NRI_ALLOCATE_SCRATCH(device, StdVideoDecodeH265ReferenceInfo, referenceScratchNum);
    Scratch<VkVideoDecodeH265DpbSlotInfoKHR> h265References = NRI_ALLOCATE_SCRATCH(device, VkVideoDecodeH265DpbSlotInfoKHR, referenceScratchNum);
    Scratch<StdVideoDecodeAV1ReferenceInfo> av1StdReferences = NRI_ALLOCATE_SCRATCH(device, StdVideoDecodeAV1ReferenceInfo, referenceScratchNum);
    Scratch<VkVideoDecodeAV1DpbSlotInfoKHR> av1References = NRI_ALLOCATE_SCRATCH(device, VkVideoDecodeAV1DpbSlotInfoKHR, referenceScratchNum);
    for (uint32_t i = 0; i < videoDecodeDesc.referenceNum; i++) {
        if (!videoDecodeDesc.references[i].picture) {
            NRI_REPORT_ERROR(&device, "'references[%u].picture' is NULL", i);
            return;
        }

        VideoPictureVK& picture = *(VideoPictureVK*)videoDecodeDesc.references[i].picture;
        referenceSlots[i] = {VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR};
        referenceSlots[i].slotIndex = videoDecodeDesc.references[i].slot;
        referenceSlots[i].pPictureResource = &picture.m_Resource;
        if (session.m_Desc.codec == VideoCodec::H264) {
            const VideoH264DecodePictureDesc* h264PictureDesc = videoDecodeDesc.h264PictureDesc;
            const VideoH264DecodeReferenceDesc* referenceDesc =
                h264PictureDesc ? FindVideoH264DecodeReferenceDescVK(h264PictureDesc->references, h264PictureDesc->referenceNum, videoDecodeDesc.references[i].slot) : nullptr;
            if (!referenceDesc) {
                NRI_REPORT_ERROR(&device, "'h264PictureDesc->references' must include metadata for each H.264 reference");
                return;
            }

            h264StdReferences[i] = {};
            h264StdReferences[i].flags.top_field_flag = !!(referenceDesc->flags & VideoH264DecodeReferenceBits::TOP_FIELD);
            h264StdReferences[i].flags.bottom_field_flag = !!(referenceDesc->flags & VideoH264DecodeReferenceBits::BOTTOM_FIELD);
            h264StdReferences[i].flags.used_for_long_term_reference = !!(referenceDesc->flags & VideoH264DecodeReferenceBits::LONG_TERM);
            h264StdReferences[i].flags.is_non_existing = !!(referenceDesc->flags & VideoH264DecodeReferenceBits::NON_EXISTING);
            h264StdReferences[i].FrameNum = (uint16_t)referenceDesc->frameNum;
            h264StdReferences[i].PicOrderCnt[0] = referenceDesc->topFieldOrderCount;
            h264StdReferences[i].PicOrderCnt[1] = referenceDesc->bottomFieldOrderCount;
            h264References[i] = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_KHR};
            h264References[i].pStdReferenceInfo = &h264StdReferences[i];
            referenceSlots[i].pNext = &h264References[i];
        } else if (session.m_Desc.codec == VideoCodec::H265) {
            const VideoH265DecodePictureDesc* h265PictureDesc = videoDecodeDesc.h265PictureDesc;
            const VideoH265ReferenceDesc* referenceDesc = h265PictureDesc ? FindVideoH265ReferenceDescVK(h265PictureDesc->references, h265PictureDesc->referenceNum, videoDecodeDesc.references[i].slot) : nullptr;
            h265StdReferences[i] = {};
            h265StdReferences[i].flags.used_for_long_term_reference = referenceDesc && referenceDesc->longTerm;
            h265StdReferences[i].PicOrderCntVal = referenceDesc ? referenceDesc->pictureOrderCount : (int32_t)videoDecodeDesc.references[i].slot;
            h265References[i] = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_DPB_SLOT_INFO_KHR};
            h265References[i].pStdReferenceInfo = &h265StdReferences[i];
            referenceSlots[i].pNext = &h265References[i];
        } else if (session.m_Desc.codec == VideoCodec::AV1) {
            const VideoAV1DecodePictureDesc* av1PictureDesc = videoDecodeDesc.av1PictureDesc;
            const VideoAV1ReferenceDesc* referenceDesc = av1PictureDesc ? FindVideoAV1ReferenceDescVK(av1PictureDesc->references, av1PictureDesc->referenceNum, videoDecodeDesc.references[i].slot) : nullptr;
            if (!referenceDesc) {
                NRI_REPORT_ERROR(&device, "'av1PictureDesc->references' must include metadata for each AV1 reference");
                return;
            }

            FillVideoDecodeAV1ReferenceInfoVK(av1StdReferences[i], referenceDesc->frameType, referenceDesc->orderHint, referenceDesc->savedOrderHints);
            av1References[i] = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_DPB_SLOT_INFO_KHR};
            av1References[i].pStdReferenceInfo = &av1StdReferences[i];
            referenceSlots[i].pNext = &av1References[i];
        }
    }

    VkVideoDecodeH264PictureInfoKHR h264Picture = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PICTURE_INFO_KHR};
    StdVideoDecodeH264PictureInfo h264StdPicture = {};
    VkVideoDecodeH264DpbSlotInfoKHR h264DpbSlot = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_KHR};
    StdVideoDecodeH264ReferenceInfo h264StdReference = {};
    VkVideoDecodeH265PictureInfoKHR h265Picture = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PICTURE_INFO_KHR};
    StdVideoDecodeH265PictureInfo h265StdPicture = {};
    VkVideoDecodeH265DpbSlotInfoKHR h265DpbSlot = {VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_DPB_SLOT_INFO_KHR};
    StdVideoDecodeH265ReferenceInfo h265StdReference = {};
    VkVideoDecodeAV1PictureInfoKHR av1Picture = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_PICTURE_INFO_KHR};
    StdVideoDecodeAV1PictureInfo av1StdPicture = {};
    VkVideoDecodeAV1DpbSlotInfoKHR av1DpbSlot = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_DPB_SLOT_INFO_KHR};
    StdVideoDecodeAV1ReferenceInfo av1StdReference = {};
    StdVideoAV1TileInfo av1TileInfo = {};
    StdVideoAV1Quantization av1Quantization = {};
    StdVideoAV1LoopFilter av1LoopFilter = {};
    StdVideoAV1LoopRestoration av1LoopRestoration = {};
    StdVideoAV1Segmentation av1Segmentation = {};
    StdVideoAV1CDEF av1Cdef = {};
    StdVideoAV1GlobalMotion av1GlobalMotion = {};
    StdVideoAV1FilmGrain av1FilmGrain = {};
#if defined(VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_INLINE_SESSION_PARAMETERS_INFO_KHR)
    VkVideoDecodeAV1InlineSessionParametersInfoKHR av1InlineSessionParameters = {VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_INLINE_SESSION_PARAMETERS_INFO_KHR};
#endif
    Scratch<uint32_t> av1TileOffsets = NRI_ALLOCATE_SCRATCH(device, uint32_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum, 1u) : 1u);
    Scratch<uint32_t> av1TileSizes = NRI_ALLOCATE_SCRATCH(device, uint32_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum, 1u) : 1u);
    Scratch<uint16_t> av1MiColStarts = NRI_ALLOCATE_SCRATCH(device, uint16_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum + 1, 2u) : 2u);
    Scratch<uint16_t> av1MiRowStarts = NRI_ALLOCATE_SCRATCH(device, uint16_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum + 1, 2u) : 2u);
    Scratch<uint16_t> av1WidthInSbsMinus1 = NRI_ALLOCATE_SCRATCH(device, uint16_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum, 1u) : 1u);
    Scratch<uint16_t> av1HeightInSbsMinus1 = NRI_ALLOCATE_SCRATCH(device, uint16_t, videoDecodeDesc.av1PictureDesc ? std::max(videoDecodeDesc.av1PictureDesc->tileNum, 1u) : 1u);
    void* codecPictureInfo = nullptr;
    const void* setupReferenceInfo = nullptr;
    if (session.m_Desc.codec == VideoCodec::H264) {
        if (!videoDecodeDesc.h264PictureDesc) {
            NRI_REPORT_ERROR(&device, "'h264PictureDesc' must be valid for H.264 decode sessions");
            return;
        }

        const VideoH264DecodePictureDesc& desc = *videoDecodeDesc.h264PictureDesc;
        h264StdPicture.flags.field_pic_flag = !!(desc.flags & VideoH264DecodePictureBits::FIELD_PICTURE);
        h264StdPicture.flags.is_intra = !!(desc.flags & VideoH264DecodePictureBits::INTRA);
        h264StdPicture.flags.IdrPicFlag = !!(desc.flags & VideoH264DecodePictureBits::IDR);
        h264StdPicture.flags.bottom_field_flag = !!(desc.flags & VideoH264DecodePictureBits::BOTTOM_FIELD);
        h264StdPicture.flags.is_reference = !!(desc.flags & VideoH264DecodePictureBits::REFERENCE);
        h264StdPicture.flags.complementary_field_pair = !!(desc.flags & VideoH264DecodePictureBits::COMPLEMENTARY_FIELD_PAIR);
        h264StdPicture.seq_parameter_set_id = desc.sequenceParameterSetId;
        h264StdPicture.pic_parameter_set_id = desc.pictureParameterSetId;
        h264StdPicture.frame_num = desc.frameNum;
        h264StdPicture.idr_pic_id = desc.idrPictureId;
        h264StdPicture.PicOrderCnt[0] = desc.topFieldOrderCount;
        h264StdPicture.PicOrderCnt[1] = desc.bottomFieldOrderCount;
        h264Picture.pStdPictureInfo = &h264StdPicture;
        h264Picture.sliceCount = desc.sliceOffsetNum;
        h264Picture.pSliceOffsets = desc.sliceOffsets;
        codecPictureInfo = &h264Picture;

        if (desc.flags & VideoH264DecodePictureBits::REFERENCE) {
            h264StdReference.FrameNum = desc.frameNum;
            h264StdReference.PicOrderCnt[0] = desc.topFieldOrderCount;
            h264StdReference.PicOrderCnt[1] = desc.bottomFieldOrderCount;
            h264DpbSlot.pStdReferenceInfo = &h264StdReference;
            setupReferenceInfo = &h264DpbSlot;
        }
    } else if (session.m_Desc.codec == VideoCodec::H265) {
        if (!videoDecodeDesc.h265PictureDesc) {
            NRI_REPORT_ERROR(&device, "'h265PictureDesc' must be valid for H.265 decode sessions");
            return;
        }

        const VideoH265DecodePictureDesc& desc = *videoDecodeDesc.h265PictureDesc;
        if (desc.referenceNum != 0 && !desc.references) {
            NRI_REPORT_ERROR(&device, "'h265PictureDesc->references' is NULL");
            return;
        }
        if (desc.referenceNum > STD_VIDEO_DECODE_H265_REF_PIC_SET_LIST_SIZE) {
            NRI_REPORT_ERROR(&device, "'h265PictureDesc->referenceNum' exceeds the H.265 reference picture set list size");
            return;
        }

        h265StdPicture.flags.IrapPicFlag = !!(desc.flags & VideoH265DecodePictureBits::IRAP);
        h265StdPicture.flags.IdrPicFlag = !!(desc.flags & VideoH265DecodePictureBits::IDR);
        h265StdPicture.flags.IsReference = !!(desc.flags & VideoH265DecodePictureBits::REFERENCE);
        h265StdPicture.flags.short_term_ref_pic_set_sps_flag = !!(desc.flags & VideoH265DecodePictureBits::SHORT_TERM_REF_PIC_SET_SPS);
        h265StdPicture.sps_video_parameter_set_id = desc.videoParameterSetId;
        h265StdPicture.pps_seq_parameter_set_id = desc.sequenceParameterSetId;
        h265StdPicture.pps_pic_parameter_set_id = desc.pictureParameterSetId;
        h265StdPicture.NumDeltaPocsOfRefRpsIdx = desc.numDeltaPocsOfRefRpsIdx;
        h265StdPicture.PicOrderCntVal = desc.pictureOrderCount;
        h265StdPicture.NumBitsForSTRefPicSetInSlice = desc.numBitsForShortTermRefPicSetInSlice;
        for (uint8_t& entry : h265StdPicture.RefPicSetStCurrBefore)
            entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
        for (uint8_t& entry : h265StdPicture.RefPicSetStCurrAfter)
            entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
        for (uint8_t& entry : h265StdPicture.RefPicSetLtCurr)
            entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;

        uint32_t beforeNum = 0;
        uint32_t afterNum = 0;
        uint32_t longTermNum = 0;
        for (uint32_t i = 0; i < desc.referenceNum; i++) {
            const VideoH265ReferenceDesc& reference = desc.references[i];
            const uint8_t slot = (uint8_t)reference.slot;
            if (reference.longTerm)
                h265StdPicture.RefPicSetLtCurr[longTermNum++] = slot;
            else if (reference.pictureOrderCount <= desc.pictureOrderCount)
                h265StdPicture.RefPicSetStCurrBefore[beforeNum++] = slot;
            else
                h265StdPicture.RefPicSetStCurrAfter[afterNum++] = slot;
        }

        h265Picture.pStdPictureInfo = &h265StdPicture;
        h265Picture.sliceSegmentCount = desc.sliceSegmentOffsetNum;
        h265Picture.pSliceSegmentOffsets = desc.sliceSegmentOffsets;
        codecPictureInfo = &h265Picture;

        if (desc.flags & VideoH265DecodePictureBits::REFERENCE) {
            h265StdReference.PicOrderCntVal = desc.pictureOrderCount;
            h265DpbSlot.pStdReferenceInfo = &h265StdReference;
            setupReferenceInfo = &h265DpbSlot;
        }
    } else if (session.m_Desc.codec == VideoCodec::AV1) {
        if (!videoDecodeDesc.av1PictureDesc) {
            NRI_REPORT_ERROR(&device, "'av1PictureDesc' must be valid for AV1 decode sessions");
            return;
        }

        const VideoAV1DecodePictureDesc& desc = *videoDecodeDesc.av1PictureDesc;
        if ((desc.tileNum != 0 && !desc.tiles) || desc.referenceNum > 8 || (desc.referenceNum != 0 && !desc.references)) {
            NRI_REPORT_ERROR(&device, "'av1PictureDesc' contains invalid tile or reference data");
            return;
        }
        if (desc.tileLayout && (!desc.tileLayout->columnNum || !desc.tileLayout->rowNum ||
                !desc.tileLayout->miColumnStarts || !desc.tileLayout->miRowStarts || !desc.tileLayout->widthInSuperblocksMinus1 || !desc.tileLayout->heightInSuperblocksMinus1)) {
            NRI_REPORT_ERROR(&device, "'av1PictureDesc->tileLayout' is invalid");
            return;
        }

        for (int32_t& slotIndex : av1Picture.referenceNameSlotIndices)
            slotIndex = -1;

        const VideoAV1PictureBits pictureFlags = desc.flags == VideoAV1PictureBits::NONE ? GetDefaultVideoAV1PictureFlagsVK() : desc.flags;
        VideoDecodeAV1ReferenceMappingVK referenceMapping = {};
        if (!BuildVideoDecodeAV1ReferenceMappingVK(desc, referenceMapping)) {
            if (referenceMapping.invalidName)
                NRI_REPORT_ERROR(&device, "'av1PictureDesc->references[%u].name' or 'av1PictureDesc->primaryReferenceName' is invalid", referenceMapping.failingReference);
            else if (referenceMapping.invalidRefFrameIndex)
                NRI_REPORT_ERROR(&device, "'av1PictureDesc->references[%u].refFrameIndex' is invalid", referenceMapping.failingReference);
            else if (referenceMapping.missingPrimaryReference)
                NRI_REPORT_ERROR(&device, "'av1PictureDesc->primaryReferenceName' does not name an active reference");
            else
                NRI_REPORT_ERROR(&device, "'av1PictureDesc->referenceNum' exceeds AV1 DPB slot count");
            return;
        }

        FillVideoDecodeAV1PictureInfoVK(av1StdPicture, desc, pictureFlags);
        for (uint32_t i = 0; i < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR; i++)
            av1Picture.referenceNameSlotIndices[i] = referenceMapping.referenceNameSlotIndices[i];

        FillVideoAV1DefaultTileInfoVK(av1TileInfo, av1MiColStarts, av1MiRowStarts, av1WidthInSbsMinus1, av1HeightInSbsMinus1,
            session.m_Desc.width, session.m_Desc.height);
        if (desc.tileLayout) {
            av1TileInfo.flags.uniform_tile_spacing_flag = desc.tileLayout->uniformSpacing != 0;
            av1TileInfo.TileCols = desc.tileLayout->columnNum;
            av1TileInfo.TileRows = desc.tileLayout->rowNum;
            av1TileInfo.context_update_tile_id = desc.tileLayout->contextUpdateTileId;
            av1TileInfo.tile_size_bytes_minus_1 = desc.tileLayout->tileSizeBytesMinus1;
            av1TileInfo.pMiColStarts = desc.tileLayout->miColumnStarts;
            av1TileInfo.pMiRowStarts = desc.tileLayout->miRowStarts;
            av1TileInfo.pWidthInSbsMinus1 = desc.tileLayout->widthInSuperblocksMinus1;
            av1TileInfo.pHeightInSbsMinus1 = desc.tileLayout->heightInSuperblocksMinus1;
        }
        FillVideoDecodeAV1QuantizationVK(av1Quantization, desc);
        FillVideoDecodeAV1LoopFilterVK(av1LoopFilter, desc);
        FillVideoDecodeAV1CdefVK(av1Cdef, desc);
        if (desc.segmentation) {
            std::memcpy(av1Segmentation.FeatureEnabled, desc.segmentation->featureEnabled, sizeof(av1Segmentation.FeatureEnabled));
            std::memcpy(av1Segmentation.FeatureData, desc.segmentation->featureData, sizeof(av1Segmentation.FeatureData));
        }
        FillVideoDecodeAV1LoopRestorationVK(av1LoopRestoration, desc);
        FillVideoDecodeAV1GlobalMotionVK(av1GlobalMotion, desc);
        if (desc.filmGrain)
            FillVideoDecodeAV1FilmGrainVK(av1FilmGrain, *desc.filmGrain);
        av1StdPicture.pTileInfo = &av1TileInfo;
        av1StdPicture.pQuantization = &av1Quantization;
        av1StdPicture.pSegmentation = &av1Segmentation;
        av1StdPicture.pLoopFilter = &av1LoopFilter;
        av1StdPicture.pCDEF = &av1Cdef;
        av1StdPicture.pLoopRestoration = &av1LoopRestoration;
        av1StdPicture.pGlobalMotion = &av1GlobalMotion;
        av1StdPicture.pFilmGrain = (pictureFlags & VideoAV1PictureBits::APPLY_GRAIN) && desc.filmGrain ? &av1FilmGrain : nullptr;

        av1Picture.pStdPictureInfo = &av1StdPicture;
#if defined(VK_STRUCTURE_TYPE_VIDEO_DECODE_AV1_INLINE_SESSION_PARAMETERS_INFO_KHR)
        if (session.m_UseInlineSessionParameters) {
            av1InlineSessionParameters.pStdSequenceHeader = &parameters.m_AV1SequenceHeader;
            av1Picture.pNext = &av1InlineSessionParameters;
        }
#endif
        FillVideoDecodeAV1TilePayloadVK(av1Picture, desc, av1TileOffsets, av1TileSizes);
        codecPictureInfo = &av1Picture;

        if (desc.refreshFrameFlags) {
            FillVideoDecodeAV1SetupReferenceInfoVK(av1StdReference, desc, pictureFlags);
            av1DpbSlot.pStdReferenceInfo = &av1StdReference;
            setupReferenceInfo = &av1DpbSlot;
        }
    }

    VkVideoReferenceSlotInfoKHR setupReferenceSlot = {VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR};
    setupReferenceSlot.pNext = setupReferenceInfo;
    uint32_t setupReferenceSlotIndex = videoDecodeDesc.dstSlot;
    if (session.m_Desc.codec == VideoCodec::H264 && videoDecodeDesc.h264PictureDesc && videoDecodeDesc.h264PictureDesc->referenceSlot)
        setupReferenceSlotIndex = videoDecodeDesc.h264PictureDesc->referenceSlot;
    setupReferenceSlot.slotIndex = setupReferenceInfo ? (int32_t)setupReferenceSlotIndex : -1;
    setupReferenceSlot.pPictureResource = &setupPicture.m_Resource;

    VkVideoBeginCodingInfoKHR beginInfo = {VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR};
    const VkVideoSessionParametersKHR sessionParameters = session.m_UseInlineSessionParameters ? VK_NULL_HANDLE : parameters.m_Handle;
    beginInfo.videoSession = session.m_Handle;
    beginInfo.videoSessionParameters = sessionParameters;
    beginInfo.referenceSlotCount = videoDecodeDesc.referenceNum;
    beginInfo.pReferenceSlots = referenceSlots;
    if (setupReferenceInfo) {
        referenceSlots[beginInfo.referenceSlotCount] = setupReferenceSlot;
        referenceSlots[beginInfo.referenceSlotCount].slotIndex = -1;
        beginInfo.referenceSlotCount++;
    }

    VkVideoDecodeInfoKHR decodeInfo = {VK_STRUCTURE_TYPE_VIDEO_DECODE_INFO_KHR};
    decodeInfo.pNext = codecPictureInfo;
    decodeInfo.srcBuffer = bitstream.GetHandle();
    decodeInfo.srcBufferOffset = videoDecodeDesc.bitstream.offset;
    decodeInfo.srcBufferRange = videoDecodeDesc.bitstream.size;
    decodeInfo.dstPictureResource = dstPicture.m_Resource;
    decodeInfo.pSetupReferenceSlot = setupReferenceInfo ? &setupReferenceSlot : nullptr;
    decodeInfo.referenceSlotCount = videoDecodeDesc.referenceNum;
    decodeInfo.pReferenceSlots = referenceSlots;

    VkVideoEndCodingInfoKHR endInfo = {VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR};
    if (!session.m_Initialized) {
        VkVideoBeginCodingInfoKHR resetBeginInfo = {VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR};
        resetBeginInfo.videoSession = session.m_Handle;
        resetBeginInfo.videoSessionParameters = sessionParameters;
        VkVideoCodingControlInfoKHR controlInfo = {VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR};
        controlInfo.flags = VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR;
        vk.CmdBeginVideoCodingKHR(commandBufferVK, &resetBeginInfo);
        vk.CmdControlVideoCodingKHR(commandBufferVK, &controlInfo);
        vk.CmdEndVideoCodingKHR(commandBufferVK, &endInfo);
        session.m_Initialized = true;
    }
    vk.CmdBeginVideoCodingKHR(commandBufferVK, &beginInfo);
    vk.CmdDecodeVideoKHR(commandBufferVK, &decodeInfo);
    vk.CmdEndVideoCodingKHR(commandBufferVK, &endInfo);
}

static StdVideoH264PictureType GetVideoEncodeH264PictureTypeVK(VideoEncodeFrameType frameType) {
    switch (frameType) {
    case VideoEncodeFrameType::IDR:
        return STD_VIDEO_H264_PICTURE_TYPE_IDR;
    case VideoEncodeFrameType::I:
        return STD_VIDEO_H264_PICTURE_TYPE_I;
    case VideoEncodeFrameType::P:
        return STD_VIDEO_H264_PICTURE_TYPE_P;
    case VideoEncodeFrameType::B:
        return STD_VIDEO_H264_PICTURE_TYPE_B;
    case VideoEncodeFrameType::MAX_NUM:
        return STD_VIDEO_H264_PICTURE_TYPE_INVALID;
    }

    return STD_VIDEO_H264_PICTURE_TYPE_INVALID;
}

static StdVideoH265PictureType GetVideoEncodeH265PictureTypeVK(VideoEncodeFrameType frameType) {
    switch (frameType) {
    case VideoEncodeFrameType::IDR:
        return STD_VIDEO_H265_PICTURE_TYPE_IDR;
    case VideoEncodeFrameType::I:
        return STD_VIDEO_H265_PICTURE_TYPE_I;
    case VideoEncodeFrameType::P:
        return STD_VIDEO_H265_PICTURE_TYPE_P;
    case VideoEncodeFrameType::B:
        return STD_VIDEO_H265_PICTURE_TYPE_B;
    case VideoEncodeFrameType::MAX_NUM:
        return STD_VIDEO_H265_PICTURE_TYPE_INVALID;
    }

    return STD_VIDEO_H265_PICTURE_TYPE_INVALID;
}

static StdVideoAV1FrameType GetVideoEncodeAV1FrameTypeVK(VideoEncodeFrameType frameType) {
    switch (frameType) {
    case VideoEncodeFrameType::IDR:
    case VideoEncodeFrameType::I:
        return STD_VIDEO_AV1_FRAME_TYPE_KEY;
    case VideoEncodeFrameType::P:
    case VideoEncodeFrameType::B:
        return STD_VIDEO_AV1_FRAME_TYPE_INTER;
    case VideoEncodeFrameType::MAX_NUM:
        return STD_VIDEO_AV1_FRAME_TYPE_INVALID;
    }

    return STD_VIDEO_AV1_FRAME_TYPE_INVALID;
}

static bool HasVideoEncodeReferenceSlot(const VideoEncodeDesc& videoEncodeDesc, uint32_t slot) {
    for (uint32_t i = 0; i < videoEncodeDesc.referenceNum; i++) {
        if (videoEncodeDesc.references[i].slot == slot)
            return true;
    }

    return false;
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

static const VideoAV1ReferenceDesc* FindVideoEncodeAV1ReferenceDesc(const VideoAV1PictureDesc* av1PictureDesc, uint32_t slot) {
    if (!av1PictureDesc)
        return nullptr;

    for (uint32_t i = 0; i < av1PictureDesc->referenceNum; i++) {
        if (av1PictureDesc->references[i].slot == slot)
            return &av1PictureDesc->references[i];
    }

    return nullptr;
}

static void NRI_CALL CmdEncodeVideo(CommandBuffer& commandBuffer, const VideoEncodeDesc& videoEncodeDesc) {
    CommandBufferVK& commandBufferVK = (CommandBufferVK&)commandBuffer;
    DeviceVK& device = commandBufferVK.GetDevice();
    const auto& vk = device.GetDispatchTable();

    if (!videoEncodeDesc.session || !videoEncodeDesc.parameters || !videoEncodeDesc.srcPicture || !videoEncodeDesc.dstBitstream.buffer || !videoEncodeDesc.dstBitstream.size) {
        NRI_REPORT_ERROR(&device, "'session', 'parameters', 'srcPicture', 'dstBitstream.buffer' and 'dstBitstream.size' must be valid");
        return;
    }

    if (!vk.CmdBeginVideoCodingKHR || !vk.CmdControlVideoCodingKHR || !vk.CmdEncodeVideoKHR || !vk.CmdEndVideoCodingKHR) {
        NRI_REPORT_ERROR(&device, "Vulkan video encode commands are not available");
        return;
    }

    if (videoEncodeDesc.referenceNum != 0 && !videoEncodeDesc.references) {
        NRI_REPORT_ERROR(&device, "'references' is NULL");
        return;
    }

    VideoSessionVK& session = *(VideoSessionVK*)videoEncodeDesc.session;
    VideoSessionParametersVK& parameters = *(VideoSessionParametersVK*)videoEncodeDesc.parameters;
    VideoPictureVK& srcPicture = *(VideoPictureVK*)videoEncodeDesc.srcPicture;
    if (parameters.m_Session != &session) {
        NRI_REPORT_ERROR(&device, "'parameters' must belong to 'session'");
        return;
    }
    if (videoEncodeDesc.av1PictureDesc && session.m_Desc.codec != VideoCodec::AV1) {
        NRI_REPORT_ERROR(&device, "'av1PictureDesc' can only be used with AV1 sessions");
        return;
    }
    if (videoEncodeDesc.h264PictureDesc && session.m_Desc.codec != VideoCodec::H264) {
        NRI_REPORT_ERROR(&device, "'h264PictureDesc' can only be used with H.264 sessions");
        return;
    }
    if (videoEncodeDesc.h264PictureDesc && videoEncodeDesc.h264PictureDesc->referenceNum != 0 && !videoEncodeDesc.h264PictureDesc->references) {
        NRI_REPORT_ERROR(&device, "'h264PictureDesc->references' is NULL");
        return;
    }
    if (videoEncodeDesc.av1PictureDesc && videoEncodeDesc.av1PictureDesc->referenceNum != 0 && !videoEncodeDesc.av1PictureDesc->references) {
        NRI_REPORT_ERROR(&device, "'av1PictureDesc->references' is NULL");
        return;
    }
    if (videoEncodeDesc.h265ReferenceDescs && session.m_Desc.codec != VideoCodec::H265) {
        NRI_REPORT_ERROR(&device, "'h265ReferenceDescs' can only be used with H.265 sessions");
        return;
    }
    if (session.m_Desc.maxReferenceNum != 0 && videoEncodeDesc.reconstructedSlot > session.m_Desc.maxReferenceNum) {
        NRI_REPORT_ERROR(&device, "'reconstructedSlot' exceeds the session DPB slot count");
        return;
    }

    const VideoEncodePictureDesc defaultPicture = {VideoEncodeFrameType::IDR, 0, 0, 0, 0};
    const VideoEncodePictureDesc& pictureDesc = videoEncodeDesc.pictureDesc ? *videoEncodeDesc.pictureDesc : defaultPicture;
    const VideoEncodeRateControlDesc defaultRateControl = {VideoEncodeRateControlMode::CQP, 26, 28, 30, 30, 1};
    const VideoEncodeRateControlDesc& rateControlDesc = videoEncodeDesc.rateControlDesc ? *videoEncodeDesc.rateControlDesc : defaultRateControl;

    VkVideoEncodeH264PictureInfoKHR h264Picture = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PICTURE_INFO_KHR};
    StdVideoEncodeH264PictureInfo h264StdPicture = {};
    StdVideoEncodeH264SliceHeader h264SliceHeader = {};
    VkVideoEncodeH264NaluSliceInfoKHR h264SliceInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_NALU_SLICE_INFO_KHR};
    StdVideoEncodeH264ReferenceInfo h264StdSetupReference = {};
    VkVideoEncodeH264DpbSlotInfoKHR h264SetupReference = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_DPB_SLOT_INFO_KHR};
    StdVideoEncodeH264ReferenceListsInfo h264ReferenceLists = {};

    VkVideoEncodeH265PictureInfoKHR h265Picture = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PICTURE_INFO_KHR};
    StdVideoEncodeH265PictureInfo h265StdPicture = {};
    StdVideoEncodeH265SliceSegmentHeader h265SliceHeader = {};
    VkVideoEncodeH265NaluSliceSegmentInfoKHR h265SliceInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_NALU_SLICE_SEGMENT_INFO_KHR};
    StdVideoEncodeH265ReferenceInfo h265StdSetupReference = {};
    VkVideoEncodeH265DpbSlotInfoKHR h265SetupReference = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_DPB_SLOT_INFO_KHR};
    VkVideoEncodeH265GopRemainingFrameInfoKHR h265GopRemaining = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_GOP_REMAINING_FRAME_INFO_KHR};
    StdVideoEncodeH265ReferenceListsInfo h265ReferenceLists = {};
    StdVideoH265ShortTermRefPicSet h265ShortTermRefPicSet = {};

    VkVideoEncodeAV1PictureInfoKHR av1Picture = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_PICTURE_INFO_KHR};
    StdVideoEncodeAV1PictureInfo av1StdPicture = {};
    VkVideoEncodeAV1GopRemainingFrameInfoKHR av1GopRemaining = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_GOP_REMAINING_FRAME_INFO_KHR};
    StdVideoAV1TileInfo av1TileInfo = {};
    StdVideoAV1Quantization av1Quantization = {};
    StdVideoAV1LoopFilter av1LoopFilter = {};
    StdVideoAV1CDEF av1Cdef = {};
    StdVideoAV1GlobalMotion av1GlobalMotion = {};
    StdVideoEncodeAV1ReferenceInfo av1StdSetupReference = {};
    VkVideoEncodeAV1DpbSlotInfoKHR av1SetupReference = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_DPB_SLOT_INFO_KHR};
    const VideoAV1TileLayoutDesc* encodeAv1TileLayout = videoEncodeDesc.av1PictureDesc ? videoEncodeDesc.av1PictureDesc->tileLayout : nullptr;
    if (encodeAv1TileLayout && (!encodeAv1TileLayout->columnNum || !encodeAv1TileLayout->rowNum ||
            !encodeAv1TileLayout->miColumnStarts || !encodeAv1TileLayout->miRowStarts || !encodeAv1TileLayout->widthInSuperblocksMinus1 || !encodeAv1TileLayout->heightInSuperblocksMinus1)) {
        NRI_REPORT_ERROR(&device, "'av1PictureDesc->tileLayout' is invalid");
        return;
    }
    std::array<uint16_t, 2> av1MiColStarts = {};
    std::array<uint16_t, 2> av1MiRowStarts = {};
    std::array<uint16_t, 1> av1WidthInSbsMinus1 = {};
    std::array<uint16_t, 1> av1HeightInSbsMinus1 = {};

    const void* codecPictureInfo = nullptr;
    bool isUsedAsReferencePicture = false;
    switch (session.m_Desc.codec) {
    case VideoCodec::H264: {
        for (uint8_t& ref : h264ReferenceLists.RefPicList0)
            ref = STD_VIDEO_H264_NO_REFERENCE_PICTURE;
        for (uint8_t& ref : h264ReferenceLists.RefPicList1)
            ref = STD_VIDEO_H264_NO_REFERENCE_PICTURE;

        if (videoEncodeDesc.referenceNum) {
            const VideoH264PictureDesc* h264PictureDesc = videoEncodeDesc.h264PictureDesc;
            if (!h264PictureDesc) {
                NRI_REPORT_ERROR(&device, "'h264PictureDesc' must be valid when H.264 encode uses references");
                return;
            }
            if (h264PictureDesc->referenceNum != videoEncodeDesc.referenceNum) {
                NRI_REPORT_ERROR(&device, "'h264PictureDesc->referenceNum' must match 'referenceNum'");
                return;
            }

            uint8_t list0Num = 0;
            uint8_t list1Num = 0;
            for (uint32_t i = 0; i < h264PictureDesc->referenceNum; i++) {
                const VideoH264ReferenceDesc& reference = h264PictureDesc->references[i];
                if (!HasVideoEncodeReferenceSlot(videoEncodeDesc, reference.slot)) {
                    NRI_REPORT_ERROR(&device, "'h264PictureDesc->references[%u].slot' is not present in 'references'", i);
                    return;
                }
                if (reference.slot > UINT8_MAX) {
                    NRI_REPORT_ERROR(&device, "'h264PictureDesc->references[%u].slot' exceeds the H.264 reference list slot range", i);
                    return;
                }

                if (reference.listIndex == 0) {
                    if (list0Num >= STD_VIDEO_H264_MAX_NUM_LIST_REF) {
                        NRI_REPORT_ERROR(&device, "H.264 List0 reference count exceeds STD_VIDEO_H264_MAX_NUM_LIST_REF");
                        return;
                    }
                    h264ReferenceLists.RefPicList0[list0Num++] = (uint8_t)reference.slot;
                } else if (reference.listIndex == 1) {
                    if (list1Num >= STD_VIDEO_H264_MAX_NUM_LIST_REF) {
                        NRI_REPORT_ERROR(&device, "H.264 List1 reference count exceeds STD_VIDEO_H264_MAX_NUM_LIST_REF");
                        return;
                    }
                    h264ReferenceLists.RefPicList1[list1Num++] = (uint8_t)reference.slot;
                } else {
                    NRI_REPORT_ERROR(&device, "'h264PictureDesc->references[%u].listIndex' must be 0 or 1", i);
                    return;
                }
            }
            h264ReferenceLists.num_ref_idx_l0_active_minus1 = list0Num ? list0Num - 1 : 0;
            h264ReferenceLists.num_ref_idx_l1_active_minus1 = list1Num ? list1Num - 1 : 0;
            h264StdPicture.pRefLists = &h264ReferenceLists;
        }

        h264StdPicture.flags.IdrPicFlag = pictureDesc.frameType == VideoEncodeFrameType::IDR;
        isUsedAsReferencePicture = IsVideoEncodePictureUsedAsReferenceVK(session.m_Desc.codec, session.m_Desc.maxReferenceNum,
            videoEncodeDesc.reconstructedPicture != nullptr, 0);
        h264StdPicture.flags.is_reference = isUsedAsReferencePicture;
        h264StdPicture.flags.no_output_of_prior_pics_flag = pictureDesc.frameType == VideoEncodeFrameType::IDR;
        h264StdPicture.seq_parameter_set_id = videoEncodeDesc.h264PictureDesc ? videoEncodeDesc.h264PictureDesc->sequenceParameterSetId : 0;
        h264StdPicture.pic_parameter_set_id = videoEncodeDesc.h264PictureDesc ? videoEncodeDesc.h264PictureDesc->pictureParameterSetId : 0;
        h264StdPicture.idr_pic_id = pictureDesc.idrPictureId;
        h264StdPicture.primary_pic_type = GetVideoEncodeH264PictureTypeVK(pictureDesc.frameType);
        h264StdPicture.frame_num = pictureDesc.frameIndex;
        h264StdPicture.PicOrderCnt = pictureDesc.pictureOrderCount;
        h264StdPicture.temporal_id = pictureDesc.temporalLayer;
        h264SliceHeader.slice_type = pictureDesc.frameType == VideoEncodeFrameType::B ? STD_VIDEO_H264_SLICE_TYPE_B : (pictureDesc.frameType == VideoEncodeFrameType::P ? STD_VIDEO_H264_SLICE_TYPE_P : STD_VIDEO_H264_SLICE_TYPE_I);
        h264SliceHeader.disable_deblocking_filter_idc = STD_VIDEO_H264_DISABLE_DEBLOCKING_FILTER_IDC_DISABLED;
        h264SliceInfo.constantQp = pictureDesc.frameType == VideoEncodeFrameType::B ? rateControlDesc.qpB : (pictureDesc.frameType == VideoEncodeFrameType::P ? rateControlDesc.qpP : rateControlDesc.qpI);
        h264SliceInfo.pStdSliceHeader = &h264SliceHeader;
        h264Picture.naluSliceEntryCount = 1;
        h264Picture.pNaluSliceEntries = &h264SliceInfo;
        h264Picture.pStdPictureInfo = &h264StdPicture;
        h264Picture.generatePrefixNalu = true;
        codecPictureInfo = &h264Picture;

        h264StdSetupReference.primary_pic_type = h264StdPicture.primary_pic_type;
        h264StdSetupReference.FrameNum = h264StdPicture.frame_num;
        h264StdSetupReference.PicOrderCnt = h264StdPicture.PicOrderCnt;
        h264StdSetupReference.temporal_id = h264StdPicture.temporal_id;
        h264SetupReference.pStdReferenceInfo = &h264StdSetupReference;
        break;
    }
    case VideoCodec::H265:
        if (videoEncodeDesc.referenceNum > STD_VIDEO_H265_MAX_NUM_LIST_REF) {
            NRI_REPORT_ERROR(&device, "'referenceNum' exceeds the H.265 reference list size");
            return;
        }

        h265StdPicture.pic_type = GetVideoEncodeH265PictureTypeVK(pictureDesc.frameType);
        h265StdPicture.sps_video_parameter_set_id = 0;
        h265StdPicture.pps_seq_parameter_set_id = 0;
        h265StdPicture.pps_pic_parameter_set_id = 0;
        h265StdPicture.PicOrderCntVal = pictureDesc.pictureOrderCount;
        h265StdPicture.TemporalId = pictureDesc.temporalLayer;
        h265StdPicture.flags.IrapPicFlag = pictureDesc.frameType == VideoEncodeFrameType::IDR || pictureDesc.frameType == VideoEncodeFrameType::I;
        isUsedAsReferencePicture = IsVideoEncodePictureUsedAsReferenceVK(session.m_Desc.codec, session.m_Desc.maxReferenceNum,
            videoEncodeDesc.reconstructedPicture != nullptr, 0);
        h265StdPicture.flags.is_reference = isUsedAsReferencePicture;
        h265StdPicture.flags.pic_output_flag = true;
        h265StdPicture.flags.no_output_of_prior_pics_flag = pictureDesc.frameType == VideoEncodeFrameType::IDR;
        h265StdPicture.flags.short_term_ref_pic_set_sps_flag = false;
        for (uint8_t& entry : h265ReferenceLists.RefPicList0)
            entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
        for (uint8_t& entry : h265ReferenceLists.RefPicList1)
            entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
        for (uint8_t& entry : h265ReferenceLists.list_entry_l0)
            entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
        for (uint8_t& entry : h265ReferenceLists.list_entry_l1)
            entry = STD_VIDEO_H265_NO_REFERENCE_PICTURE;
        if (videoEncodeDesc.referenceNum) {
            VideoEncodeHEVCReferenceListsVK hevcLists = {};
            if (!BuildVideoEncodeHEVCReferenceListsVK(videoEncodeDesc.references, videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, pictureDesc.frameType,
                pictureDesc.pictureOrderCount, hevcLists)) {
                if (hevcLists.missingDescriptor)
                    NRI_REPORT_ERROR(&device, "'h265ReferenceDescs' must include an entry for each H.265 reference");
                else if (hevcLists.invalidPictureOrderCount)
                    NRI_REPORT_ERROR(&device, "'h265ReferenceDescs[%u].pictureOrderCount' is not valid for the current frame type", hevcLists.failingReference);
                else
                    NRI_REPORT_ERROR(&device, "'referenceNum' exceeds the H.265 reference list size");
                return;
            }

            const uint32_t list0ReferenceNum = hevcLists.list0Num;
            const uint32_t list1ReferenceNum = hevcLists.list1Num;
            h265ReferenceLists.num_ref_idx_l0_active_minus1 = (uint8_t)(list0ReferenceNum - 1);
            h265ReferenceLists.num_ref_idx_l1_active_minus1 = list1ReferenceNum ? (uint8_t)(list1ReferenceNum - 1) : 0;
            for (uint32_t i = 0; i < list0ReferenceNum; i++) {
                const uint32_t referenceIndex = hevcLists.list0[i];
                h265ReferenceLists.RefPicList0[i] = (uint8_t)videoEncodeDesc.references[referenceIndex].slot;
                h265ReferenceLists.list_entry_l0[i] = (uint8_t)referenceIndex;
            }
            for (uint32_t i = 0; i < list1ReferenceNum; i++) {
                const uint32_t referenceIndex = hevcLists.list1[i];
                h265ReferenceLists.RefPicList1[i] = (uint8_t)videoEncodeDesc.references[referenceIndex].slot;
                h265ReferenceLists.list_entry_l1[i] = (uint8_t)referenceIndex;
            }
            h265StdPicture.pRefLists = &h265ReferenceLists;
            h265ShortTermRefPicSet.num_negative_pics = (uint8_t)list0ReferenceNum;
            h265ShortTermRefPicSet.used_by_curr_pic_s0_flag = (uint16_t)((1u << list0ReferenceNum) - 1u);
            for (uint32_t i = 0; i < list0ReferenceNum; i++) {
                const uint32_t referenceIndex = hevcLists.list0[i];
                const VideoH265ReferenceDesc* referenceDesc = FindVideoH265ReferenceDescVK(videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, videoEncodeDesc.references[referenceIndex].slot);
                const int32_t referencePoc = referenceDesc->pictureOrderCount;
                const int32_t deltaPoc = std::max(1, pictureDesc.pictureOrderCount - referencePoc);
                h265ShortTermRefPicSet.delta_poc_s0_minus1[i] = (uint16_t)(deltaPoc - 1);
            }
            h265ShortTermRefPicSet.num_positive_pics = (uint8_t)list1ReferenceNum;
            h265ShortTermRefPicSet.used_by_curr_pic_s1_flag = (uint16_t)((1u << list1ReferenceNum) - 1u);
            for (uint32_t i = 0; i < list1ReferenceNum; i++) {
                const uint32_t referenceIndex = hevcLists.list1[i];
                const VideoH265ReferenceDesc* referenceDesc = FindVideoH265ReferenceDescVK(videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, videoEncodeDesc.references[referenceIndex].slot);
                const int32_t referencePoc = referenceDesc->pictureOrderCount;
                const int32_t deltaPoc = std::max(1, referencePoc - pictureDesc.pictureOrderCount);
                h265ShortTermRefPicSet.delta_poc_s1_minus1[i] = (uint16_t)(deltaPoc - 1);
            }
            h265StdPicture.pShortTermRefPicSet = &h265ShortTermRefPicSet;
        }
        h265SliceHeader.flags.first_slice_segment_in_pic_flag = true;
        h265SliceHeader.flags.slice_sao_luma_flag = true;
        h265SliceHeader.flags.slice_sao_chroma_flag = true;
        h265SliceHeader.flags.num_ref_idx_active_override_flag = videoEncodeDesc.referenceNum != 0;
        h265SliceHeader.slice_type = pictureDesc.frameType == VideoEncodeFrameType::B ? STD_VIDEO_H265_SLICE_TYPE_B : (pictureDesc.frameType == VideoEncodeFrameType::P ? STD_VIDEO_H265_SLICE_TYPE_P : STD_VIDEO_H265_SLICE_TYPE_I);
        h265SliceHeader.MaxNumMergeCand = 5;
        h265SliceInfo.constantQp = GetVideoEncodeQPByFrameTypeVK(rateControlDesc, pictureDesc.frameType);
        h265SliceInfo.pStdSliceSegmentHeader = &h265SliceHeader;
        h265Picture.naluSliceSegmentEntryCount = 1;
        h265Picture.pNaluSliceSegmentEntries = &h265SliceInfo;
        h265Picture.pStdPictureInfo = &h265StdPicture;
        h265GopRemaining.useGopRemainingFrames = false;
        h265Picture.pNext = &h265GopRemaining;
        codecPictureInfo = &h265Picture;

        h265StdSetupReference.pic_type = h265StdPicture.pic_type;
        h265StdSetupReference.PicOrderCntVal = h265StdPicture.PicOrderCntVal;
        h265StdSetupReference.TemporalId = h265StdPicture.TemporalId;
        h265SetupReference.pStdReferenceInfo = &h265StdSetupReference;
        break;
    case VideoCodec::AV1: {
        for (int32_t& slotIndex : av1Picture.referenceNameSlotIndices)
            slotIndex = -1;
        const VideoAV1PictureDesc* av1PictureDesc = videoEncodeDesc.av1PictureDesc;
        if (!IsVideoEncodeAV1KeyFrameReferenceStateValidVK(pictureDesc.frameType, videoEncodeDesc.referenceNum)) {
            NRI_REPORT_ERROR(&device, "AV1 key frames must not reference previous pictures");
            return;
        }
        av1StdPicture.frame_type = GetVideoEncodeAV1FrameTypeVK(pictureDesc.frameType);
        av1StdPicture.frame_presentation_time = pictureDesc.frameIndex;
        av1StdPicture.current_frame_id = videoEncodeDesc.reconstructedSlot;
        av1StdPicture.order_hint = av1PictureDesc ? av1PictureDesc->orderHint : (uint8_t)pictureDesc.pictureOrderCount;
        av1StdPicture.primary_ref_frame = STD_VIDEO_AV1_PRIMARY_REF_NONE;
        av1StdPicture.refresh_frame_flags = av1PictureDesc ? av1PictureDesc->refreshFrameFlags : (pictureDesc.frameType == VideoEncodeFrameType::IDR ? 0xFF : 0);
        av1StdPicture.render_width_minus_1 = (uint16_t)(session.m_Desc.width - 1);
        av1StdPicture.render_height_minus_1 = (uint16_t)(session.m_Desc.height - 1);
        av1StdPicture.interpolation_filter = STD_VIDEO_AV1_INTERPOLATION_FILTER_EIGHTTAP;
        av1StdPicture.TxMode = STD_VIDEO_AV1_TX_MODE_SELECT;
        av1StdPicture.flags.error_resilient_mode = true;
        av1StdPicture.flags.disable_cdf_update = true;
        av1StdPicture.flags.allow_screen_content_tools = true;
        av1StdPicture.flags.force_integer_mv = true;
        av1StdPicture.flags.show_frame = true;
        av1StdPicture.flags.showable_frame = true;
        if (av1PictureDesc && av1PictureDesc->flags != VideoAV1PictureBits::NONE) {
            FillVideoAV1PictureFlagsVK(av1StdPicture.flags, av1PictureDesc->flags);
            av1StdPicture.render_width_minus_1 = av1PictureDesc->renderWidthMinus1 ? av1PictureDesc->renderWidthMinus1 : av1StdPicture.render_width_minus_1;
            av1StdPicture.render_height_minus_1 = av1PictureDesc->renderHeightMinus1 ? av1PictureDesc->renderHeightMinus1 : av1StdPicture.render_height_minus_1;
            av1StdPicture.interpolation_filter = (StdVideoAV1InterpolationFilter)av1PictureDesc->interpolationFilter;
            av1StdPicture.TxMode = (StdVideoAV1TxMode)(av1PictureDesc->txMode ? av1PictureDesc->txMode : STD_VIDEO_AV1_TX_MODE_SELECT);
            av1StdPicture.coded_denom = av1PictureDesc->codedDenom;
            av1StdPicture.delta_q_res = av1PictureDesc->deltaQRes;
            av1StdPicture.delta_lf_res = av1PictureDesc->deltaLfRes;
        }
        for (int8_t& refFrameIndex : av1StdPicture.ref_frame_idx)
            refFrameIndex = -1;
        if (av1StdPicture.frame_type == STD_VIDEO_AV1_FRAME_TYPE_KEY) {
            av1StdPicture.primary_ref_frame = STD_VIDEO_AV1_PRIMARY_REF_NONE;
            av1StdPicture.refresh_frame_flags = 0xFF;
        } else if (videoEncodeDesc.referenceNum) {
            av1StdPicture.flags.error_resilient_mode = false;
            av1StdPicture.flags.disable_cdf_update = false;
            av1StdPicture.flags.allow_screen_content_tools = false;
            av1StdPicture.flags.force_integer_mv = false;
        }
        av1StdPicture.flags.showable_frame = av1StdPicture.frame_type != STD_VIDEO_AV1_FRAME_TYPE_KEY;
        if (av1StdPicture.refresh_frame_flags && !videoEncodeDesc.reconstructedPicture) {
            NRI_REPORT_ERROR(&device, "AV1 frames that refresh DPB slots require 'reconstructedPicture'");
            return;
        }
        isUsedAsReferencePicture = IsVideoEncodePictureUsedAsReferenceVK(session.m_Desc.codec, session.m_Desc.maxReferenceNum,
            videoEncodeDesc.reconstructedPicture != nullptr, av1StdPicture.refresh_frame_flags);

        if (av1PictureDesc) {
            VideoEncodeAV1ReferenceMappingVK referenceMapping = {};
            if (!BuildVideoEncodeAV1ReferenceMappingVK(videoEncodeDesc.references, videoEncodeDesc.referenceNum, *av1PictureDesc, referenceMapping)) {
                if (referenceMapping.missingResource)
                    NRI_REPORT_ERROR(&device, "'av1PictureDesc->references[%u].slot' is not present in 'references'", referenceMapping.failingReference);
                else if (referenceMapping.invalidName)
                    NRI_REPORT_ERROR(&device, "'av1PictureDesc->references[%u].name' is invalid", referenceMapping.failingReference);
                else if (referenceMapping.missingPrimaryReference)
                    NRI_REPORT_ERROR(&device, "'av1PictureDesc->primaryReferenceName' does not name an active reference");
                else
                    NRI_REPORT_ERROR(&device, "'av1PictureDesc->referenceNum' exceeds AV1 DPB slot count or contains an invalid refFrameIndex");
                return;
            }

            for (uint32_t i = 0; i < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR; i++) {
                av1Picture.referenceNameSlotIndices[i] = referenceMapping.referenceNameSlotIndices[i];
                av1StdPicture.ref_frame_idx[i] = referenceMapping.refFrameIndices[i];
            }
            const uint8_t primaryReferenceIndex = GetVideoAV1ReferenceNameIndexVK(av1PictureDesc->primaryReferenceName);
            const int8_t primaryRefFrameIndex = primaryReferenceIndex < VK_MAX_VIDEO_AV1_REFERENCES_PER_FRAME_KHR ? referenceMapping.refFrameIndices[primaryReferenceIndex] : -1;
            if (primaryRefFrameIndex >= 0) {
                for (int8_t& refFrameIndex : av1StdPicture.ref_frame_idx) {
                    if (refFrameIndex < 0)
                        refFrameIndex = primaryRefFrameIndex;
                }
            }
            for (uint32_t i = 0; i < av1PictureDesc->referenceNum; i++) {
                const VideoAV1ReferenceDesc& reference = av1PictureDesc->references[i];
                av1StdPicture.ref_order_hint[reference.refFrameIndex] = reference.orderHint;
            }
            av1StdPicture.primary_ref_frame = primaryRefFrameIndex >= 0 ? (uint8_t)primaryRefFrameIndex : primaryReferenceIndex;
        } else if (videoEncodeDesc.referenceNum) {
            av1Picture.referenceNameSlotIndices[0] = (int32_t)videoEncodeDesc.references[0].slot;
            av1StdPicture.ref_frame_idx[0] = 0;
            av1StdPicture.primary_ref_frame = 0;
        }
        av1TileInfo.flags.uniform_tile_spacing_flag = true;
        av1TileInfo.TileCols = 1;
        av1TileInfo.TileRows = 1;
        av1TileInfo.tile_size_bytes_minus_1 = 3;
        av1MiColStarts[0] = 0;
        av1MiColStarts[1] = (uint16_t)((session.m_Desc.width + 3) / 4);
        av1MiRowStarts[0] = 0;
        av1MiRowStarts[1] = (uint16_t)((session.m_Desc.height + 3) / 4);
        av1WidthInSbsMinus1[0] = (uint16_t)((session.m_Desc.width + 63) / 64 - 1);
        av1HeightInSbsMinus1[0] = (uint16_t)((session.m_Desc.height + 63) / 64 - 1);
        av1TileInfo.pMiColStarts = av1MiColStarts.data();
        av1TileInfo.pMiRowStarts = av1MiRowStarts.data();
        av1TileInfo.pWidthInSbsMinus1 = av1WidthInSbsMinus1.data();
        av1TileInfo.pHeightInSbsMinus1 = av1HeightInSbsMinus1.data();
        if (encodeAv1TileLayout) {
            av1TileInfo.flags.uniform_tile_spacing_flag = encodeAv1TileLayout->uniformSpacing != 0;
            av1TileInfo.TileCols = encodeAv1TileLayout->columnNum;
            av1TileInfo.TileRows = encodeAv1TileLayout->rowNum;
            av1TileInfo.context_update_tile_id = encodeAv1TileLayout->contextUpdateTileId;
            av1TileInfo.tile_size_bytes_minus_1 = encodeAv1TileLayout->tileSizeBytesMinus1;
            av1TileInfo.pMiColStarts = encodeAv1TileLayout->miColumnStarts;
            av1TileInfo.pMiRowStarts = encodeAv1TileLayout->miRowStarts;
            av1TileInfo.pWidthInSbsMinus1 = encodeAv1TileLayout->widthInSuperblocksMinus1;
            av1TileInfo.pHeightInSbsMinus1 = encodeAv1TileLayout->heightInSuperblocksMinus1;
        }
        av1Quantization.base_q_idx = av1PictureDesc && av1PictureDesc->baseQIndex ? av1PictureDesc->baseQIndex : GetVideoEncodeQPByFrameTypeVK(rateControlDesc, pictureDesc.frameType);
        av1Cdef.cdef_damping_minus_3 = av1PictureDesc && av1PictureDesc->cdefDampingMinus3 ? av1PictureDesc->cdefDampingMinus3 : 3;
        av1Cdef.cdef_bits = av1PictureDesc ? av1PictureDesc->cdefBits : 0;
        av1StdPicture.pTileInfo = &av1TileInfo;
        av1StdPicture.pQuantization = &av1Quantization;
        av1StdPicture.pLoopFilter = &av1LoopFilter;
        av1StdPicture.pCDEF = &av1Cdef;
        av1StdPicture.pGlobalMotion = &av1GlobalMotion;
        const bool hasActiveAv1References = videoEncodeDesc.referenceNum != 0;
        av1Picture.predictionMode = hasActiveAv1References
            ? (pictureDesc.frameType == VideoEncodeFrameType::B ? VK_VIDEO_ENCODE_AV1_PREDICTION_MODE_BIDIRECTIONAL_COMPOUND_KHR : VK_VIDEO_ENCODE_AV1_PREDICTION_MODE_SINGLE_REFERENCE_KHR)
            : VK_VIDEO_ENCODE_AV1_PREDICTION_MODE_INTRA_ONLY_KHR;
        av1Picture.rateControlGroup = hasActiveAv1References
            ? (pictureDesc.frameType == VideoEncodeFrameType::B ? VK_VIDEO_ENCODE_AV1_RATE_CONTROL_GROUP_BIPREDICTIVE_KHR : VK_VIDEO_ENCODE_AV1_RATE_CONTROL_GROUP_PREDICTIVE_KHR)
            : VK_VIDEO_ENCODE_AV1_RATE_CONTROL_GROUP_INTRA_KHR;
        av1Picture.constantQIndex = GetVideoEncodeQPByFrameTypeVK(rateControlDesc, pictureDesc.frameType);
        av1Picture.pStdPictureInfo = &av1StdPicture;
        av1GopRemaining.useGopRemainingFrames = false;
        av1Picture.pNext = &av1GopRemaining;
        codecPictureInfo = &av1Picture;

        av1StdSetupReference.RefFrameId = videoEncodeDesc.reconstructedSlot;
        av1StdSetupReference.frame_type = av1StdPicture.frame_type;
        av1StdSetupReference.OrderHint = av1StdPicture.order_hint;
        av1SetupReference.pStdReferenceInfo = &av1StdSetupReference;
        break;
    }
    case VideoCodec::MAX_NUM:
        NRI_REPORT_ERROR(&device, "Unsupported video encode codec");
        return;
    }

    Scratch<VkVideoReferenceSlotInfoKHR> referenceSlots = NRI_ALLOCATE_SCRATCH(device, VkVideoReferenceSlotInfoKHR, videoEncodeDesc.referenceNum + 1);
    const uint32_t referenceScratchNum = videoEncodeDesc.referenceNum ? videoEncodeDesc.referenceNum : 1;
    Scratch<StdVideoEncodeH264ReferenceInfo> h264StdReferences = NRI_ALLOCATE_SCRATCH(device, StdVideoEncodeH264ReferenceInfo, referenceScratchNum);
    Scratch<VkVideoEncodeH264DpbSlotInfoKHR> h264References = NRI_ALLOCATE_SCRATCH(device, VkVideoEncodeH264DpbSlotInfoKHR, referenceScratchNum);
    Scratch<StdVideoEncodeH265ReferenceInfo> h265StdReferences = NRI_ALLOCATE_SCRATCH(device, StdVideoEncodeH265ReferenceInfo, referenceScratchNum);
    Scratch<VkVideoEncodeH265DpbSlotInfoKHR> h265References = NRI_ALLOCATE_SCRATCH(device, VkVideoEncodeH265DpbSlotInfoKHR, referenceScratchNum);
    Scratch<StdVideoEncodeAV1ReferenceInfo> av1StdReferences = NRI_ALLOCATE_SCRATCH(device, StdVideoEncodeAV1ReferenceInfo, referenceScratchNum);
    Scratch<VkVideoEncodeAV1DpbSlotInfoKHR> av1References = NRI_ALLOCATE_SCRATCH(device, VkVideoEncodeAV1DpbSlotInfoKHR, referenceScratchNum);
    for (uint32_t i = 0; i < videoEncodeDesc.referenceNum; i++) {
        if (!videoEncodeDesc.references[i].picture) {
            NRI_REPORT_ERROR(&device, "'references[%u].picture' is NULL", i);
            return;
        }

        VideoPictureVK& picture = *(VideoPictureVK*)videoEncodeDesc.references[i].picture;
        referenceSlots[i] = {VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR};
        referenceSlots[i].slotIndex = videoEncodeDesc.references[i].slot;
        referenceSlots[i].pPictureResource = &picture.m_Resource;

        if (session.m_Desc.codec == VideoCodec::H264) {
            const VideoH264ReferenceDesc* referenceDesc = FindVideoEncodeH264ReferenceDesc(videoEncodeDesc.h264PictureDesc, videoEncodeDesc.references[i].slot);
            if (!referenceDesc) {
                NRI_REPORT_ERROR(&device, "'references[%u].slot' is not described by 'h264PictureDesc'", i);
                return;
            }

            h264StdReferences[i] = {};
            h264StdReferences[i].flags.used_for_long_term_reference = referenceDesc->longTermReference != 0;
            h264StdReferences[i].primary_pic_type = GetVideoEncodeH264PictureTypeVK(referenceDesc->frameType);
            h264StdReferences[i].FrameNum = referenceDesc->frameNum;
            h264StdReferences[i].PicOrderCnt = referenceDesc->pictureOrderCount;
            h264StdReferences[i].long_term_pic_num = referenceDesc->longTermPictureIndex;
            h264StdReferences[i].long_term_frame_idx = referenceDesc->longTermFrameIndex;
            h264StdReferences[i].temporal_id = referenceDesc->temporalLayer;
            h264References[i] = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_DPB_SLOT_INFO_KHR};
            h264References[i].pStdReferenceInfo = &h264StdReferences[i];
            referenceSlots[i].pNext = &h264References[i];
        } else if (session.m_Desc.codec == VideoCodec::H265) {
            const VideoH265ReferenceDesc* referenceDesc = FindVideoH265ReferenceDescVK(videoEncodeDesc.h265ReferenceDescs, videoEncodeDesc.referenceNum, videoEncodeDesc.references[i].slot);
            h265StdReferences[i] = {};
            h265StdReferences[i].flags.used_for_long_term_reference = referenceDesc && referenceDesc->longTerm;
            h265StdReferences[i].pic_type = referenceDesc ? GetVideoEncodeH265PictureTypeVK(referenceDesc->frameType) : STD_VIDEO_H265_PICTURE_TYPE_P;
            h265StdReferences[i].PicOrderCntVal = referenceDesc ? referenceDesc->pictureOrderCount : (int32_t)videoEncodeDesc.references[i].slot;
            h265StdReferences[i].TemporalId = referenceDesc ? referenceDesc->temporalLayer : 0;
            h265References[i] = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_DPB_SLOT_INFO_KHR};
            h265References[i].pStdReferenceInfo = &h265StdReferences[i];
            referenceSlots[i].pNext = &h265References[i];
        } else if (session.m_Desc.codec == VideoCodec::AV1) {
            const VideoAV1ReferenceDesc* referenceDesc = FindVideoEncodeAV1ReferenceDesc(videoEncodeDesc.av1PictureDesc, videoEncodeDesc.references[i].slot);
            av1StdReferences[i] = {};
            av1StdReferences[i].frame_type = referenceDesc ? GetVideoEncodeAV1FrameTypeVK(referenceDesc->frameType) : STD_VIDEO_AV1_FRAME_TYPE_KEY;
            av1StdReferences[i].RefFrameId = videoEncodeDesc.references[i].slot;
            av1StdReferences[i].OrderHint = referenceDesc ? referenceDesc->orderHint : 0;
            av1References[i] = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_DPB_SLOT_INFO_KHR};
            av1References[i].pStdReferenceInfo = &av1StdReferences[i];
            referenceSlots[i].pNext = &av1References[i];
        }
    }

    VkVideoReferenceSlotInfoKHR setupReferenceSlot = {VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR};
    if (isUsedAsReferencePicture) {
        if (session.m_Desc.codec == VideoCodec::H264)
            setupReferenceSlot.pNext = &h264SetupReference;
        else if (session.m_Desc.codec == VideoCodec::H265)
            setupReferenceSlot.pNext = &h265SetupReference;
        else if (session.m_Desc.codec == VideoCodec::AV1)
            setupReferenceSlot.pNext = &av1SetupReference;
    }
    setupReferenceSlot.slotIndex = isUsedAsReferencePicture ? (int32_t)videoEncodeDesc.reconstructedSlot : -1;
    if (isUsedAsReferencePicture) {
        VideoPictureVK& reconstructedPicture = *(VideoPictureVK*)videoEncodeDesc.reconstructedPicture;
        setupReferenceSlot.pPictureResource = &reconstructedPicture.m_Resource;
    }

    VkVideoEncodeInfoKHR encodeInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_INFO_KHR};
    encodeInfo.pNext = codecPictureInfo;
    BufferVK& dstBitstream = *(BufferVK*)videoEncodeDesc.dstBitstream.buffer;
    if (videoEncodeDesc.dstBitstream.offset >= dstBitstream.GetDesc().size || videoEncodeDesc.dstBitstream.size > dstBitstream.GetDesc().size - videoEncodeDesc.dstBitstream.offset) {
        NRI_REPORT_ERROR(&device, "'dstBitstream' range is outside of 'dstBitstream.buffer'");
        return;
    }

    encodeInfo.dstBuffer = dstBitstream.GetHandle();
    encodeInfo.dstBufferOffset = videoEncodeDesc.dstBitstream.offset;
    encodeInfo.dstBufferRange = videoEncodeDesc.dstBitstream.size;
    encodeInfo.srcPictureResource = srcPicture.m_Resource;
    encodeInfo.pSetupReferenceSlot = isUsedAsReferencePicture ? &setupReferenceSlot : nullptr;
    encodeInfo.referenceSlotCount = videoEncodeDesc.referenceNum;
    encodeInfo.pReferenceSlots = referenceSlots;

    if (isUsedAsReferencePicture) {
        referenceSlots[videoEncodeDesc.referenceNum] = setupReferenceSlot;
        referenceSlots[videoEncodeDesc.referenceNum].slotIndex = -1;
        referenceSlots[videoEncodeDesc.referenceNum].pNext = nullptr;
    }

    VkVideoBeginCodingInfoKHR beginInfo = {VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR};
    beginInfo.videoSession = session.m_Handle;
    beginInfo.videoSessionParameters = parameters.m_Handle;
    beginInfo.referenceSlotCount = videoEncodeDesc.referenceNum + (isUsedAsReferencePicture ? 1 : 0);
    beginInfo.pReferenceSlots = referenceSlots;

    VkVideoEndCodingInfoKHR endInfo = {VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR};
    vk.CmdBeginVideoCodingKHR(commandBufferVK, &beginInfo);
    if (!session.m_Initialized) {
        VkVideoCodingControlInfoKHR controlInfo = {VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR};
        VkVideoEncodeRateControlInfoKHR rateControlInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_INFO_KHR};
        VkVideoEncodeRateControlLayerInfoKHR rateControlLayer = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_LAYER_INFO_KHR};
        VkVideoEncodeH264RateControlInfoKHR h264RateControlInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_RATE_CONTROL_INFO_KHR};
        VkVideoEncodeH264RateControlLayerInfoKHR h264RateControlLayer = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_RATE_CONTROL_LAYER_INFO_KHR};
        VkVideoEncodeH265RateControlInfoKHR h265RateControlInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_RATE_CONTROL_INFO_KHR};
        VkVideoEncodeH265RateControlLayerInfoKHR h265RateControlLayer = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_RATE_CONTROL_LAYER_INFO_KHR};
        VkVideoEncodeAV1RateControlInfoKHR av1RateControlInfo = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_RATE_CONTROL_INFO_KHR};
        VkVideoEncodeAV1RateControlLayerInfoKHR av1RateControlLayer = {VK_STRUCTURE_TYPE_VIDEO_ENCODE_AV1_RATE_CONTROL_LAYER_INFO_KHR};

        rateControlInfo.rateControlMode = VK_VIDEO_ENCODE_RATE_CONTROL_MODE_DISABLED_BIT_KHR;
        rateControlInfo.layerCount = 1;
        rateControlInfo.pLayers = &rateControlLayer;
        rateControlLayer.frameRateNumerator = rateControlDesc.frameRateNumerator ? rateControlDesc.frameRateNumerator : 30;
        rateControlLayer.frameRateDenominator = rateControlDesc.frameRateDenominator ? rateControlDesc.frameRateDenominator : 1;
        controlInfo.flags = VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR | VK_VIDEO_CODING_CONTROL_ENCODE_RATE_CONTROL_BIT_KHR;
        controlInfo.pNext = &rateControlInfo;
        switch (session.m_Desc.codec) {
        case VideoCodec::H264:
            h264RateControlInfo.gopFrameCount = videoEncodeDesc.referenceNum ? 60 : 1;
            h264RateControlInfo.idrPeriod = h264RateControlInfo.gopFrameCount;
            h264RateControlInfo.temporalLayerCount = 1;
            h264RateControlLayer.useMinQp = true;
            h264RateControlLayer.minQp = {rateControlDesc.qpI, rateControlDesc.qpP, rateControlDesc.qpB};
            h264RateControlLayer.useMaxQp = true;
            h264RateControlLayer.maxQp = {rateControlDesc.qpI, rateControlDesc.qpP, rateControlDesc.qpB};
            rateControlInfo.pNext = &h264RateControlInfo;
            rateControlLayer.pNext = &h264RateControlLayer;
            break;
        case VideoCodec::H265:
            h265RateControlInfo.gopFrameCount = 0;
            h265RateControlInfo.idrPeriod = 0;
            h265RateControlInfo.subLayerCount = 1;
            h265RateControlLayer.useMinQp = true;
            h265RateControlLayer.minQp = {rateControlDesc.qpI, rateControlDesc.qpP, rateControlDesc.qpB};
            h265RateControlLayer.useMaxQp = true;
            h265RateControlLayer.maxQp = {rateControlDesc.qpI, rateControlDesc.qpP, rateControlDesc.qpB};
            rateControlInfo.pNext = &h265RateControlInfo;
            rateControlLayer.pNext = &h265RateControlLayer;
            break;
        case VideoCodec::AV1:
            // NRI does not expose a fixed GOP length. Keep the AV1 rate-control GOP open-ended so callers can submit
            // key + inter-frame sequences through the same session.
            av1RateControlInfo.gopFrameCount = 0;
            av1RateControlInfo.keyFramePeriod = 0;
            av1RateControlInfo.temporalLayerCount = 1;
            av1RateControlLayer.useMinQIndex = true;
            av1RateControlLayer.minQIndex = {rateControlDesc.qpI, rateControlDesc.qpP, rateControlDesc.qpB};
            av1RateControlLayer.useMaxQIndex = true;
            av1RateControlLayer.maxQIndex = {rateControlDesc.qpI, rateControlDesc.qpP, rateControlDesc.qpB};
            rateControlInfo.pNext = &av1RateControlInfo;
            rateControlLayer.pNext = &av1RateControlLayer;
            break;
        case VideoCodec::MAX_NUM:
            break;
        }
        vk.CmdControlVideoCodingKHR(commandBufferVK, &controlInfo);
        session.m_Initialized = true;
    }
    vk.CmdEncodeVideoKHR(commandBufferVK, &encodeInfo);
    vk.CmdEndVideoCodingKHR(commandBufferVK, &endInfo);
}

Result DeviceVK::FillFunctionTable(VideoInterface& table) const {
    if (m_Desc.adapterDesc.queueNum[(size_t)QueueType::VIDEO_DECODE] == 0 && m_Desc.adapterDesc.queueNum[(size_t)QueueType::VIDEO_ENCODE] == 0)
        return Result::UNSUPPORTED;

    table.CreateVideoSession = ::CreateVideoSession;
    table.DestroyVideoSession = ::DestroyVideoSession;
    table.CreateVideoSessionParameters = ::CreateVideoSessionParameters;
    table.DestroyVideoSessionParameters = ::DestroyVideoSessionParameters;
    table.CreateVideoPicture = ::CreateVideoPicture;
    table.DestroyVideoPicture = ::DestroyVideoPicture;
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

static Result NRI_CALL CreateVideoSessionParametersVK(Device& device, const VideoSessionParametersVKDesc& videoSessionParametersVKDesc, VideoSessionParameters*& videoSessionParameters) {
    if (!videoSessionParametersVKDesc.session)
        return Result::INVALID_ARGUMENT;

    DeviceVK& deviceVK = (DeviceVK&)device;
    VideoSessionParametersVK* impl = Allocate<VideoSessionParametersVK>(deviceVK.GetAllocationCallbacks(), deviceVK);
    Result result = impl->CreateNative(*(VideoSessionVK*)videoSessionParametersVKDesc.session, videoSessionParametersVKDesc.vkSessionParametersCreateInfo);

    if (result != Result::SUCCESS) {
        Destroy(impl);
        videoSessionParameters = nullptr;
    } else
        videoSessionParameters = (VideoSessionParameters*)impl;

    return result;
}

static uint32_t NRI_CALL GetQueueFamilyIndexVK(const Queue& queue) {
    return ((QueueVK&)queue).GetFamilyIndex();
}

static VKNonDispatchableHandle NRI_CALL GetVideoSessionVK(const VideoSession& videoSession) {
    return (VKNonDispatchableHandle)((const VideoSessionVK&)videoSession).m_Handle;
}

static VKNonDispatchableHandle NRI_CALL GetVideoSessionParametersVK(const VideoSessionParameters& videoSessionParameters) {
    return (VKNonDispatchableHandle)((const VideoSessionParametersVK&)videoSessionParameters).m_Handle;
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
    table.CreateVideoSessionParametersVK = ::CreateVideoSessionParametersVK;
    table.GetQueueFamilyIndexVK = ::GetQueueFamilyIndexVK;
    table.GetVideoSessionVK = ::GetVideoSessionVK;
    table.GetVideoSessionParametersVK = ::GetVideoSessionParametersVK;
    table.GetPhysicalDeviceVK = ::GetPhysicalDeviceVK;
    table.GetInstanceVK = ::GetInstanceVK;
    table.GetDeviceProcAddrVK = ::GetDeviceProcAddrVK;
    table.GetInstanceProcAddrVK = ::GetInstanceProcAddrVK;

    return Result::SUCCESS;
}

#pragma endregion
