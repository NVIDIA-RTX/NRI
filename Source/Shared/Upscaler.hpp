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
#        include "NIS.dxbc.h"
#    endif

#    if NRI_ENABLE_D3D12_SUPPORT
#        include "NIS.dxil.h"
#        include "NIS_fp16.dxil.h"
#    endif

#    if NRI_ENABLE_VK_SUPPORT
#        include "NIS.spirv.h"
#        include "NIS_fp16.spirv.h"
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
    std::array<DescriptorSet*, NIS_DESCRIPTOR_SET_NUM> descriptorSets = {};
    Dim2 blockSize = {};
    uint32_t descriptorSetIndex = 0;
};

#endif

//=====================================================================================================================================
// FFX
//=====================================================================================================================================
#if NRI_ENABLE_FFX_SDK
#    include "ffx_upscale.h"

#    if NRI_ENABLE_D3D12_SUPPORT
#        define FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12 0x0000002u

struct ffxCreateBackendDX12Desc { // TODO: copied from "dx12" header (can't be used with "vk" in one file)
    ffxCreateContextDescHeader header;
    ID3D12Device* device;
};

#    endif

#    if NRI_ENABLE_VK_SUPPORT
#        define FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_VK 0x0000003u

struct ffxCreateBackendVKDesc { // TODO: copied from "vk" header (can't be used with "dx12" in one file)
    ffxCreateContextDescHeader header;
    VkDevice vkDevice;
    VkPhysicalDevice vkPhysicalDevice;
    PFN_vkGetDeviceProcAddr vkDeviceProcAddr;
};

// Unfortunately, FFX devs don't understand how VK works. Some VK functions are retrieved with non-CORE names,
// despite being in CORE for years. Manual patching needed, which is not as easy in case of multiple devices.
struct FfxVkPair {
    VkDevice device;
    PFN_vkGetDeviceProcAddr getDeviceProcAddress;
};

struct FfxGlobals {
    ;
    std::array<FfxVkPair, 32> vkPairs = {};
    Lock lock = {};
} g_ffx;

static inline void FfxRegisterDevice(VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddress) {
    ExclusiveScope lock(g_ffx.lock);

    size_t i = 0;
    for (; i < g_ffx.vkPairs.size(); i++) {
        if (g_ffx.vkPairs[i].device == device) {
            // Already registered
            CHECK(g_ffx.vkPairs[i].getDeviceProcAddress == getDeviceProcAddress, "Unexpected");
            return;
        }

        // Empty slot is found
        if (!g_ffx.vkPairs[i].device)
            break;
    }

    CHECK(i < g_ffx.vkPairs.size(), "Too many devices?");

    // Add new entry
    g_ffx.vkPairs[i] = {device, getDeviceProcAddress};
}

static PFN_vkVoidFunction VKAPI_PTR FfxVkGetDeviceProcAddr(VkDevice device, const char* pName) {
    // TODO: patch FFX requests here
    if (!strcmp(pName, "vkGetBufferMemoryRequirements2KHR"))
        pName = "vkGetBufferMemoryRequirements2";

    // Find entry
    size_t i = 0;
    for (; i < g_ffx.vkPairs.size(); i++) {
        if (g_ffx.vkPairs[i].device == device)
            break;
    }

    CHECK(i < g_ffx.vkPairs.size(), "Unexpected");

    // Use corresponding "vkGetDeviceProcAddr"
    PFN_vkVoidFunction func = g_ffx.vkPairs[i].getDeviceProcAddress(device, pName);
    CHECK(func || strstr(pName, "AMD"), "Another non-CORE function name?");

    return func;
}

#    endif

struct Ffx {
    PfnFfxCreateContext CreateContext = nullptr;
    PfnFfxDestroyContext DestroyContext = nullptr;
    PfnFfxDispatch Dispatch = nullptr;
    Library* library = nullptr;
    ffxContext context = nullptr;
    ffxAllocationCallbacks allocationCallbacks = {};
    ffxAllocationCallbacks* allocationCallbacksPtr = nullptr;
};

static Result FfxConvertError(ffxReturnCode_t code) {
    switch (code) {
        case FFX_API_RETURN_OK:
            return Result::SUCCESS;
        case FFX_API_RETURN_ERROR:
            return Result::FAILURE;
        case FFX_API_RETURN_ERROR_UNKNOWN_DESCTYPE:
            return Result::INVALID_ARGUMENT;
        case FFX_API_RETURN_ERROR_RUNTIME_ERROR:
            return Result::FAILURE;
        case FFX_API_RETURN_NO_PROVIDER:
            return Result::FAILURE;
        case FFX_API_RETURN_ERROR_MEMORY:
            return Result::OUT_OF_MEMORY;
        case FFX_API_RETURN_ERROR_PARAMETER:
            return Result::INVALID_ARGUMENT;
    }

    return Result::FAILURE;
}

static void* FfxAlloc(void* pUserData, uint64_t size) {
    const auto& allocationCallbacks = *(AllocationCallbacks*)pUserData;
    return allocationCallbacks.Allocate(allocationCallbacks.userArg, size, sizeof(size_t));
}

static void FfxDealloc(void* pUserData, void* pMem) {
    const auto& allocationCallbacks = *(AllocationCallbacks*)pUserData;
    allocationCallbacks.Free(allocationCallbacks.userArg, pMem);
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
    res.resource = (void*)NRI.GetTextureNativeObject(*resource.texture);
    res.state = isStorage ? FFX_API_RESOURCE_STATE_UNORDERED_ACCESS : FFX_API_RESOURCE_STATE_COMPUTE_READ;
    res.description.flags = FFX_API_RESOURCE_FLAGS_NONE;

    if (res.resource) {
        const TextureDesc& textureDesc = NRI.GetTextureDesc(*resource.texture);
        const FormatProps& formatProps = nriGetFormatProps(textureDesc.format);

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
        }
    }

    return res;
}

#    ifndef NDEBUG

static void FfxDebugMessage(uint32_t, const wchar_t* message) {
    char s[1024];
    ConvertWcharToChar(message, s, sizeof(s));

    printf("FFX: %s\n", s);
}

#    endif

#endif

//=====================================================================================================================================
// NGX
//=====================================================================================================================================
#if NRI_ENABLE_NGX_SDK
#    include "nvsdk_ngx_helpers.h"
#    if NRI_ENABLE_VK_SUPPORT
#        include "nvsdk_ngx_helpers_vk.h"
#    endif
#    include "nvsdk_ngx_helpers_dlssd.h"
#    if NRI_ENABLE_VK_SUPPORT
#        include "nvsdk_ngx_helpers_dlssd_vk.h"
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
    Lock lock = {}; // methods in NGX library are NOT thread safe, yay! (see the first comment in "nvsdk_ngx.h")
} g_ngx;

static inline int32_t NgxIncrRef(void* deviceNative) {
    uint32_t i = 0;
    for (; i < g_ngx.refCounterNum && g_ngx.refCounters[i].deviceNative != deviceNative; i++)
        ;

    if (i == g_ngx.refCounterNum) {
        g_ngx.refCounterNum++;
        CHECK(g_ngx.refCounterNum < g_ngx.refCounters.size(), "Too many devices?");
    }

    g_ngx.refCounters[i].deviceNative = deviceNative;
    g_ngx.refCounters[i].refCounter++;

    return g_ngx.refCounters[i].refCounter;
}

