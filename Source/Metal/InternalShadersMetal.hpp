// © 2026 NVIDIA Corporation

#include "InternalMetal.metallib.h"

// "nri_clear_storage_<dimension>_<type>" follow "CLEAR_STORAGE_TEXTURE" (see "GetClearStorageKernelMetal")
constexpr std::array<const char*, (size_t)InternalKernelMetal::MAX_NUM> g_InternalKernelNames = {
    "nri_filter_draws",          // FILTER_DRAWS
    "nri_prepare_draw_roots",    // PREPARE_DRAW_ROOTS
    "nri_emulate_draws",         // EMULATE_DRAWS
    "nri_clear_storage_buffer",  // CLEAR_STORAGE_BUFFER
    "nri_clear_storage_0_0",
    "nri_clear_storage_0_1",
    "nri_clear_storage_0_2",
    "nri_clear_storage_1_0",
    "nri_clear_storage_1_1",
    "nri_clear_storage_1_2",
    "nri_clear_storage_2_0",
    "nri_clear_storage_2_1",
    "nri_clear_storage_2_2",
    "nri_clear_storage_3_0",
    "nri_clear_storage_3_1",
    "nri_clear_storage_3_2",
    "nri_clear_storage_4_0",
    "nri_clear_storage_4_1",
    "nri_clear_storage_4_2",
    "nri_clear_storage_5_0",
    "nri_clear_storage_5_1",
    "nri_clear_storage_5_2",
};
NRI_VALIDATE_ARRAY_BY_PTR(g_InternalKernelNames);

InternalShadersMetal::InternalShadersMetal(DeviceMetal& device)
    : m_Device(device), m_ClearPipelines(device.GetStdAllocator()), m_ResolvePipelines(device.GetStdAllocator()) {
}

InternalShadersMetal::~InternalShadersMetal() {
    for (std::atomic<MTL::ComputePipelineState*>& kernel : m_Kernels) {
        MTL::ComputePipelineState* pipeline = kernel.load(std::memory_order_relaxed);

        if (pipeline)
            pipeline->release();
    }

    for (ClearPipelineMetal& clear : m_ClearPipelines) {
        clear.pipeline->release();
        clear.depthStencil->release();
    }

    for (ResolvePipelineMetal& resolve : m_ResolvePipelines)
        resolve.pipeline->release();

    if (m_DepthOnlyFragmentFunction)
        m_DepthOnlyFragmentFunction->release();

    if (m_Library)
        m_Library->release();
}

Result InternalShadersMetal::Create() {
    AutoreleasePoolMetal autoreleasePool; // "NS::Error" is autoreleased

    // The embedded library is static, no need to copy it
    dispatch_data_t data = dispatch_data_create(g_InternalMetal_metallib, sizeof(g_InternalMetal_metallib), nullptr, ^{
                                                                                                            });

    NS::Error* error = nullptr;
    m_Library = m_Device.GetNativeObject()->newLibrary(data, &error);
    dispatch_release(data);

    if (!m_Library) {
        NRI_REPORT_ERROR(&m_Device, "Internal Metal library loading failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");

        return Result::FAILURE;
    }

    m_DepthOnlyFragmentFunction = NewFunction("nri_depth_only");

    return Result::SUCCESS;
}

// Internal pipelines are built by the device's Metal 4 compiler, an unknown name is reported by pipeline creation
MTL4::FunctionDescriptor* InternalShadersMetal::NewFunction(const char* name) const {
    return NewFunctionDescriptorMetal(m_Library, name);
}

