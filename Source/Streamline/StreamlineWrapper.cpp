/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include <Extensions/Optional/NRIStreamlineWrapper.h>

#define _STRINGIFY(str) L#str
#define STRINGIFY(str) _STRINGIFY(str)

const wchar_t* pluginPath = STRINGIFY(SL_PLUGINS_DIRECTORY);

bool slwrap::setConstants(const ::sl::Constants& values, uint32_t frameIndex, uint32_t id)
{
    return slSetConstants(values, frameIndex, id);
}

bool slwrap::shutdown()
{
    return slShutdown();
}

bool slwrap::init(const sl::Preferences& pref, int applicationId)
{
    sl::Preferences preferences = pref;
    if (preferences.numPathsToPlugins == 0)
    {
        preferences.pathsToPlugins = &pluginPath;
        preferences.numPathsToPlugins = 1;
    }

    return slInit(preferences, applicationId);
}

bool slwrap::setTag(const sl::Resource* resource, sl::BufferType tag, uint32_t id, const sl::Extent* extent)
{
    return slSetTag(resource, tag, id, extent);
}

bool slwrap::isFeatureSupported(sl::Feature feature, uint32_t* adapterBitMask)
{
    return slIsFeatureSupported(feature, adapterBitMask);
}

bool slwrap::setFeatureConstants(sl::Feature feature, const void* consts, uint32_t frameIndex, uint32_t id)
{
    return slSetFeatureConstants(feature, consts, frameIndex, id);
}

bool slwrap::evaluateFeature(sl::CommandBuffer* cmdBuffer, sl::Feature feature, uint32_t frameIndex, uint32_t id)
{
    return slEvaluateFeature(cmdBuffer, feature, frameIndex, id);
}

bool slwrap::getFeatureSettings(sl::Feature feature, const void* consts, void* settings)
{
    return slGetFeatureSettings(feature, consts, settings);
}
