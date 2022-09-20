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

struct ID3D11Device;
struct ID3D11Resource;
struct ID3D11DeviceContext;

namespace nri
{
    struct DeviceCreationD3D11Desc
    {
        ID3D11Device* d3d11Device;
        void* agsContextAssociatedWithDevice;
        CallbackInterface callbackInterface;
        MemoryAllocatorInterface memoryAllocatorInterface;
        bool enableNRIValidation;
        bool enableAPIValidation;
    };

    struct CommandBufferD3D11Desc
    {
        ID3D11DeviceContext* d3d11DeviceContext;
    };

    struct BufferD3D11Desc
    {
        ID3D11Resource* d3d11Resource;
    };

    struct TextureD3D11Desc
    {
        ID3D11Resource* d3d11Resource;
    };

    NRI_API Result NRI_CALL CreateDeviceFromD3D11Device(const DeviceCreationD3D11Desc& deviceDesc, Device*& device);
    NRI_API Format NRI_CALL ConvertDXGIFormatToNRI(uint32_t dxgiFormat);
    NRI_API uint32_t NRI_CALL ConvertNRIFormatToDXGI(Format format);

    struct WrapperD3D11Interface
    {
        Result (NRI_CALL *CreateCommandBufferD3D11)(Device& device, const CommandBufferD3D11Desc& commandBufferDesc, CommandBuffer*& commandBuffer);
        Result (NRI_CALL *CreateBufferD3D11)(Device& device, const BufferD3D11Desc& bufferDesc, Buffer*& buffer);
        Result (NRI_CALL *CreateTextureD3D11)(Device& device, const TextureD3D11Desc& textureDesc, Texture*& texture);
    };
}
