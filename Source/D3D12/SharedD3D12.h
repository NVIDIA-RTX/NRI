// © 2021 NVIDIA Corporation

#pragma once

#include <d3d12.h>
#include <pix.h>

#include "SharedExternal.h"

struct D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC {
    uint32_t unused;
};

struct D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY {
    uint32_t unused;
};

namespace nri {

typedef size_t DescriptorPointerCPU;
typedef uint64_t DescriptorPointerGPU;
typedef uint16_t HeapIndexType;
typedef uint16_t HeapOffsetType;

struct MemoryTypeInfo {
    uint16_t heapFlags;
    uint8_t heapType;
    bool mustBeDedicated;
};

inline MemoryType Pack(const MemoryTypeInfo& memoryTypeInfo) {
    return *(MemoryType*)&memoryTypeInfo;
}

inline MemoryTypeInfo Unpack(const MemoryType& memoryType) {
    return *(MemoryTypeInfo*)&memoryType;
}

static_assert(sizeof(MemoryTypeInfo) == sizeof(MemoryType), "Must be equal");

enum DescriptorHeapType : uint32_t {
    RESOURCE = 0,
    SAMPLER,
    MAX_NUM
};

struct DescriptorHandle {
    HeapIndexType heapIndex;
    HeapOffsetType heapOffset;
};

struct DescriptorHeapDesc {
    ComPtr<ID3D12DescriptorHeap> heap;
    DescriptorPointerCPU basePointerCPU = 0;
    DescriptorPointerGPU basePointerGPU = 0;
    uint32_t descriptorSize = 0;
    uint32_t num = 0;
};

void ConvertGeometryDescs(D3D12_RAYTRACING_GEOMETRY_DESC* geometryDescs,
    D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC* triangleDescs,
    D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC* micromapDescs,
    const BottomLevelGeometryDesc* geometries, uint32_t geometryNum);

void GetResourceDesc(D3D12_RESOURCE_DESC* desc, const BufferDesc& bufferDesc);
void GetResourceDesc(D3D12_RESOURCE_DESC* desc, const TextureDesc& textureDesc);
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
D3D12_RESOURCE_FLAGS GetBufferFlags(BufferUsageBits bufferUsage);
D3D12_RESOURCE_FLAGS GetTextureFlags(TextureUsageBits textureUsage);
D3D12_FILTER GetFilterIsotropic(Filter mip, Filter magnification, Filter minification, FilterExt filterExt, bool useComparison);
D3D12_FILTER GetFilterAnisotropic(FilterExt filterExt, bool useComparison);
D3D12_TEXTURE_ADDRESS_MODE GetAddressMode(AddressMode addressMode);
D3D12_COMPARISON_FUNC GetComparisonFunc(CompareFunc compareFunc);
D3D12_COMMAND_LIST_TYPE GetCommandListType(QueueType queueType);
D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorHeapType(DescriptorType descriptorType);
D3D12_HEAP_FLAGS GetHeapFlags(MemoryType memoryType);
D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType(Topology topology);
D3D_PRIMITIVE_TOPOLOGY GetPrimitiveTopology(Topology topology, uint8_t tessControlPointNum);
D3D12_FILL_MODE GetFillMode(FillMode fillMode);
D3D12_CULL_MODE GetCullMode(CullMode cullMode);
D3D12_STENCIL_OP GetStencilOp(StencilFunc stencilFunc);
UINT8 GetRenderTargetWriteMask(ColorWriteBits colorWriteMask);
D3D12_LOGIC_OP GetLogicOp(LogicFunc logicFunc);
D3D12_BLEND GetBlend(BlendFactor blendFactor);
D3D12_BLEND_OP GetBlendOp(BlendFunc blendFunc);
D3D12_SHADER_VISIBILITY GetShaderVisibility(StageBits shaderStage);
D3D12_DESCRIPTOR_RANGE_TYPE GetDescriptorRangesType(DescriptorType descriptorType);
D3D12_RESOURCE_DIMENSION GetResourceDimension(TextureType textureType);
D3D12_SHADING_RATE GetShadingRate(ShadingRate shadingRate);
D3D12_SHADING_RATE_COMBINER GetShadingRateCombiner(ShadingRateCombiner shadingRateCombiner);

} // namespace nri

#if NRI_ENABLE_D3D_EXTENSIONS
#    include "amd_ags.h"
#    include "nvShaderExtnEnums.h"
#    include "nvapi.h"

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

namespace D3D12MA {
class Allocator;
class Allocation;
} // namespace D3D12MA

#include "DeviceD3D12.h"
