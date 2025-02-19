// © 2021 NVIDIA Corporation

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
#include "PipelineD3D12.h"
#include "PipelineLayoutD3D12.h"
#include "QueryPoolD3D12.h"
#include "QueueD3D12.h"
#include "SwapChainD3D12.h"
#include "TextureD3D12.h"

#include "HelperDataUpload.h"
#include "HelperDeviceMemoryAllocator.h"
#include "HelperWaitIdle.h"
#include "Streamer.h"
#include "Upscaler.h"

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
#include "PipelineD3D12.hpp"
#include "PipelineLayoutD3D12.hpp"
#include "QueryPoolD3D12.hpp"
#include "QueueD3D12.hpp"
#include "ResourceAllocatorD3D12.hpp"
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
    return ((const DeviceD3D12&)device).GetDesc();
}

static const BufferDesc& NRI_CALL GetBufferDesc(const Buffer& buffer) {
    return ((const BufferD3D12&)buffer).GetDesc();
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture& texture) {
    return ((const TextureD3D12&)texture).GetDesc();
}

static FormatSupportBits NRI_CALL GetFormatSupport(const Device& device, Format format) {
    return ((const DeviceD3D12&)device).GetFormatSupport(format);
}

static uint32_t NRI_CALL GetQuerySize(const QueryPool& queryPool) {
    return ((QueryPoolD3D12&)queryPool).GetQuerySize();
}

