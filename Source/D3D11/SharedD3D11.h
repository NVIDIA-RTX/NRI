// © 2021 NVIDIA Corporation

#pragma once

#include <d3d11_4.h>
#include <pix.h>

#include "SharedExternal.h"

#define USE_ANNOTATION_INT 0 // unfortunately, just a few tools support "BeginEventInt" and "SetMarkerInt"

struct AGSContext;

struct ID3D11DeviceContext4;
typedef ID3D11DeviceContext4 ID3D11DeviceContextBest;

namespace nri {

constexpr Dim_t NULL_TEXTURE_REGION_DESC = Dim_t(-1);

enum class DescriptorTypeDX11 : uint8_t {
    // don't change order
    NO_SHADER_VISIBLE,
    RESOURCE,
    SAMPLER,
    STORAGE,
    // must be last!
    CONSTANT,
};

D3D11_PRIMITIVE_TOPOLOGY GetD3D11TopologyFromTopology(Topology topology, uint32_t patchPoints);
D3D11_CULL_MODE GetD3D11CullModeFromCullMode(CullMode cullMode);
D3D11_COMPARISON_FUNC GetD3D11ComparisonFuncFromCompareOp(CompareOp compareOp);
D3D11_STENCIL_OP GetD3D11StencilOpFromStencilOp(StencilOp stencilFunc);
D3D11_BLEND_OP GetD3D11BlendOp(BlendOp blendFunc);
D3D11_BLEND GetD3D11BlendFromBlendFactor(BlendFactor blendFactor);
D3D11_LOGIC_OP GetD3D11LogicOp(LogicOp logicalFunc);
void FillSamplerDesc(const SamplerDesc& samplerDesc, D3D11_SAMPLER_DESC& desc);
bool GetTextureDesc(const TextureD3D11Desc& textureD3D11Desc, TextureDesc& textureDesc);
bool GetBufferDesc(const BufferD3D11Desc& bufferD3D11Desc, BufferDesc& bufferDesc);
uint32_t ConvertPriority(float priority);

struct SubresourceInfo {
    const void* resource = nullptr;
    uint64_t data = 0;

    inline void Initialize(const void* tex, Dim_t mipOffset, Dim_t mipNum, Dim_t layerOffset, Dim_t layerNum) {
        resource = tex;
        data = (uint64_t(layerNum) << 48) | (uint64_t(layerOffset) << 32) | (uint64_t(mipNum) << 16) | uint64_t(mipOffset);
    }

    inline void Initialize(const void* buf) {
        resource = buf;
        data = 0;
    }

    friend bool operator==(const SubresourceInfo& a, const SubresourceInfo& b) {
        return a.resource == b.resource && a.data == b.data;
    }
};

struct SubresourceAndSlot {
    SubresourceInfo subresource;
    uint32_t slot;
};

struct BindingState {
    inline BindingState(const StdAllocator<uint8_t>& stdAllocator)
        : resources(stdAllocator)
        , storages(stdAllocator) {
    }

    inline void TrackSubresource_UnbindIfNeeded_PostponeGraphicsStorageBinding(ID3D11DeviceContextBest* deferredContext, const SubresourceInfo& subresource, void* descriptor, uint32_t slot, bool isGraphics, bool isStorage) {
        constexpr void* null = nullptr;

        if (isStorage) {
            for (uint32_t i = 0; i < (uint32_t)resources.size(); i++) {
                const SubresourceAndSlot& subresourceAndSlot = resources[i];
                if (subresourceAndSlot.subresource == subresource) {
                    // TODO: store visibility to unbind only for necessary stages
                    deferredContext->VSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
                    deferredContext->HSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
                    deferredContext->DSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
                    deferredContext->GSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
                    deferredContext->PSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
                    deferredContext->CSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);

                    resources[i] = resources.back();
                    resources.pop_back();
                    i--;
                }
            }

            storages.push_back({subresource, slot});

            if (isGraphics)
                graphicsStorageDescriptors[slot] = (ID3D11UnorderedAccessView*)descriptor;
        } else {
            for (uint32_t i = 0; i < (uint32_t)storages.size(); i++) {
                const SubresourceAndSlot& subresourceAndSlot = storages[i];
                if (subresourceAndSlot.subresource == subresource) {
                    deferredContext->CSSetUnorderedAccessViews(subresourceAndSlot.slot, 1, (ID3D11UnorderedAccessView**)&null, nullptr);

                    graphicsStorageDescriptors[subresourceAndSlot.slot] = nullptr;

                    storages[i] = storages.back();
                    storages.pop_back();
                    i--;
                }
            }

            resources.push_back({subresource, slot});
        }
    }

