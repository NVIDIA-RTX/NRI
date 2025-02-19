// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"

#include "BufferD3D11.h"
#include "CommandAllocatorD3D11.h"
#include "CommandBufferD3D11.h"
#include "CommandBufferEmuD3D11.h"
#include "DescriptorD3D11.h"
#include "DescriptorPoolD3D11.h"
#include "DescriptorSetD3D11.h"
#include "FenceD3D11.h"
#include "MemoryD3D11.h"
#include "PipelineD3D11.h"
#include "PipelineLayoutD3D11.h"
#include "QueryPoolD3D11.h"
#include "QueueD3D11.h"
#include "SwapChainD3D11.h"
#include "TextureD3D11.h"

#include "HelperDataUpload.h"
#include "HelperDeviceMemoryAllocator.h"
#include "HelperWaitIdle.h"
#include "Streamer.h"
#include "Upscaler.h"

using namespace nri;

#include "BufferD3D11.hpp"
#include "CommandAllocatorD3D11.hpp"
#include "CommandBufferD3D11.hpp"
#include "CommandBufferEmuD3D11.hpp"
#include "DescriptorD3D11.hpp"
#include "DescriptorPoolD3D11.hpp"
#include "DescriptorSetD3D11.hpp"
#include "DeviceD3D11.hpp"
#include "FenceD3D11.hpp"
#include "PipelineD3D11.hpp"
#include "PipelineLayoutD3D11.hpp"
#include "QueryPoolD3D11.hpp"
#include "QueueD3D11.hpp"
#include "SharedD3D11.hpp"
#include "SwapChainD3D11.hpp"
#include "TextureD3D11.hpp"

Result CreateDeviceD3D11(const DeviceCreationDesc& desc, const DeviceCreationD3D11Desc& descD3D11, DeviceBase*& device) {
    DeviceD3D11* impl = Allocate<DeviceD3D11>(desc.allocationCallbacks, desc.callbackInterface, desc.allocationCallbacks);
    Result result = impl->Create(desc, descD3D11);

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
    return ((const DeviceD3D11&)device).GetDesc();
}

static const BufferDesc& NRI_CALL GetBufferDesc(const Buffer& buffer) {
    return ((const BufferD3D11&)buffer).GetDesc();
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture& texture) {
    return ((const TextureD3D11&)texture).GetDesc();
}

static FormatSupportBits NRI_CALL GetFormatSupport(const Device& device, Format format) {
    return ((const DeviceD3D11&)device).GetFormatSupport(format);
}

static uint32_t NRI_CALL GetQuerySize(const QueryPool& queryPool) {
    return ((QueryPoolD3D11&)queryPool).GetQuerySize();
}

static void NRI_CALL GetBufferMemoryDesc(const Buffer& buffer, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const BufferD3D11& bufferD3D11 = (BufferD3D11&)buffer;
    bufferD3D11.GetDevice().GetMemoryDesc(bufferD3D11.GetDesc(), memoryLocation, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc(const Texture& texture, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const TextureD3D11& textureD3D11 = (TextureD3D11&)texture;
    textureD3D11.GetDevice().GetMemoryDesc(textureD3D11.GetDesc(), memoryLocation, memoryDesc);
}

static void NRI_CALL GetBufferMemoryDesc2(const Device& device, const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((const DeviceD3D11&)device).GetMemoryDesc(bufferDesc, memoryLocation, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc2(const Device& device, const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((const DeviceD3D11&)device).GetMemoryDesc(textureDesc, memoryLocation, memoryDesc);
}

static Result NRI_CALL GetQueue(Device& device, QueueType queueType, uint32_t queueIndex, Queue*& queue) {
    return ((DeviceD3D11&)device).GetQueue(queueType, queueIndex, queue);
}

static Result NRI_CALL CreateCommandAllocator(Queue& queue, CommandAllocator*& commandAllocator) {
    DeviceD3D11& device = ((QueueD3D11&)queue).GetDevice();
    return device.CreateCommandAllocator(queue, commandAllocator);
}

static Result NRI_CALL CreateCommandBuffer(CommandAllocator& commandAllocator, CommandBuffer*& commandBuffer) {
    return ((CommandAllocatorD3D11&)commandAllocator).CreateCommandBuffer(commandBuffer);
}

static Result NRI_CALL CreateFence(Device& device, uint64_t initialValue, Fence*& fence) {
    return ((DeviceD3D11&)device).CreateImplementation<FenceD3D11>(fence, initialValue);
}

static Result NRI_CALL CreateDescriptorPool(Device& device, const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceD3D11&)device).CreateImplementation<DescriptorPoolD3D11>(descriptorPool, descriptorPoolDesc);
}

static Result NRI_CALL CreateBuffer(Device& device, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceD3D11&)device).CreateImplementation<BufferD3D11>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTexture(Device& device, const TextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceD3D11&)device).CreateImplementation<TextureD3D11>(texture, textureDesc);
}

static Result NRI_CALL CreatePipelineLayout(Device& device, const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    return ((DeviceD3D11&)device).CreateImplementation<PipelineLayoutD3D11>(pipelineLayout, pipelineLayoutDesc);
}

static Result NRI_CALL CreateGraphicsPipeline(Device& device, const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    return ((DeviceD3D11&)device).CreateImplementation<PipelineD3D11>(pipeline, graphicsPipelineDesc);
}

