// © 2026 NVIDIA Corporation

#if NRI_METAL_CPP_PRIVATE_IMPLEMENTATION
#    define NS_PRIVATE_IMPLEMENTATION
#    define MTL_PRIVATE_IMPLEMENTATION
#    define CA_PRIVATE_IMPLEMENTATION
#endif

#define IR_PRIVATE_IMPLEMENTATION

#include "SharedMetal.h"

#include "BufferMetal.h"
#include "CommandAllocatorMetal.h"
#include "CommandBufferMetal.h"
#include "ConversionMetal.h"
#include "DescriptorMetal.h"
#include "DescriptorPoolMetal.h"
#include "DescriptorSetMetal.h"
#include "FenceMetal.h"
#include "MemoryMetal.h"
#include "PipelineLayoutMetal.h"
#include "PipelineMetal.h"
#include "QueryPoolMetal.h"
#include "QueueMetal.h"
#include "SwapChainMetal.h"
#include "TextureMetal.h"

#include "HelperInterface.h"
#include "ImguiInterface.h"
#include "StreamerInterface.h"
#include "UpscalerInterface.h"

using namespace nri;

#include "BufferMetal.hpp"
#include "CommandAllocatorMetal.hpp"
#include "CommandBufferMetal.hpp"
#include "ConversionMetal.hpp"
#include "DescriptorMetal.hpp"
#include "DescriptorPoolMetal.hpp"
#include "DescriptorSetMetal.hpp"
#include "DeviceMetal.hpp"
#include "FenceMetal.hpp"
#include "InternalShadersMetal.hpp"
#include "MemoryMetal.hpp"
#include "PipelineLayoutMetal.hpp"
#include "PipelineMetal.hpp"
#include "QueryPoolMetal.hpp"
#include "QueueMetal.hpp"
#include "SwapChainMetal.hpp"
#include "TextureMetal.hpp"

//============================================================================================================================================================================================
#pragma region[  Core  ]

static const DeviceDesc& NRI_CALL GetDeviceDesc(const Device& device) {
    return ((DeviceMetal&)device).GetDesc();
}

static const BufferDesc& NRI_CALL GetBufferDesc(const Buffer& buffer) {
    return ((BufferMetal&)buffer).GetDesc();
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture& texture) {
    return ((TextureMetal&)texture).GetDesc();
}

static FormatSupportBits NRI_CALL GetFormatSupport(const Device& device, Format format) {
    return GetFormatSupportMetal(*((const DeviceMetal&)device).GetNativeObject(), format);
}

static Result NRI_CALL GetQueue(Device& device, QueueType queueType, uint32_t queueIndex, Queue*& queue) {
    return ((DeviceMetal&)device).GetQueue(queueType, queueIndex, queue);
}

static Result NRI_CALL CreateCommandAllocator(Queue& queue, CommandAllocator*& commandAllocator) {
    DeviceMetal& device = ((QueueMetal&)queue).GetDevice();

    return device.CreateImplementation<CommandAllocatorMetal>(commandAllocator);
}

static Result NRI_CALL CreateCommandBuffer(CommandAllocator& commandAllocator, CommandBuffer*& commandBuffer) {
    CommandAllocatorMetal& allocator = (CommandAllocatorMetal&)commandAllocator;

    return allocator.GetDevice().CreateImplementation<CommandBufferMetal>(commandBuffer, (const CommandAllocator&)allocator);
}

static Result NRI_CALL CreateFence(Device& device, uint64_t initialValue, Fence*& fence) {
    return ((DeviceMetal&)device).CreateImplementation<FenceMetal>(fence, initialValue);
}

static Result NRI_CALL CreateDescriptorPool(Device& device, const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceMetal&)device).CreateImplementation<DescriptorPoolMetal>(descriptorPool, descriptorPoolDesc);
}

static Result NRI_CALL CreatePipelineLayout(Device& device, const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    return ((DeviceMetal&)device).CreateImplementation<PipelineLayoutMetal>(pipelineLayout, pipelineLayoutDesc);
}

static Result NRI_CALL CreateGraphicsPipeline(Device& device, const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    return ((DeviceMetal&)device).CreateImplementation<PipelineMetal>(pipeline, graphicsPipelineDesc);
}

static Result NRI_CALL CreateComputePipeline(Device& device, const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    return ((DeviceMetal&)device).CreateImplementation<PipelineMetal>(pipeline, computePipelineDesc);
}

// Pipeline caches are not supported ("features.pipelineCache = false")
static Result NRI_CALL CreatePipelineCache(Device&, const PipelineCacheDesc&, PipelineCache*& pipelineCache) {
    pipelineCache = nullptr;

    return Result::UNSUPPORTED;
}

static Result NRI_CALL CreateQueryPool(Device& device, const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    return ((DeviceMetal&)device).CreateImplementation<QueryPoolMetal>(queryPool, queryPoolDesc);
}

static Result NRI_CALL CreateSampler(Device& device, const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    return ((DeviceMetal&)device).CreateImplementation<DescriptorMetal>(sampler, samplerDesc);
}

static Result NRI_CALL CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    DeviceMetal& device = ((BufferMetal*)bufferViewDesc.buffer)->GetDevice();

    return device.CreateImplementation<DescriptorMetal>(bufferView, bufferViewDesc);
}

