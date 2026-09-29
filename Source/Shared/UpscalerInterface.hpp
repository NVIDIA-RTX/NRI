// © 2025 NVIDIA Corporation

#include <cmath>

#if NRI_ENABLE_D3D11_SUPPORT
#    include <d3d11.h>
#endif

#if NRI_ENABLE_D3D12_SUPPORT
#    include <d3d12.h>
#endif

#if NRI_ENABLE_VK_SUPPORT
#    include <vulkan/vulkan.h>
#endif

//=====================================================================================================================================
// NIS
//=====================================================================================================================================
#if NRI_ENABLE_NIS_SDK
#    include "NIS.h"
#    include "ShaderMake/ShaderBlob.h"

#    if NRI_ENABLE_D3D11_SUPPORT
#        include "NIS.cs.dxbc.h"
#    endif

#    if (NRI_ENABLE_D3D12_SUPPORT || (NRI_ENABLE_METAL_SUPPORT && NRI_ENABLE_METAL_SHADER_CONVERTER))
#        include "NIS.cs.dxil.h"
#    endif

#    if (NRI_ENABLE_VK_SUPPORT || NRI_ENABLE_WGPU_SUPPORT)
#        include "NIS.cs.spirv.h"
#    endif

// Ring buffer, should cover any reasonable number of queued frames even if "CmdDispatchUpscale" is called several times per frame
const uint32_t NIS_DESCRIPTOR_SET_NUM = 32;

struct BytecodeSize {
    const uint8_t* bytecode;
    uint32_t size;
};

struct Nis {
    DescriptorPool* descriptorPool = nullptr;
    PipelineLayout* pipelineLayout = nullptr;
    Pipeline* pipeline = nullptr;
    Texture* texScale = nullptr;
    Texture* texUsm = nullptr;
    Descriptor* srvScale = nullptr;
    Descriptor* srvUsm = nullptr;
    Descriptor* sampler = nullptr;
    DescriptorSet* descriptorSet0 = nullptr;
    std::array<DescriptorSet*, NIS_DESCRIPTOR_SET_NUM> descriptorSets1 = {};
    Dim2_t blockSize = {};
    uint32_t descriptorSetIndex = 0;
};

static inline const char* GetNisOutputFormatShaderConstant(const DeviceDesc& deviceDesc, Format format) {
    if (deviceDesc.shaderFeatures.storageWriteWithoutFormat)
        return "0";

    switch (format) {
        case Format::RGBA8_UNORM:
            return "1";
        case Format::RGBA16_SFLOAT:
            return "2";
        case Format::RGBA32_SFLOAT:
            return "3";
        default:
            return nullptr;
    }
}

#endif

//=====================================================================================================================================
// FFX
//=====================================================================================================================================
#if NRI_ENABLE_FFX_SDK
#    include "ffx_upscale.h"

// Satisfy the SDK's malloc-equivalent alignment without depending on Windows headers
constexpr size_t FFX_ALLOCATION_ALIGNMENT = 16;

#    if NRI_ENABLE_D3D12_SUPPORT
#        include "dx12/ffx_api_dx12.h"
#    endif

#    if NRI_ENABLE_VK_SUPPORT

// Legacy SDK 1.1.4 Vulkan ABI, retained alongside the newer D3D12 headers
#        define FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK 0x0000003u

struct ffxCreateBackendVKDesc {
    ffxCreateContextDescHeader header;
    VkDevice vkDevice;
    VkPhysicalDevice vkPhysicalDevice;
    PFN_vkGetDeviceProcAddr vkDeviceProcAddr;
};

struct FfxVkPair {
    VkDevice device = VK_NULL_HANDLE;
    PFN_vkGetDeviceProcAddr getDeviceProcAddress = nullptr;
    FfxVkPair* next = nullptr;
};

#    endif

struct FfxGlobals {
    // Each SDK DLL has mutable global setup and diagnostic state
#    if NRI_ENABLE_D3D12_SUPPORT
    Lock d3d12Lock = {};
#    endif

#    if NRI_ENABLE_VK_SUPPORT
    Lock vkLock = {};
    Lock vkRegistryLock = {};
    FfxVkPair* vkPairs = nullptr;
#    endif
} g_ffx;

struct Ffx {
    PfnFfxCreateContext CreateContext = nullptr;
    PfnFfxDestroyContext DestroyContext = nullptr;
    PfnFfxDispatch Dispatch = nullptr;
    PfnFfxQuery Query = nullptr;
    Library* library = nullptr;
    ffxContext context = nullptr;
    ffxAllocationCallbacks allocationCallbacks = {};
    ffxAllocationCallbacks* allocationCallbacksPtr = nullptr;
    ffxCreateContextDescUpscale contextDesc = {};
    Lock* sdkLock = nullptr;

#    if NRI_ENABLE_D3D12_SUPPORT
    ffxCreateBackendDX12Desc backendD3D12Desc = {};
    ffxCreateContextDescUpscaleVersion versionDesc = {};
#    endif

#    if NRI_ENABLE_VK_SUPPORT
    ffxCreateBackendVKDesc backendVKDesc = {};
    FfxVkPair vkPair = {};
#    endif
};

static inline Lock& FfxGetSdkLock(GraphicsAPI graphicsAPI) {
#    if (NRI_ENABLE_D3D12_SUPPORT && NRI_ENABLE_VK_SUPPORT)
    return graphicsAPI == GraphicsAPI::D3D12 ? g_ffx.d3d12Lock : g_ffx.vkLock;
#    elif NRI_ENABLE_D3D12_SUPPORT
    MaybeUnused(graphicsAPI);

    return g_ffx.d3d12Lock;
#    else
    MaybeUnused(graphicsAPI);

    return g_ffx.vkLock;
#    endif
}

#    if NRI_ENABLE_VK_SUPPORT
static inline void FfxRegisterDevice(FfxVkPair& pair, VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddress) {
    ExclusiveScope lock(g_ffx.vkRegistryLock);

    pair = {device, getDeviceProcAddress, g_ffx.vkPairs};
    g_ffx.vkPairs = &pair;
}

static inline void FfxUnregisterDevice(FfxVkPair& pair) {
    ExclusiveScope lock(g_ffx.vkRegistryLock);

    for (FfxVkPair** link = &g_ffx.vkPairs; *link; link = &(*link)->next) {
        if (*link == &pair) {
            *link = pair.next;

            return;
        }
    }

    NRI_CHECK(false, "Unregistered FFX device");
}

static inline PFN_vkVoidFunction VKAPI_PTR FfxVkGetDeviceProcAddr(VkDevice device, const char* pName) {
    // The legacy FFX Vulkan backend requests extension names for some core functions
    if (!strcmp(pName, "vkGetBufferMemoryRequirements2KHR"))
        pName = "vkGetBufferMemoryRequirements2";

    PFN_vkGetDeviceProcAddr getDeviceProcAddress = nullptr;
    {
        ExclusiveScope lock(g_ffx.vkRegistryLock);

        for (const FfxVkPair* pair = g_ffx.vkPairs; pair; pair = pair->next) {
            if (pair->device == device) {
                getDeviceProcAddress = pair->getDeviceProcAddress;
                break;
            }
        }
    }

    NRI_CHECK(getDeviceProcAddress, "Unexpected");

    // Use corresponding "vkGetDeviceProcAddr"
    PFN_vkVoidFunction func = getDeviceProcAddress(device, pName);
    NRI_CHECK(func || strstr(pName, "AMD"), "Another non-CORE function name?");

    return func;
}

#    endif

static inline Result FfxConvertError(ffxReturnCode_t code) {
    switch (code) {
        case FFX_API_RETURN_OK:
            return Result::SUCCESS;
        case FFX_API_RETURN_ERROR_UNKNOWN_DESCTYPE:
        case FFX_API_RETURN_ERROR_PARAMETER:
            return Result::INVALID_ARGUMENT;
        case FFX_API_RETURN_ERROR_MEMORY:
            return Result::OUT_OF_MEMORY;
        case FFX_API_RETURN_NO_PROVIDER:
            return Result::UNSUPPORTED;
    }

    return Result::FAILURE;
}

static inline FfxApiSurfaceFormat FfxConvertFormat(Format format) {
    switch (format) {
        case Format::RGBA32_UINT:
            return FFX_API_SURFACE_FORMAT_R32G32B32A32_UINT;
        case Format::RGBA32_SFLOAT:
            return FFX_API_SURFACE_FORMAT_R32G32B32A32_FLOAT;
        case Format::RGBA16_SFLOAT:
            return FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT;
        case Format::RGB32_SFLOAT:
            return FFX_API_SURFACE_FORMAT_R32G32B32_FLOAT;
        case Format::RG32_SFLOAT:
            return FFX_API_SURFACE_FORMAT_R32G32_FLOAT;
        case Format::R8_UINT:
            return FFX_API_SURFACE_FORMAT_R8_UINT;
        case Format::R32_UINT:
            return FFX_API_SURFACE_FORMAT_R32_UINT;
        case Format::RGBA8_UNORM:
            return FFX_API_SURFACE_FORMAT_R8G8B8A8_UNORM;
        case Format::RGBA8_SNORM:
            return FFX_API_SURFACE_FORMAT_R8G8B8A8_SNORM;
        case Format::RGBA8_SRGB:
            return FFX_API_SURFACE_FORMAT_R8G8B8A8_SRGB;
        case Format::BGRA8_UNORM:
            return FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM;
        case Format::BGRA8_SRGB:
            return FFX_API_SURFACE_FORMAT_B8G8R8A8_SRGB;
        case Format::R11_G11_B10_UFLOAT:
            return FFX_API_SURFACE_FORMAT_R11G11B10_FLOAT;
        case Format::R10_G10_B10_A2_UNORM:
            return FFX_API_SURFACE_FORMAT_R10G10B10A2_UNORM;
        case Format::RG16_SFLOAT:
            return FFX_API_SURFACE_FORMAT_R16G16_FLOAT;
        case Format::RG16_UINT:
            return FFX_API_SURFACE_FORMAT_R16G16_UINT;
        case Format::RG16_SINT:
            return FFX_API_SURFACE_FORMAT_R16G16_SINT;
        case Format::R16_SFLOAT:
            return FFX_API_SURFACE_FORMAT_R16_FLOAT;
        case Format::R16_UINT:
            return FFX_API_SURFACE_FORMAT_R16_UINT;
        case Format::R16_UNORM:
            return FFX_API_SURFACE_FORMAT_R16_UNORM;
        case Format::R16_SNORM:
            return FFX_API_SURFACE_FORMAT_R16_SNORM;
        case Format::R8_UNORM:
            return FFX_API_SURFACE_FORMAT_R8_UNORM;
        case Format::RG8_UNORM:
            return FFX_API_SURFACE_FORMAT_R8G8_UNORM;
        case Format::RG8_UINT:
            return FFX_API_SURFACE_FORMAT_R8G8_UINT;
        case Format::R32_SFLOAT:
            return FFX_API_SURFACE_FORMAT_R32_FLOAT;
        case Format::R9_G9_B9_E5_UFLOAT:
            return FFX_API_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP;
        case Format::UNKNOWN:
        default:
            return FFX_API_SURFACE_FORMAT_UNKNOWN;
    }
}

static inline FfxApiResource FfxGetResource(const CoreInterface& NRI, const UpscalerResource& resource, bool isStorage = false) {
    FfxApiResource res{};
    res.resource = (void*)NRI.GetTextureNativeObject(resource.texture);
    res.state = isStorage ? FFX_API_RESOURCE_STATE_UNORDERED_ACCESS : FFX_API_RESOURCE_STATE_COMPUTE_READ;
    res.description.flags = FFX_API_RESOURCE_FLAGS_NONE;

    if (res.resource) {
        const TextureDesc& textureDesc = NRI.GetTextureDesc(*resource.texture);
        const FormatProps& formatProps = GetFormatProps(textureDesc.format);

        res.description.format = FfxConvertFormat(textureDesc.format);

        if (textureDesc.usage & TextureUsageBits::SHADER_RESOURCE_STORAGE)
            res.description.usage |= FFX_API_RESOURCE_USAGE_UAV;

        if (textureDesc.usage & TextureUsageBits::COLOR_ATTACHMENT)
            res.description.usage |= FFX_API_RESOURCE_USAGE_RENDERTARGET;

        if (textureDesc.usage & TextureUsageBits::DEPTH_STENCIL_ATTACHMENT)
            res.description.usage |= FFX_API_RESOURCE_USAGE_DEPTHTARGET | (formatProps.isStencil ? FFX_API_RESOURCE_USAGE_STENCILTARGET : 0);

        res.description.width = textureDesc.width;
        res.description.height = textureDesc.height;
        res.description.depth = textureDesc.type == TextureType::TEXTURE_3D ? textureDesc.depth : textureDesc.layerNum;
        res.description.mipCount = textureDesc.mipNum;

        switch (textureDesc.type) {
            case TextureType::TEXTURE_1D:
                res.description.type = FFX_API_RESOURCE_TYPE_TEXTURE1D;
                break;
            case TextureType::TEXTURE_2D:
                res.description.type = FFX_API_RESOURCE_TYPE_TEXTURE2D;
                break;
            case TextureType::TEXTURE_3D:
                res.description.type = FFX_API_RESOURCE_TYPE_TEXTURE3D;
                break;
            default:
                NRI_CHECK(false, "Unexpected 'textureDesc.type'");
                break;
        }
    }

    return res;
}