static inline int32_t NgxDecrRef(void* deviceNative) {
    uint32_t i = 0;
    for (; i < g_ngx.refCounterNum && g_ngx.refCounters[i].deviceNative != deviceNative; i++)
        ;

    CHECK(i < g_ngx.refCounterNum, "Destroy before create?");
    CHECK(g_ngx.refCounters[i].refCounter > 0, "Unexpected");

    g_ngx.refCounters[i].deviceNative = deviceNative;
    g_ngx.refCounters[i].refCounter--;

    return g_ngx.refCounters[i].refCounter;
}

static void NVSDK_CONV NgxLogCallback(const char*, NVSDK_NGX_Logging_Level, NVSDK_NGX_Feature) {
}

#    if NRI_ENABLE_VK_SUPPORT

static inline NVSDK_NGX_Resource_VK NgxGetResource(const CoreInterface& NRI, const UpscalerResource& resource, uint64_t resourceNative, bool isStorage = false) {
    if (!resource.texture)
        return {};

    const TextureDesc& textureDesc = NRI.GetTextureDesc(*resource.texture);
    VkImageView view = (VkImageView)NRI.GetDescriptorNativeObject(*resource.descriptor);
    VkImageSubresourceRange subresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkFormat format = (VkFormat)nriConvertNRIFormatToVK(textureDesc.format);

    return NVSDK_NGX_Create_ImageView_Resource_VK(view, (VkImage)resourceNative, subresource, format, textureDesc.width, textureDesc.height, isStorage);
}

#    endif
#endif

//=====================================================================================================================================
// Upscaler
//=====================================================================================================================================
bool nri::IsUpscalerSupported(const DeviceDesc& deviceDesc, UpscalerType type) {
    MaybeUnused(deviceDesc, type);

#if NRI_ENABLE_NIS_SDK
    if (type == UpscalerType::NIS) {
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D11 || deviceDesc.graphicsAPI == GraphicsAPI::D3D12 || deviceDesc.graphicsAPI != GraphicsAPI::VK)
            return true;
    }
#endif

#if NRI_ENABLE_FFX_SDK
    if (type == UpscalerType::FSR) {
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12 || deviceDesc.graphicsAPI != GraphicsAPI::VK)
            return true;
    }
#endif

#if NRI_ENABLE_NGX_SDK
    if (type == UpscalerType::DLSR || type == UpscalerType::DLRR) {
        if (deviceDesc.adapterDesc.vendor == Vendor::NVIDIA && strstr(deviceDesc.adapterDesc.name, " RTX ")) // TODO: true?
            return true;
    }
#endif

    return false;
}

UpscalerImpl::~UpscalerImpl() {
#if NRI_ENABLE_NIS_SDK
    if (m_Desc.type == UpscalerType::NIS && m.nis) {
        m_NRI.DestroyDescriptor(*m.nis->srvScale);
        m_NRI.DestroyDescriptor(*m.nis->srvUsm);
        m_NRI.DestroyDescriptor(*m.nis->sampler);
        m_NRI.DestroyTexture(*m.nis->texScale);
        m_NRI.DestroyTexture(*m.nis->texUsm);
        m_NRI.DestroyPipeline(*m.nis->pipeline);
        m_NRI.DestroyPipelineLayout(*m.nis->pipelineLayout);
        m_NRI.DestroyDescriptorPool(*m.nis->descriptorPool);

        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        Destroy<Nis>(allocationCallbacks, m.nis);
    }
#endif

#if NRI_ENABLE_FFX_SDK
    if (m_Desc.type == UpscalerType::FSR && m.ffx) {
        ffxReturnCode_t result = m.ffx->DestroyContext(&m.ffx->context, m.ffx->allocationCallbacksPtr);
        CHECK(result == FFX_API_RETURN_OK, "ffxDestroyContext() failed!");

        UnloadSharedLibrary(*m.ffx->library);

        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        Destroy<Ffx>(allocationCallbacks, m.ffx);
    }
#endif

#if NRI_ENABLE_NGX_SDK
    if ((m_Desc.type == UpscalerType::DLSR || m_Desc.type == UpscalerType::DLRR) && m.ngx) {
        ExclusiveScope lock(g_ngx.lock);

        const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);
        void* deviceNative = m_NRI.GetDeviceNativeObject(m_Device);
        int32_t refCount = NgxDecrRef(deviceNative);

#    if NRI_ENABLE_D3D11_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D11) {
            NVSDK_NGX_Result result = NVSDK_NGX_D3D11_DestroyParameters(m.ngx->params);
            CHECK(!m.ngx->params || result == NVSDK_NGX_Result_Success, "NVSDK_NGX_D3D11_DestroyParameters() failed!");

            result = NVSDK_NGX_D3D11_ReleaseFeature(m.ngx->handle);
            CHECK(!m.ngx->handle || result == NVSDK_NGX_Result_Success, "NVSDK_NGX_D3D11_ReleaseFeature() failed!");

            if (refCount == 0) {
                result = NVSDK_NGX_D3D11_Shutdown1((ID3D11Device*)deviceNative);
                CHECK(result == NVSDK_NGX_Result_Success, "NVSDK_NGX_D3D11_Shutdown1() failed!");
            }
        }
#    endif

#    if NRI_ENABLE_D3D12_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12) {
            NVSDK_NGX_Result result = NVSDK_NGX_D3D12_DestroyParameters(m.ngx->params);
            CHECK(!m.ngx->params || result == NVSDK_NGX_Result_Success, "NVSDK_NGX_D3D12_DestroyParameters() failed!");

            result = NVSDK_NGX_D3D12_ReleaseFeature(m.ngx->handle);
            CHECK(!m.ngx->handle || result == NVSDK_NGX_Result_Success, "NVSDK_NGX_D3D12_ReleaseFeature() failed!");

            if (refCount == 0) {
                result = NVSDK_NGX_D3D12_Shutdown1((ID3D12Device*)deviceNative);
                CHECK(result == NVSDK_NGX_Result_Success, "NVSDK_NGX_D3D12_Shutdown1() failed!");
            }
        }
#    endif

#    if NRI_ENABLE_VK_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::VK) {
            NVSDK_NGX_Result result = NVSDK_NGX_VULKAN_DestroyParameters(m.ngx->params);
            CHECK(!m.ngx->params || result == NVSDK_NGX_Result_Success, "NVSDK_NGX_VULKAN_DestroyParameters() failed!");

            result = NVSDK_NGX_VULKAN_ReleaseFeature(m.ngx->handle);
            CHECK(!m.ngx->handle || result == NVSDK_NGX_Result_Success, "NVSDK_NGX_VULKAN_ReleaseFeature() failed!");

            if (refCount == 0) {
                result = NVSDK_NGX_VULKAN_Shutdown1((VkDevice)deviceNative);
                CHECK(result == NVSDK_NGX_Result_Success, "NVSDK_NGX_VULKAN_Shutdown1() failed!");
            }
        }
#    endif

        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        Destroy<Ngx>(allocationCallbacks, m.ngx);
    }
#endif
}