static Result NRI_CALL CreateTextureView(const TextureViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceMetal& device = ((TextureMetal*)textureViewDesc.texture)->GetDevice();

    return device.CreateImplementation<DescriptorMetal>(textureView, textureViewDesc);
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator* commandAllocator) {
    Destroy((CommandAllocatorMetal*)commandAllocator);
}

static void NRI_CALL DestroyCommandBuffer(CommandBuffer* commandBuffer) {
    Destroy((CommandBufferMetal*)commandBuffer);
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool* descriptorPool) {
    Destroy((DescriptorPoolMetal*)descriptorPool);
}

static void NRI_CALL DestroyBuffer(Buffer* buffer) {
    Destroy((BufferMetal*)buffer);
}

static void NRI_CALL DestroyTexture(Texture* texture) {
    Destroy((TextureMetal*)texture);
}

static void NRI_CALL DestroyDescriptor(Descriptor* descriptor) {
    Destroy((DescriptorMetal*)descriptor);
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout* pipelineLayout) {
    Destroy((PipelineLayoutMetal*)pipelineLayout);
}

static void NRI_CALL DestroyPipeline(Pipeline* pipeline) {
    Destroy((PipelineMetal*)pipeline);
}

static void NRI_CALL DestroyPipelineCache(PipelineCache*) {
}

static Result NRI_CALL GetPipelineCacheData(PipelineCache&, void*, uint64_t& size) {
    size = 0;

    return Result::UNSUPPORTED;
}

static void NRI_CALL DestroyQueryPool(QueryPool* queryPool) {
    Destroy((QueryPoolMetal*)queryPool);
}

static void NRI_CALL DestroyFence(Fence* fence) {
    Destroy((FenceMetal*)fence);
}

static Result NRI_CALL AllocateMemory(Device& device, const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    return ((DeviceMetal&)device).CreateImplementation<MemoryMetal>(memory, allocateMemoryDesc);
}

static void NRI_CALL FreeMemory(Memory* memory) {
    Destroy((MemoryMetal*)memory);
}

static Result NRI_CALL CreateBuffer(Device& device, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceMetal&)device).CreateImplementation<BufferMetal>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTexture(Device& device, const TextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceMetal&)device).CreateImplementation<TextureMetal>(texture, textureDesc);
}

static void NRI_CALL GetBufferMemoryDesc(const Buffer& buffer, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((BufferMetal&)buffer).GetMemoryDesc(memoryLocation, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc(const Texture& texture, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((TextureMetal&)texture).GetMemoryDesc(memoryLocation, memoryDesc);
}

static Result NRI_CALL BindBufferMemory(const BindBufferMemoryDesc* bindBufferMemoryDescs, uint32_t bindBufferMemoryDescNum) {
    for (uint32_t i = 0; i < bindBufferMemoryDescNum; i++) {
        BufferMetal& buffer = *(BufferMetal*)bindBufferMemoryDescs[i].buffer;
        MemoryMetal& memory = *(MemoryMetal*)bindBufferMemoryDescs[i].memory;
        Result result = buffer.Bind(memory, bindBufferMemoryDescs[i].offset);

        if (result != Result::SUCCESS)
            return result;
    }

    return Result::SUCCESS;
}

static Result NRI_CALL BindTextureMemory(const BindTextureMemoryDesc* descs, uint32_t num) {
    for (uint32_t i = 0; i < num; i++) {
        Result result = ((TextureMetal*)descs[i].texture)->Bind(*(MemoryMetal*)descs[i].memory, descs[i].offset);

        if (result != Result::SUCCESS)
            return result;
    }

    return Result::SUCCESS;
}

static void NRI_CALL GetBufferMemoryDesc2(const Device& device, const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    BufferMetal buffer((DeviceMetal&)device);
    buffer.Create(bufferDesc);
    buffer.GetMemoryDesc(memoryLocation, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc2(const Device& device, const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    TextureMetal texture((DeviceMetal&)device);
    texture.Create(textureDesc);
    texture.GetMemoryDesc(memoryLocation, memoryDesc);
}

static Result NRI_CALL CreateCommittedBuffer(Device& device, MemoryLocation memoryLocation, float priority, const BufferDesc& bufferDesc, Buffer*& buffer) {
    MaybeUnused(priority);

    return ((DeviceMetal&)device).CreateImplementation<BufferMetal>(buffer, bufferDesc, memoryLocation);
}

static Result NRI_CALL CreateCommittedTexture(Device& device, MemoryLocation memoryLocation, float priority, const TextureDesc& textureDesc, Texture*& texture) {
    MaybeUnused(priority);

    return ((DeviceMetal&)device).CreateImplementation<TextureMetal>(texture, textureDesc, memoryLocation);
}

static Result NRI_CALL CreatePlacedBuffer(Device& device, Memory* memory, uint64_t offset, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceMetal&)device).CreatePlacedBuffer(memory, offset, bufferDesc, buffer);
}

static Result NRI_CALL CreatePlacedTexture(Device& device, Memory* memory, uint64_t offset, const TextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceMetal&)device).CreatePlacedTexture(memory, offset, textureDesc, texture);
}

static Result NRI_CALL AllocateDescriptorSets(DescriptorPool& descriptorPool, const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    return ((DescriptorPoolMetal&)descriptorPool).AllocateDescriptorSets(pipelineLayout, setIndex, descriptorSets, instanceNum, variableDescriptorNum);
}

