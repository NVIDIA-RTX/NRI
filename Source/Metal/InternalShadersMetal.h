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
    CONVERT_INSTANCES,
    COPY_TOP_LEVEL_HEADER,
    COPY_WORDS,
    PREPARE_RAYS_INDIRECT,

    // "CLEAR_STORAGE_TEXTURE + dimension * 3 + type", see "GetClearStorageKernelMetal"
    CLEAR_STORAGE_TEXTURE,

    MAX_NUM = CLEAR_STORAGE_TEXTURE + 6 * 3
};

// Host mirrors of the ray-tracing helper kernel argument structures ("buffer(3)")
struct ConvertInstancesArgsMetal {
    MTL::GPUAddress src; // "TopLevelInstance" array
    MTL::GPUAddress dst; // "MTLIndirectAccelerationStructureInstanceDescriptor" array
    MTL::GPUAddress header;
    uint64_t accelerationStructure;
    uint32_t instanceNum;
    uint32_t padding;
};

struct CopyTopLevelHeaderArgsMetal {
    MTL::GPUAddress srcContributions;
    MTL::GPUAddress dstHeader;
    uint64_t dstAccelerationStructure;
    uint32_t num;
    uint32_t padding;
};

struct CopyWordsArgsMetal {
    MTL::GPUAddress src;
    MTL::GPUAddress dst;
};

struct PrepareRaysIndirectArgsMetal {
    MTL::GPUAddress src;      // "DispatchRaysIndirectDesc"
    MTL::GPUAddress dst;      // "IRDispatchRaysArgument::DispatchRaysDesc"
    MTL::GPUAddress dispatch; // "MTLDispatchThreadsIndirectArguments"
};

// "TopLevelInstanceBits" 0-3 match "MTLAccelerationStructureInstanceOptions", micromap bits are dropped by the kernel
static_assert(sizeof(TopLevelInstance) == 64, "Unexpected 'TopLevelInstance' size");
static_assert(sizeof(MTL::IndirectAccelerationStructureInstanceDescriptor) == 72, "Unexpected 'MTLIndirectAccelerationStructureInstanceDescriptor' size");
static_assert((uint32_t)TopLevelInstanceBits::TRIANGLE_CULL_DISABLE == MTL::AccelerationStructureInstanceOptionDisableTriangleCulling, "Instance flag mismatch");
static_assert((uint32_t)TopLevelInstanceBits::TRIANGLE_FLIP_FACING == MTL::AccelerationStructureInstanceOptionTriangleFrontFacingWindingCounterClockwise, "Instance flag mismatch");
static_assert((uint32_t)TopLevelInstanceBits::FORCE_OPAQUE == MTL::AccelerationStructureInstanceOptionOpaque, "Instance flag mismatch");
static_assert((uint32_t)TopLevelInstanceBits::FORCE_NON_OPAQUE == MTL::AccelerationStructureInstanceOptionNonOpaque, "Instance flag mismatch");
static_assert(offsetof(DispatchRaysIndirectDesc, width) == 88 && sizeof(DispatchRaysIndirectDesc) == 104, "'NriPrepareRaysIndirect' layout mismatch");

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
