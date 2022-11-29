/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

NRI_NAMESPACE_BEGIN

NRI_STRUCT(TextureSubresourceUploadDesc)
{
    const void* slices;
    uint32_t sliceNum;
    uint32_t rowPitch;
    uint32_t slicePitch;
};

NRI_STRUCT(TextureUploadDesc)
{
    const NRI_NAME(TextureSubresourceUploadDesc)* subresources;
    NRI_NAME(Texture)* texture;
    NRI_NAME(AccessBits) nextAccess;
    NRI_NAME(TextureLayout) nextLayout;
    uint16_t mipNum;
    uint16_t arraySize;
};

NRI_STRUCT(BufferUploadDesc)
{
    const void* data;
    uint64_t dataSize;
    NRI_NAME(Buffer)* buffer;
    uint64_t bufferOffset;
    NRI_NAME(AccessBits) prevAccess;
    NRI_NAME(AccessBits) nextAccess;
};

NRI_STRUCT(ResourceGroupDesc)
{
    NRI_NAME(MemoryLocation) memoryLocation;
    NRI_NAME(Texture)* const* textures;
    uint32_t textureNum;
    NRI_NAME(Buffer)* const* buffers;
    uint32_t bufferNum;
};

NRI_STRUCT(HelperInterface)
{
    uint32_t (NRI_CALL *CalculateAllocationNumber)(NRI_REF_NAME(Device) device, const NRI_REF_NAME(ResourceGroupDesc) resourceGroupDesc);
    NRI_NAME(Result) (NRI_CALL *AllocateAndBindMemory)(NRI_REF_NAME(Device) device, const NRI_REF_NAME(ResourceGroupDesc) resourceGroupDesc, NRI_NAME(Memory)** allocations);
    NRI_NAME(Result) (NRI_CALL *ChangeResourceStates)(NRI_REF_NAME(CommandQueue) commandQueue, const NRI_REF_NAME(TransitionBarrierDesc) transitionBarriers);
    NRI_NAME(Result) (NRI_CALL *UploadData)(NRI_REF_NAME(CommandQueue) commandQueue, const NRI_NAME(TextureUploadDesc)* textureUploadDescs, uint32_t textureUploadDescNum,
        const NRI_NAME(BufferUploadDesc)* bufferUploadDescs, uint32_t bufferUploadDescNum);
    NRI_NAME(Result) (NRI_CALL *WaitForIdle)(NRI_REF_NAME(CommandQueue) commandQueue);
};

NRI_NAMESPACE_END