static void NRI_CALL UpdateDescriptorRanges(const UpdateDescriptorRangeDesc* updateDescriptorRangeDescs, uint32_t updateDescriptorRangeDescNum) {
    DescriptorSetMetal::UpdateDescriptorRanges(updateDescriptorRangeDescs, updateDescriptorRangeDescNum);
}

static void NRI_CALL CopyDescriptorRanges(const CopyDescriptorRangeDesc* copyDescriptorRangeDescs, uint32_t copyDescriptorRangeDescNum) {
    DescriptorSetMetal::Copy(copyDescriptorRangeDescs, copyDescriptorRangeDescNum);
}

static void NRI_CALL ResetDescriptorPool(DescriptorPool& descriptorPool) {
    ((DescriptorPoolMetal&)descriptorPool).Reset();
}

static void NRI_CALL GetDescriptorSetOffsets(const DescriptorSet& descriptorSet, uint32_t& resourceHeapOffset, uint32_t& samplerHeapOffset) {
    ((DescriptorSetMetal&)descriptorSet).GetOffsets(resourceHeapOffset, samplerHeapOffset);
}

static Result NRI_CALL BeginCommandBuffer(CommandBuffer& commandBuffer, const DescriptorPool* descriptorPool) {
    return ((CommandBufferMetal&)commandBuffer).Begin(descriptorPool);
}

static void NRI_CALL CmdSetDescriptorPool(CommandBuffer& commandBuffer, const DescriptorPool& descriptorPool) {
    ((CommandBufferMetal&)commandBuffer).CmdSetDescriptorPool(descriptorPool);
}

static void NRI_CALL CmdSetPipelineLayout(CommandBuffer& commandBuffer, BindPoint bindPoint, const PipelineLayout& pipelineLayout) {
    ((CommandBufferMetal&)commandBuffer).CmdSetPipelineLayout(bindPoint, pipelineLayout);
}

static void NRI_CALL CmdSetDescriptorSet(CommandBuffer& commandBuffer, const SetDescriptorSetDesc& setDescriptorSetDesc) {
    ((CommandBufferMetal&)commandBuffer).CmdSetDescriptorSet(setDescriptorSetDesc);
}

static void NRI_CALL CmdSetRootConstants(CommandBuffer& commandBuffer, const SetRootConstantsDesc& setRootConstantsDesc) {
    ((CommandBufferMetal&)commandBuffer).CmdSetRootConstants(setRootConstantsDesc);
}

static void NRI_CALL CmdSetRootDescriptor(CommandBuffer& commandBuffer, const SetRootDescriptorDesc& setRootDescriptorDesc) {
    ((CommandBufferMetal&)commandBuffer).CmdSetRootDescriptor(setRootDescriptorDesc);
}

static void NRI_CALL CmdSetPipeline(CommandBuffer& commandBuffer, const Pipeline& pipeline) {
    ((CommandBufferMetal&)commandBuffer).CmdSetPipeline(pipeline);
}

static void NRI_CALL CmdBarrier(CommandBuffer& commandBuffer, const BarrierDesc& barrierDesc) {
    ((CommandBufferMetal&)commandBuffer).CmdBarrier(barrierDesc);
}

static void NRI_CALL CmdSetIndexBuffer(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, IndexType indexType) {
    ((CommandBufferMetal&)commandBuffer).CmdSetIndexBuffer(buffer, offset, indexType);
}

static void NRI_CALL CmdSetVertexBuffers(CommandBuffer& commandBuffer, uint32_t baseSlot, const VertexBufferDesc* vertexBufferDescs, uint32_t vertexBufferNum) {
    ((CommandBufferMetal&)commandBuffer).CmdSetVertexBuffers(baseSlot, vertexBufferDescs, vertexBufferNum);
}

static void NRI_CALL CmdSetViewports(CommandBuffer& commandBuffer, const Viewport* viewports, uint32_t viewportNum) {
    ((CommandBufferMetal&)commandBuffer).CmdSetViewports(viewports, viewportNum);
}

static void NRI_CALL CmdSetScissors(CommandBuffer& commandBuffer, const nri::Rect* rects, uint32_t rectNum) {
    ((CommandBufferMetal&)commandBuffer).CmdSetScissors(rects, rectNum);
}

static void NRI_CALL CmdSetStencilReference(CommandBuffer& commandBuffer, uint8_t frontRef, uint8_t backRef) {
    ((CommandBufferMetal&)commandBuffer).CmdSetStencilReference(frontRef, backRef);
}

static void NRI_CALL CmdSetDepthBounds(CommandBuffer& commandBuffer, float boundsMin, float boundsMax) {
    ((CommandBufferMetal&)commandBuffer).CmdSetDepthBounds(boundsMin, boundsMax);
}

static void NRI_CALL CmdSetBlendConstants(CommandBuffer& commandBuffer, const Color32f& color) {
    ((CommandBufferMetal&)commandBuffer).CmdSetBlendConstants(color);
}

static void NRI_CALL CmdSetSampleLocations(CommandBuffer& commandBuffer, const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    ((CommandBufferMetal&)commandBuffer).CmdSetSampleLocations(locations, locationNum, sampleNum);
}

static void NRI_CALL CmdSetShadingRate(CommandBuffer& commandBuffer, const ShadingRateDesc& shadingRateDesc) {
    ((CommandBufferMetal&)commandBuffer).CmdSetShadingRate(shadingRateDesc);
}

static void NRI_CALL CmdSetDepthBias(CommandBuffer& commandBuffer, const DepthBiasDesc& depthBiasDesc) {
    ((CommandBufferMetal&)commandBuffer).CmdSetDepthBias(depthBiasDesc);
}