static void NRI_CALL GetBufferMemoryDesc(const Buffer& buffer, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const BufferD3D12& bufferD3D12 = (BufferD3D12&)buffer;

    D3D12_RESOURCE_DESC desc = {};
    GetResourceDesc(&desc, bufferD3D12.GetDesc());

    bufferD3D12.GetDevice().GetMemoryDesc(memoryLocation, desc, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc(const Texture& texture, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const TextureD3D12& textureD3D12 = (TextureD3D12&)texture;

    D3D12_RESOURCE_DESC desc = {};
    GetResourceDesc(&desc, textureD3D12.GetDesc());

    textureD3D12.GetDevice().GetMemoryDesc(memoryLocation, desc, memoryDesc);
}

static void NRI_CALL GetBufferMemoryDesc2(const Device& device, const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    D3D12_RESOURCE_DESC desc = {};
    GetResourceDesc(&desc, bufferDesc);

    ((const DeviceD3D12&)device).GetMemoryDesc(memoryLocation, desc, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc2(const Device& device, const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    D3D12_RESOURCE_DESC desc = {};
    GetResourceDesc(&desc, textureDesc);

    ((const DeviceD3D12&)device).GetMemoryDesc(memoryLocation, desc, memoryDesc);
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

static Result NRI_CALL CreateBuffer(Device& device, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceD3D12&)device).CreateImplementation<BufferD3D12>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTexture(Device& device, const TextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceD3D12&)device).CreateImplementation<TextureD3D12>(texture, textureDesc);
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
    DeviceD3D12& device = ((const BufferD3D12*)bufferViewDesc.buffer)->GetDevice();
    return device.CreateImplementation<DescriptorD3D12>(bufferView, bufferViewDesc);
}

static Result NRI_CALL CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D12& device = ((const TextureD3D12*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D12& device = ((const TextureD3D12*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D12& device = ((const TextureD3D12*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    Destroy((CommandAllocatorD3D12*)&commandAllocator);
}

static void NRI_CALL DestroyCommandBuffer(CommandBuffer& commandBuffer) {
    Destroy((CommandBufferD3D12*)&commandBuffer);
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    Destroy((DescriptorPoolD3D12*)&descriptorPool);
}

static void NRI_CALL DestroyBuffer(Buffer& buffer) {
    Destroy((BufferD3D12*)&buffer);
}

static void NRI_CALL DestroyTexture(Texture& texture) {
    Destroy((TextureD3D12*)&texture);
}

static void NRI_CALL DestroyDescriptor(Descriptor& descriptor) {
    Destroy((DescriptorD3D12*)&descriptor);
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
    Destroy((PipelineLayoutD3D12*)&pipelineLayout);
}

static void NRI_CALL DestroyPipeline(Pipeline& pipeline) {
    Destroy((PipelineD3D12*)&pipeline);
}

static void NRI_CALL DestroyQueryPool(QueryPool& queryPool) {
    Destroy((QueryPoolD3D12*)&queryPool);
}

static void NRI_CALL DestroyFence(Fence& fence) {
    Destroy((FenceD3D12*)&fence);
}

static Result NRI_CALL AllocateMemory(Device& device, const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    return ((DeviceD3D12&)device).CreateImplementation<MemoryD3D12>(memory, allocateMemoryDesc);
}

static Result NRI_CALL BindBufferMemory(Device& device, const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceD3D12&)device).BindBufferMemory(memoryBindingDescs, memoryBindingDescNum);
}

static Result NRI_CALL BindTextureMemory(Device& device, const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceD3D12&)device).BindTextureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static void NRI_CALL FreeMemory(Memory& memory) {
    Destroy((MemoryD3D12*)&memory);
}

static Result NRI_CALL BeginCommandBuffer(CommandBuffer& commandBuffer, const DescriptorPool* descriptorPool) {
    return ((CommandBufferD3D12&)commandBuffer).Begin(descriptorPool);
}

static void NRI_CALL CmdSetDescriptorPool(CommandBuffer& commandBuffer, const DescriptorPool& descriptorPool) {
    ((CommandBufferD3D12&)commandBuffer).SetDescriptorPool(descriptorPool);
}

static void NRI_CALL CmdSetPipelineLayout(CommandBuffer& commandBuffer, const PipelineLayout& pipelineLayout) {
    ((CommandBufferD3D12&)commandBuffer).SetPipelineLayout(pipelineLayout);
}

static void NRI_CALL CmdSetDescriptorSet(CommandBuffer& commandBuffer, uint32_t setIndex, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    ((CommandBufferD3D12&)commandBuffer).SetDescriptorSet(setIndex, descriptorSet, dynamicConstantBufferOffsets);
}

static void NRI_CALL CmdSetRootConstants(CommandBuffer& commandBuffer, uint32_t rootConstantIndex, const void* data, uint32_t size) {
    ((CommandBufferD3D12&)commandBuffer).SetRootConstants(rootConstantIndex, data, size);
}

static void NRI_CALL CmdSetRootDescriptor(CommandBuffer& commandBuffer, uint32_t rootDescriptorIndex, Descriptor& descriptor) {
    ((CommandBufferD3D12&)commandBuffer).SetRootDescriptor(rootDescriptorIndex, descriptor);
}

static void NRI_CALL CmdSetPipeline(CommandBuffer& commandBuffer, const Pipeline& pipeline) {
    ((CommandBufferD3D12&)commandBuffer).SetPipeline(pipeline);
}

static void NRI_CALL CmdBarrier(CommandBuffer& commandBuffer, const BarrierGroupDesc& barrierGroupDesc) {
    ((CommandBufferD3D12&)commandBuffer).Barrier(barrierGroupDesc);
}

static void NRI_CALL CmdSetIndexBuffer(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, IndexType indexType) {
    ((CommandBufferD3D12&)commandBuffer).SetIndexBuffer(buffer, offset, indexType);
}

static void NRI_CALL CmdSetVertexBuffers(CommandBuffer& commandBuffer, uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    ((CommandBufferD3D12&)commandBuffer).SetVertexBuffers(baseSlot, bufferNum, buffers, offsets);
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

static void NRI_CALL CmdCopyTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    ((CommandBufferD3D12&)commandBuffer).CopyTexture(dstTexture, dstRegionDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdResolveTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    ((CommandBufferD3D12&)commandBuffer).ResolveTexture(dstTexture, dstRegionDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdUploadBufferToTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    ((CommandBufferD3D12&)commandBuffer).UploadBufferToTexture(dstTexture, dstRegionDesc, srcBuffer, srcDataLayoutDesc);
}

static void NRI_CALL CmdReadbackTextureToBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    ((CommandBufferD3D12&)commandBuffer).ReadbackTextureToBuffer(dstBuffer, dstDataLayoutDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdClearStorageBuffer(CommandBuffer& commandBuffer, const ClearStorageBufferDesc& clearDesc) {
    ((CommandBufferD3D12&)commandBuffer).ClearStorageBuffer(clearDesc);
}

static void NRI_CALL CmdClearStorageTexture(CommandBuffer& commandBuffer, const ClearStorageTextureDesc& clearDesc) {
    ((CommandBufferD3D12&)commandBuffer).ClearStorageTexture(clearDesc);
}

static void NRI_CALL CmdResetQueries(CommandBuffer&, QueryPool&, uint32_t, uint32_t) {
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

static void NRI_CALL QueueSubmit(Queue& queue, const QueueSubmitDesc& queueSubmitDesc) {
    ((QueueD3D12&)queue).Submit(queueSubmitDesc);
}

static void NRI_CALL Wait(Fence& fence, uint64_t value) {
    ((FenceD3D12&)fence).Wait(value);
}

static uint64_t NRI_CALL GetFenceValue(Fence& fence) {
    return ((FenceD3D12&)fence).GetFenceValue();
}

static void NRI_CALL UpdateDescriptorRanges(DescriptorSet& descriptorSet, uint32_t baseRange, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    ((DescriptorSetD3D12&)descriptorSet).UpdateDescriptorRanges(baseRange, rangeNum, rangeUpdateDescs);
}

static void NRI_CALL UpdateDynamicConstantBuffers(DescriptorSet& descriptorSet, uint32_t baseDynamicConstantBuffer, uint32_t dynamicConstantBufferNum, const Descriptor* const* descriptors) {
    ((DescriptorSetD3D12&)descriptorSet).UpdateDynamicConstantBuffers(baseDynamicConstantBuffer, dynamicConstantBufferNum, descriptors);
}

static void NRI_CALL CopyDescriptorSet(DescriptorSet& descriptorSet, const DescriptorSetCopyDesc& descriptorSetCopyDesc) {
    ((DescriptorSetD3D12&)descriptorSet).Copy(descriptorSetCopyDesc);
}

static Result NRI_CALL AllocateDescriptorSets(DescriptorPool& descriptorPool, const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    return ((DescriptorPoolD3D12&)descriptorPool).AllocateDescriptorSets(pipelineLayout, setIndex, descriptorSets, instanceNum, variableDescriptorNum);
}

static void NRI_CALL ResetDescriptorPool(DescriptorPool& descriptorPool) {
    ((DescriptorPoolD3D12&)descriptorPool).Reset();
}

static void NRI_CALL ResetCommandAllocator(CommandAllocator& commandAllocator) {
    ((CommandAllocatorD3D12&)commandAllocator).Reset();
}

static void* NRI_CALL MapBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    return ((BufferD3D12&)buffer).Map(offset, size);
}

static void NRI_CALL UnmapBuffer(Buffer& buffer) {
    ((BufferD3D12&)buffer).Unmap();
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

    return ((DeviceD3D12&)device).GetNativeObject();
}

static void* NRI_CALL GetQueueNativeObject(const Queue& queue) {
    if (!(&queue))
        return nullptr;

    return (ID3D12CommandQueue*)((QueueD3D12&)queue);
}

static void* NRI_CALL GetCommandBufferNativeObject(const CommandBuffer& commandBuffer) {
    if (!(&commandBuffer))
        return nullptr;

    return (ID3D12GraphicsCommandList*)(CommandBufferD3D12&)commandBuffer;
}

static uint64_t NRI_CALL GetBufferNativeObject(const Buffer& buffer) {
    if (!(&buffer))
        return 0;

    return uint64_t((ID3D12Resource*)((BufferD3D12&)buffer));
}

static uint64_t NRI_CALL GetTextureNativeObject(const Texture& texture) {
    if (!(&texture))
        return 0;

    return uint64_t((ID3D12Resource*)((TextureD3D12&)texture));
}

static uint64_t NRI_CALL GetDescriptorNativeObject(const Descriptor& descriptor) {
    if (!(&descriptor))
        return 0;

    return uint64_t(((DescriptorD3D12&)descriptor).GetPointerCPU());
}

Result DeviceD3D12::FillFunctionTable(CoreInterface& table) const {
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
    QueueD3D12& queueD3D12 = (QueueD3D12&)queue;
    DeviceD3D12& deviceD3D12 = queueD3D12.GetDevice();
    HelperDataUpload helperDataUpload(deviceD3D12.GetCoreInterface(), (Device&)deviceD3D12, queue);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

static Result NRI_CALL WaitForIdle(Queue& queue) {
    if (!(&queue))
        return Result::SUCCESS;

    QueueD3D12& queueD3D12 = (QueueD3D12&)queue;
    DeviceD3D12& deviceD3D12 = queueD3D12.GetDevice();

    return WaitIdle(deviceD3D12.GetCoreInterface(), (Device&)deviceD3D12, queue);
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
    uint64_t luid = ((DeviceD3D12&)device).GetDesc().adapterDesc.luid;

    return QueryVideoMemoryInfoDXGI(luid, memoryLocation, videoMemoryInfo);
}

Result DeviceD3D12::FillFunctionTable(HelperInterface& table) const {
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
    return ((SwapChainD3D12&)swapChain).SetLatencySleepMode(latencySleepMode);
}

static Result SetLatencyMarker(SwapChain& swapChain, LatencyMarker latencyMarker) {
    return ((SwapChainD3D12&)swapChain).SetLatencyMarker(latencyMarker);
}

static Result LatencySleep(SwapChain& swapChain) {
    return ((SwapChainD3D12&)swapChain).LatencySleep();
}

static Result GetLatencyReport(const SwapChain& swapChain, LatencyReport& latencyReport) {
    return ((SwapChainD3D12&)swapChain).GetLatencyReport(latencyReport);
}

static void NRI_CALL QueueSubmitTrackable(Queue& queue, const QueueSubmitDesc& workSubmissionDesc, const SwapChain&) {
    ((QueueD3D12&)queue).Submit(workSubmissionDesc);
}

Result DeviceD3D12::FillFunctionTable(LowLatencyInterface& table) const {
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
    ((CommandBufferD3D12&)commandBuffer).DrawMeshTasks(drawMeshTasksDesc);
}

static void NRI_CALL CmdDrawMeshTasksIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferD3D12&)commandBuffer).DrawMeshTasksIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

Result DeviceD3D12::FillFunctionTable(MeshShaderInterface& table) const {
    if (!m_Desc.isMeshShaderSupported)
        return Result::UNSUPPORTED;

    table.CmdDrawMeshTasks = ::CmdDrawMeshTasks;
    table.CmdDrawMeshTasksIndirect = ::CmdDrawMeshTasksIndirect;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  RayTracing  ]

static Result NRI_CALL CreateAccelerationStructureDescriptor(const AccelerationStructure& accelerationStructure, Descriptor*& descriptor) {
    return ((AccelerationStructureD3D12&)accelerationStructure).CreateDescriptor(descriptor);
}

static uint64_t NRI_CALL GetAccelerationStructureUpdateScratchBufferSize(const AccelerationStructure& accelerationStructure) {
    return ((AccelerationStructureD3D12&)accelerationStructure).GetUpdateScratchBufferSize();
}

static uint64_t NRI_CALL GetAccelerationStructureBuildScratchBufferSize(const AccelerationStructure& accelerationStructure) {
    return ((AccelerationStructureD3D12&)accelerationStructure).GetBuildScratchBufferSize();
}

static uint64_t NRI_CALL GetAccelerationStructureHandle(const AccelerationStructure& accelerationStructure) {
    return ((AccelerationStructureD3D12&)accelerationStructure).GetHandle();
}

static Buffer* NRI_CALL GetAccelerationStructureBuffer(const AccelerationStructure& accelerationStructure) {
    return (Buffer*)((AccelerationStructureD3D12&)accelerationStructure).GetBuffer();
}

static uint64_t NRI_CALL GetAccelerationStructureNativeObject(const AccelerationStructure& accelerationStructure) {
    return uint64_t((ID3D12Resource*)((AccelerationStructureD3D12&)accelerationStructure));
}

static void NRI_CALL CmdBuildTopLevelAccelerationStructure(CommandBuffer& commandBuffer, uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {
    ((CommandBufferD3D12&)commandBuffer).BuildTopLevelAccelerationStructure(instanceNum, buffer, bufferOffset, flags, dst, scratch, scratchOffset);
}

static void NRI_CALL CmdBuildBottomLevelAccelerationStructure(CommandBuffer& commandBuffer, uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {
    ((CommandBufferD3D12&)commandBuffer).BuildBottomLevelAccelerationStructure(geometryObjectNum, geometryObjects, flags, dst, scratch, scratchOffset);
}

static void NRI_CALL CmdUpdateTopLevelAccelerationStructure(CommandBuffer& commandBuffer, uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, const AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {
    ((CommandBufferD3D12&)commandBuffer).UpdateTopLevelAccelerationStructure(instanceNum, buffer, bufferOffset, flags, dst, src, scratch, scratchOffset);
}

static void NRI_CALL CmdUpdateBottomLevelAccelerationStructure(CommandBuffer& commandBuffer, uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, const AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {
    ((CommandBufferD3D12&)commandBuffer).UpdateBottomLevelAccelerationStructure(geometryObjectNum, geometryObjects, flags, dst, src, scratch, scratchOffset);
}

static void NRI_CALL CmdCopyAccelerationStructure(CommandBuffer& commandBuffer, AccelerationStructure& dst, const AccelerationStructure& src, CopyMode copyMode) {
    ((CommandBufferD3D12&)commandBuffer).CopyAccelerationStructure(dst, src, copyMode);
}

static void NRI_CALL CmdWriteAccelerationStructureSize(CommandBuffer& commandBuffer, const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryOffset) {
    ((CommandBufferD3D12&)commandBuffer).WriteAccelerationStructureSize(accelerationStructures, accelerationStructureNum, queryPool, queryOffset);
}

static void NRI_CALL CmdDispatchRays(CommandBuffer& commandBuffer, const DispatchRaysDesc& dispatchRaysDesc) {
    ((CommandBufferD3D12&)commandBuffer).DispatchRays(dispatchRaysDesc);
}

static void NRI_CALL CmdDispatchRaysIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset) {
    ((CommandBufferD3D12&)commandBuffer).DispatchRaysIndirect(buffer, offset);
}

static void NRI_CALL GetAccelerationStructureMemoryDesc(const AccelerationStructure& accelerationStructure, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const AccelerationStructureD3D12& accelerationStructureD3D12 = (AccelerationStructureD3D12&)accelerationStructure;
    accelerationStructureD3D12.GetMemoryDesc(memoryLocation, memoryDesc);
}

static void NRI_CALL GetAccelerationStructureMemoryDesc2(const Device& device, const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((DeviceD3D12&)device).GetMemoryDesc(accelerationStructureDesc, memoryLocation, memoryDesc);
}

static Result NRI_CALL CreateRayTracingPipeline(Device& device, const RayTracingPipelineDesc& rayTracingPipelineDesc, Pipeline*& pipeline) {
    return ((DeviceD3D12&)device).CreateImplementation<PipelineD3D12>(pipeline, rayTracingPipelineDesc);
}

static Result NRI_CALL CreateAccelerationStructure(Device& device, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceD3D12&)device).CreateImplementation<AccelerationStructureD3D12>(accelerationStructure, accelerationStructureDesc);
}

static void NRI_CALL DestroyAccelerationStructure(AccelerationStructure& accelerationStructure) {
    Destroy((AccelerationStructureD3D12*)&accelerationStructure);
}

static Result NRI_CALL BindAccelerationStructureMemory(Device& device, const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceD3D12&)device).BindAccelerationStructureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static Result NRI_CALL WriteShaderGroupIdentifiers(const Pipeline& pipeline, uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* buffer) {
    return ((const PipelineD3D12&)pipeline).WriteShaderGroupIdentifiers(baseShaderGroupIndex, shaderGroupNum, buffer);
}

Result DeviceD3D12::FillFunctionTable(RayTracingInterface& table) const {
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
    return ((DeviceD3D12&)device).CreateImplementation<BufferD3D12>(buffer, bufferDesc);
}

static Result AllocateTexture(Device& device, const AllocateTextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceD3D12&)device).CreateImplementation<TextureD3D12>(texture, textureDesc);
}

static Result AllocateAccelerationStructure(Device& device, const AllocateAccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceD3D12&)device).CreateImplementation<AccelerationStructureD3D12>(accelerationStructure, accelerationStructureDesc);
}

Result DeviceD3D12::FillFunctionTable(ResourceAllocatorInterface& table) const {
    table.AllocateBuffer = ::AllocateBuffer;
    table.AllocateTexture = ::AllocateTexture;
    table.AllocateAccelerationStructure = ::AllocateAccelerationStructure;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Streamer  ]

static Result CreateStreamer(Device& device, const StreamerDesc& streamerDesc, Streamer*& streamer) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;
    StreamerImpl* impl = Allocate<StreamerImpl>(deviceD3D12.GetAllocationCallbacks(), device, deviceD3D12.GetCoreInterface());
    Result result = impl->Create(streamerDesc);

    if (result != Result::SUCCESS) {
        Destroy(deviceD3D12.GetAllocationCallbacks(), impl);
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

Result DeviceD3D12::FillFunctionTable(StreamerInterface& table) const {
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
    return ((DeviceD3D12&)device).CreateImplementation<SwapChainD3D12>(swapChain, swapChainDesc);
}

static void NRI_CALL DestroySwapChain(SwapChain& swapChain) {
    Destroy((SwapChainD3D12*)&swapChain);
}

static Texture* const* NRI_CALL GetSwapChainTextures(const SwapChain& swapChain, uint32_t& textureNum) {
    return ((SwapChainD3D12&)swapChain).GetTextures(textureNum);
}

static uint32_t NRI_CALL AcquireNextSwapChainTexture(SwapChain& swapChain) {
    return ((SwapChainD3D12&)swapChain).AcquireNextTexture();
}

static Result NRI_CALL WaitForPresent(SwapChain& swapChain) {
    return ((SwapChainD3D12&)swapChain).WaitForPresent();
}

static Result NRI_CALL QueuePresent(SwapChain& swapChain) {
    return ((SwapChainD3D12&)swapChain).Present();
}

static Result NRI_CALL GetDisplayDesc(SwapChain& swapChain, DisplayDesc& displayDesc) {
    return ((SwapChainD3D12&)swapChain).GetDisplayDesc(displayDesc);
}

Result DeviceD3D12::FillFunctionTable(SwapChainInterface& table) const {
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

static void DestroyUpscaler(Upscaler& upscaler) {
    Destroy((UpscalerImpl*)&upscaler);
}

static bool IsUpscalerSupported(const Device& device, UpscalerType upscalerType) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;

    return IsUpscalerSupported(deviceD3D12.GetDesc(), upscalerType);
}

static void GetUpscalerProps(const Upscaler& upscaler, UpscalerProps& upscalerProps) {
    UpscalerImpl& upscalerImpl = (UpscalerImpl&)upscaler;
    
    return upscalerImpl.GetUpscalerProps(upscalerProps);
}

static void CmdDispatchUpscale(CommandBuffer& commandBuffer, Upscaler& upscaler, const DispatchUpscaleDesc& dispatchUpscalerDesc) {
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

static Result NRI_CALL CreateCommandBufferD3D12(Device& device, const CommandBufferD3D12Desc& commandBufferDesc, CommandBuffer*& commandBuffer) {
    return ((DeviceD3D12&)device).CreateImplementation<CommandBufferD3D12>(commandBuffer, commandBufferDesc);
}

static Result NRI_CALL CreateDescriptorPoolD3D12(Device& device, const DescriptorPoolD3D12Desc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceD3D12&)device).CreateImplementation<DescriptorPoolD3D12>(descriptorPool, descriptorPoolDesc);
}

static Result NRI_CALL CreateBufferD3D12(Device& device, const BufferD3D12Desc& bufferDesc, Buffer*& buffer) {
    return ((DeviceD3D12&)device).CreateImplementation<BufferD3D12>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTextureD3D12(Device& device, const TextureD3D12Desc& textureDesc, Texture*& texture) {
    return ((DeviceD3D12&)device).CreateImplementation<TextureD3D12>(texture, textureDesc);
}

static Result NRI_CALL CreateMemoryD3D12(Device& device, const MemoryD3D12Desc& memoryDesc, Memory*& memory) {
    return ((DeviceD3D12&)device).CreateImplementation<MemoryD3D12>(memory, memoryDesc);
}

static Result NRI_CALL CreateAccelerationStructureD3D12(Device& device, const AccelerationStructureD3D12Desc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceD3D12&)device).CreateImplementation<AccelerationStructureD3D12>(accelerationStructure, accelerationStructureDesc);
}

Result DeviceD3D12::FillFunctionTable(WrapperD3D12Interface& table) const {
    table.CreateCommandBufferD3D12 = ::CreateCommandBufferD3D12;
    table.CreateDescriptorPoolD3D12 = ::CreateDescriptorPoolD3D12;
    table.CreateBufferD3D12 = ::CreateBufferD3D12;
    table.CreateTextureD3D12 = ::CreateTextureD3D12;
    table.CreateMemoryD3D12 = ::CreateMemoryD3D12;
    table.CreateAccelerationStructureD3D12 = ::CreateAccelerationStructureD3D12;

    return Result::SUCCESS;
}

#pragma endregion
