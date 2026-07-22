// © 2026 NVIDIA Corporation

#pragma once

#if defined(__EMSCRIPTEN__)
#    include <webgpu/webgpu.h>
#else
#    include <webgpu/wgpu.h>
#endif

#if defined(_WIN32)
#    include <windows.h>
#endif

#include "SharedExternal.h"

namespace nri {

struct BufferWGPU;
struct CommandAllocatorWGPU;
struct CommandBufferWGPU;
struct DescriptorPoolWGPU;
struct DescriptorSetWGPU;
struct DescriptorWGPU;
struct DeviceWGPU;
struct FenceWGPU;
struct MemoryWGPU;
struct PipelineCacheWGPU;
struct PipelineLayoutWGPU;
struct PipelineWGPU;
struct QueryPoolWGPU;
struct QueueWGPU;
struct SwapChainWGPU;
struct TextureWGPU;

inline WGPUStringView WGPUString(const char* string) {
    return {string, WGPU_STRLEN};
}

#if defined(__EMSCRIPTEN__)
inline bool WaitForFuture(WGPUInstance instance, WGPUFuture future, uint64_t timeoutNs = UINT64_MAX) {
    WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
    waitInfo.future = future;

    return wgpuInstanceWaitAny(instance, 1, &waitInfo, timeoutNs) == WGPUWaitStatus_Success && waitInfo.completed == WGPU_TRUE;
}
#endif

inline uint32_t GetCountOrOne(uint32_t value) {
    return value ? value : 1;
}

inline Dim_t GetCountOrOne(Dim_t value) {
    return value ? value : 1;
}

WGPUShaderStage GetShaderStageFlags(StageBits stageBits);
WGPUTextureFormat GetTextureFormat(Format format);
WGPUTextureSampleType GetTextureSampleType(Format format);
WGPUTextureFormat GetSwapChainTextureFormat(SwapChainFormat format);
Format GetNRIFormat(WGPUTextureFormat format);
WGPUTextureUsage GetTextureUsage(TextureUsageBits usage);
WGPUBufferUsage GetBufferUsage(BufferUsageBits usage);
WGPUVertexFormat GetVertexFormat(Format format);
WGPUTextureDimension GetTextureDimension(TextureType type);
WGPUTextureViewDimension GetTextureViewDimension(TextureView type, const TextureDesc& textureDesc);
WGPUTextureAspect GetTextureAspect(PlaneBits planes);
WGPUAddressMode GetAddressMode(AddressMode mode);
WGPUFilterMode GetFilterMode(Filter filter);
WGPUMipmapFilterMode GetMipmapFilterMode(Filter filter);
WGPUCompareFunction GetCompareFunction(CompareOp compareOp);
WGPUStencilOperation GetStencilOperation(StencilOp stencilOp);
WGPUPrimitiveTopology GetPrimitiveTopology(Topology topology);
WGPUIndexFormat GetIndexFormat(IndexType indexType);
WGPUFrontFace GetFrontFace(bool frontCounterClockwise);
WGPUCullMode GetCullMode(CullMode cullMode);
WGPUBlendFactor GetBlendFactor(BlendFactor blendFactor);
WGPUBlendOperation GetBlendOperation(BlendOp blendOp);
WGPUColorWriteMask GetColorWriteMask(ColorWriteBits colorWriteMask);
WGPULoadOp GetLoadOp(LoadOp loadOp);
WGPUStoreOp GetStoreOp(StoreOp storeOp);
WGPUComponentSwizzle GetComponentSwizzle(ComponentSwizzle componentSwizzle);
FormatSupportBits GetFormatSupportWGPU(Format format);
Vendor GetVendorFromPCIID(uint32_t vendorId);
Architecture GetArchitecture(WGPUAdapterType adapterType);

} // namespace nri

#include "DeviceWGPU.h"
