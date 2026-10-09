// © 2026 NVIDIA Corporation

#pragma once

// Keep metal-cpp's private implementation symbols out of the NRI binary's exported symbol table
#define METALCPP_SYMBOL_VISIBILITY_HIDDEN

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <mutex>
#include <shared_mutex>

#if NRI_ENABLE_METAL_SHADER_CONVERTER
#    define IR_RUNTIME_METALCPP
#    define IR_RUNTIME_METAL4
#    include <metal_irconverter/metal_irconverter.h>

// "API_AVAILABLE" makes runtime functions visible (exported)
#    pragma push_macro("API_AVAILABLE")
#    undef API_AVAILABLE
#    define API_AVAILABLE(...)
#    include <metal_irconverter_runtime/metal_irconverter_runtime.h>
#    pragma pop_macro("API_AVAILABLE")
#endif

#include "SharedExternal.h"

namespace nri {

struct DeviceMetal;
struct MemoryMetal;
struct BufferMetal;
struct AccelerationStructureMetal;
struct TextureMetal;
struct QueueMetal;
struct FenceMetal;
struct CommandAllocatorMetal;
struct CommandBufferMetal;
struct DescriptorMetal;
struct DescriptorPoolMetal;
struct DescriptorSetMetal;
struct PipelineLayoutMetal;
struct PipelineMetal;
struct PipelineCacheMetal;
struct QueryPoolMetal;
struct SwapChainMetal;

// Metal factory methods and encoder creation return autoreleased objects. NRI can be called from threads without
// an autorelease pool, so drain them locally on paths that run repeatedly (retain what must outlive the scope)
struct AutoreleasePoolMetal final {
    inline AutoreleasePoolMetal()
        : m_Pool(NS::AutoreleasePool::alloc()->init()) {
    }

    inline ~AutoreleasePoolMetal() {
        m_Pool->release();
    }

private:
    NS::AutoreleasePool* m_Pool;
};

// Metal command queues are untyped, so each supported "QueueType" can be backed by several native queues
constexpr uint32_t QUEUE_TYPE_NUM = 3;
constexpr uint32_t QUEUE_NUM_PER_TYPE = 4;
constexpr uint32_t DRAW_INDIRECT_MAX_NUM = 1 << 16;

// Argument-table slots shared by native and converted shaders (see "NRI.metal"). Converted slots match "kIR*BindPoint"
constexpr uint32_t ARGUMENT_SLOT_RESOURCE_HEAP = 0;
constexpr uint32_t ARGUMENT_SLOT_SAMPLER_HEAP = 1;
constexpr uint32_t ARGUMENT_SLOT_ROOT = 2;
constexpr uint32_t ARGUMENT_SLOT_MULTIVIEW = 3;      // native only
constexpr uint32_t ARGUMENT_SLOT_DRAW_ARGUMENTS = 4; // converted only
constexpr uint32_t ARGUMENT_SLOT_DRAW_UNIFORMS = 5;  // converted only
constexpr uint32_t ARGUMENT_SLOT_VERTEX_BUFFER_BASE = 6;

// Converted vertex shaders read stage-in attributes from "kIRStageInAttributeStartIndex + reflected attribute index"
constexpr uint32_t CONVERTED_VERTEX_ATTRIBUTE_BASE = 11;
constexpr uint32_t CONVERTED_VERTEX_ATTRIBUTE_NUM = 20;
static_assert(CONVERTED_VERTEX_ATTRIBUTE_BASE + CONVERTED_VERTEX_ATTRIBUTE_NUM == 31, "'MTLVertexDescriptor' has 31 attributes");

// Reserved DXIL register spaces
constexpr uint32_t DRAW_EMULATION_SPACE = 999;    // "NRI_BASE_ATTRIBUTES_EMULATION_SPACE" in "NRI.hlsl"
constexpr uint32_t FRAMEBUFFER_FETCH_SPACE = 998; // input attachments

// Texture atomics: 32-bit "R32Uint" and "R32Sint", 64-bit min/max "RG32Uint" (Apple8+)
static inline bool IsAtomicFormat(MTL::Device& device, Format format) {
    if (format == Format::RG32_UINT)
        return device.supportsFamily(MTL::GPUFamilyApple8);

    return format == Format::R32_UINT || format == Format::R32_SINT;
}

// FNV-1a, used for persistent data (pipeline caches and converted shaders)
static inline uint64_t HashMetal(const void* data, size_t size, uint64_t hash = 0xCBF29CE484222325ull) {
    const uint8_t* bytes = (const uint8_t*)data;

    for (size_t i = 0; i < size; i++)
        hash = (hash ^ bytes[i]) * 0x100000001B3ull;

    return hash;
}

// Metal 4 function descriptor for a library function, optionally specialized with function constants and / or renamed
// (a unique name is required for visible functions of different libraries linked into one pipeline). Owned by the caller
static inline MTL4::FunctionDescriptor* NewFunctionDescriptorMetal(MTL::Library* library, const char* name, const MTL::FunctionConstantValues* constants = nullptr, const char* specializedName = nullptr) {
    MTL4::LibraryFunctionDescriptor* function = MTL4::LibraryFunctionDescriptor::alloc()->init();
    function->setLibrary(library);
    function->setName(NS::String::string(name, NS::UTF8StringEncoding));

    if (!constants && !specializedName)
        return function;

    MTL4::SpecializedFunctionDescriptor* specialized = MTL4::SpecializedFunctionDescriptor::alloc()->init();
    specialized->setFunctionDescriptor(function);
    function->release();

    if (constants)
        specialized->setConstantValues(constants);

    if (specializedName)
        specialized->setSpecializedName(NS::String::string(specializedName, NS::UTF8StringEncoding));

    return specialized;
}

} // namespace nri

#include "DeviceMetal.h"
