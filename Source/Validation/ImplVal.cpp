// © 2021 NVIDIA Corporation

#include "SharedVal.h"

#include "AccelerationStructureVal.h"
#include "BufferVal.h"
#include "CommandAllocatorVal.h"
#include "CommandBufferVal.h"
#include "DescriptorPoolVal.h"
#include "DescriptorSetVal.h"
#include "DescriptorVal.h"
#include "DeviceVal.h"
#include "FenceVal.h"
#include "MemoryVal.h"
#include "PipelineLayoutVal.h"
#include "PipelineVal.h"
#include "QueryPoolVal.h"
#include "QueueVal.h"
#include "SwapChainVal.h"
#include "TextureVal.h"

#include "HelperDataUpload.h"
#include "HelperDeviceMemoryAllocator.h"
#include "HelperWaitIdle.h"
#include "Streamer.h"
#include "Upscaler.h"

using namespace nri;

#include "AccelerationStructureVal.hpp"
#include "BufferVal.hpp"
#include "CommandAllocatorVal.hpp"
#include "CommandBufferVal.hpp"
#include "ConversionVal.hpp"
#include "DescriptorPoolVal.hpp"
#include "DescriptorSetVal.hpp"
#include "DescriptorVal.hpp"
#include "DeviceVal.hpp"
#include "FenceVal.hpp"
#include "MemoryVal.hpp"
#include "PipelineLayoutVal.hpp"
#include "PipelineVal.hpp"
#include "QueryPoolVal.hpp"
#include "QueueVal.hpp"
#include "SwapChainVal.hpp"
#include "TextureVal.hpp"

DeviceBase* CreateDeviceValidation(const DeviceCreationDesc& desc, DeviceBase& device) {
    DeviceVal* deviceVal = Allocate<DeviceVal>(desc.allocationCallbacks, desc.callbackInterface, desc.allocationCallbacks, device);

    if (!deviceVal->Create()) {
        Destroy(desc.allocationCallbacks, deviceVal);
        return nullptr;
    }

    return deviceVal;
}

//============================================================================================================================================================================================
#pragma region[  Core  ]

static const DeviceDesc& NRI_CALL GetDeviceDesc(const Device& device) {
    return ((const DeviceVal&)device).GetDesc();
}

static const BufferDesc& NRI_CALL GetBufferDesc(const Buffer& buffer) {
    return ((const BufferVal&)buffer).GetDesc();
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture& texture) {
    return ((const TextureVal&)texture).GetDesc();
}

static FormatSupportBits NRI_CALL GetFormatSupport(const Device& device, Format format) {
    return ((const DeviceVal&)device).GetFormatSupport(format);
}

static uint32_t NRI_CALL GetQuerySize(const QueryPool& queryPool) {
    return ((QueryPoolVal&)queryPool).GetQuerySize();
}

static void NRI_CALL GetBufferMemoryDesc(const Buffer& buffer, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const BufferVal& bufferVal = (BufferVal&)buffer;
    DeviceVal& deviceVal = bufferVal.GetDevice();

    deviceVal.GetCoreInterface().GetBufferMemoryDesc(*bufferVal.GetImpl(), memoryLocation, memoryDesc);
    deviceVal.RegisterMemoryType(memoryDesc.type, memoryLocation);
}

static void NRI_CALL GetTextureMemoryDesc(const Texture& texture, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const TextureVal& bufferVal = (TextureVal&)texture;
    DeviceVal& deviceVal = bufferVal.GetDevice();

    deviceVal.GetCoreInterface().GetTextureMemoryDesc(*bufferVal.GetImpl(), memoryLocation, memoryDesc);
    deviceVal.RegisterMemoryType(memoryDesc.type, memoryLocation);
}

static void NRI_CALL GetBufferMemoryDesc2(const Device& device, const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    DeviceVal& deviceVal = (DeviceVal&)device;
    deviceVal.GetCoreInterface().GetBufferMemoryDesc2(deviceVal.GetImpl(), bufferDesc, memoryLocation, memoryDesc);
    deviceVal.RegisterMemoryType(memoryDesc.type, memoryLocation);
}

static void NRI_CALL GetTextureMemoryDesc2(const Device& device, const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    DeviceVal& deviceVal = (DeviceVal&)device;
    deviceVal.GetCoreInterface().GetTextureMemoryDesc2(deviceVal.GetImpl(), textureDesc, memoryLocation, memoryDesc);
    deviceVal.RegisterMemoryType(memoryDesc.type, memoryLocation);
}

static Result NRI_CALL GetQueue(Device& device, QueueType queueType, uint32_t queueIndex, Queue*& queue) {
    return ((DeviceVal&)device).GetQueue(queueType, queueIndex, queue);
}

static Result NRI_CALL CreateCommandAllocator(Queue& queue, CommandAllocator*& commandAllocator) {
    return GetDeviceVal(queue).CreateCommandAllocator(queue, commandAllocator);
}

static Result NRI_CALL CreateCommandBuffer(CommandAllocator& commandAllocator, CommandBuffer*& commandBuffer) {
    return ((CommandAllocatorVal&)commandAllocator).CreateCommandBuffer(commandBuffer);
}

static Result NRI_CALL CreateFence(Device& device, uint64_t initialValue, Fence*& fence) {
    return ((DeviceVal&)device).CreateFence(initialValue, fence);
}

static Result NRI_CALL CreateDescriptorPool(Device& device, const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceVal&)device).CreateDescriptorPool(descriptorPoolDesc, descriptorPool);
}

static Result NRI_CALL CreateBuffer(Device& device, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceVal&)device).CreateBuffer(bufferDesc, buffer);
}

static Result NRI_CALL CreateTexture(Device& device, const TextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceVal&)device).CreateTexture(textureDesc, texture);
}

static Result NRI_CALL CreatePipelineLayout(Device& device, const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    return ((DeviceVal&)device).CreatePipelineLayout(pipelineLayoutDesc, pipelineLayout);
}

static Result NRI_CALL CreateGraphicsPipeline(Device& device, const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    return ((DeviceVal&)device).CreatePipeline(graphicsPipelineDesc, pipeline);
}

static Result NRI_CALL CreateComputePipeline(Device& device, const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    return ((DeviceVal&)device).CreatePipeline(computePipelineDesc, pipeline);
}

static Result NRI_CALL CreateQueryPool(Device& device, const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    return ((DeviceVal&)device).CreateQueryPool(queryPoolDesc, queryPool);
}

static Result NRI_CALL CreateSampler(Device& device, const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    return ((DeviceVal&)device).CreateDescriptor(samplerDesc, sampler);
}

static Result NRI_CALL CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    DeviceVal& device = GetDeviceVal(*bufferViewDesc.buffer);

    return device.CreateDescriptor(bufferViewDesc, bufferView);
}

static Result NRI_CALL CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceVal& device = GetDeviceVal(*textureViewDesc.texture);

    return device.CreateDescriptor(textureViewDesc, textureView);
}

static Result NRI_CALL CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceVal& device = GetDeviceVal(*textureViewDesc.texture);

    return device.CreateDescriptor(textureViewDesc, textureView);
}

static Result NRI_CALL CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceVal& device = GetDeviceVal(*textureViewDesc.texture);

    return device.CreateDescriptor(textureViewDesc, textureView);
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    if (!(&commandAllocator))
        return;

    GetDeviceVal(commandAllocator).DestroyCommandAllocator(commandAllocator);
}

