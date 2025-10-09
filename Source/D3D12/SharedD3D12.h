// © 2021 NVIDIA Corporation

#pragma once

#include <d3d12.h>
#include <pix.h>

// Validate Windows SDK version
static_assert(D3D12_SDK_VERSION >= 4, "Outdated Windows SDK. D3D12 Ultimate needed (SDK 1.4.9+, released 2021.04.20). Always prefer using latest Agility SDK!");

// TODO: "D3D12_SDK_VERSION" and "D3D12_PREVIEW_SDK_VERSION" are inconsistent and can't be used to check features support

#if (NRI_AGILITY_SDK_VERSION_MAJOR >= 618)
#    define NRI_D3D12_HAS_TIGHT_ALIGNMENT
#endif

#if (NRI_AGILITY_SDK_VERSION_MAJOR >= 616)
#    define NRI_D3D12_HAS_OPACITY_MICROMAP

#    ifndef D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_BYTE_ALIGNMENT // TODO: remove when fixed
#        define D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_BYTE_ALIGNMENT (128)
#    endif

#    ifndef D3D12_RAYTRACING_OPACITY_MICROMAP_OC1_MAX_SUBDIVISION_LEVEL // TODO: remove when fixed
#        define D3D12_RAYTRACING_OPACITY_MICROMAP_OC1_MAX_SUBDIVISION_LEVEL (12)
#    endif
#else
struct D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC {
    uint32_t unused;
};

struct D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY {
    uint32_t unused;
};
#endif

#include "SharedExternal.h"

namespace nri {

typedef size_t DescriptorHandleCPU;   // D3D12_CPU_DESCRIPTOR_HANDLE
typedef uint64_t DescriptorHandleGPU; // D3D12_GPU_DESCRIPTOR_HANDLE

struct MemoryTypeInfo {
    uint16_t heapFlags;
    uint8_t heapType;
    bool mustBeDedicated;
};

const D3D12_HEAP_FLAGS HEAP_FLAG_MSAA_ALIGNMENT = (D3D12_HEAP_FLAGS)(1 << 15);

inline MemoryType Pack(const MemoryTypeInfo& memoryTypeInfo) {
    return *(MemoryType*)&memoryTypeInfo;
}

inline MemoryTypeInfo Unpack(const MemoryType& memoryType) {
    return *(MemoryTypeInfo*)&memoryType;
}

static_assert(sizeof(MemoryTypeInfo) == sizeof(MemoryType), "Must be equal");

enum DescriptorHeapType : uint8_t {
    RESOURCE = 0,
    SAMPLER,
    MAX_NUM
};

#define DESCRIPTOR_HANDLE_HEAP_TYPE_BIT_NUM   2
#define DESCRIPTOR_HANDLE_HEAP_INDEX_BIT_NUM  16
#define DESCRIPTOR_HANDLE_HEAP_OFFSET_BIT_NUM 14

// TODO: no castable formats since typed resources are initially "TYPELESS"
#define NO_CASTABLE_FORMATS 0, nullptr

struct DescriptorHandle {
    uint32_t heapType : DESCRIPTOR_HANDLE_HEAP_TYPE_BIT_NUM;
    uint32_t heapIndex : DESCRIPTOR_HANDLE_HEAP_INDEX_BIT_NUM;
    uint32_t heapOffset : DESCRIPTOR_HANDLE_HEAP_OFFSET_BIT_NUM;
};

constexpr uint32_t DESCRIPTORS_BATCH_SIZE = 1024;

static_assert(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES <= (1 << DESCRIPTOR_HANDLE_HEAP_TYPE_BIT_NUM), "Out of bounds");
static_assert(DESCRIPTORS_BATCH_SIZE <= (1 << DESCRIPTOR_HANDLE_HEAP_OFFSET_BIT_NUM), "Out of bounds");

struct DescriptorHeapDesc {
    ComPtr<ID3D12DescriptorHeap> heap;
    DescriptorHandleGPU baseHandleGPU = 0;
    DescriptorHandleCPU baseHandleCPU = 0;
    uint32_t descriptorSize = 0;
    uint32_t num = 0;
};

void ConvertBotomLevelGeometries(const BottomLevelGeometryDesc* geometries, uint32_t geometryNum,
    D3D12_RAYTRACING_GEOMETRY_DESC* geometryDescs,
    D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC* triangleDescs,
    D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC* micromapDescs);

bool GetTextureDesc(const TextureD3D12Desc& textureD3D12Desc, TextureDesc& textureDesc);
bool GetBufferDesc(const BufferD3D12Desc& bufferD3D12Desc, BufferDesc& bufferDesc);
uint64_t GetMemorySizeD3D12(const MemoryD3D12Desc& memoryD3D12Desc);
D3D12_RESIDENCY_PRIORITY ConvertPriority(float priority);
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE GetAccelerationStructureType(AccelerationStructureType accelerationStructureType);
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS GetAccelerationStructureFlags(AccelerationStructureBits accelerationStructureBits);
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS GetMicromapFlags(MicromapBits micromapBits);
D3D12_RAYTRACING_GEOMETRY_TYPE GetGeometryType(BottomLevelGeometryType geometryType);
D3D12_RAYTRACING_GEOMETRY_FLAGS GetGeometryFlags(BottomLevelGeometryBits bottomLevelGeometryBits);
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE GetCopyMode(CopyMode copyMode);
D3D12_TEXTURE_ADDRESS_MODE GetAddressMode(AddressMode addressMode);
D3D12_COMPARISON_FUNC GetCompareOp(CompareOp compareOp);
D3D12_COMMAND_LIST_TYPE GetCommandListType(QueueType queueType);
D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorHeapType(DescriptorType descriptorType);
D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType(Topology topology);
D3D_PRIMITIVE_TOPOLOGY GetPrimitiveTopology(Topology topology, uint8_t tessControlPointNum);
D3D12_FILL_MODE GetFillMode(FillMode fillMode);
D3D12_CULL_MODE GetCullMode(CullMode cullMode);
D3D12_STENCIL_OP GetStencilOp(StencilOp stencilFunc);
UINT8 GetRenderTargetWriteMask(ColorWriteBits colorWriteMask);
D3D12_LOGIC_OP GetLogicOp(LogicOp logicOp);
D3D12_BLEND GetBlend(BlendFactor blendFactor);
D3D12_BLEND_OP GetBlendOp(BlendOp blendFunc);
D3D12_DESCRIPTOR_RANGE_TYPE GetDescriptorRangesType(DescriptorType descriptorType);
D3D12_RESOURCE_DIMENSION GetResourceDimension(TextureType textureType);
D3D12_SHADING_RATE GetShadingRate(ShadingRate shadingRate);
D3D12_SHADING_RATE_COMBINER GetShadingRateCombiner(ShadingRateCombiner shadingRateCombiner);
D3D12_FILTER GetFilter(const SamplerDesc& samplerDesc);

} // namespace nri