    inline void UnbindAndReset(ID3D11DeviceContextBest* deferredContext) {
        constexpr void* null = nullptr;

        for (const SubresourceAndSlot& subresourceAndSlot : resources) {
            // TODO: store visibility to unbind only for necessary stages
            deferredContext->VSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
            deferredContext->HSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
            deferredContext->DSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
            deferredContext->GSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
            deferredContext->PSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
            deferredContext->CSSetShaderResources(subresourceAndSlot.slot, 1, (ID3D11ShaderResourceView**)&null);
        }
        resources.clear();

        if (!storages.empty())
            deferredContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, 0, 0, nullptr, nullptr);
        for (const SubresourceAndSlot& subresourceAndSlot : storages)
            deferredContext->CSSetUnorderedAccessViews(subresourceAndSlot.slot, 1, (ID3D11UnorderedAccessView**)&null, nullptr);
        storages.clear();

        memset(&graphicsStorageDescriptors, 0, sizeof(graphicsStorageDescriptors));
    }

    Vector<SubresourceAndSlot> resources; // max expected size - D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT
    Vector<SubresourceAndSlot> storages;  // max expected size - D3D11_1_UAV_SLOT_COUNT
    std::array<ID3D11UnorderedAccessView*, D3D11_1_UAV_SLOT_COUNT> graphicsStorageDescriptors = {};
};

struct CommandBufferBase : public DebugNameBase {
    inline CommandBufferBase() {
    }

    virtual ~CommandBufferBase() {
    }

    virtual Result Create(ID3D11DeviceContext* precreatedContext) = 0;
    virtual void Submit() = 0;
    virtual ID3D11DeviceContextBest* GetNativeObject() const = 0;
    virtual const AllocationCallbacks& GetAllocationCallbacks() const = 0;
};

static inline uint64_t ComputeHash(const void* key, uint32_t len) {
    const uint8_t* p = (uint8_t*)key;
    uint64_t result = 14695981039346656037ull;
    while (len--)
        result = (result ^ (*p++)) * 1099511628211ull;

    return result;
}

struct SamplePositionsState {
    std::array<SampleLocation, 32> positions;
    uint64_t positionHash;
    Sample_t positionNum;

    inline void Reset() {
        memset(&positions, 0, sizeof(positions));

        positionNum = 0;
        positionHash = 0;
    }

    inline void Set(const SampleLocation* samplePositions, Sample_t samplePositionNum) {
        const uint32_t size = sizeof(SampleLocation) * samplePositionNum;

        memcpy(&positions, samplePositions, size);

        positionHash = ComputeHash(samplePositions, size);
        positionNum = samplePositionNum;
    }
};

} // namespace nri

#if NRI_ENABLE_AMDAGS
#    define AGS_GCC // Fixes Clang warning: 'dllexport' attribute only applies to functions, variables, classes, and Objective-C interfaces [-Werror,-Wignored-attributes]
#    include "amd_ags.h"

struct AmdExtD3D11 {
    // Funcs first
    AGS_INITIALIZE Initialize;
    AGS_DEINITIALIZE Deinitialize;
    AGS_DRIVEREXTENSIONSDX11_CREATEDEVICE CreateDeviceD3D11;
    AGS_DRIVEREXTENSIONSDX11_DESTROYDEVICE DestroyDeviceD3D11;
    AGS_DRIVEREXTENSIONSDX11_BEGINUAVOVERLAP BeginUAVOverlap;
    AGS_DRIVEREXTENSIONSDX11_ENDUAVOVERLAP EndUAVOverlap;
    AGS_DRIVEREXTENSIONSDX11_SETDEPTHBOUNDS SetDepthBounds;
    AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINSTANCEDINDIRECT DrawIndirect;
    AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINDEXEDINSTANCEDINDIRECT DrawIndexedIndirect;
    AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINSTANCEDINDIRECTCOUNTINDIRECT DrawIndirectCount;
    AGS_DRIVEREXTENSIONSDX11_MULTIDRAWINDEXEDINSTANCEDINDIRECTCOUNTINDIRECT DrawIndexedIndirectCount;
    AGS_DRIVEREXTENSIONSDX11_SETVIEWBROADCASTMASKS SetViewBroadcastMasks;
    Library* library;
    AGSContext* context;
    bool isWrapped;

    ~AmdExtD3D11() {
        if (context && !isWrapped)
            Deinitialize(context);

        if (library)
            UnloadSharedLibrary(*library);
    }
};

#endif

#if NRI_ENABLE_NVAPI
#    include "nvShaderExtnEnums.h"
#    include "nvapi.h"

struct NvExt {
    bool available;

    ~NvExt() {
        if (available)
            NvAPI_Unload();
    }
};

#endif

#include "DeviceD3D11.h"