static void NRI_CALL CmdBeginRendering(CommandBuffer& commandBuffer, const RenderingDesc& renderingDesc) {
    ((CommandBufferMetal&)commandBuffer).CmdBeginRendering(renderingDesc);
}

static void NRI_CALL CmdClearAttachments(CommandBuffer& commandBuffer, const ClearAttachmentDesc* clearAttachmentDescs, uint32_t clearAttachmentDescNum, const nri::Rect* rects, uint32_t rectNum) {
    ((CommandBufferMetal&)commandBuffer).CmdClearAttachments(clearAttachmentDescs, clearAttachmentDescNum, rects, rectNum);
}

static void NRI_CALL CmdDraw(CommandBuffer& commandBuffer, const DrawDesc& drawDesc) {
    ((CommandBufferMetal&)commandBuffer).CmdDraw(drawDesc);
}

static void NRI_CALL CmdDrawIndexed(CommandBuffer& commandBuffer, const DrawIndexedDesc& drawIndexedDesc) {
    ((CommandBufferMetal&)commandBuffer).CmdDrawIndexed(drawIndexedDesc);
}

static void NRI_CALL CmdDrawIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferMetal&)commandBuffer).CmdDrawIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL CmdDrawIndexedIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferMetal&)commandBuffer).CmdDrawIndexedIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL CmdEndRendering(CommandBuffer& commandBuffer) {
    ((CommandBufferMetal&)commandBuffer).CmdEndRendering();
}

static void NRI_CALL CmdDispatch(CommandBuffer& commandBuffer, const DispatchDesc& dispatchDesc) {
    ((CommandBufferMetal&)commandBuffer).CmdDispatch(dispatchDesc);
}

static void NRI_CALL CmdDispatchIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset) {
    ((CommandBufferMetal&)commandBuffer).CmdDispatchIndirect(buffer, offset);
}

static void NRI_CALL CmdCopyBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    ((CommandBufferMetal&)commandBuffer).CmdCopyBuffer(dstBuffer, dstOffset, srcBuffer, srcOffset, size);
}

static void NRI_CALL CmdCopyTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion) {
    ((CommandBufferMetal&)commandBuffer).CmdCopyTexture(dstTexture, dstRegion, srcTexture, srcRegion);
}

static void NRI_CALL CmdUploadBufferToTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc& dstRegion, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayout) {
    ((CommandBufferMetal&)commandBuffer).CmdUploadBufferToTexture(dstTexture, dstRegion, srcBuffer, srcDataLayout);
}

static void NRI_CALL CmdReadbackTextureToBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayout, const Texture& srcTexture, const TextureRegionDesc& srcRegion) {
    ((CommandBufferMetal&)commandBuffer).CmdReadbackTextureToBuffer(dstBuffer, dstDataLayout, srcTexture, srcRegion);
}

static void NRI_CALL CmdZeroBuffer(CommandBuffer& commandBuffer, Buffer& buffer, uint64_t offset, uint64_t size) {
    ((CommandBufferMetal&)commandBuffer).CmdZeroBuffer(buffer, offset, size);
}

static void NRI_CALL CmdResolveTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegion, const Texture& srcTexture, const TextureRegionDesc* srcRegion, ResolveOp resolveOp) {
    ((CommandBufferMetal&)commandBuffer).CmdResolveTexture(dstTexture, dstRegion, srcTexture, srcRegion, resolveOp);
}

static void NRI_CALL CmdClearStorage(CommandBuffer& commandBuffer, const ClearStorageDesc& clearStorageDesc) {
    ((CommandBufferMetal&)commandBuffer).CmdClearStorage(clearStorageDesc);
}

static void NRI_CALL CmdResetQueries(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset, uint32_t num) {
    ((CommandBufferMetal&)commandBuffer).CmdResetQueries(queryPool, offset, num);
}

static void NRI_CALL CmdBeginQuery(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferMetal&)commandBuffer).CmdBeginQuery(queryPool, offset);
}

static void NRI_CALL CmdEndQuery(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferMetal&)commandBuffer).CmdEndQuery(queryPool, offset);
}

static void NRI_CALL CmdCopyQueries(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    ((CommandBufferMetal&)commandBuffer).CmdCopyQueries(queryPool, offset, num, dstBuffer, dstOffset);
}

static void NRI_CALL CmdBeginAnnotation(CommandBuffer& commandBuffer, const char* name, uint32_t bgra) {
    MaybeUnused(commandBuffer, name, bgra);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferMetal&)commandBuffer).CmdBeginAnnotation(name, bgra);
#endif
}

static void NRI_CALL CmdEndAnnotation(CommandBuffer& commandBuffer) {
    MaybeUnused(commandBuffer);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferMetal&)commandBuffer).CmdEndAnnotation();
#endif
}

static void NRI_CALL CmdAnnotation(CommandBuffer& commandBuffer, const char* name, uint32_t bgra) {
    MaybeUnused(commandBuffer, name, bgra);
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    ((CommandBufferMetal&)commandBuffer).CmdAnnotation(name, bgra);
#endif
}

static Result NRI_CALL EndCommandBuffer(CommandBuffer& commandBuffer) {
    return ((CommandBufferMetal&)commandBuffer).End();
}

// "MTL4CommandQueue" has no debug groups or signposts
static void NRI_CALL QueueBeginAnnotation(Queue& queue, const char* name, uint32_t bgra) {
    MaybeUnused(queue, name, bgra);
}