static void NRI_CALL DestroyCommandBuffer(CommandBuffer& commandBuffer) {
    if (!(&commandBuffer))
        return;

    GetDeviceVal(commandBuffer).DestroyCommandBuffer(commandBuffer);
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    if (!(&descriptorPool))
        return;

    GetDeviceVal(descriptorPool).DestroyDescriptorPool(descriptorPool);
}

static void NRI_CALL DestroyBuffer(Buffer& buffer) {
    if (!(&buffer))
        return;

    GetDeviceVal(buffer).DestroyBuffer(buffer);
}

static void NRI_CALL DestroyTexture(Texture& texture) {
    if (!(&texture))
        return;

    GetDeviceVal(texture).DestroyTexture(texture);
}

static void NRI_CALL DestroyDescriptor(Descriptor& descriptor) {
    if (!(&descriptor))
        return;

    GetDeviceVal(descriptor).DestroyDescriptor(descriptor);
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
    if (!(&pipelineLayout))
        return;

    GetDeviceVal(pipelineLayout).DestroyPipelineLayout(pipelineLayout);
}

static void NRI_CALL DestroyPipeline(Pipeline& pipeline) {
    if (!(&pipeline))
        return;

    GetDeviceVal(pipeline).DestroyPipeline(pipeline);
}

static void NRI_CALL DestroyQueryPool(QueryPool& queryPool) {
    if (!(&queryPool))
        return;

    GetDeviceVal(queryPool).DestroyQueryPool(queryPool);
}

static void NRI_CALL DestroyFence(Fence& fence) {
    if (!(&fence))
        return;

    GetDeviceVal(fence).DestroyFence(fence);
}

static Result NRI_CALL AllocateMemory(Device& device, const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    return ((DeviceVal&)device).AllocateMemory(allocateMemoryDesc, memory);
}

static Result NRI_CALL BindBufferMemory(Device& device, const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceVal&)device).BindBufferMemory(memoryBindingDescs, memoryBindingDescNum);
}

static Result NRI_CALL BindTextureMemory(Device& device, const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceVal&)device).BindTextureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static void NRI_CALL FreeMemory(Memory& memory) {
    if (!(&memory))
        return;

    GetDeviceVal(memory).FreeMemory(memory);
}

static Result NRI_CALL BeginCommandBuffer(CommandBuffer& commandBuffer, const DescriptorPool* descriptorPool) {
    return ((CommandBufferVal&)commandBuffer).Begin(descriptorPool);
}

static void NRI_CALL CmdSetDescriptorPool(CommandBuffer& commandBuffer, const DescriptorPool& descriptorPool) {
    ((CommandBufferVal&)commandBuffer).SetDescriptorPool(descriptorPool);
}

static void NRI_CALL CmdSetPipelineLayout(CommandBuffer& commandBuffer, const PipelineLayout& pipelineLayout) {
    ((CommandBufferVal&)commandBuffer).SetPipelineLayout(pipelineLayout);
}

static void NRI_CALL CmdSetDescriptorSet(CommandBuffer& commandBuffer, uint32_t setIndex, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    ((CommandBufferVal&)commandBuffer).SetDescriptorSet(setIndex, descriptorSet, dynamicConstantBufferOffsets);
}

static void NRI_CALL CmdSetRootConstants(CommandBuffer& commandBuffer, uint32_t rootConstantIndex, const void* data, uint32_t size) {
    ((CommandBufferVal&)commandBuffer).SetRootConstants(rootConstantIndex, data, size);
}

static void NRI_CALL CmdSetRootDescriptor(CommandBuffer& commandBuffer, uint32_t rootDescriptorIndex, Descriptor& descriptor) {
    ((CommandBufferVal&)commandBuffer).SetRootDescriptor(rootDescriptorIndex, descriptor);
}

static void NRI_CALL CmdSetPipeline(CommandBuffer& commandBuffer, const Pipeline& pipeline) {
    ((CommandBufferVal&)commandBuffer).SetPipeline(pipeline);
}

static void NRI_CALL CmdBarrier(CommandBuffer& commandBuffer, const BarrierGroupDesc& barrierGroupDesc) {
    ((CommandBufferVal&)commandBuffer).Barrier(barrierGroupDesc);
}

static void NRI_CALL CmdSetIndexBuffer(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, IndexType indexType) {
    ((CommandBufferVal&)commandBuffer).SetIndexBuffer(buffer, offset, indexType);
}

static void NRI_CALL CmdSetVertexBuffers(CommandBuffer& commandBuffer, uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    ((CommandBufferVal&)commandBuffer).SetVertexBuffers(baseSlot, bufferNum, buffers, offsets);
}

static void NRI_CALL CmdSetViewports(CommandBuffer& commandBuffer, const Viewport* viewports, uint32_t viewportNum) {
    ((CommandBufferVal&)commandBuffer).SetViewports(viewports, viewportNum);
}

static void NRI_CALL CmdSetScissors(CommandBuffer& commandBuffer, const Rect* rects, uint32_t rectNum) {
    ((CommandBufferVal&)commandBuffer).SetScissors(rects, rectNum);
}

static void NRI_CALL CmdSetStencilReference(CommandBuffer& commandBuffer, uint8_t frontRef, uint8_t backRef) {
    ((CommandBufferVal&)commandBuffer).SetStencilReference(frontRef, backRef);
}

static void NRI_CALL CmdSetDepthBounds(CommandBuffer& commandBuffer, float boundsMin, float boundsMax) {
    ((CommandBufferVal&)commandBuffer).SetDepthBounds(boundsMin, boundsMax);
}

static void NRI_CALL CmdSetBlendConstants(CommandBuffer& commandBuffer, const Color32f& color) {
    ((CommandBufferVal&)commandBuffer).SetBlendConstants(color);
}

static void NRI_CALL CmdSetSampleLocations(CommandBuffer& commandBuffer, const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    ((CommandBufferVal&)commandBuffer).SetSampleLocations(locations, locationNum, sampleNum);
}

static void NRI_CALL CmdSetShadingRate(CommandBuffer& commandBuffer, const ShadingRateDesc& shadingRateDesc) {
    ((CommandBufferVal&)commandBuffer).SetShadingRate(shadingRateDesc);
}

static void NRI_CALL CmdSetDepthBias(CommandBuffer& commandBuffer, const DepthBiasDesc& depthBiasDesc) {
    ((CommandBufferVal&)commandBuffer).SetDepthBias(depthBiasDesc);
}

static void NRI_CALL CmdBeginRendering(CommandBuffer& commandBuffer, const AttachmentsDesc& attachmentsDesc) {
    ((CommandBufferVal&)commandBuffer).BeginRendering(attachmentsDesc);
}

static void NRI_CALL CmdClearAttachments(CommandBuffer& commandBuffer, const ClearDesc* clearDescs, uint32_t clearDescNum, const Rect* rects, uint32_t rectNum) {
    ((CommandBufferVal&)commandBuffer).ClearAttachments(clearDescs, clearDescNum, rects, rectNum);
}

static void NRI_CALL CmdDraw(CommandBuffer& commandBuffer, const DrawDesc& drawDesc) {
    ((CommandBufferVal&)commandBuffer).Draw(drawDesc);
}

static void NRI_CALL CmdDrawIndexed(CommandBuffer& commandBuffer, const DrawIndexedDesc& drawIndexedDesc) {
    ((CommandBufferVal&)commandBuffer).DrawIndexed(drawIndexedDesc);
}

