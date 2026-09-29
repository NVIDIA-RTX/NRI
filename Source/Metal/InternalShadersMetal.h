// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

// Argument table slots of internal shaders ("Shaders/InternalMetal.metal"). "CmdClearAttachments" uses "INTERNAL_SLOT_CONSTANTS" in the
// app's argument table, it must not overlap resource heap, sampler heap and root slots
constexpr uint32_t INTERNAL_SLOT_KERNEL_CONSTANTS = 0; // indirect draw preparation
constexpr uint32_t INTERNAL_SLOT_CONSTANTS = 3;
constexpr uint32_t INTERNAL_SLOT_CLEAR_VALUE = 4;
constexpr uint32_t INTERNAL_SLOT_CLEAR_OFFSET = 5; // 3D slice or typed buffer element offset
constexpr uint32_t INTERNAL_SLOT_TEXTURE = 0;

// Compute kernels in "Shaders/InternalMetal.metal"
enum class InternalKernelMetal : uint8_t {
    FILTER_DRAWS,
    PREPARE_DRAW_ROOTS,
    EMULATE_DRAWS,
    CLEAR_STORAGE_BUFFER,

    // "CLEAR_STORAGE_TEXTURE + dimension * 3 + type", see "GetClearStorageKernelMetal"
    CLEAR_STORAGE_TEXTURE,

    MAX_NUM = CLEAR_STORAGE_TEXTURE + 6 * 3
};

struct ClearPipelineKeyMetal {
    MTL::PixelFormat colors[8] = {}; // Metal 4 pipelines don't include depth / stencil formats
    uint8_t colorNum = 0;
    uint8_t colorIndex = 0;
    uint8_t sampleNum = 1;
    uint8_t colorType = 0; // "ColorTypeMetal"
    PlaneBits planes = PlaneBits::NONE;
};

struct ClearPipelineMetal {
    ClearPipelineKeyMetal key;
    MTL::RenderPipelineState* pipeline = nullptr;
    MTL::DepthStencilState* depthStencil = nullptr;
};

struct ResolvePipelineMetal {
    MTL::RenderPipelineState* pipeline = nullptr;
    MTL::PixelFormat format = MTL::PixelFormatInvalid;
    uint8_t colorType = 0; // "ColorTypeMetal"
    bool isArray = false;
};

// Device-level internal shaders ("Shaders/InternalMetal.metal"), pipelines are created on first use. Thread-safe
struct InternalShadersMetal {
    InternalShadersMetal(DeviceMetal& device);
    ~InternalShadersMetal();

    inline MTL4::FunctionDescriptor* GetDepthOnlyFragmentFunction() const {
        return m_DepthOnlyFragmentFunction;
    }

    Result Create();
    MTL::ComputePipelineState* GetKernel(InternalKernelMetal kernel);
    ClearPipelineMetal GetClearPipeline(const ClearPipelineKeyMetal& key); // returned by value, since the cache can grow concurrently
    MTL::RenderPipelineState* GetResolvePipeline(MTL::PixelFormat format, uint8_t colorType, bool isArray);

private:
    MTL4::FunctionDescriptor* NewFunction(const char* name) const;
    const ClearPipelineMetal* FindClearPipeline(const ClearPipelineKeyMetal& key) const;
    MTL::RenderPipelineState* FindResolvePipeline(MTL::PixelFormat format, uint8_t colorType, bool isArray) const;

    DeviceMetal& m_Device;
    MTL::Library* m_Library = nullptr;
    MTL4::FunctionDescriptor* m_DepthOnlyFragmentFunction = nullptr;
    std::atomic<MTL::ComputePipelineState*> m_Kernels[(size_t)InternalKernelMetal::MAX_NUM] = {};
    Vector<ClearPipelineMetal> m_ClearPipelines;
    Vector<ResolvePipelineMetal> m_ResolvePipelines;
    std::mutex m_KernelLock;                // kernel creation
    std::shared_mutex m_RenderPipelineLock; // render pipeline lookups (shared) and creation (exclusive)
};

} // namespace nri