static void NRI_CALL QueueEndAnnotation(Queue& queue) {
    MaybeUnused(queue);
}

static void NRI_CALL QueueAnnotation(Queue& queue, const char* name, uint32_t bgra) {
    MaybeUnused(queue, name, bgra);
}

static void NRI_CALL GetCalibratedTimestamps(Queue& queue, uint64_t& timestampGPU, uint64_t& timestampCPU) {
    ((QueueMetal&)queue).GetCalibratedTimestamps(timestampGPU, timestampCPU);
}

static void NRI_CALL ResetQueries(QueryPool& queryPool, uint32_t offset, uint32_t num) {
    ((QueryPoolMetal&)queryPool).Reset(offset, num);
}

static uint32_t NRI_CALL GetQuerySize(const QueryPool& queryPool) {
    return ((QueryPoolMetal&)queryPool).GetQuerySize();
}

static Result NRI_CALL QueueSubmit(Queue& queue, const QueueSubmitDesc& queueSubmitDesc) {
    return ((QueueMetal&)queue).Submit(queueSubmitDesc);
}

static Result NRI_CALL DeviceWaitIdle(Device* device) {
    if (!device)
        return Result::SUCCESS;

    return ((DeviceMetal*)device)->WaitIdle();
}

static Result NRI_CALL QueueWaitIdle(Queue* queue) {
    if (!queue)
        return Result::SUCCESS;

    return ((QueueMetal*)queue)->WaitIdle();
}

static void NRI_CALL Wait(Fence& fence, uint64_t value) {
    ((FenceMetal&)fence).Wait(value);
}

static uint64_t NRI_CALL GetFenceValue(Fence& fence) {
    return ((FenceMetal&)fence).GetValue();
}

static void NRI_CALL ResetCommandAllocator(CommandAllocator& commandAllocator) {
    ((CommandAllocatorMetal&)commandAllocator).Reset();
}

static void* NRI_CALL MapBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    return ((BufferMetal&)buffer).Map(offset, size);
}

static void NRI_CALL UnmapBuffer(Buffer& buffer) {
    ((BufferMetal&)buffer).Unmap();
}

static Result NRI_CALL UploadHostMemoryToTexture(Queue& queue, const UploadHostMemoryToTextureDesc* copyDescs, uint32_t copyDescNum) {
    return ((QueueMetal&)queue).UploadHostMemoryToTexture(copyDescs, copyDescNum);
}

static Result NRI_CALL ReadbackTextureToHostMemory(Queue& queue, const ReadbackTextureToHostMemoryDesc* copyDescs, uint32_t copyDescNum) {
    return ((QueueMetal&)queue).ReadbackTextureToHostMemory(copyDescs, copyDescNum);
}

static uint64_t NRI_CALL GetBufferDeviceAddress(const Buffer& buffer) {
    return ((BufferMetal&)buffer).GetGpuAddress();
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

    return ((DeviceMetal*)device)->GetNativeObject();
}

static void* NRI_CALL GetQueueNativeObject(const Queue* queue) {
    if (!queue)
        return nullptr;

    return ((QueueMetal*)queue)->GetNativeObject();
}

static void* NRI_CALL GetCommandBufferNativeObject(const CommandBuffer* commandBuffer) {
    if (!commandBuffer)
        return nullptr;

    return ((CommandBufferMetal*)commandBuffer)->GetNativeObject();
}

static uint64_t NRI_CALL GetBufferNativeObject(const Buffer* buffer) {
    if (!buffer)
        return 0;

    return uint64_t(((BufferMetal*)buffer)->GetNativeObject());
}

static uint64_t NRI_CALL GetTextureNativeObject(const Texture* texture) {
    if (!texture)
        return 0;

    return uint64_t(((TextureMetal*)texture)->GetNativeObject());
}

static uint64_t NRI_CALL GetDescriptorNativeObject(const Descriptor* descriptor) {
    if (!descriptor)
        return 0;

    return ((DescriptorMetal*)descriptor)->GetNativeObject();
}

Result DeviceMetal::FillFunctionTable(CoreInterface& table) const {
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
    table.GetCalibratedTimestamps = ::GetCalibratedTimestamps;
    table.ResetQueries = ::ResetQueries;
    table.QueueSubmit = ::QueueSubmit;
    table.QueueWaitIdle = ::QueueWaitIdle;
    table.DeviceWaitIdle = ::DeviceWaitIdle;
    table.Wait = ::Wait;
    table.ResetCommandAllocator = ::ResetCommandAllocator;
    table.MapBuffer = ::MapBuffer;
    table.UnmapBuffer = ::UnmapBuffer;
    table.UploadHostMemoryToTexture = ::UploadHostMemoryToTexture;
    table.ReadbackTextureToHostMemory = ::ReadbackTextureToHostMemory;
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
#pragma region[  MeshShader  ]

static void NRI_CALL CmdDrawMeshTasks(CommandBuffer& commandBuffer, const DrawMeshTasksDesc& desc) {
    ((CommandBufferMetal&)commandBuffer).CmdDrawMeshTasks(desc);
}

static void NRI_CALL CmdDrawMeshTasksIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferMetal&)commandBuffer).CmdDrawMeshTasksIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