static void NRI_CALL CmdDrawIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferVal&)commandBuffer).DrawIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL CmdDrawIndexedIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferVal&)commandBuffer).DrawIndexedIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL CmdEndRendering(CommandBuffer& commandBuffer) {
    ((CommandBufferVal&)commandBuffer).EndRendering();
}

static void NRI_CALL CmdDispatch(CommandBuffer& commandBuffer, const DispatchDesc& dispatchDesc) {
    ((CommandBufferVal&)commandBuffer).Dispatch(dispatchDesc);
}

static void NRI_CALL CmdDispatchIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset) {
    ((CommandBufferVal&)commandBuffer).DispatchIndirect(buffer, offset);
}

static void NRI_CALL CmdCopyBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    ((CommandBufferVal&)commandBuffer).CopyBuffer(dstBuffer, dstOffset, srcBuffer, srcOffset, size);
}

static void NRI_CALL CmdCopyTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    ((CommandBufferVal&)commandBuffer).CopyTexture(dstTexture, dstRegionDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdResolveTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    ((CommandBufferVal&)commandBuffer).ResolveTexture(dstTexture, dstRegionDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdUploadBufferToTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    ((CommandBufferVal&)commandBuffer).UploadBufferToTexture(dstTexture, dstRegionDesc, srcBuffer, srcDataLayoutDesc);
}

static void NRI_CALL CmdReadbackTextureToBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    ((CommandBufferVal&)commandBuffer).ReadbackTextureToBuffer(dstBuffer, dstDataLayoutDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdClearStorageBuffer(CommandBuffer& commandBuffer, const ClearStorageBufferDesc& clearDesc) {
    ((CommandBufferVal&)commandBuffer).ClearStorageBuffer(clearDesc);
}

static void NRI_CALL CmdClearStorageTexture(CommandBuffer& commandBuffer, const ClearStorageTextureDesc& clearDesc) {
    ((CommandBufferVal&)commandBuffer).ClearStorageTexture(clearDesc);
}

static void NRI_CALL CmdResetQueries(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset, uint32_t num) {
    ((CommandBufferVal&)commandBuffer).ResetQueries(queryPool, offset, num);
}

static void NRI_CALL CmdBeginQuery(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferVal&)commandBuffer).BeginQuery(queryPool, offset);
}

static void NRI_CALL CmdEndQuery(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferVal&)commandBuffer).EndQuery(queryPool, offset);
}

static void NRI_CALL CmdCopyQueries(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    ((CommandBufferVal&)commandBuffer).CopyQueries(queryPool, offset, num, dstBuffer, dstOffset);
}

static void NRI_CALL CmdBeginAnnotation(CommandBuffer& commandBuffer, const char* name, uint32_t bgra) {
    ((CommandBufferVal&)commandBuffer).BeginAnnotation(name, bgra);
}

static void NRI_CALL CmdEndAnnotation(CommandBuffer& commandBuffer) {
    ((CommandBufferVal&)commandBuffer).EndAnnotation();
}

static void NRI_CALL CmdAnnotation(CommandBuffer& commandBuffer, const char* name, uint32_t bgra) {
    ((CommandBufferVal&)commandBuffer).Annotation(name, bgra);
}

static Result NRI_CALL EndCommandBuffer(CommandBuffer& commandBuffer) {
    return ((CommandBufferVal&)commandBuffer).End();
}

static void NRI_CALL QueueBeginAnnotation(Queue& queue, const char* name, uint32_t bgra) {
    ((QueueVal&)queue).BeginAnnotation(name, bgra);
}

static void NRI_CALL QueueEndAnnotation(Queue& queue) {
    ((QueueVal&)queue).EndAnnotation();
}

static void NRI_CALL QueueAnnotation(Queue& queue, const char* name, uint32_t bgra) {
    ((QueueVal&)queue).Annotation(name, bgra);
}

static void NRI_CALL ResetQueries(QueryPool& queryPool, uint32_t offset, uint32_t num) {
    ((QueryPoolVal&)queryPool).ResetQueries(offset, num);
}

static void NRI_CALL QueueSubmit(Queue& queue, const QueueSubmitDesc& queueSubmitDesc) {
    ((QueueVal&)queue).Submit(queueSubmitDesc, nullptr);
}

static void NRI_CALL Wait(Fence& fence, uint64_t value) {
    ((FenceVal&)fence).Wait(value);
}

static uint64_t NRI_CALL GetFenceValue(Fence& fence) {
    return ((FenceVal&)fence).GetFenceValue();
}

static void NRI_CALL UpdateDescriptorRanges(DescriptorSet& descriptorSet, uint32_t baseRange, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    ((DescriptorSetVal&)descriptorSet).UpdateDescriptorRanges(baseRange, rangeNum, rangeUpdateDescs);
}

static void NRI_CALL UpdateDynamicConstantBuffers(DescriptorSet& descriptorSet, uint32_t baseDynamicConstantBuffer, uint32_t dynamicConstantBufferNum, const Descriptor* const* descriptors) {
    ((DescriptorSetVal&)descriptorSet).UpdateDynamicConstantBuffers(baseDynamicConstantBuffer, dynamicConstantBufferNum, descriptors);
}

static void NRI_CALL CopyDescriptorSet(DescriptorSet& descriptorSet, const DescriptorSetCopyDesc& descriptorSetCopyDesc) {
    ((DescriptorSetVal&)descriptorSet).Copy(descriptorSetCopyDesc);
}

static Result NRI_CALL AllocateDescriptorSets(DescriptorPool& descriptorPool, const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    return ((DescriptorPoolVal&)descriptorPool).AllocateDescriptorSets(pipelineLayout, setIndex, descriptorSets, instanceNum, variableDescriptorNum);
}

static void NRI_CALL ResetDescriptorPool(DescriptorPool& descriptorPool) {
    ((DescriptorPoolVal&)descriptorPool).Reset();
}

static void NRI_CALL ResetCommandAllocator(CommandAllocator& commandAllocator) {
    ((CommandAllocatorVal&)commandAllocator).Reset();
}

static void* NRI_CALL MapBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    return ((BufferVal&)buffer).Map(offset, size);
}

static void NRI_CALL UnmapBuffer(Buffer& buffer) {
    ((BufferVal&)buffer).Unmap();
}

static void NRI_CALL SetDebugName(Object* object, const char* name) {
    if (object) {
        CHECK(((uint64_t*)object)[1] == NRI_OBJECT_SIGNATURE, "Invalid NRI object!");
        ((DebugNameBaseVal*)object)->SetDebugName(name);
    }
}

static void* NRI_CALL GetDeviceNativeObject(const Device& device) {
    if (!(&device))
        return nullptr;

    return ((DeviceVal&)device).GetNativeObject();
}

static void* NRI_CALL GetQueueNativeObject(const Queue& queue) {
    if (!(&queue))
        return nullptr;

    return ((QueueVal&)queue).GetNativeObject();
}

static void* NRI_CALL GetCommandBufferNativeObject(const CommandBuffer& commandBuffer) {
    if (!(&commandBuffer))
        return nullptr;

    return ((CommandBufferVal&)commandBuffer).GetNativeObject();
}

static uint64_t NRI_CALL GetBufferNativeObject(const Buffer& buffer) {
    if (!(&buffer))
        return 0;

    return ((BufferVal&)buffer).GetNativeObject();
}