static inline void* FfxAlloc(void* pUserData, uint64_t size) {
    const auto& allocationCallbacks = *(AllocationCallbacks*)pUserData;

    return allocationCallbacks.Allocate(allocationCallbacks.userArg, size, FFX_ALLOCATION_ALIGNMENT);
}

static inline void FfxDealloc(void* pUserData, void* pMem) {
    const auto& allocationCallbacks = *(AllocationCallbacks*)pUserData;

    if (pMem)
        allocationCallbacks.Free(allocationCallbacks.userArg, pMem);
}

#    ifndef NDEBUG

static void FfxDebugMessage(uint32_t messageType, const wchar_t* message) {
    NRI_FFX_DEBUG_LOG(messageType, message);
}

#    endif

#endif

//=====================================================================================================================================
// XESS
//=====================================================================================================================================
#if NRI_ENABLE_XESS_SDK
#    include "xess.h"
#    include "xess_debug.h"

#    if NRI_ENABLE_D3D12_SUPPORT
#        include "xess_d3d12.h"
#    endif

struct Xess {
    xess_context_handle_t context = nullptr;
};

struct XessGlobals {
    Lock lock = {}; // methods in XESS library are NOT thread safe (see "Thread safety")
} g_xess;

static inline Result XessConvertError(xess_result_t code) {
    switch (code) {
        case XESS_RESULT_WARNING_NONEXISTING_FOLDER:
        case XESS_RESULT_WARNING_OLD_DRIVER:
        case XESS_RESULT_ERROR_INVALID_ARGUMENT:
            return Result::INVALID_ARGUMENT;

        case XESS_RESULT_SUCCESS:
            return Result::SUCCESS;

        case XESS_RESULT_ERROR_UNSUPPORTED:
        case XESS_RESULT_ERROR_UNSUPPORTED_DEVICE:
        case XESS_RESULT_ERROR_UNSUPPORTED_DRIVER:
            return Result::UNSUPPORTED;

        default:
            return Result::FAILURE;
    }
}

#endif

//=====================================================================================================================================
// NGX
//=====================================================================================================================================
#if NRI_ENABLE_NGX_SDK
#    if defined(__GNUC__)
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wsign-compare"
#        pragma GCC diagnostic ignored "-Wmissing-braces"
#    elif defined(__clang__)
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wsign-compare"
#        pragma clang diagnostic ignored "-Wmissing-braces"
#    endif

#    include "nvsdk_ngx_helpers.h"
#    if NRI_ENABLE_VK_SUPPORT
#        include "nvsdk_ngx_helpers_vk.h"

#        include "nvsdk_ngx_helpers_dlssd_vk.h"
#    endif
#    if NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT
#        include "nvsdk_ngx_helpers_dlssd_d3d.h"
#    endif

#    if defined(__GNUC__)
#        pragma GCC diagnostic pop
#    elif defined(__clang__)
#        pragma clang diagnostic pop
#    endif

struct Ngx {
    NVSDK_NGX_Handle* handle = nullptr;
    NVSDK_NGX_Parameter* params = nullptr;
};

struct RefCounter {
    void* deviceNative;
    int32_t refCounter;
};

const uint32_t APPLICATION_ID = 0x3143DEC; // don't care, but can't be 0

struct NgxGlobals {
    std::array<RefCounter, 32> refCounters; // awful API borns awful solutions...
    uint32_t refCounterNum = 0;
    Lock lock = {}; // methods in NGX library are NOT thread safe (see the comment in "nvsdk_ngx.h")
} g_ngx;

static inline int32_t NgxIncrRef(void* deviceNative) {
    uint32_t i = 0;
    for (; i < g_ngx.refCounterNum && g_ngx.refCounters[i].deviceNative != deviceNative; i++)
        ;

    if (i == g_ngx.refCounterNum) {
        g_ngx.refCounterNum++;
        NRI_CHECK(g_ngx.refCounterNum < g_ngx.refCounters.size(), "Too many devices?");
    }

    g_ngx.refCounters[i].deviceNative = deviceNative;
    g_ngx.refCounters[i].refCounter++;

    return g_ngx.refCounters[i].refCounter;
}

static inline int32_t NgxDecrRef(void* deviceNative) {
    uint32_t i = 0;
    for (; i < g_ngx.refCounterNum && g_ngx.refCounters[i].deviceNative != deviceNative; i++)
        ;

    NRI_CHECK(i < g_ngx.refCounterNum, "Destroy before create?");
    NRI_CHECK(g_ngx.refCounters[i].refCounter > 0, "Unexpected");

    g_ngx.refCounters[i].deviceNative = deviceNative;
    g_ngx.refCounters[i].refCounter--;

    return g_ngx.refCounters[i].refCounter;
}

static inline void NgxSetEvalParams(NVSDK_NGX_Parameter* params, const DispatchUpscaleDesc& dispatchUpscaleDesc, float preExposure) {
    NVSDK_NGX_Parameter_SetF(params, NVSDK_NGX_Parameter_Jitter_Offset_X, dispatchUpscaleDesc.cameraJitter.x);
    NVSDK_NGX_Parameter_SetF(params, NVSDK_NGX_Parameter_Jitter_Offset_Y, dispatchUpscaleDesc.cameraJitter.y);
    NVSDK_NGX_Parameter_SetI(params, NVSDK_NGX_Parameter_Reset, (dispatchUpscaleDesc.flags & DispatchUpscaleBits::RESET_HISTORY) ? 1 : 0);
    NVSDK_NGX_Parameter_SetF(params, NVSDK_NGX_Parameter_MV_Scale_X, dispatchUpscaleDesc.mvScale.x == 0.0f ? 1.0f : dispatchUpscaleDesc.mvScale.x);
    NVSDK_NGX_Parameter_SetF(params, NVSDK_NGX_Parameter_MV_Scale_Y, dispatchUpscaleDesc.mvScale.y == 0.0f ? 1.0f : dispatchUpscaleDesc.mvScale.y);
    NVSDK_NGX_Parameter_SetUI(params, NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Width, dispatchUpscaleDesc.currentResolution.w);
    NVSDK_NGX_Parameter_SetUI(params, NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Height, dispatchUpscaleDesc.currentResolution.h);
    NVSDK_NGX_Parameter_SetF(params, NVSDK_NGX_Parameter_DLSS_Pre_Exposure, preExposure);
    NVSDK_NGX_Parameter_SetF(params, NVSDK_NGX_Parameter_DLSS_Exposure_Scale, 1.0f);
}

static void NVSDK_CONV NgxLogCallback(const char*, NVSDK_NGX_Logging_Level, NVSDK_NGX_Feature) {
}

#    if NRI_ENABLE_VK_SUPPORT

static inline NVSDK_NGX_Resource_VK NgxGetResource(const CoreInterface& NRI, const UpscalerResource& resource, uint64_t resourceNative, bool isStorage = false) {
    if (!resource.texture)
        return {};

    const TextureDesc& textureDesc = NRI.GetTextureDesc(*resource.texture);
    VkImageView view = (VkImageView)NRI.GetDescriptorNativeObject(resource.descriptor);
    VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkFormat format = (VkFormat)nriConvertNRIFormatToVK(textureDesc.format);

    return NVSDK_NGX_Create_ImageView_Resource_VK(view, (VkImage)resourceNative, subresourceRange, format, textureDesc.width, textureDesc.height, isStorage);
}

#    endif
#endif

//=====================================================================================================================================
// MetalFX
//=====================================================================================================================================
#if NRI_ENABLE_METAL_SUPPORT

// MetalFX encodes its own passes. "Dispatch" is ordered by app barriers with "COMPUTE_SHADER" stage, other stages are bridged
constexpr MTL::Stages METALFX_EXTRA_STAGES = MTL::StageVertex | MTL::StageFragment | MTL::StageMachineLearning;

struct MetalFx {
    MTL4::Compiler* compiler = nullptr;
    MTL4FX::SpatialScaler* spatial = nullptr;
    MTL4FX::TemporalScaler* temporal = nullptr;
    MTL4FX::TemporalDenoisedScaler* denoised = nullptr;
};

static inline bool IsMetalFx(UpscalerType type) {
    return type == UpscalerType::METALFX_SPATIAL || type == UpscalerType::METALFX_TEMPORAL || type == UpscalerType::METALFX_DENOISED;
}

static inline MTL::Texture* MetalFxGetTexture(const CoreInterface& NRI, const UpscalerResource& resource) {
    return (MTL::Texture*)NRI.GetDescriptorNativeObject(resource.descriptor);
}

static inline MTL::PixelFormat MetalFxGetFormat(const CoreInterface& NRI, const UpscalerResource& resource) {
    MTL::Texture* texture = MetalFxGetTexture(NRI, resource);

    return texture ? texture->pixelFormat() : MTL::PixelFormatInvalid;
}

// Metal 4 doesn't retain resources used by command buffers, so the "roughness" view (swizzled "normalRoughness") lives until the command allocator reset
static MTL::Texture* MetalFxCreateRoughnessView(MTL4::CommandBuffer* commandBuffer, MTL::Texture* normalRoughness) {
    MTL::TextureSwizzleChannels swizzle = MTL::TextureSwizzleChannels::Make(MTL::TextureSwizzleAlpha, MTL::TextureSwizzleAlpha, MTL::TextureSwizzleAlpha, MTL::TextureSwizzleAlpha);
    NS::Range levels = NS::Range::Make(0, normalRoughness->mipmapLevelCount());
    NS::Range slices = NS::Range::Make(0, normalRoughness->arrayLength());

    MTL::Texture* view = normalRoughness->newTextureView(normalRoughness->pixelFormat(), normalRoughness->textureType(), levels, slices, swizzle);

    if (view)
        ReleaseOnResetMetal(commandBuffer, view);

    return view;
}

static bool MetalFxCreateScaler(MetalFx& metalFx, MTL::Device* device, const CoreInterface& NRI, const UpscalerDesc& upscalerDesc, const DispatchUpscaleDesc& dispatchUpscaleDesc) {
    MTL::Texture* input = MetalFxGetTexture(NRI, dispatchUpscaleDesc.input);
    MTL::Texture* output = MetalFxGetTexture(NRI, dispatchUpscaleDesc.output);

    if (upscalerDesc.type == UpscalerType::METALFX_SPATIAL) {
        MTLFX::SpatialScalerColorProcessingMode colorProcessingMode = MTLFX::SpatialScalerColorProcessingModeLinear;

        if (upscalerDesc.flags & UpscalerBits::HDR)
            colorProcessingMode = MTLFX::SpatialScalerColorProcessingModeHDR;
        else if (upscalerDesc.flags & UpscalerBits::SRGB)
            colorProcessingMode = MTLFX::SpatialScalerColorProcessingModePerceptual;

        MTLFX::SpatialScalerDescriptor* scalerDesc = MTLFX::SpatialScalerDescriptor::alloc()->init();
        scalerDesc->setInputWidth(input->width());
        scalerDesc->setInputHeight(input->height());
        scalerDesc->setOutputWidth(upscalerDesc.upscaleResolution.w);
        scalerDesc->setOutputHeight(upscalerDesc.upscaleResolution.h);
        scalerDesc->setColorTextureFormat(input->pixelFormat());
        scalerDesc->setOutputTextureFormat(output->pixelFormat());
        scalerDesc->setColorProcessingMode(colorProcessingMode);

        metalFx.spatial = scalerDesc->newSpatialScaler(device, metalFx.compiler);
        scalerDesc->release();

        return metalFx.spatial != nullptr;
    }

    if (upscalerDesc.type == UpscalerType::METALFX_TEMPORAL) {
        const UpscalerGuides& guides = dispatchUpscaleDesc.guides.upscaler;

        MTLFX::TemporalScalerDescriptor* scalerDesc = MTLFX::TemporalScalerDescriptor::alloc()->init();
        scalerDesc->setInputWidth(input->width());
        scalerDesc->setInputHeight(input->height());
        scalerDesc->setOutputWidth(upscalerDesc.upscaleResolution.w);
        scalerDesc->setOutputHeight(upscalerDesc.upscaleResolution.h);
        scalerDesc->setColorTextureFormat(input->pixelFormat());
        scalerDesc->setOutputTextureFormat(output->pixelFormat());
        scalerDesc->setDepthTextureFormat(MetalFxGetFormat(NRI, guides.depth));
        scalerDesc->setMotionTextureFormat(MetalFxGetFormat(NRI, guides.mv));
        scalerDesc->setAutoExposureEnabled(!(upscalerDesc.flags & UpscalerBits::USE_EXPOSURE));
        scalerDesc->setInputContentPropertiesEnabled(true);
        scalerDesc->setInputContentMinScale(MTLFX::TemporalScalerDescriptor::supportedInputContentMinScale(device));
        scalerDesc->setInputContentMaxScale(MTLFX::TemporalScalerDescriptor::supportedInputContentMaxScale(device));

        if (upscalerDesc.flags & UpscalerBits::MV_UPSCALED)
            scalerDesc->setOutputResolutionMotionVectorsEnabled(true); // macOS 27+ (checked in "Create")

        if (upscalerDesc.flags & UpscalerBits::USE_REACTIVE) {
            scalerDesc->setReactiveMaskTextureEnabled(true);
            scalerDesc->setReactiveMaskTextureFormat(MetalFxGetFormat(NRI, guides.reactive));
        }

        metalFx.temporal = scalerDesc->newTemporalScaler(device, metalFx.compiler);
        scalerDesc->release();

        return metalFx.temporal != nullptr;
    }

    const DenoiserGuides& guides = dispatchUpscaleDesc.guides.denoiser;
    MTL::PixelFormat normalRoughnessFormat = MetalFxGetFormat(NRI, guides.normalRoughness);

    MTLFX::TemporalDenoisedScalerDescriptor* scalerDesc = MTLFX::TemporalDenoisedScalerDescriptor::alloc()->init();
    scalerDesc->setInputWidth(input->width());
    scalerDesc->setInputHeight(input->height());
    scalerDesc->setOutputWidth(upscalerDesc.upscaleResolution.w);
    scalerDesc->setOutputHeight(upscalerDesc.upscaleResolution.h);
    scalerDesc->setColorTextureFormat(input->pixelFormat());
    scalerDesc->setOutputTextureFormat(output->pixelFormat());
    scalerDesc->setDepthTextureFormat(MetalFxGetFormat(NRI, guides.depth));
    scalerDesc->setMotionTextureFormat(MetalFxGetFormat(NRI, guides.mv));
    scalerDesc->setNormalTextureFormat(normalRoughnessFormat);
    scalerDesc->setRoughnessTextureFormat(normalRoughnessFormat); // a swizzled view of "normalRoughness"
    scalerDesc->setDiffuseAlbedoTextureFormat(MetalFxGetFormat(NRI, guides.diffuseAlbedo));
    scalerDesc->setSpecularAlbedoTextureFormat(MetalFxGetFormat(NRI, guides.specularAlbedo));
    scalerDesc->setSpecularHitDistanceTextureFormat(MetalFxGetFormat(NRI, guides.specularMvOrHitT));
    scalerDesc->setSpecularHitDistanceTextureEnabled(true);
    scalerDesc->setAutoExposureEnabled(!(upscalerDesc.flags & UpscalerBits::USE_EXPOSURE));

    if (upscalerDesc.flags & UpscalerBits::USE_REACTIVE) {
        scalerDesc->setReactiveMaskTextureEnabled(true);
        scalerDesc->setReactiveMaskTextureFormat(MetalFxGetFormat(NRI, guides.reactive));
    }

    metalFx.denoised = scalerDesc->newTemporalDenoisedScaler(device, metalFx.compiler);
    scalerDesc->release();

    return metalFx.denoised != nullptr;
}