Result DeviceMetal::FillFunctionTable(MeshShaderInterface& table) const {
    if (!m_Desc.features.meshShader)
        return Result::UNSUPPORTED;

    table.CmdDrawMeshTasks = ::CmdDrawMeshTasks;
    table.CmdDrawMeshTasksIndirect = ::CmdDrawMeshTasksIndirect;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  DescriptorHeap  ]

static Result NRI_CALL CreateDescriptorHeap(Device& device, const DescriptorHeapDesc& desc, DescriptorHeap*& heap) {
    return ((DeviceMetal&)device).CreateImplementation<DescriptorPoolMetal>(heap, desc);
}

static void NRI_CALL DestroyDescriptorHeap(DescriptorHeap* heap) {
    if (heap) {
        auto* impl = (DescriptorPoolMetal*)heap;
        Destroy(impl->GetDevice().GetAllocationCallbacks(), impl);
    }
}

static Result NRI_CALL WriteResourceDescriptors(DescriptorHeap& heap, const WriteResourceDescriptorsDesc* descs, uint32_t num) {
    return ((DescriptorPoolMetal&)heap).WriteResourceDescriptors(descs, num);
}

static Result NRI_CALL WriteSamplerDescriptors(DescriptorHeap& heap, const WriteSamplerDescriptorsDesc* descs, uint32_t num) {
    return ((DescriptorPoolMetal&)heap).WriteSamplerDescriptors(descs, num);
}

static void NRI_CALL CmdSetDescriptorHeap(CommandBuffer& commandBuffer, const DescriptorHeap& heap) {
    // Both NRI heap APIs use the same Metal argument-buffer storage and binding.
    ((CommandBufferMetal&)commandBuffer).CmdSetDescriptorPool((const DescriptorPool&)heap);
}

Result DeviceMetal::FillFunctionTable(DescriptorHeapInterface& table) const {
    table.CreateDescriptorHeap = ::CreateDescriptorHeap;
    table.DestroyDescriptorHeap = ::DestroyDescriptorHeap;
    table.WriteResourceDescriptors = ::WriteResourceDescriptors;
    table.WriteSamplerDescriptors = ::WriteSamplerDescriptors;
    table.CmdSetDescriptorHeap = ::CmdSetDescriptorHeap;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Helper  ]