Result UpscalerImpl::Create(const UpscalerDesc& upscalerDesc) {
    m_Desc = upscalerDesc;

    UpscalerProps upscalerProps = {};
    GetUpscalerProps(upscalerProps);

#if NRI_ENABLE_NIS_SDK
    if (upscalerDesc.type == UpscalerType::NIS) {
        const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);
        if (!(deviceDesc.graphicsAPI == GraphicsAPI::D3D11 || deviceDesc.graphicsAPI == GraphicsAPI::D3D12 || deviceDesc.graphicsAPI == GraphicsAPI::VK))
            return Result::UNSUPPORTED;

        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        m.nis = Allocate<Nis>(allocationCallbacks);

        { // Pipeline layout
            const DescriptorRangeDesc descriptorRanges[] = {
                {1, 1, DescriptorType::SAMPLER, StageBits::COMPUTE_SHADER},
                {2, 3, DescriptorType::TEXTURE, StageBits::COMPUTE_SHADER},
                {5, 1, DescriptorType::STORAGE_TEXTURE, StageBits::COMPUTE_SHADER},
            };

            DescriptorSetDesc descriptorSetDesc = {};
            descriptorSetDesc.registerSpace = 0;
            descriptorSetDesc.ranges = descriptorRanges;
            descriptorSetDesc.rangeNum = GetCountOf(descriptorRanges);

            RootConstantDesc rootConstants = {};
            rootConstants.registerIndex = 0;
            rootConstants.shaderStages = StageBits::COMPUTE_SHADER;
            rootConstants.size = sizeof(NIS::Constants);

            PipelineLayoutDesc pipelineLayoutDesc = {};
            pipelineLayoutDesc.rootConstants = &rootConstants;
            pipelineLayoutDesc.rootConstantNum = 1;
            pipelineLayoutDesc.descriptorSets = &descriptorSetDesc;
            pipelineLayoutDesc.descriptorSetNum = 1;
            pipelineLayoutDesc.shaderStages = StageBits::COMPUTE_SHADER;
            pipelineLayoutDesc.ignoreGlobalSPIRVOffsets = true;

            Result result = m_NRI.CreatePipelineLayout(m_Device, pipelineLayoutDesc, m.nis->pipelineLayout);
            if (result != Result::SUCCESS)
                return result;
        }

        { // Pipeline
            m.nis->blockSize.w = 32;
            m.nis->blockSize.h = deviceDesc.shaderModel >= 62 ? 32 : 24;

            uint32_t permutation = 0;
            if (deviceDesc.adapterDesc.vendor != Vendor::NVIDIA) // TODO: verify performance
                permutation += 2;
            if (upscalerDesc.flags & UpscalerBits::HDR)
                permutation += 1;

            std::array<ShaderMake::ShaderConstant, 3> defines = {{
                {"NIS_FP16", (deviceDesc.shaderModel >= 62) ? "1" : "0"},
                {"NIS_HDR_MODE", (upscalerDesc.flags & UpscalerBits::HDR) ? "1" : "0"},
                {"NIS_THREAD_GROUP_SIZE", (deviceDesc.adapterDesc.vendor == Vendor::NVIDIA) ? "128" : "256"},
            }};

            const void* bytecode = nullptr;
            size_t size = 0;
            bool shaderMakeResult = false;
#    if NRI_ENABLE_D3D11_SUPPORT
            if (deviceDesc.graphicsAPI == GraphicsAPI::D3D11)
                shaderMakeResult = ShaderMake::FindPermutationInBlob(g_NIS_dxbc, GetCountOf(g_NIS_dxbc), defines.data(), (uint32_t)defines.size(), &bytecode, &size);
#    endif
#    if NRI_ENABLE_D3D12_SUPPORT
            if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12) {
                if (deviceDesc.shaderModel >= 62)
                    shaderMakeResult = ShaderMake::FindPermutationInBlob(g_NIS_fp16_dxil, GetCountOf(g_NIS_fp16_dxil), defines.data(), (uint32_t)defines.size(), &bytecode, &size);
                else
                    shaderMakeResult = ShaderMake::FindPermutationInBlob(g_NIS_dxil, GetCountOf(g_NIS_dxil), defines.data(), (uint32_t)defines.size(), &bytecode, &size);
            }
#    endif
#    if NRI_ENABLE_VK_SUPPORT
            if (deviceDesc.graphicsAPI == GraphicsAPI::VK) {
                if (deviceDesc.shaderModel >= 62)
                    shaderMakeResult = ShaderMake::FindPermutationInBlob(g_NIS_fp16_spirv, GetCountOf(g_NIS_fp16_spirv), defines.data(), (uint32_t)defines.size(), &bytecode, &size);
                else
                    shaderMakeResult = ShaderMake::FindPermutationInBlob(g_NIS_spirv, GetCountOf(g_NIS_spirv), defines.data(), (uint32_t)defines.size(), &bytecode, &size);
            }
#    endif
            if (!shaderMakeResult)
                return Result::FAILURE;

            ComputePipelineDesc computePipelineDesc = {};
            computePipelineDesc.pipelineLayout = m.nis->pipelineLayout;
            computePipelineDesc.shader.stage = StageBits::COMPUTE_SHADER;
            computePipelineDesc.shader.bytecode = bytecode;
            computePipelineDesc.shader.size = size;

            Result result = m_NRI.CreateComputePipeline(m_Device, computePipelineDesc, m.nis->pipeline);
            if (result != Result::SUCCESS)
                return result;
        }

        { // Textures
            ResourceAllocatorInterface iResourceAllocator = {};
            Result result = nriGetInterface(m_Device, NRI_INTERFACE(ResourceAllocatorInterface), &iResourceAllocator);
            if (result != Result::SUCCESS)
                return result;

            AllocateTextureDesc allocateTextureDesc = {};
            allocateTextureDesc.memoryLocation = MemoryLocation::DEVICE;
            allocateTextureDesc.dedicated = true;
            allocateTextureDesc.desc.type = TextureType::TEXTURE_2D;
            allocateTextureDesc.desc.usage = TextureUsageBits::SHADER_RESOURCE;
            allocateTextureDesc.desc.format = Format::RGBA16_SFLOAT;
            allocateTextureDesc.desc.width = NIS::kFilterSize / 4;
            allocateTextureDesc.desc.height = NIS::kPhaseCount;
            allocateTextureDesc.desc.mipNum = 1;
            allocateTextureDesc.desc.layerNum = 1;

            result = iResourceAllocator.AllocateTexture(m_Device, allocateTextureDesc, m.nis->texScale);
            if (result != Result::SUCCESS)
                return result;

            result = iResourceAllocator.AllocateTexture(m_Device, allocateTextureDesc, m.nis->texUsm);
            if (result != Result::SUCCESS)
                return result;
        }

        { // Upload data
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
            result = m_NRI.GetQueue(m_Device, QueueType::GRAPHICS, 0, graphicsQueue);
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

            Result result = m_NRI.CreateSampler(m_Device, samplerDesc, m.nis->sampler);
            if (result != Result::SUCCESS)
                return result;

            Texture2DViewDesc textureViewDesc = {};
            textureViewDesc.viewType = Texture2DViewType::SHADER_RESOURCE_2D;
            textureViewDesc.format = Format::RGBA16_SFLOAT;
            textureViewDesc.mipNum = 1;
            textureViewDesc.layerNum = 1;

            textureViewDesc.texture = m.nis->texScale;
            result = m_NRI.CreateTexture2DView(textureViewDesc, m.nis->srvScale);
            if (result != Result::SUCCESS)
                return result;

            textureViewDesc.texture = m.nis->texUsm;
            result = m_NRI.CreateTexture2DView(textureViewDesc, m.nis->srvUsm);
            if (result != Result::SUCCESS)
                return result;
        }

        { // Descriptor pool & set
            DescriptorPoolDesc descriptorPoolDesc = {};
            descriptorPoolDesc.descriptorSetMaxNum = NIS_DESCRIPTOR_SET_NUM;
            descriptorPoolDesc.samplerMaxNum = 1 * NIS_DESCRIPTOR_SET_NUM;
            descriptorPoolDesc.textureMaxNum = 3 * NIS_DESCRIPTOR_SET_NUM;
            descriptorPoolDesc.storageTextureMaxNum = 1 * NIS_DESCRIPTOR_SET_NUM;

            Result result = m_NRI.CreateDescriptorPool(m_Device, descriptorPoolDesc, m.nis->descriptorPool);
            if (result != Result::SUCCESS)
                return result;

            result = m_NRI.AllocateDescriptorSets(*m.nis->descriptorPool, *m.nis->pipelineLayout, 0, m.nis->descriptorSets.data(), NIS_DESCRIPTOR_SET_NUM, 0);
            if (result != Result::SUCCESS)
                return result;
        }
    }