#endif

//=====================================================================================================================================
// Upscaler
//=====================================================================================================================================
bool nri::IsUpscalerSupported(const DeviceDesc& deviceDesc, UpscalerType type) {
    MaybeUnused(deviceDesc, type);

#if NRI_ENABLE_NIS_SDK
    if (type == UpscalerType::NIS) {
#    if (NRI_ENABLE_METAL_SUPPORT && !NRI_ENABLE_METAL_SHADER_CONVERTER)
        if (deviceDesc.graphicsAPI == GraphicsAPI::METAL)
            return false;
#    endif
        return true;
    }
#endif

#if NRI_ENABLE_FFX_SDK
    if (type == UpscalerType::FSR) {
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12 || deviceDesc.graphicsAPI == GraphicsAPI::VK)
            return true;
    }
#endif

#if NRI_ENABLE_XESS_SDK
    if (type == UpscalerType::XESS) {
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12)
            return true;
    }
#endif

#if NRI_ENABLE_NGX_SDK
    if (type == UpscalerType::DLSR || type == UpscalerType::DLRR) {
        if (deviceDesc.adapterDesc.vendor == Vendor::NVIDIA && deviceDesc.tiers.rayTracing) // an elegant way to detect an RTX GPU?
            return true;
    }
#endif

#if NRI_ENABLE_METAL_SUPPORT
    if (IsMetalFx(type)) {
        if (deviceDesc.graphicsAPI == GraphicsAPI::METAL) // exact "supportsMetal4FX" check happens in "Create"
            return true;
    }
#endif

    return false;
}

UpscalerImpl::~UpscalerImpl() {
#if NRI_ENABLE_NIS_SDK
    if (m_Desc.type == UpscalerType::NIS && m.nis) {
        m_iCore.DestroyDescriptor(m.nis->srvScale);
        m_iCore.DestroyDescriptor(m.nis->srvUsm);
        m_iCore.DestroyDescriptor(m.nis->sampler);
        m_iCore.DestroyTexture(m.nis->texScale);
        m_iCore.DestroyTexture(m.nis->texUsm);
        m_iCore.DestroyPipeline(m.nis->pipeline);
        m_iCore.DestroyPipelineLayout(m.nis->pipelineLayout);
        m_iCore.DestroyDescriptorPool(m.nis->descriptorPool);

        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        Destroy<Nis>(allocationCallbacks, m.nis);
    }
#endif

#if NRI_ENABLE_XESS_SDK
    if (m_Desc.type == UpscalerType::XESS && m.xess) {
        ExclusiveScope lock(g_xess.lock);

        xess_result_t result = xessDestroyContext(m.xess->context);
        MaybeUnused(result);
        NRI_CHECK(result == XESS_RESULT_SUCCESS, "xessDestroyContext() failed!");

        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        Destroy<Xess>(allocationCallbacks, m.xess);
    }
#endif

#if NRI_ENABLE_FFX_SDK
    if (m_Desc.type == UpscalerType::FSR && m.ffx) {
        ExclusiveScope lock(*m.ffx->sdkLock);

        if (m.ffx->context) {
            ffxReturnCode_t result = m.ffx->DestroyContext(&m.ffx->context, m.ffx->allocationCallbacksPtr);
            MaybeUnused(result);
            NRI_CHECK(result == FFX_API_RETURN_OK, "ffxDestroyContext() failed!");
        }

#    if NRI_ENABLE_VK_SUPPORT
        if (m.ffx->vkPair.device)
            FfxUnregisterDevice(m.ffx->vkPair);
#    endif

        if (m.ffx->library)
            UnloadSharedLibrary(*m.ffx->library);

        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        Destroy<Ffx>(allocationCallbacks, m.ffx);
    }
#endif

#if NRI_ENABLE_NGX_SDK
    if ((m_Desc.type == UpscalerType::DLSR || m_Desc.type == UpscalerType::DLRR) && m.ngx) {
        ExclusiveScope lock(g_ngx.lock);

        const DeviceDesc& deviceDesc = m_iCore.GetDeviceDesc(m_Device);
        void* deviceNative = m_iCore.GetDeviceNativeObject(&m_Device);
        int32_t refCount = NgxDecrRef(deviceNative);

#    if NRI_ENABLE_D3D11_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D11) {
            NVSDK_NGX_Result result = NVSDK_NGX_D3D11_DestroyParameters(m.ngx->params);
            NRI_CHECK(!m.ngx->params || result == NVSDK_NGX_Result_Success, "NVSDK_NGX_D3D11_DestroyParameters() failed!");

            result = NVSDK_NGX_D3D11_ReleaseFeature(m.ngx->handle);
            NRI_CHECK(!m.ngx->handle || result == NVSDK_NGX_Result_Success, "NVSDK_NGX_D3D11_ReleaseFeature() failed!");

            if (refCount == 0) {
                result = NVSDK_NGX_D3D11_Shutdown1((ID3D11Device*)deviceNative);
                NRI_CHECK(result == NVSDK_NGX_Result_Success, "NVSDK_NGX_D3D11_Shutdown1() failed!");
            }
        }
#    endif

#    if NRI_ENABLE_D3D12_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12) {
            NVSDK_NGX_Result result = NVSDK_NGX_D3D12_DestroyParameters(m.ngx->params);
            NRI_CHECK(!m.ngx->params || result == NVSDK_NGX_Result_Success, "NVSDK_NGX_D3D12_DestroyParameters() failed!");

            result = NVSDK_NGX_D3D12_ReleaseFeature(m.ngx->handle);
            NRI_CHECK(!m.ngx->handle || result == NVSDK_NGX_Result_Success, "NVSDK_NGX_D3D12_ReleaseFeature() failed!");

            if (refCount == 0) {
                result = NVSDK_NGX_D3D12_Shutdown1((ID3D12Device*)deviceNative);
                NRI_CHECK(result == NVSDK_NGX_Result_Success, "NVSDK_NGX_D3D12_Shutdown1() failed!");
            }
        }
#    endif

#    if NRI_ENABLE_VK_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::VK) {
            NVSDK_NGX_Result result = NVSDK_NGX_VULKAN_DestroyParameters(m.ngx->params);
            NRI_CHECK(!m.ngx->params || result == NVSDK_NGX_Result_Success, "NVSDK_NGX_VULKAN_DestroyParameters() failed!");

            result = NVSDK_NGX_VULKAN_ReleaseFeature(m.ngx->handle);
            NRI_CHECK(!m.ngx->handle || result == NVSDK_NGX_Result_Success, "NVSDK_NGX_VULKAN_ReleaseFeature() failed!");

            if (refCount == 0) {
                result = NVSDK_NGX_VULKAN_Shutdown1((VkDevice)deviceNative);
                NRI_CHECK(result == NVSDK_NGX_Result_Success, "NVSDK_NGX_VULKAN_Shutdown1() failed!");
            }
        }
#    endif

        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        Destroy<Ngx>(allocationCallbacks, m.ngx);
    }
#endif

#if NRI_ENABLE_METAL_SUPPORT
    if (IsMetalFx(m_Desc.type) && m.metalfx) {
        if (m.metalfx->spatial)
            m.metalfx->spatial->release();

        if (m.metalfx->temporal)
            m.metalfx->temporal->release();

        if (m.metalfx->denoised)
            m.metalfx->denoised->release();

        if (m.metalfx->compiler)
            m.metalfx->compiler->release();

        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        Destroy<MetalFx>(allocationCallbacks, m.metalfx);
    }
#endif
}