static Result NRI_CALL UploadData(Queue& queue, const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    QueueMetal& queueMetal = (QueueMetal&)queue;
    DeviceMetal& deviceMetal = queueMetal.GetDevice();
    HelperDataUpload helperDataUpload(deviceMetal.GetCoreInterface(), (Device&)deviceMetal, queue);

    return helperDataUpload.UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

static uint32_t NRI_CALL CalculateAllocationNumber(const Device& device, const ResourceGroupDesc& resourceGroupDesc) {
    DeviceMetal& deviceMetal = (DeviceMetal&)device;
    HelperDeviceMemoryAllocator allocator(deviceMetal.GetCoreInterface(), (Device&)device);

    return allocator.CalculateAllocationNumber(resourceGroupDesc);
}

static Result NRI_CALL AllocateAndBindMemory(Device& device, const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    DeviceMetal& deviceMetal = (DeviceMetal&)device;
    HelperDeviceMemoryAllocator allocator(deviceMetal.GetCoreInterface(), device);

    return allocator.AllocateAndBindMemory(resourceGroupDesc, allocations);
}

static Result NRI_CALL QueryVideoMemoryInfo(const Device& device, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) {
    return ((const DeviceMetal&)device).QueryVideoMemoryInfo(memoryLocation, videoMemoryInfo);
}

Result DeviceMetal::FillFunctionTable(HelperInterface& table) const {
    table.CalculateAllocationNumber = ::CalculateAllocationNumber;
    table.AllocateAndBindMemory = ::AllocateAndBindMemory;
    table.UploadData = ::UploadData;
    table.QueryVideoMemoryInfo = ::QueryVideoMemoryInfo;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Streamer  ]

static Result NRI_CALL CreateStreamer(Device& device, const StreamerDesc& streamerDesc, Streamer*& streamer) {
    DeviceMetal& deviceMetal = (DeviceMetal&)device;
    StreamerImpl* impl = Allocate<StreamerImpl>(deviceMetal.GetAllocationCallbacks(), device, deviceMetal.GetCoreInterface());

    if (!impl)
        return Result::OUT_OF_MEMORY;

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

static StreamerCopyBatch NRI_CALL BeginStreamerCopyBatch(Streamer& streamer) {
    return ((StreamerImpl&)streamer).BeginCopyBatch();
}

static Buffer* NRI_CALL GetStreamerConstantBuffer(Streamer& streamer) {
    return ((StreamerImpl&)streamer).GetConstantBuffer();
}

static uint32_t NRI_CALL StreamConstantData(Streamer& streamer, const void* data, uint32_t dataSize) {
    return ((StreamerImpl&)streamer).StreamConstantData(data, dataSize);
}

static void* NRI_CALL StreamHostData(Streamer& streamer, const void* data, uint64_t dataSize, uint32_t placementAlignment) {
    return ((StreamerImpl&)streamer).StreamHostData(data, dataSize, placementAlignment);
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

static void NRI_CALL CmdCopyStreamedData(CommandBuffer& commandBuffer, Streamer& streamer, StreamerCopyBatch copyBatch) {
    ((StreamerImpl&)streamer).CmdCopyStreamedData(commandBuffer, copyBatch);
}

Result DeviceMetal::FillFunctionTable(StreamerInterface& table) const {
    table.CreateStreamer = ::CreateStreamer;
    table.DestroyStreamer = ::DestroyStreamer;
    table.BeginStreamerCopyBatch = ::BeginStreamerCopyBatch;
    table.GetStreamerConstantBuffer = ::GetStreamerConstantBuffer;
    table.StreamBufferData = ::StreamBufferData;
    table.StreamTextureData = ::StreamTextureData;
    table.StreamConstantData = ::StreamConstantData;
    table.StreamHostData = ::StreamHostData;
    table.EndStreamerFrame = ::EndStreamerFrame;
    table.CmdCopyStreamedData = ::CmdCopyStreamedData;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  SwapChain  ]

static Result NRI_CALL CreateSwapChain(Device& device, const SwapChainDesc& swapChainDesc, SwapChain*& swapChain) {
    return ((DeviceMetal&)device).CreateImplementation<SwapChainMetal>(swapChain, swapChainDesc);
}

static void NRI_CALL DestroySwapChain(SwapChain* swapChain) {
    Destroy((SwapChainMetal*)swapChain);
}

static Texture* const* NRI_CALL GetSwapChainTextures(const SwapChain& swapChain, uint32_t& textureNum) {
    return ((SwapChainMetal&)swapChain).GetTextures(textureNum);
}

static Result NRI_CALL GetDisplayDesc(SwapChain& swapChain, DisplayDesc& displayDesc) {
    return ((SwapChainMetal&)swapChain).GetDisplayDesc(displayDesc);
}

static Result NRI_CALL AcquireNextTexture(SwapChain& swapChain, Fence& fence, uint32_t& textureIndex) {
    return ((SwapChainMetal&)swapChain).AcquireNextTexture((FenceMetal&)fence, textureIndex);
}

static Result NRI_CALL WaitForPresent(SwapChain& swapChain, uint64_t presentId) {
    return ((SwapChainMetal&)swapChain).WaitForPresent(presentId);
}

static Result NRI_CALL QueuePresent(SwapChain& swapChain, Fence& fence, uint64_t presentId) {
    return ((SwapChainMetal&)swapChain).Present((FenceMetal&)fence, presentId);
}

Result DeviceMetal::FillFunctionTable(SwapChainInterface& table) const {
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
#pragma region[  Imgui  ]

#if NRI_ENABLE_IMGUI_EXTENSION

static Result NRI_CALL CreateImgui(Device& device, const ImguiDesc& imguiDesc, Imgui*& imgui) {
    DeviceMetal& deviceMetal = (DeviceMetal&)device;
    ImguiImpl* impl = Allocate<ImguiImpl>(deviceMetal.GetAllocationCallbacks(), device, deviceMetal.GetCoreInterface());
    imgui = nullptr;

    if (!impl)
        return Result::OUT_OF_MEMORY;

    Result result = impl->Create(imguiDesc);

    if (result == Result::SUCCESS)
        imgui = (Imgui*)impl;
    else
        Destroy(impl);

    return result;
}

static void NRI_CALL DestroyImgui(Imgui* imgui) {
    Destroy((ImguiImpl*)imgui);
}

static void NRI_CALL CmdCopyImguiData(CommandBuffer& commandBuffer, Streamer& streamer, Imgui& imgui, const CopyImguiDataDesc& copyImguiDataDesc, ImguiRenderData& imguiRenderData) {
    ((ImguiImpl&)imgui).CmdCopyData(commandBuffer, streamer, copyImguiDataDesc, imguiRenderData);
}

static void NRI_CALL CmdDrawImgui(CommandBuffer& commandBuffer, const ImguiRenderData& imguiRenderData, const DrawImguiDesc& drawImguiDesc) {
    ((ImguiImpl&)*imguiRenderData.imgui).CmdDraw(commandBuffer, imguiRenderData, drawImguiDesc);
}

Result DeviceMetal::FillFunctionTable(ImguiInterface& table) const {
    table.CreateImgui = ::CreateImgui;
    table.DestroyImgui = ::DestroyImgui;
    table.CmdCopyImguiData = ::CmdCopyImguiData;
    table.CmdDrawImgui = ::CmdDrawImgui;

    return Result::SUCCESS;
}

#endif

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  Upscaler  ]

static Result NRI_CALL CreateUpscaler(Device& device, const UpscalerDesc& upscalerDesc, Upscaler*& upscaler) {
    DeviceMetal& deviceMetal = (DeviceMetal&)device;
    UpscalerImpl* impl = Allocate<UpscalerImpl>(deviceMetal.GetAllocationCallbacks(), device, deviceMetal.GetCoreInterface());
    Result result = impl->Create(upscalerDesc);

    if (result != Result::SUCCESS) {
        Destroy(deviceMetal.GetAllocationCallbacks(), impl);
        upscaler = nullptr;
    } else
        upscaler = (Upscaler*)impl;

    return result;
}

static void NRI_CALL DestroyUpscaler(Upscaler* upscaler) {
    Destroy((UpscalerImpl*)upscaler);
}

static bool NRI_CALL IsUpscalerSupported(const Device& device, UpscalerType upscalerType) {
    DeviceMetal& deviceMetal = (DeviceMetal&)device;

    return IsUpscalerSupported(deviceMetal.GetDesc(), upscalerType);
}

static void NRI_CALL GetUpscalerProps(const Upscaler& upscaler, UpscalerProps& upscalerProps) {
    UpscalerImpl& upscalerImpl = (UpscalerImpl&)upscaler;

    return upscalerImpl.GetUpscalerProps(upscalerProps);
}

static void NRI_CALL CmdDispatchUpscale(CommandBuffer& commandBuffer, Upscaler& upscaler, const DispatchUpscaleDesc& dispatchUpscalerDesc) {
    ((UpscalerImpl&)upscaler).CmdDispatchUpscale(commandBuffer, dispatchUpscalerDesc);
}

Result DeviceMetal::FillFunctionTable(UpscalerInterface& table) const {
    table.CreateUpscaler = ::CreateUpscaler;
    table.DestroyUpscaler = ::DestroyUpscaler;
    table.IsUpscalerSupported = ::IsUpscalerSupported;
    table.GetUpscalerProps = ::GetUpscalerProps;
    table.CmdDispatchUpscale = ::CmdDispatchUpscale;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
#pragma region[  WrapperMetal  ]

static Result NRI_CALL CreateBufferMetal(Device& device, const BufferMetalDesc& desc, Buffer*& buffer) {
    return ((DeviceMetal&)device).CreateImplementation<BufferMetal>(buffer, desc);
}

static Result NRI_CALL CreateTextureMetal(Device& device, const TextureMetalDesc& desc, Texture*& texture) {
    return ((DeviceMetal&)device).CreateImplementation<TextureMetal>(texture, desc);
}

static Result NRI_CALL CreateFenceMetal(Device& device, const FenceMetalDesc& desc, Fence*& fence) {
    return ((DeviceMetal&)device).CreateImplementation<FenceMetal>(fence, desc);
}

Result DeviceMetal::FillFunctionTable(WrapperMetalInterface& table) const {
    table.CreateBufferMetal = ::CreateBufferMetal;
    table.CreateTextureMetal = ::CreateTextureMetal;
    table.CreateFenceMetal = ::CreateFenceMetal;

    return Result::SUCCESS;
}

#pragma endregion

//============================================================================================================================================================================================
static bool FillAdapterDescMetal(AdapterDesc& adapterDesc, MTL::Device* device) {
    if (!device || !device->supportsFamily(MTL::GPUFamilyMetal4))
        return false;

    AutoreleasePoolMetal autoreleasePool;

    adapterDesc = {};
    adapterDesc.uid.low = device->registryID(); // "LUID"-like: unique and stable while the system runs
    adapterDesc.videoMemorySize = device->recommendedMaxWorkingSetSize();
    adapterDesc.sharedSystemMemorySize = device->hasUnifiedMemory() ? 0 : adapterDesc.videoMemorySize; // unified memory is already reported as video memory
    adapterDesc.vendor = Vendor::UNKNOWN;                                                              // Metal exposes no PCI vendor or device IDs, "deviceId" stays 0
    adapterDesc.architecture = device->hasUnifiedMemory() ? Architecture::INTEGRATED : Architecture::DISCRETE;
    adapterDesc.supportedGraphicsAPIs = GraphicsAPI::METAL;
    adapterDesc.queueNum[(uint32_t)QueueType::GRAPHICS] = QUEUE_NUM_PER_TYPE_METAL;
    adapterDesc.queueNum[(uint32_t)QueueType::COMPUTE] = QUEUE_NUM_PER_TYPE_METAL;
    adapterDesc.queueNum[(uint32_t)QueueType::COPY] = QUEUE_NUM_PER_TYPE_METAL;
    strncpy(adapterDesc.name, device->name()->utf8String(), sizeof(adapterDesc.name) - 1);

    return true;
}

void UpdateAdaptersMetal(AdapterDesc* adapterDescs, uint32_t& adapterDescNum, uint32_t adapterDescMaxNum) {
    NS::Array* devices = MTL::CopyAllDevices();

    if (!devices)
        return;

    for (NS::UInteger i = 0; i < devices->count(); i++) {
        AdapterDesc metalAdapterDesc = {};

        if (!FillAdapterDescMetal(metalAdapterDesc, devices->object<MTL::Device>(i)))
            continue;

        // Merge with the same adapter exposed by another API (i.e. MoltenVK). "deviceId" can't be used since it's 0
        bool isMerged = false;

        for (uint32_t j = 0; j < adapterDescNum && !isMerged; j++) {
            AdapterDesc& adapterDesc = adapterDescs[j];

            if (strcmp(adapterDesc.name, metalAdapterDesc.name) == 0 || (adapterDesc.uid.low == metalAdapterDesc.uid.low && adapterDesc.uid.high == 0)) {
                adapterDesc.supportedGraphicsAPIs |= GraphicsAPI::METAL;
                isMerged = true;
            }
        }

        if (!isMerged && adapterDescNum < adapterDescMaxNum)
            adapterDescs[adapterDescNum++] = metalAdapterDesc;
    }

    devices->release();
}

bool GetAdapterDescMetal(AdapterDesc& adapterDesc, void* device) {
    return FillAdapterDescMetal(adapterDesc, (MTL::Device*)device);
}

Result CreateDeviceMetal(const DeviceCreationDesc& desc, const DeviceCreationMetalDesc& descMetal, DeviceBase*& device) {
    DeviceMetal* impl = Allocate<DeviceMetal>(desc.allocationCallbacks, desc.callbackInterface, desc.allocationCallbacks);
    Result result = impl->Create(desc, descMetal);

    if (result != Result::SUCCESS) {
        Destroy(desc.allocationCallbacks, impl);
        device = nullptr;
    } else
        device = (DeviceBase*)impl;

    return result;
}