static Result NRI_CALL CreateComputePipeline(Device& device, const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    return ((DeviceD3D11&)device).CreateImplementation<PipelineD3D11>(pipeline, computePipelineDesc);
}

static Result NRI_CALL CreateQueryPool(Device& device, const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    return ((DeviceD3D11&)device).CreateImplementation<QueryPoolD3D11>(queryPool, queryPoolDesc);
}

static Result NRI_CALL CreateSampler(Device& device, const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    return ((DeviceD3D11&)device).CreateImplementation<DescriptorD3D11>(sampler, samplerDesc);
}

static Result NRI_CALL CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    DeviceD3D11& device = ((const BufferD3D11*)bufferViewDesc.buffer)->GetDevice();
    return device.CreateImplementation<DescriptorD3D11>(bufferView, bufferViewDesc);
}

static Result NRI_CALL CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D11& device = ((const TextureD3D11*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D11>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D11& device = ((const TextureD3D11*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D11>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D11& device = ((const TextureD3D11*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D11>(textureView, textureViewDesc);
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    Destroy((CommandAllocatorD3D11*)&commandAllocator);
}

static void NRI_CALL DestroyCommandBuffer(CommandBuffer& commandBuffer) {
    if (!(&commandBuffer))
        return;

    CommandBufferBase& commandBufferBase = (CommandBufferBase&)commandBuffer;
    Destroy(commandBufferBase.GetAllocationCallbacks(), &commandBufferBase);
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    Destroy((DescriptorPoolD3D11*)&descriptorPool);
}

static void NRI_CALL DestroyBuffer(Buffer& buffer) {
    Destroy((BufferD3D11*)&buffer);
}

static void NRI_CALL DestroyTexture(Texture& texture) {
    Destroy((TextureD3D11*)&texture);
}

static void NRI_CALL DestroyDescriptor(Descriptor& descriptor) {
    Destroy((DescriptorD3D11*)&descriptor);
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
    Destroy((PipelineLayoutD3D11*)&pipelineLayout);
}

static void NRI_CALL DestroyPipeline(Pipeline& pipeline) {
    Destroy((PipelineD3D11*)&pipeline);
}

static void NRI_CALL DestroyQueryPool(QueryPool& queryPool) {
    Destroy((QueryPoolD3D11*)&queryPool);
}

static void NRI_CALL DestroyFence(Fence& fence) {
    Destroy((FenceD3D11*)&fence);
}

static Result NRI_CALL AllocateMemory(Device& device, const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    return ((DeviceD3D11&)device).CreateImplementation<MemoryD3D11>(memory, allocateMemoryDesc);
}

static Result NRI_CALL BindBufferMemory(Device& device, const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceD3D11&)device).BindBufferMemory(memoryBindingDescs, memoryBindingDescNum);
}

static Result NRI_CALL BindTextureMemory(Device& device, const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceD3D11&)device).BindTextureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static void NRI_CALL FreeMemory(Memory& memory) {
    Destroy((MemoryD3D11*)&memory);
}

static Result NRI_CALL BeginCommandBuffer(CommandBuffer& commandBuffer, const DescriptorPool* descriptorPool) {
    return ((CommandBufferD3D11&)commandBuffer).Begin(descriptorPool);
}

static void NRI_CALL CmdSetDescriptorPool(CommandBuffer& commandBuffer, const DescriptorPool& descriptorPool) {
    ((CommandBufferD3D11&)commandBuffer).SetDescriptorPool(descriptorPool);
}

static void NRI_CALL CmdSetPipelineLayout(CommandBuffer& commandBuffer, const PipelineLayout& pipelineLayout) {
    ((CommandBufferD3D11&)commandBuffer).SetPipelineLayout(pipelineLayout);
}

static void NRI_CALL CmdSetDescriptorSet(CommandBuffer& commandBuffer, uint32_t setIndex, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    ((CommandBufferD3D11&)commandBuffer).SetDescriptorSet(setIndex, descriptorSet, dynamicConstantBufferOffsets);
}

static void NRI_CALL CmdSetRootConstants(CommandBuffer& commandBuffer, uint32_t rootConstantIndex, const void* data, uint32_t size) {
    ((CommandBufferD3D11&)commandBuffer).SetRootConstants(rootConstantIndex, data, size);
}

static void NRI_CALL CmdSetRootDescriptor(CommandBuffer& commandBuffer, uint32_t rootDescriptorIndex, Descriptor& descriptor) {
    ((CommandBufferD3D11&)commandBuffer).SetRootDescriptor(rootDescriptorIndex, descriptor);
}

static void NRI_CALL CmdSetPipeline(CommandBuffer& commandBuffer, const Pipeline& pipeline) {
    ((CommandBufferD3D11&)commandBuffer).SetPipeline(pipeline);
}

static void NRI_CALL CmdBarrier(CommandBuffer& commandBuffer, const BarrierGroupDesc& barrierGroupDesc) {
    ((CommandBufferD3D11&)commandBuffer).Barrier(barrierGroupDesc);
}

static void NRI_CALL CmdSetIndexBuffer(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, IndexType indexType) {
    ((CommandBufferD3D11&)commandBuffer).SetIndexBuffer(buffer, offset, indexType);
}

static void NRI_CALL CmdSetVertexBuffers(CommandBuffer& commandBuffer, uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    ((CommandBufferD3D11&)commandBuffer).SetVertexBuffers(baseSlot, bufferNum, buffers, offsets);
}

static void NRI_CALL CmdSetViewports(CommandBuffer& commandBuffer, const Viewport* viewports, uint32_t viewportNum) {
    ((CommandBufferD3D11&)commandBuffer).SetViewports(viewports, viewportNum);
}

static void NRI_CALL CmdSetScissors(CommandBuffer& commandBuffer, const Rect* rects, uint32_t rectNum) {
    ((CommandBufferD3D11&)commandBuffer).SetScissors(rects, rectNum);
}

static void NRI_CALL CmdSetStencilReference(CommandBuffer& commandBuffer, uint8_t frontRef, uint8_t backRef) {
    ((CommandBufferD3D11&)commandBuffer).SetStencilReference(frontRef, backRef);
}

static void NRI_CALL CmdSetDepthBounds(CommandBuffer& commandBuffer, float boundsMin, float boundsMax) {
    ((CommandBufferD3D11&)commandBuffer).SetDepthBounds(boundsMin, boundsMax);
}

static void NRI_CALL CmdSetBlendConstants(CommandBuffer& commandBuffer, const Color32f& color) {
    ((CommandBufferD3D11&)commandBuffer).SetBlendConstants(color);
}

static void NRI_CALL CmdSetSampleLocations(CommandBuffer& commandBuffer, const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    ((CommandBufferD3D11&)commandBuffer).SetSampleLocations(locations, locationNum, sampleNum);
}

static void NRI_CALL CmdSetShadingRate(CommandBuffer&, const ShadingRateDesc&) {
}

static void NRI_CALL CmdSetDepthBias(CommandBuffer&, const DepthBiasDesc&) {
}

static void NRI_CALL CmdBeginRendering(CommandBuffer& commandBuffer, const AttachmentsDesc& attachmentsDesc) {
    ((CommandBufferD3D11&)commandBuffer).BeginRendering(attachmentsDesc);
}

static void NRI_CALL CmdClearAttachments(CommandBuffer& commandBuffer, const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    ((CommandBufferD3D11&)commandBuffer).ClearAttachments(clearDescs, clearDescNum, rects, rectNum);
}

static void NRI_CALL CmdDraw(CommandBuffer& commandBuffer, const DrawDesc& drawDesc) {
    ((CommandBufferD3D11&)commandBuffer).Draw(drawDesc);
}

static void NRI_CALL CmdDrawIndexed(CommandBuffer& commandBuffer, const DrawIndexedDesc& drawIndexedDesc) {
    ((CommandBufferD3D11&)commandBuffer).DrawIndexed(drawIndexedDesc);
}

static void NRI_CALL CmdDrawIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferD3D11&)commandBuffer).DrawIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL CmdDrawIndexedIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferD3D11&)commandBuffer).DrawIndexedIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL CmdEndRendering(CommandBuffer& commandBuffer) {
    ((CommandBufferD3D11&)commandBuffer).ResetAttachments();
}