static uint64_t NRI_CALL GetTextureNativeObject(const Texture& texture) {
    if (!(&texture))
        return 0;

    return ((TextureVal&)texture).GetNativeObject();
}

static uint64_t NRI_CALL GetDescriptorNativeObject(const Descriptor& descriptor) {
    if (!(&descriptor))
        return 0;

    return ((DescriptorVal&)descriptor).GetNativeObject();
}

Result DeviceVal::FillFunctionTable(CoreInterface& table) const {
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

static bool ValidateTextureUploadDesc(DeviceVal& device, uint32_t i, const TextureUploadDesc& textureUploadDesc) {
    if (!textureUploadDesc.subresources) {
        REPORT_WARNING(&device, "the number of subresources in 'textureUploadDescs[%u]' is 0 (nothing to upload)", i);
        return true;
    }

    const TextureVal& textureVal = *(TextureVal*)textureUploadDesc.texture;
    const TextureDesc& textureDesc = textureVal.GetDesc();

    RETURN_ON_FAILURE(&device, textureUploadDesc.texture != nullptr, false, "'textureUploadDescs[%u].texture' is NULL", i);
    RETURN_ON_FAILURE(&device, textureUploadDesc.after.layout < Layout::MAX_NUM, false, "'textureUploadDescs[%u].nextLayout' is invalid", i);
    RETURN_ON_FAILURE(&device, textureVal.IsBoundToMemory(), false, "'textureUploadDescs[%u].texture' is not bound to memory", i);

    uint32_t subresourceNum = (uint32_t)textureDesc.layerNum * (uint32_t)textureDesc.mipNum;
    for (uint32_t j = 0; j < subresourceNum; j++) {
        const TextureSubresourceUploadDesc& subresource = textureUploadDesc.subresources[j];

        if (subresource.sliceNum == 0) {
            REPORT_WARNING(&device, "No data to upload: the number of subresources in 'textureUploadDescs[%u].subresources[%u].sliceNum' is 0", i, j);
            continue;
        }

        RETURN_ON_FAILURE(&device, subresource.slices != nullptr, false, "'textureUploadDescs[%u].subresources[%u].slices' is invalid", i, j);
        RETURN_ON_FAILURE(&device, subresource.rowPitch != 0, false, "'textureUploadDescs[%u].subresources[%u].rowPitch' is 0", i, j);
        RETURN_ON_FAILURE(&device, subresource.slicePitch != 0, false, "'textureUploadDescs[%u].subresources[%u].slicePitch' is 0", i, j);
    }

    return true;
}

static bool ValidateBufferUploadDesc(DeviceVal& device, uint32_t i, const BufferUploadDesc& bufferUploadDesc) {
    if (bufferUploadDesc.dataSize == 0) {
        REPORT_WARNING(&device, "'bufferUploadDescs[%u].dataSize' is 0 (nothing to upload)", i);
        return true;
    }

    const BufferVal& bufferVal = *(BufferVal*)bufferUploadDesc.buffer;
    const uint64_t rangeEnd = bufferUploadDesc.bufferOffset + bufferUploadDesc.dataSize;

    RETURN_ON_FAILURE(&device, bufferUploadDesc.buffer != nullptr, false, "'bufferUploadDescs[%u].buffer' is invalid", i);
    RETURN_ON_FAILURE(&device, bufferUploadDesc.data != nullptr, false, "'bufferUploadDescs[%u].data' is invalid", i);
    RETURN_ON_FAILURE(&device, bufferVal.IsBoundToMemory(), false, "'bufferUploadDescs[%u].buffer' is not bound to memory", i);
    RETURN_ON_FAILURE(&device, rangeEnd <= bufferVal.GetDesc().size, false, "'bufferUploadDescs[%u].bufferOffset + bufferUploadDescs[%u].dataSize' is out of bounds", i, i);

    return true;
}

static Result NRI_CALL UploadData(Queue& queue, const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    QueueVal& queueVal = (QueueVal&)queue;
    DeviceVal& deviceVal = queueVal.GetDevice();

    RETURN_ON_FAILURE(&deviceVal, textureUploadDescNum == 0 || textureUploadDescs != nullptr, Result::INVALID_ARGUMENT, "'textureUploadDescs' is NULL");
    RETURN_ON_FAILURE(&deviceVal, bufferUploadDescNum == 0 || bufferUploadDescs != nullptr, Result::INVALID_ARGUMENT, "'bufferUploadDescs' is NULL");

    for (uint32_t i = 0; i < textureUploadDescNum; i++) {
        if (!ValidateTextureUploadDesc(deviceVal, i, textureUploadDescs[i]))
            return Result::INVALID_ARGUMENT;
    }

    for (uint32_t i = 0; i < bufferUploadDescNum; i++) {
        if (!ValidateBufferUploadDesc(deviceVal, i, bufferUploadDescs[i]))
            return Result::INVALID_ARGUMENT;
    }

    HelperDataUpload helperDataUpload(deviceVal.GetCoreInterfaceVal(), (Device&)deviceVal, queue);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

static Result NRI_CALL WaitForIdle(Queue& queue) {
    if (!(&queue))
        return Result::SUCCESS;

    QueueVal& queueVal = (QueueVal&)queue;
    DeviceVal& deviceVal = queueVal.GetDevice();

    return WaitIdle(deviceVal.GetCoreInterfaceVal(), (Device&)deviceVal, queue);
}

static uint32_t NRI_CALL CalculateAllocationNumber(const Device& device, const ResourceGroupDesc& resourceGroupDesc) {
    DeviceVal& deviceVal = (DeviceVal&)device;

    RETURN_ON_FAILURE(&deviceVal, resourceGroupDesc.memoryLocation < MemoryLocation::MAX_NUM, 0, "'memoryLocation' is invalid");
    RETURN_ON_FAILURE(&deviceVal, resourceGroupDesc.bufferNum == 0 || resourceGroupDesc.buffers != nullptr, 0, "'buffers' is NULL");
    RETURN_ON_FAILURE(&deviceVal, resourceGroupDesc.textureNum == 0 || resourceGroupDesc.textures != nullptr, 0, "'textures' is NULL");

    for (uint32_t i = 0; i < resourceGroupDesc.bufferNum; i++) {
        RETURN_ON_FAILURE(&deviceVal, resourceGroupDesc.buffers[i] != nullptr, 0, "'buffers[%u]' is NULL", i);
    }

    for (uint32_t i = 0; i < resourceGroupDesc.textureNum; i++) {
        RETURN_ON_FAILURE(&deviceVal, resourceGroupDesc.textures[i] != nullptr, 0, "'textures[%u]' is NULL", i);
    }

    HelperDeviceMemoryAllocator allocator(deviceVal.GetCoreInterfaceVal(), (Device&)device);

    return allocator.CalculateAllocationNumber(resourceGroupDesc);
}

static Result NRI_CALL AllocateAndBindMemory(Device& device, const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    DeviceVal& deviceVal = (DeviceVal&)device;

    RETURN_ON_FAILURE(&deviceVal, allocations != nullptr, Result::INVALID_ARGUMENT, "'allocations' is NULL");
    RETURN_ON_FAILURE(&deviceVal, resourceGroupDesc.memoryLocation < MemoryLocation::MAX_NUM, Result::INVALID_ARGUMENT, "'memoryLocation' is invalid");
    RETURN_ON_FAILURE(&deviceVal, resourceGroupDesc.bufferNum == 0 || resourceGroupDesc.buffers != nullptr, Result::INVALID_ARGUMENT, "'buffers' is NULL");
    RETURN_ON_FAILURE(&deviceVal, resourceGroupDesc.textureNum == 0 || resourceGroupDesc.textures != nullptr, Result::INVALID_ARGUMENT, "'textures' is NULL");

    for (uint32_t i = 0; i < resourceGroupDesc.bufferNum; i++) {
        RETURN_ON_FAILURE(&deviceVal, resourceGroupDesc.buffers[i] != nullptr, Result::INVALID_ARGUMENT, "'buffers[%u]' is NULL", i);
    }

    for (uint32_t i = 0; i < resourceGroupDesc.textureNum; i++) {
        RETURN_ON_FAILURE(&deviceVal, resourceGroupDesc.textures[i] != nullptr, Result::INVALID_ARGUMENT, "'textures[%u]' is NULL", i);
    }

    HelperDeviceMemoryAllocator allocator(deviceVal.GetCoreInterfaceVal(), device);
    Result result = allocator.AllocateAndBindMemory(resourceGroupDesc, allocations);

    return result;
}

static Result NRI_CALL QueryVideoMemoryInfo(const Device& device, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) {
    DeviceVal& deviceVal = (DeviceVal&)device;

    return deviceVal.GetHelperInterface().QueryVideoMemoryInfo(deviceVal.GetImpl(), memoryLocation, videoMemoryInfo);
}

Result DeviceVal::FillFunctionTable(HelperInterface& table) const {
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
    ((QueueVal&)queue).Submit(workSubmissionDesc, &swapChain);
}

static Result SetLatencySleepMode(SwapChain& swapChain, const LatencySleepMode& latencySleepMode) {
    return ((SwapChainVal&)swapChain).SetLatencySleepMode(latencySleepMode);
}

static Result SetLatencyMarker(SwapChain& swapChain, LatencyMarker latencyMarker) {
    return ((SwapChainVal&)swapChain).SetLatencyMarker(latencyMarker);
}

static Result LatencySleep(SwapChain& swapChain) {
    return ((SwapChainVal&)swapChain).LatencySleep();
}

static Result GetLatencyReport(const SwapChain& swapChain, LatencyReport& latencyReport) {
    return ((SwapChainVal&)swapChain).GetLatencyReport(latencyReport);
}

Result DeviceVal::FillFunctionTable(LowLatencyInterface& table) const {
    if (!m_IsExtSupported.lowLatency)
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
    ((CommandBufferVal&)commandBuffer).DrawMeshTasks(drawMeshTasksDesc);
}

static void NRI_CALL CmdDrawMeshTasksIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferVal&)commandBuffer).DrawMeshTasksIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