Result UpscalerImpl::Create(const UpscalerDesc& upscalerDesc) {
    m_Desc = upscalerDesc;

    const DeviceDesc& deviceDesc = m_iCore.GetDeviceDesc(m_Device);
    if (!IsUpscalerSupported(deviceDesc, upscalerDesc.type))
        return Result::UNSUPPORTED;

    UpscalerProps upscalerProps = {};
    GetUpscalerProps(upscalerProps);

#if NRI_ENABLE_NIS_SDK
    if (upscalerDesc.type == UpscalerType::NIS) {
        const char* outputFormat = GetNisOutputFormatShaderConstant(deviceDesc, upscalerDesc.outputFormat);
        if (!outputFormat)
            return Result::UNSUPPORTED;

        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        m.nis = Allocate<Nis>(allocationCallbacks);

        { // Pipeline layout
            RootConstantDesc rootConstants = {};
            rootConstants.registerIndex = 0;
            rootConstants.shaderStages = StageBits::COMPUTE_SHADER;
            rootConstants.size = sizeof(NIS::Constants);

            const DescriptorRangeDesc descriptorSet0Ranges[] = {
                {1, 1, DescriptorType::SAMPLER, StageBits::COMPUTE_SHADER},
                {2, 2, DescriptorType::TEXTURE, StageBits::COMPUTE_SHADER},
            };

            const DescriptorRangeDesc descriptorSet1Ranges[] = {
                {0, 1, DescriptorType::TEXTURE, StageBits::COMPUTE_SHADER, DescriptorRangeBits::ALLOW_UPDATE_AFTER_SET},
                {1, 1, DescriptorType::STORAGE_TEXTURE, StageBits::COMPUTE_SHADER, DescriptorRangeBits::ALLOW_UPDATE_AFTER_SET},
            };

            DescriptorSetDesc descriptorSetDescs[2] = {};
            {
                descriptorSetDescs[0].registerSpace = NRI_NIS_SET_STATIC;
                descriptorSetDescs[0].ranges = descriptorSet0Ranges;
                descriptorSetDescs[0].rangeNum = GetCountOf(descriptorSet0Ranges);

                descriptorSetDescs[1].registerSpace = NRI_NIS_SET_DYNAMIC;
                descriptorSetDescs[1].ranges = descriptorSet1Ranges;
                descriptorSetDescs[1].rangeNum = GetCountOf(descriptorSet1Ranges);
                descriptorSetDescs[1].flags = DescriptorSetBits::ALLOW_UPDATE_AFTER_SET;
            }

            PipelineLayoutDesc pipelineLayoutDesc = {};
            pipelineLayoutDesc.rootConstants = &rootConstants;
            pipelineLayoutDesc.rootConstantNum = 1;
            pipelineLayoutDesc.descriptorSets = descriptorSetDescs;
            pipelineLayoutDesc.descriptorSetNum = GetCountOf(descriptorSetDescs);
            pipelineLayoutDesc.shaderStages = StageBits::COMPUTE_SHADER;
            pipelineLayoutDesc.flags = PipelineLayoutBits::IGNORE_GLOBAL_SPIRV_OFFSETS;

            Result result = m_iCore.CreatePipelineLayout(m_Device, pipelineLayoutDesc, m.nis->pipelineLayout);
            if (result != Result::SUCCESS)
                return result;
        }

        { // Pipeline
            m.nis->blockSize.w = 32;
            m.nis->blockSize.h = deviceDesc.shaderModel >= NriShaderModel(6, 2) ? 32 : 24;

            std::array<ShaderMake::ShaderConstant, 4> defines = {{
                {"NIS_FP16", (deviceDesc.shaderModel >= NriShaderModel(6, 2)) ? "1" : "0"},
                {"NIS_HDR_MODE", (upscalerDesc.flags & UpscalerBits::HDR) ? "1" : "0"},
                {"NIS_OUTPUT_FORMAT", outputFormat},
                {"NIS_THREAD_GROUP_SIZE", (deviceDesc.adapterDesc.vendor == Vendor::NVIDIA) ? "128" : "256"}, // TODO: verify performance
            }};

            const void* bytecode = nullptr;
            size_t size = 0;
            bool shaderMakeResult = false;
#    if NRI_ENABLE_D3D11_SUPPORT
            if (deviceDesc.graphicsAPI == GraphicsAPI::D3D11)
                shaderMakeResult = ShaderMake::FindPermutationInBlob(g_NIS_cs_dxbc, GetCountOf(g_NIS_cs_dxbc), defines.data(), (uint32_t)defines.size(), &bytecode, &size);
#    endif
#    if (NRI_ENABLE_D3D12_SUPPORT || (NRI_ENABLE_METAL_SUPPORT && NRI_ENABLE_METAL_SHADER_CONVERTER))
            if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12 || deviceDesc.graphicsAPI == GraphicsAPI::METAL)
                shaderMakeResult = ShaderMake::FindPermutationInBlob(g_NIS_cs_dxil, GetCountOf(g_NIS_cs_dxil), defines.data(), (uint32_t)defines.size(), &bytecode, &size);
#    endif
#    if (NRI_ENABLE_VK_SUPPORT || NRI_ENABLE_WGPU_SUPPORT)
            if (deviceDesc.graphicsAPI == GraphicsAPI::VK || deviceDesc.graphicsAPI == GraphicsAPI::WGPU)
                shaderMakeResult = ShaderMake::FindPermutationInBlob(g_NIS_cs_spirv, GetCountOf(g_NIS_cs_spirv), defines.data(), (uint32_t)defines.size(), &bytecode, &size);
#    endif
            if (!shaderMakeResult)
                return Result::FAILURE;

            ComputePipelineDesc computePipelineDesc = {};
            computePipelineDesc.pipelineLayout = m.nis->pipelineLayout;
            computePipelineDesc.shader.stage = StageBits::COMPUTE_SHADER;
            computePipelineDesc.shader.bytecode = bytecode;
            computePipelineDesc.shader.size = size;

            Result result = m_iCore.CreateComputePipeline(m_Device, computePipelineDesc, m.nis->pipeline);
            if (result != Result::SUCCESS)
                return result;
        }

        { // Textures
            TextureDesc textureDesc = {};
            textureDesc.type = TextureType::TEXTURE_2D;
            textureDesc.usage = TextureUsageBits::SHADER_RESOURCE;
            textureDesc.format = Format::RGBA16_SFLOAT;
            textureDesc.width = NIS::kFilterSize / 4;
            textureDesc.height = NIS::kPhaseCount;
            textureDesc.mipNum = 1;
            textureDesc.layerNum = 1;

            Result result = m_iCore.CreateCommittedTexture(m_Device, MemoryLocation::DEVICE, 0.0f, textureDesc, m.nis->texScale);
            if (result != Result::SUCCESS)
                return result;

            result = m_iCore.CreateCommittedTexture(m_Device, MemoryLocation::DEVICE, 0.0f, textureDesc, m.nis->texUsm);
            if (result != Result::SUCCESS)
                return result;
        }

        { // Upload data // TODO: allow to merge with "UploadData" requests on user side
            HelperInterface iHelper = {};
            Result result = nriGetInterface(m_Device, NRI_INTERFACE(HelperInterface), &iHelper);
            if (result != Result::SUCCESS)
                return result;

            const uint32_t rowPitch = (NIS::kFilterSize / 4) * 8;
            const uint32_t slicePitch = rowPitch * NIS::kPhaseCount;

            std::array<TextureSubresourceUploadDesc, 2> subresources;
            subresources[0] = {NIS::coef_scale_fp16, 1, rowPitch, slicePitch};
            subresources[1] = {NIS::coef_usm_fp16, 1, rowPitch, slicePitch};

            std::array<TextureUploadDesc, 2> textureUploadDescs;
            textureUploadDescs[0] = {&subresources[0], m.nis->texScale, {AccessBits::SHADER_RESOURCE, Layout::SHADER_RESOURCE}};
            textureUploadDescs[1] = {&subresources[1], m.nis->texUsm, {AccessBits::SHADER_RESOURCE, Layout::SHADER_RESOURCE}};

            Queue* graphicsQueue = nullptr;
            result = m_iCore.GetQueue(m_Device, QueueType::GRAPHICS, 0, graphicsQueue);
            if (result != Result::SUCCESS)
                return result;

            result = iHelper.UploadData(*graphicsQueue, textureUploadDescs.data(), GetCountOf(textureUploadDescs), nullptr, 0);
            if (result != Result::SUCCESS)
                return result;
        }

        { // Descriptors
            SamplerDesc samplerDesc = {};
            samplerDesc.addressModes = {AddressMode::CLAMP_TO_EDGE, AddressMode::CLAMP_TO_EDGE};
            samplerDesc.filters = {Filter::LINEAR, Filter::LINEAR, Filter::LINEAR};

            Result result = m_iCore.CreateSampler(m_Device, samplerDesc, m.nis->sampler);
            if (result != Result::SUCCESS)
                return result;

            TextureViewDesc textureViewDesc = {};
            textureViewDesc.type = TextureView::TEXTURE;
            textureViewDesc.format = Format::RGBA16_SFLOAT;
            textureViewDesc.mipNum = 1;
            textureViewDesc.layerNum = 1;

            textureViewDesc.texture = m.nis->texScale;
            result = m_iCore.CreateTextureView(textureViewDesc, m.nis->srvScale);
            if (result != Result::SUCCESS)
                return result;

            textureViewDesc.texture = m.nis->texUsm;
            result = m_iCore.CreateTextureView(textureViewDesc, m.nis->srvUsm);
            if (result != Result::SUCCESS)
                return result;
        }

        { // Descriptor pool
            DescriptorPoolDesc descriptorPoolDesc = {};

            // Static
            descriptorPoolDesc.descriptorSetMaxNum = 1;
            descriptorPoolDesc.samplerMaxNum = 1;
            descriptorPoolDesc.textureMaxNum = 2;

            // Dynamic
            descriptorPoolDesc.descriptorSetMaxNum += NIS_DESCRIPTOR_SET_NUM;
            descriptorPoolDesc.textureMaxNum += 1 * NIS_DESCRIPTOR_SET_NUM;
            descriptorPoolDesc.storageTextureMaxNum += 1 * NIS_DESCRIPTOR_SET_NUM;
            descriptorPoolDesc.flags = DescriptorPoolBits::ALLOW_UPDATE_AFTER_SET;

            Result result = m_iCore.CreateDescriptorPool(m_Device, descriptorPoolDesc, m.nis->descriptorPool);
            if (result != Result::SUCCESS)
                return result;
        }

        { // Descriptor sets
            Result result = m_iCore.AllocateDescriptorSets(*m.nis->descriptorPool, *m.nis->pipelineLayout, NRI_NIS_SET_STATIC, &m.nis->descriptorSet0, 1, 0);
            if (result != Result::SUCCESS)
                return result;

            result = m_iCore.AllocateDescriptorSets(*m.nis->descriptorPool, *m.nis->pipelineLayout, NRI_NIS_SET_DYNAMIC, m.nis->descriptorSets1.data(), NIS_DESCRIPTOR_SET_NUM, 0);
            if (result != Result::SUCCESS)
                return result;
        }

        { // Update static descriptor set
            Descriptor* resources[] = {
                m.nis->srvScale,
                m.nis->srvUsm,
            };

            const UpdateDescriptorRangeDesc updateDescriptorRangeDescs[] = {
                {m.nis->descriptorSet0, 0, 0, &m.nis->sampler, 1},
                {m.nis->descriptorSet0, 1, 0, resources, GetCountOf(resources)},
            };

            m_iCore.UpdateDescriptorRanges(updateDescriptorRangeDescs, GetCountOf(updateDescriptorRangeDescs));
        }
    }
#endif

#if NRI_ENABLE_FFX_SDK
    if (upscalerDesc.type == UpscalerType::FSR) {
        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        m.ffx = Allocate<Ffx>(allocationCallbacks);
        m.ffx->sdkLock = &FfxGetSdkLock(deviceDesc.graphicsAPI);
        ExclusiveScope lock(*m.ffx->sdkLock);

        // Load library
        const char* libraryName = deviceDesc.graphicsAPI == GraphicsAPI::D3D12 ? "amd_fidelityfx_loader_dx12.dll" : "amd_fidelityfx_vk.dll";
        Library* ffxLibrary = LoadSharedLibrary(libraryName);
        if (!ffxLibrary)
            return Result::FAILURE;

        // Get functions
        m.ffx->library = ffxLibrary;
        m.ffx->CreateContext = (PfnFfxCreateContext)GetSharedLibraryFunction(*ffxLibrary, "ffxCreateContext");
        m.ffx->DestroyContext = (PfnFfxDestroyContext)GetSharedLibraryFunction(*ffxLibrary, "ffxDestroyContext");
        m.ffx->Dispatch = (PfnFfxDispatch)GetSharedLibraryFunction(*ffxLibrary, "ffxDispatch");
        m.ffx->Query = (PfnFfxQuery)GetSharedLibraryFunction(*ffxLibrary, "ffxQuery");

        if (!m.ffx->CreateContext || !m.ffx->DestroyContext || !m.ffx->Dispatch || !m.ffx->Query)
            return Result::FAILURE;

        // Allocation callbacks
        m.ffx->allocationCallbacks.alloc = FfxAlloc;
        m.ffx->allocationCallbacks.dealloc = FfxDealloc;
        m.ffx->allocationCallbacks.pUserData = (void*)(&((DeviceBase&)m_Device).GetAllocationCallbacks());

        if (!((DeviceBase&)m_Device).GetAllocationCallbacks().disable3rdPartyAllocationCallbacks)
            m.ffx->allocationCallbacksPtr = &m.ffx->allocationCallbacks;

        // Create context
        uint32_t flags = FFX_UPSCALE_ENABLE_DYNAMIC_RESOLUTION; // TODO: move to "UpscalerBits"?
        if (upscalerDesc.flags & UpscalerBits::HDR)
            flags |= FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE;
        if (upscalerDesc.flags & UpscalerBits::SRGB)
            flags |= FFX_UPSCALE_ENABLE_NON_LINEAR_COLORSPACE;
        if (!(upscalerDesc.flags & UpscalerBits::USE_EXPOSURE))
            flags |= FFX_UPSCALE_ENABLE_AUTO_EXPOSURE;
        if (upscalerDesc.flags & UpscalerBits::DEPTH_INVERTED)
            flags |= FFX_UPSCALE_ENABLE_DEPTH_INVERTED;
        if (upscalerDesc.flags & UpscalerBits::DEPTH_INFINITE)
            flags |= FFX_UPSCALE_ENABLE_DEPTH_INFINITE;
        if (upscalerDesc.flags & UpscalerBits::MV_UPSCALED)
            flags |= FFX_UPSCALE_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS;
        if (upscalerDesc.flags & UpscalerBits::MV_JITTERED)
            flags |= FFX_UPSCALE_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION;

        ffxCreateContextDescUpscale& contextDesc = m.ffx->contextDesc;
        contextDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
        contextDesc.maxRenderSize = {upscalerProps.renderResolution.w, upscalerProps.renderResolution.h};
        contextDesc.maxUpscaleSize = {upscalerProps.upscaleResolution.w, upscalerProps.upscaleResolution.h};
        contextDesc.flags = flags;

#    ifndef NDEBUG
        contextDesc.flags |= FFX_UPSCALE_ENABLE_DEBUG_CHECKING;
        contextDesc.fpMessage = FfxDebugMessage;
#    endif

#    if NRI_ENABLE_D3D12_SUPPORT
        ffxCreateBackendDX12Desc& backendD3D12Desc = m.ffx->backendD3D12Desc;
        ffxCreateContextDescUpscaleVersion& versionDesc = m.ffx->versionDesc;

        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12) {
            backendD3D12Desc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
            backendD3D12Desc.device = (ID3D12Device*)m_iCore.GetDeviceNativeObject(&m_Device);

            versionDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE_VERSION;
            versionDesc.version = FFX_UPSCALER_VERSION;
            backendD3D12Desc.header.pNext = &versionDesc.header;
            contextDesc.header.pNext = &backendD3D12Desc.header;
        }