static void NRI_CALL CmdDispatch(CommandBuffer& commandBuffer, const DispatchDesc& dispatchDesc) {
    ((CommandBufferD3D11&)commandBuffer).Dispatch(dispatchDesc);
}

static void NRI_CALL CmdDispatchIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset) {
    ((CommandBufferD3D11&)commandBuffer).DispatchIndirect(buffer, offset);
}

static void NRI_CALL CmdCopyBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    ((CommandBufferD3D11&)commandBuffer).CopyBuffer(dstBuffer, dstOffset, srcBuffer, srcOffset, size);
}

static void NRI_CALL CmdCopyTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    ((CommandBufferD3D11&)commandBuffer).CopyTexture(dstTexture, dstRegionDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdResolveTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    ((CommandBufferD3D11&)commandBuffer).ResolveTexture(dstTexture, dstRegionDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdUploadBufferToTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    ((CommandBufferD3D11&)commandBuffer).UploadBufferToTexture(dstTexture, dstRegionDesc, srcBuffer, srcDataLayoutDesc);
}

static void NRI_CALL CmdReadbackTextureToBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    ((CommandBufferD3D11&)commandBuffer).ReadbackTextureToBuffer(dstBuffer, dstDataLayoutDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdClearStorageBuffer(CommandBuffer& commandBuffer, const ClearStorageBufferDesc& clearDesc) {
    ((CommandBufferD3D11&)commandBuffer).ClearStorageBuffer(clearDesc);
}

static void NRI_CALL CmdClearStorageTexture(CommandBuffer& commandBuffer, const ClearStorageTextureDesc& clearDesc) {
    ((CommandBufferD3D11&)commandBuffer).ClearStorageTexture(clearDesc);
}

static void NRI_CALL CmdResetQueries(CommandBuffer&, QueryPool&, uint32_t, uint32_t) {
}

static void NRI_CALL CmdBeginQuery(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferD3D11&)commandBuffer).BeginQuery(queryPool, offset);
}

static void NRI_CALL CmdEndQuery(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferD3D11&)commandBuffer).EndQuery(queryPool, offset);
}

static void NRI_CALL CmdCopyQueries(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    ((CommandBufferD3D11&)commandBuffer).CopyQueries(queryPool, offset, num, dstBuffer, dstOffset);
}

