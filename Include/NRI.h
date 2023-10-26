/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include <stdint.h>
#include <stddef.h>

#define NRI_VERSION_MAJOR 1
#define NRI_VERSION_MINOR 100
#define NRI_VERSION_DATE "25 October 2023"

#ifdef _WIN32
    #define NRI_CALL __fastcall
#else
    #define NRI_CALL
#endif

#ifndef NRI_API
    #if NRI_STATIC_LIBRARY
        #define NRI_API
    #elif( defined(__cplusplus) )
        #define NRI_API extern "C"
    #else
        #define NRI_API extern
    #endif
#endif

#include "NRIDescs.h"

NRI_NAMESPACE_BEGIN

NRI_STRUCT(CoreInterface)
{
    // Get
    const NRI_NAME_REF(DeviceDesc) (NRI_CALL *GetDeviceDesc)(const NRI_NAME_REF(Device) device);
    NRI_NAME(FormatSupportBits) (NRI_CALL *GetFormatSupport)(const NRI_NAME_REF(Device) device, NRI_NAME(Format) format);
    uint32_t (NRI_CALL *GetQuerySize)(const NRI_NAME_REF(QueryPool) queryPool);
    void (NRI_CALL *GetBufferMemoryInfo)(const NRI_NAME_REF(Buffer) buffer, NRI_NAME(MemoryLocation) memoryLocation, NRI_NAME_REF(MemoryDesc) memoryDesc);
    void (NRI_CALL *GetTextureMemoryInfo)(const NRI_NAME_REF(Texture) texture, NRI_NAME(MemoryLocation) memoryLocation, NRI_NAME_REF(MemoryDesc) memoryDesc);
    NRI_NAME(Result) (NRI_CALL *GetCommandQueue)(NRI_NAME_REF(Device) device, NRI_NAME(CommandQueueType) commandQueueType, NRI_NAME_REF(CommandQueue*) commandQueue);

    // Create
    NRI_NAME(Result) (NRI_CALL *CreateCommandAllocator)(const NRI_NAME_REF(CommandQueue) commandQueue, uint32_t physicalDeviceMask, NRI_NAME_REF(CommandAllocator*) commandAllocator);
    NRI_NAME(Result) (NRI_CALL *CreateCommandBuffer)(NRI_NAME_REF(CommandAllocator) commandAllocator, NRI_NAME_REF(CommandBuffer*) commandBuffer);
    NRI_NAME(Result) (NRI_CALL *CreateDescriptorPool)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(DescriptorPoolDesc) descriptorPoolDesc, NRI_NAME_REF(DescriptorPool*) descriptorPool);
    NRI_NAME(Result) (NRI_CALL *CreateBuffer)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(BufferDesc) bufferDesc, NRI_NAME_REF(Buffer*) buffer);
    NRI_NAME(Result) (NRI_CALL *CreateTexture)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(TextureDesc) textureDesc, NRI_NAME_REF(Texture*) texture);
    NRI_NAME(Result) (NRI_CALL *CreateBufferView)(const NRI_NAME_REF(BufferViewDesc) bufferViewDesc, NRI_NAME_REF(Descriptor*) bufferView);
    NRI_NAME(Result) (NRI_CALL *CreateTexture1DView)(const NRI_NAME_REF(Texture1DViewDesc) textureViewDesc, NRI_NAME_REF(Descriptor*) textureView);
    NRI_NAME(Result) (NRI_CALL *CreateTexture2DView)(const NRI_NAME_REF(Texture2DViewDesc) textureViewDesc, NRI_NAME_REF(Descriptor*) textureView);
    NRI_NAME(Result) (NRI_CALL *CreateTexture3DView)(const NRI_NAME_REF(Texture3DViewDesc) textureViewDesc, NRI_NAME_REF(Descriptor*) textureView);
    NRI_NAME(Result) (NRI_CALL *CreateSampler)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(SamplerDesc) samplerDesc, NRI_NAME_REF(Descriptor*) sampler);
    NRI_NAME(Result) (NRI_CALL *CreatePipelineLayout)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(PipelineLayoutDesc) pipelineLayoutDesc, NRI_NAME_REF(PipelineLayout*) pipelineLayout);
    NRI_NAME(Result) (NRI_CALL *CreateGraphicsPipeline)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(GraphicsPipelineDesc) graphicsPipelineDesc, NRI_NAME_REF(Pipeline*) pipeline);
    NRI_NAME(Result) (NRI_CALL *CreateComputePipeline)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(ComputePipelineDesc) computePipelineDesc, NRI_NAME_REF(Pipeline*) pipeline);
    NRI_NAME(Result) (NRI_CALL *CreateFrameBuffer)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(FrameBufferDesc) frameBufferDesc, NRI_NAME_REF(FrameBuffer*) frameBuffer);
    NRI_NAME(Result) (NRI_CALL *CreateQueryPool)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(QueryPoolDesc) queryPoolDesc, NRI_NAME_REF(QueryPool*) queryPool);
    NRI_NAME(Result) (NRI_CALL *CreateFence)(NRI_NAME_REF(Device) device, uint64_t initialValue, NRI_NAME_REF(Fence*) fence);

    // Destroy
    void (NRI_CALL *DestroyCommandAllocator)(NRI_NAME_REF(CommandAllocator) commandAllocator);
    void (NRI_CALL *DestroyCommandBuffer)(NRI_NAME_REF(CommandBuffer) commandBuffer);
    void (NRI_CALL *DestroyDescriptorPool)(NRI_NAME_REF(DescriptorPool) descriptorPool);
    void (NRI_CALL *DestroyBuffer)(NRI_NAME_REF(Buffer) buffer);
    void (NRI_CALL *DestroyTexture)(NRI_NAME_REF(Texture) texture);
    void (NRI_CALL *DestroyDescriptor)(NRI_NAME_REF(Descriptor) descriptor);
    void (NRI_CALL *DestroyPipelineLayout)(NRI_NAME_REF(PipelineLayout) pipelineLayout);
    void (NRI_CALL *DestroyPipeline)(NRI_NAME_REF(Pipeline) pipeline);
    void (NRI_CALL *DestroyFrameBuffer)(NRI_NAME_REF(FrameBuffer) frameBuffer);
    void (NRI_CALL *DestroyQueryPool)(NRI_NAME_REF(QueryPool) queryPool);
    void (NRI_CALL *DestroyFence)(NRI_NAME_REF(Fence) fence);

    // Memory
    NRI_NAME(Result) (NRI_CALL *AllocateMemory)(NRI_NAME_REF(Device) device, uint32_t physicalDeviceMask, NRI_NAME(MemoryType) memoryType, uint64_t size, NRI_NAME_REF(Memory*) memory);
    NRI_NAME(Result) (NRI_CALL *BindBufferMemory)(NRI_NAME_REF(Device) device, const NRI_NAME(BufferMemoryBindingDesc)* memoryBindingDescs, uint32_t memoryBindingDescNum);
    NRI_NAME(Result) (NRI_CALL *BindTextureMemory)(NRI_NAME_REF(Device) device, const NRI_NAME(TextureMemoryBindingDesc)* memoryBindingDescs, uint32_t memoryBindingDescNum);
    void (NRI_CALL *FreeMemory)(NRI_NAME_REF(Memory) memory);

    // Command buffer commands
    NRI_NAME(Result) (NRI_CALL *BeginCommandBuffer)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME(DescriptorPool)* descriptorPool, uint32_t physicalDeviceIndex);
    NRI_NAME(Result) (NRI_CALL *EndCommandBuffer)(NRI_NAME_REF(CommandBuffer) commandBuffer);

    void (NRI_CALL *CmdSetPipeline)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(Pipeline) pipeline);
    void (NRI_CALL *CmdSetPipelineLayout)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(PipelineLayout) pipelineLayout);
    void (NRI_CALL *CmdSetDescriptorSet)(NRI_NAME_REF(CommandBuffer) commandBuffer, uint32_t setIndexInPipelineLayout, const NRI_NAME_REF(DescriptorSet) descriptorSet, const uint32_t* dynamicConstantBufferOffsets);
    void (NRI_CALL *CmdSetConstants)(NRI_NAME_REF(CommandBuffer) commandBuffer, uint32_t pushConstantIndex, const void* data, uint32_t size);
    void (NRI_CALL *CmdSetDescriptorPool)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(DescriptorPool) descriptorPool);
    void (NRI_CALL *CmdPipelineBarrier)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME(TransitionBarrierDesc)* transitionBarriers, const NRI_NAME(AliasingBarrierDesc)* aliasingBarriers, NRI_NAME(BarrierDependency) dependency);

    void (NRI_CALL *CmdBeginRenderPass)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(FrameBuffer) frameBuffer, NRI_NAME(RenderPassBeginFlag) renderPassBeginFlag);
    void (NRI_CALL *CmdEndRenderPass)(NRI_NAME_REF(CommandBuffer) commandBuffer);
    void (NRI_CALL *CmdSetViewports)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME(Viewport)* viewports, uint32_t viewportNum);
    void (NRI_CALL *CmdSetScissors)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME(Rect)* rects, uint32_t rectNum);
    void (NRI_CALL *CmdSetDepthBounds)(NRI_NAME_REF(CommandBuffer) commandBuffer, float boundsMin, float boundsMax);
    void (NRI_CALL *CmdSetStencilReference)(NRI_NAME_REF(CommandBuffer) commandBuffer, uint8_t reference);
    void (NRI_CALL *CmdSetSamplePositions)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME(SamplePosition)* positions, uint32_t positionNum);
    void (NRI_CALL *CmdClearAttachments)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME(ClearDesc)* clearDescs, uint32_t clearDescNum, const NRI_NAME(Rect)* rects, uint32_t rectNum);
    void (NRI_CALL *CmdSetIndexBuffer)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(Buffer) buffer, uint64_t offset, NRI_NAME(IndexType) indexType);
    void (NRI_CALL *CmdSetVertexBuffers)(NRI_NAME_REF(CommandBuffer) commandBuffer, uint32_t baseSlot, uint32_t bufferNum, const NRI_NAME(Buffer)* const* buffers, const uint64_t* offsets);

    void (NRI_CALL *CmdDraw)(NRI_NAME_REF(CommandBuffer) commandBuffer, uint32_t vertexNum, uint32_t instanceNum, uint32_t baseVertex, uint32_t baseInstance);
    void (NRI_CALL *CmdDrawIndexed)(NRI_NAME_REF(CommandBuffer) commandBuffer, uint32_t indexNum, uint32_t instanceNum, uint32_t baseIndex, uint32_t baseVertex, uint32_t baseInstance);
    void (NRI_CALL *CmdDrawIndirect)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(Buffer) buffer, uint64_t offset, uint32_t drawNum, uint32_t stride);
    void (NRI_CALL *CmdDrawIndexedIndirect)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(Buffer) buffer, uint64_t offset, uint32_t drawNum, uint32_t stride);
    void (NRI_CALL *CmdDispatch)(NRI_NAME_REF(CommandBuffer) commandBuffer, uint32_t x, uint32_t y, uint32_t z);
    void (NRI_CALL *CmdDispatchIndirect)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(Buffer) buffer, uint64_t offset);
    void (NRI_CALL *CmdBeginQuery)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(QueryPool) queryPool, uint32_t offset);
    void (NRI_CALL *CmdEndQuery)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(QueryPool) queryPool, uint32_t offset);
    void (NRI_CALL *CmdBeginAnnotation)(NRI_NAME_REF(CommandBuffer) commandBuffer, const char* name);
    void (NRI_CALL *CmdEndAnnotation)(NRI_NAME_REF(CommandBuffer) commandBuffer);

    void (NRI_CALL *CmdClearStorageBuffer)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(ClearStorageBufferDesc) clearDesc);
    void (NRI_CALL *CmdClearStorageTexture)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(ClearStorageTextureDesc) clearDesc);
    void (NRI_CALL *CmdCopyBuffer)(NRI_NAME_REF(CommandBuffer) commandBuffer, NRI_NAME_REF(Buffer) dstBuffer, uint32_t dstPhysicalDeviceIndex, uint64_t dstOffset, const NRI_NAME_REF(Buffer) srcBuffer, uint32_t srcPhysicalDeviceIndex, uint64_t srcOffset, uint64_t size);
    void (NRI_CALL *CmdCopyTexture)(NRI_NAME_REF(CommandBuffer) commandBuffer, NRI_NAME_REF(Texture) dstTexture, uint32_t dstPhysicalDeviceIndex, const NRI_NAME(TextureRegionDesc)* dstRegionDesc, const NRI_NAME_REF(Texture) srcTexture, uint32_t srcPhysicalDeviceIndex, const NRI_NAME(TextureRegionDesc)* srcRegionDesc);
    void (NRI_CALL *CmdUploadBufferToTexture)(NRI_NAME_REF(CommandBuffer) commandBuffer, NRI_NAME_REF(Texture) dstTexture, const NRI_NAME_REF(TextureRegionDesc) dstRegionDesc, const NRI_NAME_REF(Buffer) srcBuffer, const NRI_NAME_REF(TextureDataLayoutDesc) srcDataLayoutDesc);
    void (NRI_CALL *CmdReadbackTextureToBuffer)(NRI_NAME_REF(CommandBuffer) commandBuffer, NRI_NAME_REF(Buffer) dstBuffer, NRI_NAME_REF(TextureDataLayoutDesc) dstDataLayoutDesc, const NRI_NAME_REF(Texture) srcTexture, const NRI_NAME_REF(TextureRegionDesc) srcRegionDesc);
    void (NRI_CALL *CmdCopyQueries)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(QueryPool) queryPool, uint32_t offset, uint32_t num, NRI_NAME_REF(Buffer) dstBuffer, uint64_t dstOffset);
    void (NRI_CALL *CmdResetQueries)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(QueryPool) queryPool, uint32_t offset, uint32_t num);

    // Synchronization
    uint64_t (NRI_CALL *GetFenceValue)(NRI_NAME_REF(Fence) fence);
    void (NRI_CALL *QueueSignal)(NRI_NAME_REF(CommandQueue) commandQueue, NRI_NAME_REF(Fence) fence, uint64_t value);
    void (NRI_CALL *QueueWait)(NRI_NAME_REF(CommandQueue) commandQueue, NRI_NAME_REF(Fence) fence, uint64_t value);
    void (NRI_CALL *Wait)(NRI_NAME_REF(Fence) fence, uint64_t value);

    // Work submission
    void (NRI_CALL *QueueSubmit)(NRI_NAME_REF(CommandQueue) commandQueue, const NRI_NAME_REF(QueueSubmitDesc) queueSubmitDesc);

    // Descriptor set
    void (NRI_CALL *UpdateDescriptorRanges)(NRI_NAME_REF(DescriptorSet) descriptorSet, uint32_t physicalDeviceMask, uint32_t baseRange, uint32_t rangeNum, const NRI_NAME(DescriptorRangeUpdateDesc)* rangeUpdateDescs);
    void (NRI_CALL *UpdateDynamicConstantBuffers)(NRI_NAME_REF(DescriptorSet) descriptorSet, uint32_t physicalDeviceMask, uint32_t baseBuffer, uint32_t bufferNum, const NRI_NAME(Descriptor)* const* descriptors);
    void (NRI_CALL *CopyDescriptorSet)(NRI_NAME_REF(DescriptorSet) descriptorSet, const NRI_NAME_REF(DescriptorSetCopyDesc) descriptorSetCopyDesc);

    // Descriptor pool
    NRI_NAME(Result) (NRI_CALL *AllocateDescriptorSets)(NRI_NAME_REF(DescriptorPool) descriptorPool, const NRI_NAME_REF(PipelineLayout) pipelineLayout, uint32_t setIndexInPipelineLayout, NRI_NAME(DescriptorSet)** descriptorSets, uint32_t instanceNum, uint32_t physicalDeviceMask, uint32_t variableDescriptorNum);
    void (NRI_CALL *ResetDescriptorPool)(NRI_NAME_REF(DescriptorPool) descriptorPool);

    // Command allocator
    void (NRI_CALL *ResetCommandAllocator)(NRI_NAME_REF(CommandAllocator) commandAllocator);

    // Map / Unmap
    void* (NRI_CALL *MapBuffer)(NRI_NAME_REF(Buffer) buffer, uint64_t offset, uint64_t size);
    void (NRI_CALL *UnmapBuffer)(NRI_NAME_REF(Buffer) buffer);

    // Debug name
    void (NRI_CALL *SetDeviceDebugName)(NRI_NAME_REF(Device) device, const char* name);
    void (NRI_CALL *SetCommandQueueDebugName)(NRI_NAME_REF(CommandQueue) commandQueue, const char* name);
    void (NRI_CALL *SetFenceDebugName)(NRI_NAME_REF(Fence) fence, const char* name);
    void (NRI_CALL *SetCommandAllocatorDebugName)(NRI_NAME_REF(CommandAllocator) commandAllocator, const char* name);
    void (NRI_CALL *SetDescriptorPoolDebugName)(NRI_NAME_REF(DescriptorPool) descriptorPool, const char* name);
    void (NRI_CALL *SetBufferDebugName)(NRI_NAME_REF(Buffer) buffer, const char* name);
    void (NRI_CALL *SetTextureDebugName)(NRI_NAME_REF(Texture) texture, const char* name);
    void (NRI_CALL *SetDescriptorDebugName)(NRI_NAME_REF(Descriptor) descriptor, const char* name);
    void (NRI_CALL *SetPipelineLayoutDebugName)(NRI_NAME_REF(PipelineLayout) pipelineLayout, const char* name);
    void (NRI_CALL *SetPipelineDebugName)(NRI_NAME_REF(Pipeline) pipeline, const char* name);
    void (NRI_CALL *SetFrameBufferDebugName)(NRI_NAME_REF(FrameBuffer) frameBuffer, const char* name);
    void (NRI_CALL *SetQueryPoolDebugName)(NRI_NAME_REF(QueryPool) queryPool, const char* name);
    void (NRI_CALL *SetDescriptorSetDebugName)(NRI_NAME_REF(DescriptorSet) descriptorSet, const char* name);
    void (NRI_CALL *SetCommandBufferDebugName)(NRI_NAME_REF(CommandBuffer) commandBuffer, const char* name);
    void (NRI_CALL *SetMemoryDebugName)(NRI_NAME_REF(Memory) memory, const char* name);

    // Native objects
    void* (NRI_CALL *GetDeviceNativeObject)(const NRI_NAME_REF(Device) device); // ID3D11Device*, ID3D12Device* or VkDevice
    void* (NRI_CALL *GetCommandBufferNativeObject)(const NRI_NAME_REF(CommandBuffer) commandBuffer); // ID3D11DeviceContext*, ID3D12GraphicsCommandList* or VkCommandBuffer
    uint64_t (NRI_CALL *GetBufferNativeObject)(const NRI_NAME_REF(Buffer) buffer, uint32_t physicalDeviceIndex); // ID3D11Buffer*, ID3D12Resource* or VkBuffer
    uint64_t (NRI_CALL *GetTextureNativeObject)(const NRI_NAME_REF(Texture) texture, uint32_t physicalDeviceIndex); // ID3D11Resource*, ID3D12Resource* or VkImage
    uint64_t (NRI_CALL *GetDescriptorNativeObject)(const NRI_NAME_REF(Descriptor) descriptor, uint32_t physicalDeviceIndex); // ID3D11View*, D3D12_CPU_DESCRIPTOR_HANDLE or VkImageView/VkBufferView
};

NRI_API NRI_NAME(Result) NRI_CALL nriGetInterface(const NRI_NAME_REF(Device) device, const char* interfaceName, size_t interfaceSize, void* interfacePtr);

NRI_NAMESPACE_END