#    endif

#    if NRI_ENABLE_VK_SUPPORT
        ffxCreateBackendVKDesc& backendVKDesc = m.ffx->backendVKDesc;

        if (deviceDesc.graphicsAPI == GraphicsAPI::VK) {
            WrapperVKInterface iWrapperVK = {};
            if (nriGetInterface(m_Device, NRI_INTERFACE(WrapperVKInterface), &iWrapperVK) != Result::SUCCESS)
                return Result::UNSUPPORTED;

            VkDevice vkDevice = (VkDevice)m_iCore.GetDeviceNativeObject(&m_Device);
            VkPhysicalDevice vkPhysicalDevice = (VkPhysicalDevice)iWrapperVK.GetPhysicalDeviceVK(m_Device);
            PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)iWrapperVK.GetDeviceProcAddrVK(m_Device);
            FfxRegisterDevice(m.ffx->vkPair, vkDevice, vkGetDeviceProcAddr);

            backendVKDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK;
            backendVKDesc.vkDevice = vkDevice;
            backendVKDesc.vkPhysicalDevice = vkPhysicalDevice;
            backendVKDesc.vkDeviceProcAddr = FfxVkGetDeviceProcAddr;

            contextDesc.header.pNext = &backendVKDesc.header;
        }
#    endif

        ffxReturnCode_t result = m.ffx->CreateContext(&m.ffx->context, &contextDesc.header, m.ffx->allocationCallbacksPtr);
        if (result != FFX_API_RETURN_OK)
            return FfxConvertError(result);

        ffxQueryGetProviderVersion providerVersion = {};
        providerVersion.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;
        result = m.ffx->Query(&m.ffx->context, &providerVersion.header);

        if (result == FFX_API_RETURN_OK)
            NRI_REPORT_INFO((DeviceBase*)&m_Device, "FSR provider: %s", providerVersion.versionName);
    }
#endif

#if NRI_ENABLE_XESS_SDK
    if (upscalerDesc.type == UpscalerType::XESS) {
        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        m.xess = Allocate<Xess>(allocationCallbacks);

#    if NRI_ENABLE_D3D12_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12) {
            ExclusiveScope lock(g_xess.lock);

            ID3D12Device* deviceNative = (ID3D12Device*)m_iCore.GetDeviceNativeObject(&m_Device);

            xess_result_t result = xessD3D12CreateContext(deviceNative, &m.xess->context);
            if (result != XESS_RESULT_SUCCESS)
                return XessConvertError(result);

            result = xessForceLegacyScaleFactors(m.xess->context, true);
            if (result != XESS_RESULT_SUCCESS)
                return XessConvertError(result);

            xess_quality_settings_t qualitySetting = XESS_QUALITY_SETTING_ULTRA_PERFORMANCE;
            if (upscalerDesc.mode == UpscalerMode::NATIVE)
                qualitySetting = XESS_QUALITY_SETTING_AA;
            else if (upscalerDesc.mode == UpscalerMode::ULTRA_QUALITY)
                qualitySetting = XESS_QUALITY_SETTING_ULTRA_QUALITY;
            else if (upscalerDesc.mode == UpscalerMode::QUALITY)
                qualitySetting = XESS_QUALITY_SETTING_QUALITY;
            else if (upscalerDesc.mode == UpscalerMode::BALANCED)
                qualitySetting = XESS_QUALITY_SETTING_BALANCED;
            else if (upscalerDesc.mode == UpscalerMode::PERFORMANCE)
                qualitySetting = XESS_QUALITY_SETTING_PERFORMANCE;
            else if (upscalerDesc.mode == UpscalerMode::ULTRA_PERFORMANCE)
                qualitySetting = XESS_QUALITY_SETTING_ULTRA_PERFORMANCE;

            uint32_t initFlags = 0;
            if (!(upscalerDesc.flags & UpscalerBits::HDR))
                initFlags |= XESS_INIT_FLAG_LDR_INPUT_COLOR;
            if (upscalerDesc.flags & UpscalerBits::USE_REACTIVE)
                initFlags |= XESS_INIT_FLAG_RESPONSIVE_PIXEL_MASK;
            if (upscalerDesc.flags & UpscalerBits::DEPTH_INVERTED)
                initFlags |= XESS_INIT_FLAG_INVERTED_DEPTH;
            if (upscalerDesc.flags & UpscalerBits::MV_UPSCALED)
                initFlags |= XESS_INIT_FLAG_HIGH_RES_MV;
            if (upscalerDesc.flags & UpscalerBits::MV_JITTERED)
                initFlags |= XESS_INIT_FLAG_JITTERED_MV;

            if (upscalerDesc.flags & UpscalerBits::USE_EXPOSURE)
                initFlags |= XESS_INIT_FLAG_EXPOSURE_SCALE_TEXTURE;
            else
                initFlags |= XESS_INIT_FLAG_ENABLE_AUTOEXPOSURE;

            xess_d3d12_init_params_t initParams = {};
            initParams.outputResolution = {upscalerProps.upscaleResolution.w, upscalerProps.upscaleResolution.h};
            initParams.qualitySetting = qualitySetting;
            initParams.initFlags = initFlags;
            initParams.creationNodeMask = NODE_MASK;
            initParams.visibleNodeMask = NODE_MASK;

            result = xessD3D12BuildPipelines(m.xess->context, NULL, true, initParams.initFlags);
            if (result != XESS_RESULT_SUCCESS)
                return XessConvertError(result);

            result = xessD3D12Init(m.xess->context, &initParams);
            if (result != XESS_RESULT_SUCCESS)
                return XessConvertError(result);

            if (upscalerDesc.preset != 0) {
                xess_network_model_t preset = (xess_network_model_t)(upscalerDesc.preset - 1);
                result = xessSelectNetworkModel(m.xess->context, preset);
                if (result != XESS_RESULT_SUCCESS)
                    return XessConvertError(result);
            }
        }
#    endif
    }
#endif

#if NRI_ENABLE_NGX_SDK
    if (upscalerDesc.type == UpscalerType::DLSR || upscalerDesc.type == UpscalerType::DLRR) {
        void* deviceNative = m_iCore.GetDeviceNativeObject(&m_Device);
        const wchar_t* path = L""; // don't care
        NVSDK_NGX_Result ngxResult = NVSDK_NGX_Result_Fail;

        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        m.ngx = Allocate<Ngx>(allocationCallbacks);

        { // Create instance
            ExclusiveScope lock(g_ngx.lock);

            NVSDK_NGX_FeatureCommonInfo featureCommonInfo = {};
            featureCommonInfo.LoggingInfo.LoggingCallback = NgxLogCallback;
            featureCommonInfo.LoggingInfo.MinimumLoggingLevel = NVSDK_NGX_LOGGING_LEVEL_OFF; // TODO: NGX spams to "stdout" if not OFF
            featureCommonInfo.LoggingInfo.DisableOtherLoggingSinks = true;

#    if NRI_ENABLE_D3D11_SUPPORT
            if (deviceDesc.graphicsAPI == GraphicsAPI::D3D11) {
                ngxResult = NVSDK_NGX_D3D11_Init(APPLICATION_ID, path, (ID3D11Device*)deviceNative, &featureCommonInfo);
                if (ngxResult == NVSDK_NGX_Result_Success) {
                    NgxIncrRef(deviceNative);
                    ngxResult = NVSDK_NGX_D3D11_GetCapabilityParameters(&m.ngx->params);
                }
            }
#    endif

#    if NRI_ENABLE_D3D12_SUPPORT
            if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12) {
                ngxResult = NVSDK_NGX_D3D12_Init(APPLICATION_ID, path, (ID3D12Device*)deviceNative, &featureCommonInfo);
                if (ngxResult == NVSDK_NGX_Result_Success) {
                    NgxIncrRef(deviceNative);
                    ngxResult = NVSDK_NGX_D3D12_GetCapabilityParameters(&m.ngx->params);
                }
            }
#    endif

#    if NRI_ENABLE_VK_SUPPORT
            if (deviceDesc.graphicsAPI == GraphicsAPI::VK) {
                WrapperVKInterface iWrapperVK = {};
                if (nriGetInterface(m_Device, NRI_INTERFACE(WrapperVKInterface), &iWrapperVK) != Result::SUCCESS)
                    return Result::UNSUPPORTED;

                VkPhysicalDevice vkPhysicalDevice = (VkPhysicalDevice)iWrapperVK.GetPhysicalDeviceVK(m_Device);
                VkInstance vkInstance = (VkInstance)iWrapperVK.GetInstanceVK(m_Device);
                PFN_vkGetInstanceProcAddr vkGipa = (PFN_vkGetInstanceProcAddr)iWrapperVK.GetInstanceProcAddrVK(m_Device);
                PFN_vkGetDeviceProcAddr vkGdpa = (PFN_vkGetDeviceProcAddr)iWrapperVK.GetDeviceProcAddrVK(m_Device);

                ngxResult = NVSDK_NGX_VULKAN_Init(APPLICATION_ID, path, vkInstance, vkPhysicalDevice, (VkDevice)deviceNative, vkGipa, vkGdpa, &featureCommonInfo);
                if (ngxResult == NVSDK_NGX_Result_Success) {
                    NgxIncrRef(deviceNative);
                    ngxResult = NVSDK_NGX_VULKAN_GetCapabilityParameters(&m.ngx->params);
                }
            }
#    endif
        }

        if (ngxResult != NVSDK_NGX_Result_Success)
            return Result::FAILURE;

        // Create command buffer if not provided
        Queue* graphicsQueue = nullptr;
        CommandAllocator* commandAllocator = nullptr;
        CommandBuffer* commandBuffer = upscalerDesc.commandBuffer;
        Fence* fence = nullptr;
        Result result = Result::SUCCESS;

        if (!upscalerDesc.commandBuffer) {
            result = m_iCore.GetQueue(m_Device, QueueType::GRAPHICS, 0, graphicsQueue);
            if (result == Result::SUCCESS)
                result = m_iCore.CreateCommandAllocator(*graphicsQueue, commandAllocator);
            if (result == Result::SUCCESS)
                result = m_iCore.CreateCommandBuffer(*commandAllocator, commandBuffer);
            if (result == Result::SUCCESS)
                result = m_iCore.CreateFence(m_Device, 0, fence);
            if (result == Result::SUCCESS)
                result = m_iCore.BeginCommandBuffer(*commandBuffer, nullptr);
        }

        if (result == Result::SUCCESS) { // Record creation commands
            ExclusiveScope lock(g_ngx.lock);

            void* commandBufferNative = m_iCore.GetCommandBufferNativeObject(commandBuffer);

            NVSDK_NGX_PerfQuality_Value qualityValue = NVSDK_NGX_PerfQuality_Value_UltraPerformance;
            if (upscalerDesc.mode == UpscalerMode::NATIVE) {
                qualityValue = NVSDK_NGX_PerfQuality_Value_DLAA;
                NVSDK_NGX_Parameter_SetUI(m.ngx->params, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA, upscalerDesc.preset);
            } else if (upscalerDesc.mode == UpscalerMode::QUALITY || upscalerDesc.mode == UpscalerMode::ULTRA_QUALITY) {
                qualityValue = NVSDK_NGX_PerfQuality_Value_MaxQuality;
                NVSDK_NGX_Parameter_SetUI(m.ngx->params, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality, upscalerDesc.preset);
            } else if (upscalerDesc.mode == UpscalerMode::BALANCED) {
                qualityValue = NVSDK_NGX_PerfQuality_Value_Balanced;
                NVSDK_NGX_Parameter_SetUI(m.ngx->params, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced, upscalerDesc.preset);
            } else if (upscalerDesc.mode == UpscalerMode::PERFORMANCE) {
                qualityValue = NVSDK_NGX_PerfQuality_Value_MaxPerf;
                NVSDK_NGX_Parameter_SetUI(m.ngx->params, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance, upscalerDesc.preset);
            } else if (upscalerDesc.mode == UpscalerMode::ULTRA_PERFORMANCE) {
                qualityValue = NVSDK_NGX_PerfQuality_Value_UltraPerformance;
                NVSDK_NGX_Parameter_SetUI(m.ngx->params, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance, upscalerDesc.preset);
            }

            int32_t featureCreateFlags = 0;
            if (upscalerDesc.flags & UpscalerBits::HDR)
                featureCreateFlags |= NVSDK_NGX_DLSS_Feature_Flags_IsHDR;
            if (!(upscalerDesc.flags & UpscalerBits::USE_EXPOSURE))
                featureCreateFlags |= NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;
            if (upscalerDesc.flags & UpscalerBits::DEPTH_INVERTED)
                featureCreateFlags |= NVSDK_NGX_DLSS_Feature_Flags_DepthInverted;
            if (!(upscalerDesc.flags & UpscalerBits::MV_UPSCALED))
                featureCreateFlags |= NVSDK_NGX_DLSS_Feature_Flags_MVLowRes;
            if (upscalerDesc.flags & UpscalerBits::MV_JITTERED)
                featureCreateFlags |= NVSDK_NGX_DLSS_Feature_Flags_MVJittered;

            if (upscalerDesc.type == UpscalerType::DLSR) {
                NVSDK_NGX_DLSS_Create_Params srCreateParams = {};
                srCreateParams.Feature.InWidth = upscalerProps.renderResolution.w;
                srCreateParams.Feature.InHeight = upscalerProps.renderResolution.h;
                srCreateParams.Feature.InTargetWidth = upscalerProps.upscaleResolution.w;
                srCreateParams.Feature.InTargetHeight = upscalerProps.upscaleResolution.h;
                srCreateParams.Feature.InPerfQualityValue = qualityValue;
                srCreateParams.InFeatureCreateFlags = featureCreateFlags;

#    if NRI_ENABLE_D3D11_SUPPORT
                if (deviceDesc.graphicsAPI == GraphicsAPI::D3D11)
                    ngxResult = NGX_D3D11_CREATE_DLSS_EXT((ID3D11DeviceContext*)commandBufferNative, &m.ngx->handle, m.ngx->params, &srCreateParams);
#    endif

#    if NRI_ENABLE_D3D12_SUPPORT
                if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12)
                    ngxResult = NGX_D3D12_CREATE_DLSS_EXT((ID3D12GraphicsCommandList*)commandBufferNative, NODE_MASK, NODE_MASK, &m.ngx->handle, m.ngx->params, &srCreateParams);
#    endif

#    if NRI_ENABLE_VK_SUPPORT
                if (deviceDesc.graphicsAPI == GraphicsAPI::VK)
                    ngxResult = NGX_VULKAN_CREATE_DLSS_EXT1((VkDevice)deviceNative, (VkCommandBuffer)commandBufferNative, NODE_MASK, NODE_MASK, &m.ngx->handle, m.ngx->params, &srCreateParams);
#    endif
            }

            if (upscalerDesc.type == UpscalerType::DLRR) {
                NVSDK_NGX_DLSSD_Create_Params rrCreateParams = {};
                rrCreateParams.InDenoiseMode = NVSDK_NGX_DLSS_Denoise_Mode_DLUnified;
                rrCreateParams.InRoughnessMode = NVSDK_NGX_DLSS_Roughness_Mode_Packed;
                rrCreateParams.InUseHWDepth = (upscalerDesc.flags & UpscalerBits::DEPTH_LINEAR) ? NVSDK_NGX_DLSS_Depth_Type_Linear : NVSDK_NGX_DLSS_Depth_Type_HW;
                rrCreateParams.InWidth = upscalerProps.renderResolution.w;
                rrCreateParams.InHeight = upscalerProps.renderResolution.h;
                rrCreateParams.InTargetWidth = upscalerProps.upscaleResolution.w;
                rrCreateParams.InTargetHeight = upscalerProps.upscaleResolution.h;
                rrCreateParams.InPerfQualityValue = qualityValue;
                rrCreateParams.InFeatureCreateFlags = featureCreateFlags;

#    if NRI_ENABLE_D3D11_SUPPORT
                if (deviceDesc.graphicsAPI == GraphicsAPI::D3D11)
                    ngxResult = NGX_D3D11_CREATE_DLSSD_EXT((ID3D11DeviceContext*)commandBufferNative, &m.ngx->handle, m.ngx->params, &rrCreateParams);
#    endif

#    if NRI_ENABLE_D3D12_SUPPORT
                if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12)
                    ngxResult = NGX_D3D12_CREATE_DLSSD_EXT((ID3D12GraphicsCommandList*)commandBufferNative, NODE_MASK, NODE_MASK, &m.ngx->handle, m.ngx->params, &rrCreateParams);