MTL::ComputePipelineState* InternalShadersMetal::GetKernel(InternalKernelMetal kernel) {
    std::atomic<MTL::ComputePipelineState*>& slot = m_Kernels[(size_t)kernel];

    MTL::ComputePipelineState* pipeline = slot.load(std::memory_order_acquire);

    if (pipeline)
        return pipeline;

    std::lock_guard<std::mutex> lock(m_KernelLock);

    pipeline = slot.load(std::memory_order_relaxed);

    if (pipeline)
        return pipeline;

    const char* name = g_InternalKernelNames[(size_t)kernel];

    AutoreleasePoolMetal autoreleasePool; // "NS::Error" is autoreleased

    MTL4::ComputePipelineDescriptor* pipelineDesc = MTL4::ComputePipelineDescriptor::alloc()->init();
    MTL4::FunctionDescriptor* function = NewFunction(name);
    pipelineDesc->setComputeFunctionDescriptor(function);
    function->release();

    NS::Error* error = nullptr;
    pipeline = m_Device.GetCompiler()->newComputePipelineState(pipelineDesc, nullptr, &error);
    pipelineDesc->release();

    if (!pipeline) {
        NRI_REPORT_ERROR(&m_Device, "Internal Metal kernel '%s' creation failed: %s", name, error ? error->localizedDescription()->utf8String() : "unknown error");

        return nullptr;
    }

    slot.store(pipeline, std::memory_order_release);

    return pipeline;
}

static inline bool IsEqual(const ClearPipelineKeyMetal& a, const ClearPipelineKeyMetal& b) {
    bool isEqual = a.colorNum == b.colorNum && a.colorIndex == b.colorIndex && a.sampleNum == b.sampleNum && a.colorType == b.colorType && a.planes == b.planes;

    for (uint32_t i = 0; isEqual && i < a.colorNum; i++)
        isEqual = a.colors[i] == b.colors[i];

    return isEqual;
}

const ClearPipelineMetal* InternalShadersMetal::FindClearPipeline(const ClearPipelineKeyMetal& key) const {
    for (const ClearPipelineMetal& clear : m_ClearPipelines) {
        if (IsEqual(clear.key, key))
            return &clear;
    }

    return nullptr;
}

ClearPipelineMetal InternalShadersMetal::GetClearPipeline(const ClearPipelineKeyMetal& key) {
    {
        std::shared_lock<std::shared_mutex> lock(m_RenderPipelineLock);
        const ClearPipelineMetal* clear = FindClearPipeline(key);

        if (clear)
            return *clear;
    }

    std::unique_lock<std::shared_mutex> lock(m_RenderPipelineLock);
    const ClearPipelineMetal* existing = FindClearPipeline(key);

    if (existing)
        return *existing;

    AutoreleasePoolMetal autoreleasePool; // "NS::Error" is autoreleased

    char name[32] = {};
    snprintf(name, sizeof(name), "nri_clear_fs_%u_%u", key.colorType, key.colorIndex);

    MTL4::FunctionDescriptor* vertex = NewFunction("nri_clear_vs");
    MTL4::FunctionDescriptor* fragment = (key.planes & PlaneBits::COLOR) ? NewFunction(name) : nullptr;

    MTL4::RenderPipelineDescriptor* pipelineDesc = MTL4::RenderPipelineDescriptor::alloc()->init();
    pipelineDesc->setVertexFunctionDescriptor(vertex);
    pipelineDesc->setFragmentFunctionDescriptor(fragment);
    pipelineDesc->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassTriangle);
    pipelineDesc->setRasterSampleCount(key.sampleNum);

    for (uint32_t i = 0; i < key.colorNum; i++) {
        MTL4::RenderPipelineColorAttachmentDescriptor* attachment = pipelineDesc->colorAttachments()->object(i);
        attachment->setPixelFormat(key.colors[i]);
        attachment->setWriteMask(i == key.colorIndex && (key.planes & PlaneBits::COLOR) ? MTL::ColorWriteMaskAll : MTL::ColorWriteMaskNone);
    }

    NS::Error* error = nullptr;
    ClearPipelineMetal clear = {};
    clear.key = key;
    clear.pipeline = m_Device.GetCompiler()->newRenderPipelineState(pipelineDesc, nullptr, &error);

    MTL::StencilDescriptor* stencil = MTL::StencilDescriptor::alloc()->init();
    stencil->setStencilCompareFunction(MTL::CompareFunctionAlways);
    stencil->setDepthStencilPassOperation(key.planes & PlaneBits::STENCIL ? MTL::StencilOperationReplace : MTL::StencilOperationKeep);
    stencil->setWriteMask(key.planes & PlaneBits::STENCIL ? 0xFF : 0);

    MTL::DepthStencilDescriptor* depthStencilDesc = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDesc->setDepthCompareFunction(MTL::CompareFunctionAlways);
    depthStencilDesc->setDepthWriteEnabled(key.planes & PlaneBits::DEPTH);
    depthStencilDesc->setFrontFaceStencil(stencil);
    depthStencilDesc->setBackFaceStencil(stencil);
    clear.depthStencil = m_Device.GetNativeObject()->newDepthStencilState(depthStencilDesc);

    depthStencilDesc->release();
    stencil->release();
    pipelineDesc->release();

    if (fragment)
        fragment->release();

    if (vertex)
        vertex->release();

    if (!clear.pipeline || !clear.depthStencil) {
        if (clear.pipeline)
            clear.pipeline->release();

        if (clear.depthStencil)
            clear.depthStencil->release();

        NRI_REPORT_ERROR(&m_Device, "Internal Metal clear pipeline creation failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");

        return {};
    }

    m_ClearPipelines.push_back(clear);

    return clear;
}