#endif

#if NRI_ENABLE_FFX_SDK
    if (upscalerDesc.type == UpscalerType::FSR) {
        const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);
        if (!(deviceDesc.graphicsAPI == GraphicsAPI::D3D12 || deviceDesc.graphicsAPI == GraphicsAPI::VK))
            return Result::UNSUPPORTED;

        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        m.ffx = Allocate<Ffx>(allocationCallbacks);

        // Load library
        const char* libraryName = deviceDesc.graphicsAPI == GraphicsAPI::D3D12 ? "amd_fidelityfx_dx12.dll" : "amd_fidelityfx_vk.dll";
        Library* ffxLibrary = LoadSharedLibrary(libraryName);
        if (!ffxLibrary)
            return Result::FAILURE;

        // Get functions
        m.ffx->library = ffxLibrary;
        m.ffx->CreateContext = (PfnFfxCreateContext)GetSharedLibraryFunction(*ffxLibrary, "ffxCreateContext");
        m.ffx->DestroyContext = (PfnFfxDestroyContext)GetSharedLibraryFunction(*ffxLibrary, "ffxDestroyContext");
        m.ffx->Dispatch = (PfnFfxDispatch)GetSharedLibraryFunction(*ffxLibrary, "ffxDispatch");

        // Verify
        const void** functionArray = (const void**)&m.ffx->CreateContext;
        const size_t functionArraySize = 3;
        size_t i = 0;
        for (; i < functionArraySize && functionArray[i] != nullptr; i++)
            ;

        if (i != functionArraySize)
            return Result::FAILURE;

        // Allocation callbacks
        m.ffx->allocationCallbacks.alloc = FfxAlloc;
        m.ffx->allocationCallbacks.dealloc = FfxDealloc;
        m.ffx->allocationCallbacks.pUserData = (void*)(&((DeviceBase&)m_Device).GetAllocationCallbacks());

        if (!((DeviceBase&)m_Device).GetAllocationCallbacks().disable3rdPartyAllocationCallbacks)
            m.ffx->allocationCallbacksPtr = &m.ffx->allocationCallbacks;

        // Create context
        ffxCreateContextDescUpscale contextDesc = {};
        contextDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
        contextDesc.maxRenderSize = {upscalerProps.renderResolution.w, upscalerProps.renderResolution.h};
        contextDesc.maxUpscaleSize = {upscalerProps.upscaleResolution.w, upscalerProps.upscaleResolution.h};
        contextDesc.flags = FFX_UPSCALE_ENABLE_DYNAMIC_RESOLUTION; // TODO: move to "UpscalerBits"?

        if (upscalerDesc.flags & UpscalerBits::HDR)
            contextDesc.flags |= FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE;
        if (upscalerDesc.flags & UpscalerBits::NON_LINEAR)
            contextDesc.flags |= FFX_UPSCALE_ENABLE_NON_LINEAR_COLORSPACE;
        if (upscalerDesc.flags & UpscalerBits::AUTO_EXPOSURE)
            contextDesc.flags |= FFX_UPSCALE_ENABLE_AUTO_EXPOSURE;
        if (upscalerDesc.flags & UpscalerBits::DEPTH_INVERTED)
            contextDesc.flags |= FFX_UPSCALE_ENABLE_DEPTH_INVERTED;
        if (upscalerDesc.flags & UpscalerBits::DEPTH_INFINITE)
            contextDesc.flags |= FFX_UPSCALE_ENABLE_DEPTH_INFINITE;
        if (upscalerDesc.flags & UpscalerBits::UPSCALE_RES_MV)
            contextDesc.flags |= FFX_UPSCALE_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS;

#    ifndef NDEBUG
        contextDesc.flags |= FFX_UPSCALE_ENABLE_DEBUG_CHECKING;
        contextDesc.fpMessage = FfxDebugMessage;
#    endif

#    if NRI_ENABLE_D3D12_SUPPORT
        ffxCreateBackendDX12Desc backendD3D12Desc = {};

        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12) {
            backendD3D12Desc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
            backendD3D12Desc.device = (ID3D12Device*)m_NRI.GetDeviceNativeObject(m_Device);

            contextDesc.header.pNext = &backendD3D12Desc.header;
        }
#    endif

#    if NRI_ENABLE_VK_SUPPORT
        ffxCreateBackendVKDesc backendVKDesc = {};

        if (deviceDesc.graphicsAPI == GraphicsAPI::VK) {
            WrapperVKInterface iWrapperVK = {};
            if (nriGetInterface(m_Device, NRI_INTERFACE(WrapperVKInterface), &iWrapperVK) != Result::SUCCESS)
                return Result::UNSUPPORTED;

            VkDevice vkDevice = (VkDevice)m_NRI.GetDeviceNativeObject(m_Device);
            VkPhysicalDevice vkPhysicalDevice = (VkPhysicalDevice)iWrapperVK.GetPhysicalDeviceVK(m_Device);
            PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)iWrapperVK.GetDeviceProcAddrVK(m_Device);
            FfxRegisterDevice(vkDevice, vkGetDeviceProcAddr);

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

        return Result::SUCCESS;
    }
#endif