#    endif

#    if NRI_ENABLE_VK_SUPPORT
                if (deviceDesc.graphicsAPI == GraphicsAPI::VK)
                    ngxResult = NGX_VULKAN_CREATE_DLSSD_EXT1((VkDevice)deviceNative, (VkCommandBuffer)commandBufferNative, NODE_MASK, NODE_MASK, &m.ngx->handle, m.ngx->params, &rrCreateParams);
#    endif
            }
        }

        if (!upscalerDesc.commandBuffer) {
            if (result == Result::SUCCESS) {
                result = m_iCore.EndCommandBuffer(*commandBuffer);

                if (result == Result::SUCCESS) {
                    // Submit & wait for completion
                    FenceSubmitDesc signalFence = {};
                    signalFence.fence = fence;
                    signalFence.value = 1;

                    QueueSubmitDesc queueSubmitDesc = {};
                    queueSubmitDesc.commandBuffers = &commandBuffer;
                    queueSubmitDesc.commandBufferNum = 1;
                    queueSubmitDesc.signalFences = &signalFence;
                    queueSubmitDesc.signalFenceNum = 1;

                    result = m_iCore.QueueSubmit(*graphicsQueue, queueSubmitDesc);
                }

                if (result == Result::SUCCESS)
                    m_iCore.Wait(*fence, 1);
            }

            // Cleanup
            if (fence)
                m_iCore.DestroyFence(fence);
            if (commandBuffer)
                m_iCore.DestroyCommandBuffer(commandBuffer);
            if (commandAllocator)
                m_iCore.DestroyCommandAllocator(commandAllocator);
        }

        if (result != Result::SUCCESS)
            return result;

        if (ngxResult != NVSDK_NGX_Result_Success)
            return Result::FAILURE;
    }
#endif

#if NRI_ENABLE_METAL_SUPPORT
    if (IsMetalFx(upscalerDesc.type)) {
        MTL::Device* deviceNative = (MTL::Device*)m_iCore.GetDeviceNativeObject(&m_Device);

        bool isSupported = false;
        float scaleMin = 1.0f;
        float scaleMax = upscalerProps.scalingFactor;

        if (upscalerDesc.type == UpscalerType::METALFX_SPATIAL)
            isSupported = MTLFX::SpatialScalerDescriptor::supportsMetal4FX(deviceNative);
        else if (upscalerDesc.type == UpscalerType::METALFX_TEMPORAL) {
            isSupported = MTLFX::TemporalScalerDescriptor::supportsMetal4FX(deviceNative);

            // "MV_UPSCALED" requires macOS 27
            if (upscalerDesc.flags & UpscalerBits::MV_UPSCALED)
                isSupported = isSupported && class_respondsToSelector((Class)_MTLFX_PRIVATE_CLS(MTLFXTemporalScalerDescriptor), _MTLFX_PRIVATE_SEL(setOutputResolutionMotionVectorsEnabled_));

            scaleMin = MTLFX::TemporalScalerDescriptor::supportedInputContentMinScale(deviceNative);
            scaleMax = MTLFX::TemporalScalerDescriptor::supportedInputContentMaxScale(deviceNative);
        } else {
            isSupported = MTLFX::TemporalDenoisedScalerDescriptor::supportsMetal4FX(deviceNative);
            scaleMin = MTLFX::TemporalDenoisedScalerDescriptor::supportedInputContentMinScale(deviceNative);
            scaleMax = MTLFX::TemporalDenoisedScalerDescriptor::supportedInputContentMaxScale(deviceNative);
        }

        if (!isSupported)
            return Result::UNSUPPORTED;

        // Scaling factors, including dynamic resolution, must be in the supported range
        if (upscalerDesc.type != UpscalerType::METALFX_SPATIAL) {
            float scaleW = (float)upscalerProps.upscaleResolution.w / (float)upscalerProps.renderResolutionMin.w;
            float scaleH = (float)upscalerProps.upscaleResolution.h / (float)upscalerProps.renderResolutionMin.h;

            if (upscalerProps.scalingFactor < scaleMin || scaleW > scaleMax || scaleH > scaleMax)
                return Result::UNSUPPORTED;
        }

        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        m.metalfx = Allocate<MetalFx>(allocationCallbacks);

        // Scalers are created on the first dispatch, because resource formats are unknown here
        MTL4::CompilerDescriptor* compilerDesc = MTL4::CompilerDescriptor::alloc()->init();
        m.metalfx->compiler = deviceNative->newCompiler(compilerDesc, nullptr);
        compilerDesc->release();

        if (!m.metalfx->compiler)
            return Result::FAILURE;
    }
#endif

    return Result::SUCCESS;
}

void UpscalerImpl::GetUpscalerProps(UpscalerProps& upscalerProps) const {
    float scalingFactor = 1.0f;
    if (m_Desc.mode == UpscalerMode::ULTRA_QUALITY)
        scalingFactor = 1.3f;
    else if (m_Desc.mode == UpscalerMode::QUALITY)
        scalingFactor = 1.5f;
    else if (m_Desc.mode == UpscalerMode::BALANCED)
        scalingFactor = 1.7f;
    else if (m_Desc.mode == UpscalerMode::PERFORMANCE)
        scalingFactor = 2.0f;
    else if (m_Desc.mode == UpscalerMode::ULTRA_PERFORMANCE)
        scalingFactor = 3.0f;

    upscalerProps = {};
    upscalerProps.scalingFactor = scalingFactor;
    upscalerProps.mipBias = -std::log2(scalingFactor) - 1;
    upscalerProps.upscaleResolution = m_Desc.upscaleResolution;
    upscalerProps.renderResolution.w = (Dim_t)(m_Desc.upscaleResolution.w / scalingFactor + 0.5f);
    upscalerProps.renderResolution.h = (Dim_t)(m_Desc.upscaleResolution.h / scalingFactor + 0.5f);
    upscalerProps.jitterPhaseNum = (uint8_t)std::ceil(8.0f * scalingFactor * scalingFactor);

    if (m_Desc.type == UpscalerType::NIS || m_Desc.type == UpscalerType::METALFX_SPATIAL) {
        upscalerProps.renderResolutionMin.w = 0;
        upscalerProps.renderResolutionMin.h = 0;
    } else if (m_Desc.type != UpscalerType::METALFX_DENOISED && (m_Desc.mode == UpscalerMode::ULTRA_QUALITY || m_Desc.mode == UpscalerMode::QUALITY || m_Desc.mode == UpscalerMode::BALANCED)) {
        upscalerProps.renderResolutionMin.w = m_Desc.upscaleResolution.w / 2;
        upscalerProps.renderResolutionMin.h = m_Desc.upscaleResolution.h / 2;
    } else
        upscalerProps.renderResolutionMin = upscalerProps.renderResolution; // no DRS support because of DLSS
}

