/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include <climits>
#include <NRI.h>

#ifndef UINT_MAX
#define UNDEF_UINT_MAX
#define UINT_MAX 0xFFFFFFFF
#endif

#include <sl.h>

#ifdef UNDEF_UINT_MAX
#undef UINT_MAX
#endif

namespace slwrap
{
    NRI_API bool NRI_CALL setConstants(const ::sl::Constants& values, uint32_t frameIndex, uint32_t id);
    NRI_API bool NRI_CALL shutdown();
    NRI_API bool NRI_CALL init(const sl::Preferences& pref, int applicationId = 0);
    NRI_API bool NRI_CALL setTag(const sl::Resource* resource, sl::BufferType tag, uint32_t id, const sl::Extent* extent);
    NRI_API bool NRI_CALL isFeatureSupported(sl::Feature feature, uint32_t* adapterBitMask = nullptr);
    NRI_API bool NRI_CALL setFeatureConstants(sl::Feature feature, const void* consts, uint32_t frameIndex, uint32_t id);
    NRI_API bool NRI_CALL evaluateFeature(sl::CommandBuffer* cmdBuffer, sl::Feature feature, uint32_t frameIndex, uint32_t id);
    NRI_API bool NRI_CALL getFeatureSettings(sl::Feature feature, const void* consts, void* settings);
}