MTL::RenderPipelineState* InternalShadersMetal::FindResolvePipeline(MTL::PixelFormat format, uint8_t colorType, bool isArray) const {
    for (const ResolvePipelineMetal& resolve : m_ResolvePipelines) {
        if (resolve.format == format && resolve.colorType == colorType && resolve.isArray == isArray)
            return resolve.pipeline;
    }

    return nullptr;
}

MTL::RenderPipelineState* InternalShadersMetal::GetResolvePipeline(MTL::PixelFormat format, uint8_t colorType, bool isArray) {
    {
        std::shared_lock<std::shared_mutex> lock(m_RenderPipelineLock);
        MTL::RenderPipelineState* pipeline = FindResolvePipeline(format, colorType, isArray);

        if (pipeline)
            return pipeline;
    }

    std::unique_lock<std::shared_mutex> lock(m_RenderPipelineLock);
    MTL::RenderPipelineState* existing = FindResolvePipeline(format, colorType, isArray);

    if (existing)
        return existing;

    AutoreleasePoolMetal autoreleasePool; // "NS::Error" is autoreleased

    char name[32] = {};
    snprintf(name, sizeof(name), "nri_resolve_%s_%u", isArray ? "array" : "2d", colorType);

    MTL4::FunctionDescriptor* vertex = NewFunction("nri_resolve_vs");
    MTL4::FunctionDescriptor* fragment = NewFunction(name);

    MTL4::RenderPipelineDescriptor* pipelineDesc = MTL4::RenderPipelineDescriptor::alloc()->init();
    pipelineDesc->setVertexFunctionDescriptor(vertex);
    pipelineDesc->setFragmentFunctionDescriptor(fragment);
    pipelineDesc->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassTriangle);
    pipelineDesc->colorAttachments()->object(0)->setPixelFormat(format);

    NS::Error* error = nullptr;
    MTL::RenderPipelineState* pipeline = m_Device.GetCompiler()->newRenderPipelineState(pipelineDesc, nullptr, &error);

    pipelineDesc->release();

    if (fragment)
        fragment->release();

    if (vertex)
        vertex->release();

    if (!pipeline) {
        NRI_REPORT_ERROR(&m_Device, "Internal Metal resolve pipeline creation failed: %s", error ? error->localizedDescription()->utf8String() : "unknown error");

        return nullptr;
    }

    m_ResolvePipelines.push_back({pipeline, format, colorType, isArray});

    return pipeline;
}