#if NRI_ENABLE_NGX_SDK
    if (upscalerDesc.type == UpscalerType::DLSR || upscalerDesc.type == UpscalerType::DLRR) {
        const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);
        void* deviceNative = m_NRI.GetDeviceNativeObject(m_Device);
        const wchar_t* path = L""; // don't care
        NVSDK_NGX_Result ngxResult = NVSDK_NGX_Result_Fail;

        const auto& allocationCallbacks = ((DeviceBase&)m_Device).GetAllocationCallbacks();
        m.ngx = Allocate<Ngx>(allocationCallbacks);

        { // Create instance
            ExclusiveScope lock(g_ngx.lock);

            NVSDK_NGX_FeatureCommonInfo featureCommonInfo = {};
            featureCommonInfo.LoggingInfo.LoggingCallback = NgxLogCallback;
            featureCommonInfo.LoggingInfo.MinimumLoggingLevel = NVSDK_NGX_LOGGING_LEVEL_OFF;
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

        if (!upscalerDesc.commandBuffer) {
            Result result = m_NRI.GetQueue(m_Device, QueueType::GRAPHICS, 0, graphicsQueue);
            if (result != Result::SUCCESS)
                return result;

            result = m_NRI.CreateCommandAllocator(*graphicsQueue, commandAllocator);
            if (result != Result::SUCCESS)
                return result;

            result = m_NRI.CreateCommandBuffer(*commandAllocator, commandBuffer);
            if (result != Result::SUCCESS)
                return result;

            result = m_NRI.CreateFence(m_Device, 0, fence);
            if (result != Result::SUCCESS)
                return result;

            m_NRI.BeginCommandBuffer(*commandBuffer, nullptr);
        }

        { // Record creation commands
            ExclusiveScope lock(g_ngx.lock);

            void* commandBufferNative = m_NRI.GetCommandBufferNativeObject(*commandBuffer);

            NVSDK_NGX_PerfQuality_Value qualityValue = NVSDK_NGX_PerfQuality_Value_UltraPerformance;
            if (upscalerDesc.mode == UpscalerMode::NATIVE) {
                qualityValue = NVSDK_NGX_PerfQuality_Value_DLAA;
                NVSDK_NGX_Parameter_SetUI(m.ngx->params, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA, upscalerDesc.preset);
            } else if (upscalerDesc.mode == UpscalerMode::QUALITY) {
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
            if (upscalerDesc.flags & UpscalerBits::AUTO_EXPOSURE)
                featureCreateFlags |= NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;
            if (upscalerDesc.flags & UpscalerBits::DEPTH_INVERTED)
                featureCreateFlags |= NVSDK_NGX_DLSS_Feature_Flags_DepthInverted;
            if (!(upscalerDesc.flags & UpscalerBits::UPSCALE_RES_MV))
                featureCreateFlags |= NVSDK_NGX_DLSS_Feature_Flags_MVLowRes;

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
                    ngxResult = NGX_D3D12_CREATE_DLSS_EXT((ID3D12GraphicsCommandList*)commandBufferNative, NRI_NODE_MASK, NRI_NODE_MASK, &m.ngx->handle, m.ngx->params, &srCreateParams);
#    endif

#    if NRI_ENABLE_VK_SUPPORT
                if (deviceDesc.graphicsAPI == GraphicsAPI::VK)
                    ngxResult = NGX_VULKAN_CREATE_DLSS_EXT1((VkDevice)deviceNative, (VkCommandBuffer)commandBufferNative, NRI_NODE_MASK, NRI_NODE_MASK, &m.ngx->handle, m.ngx->params, &srCreateParams);
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
                    ngxResult = NGX_D3D12_CREATE_DLSSD_EXT((ID3D12GraphicsCommandList*)commandBufferNative, NRI_NODE_MASK, NRI_NODE_MASK, &m.ngx->handle, m.ngx->params, &rrCreateParams);
#    endif

#    if NRI_ENABLE_VK_SUPPORT
                if (deviceDesc.graphicsAPI == GraphicsAPI::VK)
                    ngxResult = NGX_VULKAN_CREATE_DLSSD_EXT1((VkDevice)deviceNative, (VkCommandBuffer)commandBufferNative, NRI_NODE_MASK, NRI_NODE_MASK, &m.ngx->handle, m.ngx->params, &rrCreateParams);
#    endif
            }
        }

        if (!upscalerDesc.commandBuffer) {
            m_NRI.EndCommandBuffer(*commandBuffer);

            // Submit & wait for completion
            FenceSubmitDesc signalFence = {};
            signalFence.fence = fence;
            signalFence.value = 1;

            QueueSubmitDesc queueSubmitDesc = {};
            queueSubmitDesc.commandBuffers = &commandBuffer;
            queueSubmitDesc.commandBufferNum = 1;
            queueSubmitDesc.signalFences = &signalFence;
            queueSubmitDesc.signalFenceNum = 1;

            m_NRI.QueueSubmit(*graphicsQueue, queueSubmitDesc);
            m_NRI.Wait(*fence, 1);

            // Cleanup
            m_NRI.DestroyFence(*fence);
            m_NRI.DestroyCommandBuffer(*commandBuffer);
            m_NRI.DestroyCommandAllocator(*commandAllocator);
        }

        if (ngxResult != NVSDK_NGX_Result_Success)
            return Result::FAILURE;
    }
#endif

    return Result::SUCCESS;
}

void UpscalerImpl::GetUpscalerProps(UpscalerProps& upscalerProps) const {
    float scalingFactor = 1.0f;
    if (m_Desc.mode == UpscalerMode::QUALITY)
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
    upscalerProps.renderResolutionMin.w = m_Desc.upscaleResolution.w / (m_Desc.mode == UpscalerMode::ULTRA_PERFORMANCE ? 3 : 2);
    upscalerProps.renderResolutionMin.h = m_Desc.upscaleResolution.h / (m_Desc.mode == UpscalerMode::ULTRA_PERFORMANCE ? 3 : 2);
    upscalerProps.renderResolution.w = (Dim_t)(m_Desc.upscaleResolution.w / scalingFactor + 0.5f);
    upscalerProps.renderResolution.h = (Dim_t)(m_Desc.upscaleResolution.h / scalingFactor + 0.5f);
    upscalerProps.jitterPhaseNum = (uint8_t)std::ceil(8.0f * scalingFactor * scalingFactor);
}