static void NRI_CALL CmdBeginAnnotation(CommandBuffer& commandBuffer, const char* name, uint32_t bgra) {
    MaybeUnused(commandBuffer, name, bgra);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferD3D11&)commandBuffer).BeginAnnotation(name, bgra);
#endif
}

static void NRI_CALL CmdEndAnnotation(CommandBuffer& commandBuffer) {
    MaybeUnused(commandBuffer);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferD3D11&)commandBuffer).EndAnnotation();
#endif
}

static void NRI_CALL CmdAnnotation(CommandBuffer& commandBuffer, const char* name, uint32_t bgra) {
    MaybeUnused(commandBuffer, name, bgra);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferD3D11&)commandBuffer).Annotation(name, bgra);
#endif
}

static Result NRI_CALL EndCommandBuffer(CommandBuffer& commandBuffer) {
    return ((CommandBufferD3D11&)commandBuffer).End();
}

static void NRI_CALL QueueBeginAnnotation(Queue&, const char*, uint32_t) {
}

static void NRI_CALL QueueEndAnnotation(Queue&) {
}

static void NRI_CALL QueueAnnotation(Queue&, const char*, uint32_t) {
}

static void NRI_CALL ResetQueries(QueryPool&, uint32_t, uint32_t) {
}

static void NRI_CALL QueueSubmit(Queue& queue, const QueueSubmitDesc& queueSubmitDesc) {
    ((QueueD3D11&)queue).Submit(queueSubmitDesc);
}

static void NRI_CALL Wait(Fence& fence, uint64_t value) {
    ((FenceD3D11&)fence).Wait(value);
}

static uint64_t NRI_CALL GetFenceValue(Fence& fence) {
    return ((FenceD3D11&)fence).GetFenceValue();
}

static void NRI_CALL UpdateDescriptorRanges(DescriptorSet& descriptorSet, uint32_t baseRange, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    ((DescriptorSetD3D11&)descriptorSet).UpdateDescriptorRanges(baseRange, rangeNum, rangeUpdateDescs);
}

static void NRI_CALL UpdateDynamicConstantBuffers(DescriptorSet& descriptorSet, uint32_t baseDynamicConstantBuffer, uint32_t dynamicConstantBufferNum, const Descriptor* const* descriptors) {
    ((DescriptorSetD3D11&)descriptorSet).UpdateDynamicConstantBuffers(baseDynamicConstantBuffer, dynamicConstantBufferNum, descriptors);
}

static void NRI_CALL CopyDescriptorSet(DescriptorSet& descriptorSet, const DescriptorSetCopyDesc& descriptorSetCopyDesc) {
    ((DescriptorSetD3D11&)descriptorSet).Copy(descriptorSetCopyDesc);
}

static Result NRI_CALL AllocateDescriptorSets(DescriptorPool& descriptorPool, const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    return ((DescriptorPoolD3D11&)descriptorPool).AllocateDescriptorSets(pipelineLayout, setIndex, descriptorSets, instanceNum, variableDescriptorNum);
}

static void NRI_CALL ResetDescriptorPool(DescriptorPool& descriptorPool) {
    ((DescriptorPoolD3D11&)descriptorPool).Reset();
}

static void NRI_CALL ResetCommandAllocator(CommandAllocator& commandAllocator) {
    ((CommandAllocatorD3D11&)commandAllocator).Reset();
}

static void* NRI_CALL MapBuffer(Buffer& buffer, uint64_t offset, uint64_t) {
    return ((BufferD3D11&)buffer).Map(offset);
}

static void NRI_CALL UnmapBuffer(Buffer& buffer) {
    ((BufferD3D11&)buffer).Unmap();
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

    return ((DeviceD3D11&)device).GetNativeObject();
}

static void* NRI_CALL GetQueueNativeObject(const Queue&) {
    return nullptr;
}

static void* NRI_CALL GetCommandBufferNativeObject(const CommandBuffer& commandBuffer) {
    if (!(&commandBuffer))
        return nullptr;

    return ((CommandBufferD3D11&)commandBuffer).GetNativeObject();
}

static uint64_t NRI_CALL GetBufferNativeObject(const Buffer& buffer) {
    if (!(&buffer))
        return 0;

    return uint64_t((ID3D11Buffer*)((BufferD3D11&)buffer));
}

static uint64_t NRI_CALL GetDescriptorNativeObject(const Descriptor& descriptor) {
    if (!(&descriptor))
        return 0;

    return uint64_t((ID3D11View*)((DescriptorD3D11&)descriptor));
}

static uint64_t NRI_CALL GetTextureNativeObject(const Texture& texture) {
    if (!(&texture))
        return 0;

    return uint64_t((ID3D11Resource*)((TextureD3D11&)texture));
}

// Command buffer emulation

static Result NRI_CALL EmuBeginCommandBuffer(CommandBuffer& commandBuffer, const DescriptorPool* descriptorPool) {
    return ((CommandBufferEmuD3D11&)commandBuffer).Begin(descriptorPool);
}

static void NRI_CALL EmuCmdSetDescriptorPool(CommandBuffer&, const DescriptorPool&) {
}

static void NRI_CALL EmuCmdSetPipelineLayout(CommandBuffer& commandBuffer, const PipelineLayout& pipelineLayout) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetPipelineLayout(pipelineLayout);
}

