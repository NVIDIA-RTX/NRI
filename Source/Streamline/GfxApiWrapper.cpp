/*
Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include <Extensions/Optional/NRIGfxApiWrapper.h>

#include <d3d12.h>

#include "../Shared/SharedExternal.h"
#include "../D3D12/CommandBufferD3D12.h"
#include "../D3D12/BufferD3D12.h"
#include "../D3D12/TextureD3D12.h"

#define STREAMLINE_ABORT_ON_FAILURE(result) \
    if ((result) != nri::Result::SUCCESS) \
        throw _STD exception{ "Unknown problem occured." };

#pragma warning( disable : 4100 )

class D3D12ApiWrapper : public GfxApiWrapperInterface
{
    nri::WrapperD3D12Interface wrapper;

public:

    virtual Resource* GetNative(nri::Texture const& texture) override;
    virtual Resource* GetNative(nri::Buffer const& buffer) override;
    virtual CommandList* GetNative(nri::CommandBuffer const& buffer) override;

    D3D12ApiWrapper(nri::Device const& device)
    {
        STREAMLINE_ABORT_ON_FAILURE(nri::GetInterface(device, NRI_INTERFACE(nri::WrapperD3D12Interface), &wrapper));
    }
};

class D3D11ApiWrapper : public GfxApiWrapperInterface
{
    nri::WrapperD3D11Interface wrapper;

public:

    virtual Resource* GetNative(nri::Texture const& texture) override 
    {
        return nullptr;
    }
    virtual Resource* GetNative(nri::Buffer const& buffer) override 
    {
        return nullptr;
    }
    virtual CommandList* GetNative(nri::CommandBuffer const& buffer) override 
    {
        return nullptr;
    }

    D3D11ApiWrapper(nri::Device const& device)
    {
        STREAMLINE_ABORT_ON_FAILURE(nri::GetInterface(device, NRI_INTERFACE(nri::WrapperD3D11Interface), &wrapper));
    }
};

std::shared_ptr<GfxApiWrapperInterface> createGfxApiWrapper(nri::GraphicsAPI graphicsAPI, nri::Device const& device)
{
    switch (graphicsAPI)
    {
    case nri::GraphicsAPI::D3D11: return std::make_shared<D3D11ApiWrapper>(device);
    case nri::GraphicsAPI::D3D12: return std::make_shared<D3D12ApiWrapper>(device);
    case nri::GraphicsAPI::VULKAN:
    default: throw std::exception{ "Unknown or unsupported graphics api." };
    }
}

inline auto D3D12ApiWrapper::GetNative(nri::Texture const& texture) -> Resource*
{
    nri::TextureD3D12 const& nriTexture = reinterpret_cast<nri::TextureD3D12 const&>(texture);
    return static_cast<ID3D12Resource*>(nriTexture);
}

inline auto D3D12ApiWrapper::GetNative(nri::Buffer const& buffer) -> Resource*
{
    nri::BufferD3D12 const& nriBuffer = reinterpret_cast<nri::BufferD3D12 const&>(buffer);
    return static_cast<ID3D12Resource*>(nriBuffer);
}

inline auto D3D12ApiWrapper::GetNative(nri::CommandBuffer const& buffer) -> CommandList*
{
    nri::CommandBufferD3D12 const& commandBuffer = reinterpret_cast<nri::CommandBufferD3D12 const&>(buffer);
    ID3D12GraphicsCommandList* nativeCommandBuffer = commandBuffer;
    return nativeCommandBuffer;
}