void UpscalerImpl::CmdDispatchUpscale(CommandBuffer& commandBuffer, const DispatchUpscaleDesc& dispatchUpscaleDesc) {
    const UpscalerResource& output = dispatchUpscaleDesc.output;
    const UpscalerResource& input = dispatchUpscaleDesc.input;

    MaybeUnused(commandBuffer, output, input);

#if NRI_ENABLE_NIS_SDK
    if (m_Desc.type == UpscalerType::NIS) {
        // Update ranges
        Descriptor* resources[3] = {
            input.descriptor,
            m.nis->srvScale,
            m.nis->srvUsm,
        };

        const DescriptorRangeUpdateDesc descriptorRangeUpdateDescs[] = {
            {&m.nis->sampler, 1},
            {resources, GetCountOf(resources)},
            {&output.descriptor, 1},
        };

        DescriptorSet* descriptorSet = m.nis->descriptorSets[m.nis->descriptorSetIndex];
        m_NRI.UpdateDescriptorRanges(*descriptorSet, 0, GetCountOf(descriptorRangeUpdateDescs), descriptorRangeUpdateDescs); // TODO: separate into static and dynamic descriptor sets, move "srvScale", "srvUsm" and "sampler" to static

        // Update constants
        const TextureDesc& inputDesc = m_NRI.GetTextureDesc(*input.texture);

        NIS::Constants constants = {};
        NIS::UpdateConstants(constants, dispatchUpscaleDesc.settings.nis.sharpness,
            dispatchUpscaleDesc.currentResolution.w, dispatchUpscaleDesc.currentResolution.h, // render resolution
            inputDesc.width, inputDesc.height,                                                // input dims
            m_Desc.upscaleResolution.w, m_Desc.upscaleResolution.h,                           // output resolution
            m_Desc.upscaleResolution.w, m_Desc.upscaleResolution.h,                           // output dims
            (m_Desc.flags & UpscalerBits::HDR) ? NIS::HDRMode::Linear : NIS::HDRMode::None);

        // Dispatch
        m_NRI.CmdSetDescriptorPool(commandBuffer, *m.nis->descriptorPool);
        m_NRI.CmdSetPipelineLayout(commandBuffer, *m.nis->pipelineLayout);
        m_NRI.CmdSetPipeline(commandBuffer, *m.nis->pipeline);
        m_NRI.CmdSetRootConstants(commandBuffer, 0, &constants, sizeof(constants));
        m_NRI.CmdSetDescriptorSet(commandBuffer, 0, *descriptorSet, nullptr);

        DispatchDesc dispatchDesc = {};
        dispatchDesc.x = (m_Desc.upscaleResolution.w + m.nis->blockSize.w - 1) / m.nis->blockSize.w;
        dispatchDesc.y = (m_Desc.upscaleResolution.h + m.nis->blockSize.h - 1) / m.nis->blockSize.h;
        dispatchDesc.z = 1;

        m_NRI.CmdDispatch(commandBuffer, dispatchDesc);

        // Update descriptor set for the next time
        m.nis->descriptorSetIndex++;
        m.nis->descriptorSetIndex %= NIS_DESCRIPTOR_SET_NUM;
    }
#endif

#if NRI_ENABLE_FFX_SDK
    if (m_Desc.type == UpscalerType::FSR) {
        const FSRGuides& guides = dispatchUpscaleDesc.guides.fsr;

        ffxDispatchDescUpscale dispatchDesc = {};
        dispatchDesc.header.type = FFX_API_DISPATCH_DESC_TYPE_UPSCALE;
        dispatchDesc.commandList = m_NRI.GetCommandBufferNativeObject(commandBuffer);
        dispatchDesc.output = FfxGetResource(m_NRI, output, true);
        dispatchDesc.color = FfxGetResource(m_NRI, input);
        dispatchDesc.depth = FfxGetResource(m_NRI, guides.depth);
        dispatchDesc.motionVectors = FfxGetResource(m_NRI, guides.mv);
        dispatchDesc.exposure = FfxGetResource(m_NRI, guides.exposure);
        dispatchDesc.reactive = FfxGetResource(m_NRI, guides.reactive);
        dispatchDesc.jitterOffset = {dispatchUpscaleDesc.cameraJitter.x, dispatchUpscaleDesc.cameraJitter.y};
        dispatchDesc.motionVectorScale = {dispatchUpscaleDesc.mvScale.x, dispatchUpscaleDesc.mvScale.y};
        dispatchDesc.renderSize = {dispatchUpscaleDesc.currentResolution.w, dispatchUpscaleDesc.currentResolution.h};
        dispatchDesc.enableSharpening = dispatchUpscaleDesc.settings.fsr.sharpness != 0.0f;
        dispatchDesc.sharpness = dispatchUpscaleDesc.settings.fsr.sharpness;
        dispatchDesc.frameTimeDelta = dispatchUpscaleDesc.settings.fsr.frameTime;
        dispatchDesc.preExposure = 1.0f;
        dispatchDesc.reset = (dispatchUpscaleDesc.flags & DispatchUpscaleBits::RESET_HISTORY) != 0;
        dispatchDesc.cameraNear = dispatchUpscaleDesc.settings.fsr.zNear;
        dispatchDesc.cameraFar = (m_Desc.flags & UpscalerBits::DEPTH_INFINITE) ? FLT_MAX : dispatchUpscaleDesc.settings.fsr.zFar;
        dispatchDesc.cameraFovAngleVertical = dispatchUpscaleDesc.settings.fsr.verticalFov;
        dispatchDesc.viewSpaceToMetersFactor = dispatchUpscaleDesc.settings.fsr.viewSpaceToMetersFactor;
        dispatchDesc.flags = (m_Desc.flags & UpscalerBits::NON_LINEAR) ? FFX_UPSCALE_FLAG_NON_LINEAR_COLOR_SRGB : 0;

        ffxReturnCode_t result = m.ffx->Dispatch(&m.ffx->context, &dispatchDesc.header);
        CHECK(result == FFX_API_RETURN_OK, "ffxDispatch() failed!");
    }
#endif

#if NRI_ENABLE_NGX_SDK
    if (m_Desc.type == UpscalerType::DLSR) {
        ExclusiveScope lock(g_ngx.lock);

        const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);
        const DLSRGuides& guides = dispatchUpscaleDesc.guides.dlsr;

        uint64_t outputNative = m_NRI.GetTextureNativeObject(*output.texture);
        uint64_t inputNative = m_NRI.GetTextureNativeObject(*input.texture);
        uint64_t mvNative = m_NRI.GetTextureNativeObject(*guides.mv.texture);
        uint64_t depthNative = m_NRI.GetTextureNativeObject(*guides.depth.texture);
        uint64_t exposureNative = m_NRI.GetTextureNativeObject(*guides.exposure.texture);
        uint64_t reactiveNative = m_NRI.GetTextureNativeObject(*guides.reactive.texture);

        void* commandBufferNative = m_NRI.GetCommandBufferNativeObject(commandBuffer);

        NVSDK_NGX_Result result = NVSDK_NGX_Result_Fail;

#    if NRI_ENABLE_D3D11_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D11) {
            NVSDK_NGX_D3D11_DLSS_Eval_Params srEvalParams = {};
            srEvalParams.Feature.pInColor = (ID3D11Resource*)inputNative;
            srEvalParams.Feature.pInOutput = (ID3D11Resource*)outputNative;
            srEvalParams.pInMotionVectors = (ID3D11Resource*)mvNative;
            srEvalParams.pInDepth = (ID3D11Resource*)depthNative;
            srEvalParams.pInExposureTexture = (ID3D11Resource*)exposureNative;
            srEvalParams.pInBiasCurrentColorMask = (ID3D11Resource*)reactiveNative;
            srEvalParams.InJitterOffsetX = dispatchUpscaleDesc.cameraJitter.x;
            srEvalParams.InJitterOffsetY = dispatchUpscaleDesc.cameraJitter.y;
            srEvalParams.InRenderSubrectDimensions = {dispatchUpscaleDesc.currentResolution.w, dispatchUpscaleDesc.currentResolution.h};
            srEvalParams.InReset = (dispatchUpscaleDesc.flags & DispatchUpscaleBits::RESET_HISTORY) ? true : false;
            srEvalParams.InMVScaleX = dispatchUpscaleDesc.mvScale.x;
            srEvalParams.InMVScaleY = dispatchUpscaleDesc.mvScale.y;

            result = NGX_D3D11_EVALUATE_DLSS_EXT((ID3D11DeviceContext*)commandBufferNative, m.ngx->handle, m.ngx->params, &srEvalParams);
        }
#    endif