Result DeviceVal::FillFunctionTable(MeshShaderInterface& table) const {
    if (!m_IsExtSupported.meshShader)
        return Result::UNSUPPORTED;

    table.CmdDrawMeshTasks = ::CmdDrawMeshTasks;
    table.CmdDrawMeshTasksIndirect = ::CmdDrawMeshTasksIndirect;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  RayTracing  ]

static uint64_t NRI_CALL GetAccelerationStructureUpdateScratchBufferSize(const AccelerationStructure& accelerationStructure) {
    return ((const AccelerationStructureVal&)accelerationStructure).GetUpdateScratchBufferSize();
}

static uint64_t NRI_CALL GetAccelerationStructureBuildScratchBufferSize(const AccelerationStructure& accelerationStructure) {
    return ((const AccelerationStructureVal&)accelerationStructure).GetBuildScratchBufferSize();
}

static uint64_t NRI_CALL GetAccelerationStructureHandle(const AccelerationStructure& accelerationStructure) {
    return ((const AccelerationStructureVal&)accelerationStructure).GetHandle();
}

static Buffer* NRI_CALL GetAccelerationStructureBuffer(const AccelerationStructure& accelerationStructure) {
    return ((AccelerationStructureVal&)accelerationStructure).GetBuffer();
}

static Result NRI_CALL CreateAccelerationStructureDescriptor(const AccelerationStructure& accelerationStructure, Descriptor*& descriptor) {
    return ((AccelerationStructureVal&)accelerationStructure).CreateDescriptor(descriptor);
}

static uint64_t NRI_CALL GetAccelerationStructureNativeObject(const AccelerationStructure& accelerationStructure) {
    return ((AccelerationStructureVal&)accelerationStructure).GetNativeObject();
}

static void NRI_CALL CmdBuildTopLevelAccelerationStructure(CommandBuffer& commandBuffer, uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {
    ((CommandBufferVal&)commandBuffer).BuildTopLevelAccelerationStructure(instanceNum, buffer, bufferOffset, flags, dst, scratch, scratchOffset);
}

static void NRI_CALL CmdBuildBottomLevelAccelerationStructure(CommandBuffer& commandBuffer, uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, Buffer& scratch, uint64_t scratchOffset) {
    ((CommandBufferVal&)commandBuffer).BuildBottomLevelAccelerationStructure(geometryObjectNum, geometryObjects, flags, dst, scratch, scratchOffset);
}

static void NRI_CALL CmdUpdateTopLevelAccelerationStructure(CommandBuffer& commandBuffer, uint32_t instanceNum, const Buffer& buffer, uint64_t bufferOffset, AccelerationStructureBuildBits flags, AccelerationStructure& dst, const AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {
    ((CommandBufferVal&)commandBuffer).UpdateTopLevelAccelerationStructure(instanceNum, buffer, bufferOffset, flags, dst, src, scratch, scratchOffset);
}

static void NRI_CALL CmdUpdateBottomLevelAccelerationStructure(CommandBuffer& commandBuffer, uint32_t geometryObjectNum, const GeometryObject* geometryObjects, AccelerationStructureBuildBits flags, AccelerationStructure& dst, const AccelerationStructure& src, Buffer& scratch, uint64_t scratchOffset) {
    ((CommandBufferVal&)commandBuffer).UpdateBottomLevelAccelerationStructure(geometryObjectNum, geometryObjects, flags, dst, src, scratch, scratchOffset);
}

static void NRI_CALL CmdCopyAccelerationStructure(CommandBuffer& commandBuffer, AccelerationStructure& dst, const AccelerationStructure& src, CopyMode mode) {
    ((CommandBufferVal&)commandBuffer).CopyAccelerationStructure(dst, src, mode);
}

static void NRI_CALL CmdWriteAccelerationStructureSize(CommandBuffer& commandBuffer, const AccelerationStructure* const* accelerationStructures, uint32_t accelerationStructureNum, QueryPool& queryPool, uint32_t queryOffset) {
    ((CommandBufferVal&)commandBuffer).WriteAccelerationStructureSize(accelerationStructures, accelerationStructureNum, queryPool, queryOffset);
}

static void NRI_CALL CmdDispatchRays(CommandBuffer& commandBuffer, const DispatchRaysDesc& dispatchRaysDesc) {
    ((CommandBufferVal&)commandBuffer).DispatchRays(dispatchRaysDesc);
}

static void NRI_CALL CmdDispatchRaysIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset) {
    ((CommandBufferVal&)commandBuffer).DispatchRaysIndirect(buffer, offset);
}

static void NRI_CALL GetAccelerationStructureMemoryDesc(const AccelerationStructure& accelerationStructure, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    const AccelerationStructureVal& accelerationStructureVal = (AccelerationStructureVal&)accelerationStructure;
    DeviceVal& deviceVal = (DeviceVal&)accelerationStructureVal.GetDevice();

    deviceVal.GetRayTracingInterface().GetAccelerationStructureMemoryDesc(*accelerationStructureVal.GetImpl(), memoryLocation, memoryDesc);
    deviceVal.RegisterMemoryType(memoryDesc.type, memoryLocation);
}

static void NRI_CALL GetAccelerationStructureMemoryDesc2(const Device& device, const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    DeviceVal& deviceVal = (DeviceVal&)device;

    auto accelerationStructureDescImpl = accelerationStructureDesc;

    uint32_t geometryObjectNum = accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL ? accelerationStructureDesc.instanceOrGeometryObjectNum : 0;
    Scratch<GeometryObject> objectImplArray = AllocateScratch(deviceVal, GeometryObject, geometryObjectNum);

    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL) {
        ConvertGeometryObjectsVal(objectImplArray, accelerationStructureDesc.geometryObjects, geometryObjectNum);
        accelerationStructureDescImpl.geometryObjects = objectImplArray;
    }

    deviceVal.GetRayTracingInterface().GetAccelerationStructureMemoryDesc2(deviceVal.GetImpl(), accelerationStructureDescImpl, memoryLocation, memoryDesc);
    deviceVal.RegisterMemoryType(memoryDesc.type, memoryLocation);
}