static void NRI_CALL EmuCmdSetDescriptorSet(CommandBuffer& commandBuffer, uint32_t setIndex, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetDescriptorSet(setIndex, descriptorSet, dynamicConstantBufferOffsets);
}

static void NRI_CALL EmuSetRootConstants(CommandBuffer& commandBuffer, uint32_t rootConstantIndex, const void* data, uint32_t size) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetRootConstants(rootConstantIndex, data, size);
}

static void NRI_CALL EmuCmdSetRootDescriptor(CommandBuffer& commandBuffer, uint32_t rootDescriptorIndex, Descriptor& descriptor) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetRootDescriptor(rootDescriptorIndex, descriptor);
}

static void NRI_CALL EmuCmdSetPipeline(CommandBuffer& commandBuffer, const Pipeline& pipeline) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetPipeline(pipeline);
}

static void NRI_CALL EmuCmdBarrier(CommandBuffer& commandBuffer, const BarrierGroupDesc& barrierGroupDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).Barrier(barrierGroupDesc);
}

static void NRI_CALL EmuCmdSetIndexBuffer(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, IndexType indexType) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetIndexBuffer(buffer, offset, indexType);
}

static void NRI_CALL EmuCmdSetVertexBuffers(CommandBuffer& commandBuffer, uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetVertexBuffers(baseSlot, bufferNum, buffers, offsets);
}

static void NRI_CALL EmuCmdSetViewports(CommandBuffer& commandBuffer, const Viewport* viewports, uint32_t viewportNum) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetViewports(viewports, viewportNum);
}

static void NRI_CALL EmuCmdSetScissors(CommandBuffer& commandBuffer, const Rect* rects, uint32_t rectNum) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetScissors(rects, rectNum);
}

static void NRI_CALL EmuCmdSetStencilReference(CommandBuffer& commandBuffer, uint8_t frontRef, uint8_t backRef) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetStencilReference(frontRef, backRef);
}

static void NRI_CALL EmuCmdSetDepthBounds(CommandBuffer& commandBuffer, float boundsMin, float boundsMax) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetDepthBounds(boundsMin, boundsMax);
}

static void NRI_CALL EmuCmdSetBlendConstants(CommandBuffer& commandBuffer, const Color32f& color) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetBlendConstants(color);
}

static void NRI_CALL EmuCmdSetSampleLocations(CommandBuffer& commandBuffer, const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    ((CommandBufferEmuD3D11&)commandBuffer).SetSampleLocations(locations, locationNum, sampleNum);
}

static void NRI_CALL EmuCmdSetShadingRate(CommandBuffer&, const ShadingRateDesc&) {
}

static void NRI_CALL EmuCmdSetDepthBias(CommandBuffer&, const DepthBiasDesc&) {
}

static void NRI_CALL EmuCmdBeginRendering(CommandBuffer& commandBuffer, const AttachmentsDesc& attachmentsDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).BeginRendering(attachmentsDesc);
}

static void NRI_CALL EmuCmdClearAttachments(CommandBuffer& commandBuffer, const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    ((CommandBufferEmuD3D11&)commandBuffer).ClearAttachments(clearDescs, clearDescNum, rects, rectNum);
}

static void NRI_CALL EmuCmdDraw(CommandBuffer& commandBuffer, const DrawDesc& drawDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).Draw(drawDesc);
}

static void NRI_CALL EmuCmdDrawIndexed(CommandBuffer& commandBuffer, const DrawIndexedDesc& drawIndexedDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).DrawIndexed(drawIndexedDesc);
}

static void NRI_CALL EmuCmdDrawIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferEmuD3D11&)commandBuffer).DrawIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL EmuCmdDrawIndexedIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferEmuD3D11&)commandBuffer).DrawIndexedIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL EmuCmdEndRendering(CommandBuffer& commandBuffer) {
    ((CommandBufferEmuD3D11&)commandBuffer).EndRendering();
}

static void NRI_CALL EmuCmdDispatch(CommandBuffer& commandBuffer, const DispatchDesc& dispatchDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).Dispatch(dispatchDesc);
}

static void NRI_CALL EmuCmdDispatchIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset) {
    ((CommandBufferEmuD3D11&)commandBuffer).DispatchIndirect(buffer, offset);
}

static void NRI_CALL EmuCmdCopyBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    ((CommandBufferEmuD3D11&)commandBuffer).CopyBuffer(dstBuffer, dstOffset, srcBuffer, srcOffset, size);
}

static void NRI_CALL EmuCmdCopyTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).CopyTexture(dstTexture, dstRegionDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL EmuCmdResolveTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).ResolveTexture(dstTexture, dstRegionDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL EmuCmdUploadBufferToTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).UploadBufferToTexture(dstTexture, dstRegionDesc, srcBuffer, srcDataLayoutDesc);
}

static void NRI_CALL EmuCmdReadbackTextureToBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).ReadbackTextureToBuffer(dstBuffer, dstDataLayoutDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL EmuCmdClearStorageBuffer(CommandBuffer& commandBuffer, const ClearStorageBufferDesc& clearDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).ClearStorageBuffer(clearDesc);
}

static void NRI_CALL EmuCmdClearStorageTexture(CommandBuffer& commandBuffer, const ClearStorageTextureDesc& clearDesc) {
    ((CommandBufferEmuD3D11&)commandBuffer).ClearStorageTexture(clearDesc);
}