#    if NRI_ENABLE_D3D12_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12) {
            NVSDK_NGX_D3D12_DLSS_Eval_Params srEvalParams = {};
            srEvalParams.Feature.pInColor = (ID3D12Resource*)inputNative;
            srEvalParams.Feature.pInOutput = (ID3D12Resource*)outputNative;
            srEvalParams.pInMotionVectors = (ID3D12Resource*)mvNative;
            srEvalParams.pInDepth = (ID3D12Resource*)depthNative;
            srEvalParams.pInExposureTexture = (ID3D12Resource*)exposureNative;
            srEvalParams.pInBiasCurrentColorMask = (ID3D12Resource*)reactiveNative;
            srEvalParams.InJitterOffsetX = dispatchUpscaleDesc.cameraJitter.x;
            srEvalParams.InJitterOffsetY = dispatchUpscaleDesc.cameraJitter.y;
            srEvalParams.InRenderSubrectDimensions = {dispatchUpscaleDesc.currentResolution.w, dispatchUpscaleDesc.currentResolution.h};
            srEvalParams.InReset = (dispatchUpscaleDesc.flags & DispatchUpscaleBits::RESET_HISTORY) ? true : false;
            srEvalParams.InMVScaleX = dispatchUpscaleDesc.mvScale.x;
            srEvalParams.InMVScaleY = dispatchUpscaleDesc.mvScale.y;

            result = NGX_D3D12_EVALUATE_DLSS_EXT((ID3D12GraphicsCommandList*)commandBufferNative, m.ngx->handle, m.ngx->params, &srEvalParams);
        }
#    endif

#    if NRI_ENABLE_VK_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::VK) {
            NVSDK_NGX_Resource_VK outputVk = NgxGetResource(m_NRI, output, outputNative, true);
            NVSDK_NGX_Resource_VK inputVk = NgxGetResource(m_NRI, input, inputNative);
            NVSDK_NGX_Resource_VK mvVk = NgxGetResource(m_NRI, guides.mv, mvNative);
            NVSDK_NGX_Resource_VK depthVk = NgxGetResource(m_NRI, guides.depth, depthNative);
            NVSDK_NGX_Resource_VK exposureVk = NgxGetResource(m_NRI, guides.exposure, exposureNative);
            NVSDK_NGX_Resource_VK reactiveVk = NgxGetResource(m_NRI, guides.reactive, reactiveNative);

            NVSDK_NGX_VK_DLSS_Eval_Params srEvalParams = {};
            srEvalParams.Feature.pInColor = &inputVk;
            srEvalParams.Feature.pInOutput = &outputVk;
            srEvalParams.pInMotionVectors = &mvVk;
            srEvalParams.pInDepth = &depthVk;
            srEvalParams.pInExposureTexture = guides.exposure.texture ? &exposureVk : nullptr;
            srEvalParams.pInBiasCurrentColorMask = guides.reactive.texture ? &reactiveVk : nullptr;
            srEvalParams.InJitterOffsetX = dispatchUpscaleDesc.cameraJitter.x;
            srEvalParams.InJitterOffsetY = dispatchUpscaleDesc.cameraJitter.y;
            srEvalParams.InRenderSubrectDimensions = {dispatchUpscaleDesc.currentResolution.w, dispatchUpscaleDesc.currentResolution.h};
            srEvalParams.InReset = (dispatchUpscaleDesc.flags & DispatchUpscaleBits::RESET_HISTORY) ? true : false;
            srEvalParams.InMVScaleX = dispatchUpscaleDesc.mvScale.x;
            srEvalParams.InMVScaleY = dispatchUpscaleDesc.mvScale.y;

            result = NGX_VULKAN_EVALUATE_DLSS_EXT((VkCommandBuffer)commandBufferNative, m.ngx->handle, m.ngx->params, &srEvalParams);
        }
#    endif

        CHECK(result == NVSDK_NGX_Result_Success, "DLSR evaluation failed!");
    }

    if (m_Desc.type == UpscalerType::DLRR) {
        ExclusiveScope lock(g_ngx.lock);

        const DeviceDesc& deviceDesc = m_NRI.GetDeviceDesc(m_Device);
        const DLRRGuides& guides = dispatchUpscaleDesc.guides.dlrr;

        uint64_t outputNative = m_NRI.GetTextureNativeObject(*output.texture);
        uint64_t inputNative = m_NRI.GetTextureNativeObject(*input.texture);
        uint64_t mvNative = m_NRI.GetTextureNativeObject(*guides.mv.texture);
        uint64_t depthNative = m_NRI.GetTextureNativeObject(*guides.depth.texture);
        uint64_t normalRoughnessNative = m_NRI.GetTextureNativeObject(*guides.normalRoughness.texture);
        uint64_t diffuseAlbedoNative = m_NRI.GetTextureNativeObject(*guides.diffuseAlbedo.texture);
        uint64_t specularAlbedoNative = m_NRI.GetTextureNativeObject(*guides.specularAlbedo.texture);
        uint64_t specularMvOrHitTNative = m_NRI.GetTextureNativeObject(*guides.specularMvOrHitT.texture);
        uint64_t exposureNative = m_NRI.GetTextureNativeObject(*guides.exposure.texture);
        uint64_t reactiveNative = m_NRI.GetTextureNativeObject(*guides.reactive.texture);
        uint64_t sssNative = m_NRI.GetTextureNativeObject(*guides.sss.texture);

        void* commandBufferNative = m_NRI.GetCommandBufferNativeObject(commandBuffer);

        NVSDK_NGX_Result result = NVSDK_NGX_Result_Fail;

#    if NRI_ENABLE_D3D11_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D11) {
            NVSDK_NGX_D3D11_DLSSD_Eval_Params rrEvalParams = {};
            rrEvalParams.pInColor = (ID3D11Resource*)inputNative;
            rrEvalParams.pInOutput = (ID3D11Resource*)outputNative;
            rrEvalParams.pInMotionVectors = (ID3D11Resource*)mvNative;
            rrEvalParams.pInDepth = (ID3D11Resource*)depthNative;
            rrEvalParams.pInNormals = (ID3D11Resource*)normalRoughnessNative;
            rrEvalParams.pInDiffuseAlbedo = (ID3D11Resource*)diffuseAlbedoNative;
            rrEvalParams.pInSpecularAlbedo = (ID3D11Resource*)specularAlbedoNative;
            rrEvalParams.pInExposureTexture = (ID3D11Resource*)exposureNative;
            rrEvalParams.pInBiasCurrentColorMask = (ID3D11Resource*)reactiveNative;
            rrEvalParams.pInScreenSpaceSubsurfaceScatteringGuide = (ID3D11Resource*)sssNative;
            rrEvalParams.InJitterOffsetX = dispatchUpscaleDesc.cameraJitter.x;
            rrEvalParams.InJitterOffsetY = dispatchUpscaleDesc.cameraJitter.y;
            rrEvalParams.InRenderSubrectDimensions = {dispatchUpscaleDesc.currentResolution.w, dispatchUpscaleDesc.currentResolution.h};
            rrEvalParams.InReset = (dispatchUpscaleDesc.flags & DispatchUpscaleBits::RESET_HISTORY) ? true : false;
            rrEvalParams.InMVScaleX = dispatchUpscaleDesc.mvScale.x;
            rrEvalParams.InMVScaleY = dispatchUpscaleDesc.mvScale.y;

            if (dispatchUpscaleDesc.flags & DispatchUpscaleBits::USE_SPECULAR_MOTION)
                rrEvalParams.pInMotionVectorsReflections = (ID3D11Resource*)specularMvOrHitTNative;
            else {
                rrEvalParams.pInSpecularHitDistance = (ID3D11Resource*)specularMvOrHitTNative;
                rrEvalParams.pInWorldToViewMatrix = (float*)dispatchUpscaleDesc.settings.dlrr.worldToViewMatrix;
                rrEvalParams.pInViewToClipMatrix = (float*)dispatchUpscaleDesc.settings.dlrr.viewToClipMatrix;
            }

            result = NGX_D3D11_EVALUATE_DLSSD_EXT((ID3D11DeviceContext*)commandBufferNative, m.ngx->handle, m.ngx->params, &rrEvalParams);
        }