#if NRI_ENABLE_AMDAGS
#    define AGS_GCC // Fixes Clang warning: 'dllexport' attribute only applies to functions, variables, classes, and Objective-C interfaces [-Werror,-Wignored-attributes]
#    include "amd_ags.h"

struct AmdExtD3D12 {
    // Funcs first
    AGS_INITIALIZE Initialize;
    AGS_DEINITIALIZE Deinitialize;
    AGS_DRIVEREXTENSIONSDX12_CREATEDEVICE CreateDeviceD3D12;
    AGS_DRIVEREXTENSIONSDX12_DESTROYDEVICE DestroyDeviceD3D12;
    Library* library;
    AGSContext* context;
    bool isWrapped;

    ~AmdExtD3D12() {
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

typedef HRESULT(WINAPI* PIX_BEGINEVENTONCOMMANDLIST)(ID3D12GraphicsCommandList* commandList, UINT64 color, _In_ PCSTR formatString);
typedef HRESULT(WINAPI* PIX_ENDEVENTONCOMMANDLIST)(ID3D12GraphicsCommandList* commandList);
typedef HRESULT(WINAPI* PIX_SETMARKERONCOMMANDLIST)(ID3D12GraphicsCommandList* commandList, UINT64 color, _In_ PCSTR formatString);
typedef HRESULT(WINAPI* PIX_BEGINEVENTONCOMMANDQUEUE)(ID3D12CommandQueue* queue, UINT64 color, _In_ PCSTR formatString);
typedef HRESULT(WINAPI* PIX_ENDEVENTONCOMMANDQUEUE)(ID3D12CommandQueue* queue);
typedef HRESULT(WINAPI* PIX_SETMARKERONCOMMANDQUEUE)(ID3D12CommandQueue* queue, UINT64 color, _In_ PCSTR formatString);

struct PixExt {
    // Funcs first
    PIX_BEGINEVENTONCOMMANDLIST BeginEventOnCommandList;
    PIX_ENDEVENTONCOMMANDLIST EndEventOnCommandList;
    PIX_SETMARKERONCOMMANDLIST SetMarkerOnCommandList;
    PIX_BEGINEVENTONCOMMANDQUEUE BeginEventOnQueue;
    PIX_ENDEVENTONCOMMANDQUEUE EndEventOnQueue;
    PIX_SETMARKERONCOMMANDQUEUE SetMarkerOnQueue;
    Library* library;

    ~PixExt() {
        if (library)
            UnloadSharedLibrary(*library);
    }
};

#include "DeviceD3D12.h"