void UpscalerImpl::CmdDispatchUpscale(CommandBuffer& commandBuffer, const DispatchUpscaleDesc& dispatchUpscaleDesc) {
    const UpscalerResource& output = dispatchUpscaleDesc.output;
    const UpscalerResource& input = dispatchUpscaleDesc.input;

    const float preExposure = dispatchUpscaleDesc.preExposure == 0.0f ? 1.0f : dispatchUpscaleDesc.preExposure;

    MaybeUnused(commandBuffer, output, input, preExposure);

#if NRI_ENABLE_NIS_SDK
    if (m_Desc.type == UpscalerType::NIS) {
        DescriptorSet* descriptorSet1 = m.nis->descriptorSets1[m.nis->descriptorSetIndex];

        { // Setup
            m_iCore.CmdSetDescriptorPool(commandBuffer, *m.nis->descriptorPool);
            m_iCore.CmdSetPipelineLayout(commandBuffer, BindPoint::COMPUTE, *m.nis->pipelineLayout);
            m_iCore.CmdSetPipeline(commandBuffer, *m.nis->pipeline);

            SetDescriptorSetDesc staticSet = {NRI_NIS_SET_STATIC, m.nis->descriptorSet0};
            m_iCore.CmdSetDescriptorSet(commandBuffer, staticSet);

            SetDescriptorSetDesc dynamicSet = {NRI_NIS_SET_DYNAMIC, descriptorSet1};
            m_iCore.CmdSetDescriptorSet(commandBuffer, dynamicSet);
        }

        { // Update constants
            const TextureDesc& inputDesc = m_iCore.GetTextureDesc(*input.texture);

            NIS::Constants constants = {};
            NIS::UpdateConstants(constants, dispatchUpscaleDesc.settings.nis.sharpness,
                dispatchUpscaleDesc.currentResolution.w, dispatchUpscaleDesc.currentResolution.h, // render resolution
                inputDesc.width, inputDesc.height,                                                // input dims
                m_Desc.upscaleResolution.w, m_Desc.upscaleResolution.h,                           // output resolution
                m_Desc.upscaleResolution.w, m_Desc.upscaleResolution.h,                           // output dims
                (m_Desc.flags & UpscalerBits::HDR) ? NIS::HDRMode::Linear : NIS::HDRMode::None);

            SetRootConstantsDesc rootConstants = {0, &constants, sizeof(constants)};
            m_iCore.CmdSetRootConstants(commandBuffer, rootConstants);
        }

        { // Update dynamic resources
            const UpdateDescriptorRangeDesc updateDescriptorRangeDescs[] = {
                {descriptorSet1, 0, 0, &input.descriptor, 1},
                {descriptorSet1, 1, 0, &output.descriptor, 1},
            };

            m_iCore.UpdateDescriptorRanges(updateDescriptorRangeDescs, GetCountOf(updateDescriptorRangeDescs));
        }

        { // Dispatch
            DispatchDesc dispatchDesc = {};
            dispatchDesc.workGroupNumX = (m_Desc.upscaleResolution.w + m.nis->blockSize.w - 1) / m.nis->blockSize.w;
            dispatchDesc.workGroupNumY = (m_Desc.upscaleResolution.h + m.nis->blockSize.h - 1) / m.nis->blockSize.h;
            dispatchDesc.workGroupNumZ = 1;

            m_iCore.CmdDispatch(commandBuffer, dispatchDesc);
        }

        // Update descriptor set for the next time
        m.nis->descriptorSetIndex++;
        m.nis->descriptorSetIndex %= NIS_DESCRIPTOR_SET_NUM;
    }
#endif

#if NRI_ENABLE_FFX_SDK
    if (m_Desc.type == UpscalerType::FSR) {
        ExclusiveScope lock(*m.ffx->sdkLock);

        const UpscalerGuides& guides = dispatchUpscaleDesc.guides.upscaler;

        ffxDispatchDescUpscale dispatchDesc = {};
        dispatchDesc.header.type = FFX_API_DISPATCH_DESC_TYPE_UPSCALE;
        dispatchDesc.commandList = m_iCore.GetCommandBufferNativeObject(&commandBuffer);
        dispatchDesc.output = FfxGetResource(m_iCore, output, true);
        dispatchDesc.color = FfxGetResource(m_iCore, input);
        dispatchDesc.depth = FfxGetResource(m_iCore, guides.depth);
        dispatchDesc.motionVectors = FfxGetResource(m_iCore, guides.mv);
        dispatchDesc.exposure = FfxGetResource(m_iCore, guides.exposure);
        dispatchDesc.reactive = FfxGetResource(m_iCore, guides.reactive);
        dispatchDesc.jitterOffset = {dispatchUpscaleDesc.cameraJitter.x, dispatchUpscaleDesc.cameraJitter.y};
        dispatchDesc.motionVectorScale = {dispatchUpscaleDesc.mvScale.x, dispatchUpscaleDesc.mvScale.y};
        dispatchDesc.renderSize = {dispatchUpscaleDesc.currentResolution.w, dispatchUpscaleDesc.currentResolution.h};
        dispatchDesc.enableSharpening = dispatchUpscaleDesc.settings.fsr.sharpness != 0.0f;
        dispatchDesc.sharpness = dispatchUpscaleDesc.settings.fsr.sharpness;
        dispatchDesc.frameTimeDelta = dispatchUpscaleDesc.settings.fsr.frameTime;
        dispatchDesc.preExposure = preExposure;
        dispatchDesc.reset = (dispatchUpscaleDesc.flags & DispatchUpscaleBits::RESET_HISTORY) != 0;
        const float zNear = dispatchUpscaleDesc.settings.fsr.zNear;
        const float zFar = (m_Desc.flags & UpscalerBits::DEPTH_INFINITE) ? FLT_MAX : dispatchUpscaleDesc.settings.fsr.zFar;

        // FFX swaps camera near and far for inverted depth
        dispatchDesc.cameraNear = (m_Desc.flags & UpscalerBits::DEPTH_INVERTED) ? zFar : zNear;
        dispatchDesc.cameraFar = (m_Desc.flags & UpscalerBits::DEPTH_INVERTED) ? zNear : zFar;
        dispatchDesc.cameraFovAngleVertical = dispatchUpscaleDesc.settings.fsr.verticalFov;
        dispatchDesc.viewSpaceToMetersFactor = dispatchUpscaleDesc.settings.fsr.viewSpaceToMetersFactor;
        dispatchDesc.flags = (m_Desc.flags & UpscalerBits::SRGB) ? FFX_UPSCALE_FLAG_NON_LINEAR_COLOR_SRGB : 0;

        ffxReturnCode_t result = m.ffx->Dispatch(&m.ffx->context, &dispatchDesc.header);
        MaybeUnused(result);
        NRI_CHECK(result == FFX_API_RETURN_OK, "ffxDispatch() failed!");
    }
#endif

#if NRI_ENABLE_XESS_SDK
    if (m_Desc.type == UpscalerType::XESS) {
        const UpscalerGuides& guides = dispatchUpscaleDesc.guides.upscaler;

        xess_d3d12_execute_params_t executeParams = {};
        executeParams.pColorTexture = (ID3D12Resource*)m_iCore.GetTextureNativeObject(input.texture);
        executeParams.pVelocityTexture = (ID3D12Resource*)m_iCore.GetTextureNativeObject(guides.mv.texture);
        executeParams.pDepthTexture = (ID3D12Resource*)m_iCore.GetTextureNativeObject(guides.depth.texture);
        executeParams.pExposureScaleTexture = (ID3D12Resource*)m_iCore.GetTextureNativeObject(guides.exposure.texture);
        executeParams.pResponsivePixelMaskTexture = (ID3D12Resource*)m_iCore.GetTextureNativeObject(guides.reactive.texture);
        executeParams.pOutputTexture = (ID3D12Resource*)m_iCore.GetTextureNativeObject(output.texture);
        executeParams.jitterOffsetX = dispatchUpscaleDesc.cameraJitter.x;
        executeParams.jitterOffsetY = dispatchUpscaleDesc.cameraJitter.y;
        executeParams.exposureScale = 1.0f;
        executeParams.resetHistory = (dispatchUpscaleDesc.flags & DispatchUpscaleBits::RESET_HISTORY) != 0;
        executeParams.inputWidth = dispatchUpscaleDesc.currentResolution.w;
        executeParams.inputHeight = dispatchUpscaleDesc.currentResolution.h;

        ID3D12GraphicsCommandList* commandList = (ID3D12GraphicsCommandList*)m_iCore.GetCommandBufferNativeObject(&commandBuffer);

        xess_result_t result = xessSetExposureMultiplier(m.xess->context, 1.0f / preExposure);
        NRI_CHECK(result == XESS_RESULT_SUCCESS, "xessSetExposureMultiplier() failed!");

        result = xessD3D12Execute(m.xess->context, commandList, &executeParams);
        MaybeUnused(result);
        NRI_CHECK(result == XESS_RESULT_SUCCESS, "xessD3D12Execute() failed!");
    }
#endif

#if NRI_ENABLE_NGX_SDK
    if (m_Desc.type == UpscalerType::DLSR) {
        ExclusiveScope lock(g_ngx.lock);

        const DeviceDesc& deviceDesc = m_iCore.GetDeviceDesc(m_Device);
        const UpscalerGuides& guides = dispatchUpscaleDesc.guides.upscaler;

        uint64_t outputNative = m_iCore.GetTextureNativeObject(output.texture);
        uint64_t inputNative = m_iCore.GetTextureNativeObject(input.texture);
        uint64_t mvNative = m_iCore.GetTextureNativeObject(guides.mv.texture);
        uint64_t depthNative = m_iCore.GetTextureNativeObject(guides.depth.texture);
        uint64_t exposureNative = m_iCore.GetTextureNativeObject(guides.exposure.texture);
        uint64_t reactiveNative = m_iCore.GetTextureNativeObject(guides.reactive.texture);

        void* commandBufferNative = m_iCore.GetCommandBufferNativeObject(&commandBuffer);

        NgxSetEvalParams(m.ngx->params, dispatchUpscaleDesc, preExposure);
        NVSDK_NGX_Parameter_SetF(m.ngx->params, NVSDK_NGX_Parameter_Sharpness, 0.0f);

        NVSDK_NGX_Result result = NVSDK_NGX_Result_Fail;

#    if NRI_ENABLE_D3D11_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D11) {
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_Color, (ID3D11Resource*)inputNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_Output, (ID3D11Resource*)outputNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_MotionVectors, (ID3D11Resource*)mvNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_Depth, (ID3D11Resource*)depthNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_ExposureTexture, (ID3D11Resource*)exposureNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask, (ID3D11Resource*)reactiveNative);

            result = NVSDK_NGX_D3D11_EvaluateFeature_C((ID3D11DeviceContext*)commandBufferNative, m.ngx->handle, m.ngx->params, nullptr);
        }
#    endif

#    if NRI_ENABLE_D3D12_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12) {
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_Color, (ID3D12Resource*)inputNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_Output, (ID3D12Resource*)outputNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_MotionVectors, (ID3D12Resource*)mvNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_Depth, (ID3D12Resource*)depthNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_ExposureTexture, (ID3D12Resource*)exposureNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask, (ID3D12Resource*)reactiveNative);

            result = NVSDK_NGX_D3D12_EvaluateFeature_C((ID3D12GraphicsCommandList*)commandBufferNative, m.ngx->handle, m.ngx->params, nullptr);
        }
#    endif

#    if NRI_ENABLE_VK_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::VK) {
            NVSDK_NGX_Resource_VK outputVk = NgxGetResource(m_iCore, output, outputNative, true);
            NVSDK_NGX_Resource_VK inputVk = NgxGetResource(m_iCore, input, inputNative);
            NVSDK_NGX_Resource_VK mvVk = NgxGetResource(m_iCore, guides.mv, mvNative);
            NVSDK_NGX_Resource_VK depthVk = NgxGetResource(m_iCore, guides.depth, depthNative);
            NVSDK_NGX_Resource_VK exposureVk = NgxGetResource(m_iCore, guides.exposure, exposureNative);
            NVSDK_NGX_Resource_VK reactiveVk = NgxGetResource(m_iCore, guides.reactive, reactiveNative);

            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_Color, &inputVk);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_Output, &outputVk);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_MotionVectors, &mvVk);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_Depth, &depthVk);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_ExposureTexture, guides.exposure.texture ? &exposureVk : nullptr);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask, guides.reactive.texture ? &reactiveVk : nullptr);

            result = NVSDK_NGX_VULKAN_EvaluateFeature_C((VkCommandBuffer)commandBufferNative, m.ngx->handle, m.ngx->params, nullptr);
        }
#    endif

        NRI_CHECK(result == NVSDK_NGX_Result_Success, "DLSR evaluation failed!");
    }

    if (m_Desc.type == UpscalerType::DLRR) {
        ExclusiveScope lock(g_ngx.lock);

        const DeviceDesc& deviceDesc = m_iCore.GetDeviceDesc(m_Device);
        const DenoiserGuides& guides = dispatchUpscaleDesc.guides.denoiser;

        uint64_t outputNative = m_iCore.GetTextureNativeObject(output.texture);
        uint64_t inputNative = m_iCore.GetTextureNativeObject(input.texture);
        uint64_t mvNative = m_iCore.GetTextureNativeObject(guides.mv.texture);
        uint64_t depthNative = m_iCore.GetTextureNativeObject(guides.depth.texture);
        uint64_t normalRoughnessNative = m_iCore.GetTextureNativeObject(guides.normalRoughness.texture);
        uint64_t diffuseAlbedoNative = m_iCore.GetTextureNativeObject(guides.diffuseAlbedo.texture);
        uint64_t specularAlbedoNative = m_iCore.GetTextureNativeObject(guides.specularAlbedo.texture);
        uint64_t specularMvOrHitTNative = m_iCore.GetTextureNativeObject(guides.specularMvOrHitT.texture);
        uint64_t exposureNative = m_iCore.GetTextureNativeObject(guides.exposure.texture);
        uint64_t reactiveNative = m_iCore.GetTextureNativeObject(guides.reactive.texture);
        uint64_t sssNative = m_iCore.GetTextureNativeObject(guides.sss.texture);
        uint64_t preTransparencyNative = m_iCore.GetTextureNativeObject(guides.preTransparency.texture);

        void* commandBufferNative = m_iCore.GetCommandBufferNativeObject(&commandBuffer);

        NgxSetEvalParams(m.ngx->params, dispatchUpscaleDesc, preExposure);

        const bool useSpecularMotion = (dispatchUpscaleDesc.flags & DispatchUpscaleBits::USE_SPECULAR_MOTION) != 0;
        NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_DLSS_WORLD_TO_VIEW_MATRIX, useSpecularMotion ? nullptr : (float*)dispatchUpscaleDesc.settings.dlrr.worldToViewMatrix);
        NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_DLSS_VIEW_TO_CLIP_MATRIX, useSpecularMotion ? nullptr : (float*)dispatchUpscaleDesc.settings.dlrr.viewToClipMatrix);

        NVSDK_NGX_Result result = NVSDK_NGX_Result_Fail;

#    if NRI_ENABLE_D3D11_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D11) {
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_Color, (ID3D11Resource*)inputNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_Output, (ID3D11Resource*)outputNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_MotionVectors, (ID3D11Resource*)mvNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_Depth, (ID3D11Resource*)depthNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_DiffuseAlbedo, (ID3D11Resource*)diffuseAlbedoNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_SpecularAlbedo, (ID3D11Resource*)specularAlbedoNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_ExposureTexture, (ID3D11Resource*)exposureNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_GBuffer_Normals, (ID3D11Resource*)normalRoughnessNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_GBuffer_SpecularMvec, (ID3D11Resource*)(useSpecularMotion ? specularMvOrHitTNative : 0));
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_DLSSD_ResponsivityMask, (ID3D11Resource*)reactiveNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_DLSSD_SpecularHitDistance, (ID3D11Resource*)(useSpecularMotion ? 0 : specularMvOrHitTNative));
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_DLSSD_ScreenSpaceSubsurfaceScatteringGuide, (ID3D11Resource*)sssNative);
            NVSDK_NGX_Parameter_SetD3d11Resource(m.ngx->params, NVSDK_NGX_Parameter_DLSSD_ColorBeforeTransparency, (ID3D11Resource*)preTransparencyNative);

            result = NVSDK_NGX_D3D11_EvaluateFeature_C((ID3D11DeviceContext*)commandBufferNative, m.ngx->handle, m.ngx->params, nullptr);
        }