#    endif

#    if NRI_ENABLE_D3D12_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::D3D12) {
            NVSDK_NGX_D3D12_DLSSD_Eval_Params rrEvalParams = {};
            rrEvalParams.pInColor = (ID3D12Resource*)inputNative;
            rrEvalParams.pInOutput = (ID3D12Resource*)outputNative;
            rrEvalParams.pInMotionVectors = (ID3D12Resource*)mvNative;
            rrEvalParams.pInDepth = (ID3D12Resource*)depthNative;
            rrEvalParams.pInNormals = (ID3D12Resource*)normalRoughnessNative;
            rrEvalParams.pInDiffuseAlbedo = (ID3D12Resource*)diffuseAlbedoNative;
            rrEvalParams.pInSpecularAlbedo = (ID3D12Resource*)specularAlbedoNative;
            rrEvalParams.pInExposureTexture = (ID3D12Resource*)exposureNative;
            rrEvalParams.pInBiasCurrentColorMask = (ID3D12Resource*)reactiveNative;
            rrEvalParams.pInScreenSpaceSubsurfaceScatteringGuide = (ID3D12Resource*)sssNative;
            rrEvalParams.InJitterOffsetX = dispatchUpscaleDesc.cameraJitter.x;
            rrEvalParams.InJitterOffsetY = dispatchUpscaleDesc.cameraJitter.y;
            rrEvalParams.InRenderSubrectDimensions = {dispatchUpscaleDesc.currentResolution.w, dispatchUpscaleDesc.currentResolution.h};
            rrEvalParams.InReset = (dispatchUpscaleDesc.flags & DispatchUpscaleBits::RESET_HISTORY) ? true : false;
            rrEvalParams.InMVScaleX = dispatchUpscaleDesc.mvScale.x;
            rrEvalParams.InMVScaleY = dispatchUpscaleDesc.mvScale.y;

            if (dispatchUpscaleDesc.flags & DispatchUpscaleBits::USE_SPECULAR_MOTION)
                rrEvalParams.pInMotionVectorsReflections = (ID3D12Resource*)specularMvOrHitTNative;
            else {
                rrEvalParams.pInSpecularHitDistance = (ID3D12Resource*)specularMvOrHitTNative;
                rrEvalParams.pInWorldToViewMatrix = (float*)dispatchUpscaleDesc.settings.dlrr.worldToViewMatrix;
                rrEvalParams.pInViewToClipMatrix = (float*)dispatchUpscaleDesc.settings.dlrr.viewToClipMatrix;
            }

            result = NGX_D3D12_EVALUATE_DLSSD_EXT((ID3D12GraphicsCommandList*)commandBufferNative, m.ngx->handle, m.ngx->params, &rrEvalParams);
        }
#    endif

#    if NRI_ENABLE_VK_SUPPORT
        if (deviceDesc.graphicsAPI == GraphicsAPI::VK) {
            NVSDK_NGX_Resource_VK outputVk = NgxGetResource(m_NRI, output, outputNative, true);
            NVSDK_NGX_Resource_VK inputVk = NgxGetResource(m_NRI, input, inputNative);
            NVSDK_NGX_Resource_VK mvVk = NgxGetResource(m_NRI, guides.mv, mvNative);
            NVSDK_NGX_Resource_VK depthVk = NgxGetResource(m_NRI, guides.depth, depthNative);
            NVSDK_NGX_Resource_VK normalRoughnessVk = NgxGetResource(m_NRI, guides.normalRoughness, normalRoughnessNative);
            NVSDK_NGX_Resource_VK diffuseAlbedoVk = NgxGetResource(m_NRI, guides.diffuseAlbedo, diffuseAlbedoNative);
            NVSDK_NGX_Resource_VK specularAlbedoVk = NgxGetResource(m_NRI, guides.specularAlbedo, specularAlbedoNative);
            NVSDK_NGX_Resource_VK specularMvOrHitTVk = NgxGetResource(m_NRI, guides.specularMvOrHitT, specularMvOrHitTNative);
            NVSDK_NGX_Resource_VK exposureVk = NgxGetResource(m_NRI, guides.exposure, exposureNative);
            NVSDK_NGX_Resource_VK reactiveVk = NgxGetResource(m_NRI, guides.reactive, reactiveNative);
            NVSDK_NGX_Resource_VK sssVk = NgxGetResource(m_NRI, guides.sss, sssNative);

            NVSDK_NGX_VK_DLSSD_Eval_Params rrEvalParams = {};
            rrEvalParams.pInColor = &inputVk;
            rrEvalParams.pInOutput = &outputVk;
            rrEvalParams.pInMotionVectors = &mvVk;
            rrEvalParams.pInDepth = &depthVk;
            rrEvalParams.pInNormals = &normalRoughnessVk;
            rrEvalParams.pInDiffuseAlbedo = &diffuseAlbedoVk;
            rrEvalParams.pInSpecularAlbedo = &specularAlbedoVk;
            rrEvalParams.pInExposureTexture = guides.exposure.texture ? &exposureVk : nullptr;
            rrEvalParams.pInBiasCurrentColorMask = guides.reactive.texture ? &reactiveVk : nullptr;
            rrEvalParams.pInScreenSpaceSubsurfaceScatteringGuide = guides.sss.texture ? &sssVk : nullptr;
            rrEvalParams.InJitterOffsetX = dispatchUpscaleDesc.cameraJitter.x;
            rrEvalParams.InJitterOffsetY = dispatchUpscaleDesc.cameraJitter.y;
            rrEvalParams.InRenderSubrectDimensions = {dispatchUpscaleDesc.currentResolution.w, dispatchUpscaleDesc.currentResolution.h};
            rrEvalParams.InReset = (dispatchUpscaleDesc.flags & DispatchUpscaleBits::RESET_HISTORY) ? true : false;
            rrEvalParams.InMVScaleX = dispatchUpscaleDesc.mvScale.x;
            rrEvalParams.InMVScaleY = dispatchUpscaleDesc.mvScale.y;

            if (dispatchUpscaleDesc.flags & DispatchUpscaleBits::USE_SPECULAR_MOTION)
                rrEvalParams.pInMotionVectorsReflections = &specularMvOrHitTVk;
            else {
                rrEvalParams.pInSpecularHitDistance = &specularMvOrHitTVk;
                rrEvalParams.pInWorldToViewMatrix = (float*)dispatchUpscaleDesc.settings.dlrr.worldToViewMatrix;
                rrEvalParams.pInViewToClipMatrix = (float*)dispatchUpscaleDesc.settings.dlrr.viewToClipMatrix;
            }

            result = NGX_VULKAN_EVALUATE_DLSSD_EXT((VkCommandBuffer)commandBufferNative, m.ngx->handle, m.ngx->params, &rrEvalParams);
        }
#    endif

        CHECK(result == NVSDK_NGX_Result_Success, "DLRR evaluation failed!");
    }
#endif
}