static Result NRI_CALL CreateRayTracingPipeline(Device& device, const RayTracingPipelineDesc& pipelineDesc, Pipeline*& pipeline) {
    return ((DeviceVal&)device).CreatePipeline(pipelineDesc, pipeline);
}

static Result NRI_CALL CreateAccelerationStructure(Device& device, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVal&)device).CreateAccelerationStructure(accelerationStructureDesc, accelerationStructure);
}

static Result NRI_CALL BindAccelerationStructureMemory(Device& device, const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceVal&)device).BindAccelerationStructureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static void NRI_CALL DestroyAccelerationStructure(AccelerationStructure& accelerationStructure) {
    if (!(&accelerationStructure))
        return;

    GetDeviceVal(accelerationStructure).DestroyAccelerationStructure(accelerationStructure);
}

static Result NRI_CALL WriteShaderGroupIdentifiers(const Pipeline& pipeline, uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* buffer) {
    return ((PipelineVal&)pipeline).WriteShaderGroupIdentifiers(baseShaderGroupIndex, shaderGroupNum, buffer);
}

Result DeviceVal::FillFunctionTable(RayTracingInterface& table) const {
    if (!m_IsExtSupported.rayTracing)
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
    return ((DeviceVal&)device).AllocateBuffer(bufferDesc, buffer);
}

static Result AllocateTexture(Device& device, const AllocateTextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceVal&)device).AllocateTexture(textureDesc, texture);
}

static Result AllocateAccelerationStructure(Device& device, const AllocateAccelerationStructureDesc& acelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVal&)device).AllocateAccelerationStructure(acelerationStructureDesc, accelerationStructure);
}

Result DeviceVal::FillFunctionTable(ResourceAllocatorInterface& table) const {
    table.AllocateBuffer = ::AllocateBuffer;
    table.AllocateTexture = ::AllocateTexture;
    table.AllocateAccelerationStructure = ::AllocateAccelerationStructure;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Streamer  ]

struct StreamerVal : public ObjectVal {
    inline StreamerVal(DeviceVal& device, StreamerImpl* impl, const StreamerDesc& desc)
        : ObjectVal(device, impl)
        , m_Desc(desc) {
    }

    inline StreamerImpl* GetImpl() const {
        return (StreamerImpl*)m_Impl;
    }

    StreamerDesc m_Desc = {}; // only for .natvis
    bool isDynamicBufferValid = false;
};

static Result CreateStreamer(Device& device, const StreamerDesc& streamerDesc, Streamer*& streamer) {
    DeviceVal& deviceVal = (DeviceVal&)device;

    bool isUpload = streamerDesc.constantBufferMemoryLocation == MemoryLocation::HOST_UPLOAD || streamerDesc.constantBufferMemoryLocation == MemoryLocation::DEVICE_UPLOAD;
    RETURN_ON_FAILURE(&deviceVal, isUpload, Result::INVALID_ARGUMENT, "'constantBufferMemoryLocation' must be an UPLOAD heap");

    isUpload = streamerDesc.dynamicBufferMemoryLocation == MemoryLocation::HOST_UPLOAD || streamerDesc.dynamicBufferMemoryLocation == MemoryLocation::DEVICE_UPLOAD;
    RETURN_ON_FAILURE(&deviceVal, isUpload, Result::INVALID_ARGUMENT, "'dynamicBufferMemoryLocation' must be an UPLOAD heap");

    StreamerImpl* impl = Allocate<StreamerImpl>(deviceVal.GetAllocationCallbacks(), device, deviceVal.GetCoreInterfaceVal());
    Result result = impl->Create(streamerDesc);

    if (result != Result::SUCCESS) {
        Destroy(deviceVal.GetAllocationCallbacks(), impl);
        streamer = nullptr;
    } else
        streamer = (Streamer*)Allocate<StreamerVal>(deviceVal.GetAllocationCallbacks(), deviceVal, impl, streamerDesc);

    return result;
}

static void DestroyStreamer(Streamer& streamer) {
    if (!(&streamer))
        return;

    DeviceVal& deviceVal = GetDeviceVal(streamer);
    StreamerVal& streamerVal = (StreamerVal&)streamer;
    StreamerImpl* streamerImpl = streamerVal.GetImpl();

    Destroy(streamerImpl);
    Destroy(deviceVal.GetAllocationCallbacks(), &streamerVal);
}

static Buffer* GetStreamerConstantBuffer(Streamer& streamer) {
    StreamerVal& streamerVal = (StreamerVal&)streamer;
    StreamerImpl* streamerImpl = streamerVal.GetImpl();

    return streamerImpl->GetConstantBuffer();
}

static uint32_t UpdateStreamerConstantBuffer(Streamer& streamer, const void* data, uint32_t dataSize) {
    DeviceVal& deviceVal = GetDeviceVal(streamer);
    StreamerVal& streamerVal = (StreamerVal&)streamer;
    StreamerImpl* streamerImpl = streamerVal.GetImpl();

    if (!dataSize)
        REPORT_WARNING(&deviceVal, "'dataSize = 0'");

    return streamerImpl->UpdateConstantBuffer(data, dataSize);
}

static uint64_t AddStreamerBufferUpdateRequest(Streamer& streamer, const BufferUpdateRequestDesc& bufferUpdateRequestDesc) {
    DeviceVal& deviceVal = GetDeviceVal(streamer);
    StreamerVal& streamerVal = (StreamerVal&)streamer;
    StreamerImpl* streamerImpl = streamerVal.GetImpl();

    streamerVal.isDynamicBufferValid = false;

    if (!bufferUpdateRequestDesc.dataSize)
        REPORT_WARNING(&deviceVal, "'bufferUpdateRequestDesc.dataSize = 0'");

    return streamerImpl->AddBufferUpdateRequest(bufferUpdateRequestDesc);
}

