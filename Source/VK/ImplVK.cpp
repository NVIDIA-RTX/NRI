// © 2021 NVIDIA Corporation

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
#include "PipelineLayoutVK.h"
#include "PipelineVK.h"
#include "QueryPoolVK.h"
#include "QueueVK.h"
#include "SwapChainVK.h"
#include "TextureVK.h"

#include "HelperDataUpload.h"
#include "HelperDeviceMemoryAllocator.h"
#include "Streamer.h"
#include "Upscaler.h"

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
#include "PipelineLayoutVK.hpp"
#include "PipelineVK.hpp"
#include "QueryPoolVK.hpp"
#include "QueueVK.hpp"
#include "ResourceAllocatorVK.hpp"
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
    return ((const BufferVK&)buffer).GetDesc();
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture& texture) {
    return ((const TextureVK&)texture).GetDesc();
}

static FormatSupportBits NRI_CALL GetFormatSupport(const Device& device, Format format) {
    return ((const DeviceVK&)device).GetFormatSupport(format);
}

static uint32_t NRI_CALL GetQuerySize(const QueryPool& queryPool) {
    return ((QueryPoolVK&)queryPool).GetQuerySize();
}

static void NRI_CALL GetBufferMemoryDesc(const Buffer& buffer, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((const BufferVK&)buffer).GetMemoryDesc(memoryLocation, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc(const Texture& texture, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((const TextureVK&)texture).GetMemoryDesc(memoryLocation, memoryDesc);
}

static void NRI_CALL GetBufferMemoryDesc2(const Device& device, const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((const DeviceVK&)device).GetMemoryDesc2(bufferDesc, memoryLocation, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc2(const Device& device, const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((const DeviceVK&)device).GetMemoryDesc2(textureDesc, memoryLocation, memoryDesc);
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

static Result NRI_CALL CreateBuffer(Device& device, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceVK&)device).CreateImplementation<BufferVK>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTexture(Device& device, const TextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceVK&)device).CreateImplementation<TextureVK>(texture, textureDesc);
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
    DeviceVK& device = ((const BufferVK*)bufferViewDesc.buffer)->GetDevice();
    return device.CreateImplementation<DescriptorVK>(bufferView, bufferViewDesc);
}

static Result NRI_CALL CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceVK& device = ((const TextureVK*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorVK>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceVK& device = ((const TextureVK*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorVK>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceVK& device = ((const TextureVK*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorVK>(textureView, textureViewDesc);
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    Destroy((CommandAllocatorVK*)&commandAllocator);
}

static void NRI_CALL DestroyCommandBuffer(CommandBuffer& commandBuffer) {
    Destroy((CommandBufferVK*)&commandBuffer);
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    Destroy((DescriptorPoolVK*)&descriptorPool);
}

static void NRI_CALL DestroyBuffer(Buffer& buffer) {
    Destroy((BufferVK*)&buffer);
}

static void NRI_CALL DestroyTexture(Texture& texture) {
    Destroy((TextureVK*)&texture);
}

static void NRI_CALL DestroyDescriptor(Descriptor& descriptor) {
    Destroy((DescriptorVK*)&descriptor);
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
    Destroy((PipelineLayoutVK*)&pipelineLayout);
}

static void NRI_CALL DestroyPipeline(Pipeline& pipeline) {
    Destroy((PipelineVK*)&pipeline);
}

static void NRI_CALL DestroyQueryPool(QueryPool& queryPool) {
    Destroy((QueryPoolVK*)&queryPool);
}

static void NRI_CALL DestroyFence(Fence& fence) {
    Destroy((FenceVK*)&fence);
}

static Result NRI_CALL AllocateMemory(Device& device, const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    return ((DeviceVK&)device).CreateImplementation<MemoryVK>(memory, allocateMemoryDesc);
}

static Result NRI_CALL BindBufferMemory(Device& device, const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceVK&)device).BindBufferMemory(memoryBindingDescs, memoryBindingDescNum);
}

static Result NRI_CALL BindTextureMemory(Device& device, const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceVK&)device).BindTextureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static void NRI_CALL FreeMemory(Memory& memory) {
    Destroy((MemoryVK*)&memory);
}

static Result NRI_CALL BeginCommandBuffer(CommandBuffer& commandBuffer, const DescriptorPool* descriptorPool) {
    return ((CommandBufferVK&)commandBuffer).Begin(descriptorPool);
}

static void NRI_CALL CmdSetDescriptorPool(CommandBuffer& commandBuffer, const DescriptorPool& descriptorPool) {
    ((CommandBufferVK&)commandBuffer).SetDescriptorPool(descriptorPool);
}

static void NRI_CALL CmdSetPipelineLayout(CommandBuffer& commandBuffer, const PipelineLayout& pipelineLayout) {
    ((CommandBufferVK&)commandBuffer).SetPipelineLayout(pipelineLayout);
}

static void NRI_CALL CmdSetDescriptorSet(CommandBuffer& commandBuffer, uint32_t setIndex, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    ((CommandBufferVK&)commandBuffer).SetDescriptorSet(setIndex, descriptorSet, dynamicConstantBufferOffsets);
}

static void NRI_CALL CmdSetRootConstants(CommandBuffer& commandBuffer, uint32_t rootConstantIndex, const void* data, uint32_t size) {
    ((CommandBufferVK&)commandBuffer).SetRootConstants(rootConstantIndex, data, size);
}

static void NRI_CALL CmdSetRootDescriptor(CommandBuffer& commandBuffer, uint32_t rootDescriptorIndex, Descriptor& descriptor) {
    ((CommandBufferVK&)commandBuffer).SetRootDescriptor(rootDescriptorIndex, descriptor);
}

static void NRI_CALL CmdSetPipeline(CommandBuffer& commandBuffer, const Pipeline& pipeline) {
    ((CommandBufferVK&)commandBuffer).SetPipeline(pipeline);
}

static void NRI_CALL CmdBarrier(CommandBuffer& commandBuffer, const BarrierGroupDesc& barrierGroupDesc) {
    ((CommandBufferVK&)commandBuffer).Barrier(barrierGroupDesc);
}

static void NRI_CALL CmdSetIndexBuffer(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, IndexType indexType) {
    ((CommandBufferVK&)commandBuffer).SetIndexBuffer(buffer, offset, indexType);
}

static void NRI_CALL CmdSetVertexBuffers(CommandBuffer& commandBuffer, uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    ((CommandBufferVK&)commandBuffer).SetVertexBuffers(baseSlot, bufferNum, buffers, offsets);
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

static void NRI_CALL CmdBeginRendering(CommandBuffer& commandBuffer, const AttachmentsDesc& attachmentsDesc) {
    ((CommandBufferVK&)commandBuffer).BeginRendering(attachmentsDesc);
}

static void NRI_CALL CmdClearAttachments(CommandBuffer& commandBuffer, const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    ((CommandBufferVK&)commandBuffer).ClearAttachments(clearDescs, clearDescNum, rects, rectNum);
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

static void NRI_CALL CmdCopyTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    ((CommandBufferVK&)commandBuffer).CopyTexture(dstTexture, dstRegionDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdResolveTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    ((CommandBufferVK&)commandBuffer).ResolveTexture(dstTexture, dstRegionDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdUploadBufferToTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    ((CommandBufferVK&)commandBuffer).UploadBufferToTexture(dstTexture, dstRegionDesc, srcBuffer, srcDataLayoutDesc);
}

static void NRI_CALL CmdReadbackTextureToBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    ((CommandBufferVK&)commandBuffer).ReadbackTextureToBuffer(dstBuffer, dstDataLayoutDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdClearStorageBuffer(CommandBuffer& commandBuffer, const ClearStorageBufferDesc& clearDesc) {
    ((CommandBufferVK&)commandBuffer).ClearStorageBuffer(clearDesc);
}

static void NRI_CALL CmdClearStorageTexture(CommandBuffer& commandBuffer, const ClearStorageTextureDesc& clearDesc) {
    ((CommandBufferVK&)commandBuffer).ClearStorageTexture(clearDesc);
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

static void NRI_CALL QueueSubmit(Queue& queue, const QueueSubmitDesc& workSubmissionDesc) {
    ((QueueVK&)queue).Submit(workSubmissionDesc, nullptr);
}

static void NRI_CALL Wait(Fence& fence, uint64_t value) {
    ((FenceVK&)fence).Wait(value);
}

static uint64_t NRI_CALL GetFenceValue(Fence& fence) {
    return ((FenceVK&)fence).GetFenceValue();
}

static void NRI_CALL UpdateDescriptorRanges(DescriptorSet& descriptorSet, uint32_t baseRange, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    ((DescriptorSetVK&)descriptorSet).UpdateDescriptorRanges(baseRange, rangeNum, rangeUpdateDescs);
}

static void NRI_CALL UpdateDynamicConstantBuffers(DescriptorSet& descriptorSet, uint32_t baseDynamicConstantBuffer, uint32_t dynamicConstantBufferNum, const Descriptor* const* descriptors) {
    ((DescriptorSetVK&)descriptorSet).UpdateDynamicConstantBuffers(baseDynamicConstantBuffer, dynamicConstantBufferNum, descriptors);
}

static void NRI_CALL CopyDescriptorSet(DescriptorSet& descriptorSet, const DescriptorSetCopyDesc& descriptorSetCopyDesc) {
    ((DescriptorSetVK&)descriptorSet).Copy(descriptorSetCopyDesc);
}

static Result NRI_CALL AllocateDescriptorSets(DescriptorPool& descriptorPool, const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    return ((DescriptorPoolVK&)descriptorPool).AllocateDescriptorSets(pipelineLayout, setIndex, descriptorSets, instanceNum, variableDescriptorNum);
}

static void NRI_CALL ResetDescriptorPool(DescriptorPool& descriptorPool) {
    ((DescriptorPoolVK&)descriptorPool).Reset();
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

static void NRI_CALL SetDebugName(Object* object, const char* name) {
    MaybeUnused(object, name);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    if (object)
        ((DebugNameBase*)object)->SetDebugName(name);
#endif
}

static void* NRI_CALL GetDeviceNativeObject(const Device& device) {
    if (!(&device))
        return nullptr;

    return (VkDevice)((DeviceVK&)device);
}

static void* NRI_CALL GetQueueNativeObject(const Queue& queue) {
    if (!(&queue))
        return nullptr;

    return (VkQueue)((QueueVK&)queue);
}

static void* NRI_CALL GetCommandBufferNativeObject(const CommandBuffer& commandBuffer) {
    if (!(&commandBuffer))
        return nullptr;

    return (VkCommandBuffer)((CommandBufferVK&)commandBuffer);
}

static uint64_t NRI_CALL GetBufferNativeObject(const Buffer& buffer) {
    if (!(&buffer))
        return 0;

    return uint64_t(((BufferVK&)buffer).GetHandle());
}

static uint64_t NRI_CALL GetTextureNativeObject(const Texture& texture) {
    if (!(&texture))
        return 0;

    return uint64_t(((TextureVK&)texture).GetHandle());
}

static uint64_t NRI_CALL GetDescriptorNativeObject(const Descriptor& descriptor) {
    if (!(&descriptor))
        return 0;

    const DescriptorVK& d = ((DescriptorVK&)descriptor);

    uint64_t handle = 0;
    if (d.GetType() == DescriptorTypeVK::BUFFER_VIEW)
        handle = (uint64_t)d.GetBufferView();
    else if (d.GetType() == DescriptorTypeVK::IMAGE_VIEW)
        handle = (uint64_t)d.GetImageView();
    else if (d.GetType() == DescriptorTypeVK::SAMPLER)
        handle = (uint64_t)d.GetSampler();
    else if (d.GetType() == DescriptorTypeVK::ACCELERATION_STRUCTURE)
        handle = (uint64_t)d.GetAccelerationStructure();

    return handle;
}

Result DeviceVK::FillFunctionTable(CoreInterface& table) const {
    table.GetDeviceDesc = ::GetDeviceDesc;
    table.GetBufferDesc = ::GetBufferDesc;
    table.GetTextureDesc = ::GetTextureDesc;
    table.GetFormatSupport = ::GetFormatSupport;
    table.GetQuerySize = ::GetQuerySize;
    table.GetBufferMemoryDesc = ::GetBufferMemoryDesc;
    table.GetTextureMemoryDesc = ::GetTextureMemoryDesc;
    table.GetBufferMemoryDesc2 = ::GetBufferMemoryDesc2;
    table.GetTextureMemoryDesc2 = ::GetTextureMemoryDesc2;
    table.GetQueue = ::GetQueue;
    table.CreateCommandAllocator = ::CreateCommandAllocator;
    table.CreateCommandBuffer = ::CreateCommandBuffer;
    table.CreateDescriptorPool = ::CreateDescriptorPool;
    table.CreateBuffer = ::CreateBuffer;
    table.CreateTexture = ::CreateTexture;
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
    table.BindBufferMemory = ::BindBufferMemory;
    table.BindTextureMemory = ::BindTextureMemory;
    table.FreeMemory = ::FreeMemory;
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
    table.CmdClearStorageBuffer = ::CmdClearStorageBuffer;
    table.CmdClearStorageTexture = ::CmdClearStorageTexture;
    table.CmdResolveTexture = ::CmdResolveTexture;
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
    table.Wait = ::Wait;
    table.GetFenceValue = ::GetFenceValue;
    table.UpdateDescriptorRanges = ::UpdateDescriptorRanges;
    table.UpdateDynamicConstantBuffers = ::UpdateDynamicConstantBuffers;
    table.CopyDescriptorSet = ::CopyDescriptorSet;
    table.AllocateDescriptorSets = ::AllocateDescriptorSets;
    table.ResetDescriptorPool = ::ResetDescriptorPool;
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
    QueueVK& queueVK = (QueueVK&)queue;
    DeviceVK& deviceVK = queueVK.GetDevice();
    HelperDataUpload helperDataUpload(deviceVK.GetCoreInterface(), (Device&)deviceVK, queue);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

static Result NRI_CALL WaitForIdle(Queue& queue) {
    if (!(&queue))
        return Result::SUCCESS;

    return ((QueueVK&)queue).WaitForIdle();
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
    table.WaitForIdle = ::WaitForIdle;
    table.QueryVideoMemoryInfo = ::QueryVideoMemoryInfo;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Low latency  ]

static void NRI_CALL QueueSubmitTrackable(Queue& queue, const QueueSubmitDesc& workSubmissionDesc, const SwapChain& swapChain) {
    ((QueueVK&)queue).Submit(workSubmissionDesc, &swapChain);
}

static Result SetLatencySleepMode(SwapChain& swapChain, const LatencySleepMode& latencySleepMode) {
    return ((SwapChainVK&)swapChain).SetLatencySleepMode(latencySleepMode);
}

static Result SetLatencyMarker(SwapChain& swapChain, LatencyMarker latencyMarker) {
    return ((SwapChainVK&)swapChain).SetLatencyMarker(latencyMarker);
}

static Result LatencySleep(SwapChain& swapChain) {
    return ((SwapChainVK&)swapChain).LatencySleep();
}

static Result GetLatencyReport(const SwapChain& swapChain, LatencyReport& latencyReport) {
    return ((SwapChainVK&)swapChain).GetLatencyReport(latencyReport);
}

Result DeviceVK::FillFunctionTable(LowLatencyInterface& table) const {
    if (!m_Desc.isLowLatencySupported)
        return Result::UNSUPPORTED;

    table.SetLatencySleepMode = ::SetLatencySleepMode;
    table.SetLatencyMarker = ::SetLatencyMarker;
    table.LatencySleep = ::LatencySleep;
    table.GetLatencyReport = ::GetLatencyReport;
    table.QueueSubmitTrackable = ::QueueSubmitTrackable;

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
    if (!m_Desc.isMeshShaderSupported)
        return Result::UNSUPPORTED;

    table.CmdDrawMeshTasks = ::CmdDrawMeshTasks;
    table.CmdDrawMeshTasksIndirect = ::CmdDrawMeshTasksIndirect;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  RayTracing  ]

static uint64_t NRI_CALL GetAccelerationStructureUpdateScratchBufferSize(const AccelerationStructure& accelerationStructure) {
    return ((AccelerationStructureVK&)accelerationStructure).GetUpdateScratchBufferSize();
}

static uint64_t NRI_CALL GetAccelerationStructureBuildScratchBufferSize(const AccelerationStructure& accelerationStructure) {
    return ((AccelerationStructureVK&)accelerationStructure).GetBuildScratchBufferSize();
}

static uint64_t NRI_CALL GetAccelerationStructureHandle(const AccelerationStructure& accelerationStructure) {
    static_assert(sizeof(uint64_t) == sizeof(VkDeviceAddress), "type mismatch");
    return (uint64_t)((AccelerationStructureVK&)accelerationStructure).GetDeviceAddress();
}

static Buffer* NRI_CALL GetAccelerationStructureBuffer(const AccelerationStructure& accelerationStructure) {
    return (Buffer*)((AccelerationStructureVK&)accelerationStructure).GetBuffer();
}

static Result NRI_CALL CreateAccelerationStructureDescriptor(const AccelerationStructure& accelerationStructure, Descriptor*& descriptor) {
    return ((AccelerationStructureVK&)accelerationStructure).CreateDescriptor(descriptor);
}

static uint64_t NRI_CALL GetAccelerationStructureNativeObject(const AccelerationStructure& accelerationStructure) {
    return uint64_t(((AccelerationStructureVK&)accelerationStructure).GetHandle());
}

static void NRI_CALL CmdBuildTopLevelAccelerationStructure(CommandBuffer& commandBuffer, uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {
    ((CommandBufferVK&)commandBuffer).BuildTopLevelAccelerationStructure(instanceNum, buffer, bufferOffset, flags, dst, scratch, scratchOffset);
}

static void NRI_CALL CmdBuildBottomLevelAccelerationStructure(CommandBuffer& commandBuffer, uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {
    ((CommandBufferVK&)commandBuffer).BuildBottomLevelAccelerationStructure(geometryObjectNum, geometryObjects, flags, dst, scratch, scratchOffset);
}

static void NRI_CALL CmdUpdateTopLevelAccelerationStructure(CommandBuffer& commandBuffer, uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, const AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {
    ((CommandBufferVK&)commandBuffer).UpdateTopLevelAccelerationStructure(instanceNum, buffer, bufferOffset, flags, dst, src, scratch, scratchOffset);
}

static void NRI_CALL CmdUpdateBottomLevelAccelerationStructure(CommandBuffer& commandBuffer, uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, const AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {
    ((CommandBufferVK&)commandBuffer).UpdateBottomLevelAccelerationStructure(geometryObjectNum, geometryObjects, flags, dst, src, scratch, scratchOffset);
}

static void NRI_CALL CmdCopyAccelerationStructure(CommandBuffer& commandBuffer, AccelerationStructure& dst, const AccelerationStructure& src, CopyMode mode) {
    ((CommandBufferVK&)commandBuffer).CopyAccelerationStructure(dst, src, mode);
}

static void NRI_CALL CmdWriteAccelerationStructureSize(CommandBuffer& commandBuffer, const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryOffset) {
    ((CommandBufferVK&)commandBuffer).WriteAccelerationStructureSize(accelerationStructures, accelerationStructureNum, queryPool, queryOffset);
}

static void NRI_CALL CmdDispatchRays(CommandBuffer& commandBuffer, const DispatchRaysDesc& dispatchRaysDesc) {
    ((CommandBufferVK&)commandBuffer).DispatchRays(dispatchRaysDesc);
}

static void NRI_CALL CmdDispatchRaysIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset) {
    ((CommandBufferVK&)commandBuffer).DispatchRaysIndirect(buffer, offset);
}

static void NRI_CALL GetAccelerationStructureMemoryDesc(const AccelerationStructure& accelerationStructure, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((AccelerationStructureVK&)accelerationStructure).GetBuffer()->GetMemoryDesc(memoryLocation, memoryDesc);
}

static void NRI_CALL GetAccelerationStructureMemoryDesc2(const Device& device, const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((DeviceVK&)device).GetMemoryDesc2(accelerationStructureDesc, memoryLocation, memoryDesc);
}

static Result NRI_CALL CreateRayTracingPipeline(Device& device, const RayTracingPipelineDesc& pipelineDesc, Pipeline*& pipeline) {
    return ((DeviceVK&)device).CreateImplementation<PipelineVK>(pipeline, pipelineDesc);
}

static Result NRI_CALL CreateAccelerationStructure(Device& device, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVK&)device).CreateImplementation<AccelerationStructureVK>(accelerationStructure, accelerationStructureDesc);
}

static Result NRI_CALL BindAccelerationStructureMemory(Device& device, const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceVK&)device).BindAccelerationStructureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static void NRI_CALL DestroyAccelerationStructure(AccelerationStructure& accelerationStructure) {
    Destroy((AccelerationStructureVK*)&accelerationStructure);
}

static Result NRI_CALL WriteShaderGroupIdentifiers(const Pipeline& pipeline, uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* buffer) {
    return ((const PipelineVK&)pipeline).WriteShaderGroupIdentifiers(baseShaderGroupIndex, shaderGroupNum, buffer);
}

Result DeviceVK::FillFunctionTable(RayTracingInterface& table) const {
    if (!m_Desc.isRayTracingSupported)
        return Result::UNSUPPORTED;

    table.GetAccelerationStructureUpdateScratchBufferSize = ::GetAccelerationStructureUpdateScratchBufferSize;
    table.GetAccelerationStructureBuildScratchBufferSize = ::GetAccelerationStructureBuildScratchBufferSize;
    table.GetAccelerationStructureHandle = ::GetAccelerationStructureHandle;
    table.GetAccelerationStructureBuffer = ::GetAccelerationStructureBuffer;
    table.GetAccelerationStructureMemoryDesc = ::GetAccelerationStructureMemoryDesc;
    table.GetAccelerationStructureMemoryDesc2 = ::GetAccelerationStructureMemoryDesc2;
    table.CreateRayTracingPipeline = ::CreateRayTracingPipeline;
    table.CreateAccelerationStructure = ::CreateAccelerationStructure;
    table.CreateAccelerationStructureDescriptor = ::CreateAccelerationStructureDescriptor;
    table.DestroyAccelerationStructure = ::DestroyAccelerationStructure;
    table.BindAccelerationStructureMemory = ::BindAccelerationStructureMemory;
    table.WriteShaderGroupIdentifiers = ::WriteShaderGroupIdentifiers;
    table.CmdBuildTopLevelAccelerationStructure = ::CmdBuildTopLevelAccelerationStructure;
    table.CmdBuildBottomLevelAccelerationStructure = ::CmdBuildBottomLevelAccelerationStructure;
    table.CmdUpdateTopLevelAccelerationStructure = ::CmdUpdateTopLevelAccelerationStructure;
    table.CmdUpdateBottomLevelAccelerationStructure = ::CmdUpdateBottomLevelAccelerationStructure;
    table.CmdDispatchRays = ::CmdDispatchRays;
    table.CmdDispatchRaysIndirect = ::CmdDispatchRaysIndirect;
    table.CmdCopyAccelerationStructure = ::CmdCopyAccelerationStructure;
    table.CmdWriteAccelerationStructureSize = ::CmdWriteAccelerationStructureSize;
    table.GetAccelerationStructureNativeObject = ::GetAccelerationStructureNativeObject;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  ResourceAllocator  ]

static Result AllocateBuffer(Device& device, const AllocateBufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceVK&)device).CreateImplementation<BufferVK>(buffer, bufferDesc);
}

static Result AllocateTexture(Device& device, const AllocateTextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceVK&)device).CreateImplementation<TextureVK>(texture, textureDesc);
}

static Result AllocateAccelerationStructure(Device& device, const AllocateAccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVK&)device).CreateImplementation<AccelerationStructureVK>(accelerationStructure, accelerationStructureDesc);
}

Result DeviceVK::FillFunctionTable(ResourceAllocatorInterface& table) const {
    table.AllocateBuffer = ::AllocateBuffer;
    table.AllocateTexture = ::AllocateTexture;
    table.AllocateAccelerationStructure = ::AllocateAccelerationStructure;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Streamer  ]

static Result CreateStreamer(Device& device, const StreamerDesc& streamerDesc, Streamer*& streamer) {
    DeviceVK& deviceVK = (DeviceVK&)device;
    StreamerImpl* impl = Allocate<StreamerImpl>(deviceVK.GetAllocationCallbacks(), device, deviceVK.GetCoreInterface());
    Result result = impl->Create(streamerDesc);

    if (result != Result::SUCCESS) {
        Destroy(deviceVK.GetAllocationCallbacks(), impl);
        streamer = nullptr;
    } else
        streamer = (Streamer*)impl;

    return result;
}

static void DestroyStreamer(Streamer& streamer) {
    Destroy(((DeviceBase&)((StreamerImpl&)streamer).GetDevice()).GetAllocationCallbacks(), (StreamerImpl*)&streamer);
}

static Buffer* GetStreamerConstantBuffer(Streamer& streamer) {
    return ((StreamerImpl&)streamer).GetConstantBuffer();
}

static uint32_t UpdateStreamerConstantBuffer(Streamer& streamer, const void* data, uint32_t dataSize) {
    return ((StreamerImpl&)streamer).UpdateConstantBuffer(data, dataSize);
}

static uint64_t AddStreamerBufferUpdateRequest(Streamer& streamer, const BufferUpdateRequestDesc& bufferUpdateRequestDesc) {
    return ((StreamerImpl&)streamer).AddBufferUpdateRequest(bufferUpdateRequestDesc);
}

static uint64_t AddStreamerTextureUpdateRequest(Streamer& streamer, const TextureUpdateRequestDesc& textureUpdateRequestDesc) {
    return ((StreamerImpl&)streamer).AddTextureUpdateRequest(textureUpdateRequestDesc);
}

static Result CopyStreamerUpdateRequests(Streamer& streamer) {
    return ((StreamerImpl&)streamer).CopyUpdateRequests();
}

static Buffer* GetStreamerDynamicBuffer(Streamer& streamer) {
    return ((StreamerImpl&)streamer).GetDynamicBuffer();
}

static void CmdUploadStreamerUpdateRequests(CommandBuffer& commandBuffer, Streamer& streamer) {
    ((StreamerImpl&)streamer).CmdUploadUpdateRequests(commandBuffer);
}

Result DeviceVK::FillFunctionTable(StreamerInterface& table) const {
    table.CreateStreamer = ::CreateStreamer;
    table.DestroyStreamer = ::DestroyStreamer;
    table.GetStreamerConstantBuffer = ::GetStreamerConstantBuffer;
    table.GetStreamerDynamicBuffer = ::GetStreamerDynamicBuffer;
    table.AddStreamerBufferUpdateRequest = ::AddStreamerBufferUpdateRequest;
    table.AddStreamerTextureUpdateRequest = ::AddStreamerTextureUpdateRequest;
    table.UpdateStreamerConstantBuffer = ::UpdateStreamerConstantBuffer;
    table.CopyStreamerUpdateRequests = ::CopyStreamerUpdateRequests;
    table.CmdUploadStreamerUpdateRequests = ::CmdUploadStreamerUpdateRequests;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  SwapChain  ]

static Result NRI_CALL CreateSwapChain(Device& device, const SwapChainDesc& swapChainDesc, SwapChain*& swapChain) {
    return ((DeviceVK&)device).CreateImplementation<SwapChainVK>(swapChain, swapChainDesc);
}

static void NRI_CALL DestroySwapChain(SwapChain& swapChain) {
    Destroy((SwapChainVK*)&swapChain);
}

static Texture* const* NRI_CALL GetSwapChainTextures(const SwapChain& swapChain, uint32_t& textureNum) {
    return ((SwapChainVK&)swapChain).GetTextures(textureNum);
}

static uint32_t NRI_CALL AcquireNextSwapChainTexture(SwapChain& swapChain) {
    return ((SwapChainVK&)swapChain).AcquireNextTexture();
}

static Result NRI_CALL WaitForPresent(SwapChain& swapChain) {
    return ((SwapChainVK&)swapChain).WaitForPresent();
}

static Result NRI_CALL QueuePresent(SwapChain& swapChain) {
    return ((SwapChainVK&)swapChain).Present();
}

static Result NRI_CALL GetDisplayDesc(SwapChain& swapChain, DisplayDesc& displayDesc) {
    return ((SwapChainVK&)swapChain).GetDisplayDesc(displayDesc);
}

Result DeviceVK::FillFunctionTable(SwapChainInterface& table) const {
    if (!m_Desc.isSwapChainSupported)
        return Result::UNSUPPORTED;

    table.CreateSwapChain = ::CreateSwapChain;
    table.DestroySwapChain = ::DestroySwapChain;
    table.GetSwapChainTextures = ::GetSwapChainTextures;
    table.AcquireNextSwapChainTexture = ::AcquireNextSwapChainTexture;
    table.WaitForPresent = ::WaitForPresent;
    table.QueuePresent = ::QueuePresent;
    table.GetDisplayDesc = ::GetDisplayDesc;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Upscaler  ]

static Result CreateUpscaler(Device& device, const UpscalerDesc& upscalerDesc, Upscaler*& upscaler) {
    DeviceVK& deviceVK = (DeviceVK&)device;
    UpscalerImpl* impl = Allocate<UpscalerImpl>(deviceVK.GetAllocationCallbacks(), device, deviceVK.GetCoreInterface());
    Result result = impl->Create(upscalerDesc);

    if (result != Result::SUCCESS) {
        Destroy(deviceVK.GetAllocationCallbacks(), impl);
        upscaler = nullptr;
    } else
        upscaler = (Upscaler*)impl;

    return result;
}

static void DestroyUpscaler(Upscaler& upscaler) {
    Destroy((UpscalerImpl*)&upscaler);
}

static bool IsUpscalerSupported(const Device& device, UpscalerType upscalerType) {
    DeviceVK& deviceVK = (DeviceVK&)device;

    return IsUpscalerSupported(deviceVK.GetDesc(), upscalerType);
}

static void GetUpscalerProps(const Upscaler& upscaler, UpscalerProps& upscalerProps) {
    UpscalerImpl& upscalerImpl = (UpscalerImpl&)upscaler;
    
    return upscalerImpl.GetUpscalerProps(upscalerProps);
}

static void CmdDispatchUpscale(CommandBuffer& commandBuffer, Upscaler& upscaler, const DispatchUpscaleDesc& dispatchUpscalerDesc) {
    UpscalerImpl& upscalerImpl = (UpscalerImpl&)upscaler;

    upscalerImpl.CmdDispatchUpscale(commandBuffer, dispatchUpscalerDesc);
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

static Result NRI_CALL CreateCommandAllocatorVK(Device& device, const CommandAllocatorVKDesc& commandAllocatorDesc, CommandAllocator*& commandAllocator) {
    return ((DeviceVK&)device).CreateImplementation<CommandAllocatorVK>(commandAllocator, commandAllocatorDesc);
}

static Result NRI_CALL CreateCommandBufferVK(Device& device, const CommandBufferVKDesc& commandBufferDesc, CommandBuffer*& commandBuffer) {
    return ((DeviceVK&)device).CreateImplementation<CommandBufferVK>(commandBuffer, commandBufferDesc);
}

static Result NRI_CALL CreateDescriptorPoolVK(Device& device, const DescriptorPoolVKDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceVK&)device).CreateImplementation<DescriptorPoolVK>(descriptorPool, descriptorPoolDesc);
}

static Result NRI_CALL CreateBufferVK(Device& device, const BufferVKDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceVK&)device).CreateImplementation<BufferVK>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTextureVK(Device& device, const TextureVKDesc& textureDesc, Texture*& texture) {
    return ((DeviceVK&)device).CreateImplementation<TextureVK>(texture, textureDesc);
}

static Result NRI_CALL CreateMemoryVK(Device& device, const MemoryVKDesc& memoryDesc, Memory*& memory) {
    return ((DeviceVK&)device).CreateImplementation<MemoryVK>(memory, memoryDesc);
}

static Result NRI_CALL CreateGraphicsPipelineVK(Device& device, VKNonDispatchableHandle vkPipeline, Pipeline*& pipeline) {
    return ((DeviceVK&)device).CreateImplementation<PipelineVK>(pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
}

static Result NRI_CALL CreateComputePipelineVK(Device& device, VKNonDispatchableHandle vkPipeline, Pipeline*& pipeline) {
    return ((DeviceVK&)device).CreateImplementation<PipelineVK>(pipeline, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline);
}

static Result NRI_CALL CreateQueryPoolVK(Device& device, const QueryPoolVKDesc& queryPoolDesc, QueryPool*& queryPool) {
    return ((DeviceVK&)device).CreateImplementation<QueryPoolVK>(queryPool, queryPoolDesc);
}

static Result NRI_CALL CreateAccelerationStructureVK(Device& device, const AccelerationStructureVKDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVK&)device).CreateImplementation<AccelerationStructureVK>(accelerationStructure, accelerationStructureDesc);
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
    table.CreateGraphicsPipelineVK = ::CreateGraphicsPipelineVK;
    table.CreateComputePipelineVK = ::CreateComputePipelineVK;
    table.CreateQueryPoolVK = ::CreateQueryPoolVK;
    table.CreateAccelerationStructureVK = ::CreateAccelerationStructureVK;
    table.GetQueueFamilyIndexVK = ::GetQueueFamilyIndexVK;
    table.GetPhysicalDeviceVK = ::GetPhysicalDeviceVK;
    table.GetInstanceVK = ::GetInstanceVK;
    table.GetDeviceProcAddrVK = ::GetDeviceProcAddrVK;
    table.GetInstanceProcAddrVK = ::GetInstanceProcAddrVK;

    return Result::SUCCESS;
}

#pragma endregion