static void NRI_CALL EmuCmdResetQueries(CommandBuffer&, QueryPool&, uint32_t, uint32_t) {
}

static void NRI_CALL EmuCmdBeginQuery(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferEmuD3D11&)commandBuffer).BeginQuery(queryPool, offset);
}

static void NRI_CALL EmuCmdEndQuery(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferEmuD3D11&)commandBuffer).EndQuery(queryPool, offset);
}

static void NRI_CALL EmuCmdCopyQueries(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    ((CommandBufferEmuD3D11&)commandBuffer).CopyQueries(queryPool, offset, num, dstBuffer, dstOffset);
}

static void NRI_CALL EmuCmdBeginAnnotation(CommandBuffer& commandBuffer, const char* name, uint32_t bgra) {
    MaybeUnused(commandBuffer, name, bgra);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferEmuD3D11&)commandBuffer).BeginAnnotation(name, bgra);
#endif
}

static void NRI_CALL EmuCmdEndAnnotation(CommandBuffer& commandBuffer) {
    MaybeUnused(commandBuffer);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferEmuD3D11&)commandBuffer).EndAnnotation();
#endif
}

static void NRI_CALL EmuCmdAnnotation(CommandBuffer& commandBuffer, const char* name, uint32_t bgra) {
    MaybeUnused(commandBuffer, name, bgra);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferEmuD3D11&)commandBuffer).Annotation(name, bgra);
#endif
}

static Result NRI_CALL EmuEndCommandBuffer(CommandBuffer& commandBuffer) {
    return ((CommandBufferEmuD3D11&)commandBuffer).End();
}

static void* NRI_CALL EmuGetCommandBufferNativeObject(const CommandBuffer& commandBuffer) {
    if (!(&commandBuffer))
        return nullptr;

    CommandBufferBase& commandBufferBase = (CommandBufferBase&)commandBuffer;
    return commandBufferBase.GetNativeObject();
}

Result DeviceD3D11::FillFunctionTable(CoreInterface& table) const {
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
    table.GetBufferNativeObject = ::GetBufferNativeObject;
    table.GetTextureNativeObject = ::GetTextureNativeObject;
    table.GetDescriptorNativeObject = ::GetDescriptorNativeObject;

    if (m_IsDeferredContextEmulated) {
        table.BeginCommandBuffer = ::EmuBeginCommandBuffer;
        table.CmdSetDescriptorPool = ::EmuCmdSetDescriptorPool;
        table.CmdSetDescriptorSet = ::EmuCmdSetDescriptorSet;
        table.CmdSetPipelineLayout = ::EmuCmdSetPipelineLayout;
        table.CmdSetPipeline = ::EmuCmdSetPipeline;
        table.CmdSetRootConstants = ::EmuSetRootConstants;
        table.CmdSetRootDescriptor = ::EmuCmdSetRootDescriptor;
        table.CmdBarrier = ::EmuCmdBarrier;
        table.CmdSetIndexBuffer = ::EmuCmdSetIndexBuffer;
        table.CmdSetVertexBuffers = ::EmuCmdSetVertexBuffers;
        table.CmdSetViewports = ::EmuCmdSetViewports;
        table.CmdSetScissors = ::EmuCmdSetScissors;
        table.CmdSetStencilReference = ::EmuCmdSetStencilReference;
        table.CmdSetDepthBounds = ::EmuCmdSetDepthBounds;
        table.CmdSetBlendConstants = ::EmuCmdSetBlendConstants;
        table.CmdSetSampleLocations = ::EmuCmdSetSampleLocations;
        table.CmdSetShadingRate = ::EmuCmdSetShadingRate;
        table.CmdSetDepthBias = ::EmuCmdSetDepthBias;
        table.CmdBeginRendering = ::EmuCmdBeginRendering;
        table.CmdClearAttachments = ::EmuCmdClearAttachments;
        table.CmdDraw = ::EmuCmdDraw;
        table.CmdDrawIndexed = ::EmuCmdDrawIndexed;
        table.CmdDrawIndirect = ::EmuCmdDrawIndirect;
        table.CmdDrawIndexedIndirect = ::EmuCmdDrawIndexedIndirect;
        table.CmdEndRendering = ::EmuCmdEndRendering;
        table.CmdDispatch = ::EmuCmdDispatch;
        table.CmdDispatchIndirect = ::EmuCmdDispatchIndirect;
        table.CmdCopyBuffer = ::EmuCmdCopyBuffer;
        table.CmdCopyTexture = ::EmuCmdCopyTexture;
        table.CmdUploadBufferToTexture = ::EmuCmdUploadBufferToTexture;
        table.CmdReadbackTextureToBuffer = ::EmuCmdReadbackTextureToBuffer;
        table.CmdClearStorageBuffer = ::EmuCmdClearStorageBuffer;
        table.CmdClearStorageTexture = ::EmuCmdClearStorageTexture;
        table.CmdResolveTexture = ::EmuCmdResolveTexture;
        table.CmdResetQueries = ::EmuCmdResetQueries;
        table.CmdBeginQuery = ::EmuCmdBeginQuery;
        table.CmdEndQuery = ::EmuCmdEndQuery;
        table.CmdCopyQueries = ::EmuCmdCopyQueries;
        table.CmdBeginAnnotation = ::EmuCmdBeginAnnotation;
        table.CmdEndAnnotation = ::EmuCmdEndAnnotation;
        table.CmdAnnotation = ::EmuCmdAnnotation;
        table.EndCommandBuffer = ::EmuEndCommandBuffer;
        table.GetCommandBufferNativeObject = ::EmuGetCommandBufferNativeObject;
    } else {
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
        table.GetCommandBufferNativeObject = ::GetCommandBufferNativeObject;
    }

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Helper  ]