static uint64_t AddStreamerTextureUpdateRequest(Streamer& streamer, const TextureUpdateRequestDesc& textureUpdateRequestDesc) {
    DeviceVal& deviceVal = GetDeviceVal(streamer);
    StreamerVal& streamerVal = (StreamerVal&)streamer;
    StreamerImpl* streamerImpl = streamerVal.GetImpl();

    streamerVal.isDynamicBufferValid = false;

    if (!textureUpdateRequestDesc.dstTexture)
        REPORT_ERROR(&deviceVal, "'textureUpdateRequestDesc.dstTexture' is NULL");
    if (!textureUpdateRequestDesc.dataRowPitch)
        REPORT_WARNING(&deviceVal, "'textureUpdateRequestDesc.dataRowPitch = 0'");
    if (!textureUpdateRequestDesc.dataSlicePitch)
        REPORT_WARNING(&deviceVal, "'textureUpdateRequestDesc.dataSlicePitch = 0'");

    return streamerImpl->AddTextureUpdateRequest(textureUpdateRequestDesc);
}

static Result CopyStreamerUpdateRequests(Streamer& streamer) {
    StreamerVal& streamerVal = (StreamerVal&)streamer;
    StreamerImpl* streamerImpl = streamerVal.GetImpl();

    streamerVal.isDynamicBufferValid = true;

    return streamerImpl->CopyUpdateRequests();
}

static Buffer* GetStreamerDynamicBuffer(Streamer& streamer) {
    DeviceVal& deviceVal = GetDeviceVal(streamer);
    StreamerVal& streamerVal = (StreamerVal&)streamer;
    StreamerImpl* streamerImpl = streamerVal.GetImpl();

    if (!streamerVal.isDynamicBufferValid)
        REPORT_ERROR(&deviceVal, "'GetStreamerDynamicBuffer' must be called after 'CopyStreamerUpdateRequests'");

    return streamerImpl->GetDynamicBuffer();
}

static void CmdUploadStreamerUpdateRequests(CommandBuffer& commandBuffer, Streamer& streamer) {
    StreamerVal& streamerVal = (StreamerVal&)streamer;
    StreamerImpl* streamerImpl = streamerVal.GetImpl();

    streamerImpl->CmdUploadUpdateRequests(commandBuffer);
}

Result DeviceVal::FillFunctionTable(StreamerInterface& table) const {
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
    return ((DeviceVal&)device).CreateSwapChain(swapChainDesc, swapChain);
}

static void NRI_CALL DestroySwapChain(SwapChain& swapChain) {
    if (!(&swapChain))
        return;

    GetDeviceVal(swapChain).DestroySwapChain(swapChain);
}

static Texture* const* NRI_CALL GetSwapChainTextures(const SwapChain& swapChain, uint32_t& textureNum) {
    return ((SwapChainVal&)swapChain).GetTextures(textureNum);
}

static uint32_t NRI_CALL AcquireNextSwapChainTexture(SwapChain& swapChain) {
    return ((SwapChainVal&)swapChain).AcquireNextTexture();
}

static Result NRI_CALL WaitForPresent(SwapChain& swapChain) {
    return ((SwapChainVal&)swapChain).WaitForPresent();
}

static Result NRI_CALL QueuePresent(SwapChain& swapChain) {
    return ((SwapChainVal&)swapChain).Present();
}

static Result NRI_CALL GetDisplayDesc(SwapChain& swapChain, DisplayDesc& displayDesc) {
    return ((SwapChainVal&)swapChain).GetDisplayDesc(displayDesc);
}

Result DeviceVal::FillFunctionTable(SwapChainInterface& table) const {
    if (!m_IsExtSupported.swapChain)
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

struct UpscalerVal : public ObjectVal {
    inline UpscalerVal(DeviceVal& device, UpscalerImpl* impl, const UpscalerDesc& desc)
        : ObjectVal(device, impl)
        , m_Desc(desc) {
    }

    inline UpscalerImpl* GetImpl() const {
        return (UpscalerImpl*)m_Impl;
    }

    UpscalerDesc m_Desc = {}; // only for .natvis
};

static Result CreateUpscaler(Device& device, const UpscalerDesc& upscalerDesc, Upscaler*& upscaler) {
    DeviceVal& deviceVal = (DeviceVal&)device;

    UpscalerImpl* impl = Allocate<UpscalerImpl>(deviceVal.GetAllocationCallbacks(), device, deviceVal.GetCoreInterfaceVal());
    Result result = impl->Create(upscalerDesc);

    if (result != Result::SUCCESS) {
        Destroy(deviceVal.GetAllocationCallbacks(), impl);
        upscaler = nullptr;
    } else
        upscaler = (Upscaler*)Allocate<UpscalerVal>(deviceVal.GetAllocationCallbacks(), deviceVal, impl, upscalerDesc);

    return result;
}

static void DestroyUpscaler(Upscaler& upscaler) {
    if (!(&upscaler))
        return;

    DeviceVal& deviceVal = GetDeviceVal(upscaler);
    UpscalerVal& upscalerVal = (UpscalerVal&)upscaler;
    UpscalerImpl* upscalerImpl = upscalerVal.GetImpl();

    Destroy(upscalerImpl);
    Destroy(deviceVal.GetAllocationCallbacks(), &upscalerVal);
}

static bool IsUpscalerSupported(const Device& device, UpscalerType upscalerType) {
    DeviceVal& deviceVal = (DeviceVal&)device;

    return IsUpscalerSupported(deviceVal.GetDesc(), upscalerType);
}

static void GetUpscalerProps(const Upscaler& upscaler, UpscalerProps& upscalerProps) {
    UpscalerVal& upscalerVal = (UpscalerVal&)upscaler;
    UpscalerImpl* upscalerImpl = upscalerVal.GetImpl();
    
    return upscalerImpl->GetUpscalerProps(upscalerProps);
}

static void CmdDispatchUpscale(CommandBuffer& commandBuffer, Upscaler& upscaler, const DispatchUpscaleDesc& dispatchUpscaleDesc) {
    UpscalerVal& upscalerVal = (UpscalerVal&)upscaler;
    UpscalerImpl* upscalerImpl = upscalerVal.GetImpl();
    
    upscalerImpl->CmdDispatchUpscale(commandBuffer, dispatchUpscaleDesc);
}

Result DeviceVal::FillFunctionTable(UpscalerInterface& table) const {
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

#if NRI_ENABLE_D3D11_SUPPORT

static Result NRI_CALL CreateCommandBufferD3D11(Device& device, const CommandBufferD3D11Desc& commandBufferD3D11Desc, CommandBuffer*& commandBuffer) {
    return ((DeviceVal&)device).CreateCommandBuffer(commandBufferD3D11Desc, commandBuffer);
}

static Result NRI_CALL CreateBufferD3D11(Device& device, const BufferD3D11Desc& bufferD3D11Desc, Buffer*& buffer) {
    return ((DeviceVal&)device).CreateBuffer(bufferD3D11Desc, buffer);
}

static Result NRI_CALL CreateTextureD3D11(Device& device, const TextureD3D11Desc& textureD3D11Desc, Texture*& texture) {
    return ((DeviceVal&)device).CreateTexture(textureD3D11Desc, texture);
}

#endif

Result DeviceVal::FillFunctionTable(WrapperD3D11Interface& table) const {
#if NRI_ENABLE_D3D11_SUPPORT
    if (!m_IsExtSupported.wrapperD3D11)
        return Result::UNSUPPORTED;

    table.CreateCommandBufferD3D11 = ::CreateCommandBufferD3D11;
    table.CreateTextureD3D11 = ::CreateTextureD3D11;
    table.CreateBufferD3D11 = ::CreateBufferD3D11;

    return Result::SUCCESS;
#else
    MaybeUnused(table);

    return Result::UNSUPPORTED;
#endif
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  WrapperD3D12  ]

#if NRI_ENABLE_D3D12_SUPPORT

static Result NRI_CALL CreateCommandBufferD3D12(Device& device, const CommandBufferD3D12Desc& commandBufferD3D12Desc, CommandBuffer*& commandBuffer) {
    return ((DeviceVal&)device).CreateCommandBuffer(commandBufferD3D12Desc, commandBuffer);
}

static Result NRI_CALL CreateDescriptorPoolD3D12(Device& device, const DescriptorPoolD3D12Desc& descriptorPoolD3D12Desc, DescriptorPool*& descriptorPool) {
    return ((DeviceVal&)device).CreateDescriptorPool(descriptorPoolD3D12Desc, descriptorPool);
}

static Result NRI_CALL CreateBufferD3D12(Device& device, const BufferD3D12Desc& bufferD3D12Desc, Buffer*& buffer) {
    return ((DeviceVal&)device).CreateBuffer(bufferD3D12Desc, buffer);
}

static Result NRI_CALL CreateTextureD3D12(Device& device, const TextureD3D12Desc& textureD3D12Desc, Texture*& texture) {
    return ((DeviceVal&)device).CreateTexture(textureD3D12Desc, texture);
}

static Result NRI_CALL CreateMemoryD3D12(Device& device, const MemoryD3D12Desc& memoryD3D12Desc, Memory*& memory) {
    return ((DeviceVal&)device).CreateMemory(memoryD3D12Desc, memory);
}

static Result NRI_CALL CreateAccelerationStructureD3D12(Device& device, const AccelerationStructureD3D12Desc& accelerationStructureD3D12Desc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVal&)device).CreateAccelerationStructure(accelerationStructureD3D12Desc, accelerationStructure);
}

