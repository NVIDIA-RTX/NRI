/*
Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include <memory>

#include <NRIDescs.h>

#ifndef NRI_API_CPP
    #if NRI_STATIC_LIBRARY
        #define NRI_CPP_API
    #else
        #ifdef NRI_GFX_API_WRAPPER_LIBRARY 
            #define NRI_API_CPP __declspec( dllexport )
        #else
            #define NRI_API_CPP __declspec( dllimport )
        #endif
    #endif
#endif

struct NRI_API_CPP GfxApiWrapperInterface
{
    using Device = void;
    using Resource = void;
    using CommandList = void;

    virtual Resource* GetNative(nri::Texture const& texture) = 0;
    virtual Resource* GetNative(nri::Buffer const& buffer) = 0;
    virtual CommandList* GetNative(nri::CommandBuffer const& buffer) = 0;

    virtual ~GfxApiWrapperInterface() {}
};

NRI_API_CPP std::shared_ptr<GfxApiWrapperInterface> createGfxApiWrapper(nri::GraphicsAPI graphicsAPI, nri::Device const& device);