#    endif

#    if NRI_ENABLE_D3D12_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12) {
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_Color, (ID3D12Resource*)inputNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_Output, (ID3D12Resource*)outputNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_MotionVectors, (ID3D12Resource*)mvNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_Depth, (ID3D12Resource*)depthNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_DiffuseAlbedo, (ID3D12Resource*)diffuseAlbedoNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_SpecularAlbedo, (ID3D12Resource*)specularAlbedoNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_ExposureTexture, (ID3D12Resource*)exposureNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_GBuffer_Normals, (ID3D12Resource*)normalRoughnessNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_GBuffer_SpecularMvec, (ID3D12Resource*)(useSpecularMotion ? specularMvOrHitTNative : 0));
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_DLSSD_ResponsivityMask, (ID3D12Resource*)reactiveNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_DLSSD_SpecularHitDistance, (ID3D12Resource*)(useSpecularMotion ? 0 : specularMvOrHitTNative));
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_DLSSD_ScreenSpaceSubsurfaceScatteringGuide, (ID3D12Resource*)sssNative);
            NVSDK_NGX_Parameter_SetD3d12Resource(m.ngx->params, NVSDK_NGX_Parameter_DLSSD_ColorBeforeTransparency, (ID3D12Resource*)preTransparencyNative);

            result = NVSDK_NGX_D3D12_EvaluateFeature_C((ID3D12GraphicsCommandList*)commandBufferNative, m.ngx->handle, m.ngx->params, nullptr);
        }
#    endif

#    if NRI_ENABLE_VK_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::VK) {
            NVSDK_NGX_Resource_VK outputVk = NgxGetResource(m_iCore, output, outputNative, true);
            NVSDK_NGX_Resource_VK inputVk = NgxGetResource(m_iCore, input, inputNative);
            NVSDK_NGX_Resource_VK mvVk = NgxGetResource(m_iCore, guides.mv, mvNative);
            NVSDK_NGX_Resource_VK depthVk = NgxGetResource(m_iCore, guides.depth, depthNative);
            NVSDK_NGX_Resource_VK normalRoughnessVk = NgxGetResource(m_iCore, guides.normalRoughness, normalRoughnessNative);
            NVSDK_NGX_Resource_VK diffuseAlbedoVk = NgxGetResource(m_iCore, guides.diffuseAlbedo, diffuseAlbedoNative);
            NVSDK_NGX_Resource_VK specularAlbedoVk = NgxGetResource(m_iCore, guides.specularAlbedo, specularAlbedoNative);
            NVSDK_NGX_Resource_VK specularMvOrHitTVk = NgxGetResource(m_iCore, guides.specularMvOrHitT, specularMvOrHitTNative);
            NVSDK_NGX_Resource_VK exposureVk = NgxGetResource(m_iCore, guides.exposure, exposureNative);
            NVSDK_NGX_Resource_VK reactiveVk = NgxGetResource(m_iCore, guides.reactive, reactiveNative);
            NVSDK_NGX_Resource_VK sssVk = NgxGetResource(m_iCore, guides.sss, sssNative);
            NVSDK_NGX_Resource_VK preTransparencyVk = NgxGetResource(m_iCore, guides.preTransparency, preTransparencyNative);

            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_Color, &inputVk);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_Output, &outputVk);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_MotionVectors, &mvVk);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_Depth, &depthVk);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_DiffuseAlbedo, &diffuseAlbedoVk);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_SpecularAlbedo, &specularAlbedoVk);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_ExposureTexture, guides.exposure.texture ? &exposureVk : nullptr);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_GBuffer_Normals, &normalRoughnessVk);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_GBuffer_SpecularMvec, useSpecularMotion ? &specularMvOrHitTVk : nullptr);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_DLSSD_ResponsivityMask, guides.reactive.texture ? &reactiveVk : nullptr);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_DLSSD_SpecularHitDistance, useSpecularMotion ? nullptr : &specularMvOrHitTVk);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_DLSSD_ScreenSpaceSubsurfaceScatteringGuide, guides.sss.texture ? &sssVk : nullptr);
            NVSDK_NGX_Parameter_SetVoidPointer(m.ngx->params, NVSDK_NGX_Parameter_DLSSD_ColorBeforeTransparency, guides.preTransparency.texture ? &preTransparencyVk : nullptr);

            result = NVSDK_NGX_VULKAN_EvaluateFeature_C((VkCommandBuffer)commandBufferNative, m.ngx->handle, m.ngx->params, nullptr);
        }
#    endif

        NRI_CHECK(result == NVSDK_NGX_Result_Success, "DLRR evaluation failed!");
    }
#endif

#if NRI_ENABLE_METAL_SUPPORT
    if (IsMetalFx(m_Desc.type)) {
        NS::AutoreleasePool* autoreleasePool = NS::AutoreleasePool::alloc()->init();
        MetalFx& metalFx = *m.metalfx;

        MTL::Texture* inputNative = MetalFxGetTexture(m_iCore, input);
        MTL::Texture* outputNative = MetalFxGetTexture(m_iCore, output);
        NRI_CHECK(inputNative && outputNative, "'input' and 'output' must be valid textures");

        // Lazy creation: needs formats (can compile shaders)
        bool isCreated = metalFx.spatial || metalFx.temporal || metalFx.denoised;

        if (!isCreated && inputNative && outputNative) {
            MTL::Device* deviceNative = (MTL::Device*)m_iCore.GetDeviceNativeObject(&m_Device);
            isCreated = MetalFxCreateScaler(metalFx, deviceNative, m_iCore, m_Desc, dispatchUpscaleDesc);

            if (!isCreated)
                NRI_REPORT_ERROR(&(DeviceBase&)m_Device, "MetalFX scaler creation failed");
        }

        // The scaler is created for the "input" size of the first dispatch
        bool isInputValid = false;

        if (isCreated && inputNative && outputNative) {
            NS::UInteger inputWidth = metalFx.spatial ? metalFx.spatial->inputWidth() : (metalFx.temporal ? metalFx.temporal->inputWidth() : metalFx.denoised->inputWidth());
            NS::UInteger inputHeight = metalFx.spatial ? metalFx.spatial->inputHeight() : (metalFx.temporal ? metalFx.temporal->inputHeight() : metalFx.denoised->inputHeight());

            isInputValid = inputNative->width() == inputWidth && inputNative->height() == inputHeight;
            NRI_CHECK(isInputValid, "'input' size must not change");
        }

        if (isInputValid) {
            // MetalFX encodes directly into the native command buffer, which allows only one open encoder
            MTL4::CommandBuffer* commandBufferNative = (MTL4::CommandBuffer*)m_iCore.GetCommandBufferNativeObject(&commandBuffer);
            BeginNativeEncodingMetal(commandBufferNative);

            { // Order "METALFX_EXTRA_STAGES" after all previous work ("dispatch" is ordered by app barriers)
                MTL4::ComputeCommandEncoder* encoder = commandBufferNative->computeCommandEncoder();
                encoder->barrierAfterStages(MTL::StageAll, METALFX_EXTRA_STAGES, MTL4::VisibilityOptionDevice);
                encoder->endEncoding();
            }

            if (metalFx.spatial) {
                metalFx.spatial->setColorTexture(inputNative);
                metalFx.spatial->setOutputTexture(outputNative);
                metalFx.spatial->setInputContentWidth(dispatchUpscaleDesc.currentResolution.w);
                metalFx.spatial->setInputContentHeight(dispatchUpscaleDesc.currentResolution.h);
                metalFx.spatial->encodeToCommandBuffer(commandBufferNative);
            } else if (metalFx.temporal) {
                const UpscalerGuides& guides = dispatchUpscaleDesc.guides.upscaler;

                metalFx.temporal->setColorTexture(inputNative);
                metalFx.temporal->setOutputTexture(outputNative);
                metalFx.temporal->setDepthTexture(MetalFxGetTexture(m_iCore, guides.depth));
                metalFx.temporal->setMotionTexture(MetalFxGetTexture(m_iCore, guides.mv));
                metalFx.temporal->setExposureTexture(MetalFxGetTexture(m_iCore, guides.exposure));
                metalFx.temporal->setReactiveMaskTexture(MetalFxGetTexture(m_iCore, guides.reactive));
                metalFx.temporal->setInputContentWidth(dispatchUpscaleDesc.currentResolution.w);
                metalFx.temporal->setInputContentHeight(dispatchUpscaleDesc.currentResolution.h);
                metalFx.temporal->setJitterOffsetX(dispatchUpscaleDesc.cameraJitter.x);
                metalFx.temporal->setJitterOffsetY(dispatchUpscaleDesc.cameraJitter.y);
                metalFx.temporal->setMotionVectorScaleX(dispatchUpscaleDesc.mvScale.x);
                metalFx.temporal->setMotionVectorScaleY(dispatchUpscaleDesc.mvScale.y);
                metalFx.temporal->setDepthReversed((m_Desc.flags & UpscalerBits::DEPTH_INVERTED) != 0);
                metalFx.temporal->setReset((dispatchUpscaleDesc.flags & DispatchUpscaleBits::RESET_HISTORY) != 0);
                metalFx.temporal->setPreExposure(preExposure);
                metalFx.temporal->encodeToCommandBuffer(commandBufferNative);
            } else {
                const DenoiserGuides& guides = dispatchUpscaleDesc.guides.denoiser;
                MTL::Texture* normalRoughnessNative = MetalFxGetTexture(m_iCore, guides.normalRoughness);

                simd::float4x4 worldToViewMatrix = {};
                memcpy(&worldToViewMatrix, dispatchUpscaleDesc.settings.dlrr.worldToViewMatrix, sizeof(worldToViewMatrix));

                simd::float4x4 viewToClipMatrix = {};
                memcpy(&viewToClipMatrix, dispatchUpscaleDesc.settings.dlrr.viewToClipMatrix, sizeof(viewToClipMatrix));

                metalFx.denoised->setColorTexture(inputNative);
                metalFx.denoised->setOutputTexture(outputNative);
                metalFx.denoised->setDepthTexture(MetalFxGetTexture(m_iCore, guides.depth));
                metalFx.denoised->setMotionTexture(MetalFxGetTexture(m_iCore, guides.mv));
                metalFx.denoised->setNormalTexture(normalRoughnessNative);
                metalFx.denoised->setRoughnessTexture(normalRoughnessNative ? MetalFxCreateRoughnessView(commandBufferNative, normalRoughnessNative) : nullptr);
                metalFx.denoised->setDiffuseAlbedoTexture(MetalFxGetTexture(m_iCore, guides.diffuseAlbedo));
                metalFx.denoised->setSpecularAlbedoTexture(MetalFxGetTexture(m_iCore, guides.specularAlbedo));
                metalFx.denoised->setSpecularHitDistanceTexture(MetalFxGetTexture(m_iCore, guides.specularMvOrHitT));
                metalFx.denoised->setExposureTexture(MetalFxGetTexture(m_iCore, guides.exposure));
                metalFx.denoised->setReactiveMaskTexture(MetalFxGetTexture(m_iCore, guides.reactive));
                metalFx.denoised->setJitterOffsetX(dispatchUpscaleDesc.cameraJitter.x);
                metalFx.denoised->setJitterOffsetY(dispatchUpscaleDesc.cameraJitter.y);
                metalFx.denoised->setMotionVectorScaleX(dispatchUpscaleDesc.mvScale.x);
                metalFx.denoised->setMotionVectorScaleY(dispatchUpscaleDesc.mvScale.y);
                metalFx.denoised->setDepthReversed((m_Desc.flags & UpscalerBits::DEPTH_INVERTED) != 0);
                metalFx.denoised->setShouldResetHistory((dispatchUpscaleDesc.flags & DispatchUpscaleBits::RESET_HISTORY) != 0);
                metalFx.denoised->setPreExposure(preExposure);
                metalFx.denoised->setWorldToViewMatrix(worldToViewMatrix);
                metalFx.denoised->setViewToClipMatrix(viewToClipMatrix);
                metalFx.denoised->encodeToCommandBuffer(commandBufferNative);
            }

            { // Order all subsequent work after "METALFX_EXTRA_STAGES" ("dispatch" is ordered by app barriers)
                MTL4::ComputeCommandEncoder* encoder = commandBufferNative->computeCommandEncoder();
                encoder->barrierAfterStages(METALFX_EXTRA_STAGES, MTL::StageAll, MTL4::VisibilityOptionDevice);
                encoder->endEncoding();
            }
        }

        autoreleasePool->release();
    }
#endif
}