#endif

Result DeviceVal::FillFunctionTable(WrapperD3D12Interface& table) const {
#if NRI_ENABLE_D3D12_SUPPORT
    if (!m_IsExtSupported.wrapperD3D12)
        return Result::UNSUPPORTED;

    table.CreateCommandBufferD3D12 = ::CreateCommandBufferD3D12;
    table.CreateDescriptorPoolD3D12 = ::CreateDescriptorPoolD3D12;
    table.CreateBufferD3D12 = ::CreateBufferD3D12;
    table.CreateTextureD3D12 = ::CreateTextureD3D12;
    table.CreateMemoryD3D12 = ::CreateMemoryD3D12;
    table.CreateAccelerationStructureD3D12 = ::CreateAccelerationStructureD3D12;

    return Result::SUCCESS;
#else
    MaybeUnused(table);

    return Result::UNSUPPORTED;
#endif
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  WrapperVK  ]

#if NRI_ENABLE_VK_SUPPORT

static Result NRI_CALL CreateCommandAllocatorVK(Device& device, const CommandAllocatorVKDesc& commandAllocatorVKDesc, CommandAllocator*& commandAllocator) {
    return ((DeviceVal&)device).CreateCommandAllocator(commandAllocatorVKDesc, commandAllocator);
}

static Result NRI_CALL CreateCommandBufferVK(Device& device, const CommandBufferVKDesc& commandBufferVKDesc, CommandBuffer*& commandBuffer) {
    return ((DeviceVal&)device).CreateCommandBuffer(commandBufferVKDesc, commandBuffer);
}

static Result NRI_CALL CreateDescriptorPoolVK(Device& device, const DescriptorPoolVKDesc& descriptorPoolVKDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceVal&)device).CreateDescriptorPool(descriptorPoolVKDesc, descriptorPool);
}

static Result NRI_CALL CreateBufferVK(Device& device, const BufferVKDesc& bufferVKDesc, Buffer*& buffer) {
    return ((DeviceVal&)device).CreateBuffer(bufferVKDesc, buffer);
}

static Result NRI_CALL CreateTextureVK(Device& device, const TextureVKDesc& textureVKDesc, Texture*& texture) {
    return ((DeviceVal&)device).CreateTexture(textureVKDesc, texture);
}

static Result NRI_CALL CreateMemoryVK(Device& device, const MemoryVKDesc& memoryVKDesc, Memory*& memory) {
    return ((DeviceVal&)device).CreateMemory(memoryVKDesc, memory);
}

static Result NRI_CALL CreateGraphicsPipelineVK(Device& device, VKNonDispatchableHandle vkPipeline, Pipeline*& pipeline) {
    return ((DeviceVal&)device).CreateGraphicsPipeline(vkPipeline, pipeline);
}

static Result NRI_CALL CreateComputePipelineVK(Device& device, VKNonDispatchableHandle vkPipeline, Pipeline*& pipeline) {
    return ((DeviceVal&)device).CreateComputePipeline(vkPipeline, pipeline);
}

static Result NRI_CALL CreateQueryPoolVK(Device& device, const QueryPoolVKDesc& queryPoolVKDesc, QueryPool*& queryPool) {
    return ((DeviceVal&)device).CreateQueryPool(queryPoolVKDesc, queryPool);
}

static Result NRI_CALL CreateAccelerationStructureVK(Device& device, const AccelerationStructureVKDesc& accelerationStructureVKDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVal&)device).CreateAccelerationStructure(accelerationStructureVKDesc, accelerationStructure);
}

static VKHandle NRI_CALL GetPhysicalDeviceVK(const Device& device) {
    return ((DeviceVal&)device).GetWrapperVKInterface().GetPhysicalDeviceVK(((DeviceVal&)device).GetImpl());
}

static uint32_t NRI_CALL GetQueueFamilyIndexVK(const Queue& queue) {
    const QueueVal& queueVal = (QueueVal&)queue;
    return queueVal.GetWrapperVKInterface().GetQueueFamilyIndexVK(*queueVal.GetImpl());
}

static VKHandle NRI_CALL GetInstanceVK(const Device& device) {
    return ((DeviceVal&)device).GetWrapperVKInterface().GetInstanceVK(((DeviceVal&)device).GetImpl());
}

static void* NRI_CALL GetInstanceProcAddrVK(const Device& device) {
    return ((DeviceVal&)device).GetWrapperVKInterface().GetInstanceProcAddrVK(((DeviceVal&)device).GetImpl());
}

static void* NRI_CALL GetDeviceProcAddrVK(const Device& device) {
    return ((DeviceVal&)device).GetWrapperVKInterface().GetDeviceProcAddrVK(((DeviceVal&)device).GetImpl());
}

#endif

Result DeviceVal::FillFunctionTable(WrapperVKInterface& table) const {
#if NRI_ENABLE_VK_SUPPORT
    if (!m_IsExtSupported.wrapperVK)
        return Result::UNSUPPORTED;

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
    table.GetPhysicalDeviceVK = ::GetPhysicalDeviceVK;
    table.GetQueueFamilyIndexVK = ::GetQueueFamilyIndexVK;
    table.GetInstanceVK = ::GetInstanceVK;
    table.GetDeviceProcAddrVK = ::GetDeviceProcAddrVK;
    table.GetInstanceProcAddrVK = ::GetInstanceProcAddrVK;

    return Result::SUCCESS;
#else
    MaybeUnused(table);

    return Result::UNSUPPORTED;
#endif
}

#pragma endregion
