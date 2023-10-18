/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include "NRIDeviceCreation.h"

NRI_FORWARD_STRUCT(ID3D12Device);
NRI_FORWARD_STRUCT(ID3D12CommandQueue);
NRI_FORWARD_STRUCT(ID3D12Resource);
NRI_FORWARD_STRUCT(ID3D12Heap);
NRI_FORWARD_STRUCT(ID3D12GraphicsCommandList);
NRI_FORWARD_STRUCT(ID3D12CommandAllocator);

NRI_NAMESPACE_BEGIN

NRI_FORWARD_STRUCT(AccelerationStructure);

NRI_STRUCT(DeviceCreationD3D12Desc)
{
    ID3D12Device* d3d12Device;
    ID3D12CommandQueue* d3d12GraphicsQueue;
    ID3D12CommandQueue* d3d12ComputeQueue;
    ID3D12CommandQueue* d3d12CopyQueue;
    NRI_NAME(CallbackInterface) callbackInterface;
    NRI_NAME(MemoryAllocatorInterface) memoryAllocatorInterface;
    bool enableNRIValidation;
};

NRI_STRUCT(CommandBufferD3D12Desc)
{
    ID3D12GraphicsCommandList* d3d12CommandList;
    ID3D12CommandAllocator* d3d12CommandAllocator;
};

NRI_STRUCT(BufferD3D12Desc)
{
    ID3D12Resource* d3d12Resource;
    uint32_t structureStride;
};

NRI_STRUCT(TextureD3D12Desc)
{
    ID3D12Resource* d3d12Resource;
};

NRI_STRUCT(MemoryD3D12Desc)
{
    ID3D12Heap* d3d12Heap;
};

NRI_STRUCT(AccelerationStructureD3D12Desc)
{
    ID3D12Resource* d3d12Resource;
    uint64_t scratchDataSizeInBytes;
    uint64_t updateScratchDataSizeInBytes;
};

NRI_STRUCT(WrapperD3D12Interface)
{
    NRI_NAME(Result) (NRI_CALL *CreateCommandBufferD3D12)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(CommandBufferD3D12Desc) commandBufferDesc, NRI_NAME_REF(CommandBuffer*) commandBuffer);
    NRI_NAME(Result) (NRI_CALL *CreateBufferD3D12)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(BufferD3D12Desc) bufferDesc, NRI_NAME_REF(Buffer*) buffer);
    NRI_NAME(Result) (NRI_CALL *CreateTextureD3D12)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(TextureD3D12Desc) textureDesc, NRI_NAME_REF(Texture*) texture);
    NRI_NAME(Result) (NRI_CALL *CreateMemoryD3D12)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(MemoryD3D12Desc) memoryDesc, NRI_NAME_REF(Memory*) memory);
    NRI_NAME(Result) (NRI_CALL *CreateAccelerationStructureD3D12)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(AccelerationStructureD3D12Desc) accelerationStructureDesc, NRI_NAME_REF(AccelerationStructure*) accelerationStructure);
};

NRI_API NRI_NAME(Result) NRI_CALL nriCreateDeviceFromD3D12Device(const NRI_NAME_REF(DeviceCreationD3D12Desc) deviceDesc, NRI_NAME_REF(Device*) device);

NRI_NAMESPACE_END