static Result NRI_CALL UploadData(Queue& queue, const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    QueueD3D11& queueD3D11 = (QueueD3D11&)queue;
    DeviceD3D11& deviceD3D11 = queueD3D11.GetDevice();
    HelperDataUpload helperDataUpload(deviceD3D11.GetCoreInterface(), (Device&)deviceD3D11, queue);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

static Result NRI_CALL WaitForIdle(Queue& queue) {
    if (!(&queue))
        return Result::SUCCESS;

    QueueD3D11& queueD3D11 = (QueueD3D11&)queue;
    DeviceD3D11& deviceD3D11 = queueD3D11.GetDevice();

    return WaitIdle(deviceD3D11.GetCoreInterface(), (Device&)deviceD3D11, queue);
}

static uint32_t NRI_CALL CalculateAllocationNumber(const Device& device, const ResourceGroupDesc& resourceGroupDesc) {
    DeviceD3D11& deviceD3D11 = (DeviceD3D11&)device;
    HelperDeviceMemoryAllocator allocator(deviceD3D11.GetCoreInterface(), (Device&)device);

    return allocator.CalculateAllocationNumber(resourceGroupDesc);
}

static Result NRI_CALL AllocateAndBindMemory(Device& device, const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    DeviceD3D11& deviceD3D11 = (DeviceD3D11&)device;
    HelperDeviceMemoryAllocator allocator(deviceD3D11.GetCoreInterface(), device);

    return allocator.AllocateAndBindMemory(resourceGroupDesc, allocations);
}

static Result NRI_CALL QueryVideoMemoryInfo(const Device& device, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) {
    uint64_t luid = ((DeviceD3D11&)device).GetDesc().adapterDesc.luid;

    return QueryVideoMemoryInfoDXGI(luid, memoryLocation, videoMemoryInfo);
}

Result DeviceD3D11::FillFunctionTable(HelperInterface& table) const {
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

static Result SetLatencySleepMode(SwapChain& swapChain, const LatencySleepMode& latencySleepMode) {
    return ((SwapChainD3D11&)swapChain).SetLatencySleepMode(latencySleepMode);
}

static Result SetLatencyMarker(SwapChain& swapChain, LatencyMarker latencyMarker) {
    return ((SwapChainD3D11&)swapChain).SetLatencyMarker(latencyMarker);
}

static Result LatencySleep(SwapChain& swapChain) {
    return ((SwapChainD3D11&)swapChain).LatencySleep();
}

static Result GetLatencyReport(const SwapChain& swapChain, LatencyReport& latencyReport) {
    return ((SwapChainD3D11&)swapChain).GetLatencyReport(latencyReport);
}

static void NRI_CALL QueueSubmitTrackable(Queue& queue, const QueueSubmitDesc& workSubmissionDesc, const SwapChain&) {
    ((QueueD3D11&)queue).Submit(workSubmissionDesc);
}

Result DeviceD3D11::FillFunctionTable(LowLatencyInterface& table) const {
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
#pragma region[  ResourceAllocator  ]

static Result AllocateBuffer(Device& device, const AllocateBufferDesc& bufferDesc, Buffer*& buffer) {
    Result result = ((DeviceD3D11&)device).CreateImplementation<BufferD3D11>(buffer, bufferDesc.desc);
    if (result == Result::SUCCESS) {
        result = ((BufferD3D11*)buffer)->Create(bufferDesc.memoryLocation, bufferDesc.memoryPriority);
        if (result != Result::SUCCESS) {
            Destroy(((DeviceD3D11&)device).GetAllocationCallbacks(), (BufferD3D11*)buffer);
            buffer = nullptr;
        }
    }

    return result;
}

static Result AllocateTexture(Device& device, const AllocateTextureDesc& textureDesc, Texture*& texture) {
    Result result = ((DeviceD3D11&)device).CreateImplementation<TextureD3D11>(texture, textureDesc.desc);
    if (result == Result::SUCCESS) {
        result = ((TextureD3D11*)texture)->Create(textureDesc.memoryLocation, textureDesc.memoryPriority);
        if (result != Result::SUCCESS) {
            Destroy(((DeviceD3D11&)device).GetAllocationCallbacks(), (TextureD3D11*)texture);
            texture = nullptr;
        }
    }

    return result;
}

static Result AllocateAccelerationStructure(Device&, const AllocateAccelerationStructureDesc&, AccelerationStructure*& accelerationStructure) {
    accelerationStructure = nullptr;

    return Result::UNSUPPORTED;
}

Result DeviceD3D11::FillFunctionTable(ResourceAllocatorInterface& table) const {
    table.AllocateBuffer = ::AllocateBuffer;
    table.AllocateTexture = ::AllocateTexture;
    table.AllocateAccelerationStructure = ::AllocateAccelerationStructure;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Streamer  ]

static Result CreateStreamer(Device& device, const StreamerDesc& streamerDesc, Streamer*& streamer) {
    DeviceD3D11& deviceD3D11 = (DeviceD3D11&)device;
    StreamerImpl* impl = Allocate<StreamerImpl>(deviceD3D11.GetAllocationCallbacks(), device, deviceD3D11.GetCoreInterface());
    Result result = impl->Create(streamerDesc);

    if (result != Result::SUCCESS) {
        Destroy(deviceD3D11.GetAllocationCallbacks(), impl);
        streamer = nullptr;
    } else
        streamer = (Streamer*)impl;

    return result;
}

static void DestroyStreamer(Streamer& streamer) {
    Destroy((StreamerImpl*)&streamer);
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

Result DeviceD3D11::FillFunctionTable(StreamerInterface& table) const {
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
    return ((DeviceD3D11&)device).CreateImplementation<SwapChainD3D11>(swapChain, swapChainDesc);
}

static void NRI_CALL DestroySwapChain(SwapChain& swapChain) {
    Destroy((SwapChainD3D11*)&swapChain);
}

static Texture* const* NRI_CALL GetSwapChainTextures(const SwapChain& swapChain, uint32_t& textureNum) {
    return ((SwapChainD3D11&)swapChain).GetTextures(textureNum);
}

static uint32_t NRI_CALL AcquireNextSwapChainTexture(SwapChain& swapChain) {
    return ((SwapChainD3D11&)swapChain).AcquireNextTexture();
}

static Result NRI_CALL WaitForPresent(SwapChain& swapChain) {
    return ((SwapChainD3D11&)swapChain).WaitForPresent();
}

static Result NRI_CALL QueuePresent(SwapChain& swapChain) {
    return ((SwapChainD3D11&)swapChain).Present();
}

static Result NRI_CALL GetDisplayDesc(SwapChain& swapChain, DisplayDesc& displayDesc) {
    return ((SwapChainD3D11&)swapChain).GetDisplayDesc(displayDesc);
}

Result DeviceD3D11::FillFunctionTable(SwapChainInterface& table) const {
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
    DeviceD3D11& deviceD3D11 = (DeviceD3D11&)device;
    UpscalerImpl* impl = Allocate<UpscalerImpl>(deviceD3D11.GetAllocationCallbacks(), device, deviceD3D11.GetCoreInterface());
    Result result = impl->Create(upscalerDesc);

    if (result != Result::SUCCESS) {
        Destroy(deviceD3D11.GetAllocationCallbacks(), impl);
        upscaler = nullptr;
    } else
        upscaler = (Upscaler*)impl;

    return result;
}

static void DestroyUpscaler(Upscaler& upscaler) {
    Destroy((UpscalerImpl*)&upscaler);
}

static bool IsUpscalerSupported(const Device& device, UpscalerType upscalerType) {
    DeviceD3D11& deviceD3D11 = (DeviceD3D11&)device;

    return IsUpscalerSupported(deviceD3D11.GetDesc(), upscalerType);
}

static void GetUpscalerProps(const Upscaler& upscaler, UpscalerProps& upscalerProps) {
    UpscalerImpl& upscalerImpl = (UpscalerImpl&)upscaler;
    
    return upscalerImpl.GetUpscalerProps(upscalerProps);
}

static void CmdDispatchUpscale(CommandBuffer& commandBuffer, Upscaler& upscaler, const DispatchUpscaleDesc& dispatchUpscalerDesc) {
    UpscalerImpl& upscalerImpl = (UpscalerImpl&)upscaler;

    upscalerImpl.CmdDispatchUpscale(commandBuffer, dispatchUpscalerDesc);
}

Result DeviceD3D11::FillFunctionTable(UpscalerInterface& table) const {
    table.CreateUpscaler = ::CreateUpscaler;
    table.DestroyUpscaler = ::DestroyUpscaler;
    table.IsUpscalerSupported = ::IsUpscalerSupported;
    table.GetUpscalerProps = ::GetUpscalerProps;
    table.CmdDispatchUpscale = ::CmdDispatchUpscale;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  WrapperD3D11  ]

static Result NRI_CALL CreateCommandBufferD3D11(Device& device, const CommandBufferD3D11Desc& commandBufferDesc, CommandBuffer*& commandBuffer) {
    DeviceD3D11& deviceD3D11 = (DeviceD3D11&)device;

    return ::CreateCommandBuffer(deviceD3D11, commandBufferDesc.d3d11DeviceContext, commandBuffer);
}

static Result NRI_CALL CreateBufferD3D11(Device& device, const BufferD3D11Desc& bufferDesc, Buffer*& buffer) {
    return ((DeviceD3D11&)device).CreateImplementation<BufferD3D11>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTextureD3D11(Device& device, const TextureD3D11Desc& textureDesc, Texture*& texture) {
    return ((DeviceD3D11&)device).CreateImplementation<TextureD3D11>(texture, textureDesc);
}

Result DeviceD3D11::FillFunctionTable(WrapperD3D11Interface& table) const {
    table.CreateCommandBufferD3D11 = ::CreateCommandBufferD3D11;
    table.CreateTextureD3D11 = ::CreateTextureD3D11;
    table.CreateBufferD3D11 = ::CreateBufferD3D11;

    return Result::SUCCESS;
}

#pragma endregion
