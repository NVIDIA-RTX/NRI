// © 2021 NVIDIA Corporation

#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
    #define NRI_CALL __stdcall
#else
    #define NRI_CALL
#endif

#ifndef NRI_API
    #if defined(__cplusplus)
        #define NRI_API extern "C"
    #else
        #define NRI_API extern
    #endif
#endif

#ifdef __cplusplus
    #if !defined(NRI_FORCE_C)
        #define NRI_CPP
    #endif
#else
    #include <stdbool.h>
#endif

#include "NRIMacro.h"

// Tips:
// - designated initializers are highly recommended!
// - always zero initialize structs via "{}" if designated initializers are not used (at least to honor "NriOptional")
// - documentation is embedded (more details can be requested by creating a GitHub issue)
// - data types are grouped into collapsible logical blocks via "#pragma region"
// - in function declarations "NriRef" implies a valid object, "NriPtr" means "NULL" is allowed

NriNamespaceBegin

// Entities
NriForwardStruct(Fence);            // a synchronization primitive that can be used to insert a dependency between queue operations or between a queue operation and the host
NriForwardStruct(Queue);            // a logical queue, providing access to a HW queue
NriForwardStruct(Memory);           // a memory blob allocated on DEVICE or HOST
NriForwardStruct(Buffer);           // a buffer object: linear arrays of data
NriForwardStruct(Device);           // a logical device
NriForwardStruct(Texture);          // a texture object: multidimensional arrays of data
NriForwardStruct(Pipeline);         // a collection of state needed for rendering: shaders + fixed
NriForwardStruct(SwapChain);        // an array of presentable images that are associated with a surface
NriForwardStruct(QueryPool);        // a collection of queries of the same type
NriForwardStruct(Descriptor);       // a handle or pointer to a resource (potentially with a header)
NriForwardStruct(CommandBuffer);    // used to record commands which can be subsequently submitted to a device queue for execution (aka command list)
NriForwardStruct(DescriptorSet);    // a continuous set of descriptors
NriForwardStruct(DescriptorPool);   // maintains a pool of descriptors, descriptor sets are allocated from (aka descriptor heap)
NriForwardStruct(PipelineLayout);   // determines the interface between shader stages and shader resources (aka root signature)
NriForwardStruct(CommandAllocator); // an object that command buffer memory is allocated from

// Basic types
typedef uint8_t Nri(Sample_t);
typedef uint16_t Nri(Dim_t);
typedef void Nri(Object);

NriStruct(Uid_t) {
    uint64_t low;
    uint64_t high;
};

NriStruct(Dim2_t) {
    Nri(Dim_t) w, h;
};

NriStruct(Float2_t) {
    float x, y;
};

// Aliases
static const uint32_t NriConstant(BGRA_UNUSED) = 0;     // only for "bgra" color for profiling
static const uint32_t NriConstant(ALL) = 0;             // only for "sampleMask" and "descriptorNum"
static const Nri(Dim_t) NriConstant(WHOLE_SIZE) = 0;    // only for "Dim_t" and "size"
static const Nri(Dim_t) NriConstant(REMAINING) = 0;     // only for "mipNum" and "layerNum"

// Readability
#define NriOptional // i.e. can be 0 (keep an eye on comments)
#define NriOut      // highlights an output argument

// Implicit memory heaps for "CreatePlacedX"
#define NriDeviceHeap 0, 0
#define NriDeviceUploadHeap 0, 1
#define NriHostUploadHeap 0, 2
#define NriHostReadbackHeap 0, 3

//============================================================================================================================================================================================
#pragma region [ Common ]
//============================================================================================================================================================================================

NriEnum(GraphicsAPI, uint8_t,
    NONE,   // Supports everything, does nothing, returns dummy non-NULL objects and ~0-filled descs, available if "NRI_ENABLE_NONE_SUPPORT = ON" in CMake
    D3D11,  // Direct3D 11 (feature set 11.1), available if "NRI_ENABLE_D3D11_SUPPORT = ON" in CMake (https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm)
    D3D12,  // Direct3D 12 (feature set 11.1+), available if "NRI_ENABLE_D3D12_SUPPORT = ON" in CMake (https://microsoft.github.io/DirectX-Specs/)
    VK      // Vulkan 1.3 or 1.2+ (can be used on MacOS via MoltenVK), available if "NRI_ENABLE_VK_SUPPORT = ON" in CMake (https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html)
);

NriEnum(Result, int8_t,
    // All bad, but optionally require an action ("callbackInterface.AbortExecution" is not triggered)
    DEVICE_LOST             = -3, // may be returned by "QueueSubmit*", "*WaitIdle", "AcquireNextTexture", "QueuePresent", "WaitForPresent"
    OUT_OF_DATE             = -2, // VK: swap chain is out of date, can be triggered if "features.resizableSwapChain" is not supported
    INVALID_SDK             = -1, // D3D: some interfaces are missing (potential reasons: unable to load "D3D12Core.dll", version or SDK mismatch)

    // All good
    SUCCESS                 = 0,

    // All bad, most likely a crash or a validation error will happen next ("callbackInterface.AbortExecution" is triggered)
    FAILURE                 = 1,
    INVALID_ARGUMENT        = 2,
    OUT_OF_MEMORY           = 3,
    UNSUPPORTED             = 4   // if enabled, NRI validation can promote some to "INVALID_ARGUMENT"
);

// The viewport origin is top-left (D3D native) by default, but can be changed to bottom-left (VK native)
// https://registry.khronos.org/vulkan/specs/latest/man/html/VkViewport.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_viewport
NriStruct(Viewport) {
    float x;
    float y;
    float width;
    float height;
    float depthMin;
    float depthMax;
    bool originBottomLeft; // expects "features.viewportOriginBottomLeft"
};

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkRect2D.html
NriStruct(Rect) {
    int16_t x;
    int16_t y;
    Nri(Dim_t) width;
    Nri(Dim_t) height;
};

NriStruct(Color32f) {
    float x, y, z, w;
};

NriStruct(Color32ui) {
    uint32_t x, y, z, w;
};

NriStruct(Color32i) {
    int32_t x, y, z, w;
};

NriStruct(DepthStencil) {
    float depth;
    uint8_t stencil;
};

NriUnion(Color) {
    Nri(Color32f) f;
    Nri(Color32ui) ui;
    Nri(Color32i) i;
};

NriUnion(ClearValue) {
    Nri(DepthStencil) depthStencil;
    Nri(Color) color;
};

NriStruct(SampleLocation) {
    int8_t x, y; // [-8; 7]
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Formats ]
//============================================================================================================================================================================================

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkFormat.html
// https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format
// left -> right : low -> high bits
// Expected (but not guaranteed) "FormatSupportBits" are provided, but "GetFormatSupport" should be used for querying real HW support
// To demote sRGB use the previous format, i.e. "format - 1"
//                                                STORAGE_BUFFER_ATOMICS
//                                                      VERTEX_BUFFER  |
//                                                  STORAGE_BUFFER  |  |
//                                                       BUFFER  |  |  |
//                                   STORAGE_TEXTURE_ATOMICS  |  |  |  |
//                                                  BLEND  |  |  |  |  |
//                            DEPTH_STENCIL_ATTACHMENT  |  |  |  |  |  |
//                                 COLOR_ATTACHMENT  |  |  |  |  |  |  |
//                               STORAGE_TEXTURE  |  |  |  |  |  |  |  |
//                                    TEXTURE  |  |  |  |  |  |  |  |  |
//                                          |  |  |  |  |  |  |  |  |  |
//                                          |    FormatSupportBits     |
NriEnum(Format, uint8_t,
    UNKNOWN,                             // -  -  -  -  -  -  -  -  -  -

    // Plain: 8 bits per channel
    R8_UNORM,                            // +  +  +  -  +  -  +  +  +  -
    R8_SNORM,                            // +  +  +  -  +  -  +  +  +  -
    R8_UINT,                             // +  +  +  -  -  -  +  +  +  - // SHADING_RATE compatible, see NRI_SHADING_RATE macro
    R8_SINT,                             // +  +  +  -  -  -  +  +  +  -

    RG8_UNORM,                           // +  +  +  -  +  -  +  +  +  - // "AccelerationStructure" compatible (requires "tiers.rayTracing >= 2")
    RG8_SNORM,                           // +  +  +  -  +  -  +  +  +  - // "AccelerationStructure" compatible (requires "tiers.rayTracing >= 2")
    RG8_UINT,                            // +  +  +  -  -  -  +  +  +  -
    RG8_SINT,                            // +  +  +  -  -  -  +  +  +  -

    BGRA8_UNORM,                         // +  +  +  -  +  -  +  +  +  -
    BGRA8_SRGB,                          // +  -  +  -  +  -  -  -  -  -

    RGBA8_UNORM,                         // +  +  +  -  +  -  +  +  +  - // "AccelerationStructure" compatible (requires "tiers.rayTracing >= 2")
    RGBA8_SRGB,                          // +  -  +  -  +  -  -  -  -  -
    RGBA8_SNORM,                         // +  +  +  -  +  -  +  +  +  - // "AccelerationStructure" compatible (requires "tiers.rayTracing >= 2")
    RGBA8_UINT,                          // +  +  +  -  -  -  +  +  +  -
    RGBA8_SINT,                          // +  +  +  -  -  -  +  +  +  -

    // Plain: 16 bits per channel
    R16_UNORM,                           // +  +  +  -  +  -  +  +  +  -
    R16_SNORM,                           // +  +  +  -  +  -  +  +  +  -
    R16_UINT,                            // +  +  +  -  -  -  +  +  +  -
    R16_SINT,                            // +  +  +  -  -  -  +  +  +  -
    R16_SFLOAT,                          // +  +  +  -  +  -  +  +  +  -

    RG16_UNORM,                          // +  +  +  -  +  -  +  +  +  - // "AccelerationStructure" compatible (requires "tiers.rayTracing >= 2")
    RG16_SNORM,                          // +  +  +  -  +  -  +  +  +  - // "AccelerationStructure" compatible
    RG16_UINT,                           // +  +  +  -  -  -  +  +  +  -
    RG16_SINT,                           // +  +  +  -  -  -  +  +  +  -
    RG16_SFLOAT,                         // +  +  +  -  +  -  +  +  +  - // "AccelerationStructure" compatible

    RGBA16_UNORM,                        // +  +  +  -  +  -  +  +  +  - // "AccelerationStructure" compatible (requires "tiers.rayTracing >= 2")
    RGBA16_SNORM,                        // +  +  +  -  +  -  +  +  +  - // "AccelerationStructure" compatible
    RGBA16_UINT,                         // +  +  +  -  -  -  +  +  +  -
    RGBA16_SINT,                         // +  +  +  -  -  -  +  +  +  -
    RGBA16_SFLOAT,                       // +  +  +  -  +  -  +  +  +  - // "AccelerationStructure" compatible

    // Plain: 32 bits per channel
    R32_UINT,                            // +  +  +  -  -  +  +  +  +  +
    R32_SINT,                            // +  +  +  -  -  +  +  +  +  +
    R32_SFLOAT,                          // +  +  +  -  +  +  +  +  +  +

    RG32_UINT,                           // +  +  +  -  -  -  +  +  +  -
    RG32_SINT,                           // +  +  +  -  -  -  +  +  +  -
    RG32_SFLOAT,                         // +  +  +  -  +  -  +  +  +  - // "AccelerationStructure" compatible

    RGB32_UINT,                          // +  -  -  -  -  -  +  -  +  -
    RGB32_SINT,                          // +  -  -  -  -  -  +  -  +  -
    RGB32_SFLOAT,                        // +  -  -  -  -  -  +  -  +  - // "AccelerationStructure" compatible

    RGBA32_UINT,                         // +  +  +  -  -  -  +  +  +  -
    RGBA32_SINT,                         // +  +  +  -  -  -  +  +  +  -
    RGBA32_SFLOAT,                       // +  +  +  -  +  -  +  +  +  -

    // Packed: 16 bits per pixel
    B5_G6_R5_UNORM,                      // +  -  +  -  +  -  -  -  -  -
    B5_G5_R5_A1_UNORM,                   // +  -  +  -  +  -  -  -  -  -
    B4_G4_R4_A4_UNORM,                   // +  -  +  -  +  -  -  -  -  -

    // Packed: 32 bits per pixel
    R10_G10_B10_A2_UNORM,                // +  +  +  -  +  -  +  +  +  - // "AccelerationStructure" compatible (requires "tiers.rayTracing >= 2")
    R10_G10_B10_A2_UINT,                 // +  +  +  -  -  -  +  +  +  -
    R11_G11_B10_UFLOAT,                  // +  +  +  -  +  -  +  +  +  -
    R9_G9_B9_E5_UFLOAT,                  // +  -  -  -  -  -  -  -  -  -

    // Block-compressed
    BC1_RGBA_UNORM,                      // +  -  -  -  -  -  -  -  -  -
    BC1_RGBA_SRGB,                       // +  -  -  -  -  -  -  -  -  -
    BC2_RGBA_UNORM,                      // +  -  -  -  -  -  -  -  -  -
    BC2_RGBA_SRGB,                       // +  -  -  -  -  -  -  -  -  -
    BC3_RGBA_UNORM,                      // +  -  -  -  -  -  -  -  -  -
    BC3_RGBA_SRGB,                       // +  -  -  -  -  -  -  -  -  -
    BC4_R_UNORM,                         // +  -  -  -  -  -  -  -  -  -
    BC4_R_SNORM,                         // +  -  -  -  -  -  -  -  -  -
    BC5_RG_UNORM,                        // +  -  -  -  -  -  -  -  -  -
    BC5_RG_SNORM,                        // +  -  -  -  -  -  -  -  -  -
    BC6H_RGB_UFLOAT,                     // +  -  -  -  -  -  -  -  -  -
    BC6H_RGB_SFLOAT,                     // +  -  -  -  -  -  -  -  -  -
    BC7_RGBA_UNORM,                      // +  -  -  -  -  -  -  -  -  -
    BC7_RGBA_SRGB,                       // +  -  -  -  -  -  -  -  -  -

    // Depth-stencil
    D16_UNORM,                           // -  -  -  +  -  -  -  -  -  -
    D24_UNORM_S8_UINT,                   // -  -  -  +  -  -  -  -  -  -
    D32_SFLOAT,                          // -  -  -  +  -  -  -  -  -  -
    D32_SFLOAT_S8_UINT_X24,              // -  -  -  +  -  -  -  -  -  -

    // Depth-stencil (SHADER_RESOURCE)
    R24_UNORM_X8,       // .x - depth    // +  -  -  -  -  -  -  -  -  -
    X24_G8_UINT,        // .y - stencil  // +  -  -  -  -  -  -  -  -  -
    R32_SFLOAT_X8_X24,  // .x - depth    // +  -  -  -  -  -  -  -  -  -
    X32_G8_UINT_X24     // .y - stencil  // +  -  -  -  -  -  -  -  -  -
);

// https://learn.microsoft.com/en-us/windows/win32/direct3d12/subresources#plane-slice
// https://registry.khronos.org/vulkan/specs/latest/man/html/VkImageAspectFlagBits.html
NriBits(PlaneBits, uint8_t,
    ALL                             = 0,
    COLOR                           = NriBit(0), // indicates "color" plane (same as "ALL" for color formats)

    // D3D11: can't be addressed individually in "copy" operations
    DEPTH                           = NriBit(1), // indicates "depth" plane (same as "ALL" for depth-only formats)
    STENCIL                         = NriBit(2)  // indicates "stencil" plane in depth-stencil formats
);

// A bit represents a feature, supported by a format
NriBits(FormatSupportBits, uint16_t,
    UNSUPPORTED                     = 0,

    // Texture
    TEXTURE                         = NriBit(0),
    STORAGE_TEXTURE                 = NriBit(1),
    STORAGE_TEXTURE_ATOMICS         = NriBit(2),  // other than Load / Store
    COLOR_ATTACHMENT                = NriBit(3),
    DEPTH_STENCIL_ATTACHMENT        = NriBit(4),
    BLEND                           = NriBit(5),
    MULTISAMPLE_2X                  = NriBit(6),
    MULTISAMPLE_4X                  = NriBit(7),
    MULTISAMPLE_8X                  = NriBit(8),

    // Buffer
    BUFFER                          = NriBit(9),
    STORAGE_BUFFER                  = NriBit(10),
    STORAGE_BUFFER_ATOMICS          = NriBit(11),  // other than Load / Store
    VERTEX_BUFFER                   = NriBit(12),

    // Texture / buffer
    STORAGE_READ_WITHOUT_FORMAT     = NriBit(13),
    STORAGE_WRITE_WITHOUT_FORMAT    = NriBit(14)
);

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Pipeline stages and barriers ]
//============================================================================================================================================================================================

// https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html
// https://docs.vulkan.org/samples/latest/samples/performance/pipeline_barriers/README.html

// A barrier consists of two phases:
// - before (source scope, 1st synchronization scope):
//   - "AccessBits" corresponding with any relevant resource usage since the preceding barrier or the start of "QueueSubmit" scope
//   - "StagesBits" of all preceding GPU work that must be completed before executing the barrier (stages to wait before the barrier)
//   - "Layout" for textures
// - after (destination scope, 2nd synchronization scope):
//   - "AccessBits" corresponding with any relevant resource usage after the barrier completes
//   - "StagesBits" of all subsequent GPU work that must wait until the barrier execution is finished (stages to halt until the barrier is executed)
//   - "Layout" for textures
// If "features.enhancedBarriers" is not supported:
//   - https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#compatibility-with-legacy-d3d12_resource_states
//   - "AccessBits::NONE" gets mapped to "COMMON" (aka "GENERAL" access), leading to potential discrepancies with VK

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineStageFlagBits2.html
// https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#d3d12_barrier_sync
NriBits(StageBits, uint32_t,
    // Special
    ALL                             = 0,          // Lazy default for barriers                          Shader stage
    NONE                            = 0x7FFFFFFF,

    // Graphics                                   // Invoked by "CmdDraw*"
    INDEX_INPUT                     = NriBit(0),  //    Index buffer consumption
    VERTEX_SHADER                   = NriBit(1),  //    Vertex shader                                   X (required within GRAPHICS bind point)
    TESS_CONTROL_SHADER             = NriBit(2),  //    Tessellation control (hull) shader              X
    TESS_EVALUATION_SHADER          = NriBit(3),  //    Tessellation evaluation (domain) shader         X
    GEOMETRY_SHADER                 = NriBit(4),  //    Geometry shader                                 X
    TASK_SHADER                     = NriBit(5),  //    Task (amplification) shader                     X
    MESH_SHADER                     = NriBit(6),  //    Mesh shader                                     X (or required within GRAPHICS bind point)
    FRAGMENT_SHADER                 = NriBit(7),  //    Fragment (pixel) shader                         X
    DEPTH_STENCIL_ATTACHMENT        = NriBit(8),  //    Depth-stencil R/W operations
    COLOR_ATTACHMENT                = NriBit(9),  //    Color R/W operations

    // Compute                                    // Invoked by "CmdDispatch*" (not Rays)
    COMPUTE_SHADER                  = NriBit(10), //    Compute shader                                  X (required within COMPUTE bind point)

    // Ray tracing                                // Invoked by "CmdDispatchRays*"
    RAYGEN_SHADER                   = NriBit(11), //    Ray generation shader                           X (required within RAY_TRACING bind point)
    MISS_SHADER                     = NriBit(12), //    Miss shader                                     X
    INTERSECTION_SHADER             = NriBit(13), //    Intersection shader                             X
    CLOSEST_HIT_SHADER              = NriBit(14), //    Closest hit shader                              X
    ANY_HIT_SHADER                  = NriBit(15), //    Any hit shader                                  X
    CALLABLE_SHADER                 = NriBit(16), //    Callable shader                                 X
    ACCELERATION_STRUCTURE          = NriBit(17), // Invoked by "Cmd*AccelerationStructure*" commands
    MICROMAP                        = NriBit(18), // Invoked by "Cmd*Micromap*" commands

    // Other
    COPY                            = NriBit(19), // Invoked by "CmdCopy*", "CmdUpload*" and "CmdReadback*"
    RESOLVE                         = NriBit(20), // Invoked by "CmdResolveTexture"
    CLEAR_STORAGE                   = NriBit(21), // Invoked by "CmdClearStorage"

    // Modifiers
    INDIRECT                        = NriBit(22), // Invoked by "Indirect" commands (used in addition to other bits)

    // Umbrella stages
    TESSELLATION_SHADERS            = NriMember(StageBits, TESS_CONTROL_SHADER)
                                    | NriMember(StageBits, TESS_EVALUATION_SHADER),

    MESH_SHADERS                    = NriMember(StageBits, TASK_SHADER)
                                    | NriMember(StageBits, MESH_SHADER),

    GRAPHICS_SHADERS                = NriMember(StageBits, VERTEX_SHADER)
                                    | NriMember(StageBits, TESSELLATION_SHADERS)
                                    | NriMember(StageBits, GEOMETRY_SHADER)
                                    | NriMember(StageBits, MESH_SHADERS)
                                    | NriMember(StageBits, FRAGMENT_SHADER),

    RAY_TRACING_SHADERS             = NriMember(StageBits, RAYGEN_SHADER)
                                    | NriMember(StageBits, MISS_SHADER)
                                    | NriMember(StageBits, INTERSECTION_SHADER)
                                    | NriMember(StageBits, CLOSEST_HIT_SHADER)
                                    | NriMember(StageBits, ANY_HIT_SHADER)
                                    | NriMember(StageBits, CALLABLE_SHADER),

    ALL_SHADERS                     = NriMember(StageBits, GRAPHICS_SHADERS)
                                    | NriMember(StageBits, COMPUTE_SHADER)
                                    | NriMember(StageBits, RAY_TRACING_SHADERS),

    GRAPHICS                        = NriMember(StageBits, INDEX_INPUT)
                                    | NriMember(StageBits, GRAPHICS_SHADERS)
                                    | NriMember(StageBits, DEPTH_STENCIL_ATTACHMENT)
                                    | NriMember(StageBits, COLOR_ATTACHMENT)
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkAccessFlagBits2.html
// https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#d3d12_barrier_access
NriBits(AccessBits, uint32_t,
    NONE                            = 0,          // Mapped to "COMMON" (aka "GENERAL" access), if AgilitySDK is not available, leading to potential discrepancies with VK

    // Buffer                                // Access  Compatible "StageBits" (including ALL)
    INDEX_BUFFER                    = NriBit(0),  // R   INDEX_INPUT
    VERTEX_BUFFER                   = NriBit(1),  // R   VERTEX_SHADER
    CONSTANT_BUFFER                 = NriBit(2),  // R   GRAPHICS_SHADERS, COMPUTE_SHADER, RAY_TRACING_SHADERS
    ARGUMENT_BUFFER                 = NriBit(3),  // R   INDIRECT
    SCRATCH_BUFFER                  = NriBit(4),  // RW  ACCELERATION_STRUCTURE, MICROMAP

    // Attachment
    COLOR_ATTACHMENT                = NriBit(5),  // RW  COLOR_ATTACHMENT
    SHADING_RATE_ATTACHMENT         = NriBit(6),  // R   FRAGMENT_SHADER
    DEPTH_STENCIL_ATTACHMENT_READ   = NriBit(7),  // R   DEPTH_STENCIL_ATTACHMENT
    DEPTH_STENCIL_ATTACHMENT_WRITE  = NriBit(8),  //  W  DEPTH_STENCIL_ATTACHMENT

    // Acceleration structure
    ACCELERATION_STRUCTURE_READ     = NriBit(9),  // R   COMPUTE_SHADER, RAY_TRACING_SHADERS, ACCELERATION_STRUCTURE
    ACCELERATION_STRUCTURE_WRITE    = NriBit(10), //  W  ACCELERATION_STRUCTURE

    // Micromap
    MICROMAP_READ                   = NriBit(11), // R   MICROMAP, ACCELERATION_STRUCTURE
    MICROMAP_WRITE                  = NriBit(12), //  W  MICROMAP

    // Shader resource
    SHADER_RESOURCE                 = NriBit(13), // R   GRAPHICS_SHADERS, COMPUTE_SHADER, RAY_TRACING_SHADERS
    SHADER_RESOURCE_STORAGE         = NriBit(14), // RW  GRAPHICS_SHADERS, COMPUTE_SHADER, RAY_TRACING_SHADERS, CLEAR_STORAGE + shaders
    SHADER_BINDING_TABLE            = NriBit(15), // R   RAY_TRACING_SHADERS

    // Copy
    COPY_SOURCE                     = NriBit(16), // R   COPY
    COPY_DESTINATION                = NriBit(17), //  W  COPY

    // Resolve
    RESOLVE_SOURCE                  = NriBit(18), // R   RESOLVE
    RESOLVE_DESTINATION             = NriBit(19), //  W  RESOLVE

    // Clear storage
    CLEAR_STORAGE                   = NriBit(20)  //  W  CLEAR_STORAGE
);

// "Layout" is ignored if "features.enhancedBarriers" is not supported
// https://registry.khronos.org/vulkan/specs/latest/man/html/VkImageLayout.html
// https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#d3d12_barrier_layout
NriEnum(Layout, uint8_t,    // Compatible "AccessBits":
    // Special
    UNDEFINED,                  // https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#d3d12_barrier_layout_undefined
    GENERAL,                    // ~ALL access, but potentially not optimal (required for "SharingMode::SIMULTANEOUS")
    PRESENT,                    // NONE

    // Access specific
    COLOR_ATTACHMENT,           // COLOR_ATTACHMENT
    SHADING_RATE_ATTACHMENT,    // SHADING_RATE_ATTACHMENT
    DEPTH_STENCIL_ATTACHMENT,   // DEPTH_STENCIL_ATTACHMENT_WRITE
    DEPTH_STENCIL_READONLY,     // DEPTH_STENCIL_ATTACHMENT_READ, SHADER_RESOURCE
    SHADER_RESOURCE,            // SHADER_RESOURCE
    SHADER_RESOURCE_STORAGE,    // SHADER_RESOURCE_STORAGE
    COPY_SOURCE,                // COPY_SOURCE
    COPY_DESTINATION,           // COPY_DESTINATION
    RESOLVE_SOURCE,             // RESOLVE_SOURCE
    RESOLVE_DESTINATION         // RESOLVE_DESTINATION
);

NriStruct(AccessStage) {
    Nri(AccessBits) access;
    Nri(StageBits) stages;
};

NriStruct(AccessLayoutStage) {
    Nri(AccessBits) access;
    Nri(Layout) layout;
    Nri(StageBits) stages;
};

NriStruct(GlobalBarrierDesc) {
    Nri(AccessStage) before;
    Nri(AccessStage) after;
};

NriStruct(BufferBarrierDesc) {
    NriPtr(Buffer) buffer;  // use "GetAccelerationStructureBuffer" and "GetMicromapBuffer" for related barriers
    Nri(AccessStage) before;
    Nri(AccessStage) after;
};

NriStruct(TextureBarrierDesc) {
    NriPtr(Texture) texture;
    Nri(AccessLayoutStage) before;
    Nri(AccessLayoutStage) after;
    Nri(Dim_t) mipOffset;
    Nri(Dim_t) mipNum;      // can be "REMAINING"
    Nri(Dim_t) layerOffset;
    Nri(Dim_t) layerNum;    // can be "REMAINING"
    Nri(PlaneBits) planes;

    // Queue ownership transfer is potentially needed only for "SharingMode::EXCLUSIVE" textures
    // https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#synchronization-queue-transfers
    NriOptional NriPtr(Queue) srcQueue;
    NriOptional NriPtr(Queue) dstQueue;
};

NriStruct(BarrierDesc) {
    const NriPtr(GlobalBarrierDesc) globals;
    uint32_t globalNum;
    const NriPtr(BufferBarrierDesc) buffers;
    uint32_t bufferNum;
    const NriPtr(TextureBarrierDesc) textures;
    uint32_t textureNum;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Resources: creation ]
//============================================================================================================================================================================================

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkImageType.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_dimension
NriEnum(TextureType, uint8_t,
    TEXTURE_1D,
    TEXTURE_2D,
    TEXTURE_3D
);

// NRI tries to ease your life and avoid using "queue ownership transfers" (see "TextureBarrierDesc").
// In most of cases "SharingMode" can be ignored. Where is it needed?
// - VK: use "EXCLUSIVE" for attachments participating into multi-queue activities to preserve DCC (Delta Color Compression) on some HW
// - D3D12: use "SIMULTANEOUS" to concurrently use a texture as a "SHADER_RESOURCE" (or "SHADER_RESOURCE_STORAGE") and as a "COPY_DESTINATION" for non overlapping texture regions
// https://registry.khronos.org/vulkan/specs/latest/man/html/VkSharingMode.html
NriEnum(SharingMode, uint8_t,
    CONCURRENT,     // VK: lazy default to avoid dealing with "queue ownership transfers", auto-optimized to "EXCLUSIVE" if all queues have the same type
    EXCLUSIVE,      // VK: may be used for attachments to preserve DCC on some HW in the cost of making a "queue ownership transfer"

    // https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#single-queue-simultaneous-access
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_flags
    SIMULTANEOUS    // D3D12: strengthened variant of "CONCURRENT", allowing simultaneous multiple readers and one writer for a texture (requires "Layout::GENERAL")
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkImageUsageFlagBits.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_flags
NriBits(TextureUsageBits, uint8_t,                 // Min compatible access:                  Usage:
    NONE                                = 0,
    SHADER_RESOURCE                     = NriBit(0),  // SHADER_RESOURCE                         Read-only shader resource (SRV)
    SHADER_RESOURCE_STORAGE             = NriBit(1),  // SHADER_RESOURCE_STORAGE                 Read/write shader resource (UAV)
    COLOR_ATTACHMENT                    = NriBit(2),  // COLOR_ATTACHMENT                        Color attachment (render target)
    DEPTH_STENCIL_ATTACHMENT            = NriBit(3),  // DEPTH_STENCIL_ATTACHMENT_READ/WRITE     Depth-stencil attachment (depth-stencil target)
    SHADING_RATE_ATTACHMENT             = NriBit(4)   // SHADING_RATE_ATTACHMENT                 Shading rate attachment (source)
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkBufferUsageFlagBits.html
NriBits(BufferUsageBits, uint16_t,                 // Min compatible access:                  Usage:
    NONE                                = 0,
    SHADER_RESOURCE                     = NriBit(0),  // SHADER_RESOURCE                         Read-only shader resource (SRV)
    SHADER_RESOURCE_STORAGE             = NriBit(1),  // SHADER_RESOURCE_STORAGE                 Read/write shader resource (UAV)
    VERTEX_BUFFER                       = NriBit(2),  // VERTEX_BUFFER                           Vertex buffer
    INDEX_BUFFER                        = NriBit(3),  // INDEX_BUFFER                            Index buffer
    CONSTANT_BUFFER                     = NriBit(4),  // CONSTANT_BUFFER                         Constant buffer (D3D11: can't be combined with other usages)
    ARGUMENT_BUFFER                     = NriBit(5),  // ARGUMENT_BUFFER                         Argument buffer in "Indirect" commands
    SCRATCH_BUFFER                      = NriBit(6),  // SCRATCH_BUFFER                          Scratch buffer in "CmdBuild*" commands
    SHADER_BINDING_TABLE                = NriBit(7),  // SHADER_BINDING_TABLE                    Shader binding table (SBT) in "CmdDispatchRays*" commands
    ACCELERATION_STRUCTURE_BUILD_INPUT  = NriBit(8),  // SHADER_RESOURCE                         Read-only input in "CmdBuildAccelerationStructures" command
    ACCELERATION_STRUCTURE_STORAGE      = NriBit(9),  // ACCELERATION_STRUCTURE_READ/WRITE       (INTERNAL) acceleration structure storage
    MICROMAP_BUILD_INPUT                = NriBit(10), // SHADER_RESOURCE                         Read-only input in "CmdBuildMicromaps" command
    MICROMAP_STORAGE                    = NriBit(11)  // MICROMAP_READ/WRITE                     (INTERNAL) micromap storage
);

NriStruct(TextureDesc) {
    Nri(TextureType) type;
    Nri(TextureUsageBits) usage;
    Nri(Format) format;
    Nri(Dim_t) width;
    NriOptional Nri(Dim_t) height;
    NriOptional Nri(Dim_t) depth;
    NriOptional Nri(Dim_t) mipNum;
    NriOptional Nri(Dim_t) layerNum;
    NriOptional Nri(Sample_t) sampleNum;
    NriOptional Nri(SharingMode) sharingMode;
    NriOptional Nri(ClearValue) optimizedClearValue; // D3D12: not needed on desktop, since any HW can track many clear values
};

// "structureStride" values:
// 0  = allows "typed" views
// 4  = allows "typed", "byte address" (raw) and "structured" views (D3D11: allows to create multiple "structured" views for a single resource, disobeying the spec)
// >4 = allows "structured" and potentially "typed" views (D3D11: locks this buffer to a single "structured" layout, no "typed" views)
// VK: buffers always created with sharing mode "CONCURRENT" to match D3D12 spec
NriStruct(BufferDesc) {
    uint64_t size;
    uint32_t structureStride;
    Nri(BufferUsageBits) usage;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Resources: binding to memory ]
//============================================================================================================================================================================================

// Contains some encoded implementation specific details
typedef uint32_t Nri(MemoryType);

// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_heap_type
NriEnum(MemoryLocation, uint8_t,
    DEVICE,
    DEVICE_UPLOAD, // soft fallback to "HOST_UPLOAD" if "deviceUploadHeapSize = 0"
    HOST_UPLOAD,
    HOST_READBACK
);

// Memory requirements for a resource (buffer or texture)
NriStruct(MemoryDesc) {
    uint64_t size;
    uint32_t alignment;
    Nri(MemoryType) type;
    bool mustBeDedicated; // must be put into a dedicated "Memory" object, containing only 1 object with offset = 0
};

// A group of non-dedicated "MemoryDesc"s of the SAME "MemoryType" can be merged into a single memory allocation
NriStruct(AllocateMemoryDesc) {
    uint64_t size;
    Nri(MemoryType) type;

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/residency
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_residency_priority
    // https://registry.khronos.org/vulkan/specs/latest/man/html/VkMemoryPriorityAllocateInfoEXT.html
    float priority; // [-1; 1]: low < 0, normal = 0, high > 0

    // Memory allocation goes through "AMD Virtual Memory Allocator" (most likely a sub-allocation from a larger allocation)
    // https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    // https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator
    bool useVMA;
};

// Binding resources to a memory (resources can overlap, i.e. alias)
NriStruct(BindBufferMemoryDesc) {
    NriPtr(Buffer) buffer;
    NriPtr(Memory) memory;
    uint64_t offset; // in memory
};

NriStruct(BindTextureMemoryDesc) {
    NriPtr(Texture) texture;
    NriPtr(Memory) memory;
    uint64_t offset; // in memory
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Resource view or sampler creation (descriptor) ]
//============================================================================================================================================================================================

// https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#creating-descriptors
NriEnum(Texture1DViewType, uint8_t,
    SHADER_RESOURCE_1D,
    SHADER_RESOURCE_1D_ARRAY,
    SHADER_RESOURCE_STORAGE_1D,
    SHADER_RESOURCE_STORAGE_1D_ARRAY,
    COLOR_ATTACHMENT,
    DEPTH_STENCIL_ATTACHMENT,
    DEPTH_READONLY_STENCIL_ATTACHMENT,
    DEPTH_ATTACHMENT_STENCIL_READONLY,
    DEPTH_STENCIL_READONLY
);

NriEnum(Texture2DViewType, uint8_t,
    SHADER_RESOURCE_2D,
    SHADER_RESOURCE_2D_ARRAY,
    SHADER_RESOURCE_CUBE,
    SHADER_RESOURCE_CUBE_ARRAY,
    SHADER_RESOURCE_STORAGE_2D,
    SHADER_RESOURCE_STORAGE_2D_ARRAY,
    COLOR_ATTACHMENT,
    DEPTH_STENCIL_ATTACHMENT,
    DEPTH_READONLY_STENCIL_ATTACHMENT,
    DEPTH_ATTACHMENT_STENCIL_READONLY,
    DEPTH_STENCIL_READONLY,
    SHADING_RATE_ATTACHMENT
);

NriEnum(Texture3DViewType, uint8_t,
    SHADER_RESOURCE_3D,
    SHADER_RESOURCE_STORAGE_3D,
    COLOR_ATTACHMENT
);

NriEnum(BufferViewType, uint8_t,
    SHADER_RESOURCE,
    SHADER_RESOURCE_STORAGE,
    CONSTANT
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkFilter.html
// https://registry.khronos.org/vulkan/specs/latest/man/html/VkSamplerMipmapMode.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_filter
NriEnum(Filter, uint8_t,
    NEAREST,
    LINEAR
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkSamplerReductionMode.html
NriEnum(ReductionMode, uint8_t,
    AVERAGE,    // a weighted average (sum) of values in the footprint (default)
    MIN,        // a component-wise minimum of values in the footprint with non-zero weights
    MAX         // a component-wise maximum of values in the footprint with non-zero weights
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkSamplerAddressMode.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_texture_address_mode
NriEnum(AddressMode, uint8_t,
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER,
    MIRROR_CLAMP_TO_EDGE
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkCompareOp.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_comparison_func
// R - fragment depth, stencil reference or "SampleCmp" reference
// D - depth or stencil buffer
NriEnum(CompareOp, uint8_t,
    NONE,                       // test is disabled
    ALWAYS,                     // true
    NEVER,                      // false
    EQUAL,                      // R == D
    NOT_EQUAL,                  // R != D
    LESS,                       // R < D
    LESS_EQUAL,                 // R <= D
    GREATER,                    // R > D
    GREATER_EQUAL               // R >= D
);

NriStruct(Texture1DViewDesc) {
    const NriPtr(Texture) texture;
    Nri(Texture1DViewType) viewType;
    Nri(Format) format;
    Nri(Dim_t) mipOffset;
    Nri(Dim_t) mipNum;          // can be "REMAINING"
    Nri(Dim_t) layerOffset;
    Nri(Dim_t) layerNum;        // can be "REMAINING"
};

NriStruct(Texture2DViewDesc) {
    const NriPtr(Texture) texture;
    Nri(Texture2DViewType) viewType;
    Nri(Format) format;
    Nri(Dim_t) mipOffset;
    Nri(Dim_t) mipNum;          // can be "REMAINING"
    Nri(Dim_t) layerOffset;
    Nri(Dim_t) layerNum;        // can be "REMAINING"
};

NriStruct(Texture3DViewDesc) {
    const NriPtr(Texture) texture;
    Nri(Texture3DViewType) viewType;
    Nri(Format) format;
    Nri(Dim_t) mipOffset;
    Nri(Dim_t) mipNum;          // can be "REMAINING"
    Nri(Dim_t) sliceOffset;
    Nri(Dim_t) sliceNum;        // can be "REMAINING"
};

NriStruct(BufferViewDesc) {
    const NriPtr(Buffer) buffer;
    Nri(BufferViewType) viewType;
    Nri(Format) format;
    uint64_t offset;            // expects "memoryAlignment.bufferShaderResourceOffset" for shader resources
    uint64_t size;              // can be "WHOLE_SIZE"
    NriOptional uint32_t structureStride; // = structure stride from "BufferDesc" if not provided
};

NriStruct(AddressModes) {
    Nri(AddressMode) u, v, w;
};

NriStruct(Filters) {
    Nri(Filter) min, mag, mip;
    Nri(ReductionMode) ext;     // requires "features.textureFilterMinMax"
};

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkSamplerCreateInfo.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_sampler_desc
NriStruct(SamplerDesc) {
    Nri(Filters) filters;
    uint8_t anisotropy;
    float mipBias;
    float mipMin;
    float mipMax;
    Nri(AddressModes) addressModes;
    Nri(CompareOp) compareOp;
    Nri(Color) borderColor;
    bool isInteger;
    bool unnormalizedCoordinates; // requires "shaderFeatures.unnormalizedCoordinates"
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Pipeline layout and descriptors management ]
//============================================================================================================================================================================================

/*
All indices are local in the currently bound pipeline layout.

Pipeline layout example:
    Descriptor set                  #0          // "setIndex" - a descriptor set index in the pipeline layout, provided as an argument or bound to the pipeline
        Descriptor range                #0      // "rangeIndex" - a descriptor range index in the descriptor set
            Descriptor                      #0  // "descriptorIndex" and "baseDescriptor" - a descriptor (base) index in the descriptor range, i.e. sub-range start
            Descriptor                      #1
            Descriptor                      #2
        Descriptor range                #1
            Descriptor                      #0
            Descriptor                      #1

    Descriptor set                  #1
        Descriptor range                #0
            Descriptor                      #0

    Descriptor set                  #2
        Descriptor range                #0
            Descriptor                      #0
            Descriptor                      #1
            Descriptor                      #2
        Descriptor range                #1
            Descriptor                      #0
            Descriptor                      #1
        Descriptor range                #2
            Descriptor                      #0

    RootConstantDesc                #0          // "rootConstantIndex" - an index in "rootConstants" in the currently bound pipeline layout

    RootDescriptorDesc              #0          // "rootDescriptorIndex" - an index in "rootDescriptors" in the currently bound pipeline layout
    RootDescriptorDesc              #1
*/

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineBindPoint.html
NriEnum(BindPoint, uint8_t,
    INHERIT,                                    // inherit from the last "CmdSetPipelineLayout" call
    GRAPHICS,
    COMPUTE,
    RAY_TRACING
);

NriBits(PipelineLayoutBits, uint8_t,
    NONE                                    = 0,
    IGNORE_GLOBAL_SPIRV_OFFSETS             = NriBit(0), // VK: ignore "DeviceCreationDesc::vkBindingOffsets"
    ENABLE_D3D12_DRAW_PARAMETERS_EMULATION  = NriBit(1),  // D3D12: enable draw parameters emulation, not needed if all vertex shaders for this layout compiled with SM 6.8 (native support)

    // https://github.com/Microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst#resourcedescriptorheaps--samplerdescriptorheaps
    // Default VK bindings (0) can be changed via "-fvk-bind-sampler-heap" and "-fvk-bind-resource-heap" DXC options
    SAMPLER_HEAP_DIRECTLY_INDEXED           = NriBit(2), // requires "shaderModel >= 66"
    RESOURCE_HEAP_DIRECTLY_INDEXED          = NriBit(3)  // requires "shaderModel >= 66"
);

NriBits(DescriptorPoolBits, uint8_t,
    NONE                                    = 0,
    ALLOW_UPDATE_AFTER_SET                  = NriBit(0)  // allows "DescriptorSetBits::ALLOW_UPDATE_AFTER_SET"
);

NriBits(DescriptorSetBits, uint8_t,
    NONE                                    = 0,
    ALLOW_UPDATE_AFTER_SET                  = NriBit(0)  // allows "DescriptorRangeBits::ALLOW_UPDATE_AFTER_SET"
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkDescriptorBindingFlagBits.html
NriBits(DescriptorRangeBits, uint8_t,
    NONE                                    = 0,
    PARTIALLY_BOUND                         = NriBit(0), // descriptors in range may not contain valid descriptors at the time the descriptors are consumed (but referenced descriptors must be valid)
    ARRAY                                   = NriBit(1), // descriptors in range are organized into an array
    VARIABLE_SIZED_ARRAY                    = NriBit(2), // descriptors in range are organized into a variable-sized array, which size is specified via "variableDescriptorNum" argument of "AllocateDescriptorSets" function

    // https://docs.vulkan.org/samples/latest/samples/extensions/descriptor_indexing/README.html#_update_after_bind_streaming_descriptors_concurrently
    ALLOW_UPDATE_AFTER_SET                  = NriBit(3), // descriptors in range can be updated after "CmdSetDescriptorSet" but before "QueueSubmit", also works as "DATA_VOLATILE"

    // https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_mutable_descriptor_type.html
    MUTABLE                                 = NriBit(4)  // descriptors in range may have various non-sampler types (aka "resource heap" concept)
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkDescriptorType.html
NriEnum(DescriptorType, uint8_t,
    SAMPLER,
    CONSTANT_BUFFER,
    TEXTURE,
    STORAGE_TEXTURE,
    BUFFER,
    STORAGE_BUFFER,
    STRUCTURED_BUFFER,
    STORAGE_STRUCTURED_BUFFER,
    ACCELERATION_STRUCTURE
);

// "DescriptorRange" consists of "Descriptor" entities
NriStruct(DescriptorRangeDesc) {
    uint32_t baseRegisterIndex;
    uint32_t descriptorNum;             // treated as max size if "VARIABLE_SIZED_ARRAY" flag is set
    Nri(DescriptorType) descriptorType;
    Nri(StageBits) shaderStages;
    Nri(DescriptorRangeBits) flags;
};

// "DescriptorSet" consists of "DescriptorRange" entities
NriStruct(DescriptorSetDesc) {
    uint32_t registerSpace;             // must be unique, avoid big gaps
    const NriPtr(DescriptorRangeDesc) ranges;
    uint32_t rangeNum;
    Nri(DescriptorSetBits) flags;
};

// "PipelineLayout" consists of "DescriptorSet" descriptions and root parameters
NriStruct(RootConstantDesc) {           // aka push constants block
    uint32_t registerIndex;
    uint32_t size;
    Nri(StageBits) shaderStages;
};

NriStruct(RootDescriptorDesc) {         // aka push descriptor
    uint32_t registerIndex;
    Nri(DescriptorType) descriptorType; // CONSTANT_BUFFER, STRUCTURED_BUFFER or STORAGE_STRUCTURED_BUFFER
    Nri(StageBits) shaderStages;
};

// https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signature-limits#static-samplers
NriStruct(RootSamplerDesc) {            // aka static (immutable) sampler
    uint32_t registerIndex;
    Nri(SamplerDesc) desc;
    Nri(StageBits) shaderStages;
};

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineLayoutCreateInfo.html
// https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#root-signature
// https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#root-signature-version-11
NriStruct(PipelineLayoutDesc) {
    uint32_t rootRegisterSpace;         // must be unique, avoid big gaps
    const NriPtr(RootConstantDesc) rootConstants;
    uint32_t rootConstantNum;
    const NriPtr(RootDescriptorDesc) rootDescriptors;
    uint32_t rootDescriptorNum;
    const NriPtr(RootSamplerDesc) rootSamplers;
    uint32_t rootSamplerNum;
    const NriPtr(DescriptorSetDesc) descriptorSets;
    uint32_t descriptorSetNum;
    Nri(StageBits) shaderStages;
    Nri(PipelineLayoutBits) flags;
};

// Descriptor pool
// https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptor-heaps
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_descriptor_heap_desc
// https://registry.khronos.org/vulkan/specs/latest/man/html/VkDescriptorPoolCreateInfo.html
NriStruct(DescriptorPoolDesc) {
    // Maximum number of descriptor sets that can be allocated from this pool
    uint32_t descriptorSetMaxNum;

    // Sampler heap:
    // - can be directly indexed in shaders if flag "SAMPLER_HEAP_DIRECTLY_INDEXED" is set
    // - root samplers do not count (not allocated from a descriptor pool)
    uint32_t samplerMaxNum;                 // number of "SAMPLER" descriptors

    // Resource heap:
    // - can be directly indexed in shaders if flag "RESOURCE_HEAP_DIRECTLY_INDEXED" is set
    // - a mutable descriptor is a proxy "union" descriptor for all resource descriptors
    // - a mutable descriptor must "mutate" to any non-mutable resource descriptor (i.e. non-sampler) via "UpdateDescriptorRanges" or "CopyDescriptorRanges"
    uint32_t constantBufferMaxNum;          // number of "CONSTANT_BUFFER" descriptors
    uint32_t textureMaxNum;                 // number of "TEXTURE" descriptors
    uint32_t storageTextureMaxNum;          // number of "STORAGE_TEXTURE" descriptors
    uint32_t bufferMaxNum;                  // number of "BUFFER" descriptors
    uint32_t storageBufferMaxNum;           // number of "STORAGE_BUFFER" descriptors
    uint32_t structuredBufferMaxNum;        // number of "STRUCTURED_BUFFER" descriptors
    uint32_t storageStructuredBufferMaxNum; // number of "STORAGE_STRUCTURED_BUFFER" descriptors
    uint32_t accelerationStructureMaxNum;   // number of "ACCELERATION_STRUCTURE" descriptors, requires "features.rayTracing"
    uint32_t mutableMaxNum;                 // number of descriptors for "DescriptorRangeBits::MUTABLE", requires "features.mutableDescriptorType"

    Nri(DescriptorPoolBits) flags;
};

// Updating/initializing descriptors in a descriptor set
NriStruct(UpdateDescriptorRangeDesc) {
    // Destination
    NriPtr(DescriptorSet) descriptorSet;
    uint32_t rangeIndex;
    uint32_t baseDescriptor;
    // Source & count
    const NriPtr(Descriptor) const* descriptors;
    uint32_t descriptorNum;
};

// Copying descriptors between descriptor sets
NriStruct(CopyDescriptorRangeDesc) {
    // Destination
    NriPtr(DescriptorSet) dstDescriptorSet;
    uint32_t dstRangeIndex;
    uint32_t dstBaseDescriptor;
    // Source & count
    const NriPtr(DescriptorSet) srcDescriptorSet;
    uint32_t srcRangeIndex;
    uint32_t srcBaseDescriptor;
    uint32_t descriptorNum;         // can be "ALL" (source)
};

// Binding
NriStruct(SetDescriptorSetDesc) {
    uint32_t setIndex;
    const NriPtr(DescriptorSet) descriptorSet;
    NriOptional Nri(BindPoint) bindPoint;
};

NriStruct(SetRootConstantsDesc) {  // requires "pipelineLayoutRootConstantMaxSize > 0"
    uint32_t rootConstantIndex;
    const void* data;
    uint32_t size;
    uint32_t offset;               // requires "features.rootConstantsOffset"
    NriOptional Nri(BindPoint) bindPoint;
};

NriStruct(SetRootDescriptorDesc) { // requires "pipelineLayoutRootDescriptorMaxNum > 0"
    uint32_t rootDescriptorIndex;
    NriPtr(Descriptor) descriptor;
    uint32_t offset;               // a non-"CONSTANT_BUFFER" descriptor requires "features.nonConstantBufferRootDescriptorOffset"
    NriOptional Nri(BindPoint) bindPoint;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Graphics pipeline: input assembly ]
//============================================================================================================================================================================================

NriEnum(IndexType, uint8_t,
    UINT16,
    UINT32
);

NriEnum(PrimitiveRestart, uint8_t,
    DISABLED,
    INDICES_UINT16, // index "0xFFFF" enforces primitive restart
    INDICES_UINT32  // index "0xFFFFFFFF" enforces primitive restart
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkVertexInputRate.html
NriEnum(VertexStreamStepRate, uint8_t,
    PER_VERTEX,
    PER_INSTANCE
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkPrimitiveTopology.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_primitive_topology
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_primitive_topology_type
NriEnum(Topology, uint8_t,
    POINT_LIST,
    LINE_LIST,
    LINE_STRIP,
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
    LINE_LIST_WITH_ADJACENCY,
    LINE_STRIP_WITH_ADJACENCY,
    TRIANGLE_LIST_WITH_ADJACENCY,
    TRIANGLE_STRIP_WITH_ADJACENCY,
    PATCH_LIST
);

NriStruct(InputAssemblyDesc) {
    Nri(Topology) topology;
    uint8_t tessControlPointNum;
    Nri(PrimitiveRestart) primitiveRestart;
};

NriStruct(VertexAttributeD3D) {
    const char* semanticName;
    uint32_t semanticIndex;
};

NriStruct(VertexAttributeVK) {
    uint32_t location;
};

NriStruct(VertexAttributeDesc) {
    Nri(VertexAttributeD3D) d3d;
    Nri(VertexAttributeVK) vk;
    uint32_t offset;
    Nri(Format) format;
    uint16_t streamIndex;
};

NriStruct(VertexStreamDesc) {
    uint16_t bindingSlot;
    Nri(VertexStreamStepRate) stepRate;
};

NriStruct(VertexInputDesc) {
    const NriPtr(VertexAttributeDesc) attributes;
    uint8_t attributeNum;
    const NriPtr(VertexStreamDesc) streams;
    uint8_t streamNum;
};

NriStruct(VertexBufferDesc) {
    const NriPtr(Buffer) buffer;
    uint64_t offset;
    uint32_t stride;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Graphics pipeline: rasterization ]
//============================================================================================================================================================================================

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkPolygonMode.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_fill_mode
NriEnum(FillMode, uint8_t,
    SOLID,
    WIREFRAME
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkCullModeFlagBits.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_cull_mode
NriEnum(CullMode, uint8_t,
    NONE,
    FRONT,
    BACK
);

// https://docs.vulkan.org/samples/latest/samples/extensions/fragment_shading_rate_dynamic/README.html
// https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html
NriEnum(ShadingRate, uint8_t,
    FRAGMENT_SIZE_1X1,
    FRAGMENT_SIZE_1X2,
    FRAGMENT_SIZE_2X1,
    FRAGMENT_SIZE_2X2,

    // Require "features.additionalShadingRates"
    FRAGMENT_SIZE_2X4,
    FRAGMENT_SIZE_4X2,
    FRAGMENT_SIZE_4X4
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkFragmentShadingRateCombinerOpKHR.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_shading_rate_combiner
//    "primitiveCombiner"      "attachmentCombiner"
// A   Pipeline shading rate    Result of Op1
// B   Primitive shading rate   Attachment shading rate
NriEnum(ShadingRateCombiner, uint8_t,
    KEEP,       // A
    REPLACE,    // B
    MIN,        // min(A, B)
    MAX,        // max(A, B)
    SUM         // (A + B) or (A * B)
);

/*
https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#primsrast-depthbias-computation
https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage-depth-bias
R - minimum resolvable difference
S - maximum slope

bias = constant * R + slopeFactor * S
if (clamp > 0)
    bias = min(bias, clamp)
else if (clamp < 0)
    bias = max(bias, clamp)

enabled if constant != 0 or slope != 0
*/
NriStruct(DepthBiasDesc) {
    float constant;
    float clamp;
    float slope;
};

NriStruct(RasterizationDesc) {
    Nri(DepthBiasDesc) depthBias;
    Nri(FillMode) fillMode;
    Nri(CullMode) cullMode;
    bool frontCounterClockwise;
    bool depthClamp;
    bool lineSmoothing;         // requires "features.lineSmoothing"
    bool conservativeRaster;    // requires "tiers.conservativeRaster != 0"
    bool shadingRate;           // requires "tiers.shadingRate != 0", expects "CmdSetShadingRate" and optionally "AttachmentsDesc::shadingRate"
};

NriStruct(MultisampleDesc) {
    uint32_t sampleMask;        // can be "ALL"
    Nri(Sample_t) sampleNum;
    bool alphaToCoverage;
    bool sampleLocations;       // requires "tiers.sampleLocations != 0", expects "CmdSetSampleLocations"
};

NriStruct(ShadingRateDesc) {
    Nri(ShadingRate) shadingRate;
    Nri(ShadingRateCombiner) primitiveCombiner;  // requires "tiers.sampleLocations >= 2"
    Nri(ShadingRateCombiner) attachmentCombiner; // requires "tiers.sampleLocations >= 2"
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Graphics pipeline: output merger ]
//============================================================================================================================================================================================

NriEnum(Multiview, uint8_t,
    // Destination "viewport" and/or "layer" must be set in shaders explicitly, "viewMask" for rendering can be < than the one used for pipeline creation (D3D12 style)
    FLEXIBLE,                   // requires "features.flexibleMultiview"

    // View instances go to statically assigned corresponding attachment layers, "viewMask" for rendering must match the one used for pipeline creation (VK style)
    LAYER_BASED,                // requires "features.layerBasedMultiview"

    // View instances go to statically assigned corresponding viewports, "viewMask" for pipeline creation is unused (D3D11 style)
    VIEWPORT_BASED              // requires "features.viewportBasedMultiview"
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkLogicOp.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_logic_op
// S - source color 0
// D - destination color
NriEnum(LogicOp, uint8_t,
    NONE,
    CLEAR,                      // 0
    AND,                        // S & D
    AND_REVERSE,                // S & ~D
    COPY,                       // S
    AND_INVERTED,               // ~S & D
    XOR,                        // S ^ D
    OR,                         // S | D
    NOR,                        // ~(S | D)
    EQUIVALENT,                 // ~(S ^ D)
    INVERT,                     // ~D
    OR_REVERSE,                 // S | ~D
    COPY_INVERTED,              // ~S
    OR_INVERTED,                // ~S | D
    NAND,                       // ~(S & D)
    SET                         // 1
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkStencilOp.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_stencil_op
// R - reference, set by "CmdSetStencilReference"
// D - stencil buffer
NriEnum(StencilOp, uint8_t,
    KEEP,                       // D = D
    ZERO,                       // D = 0
    REPLACE,                    // D = R
    INCREMENT_AND_CLAMP,        // D = min(D++, 255)
    DECREMENT_AND_CLAMP,        // D = max(D--, 0)
    INVERT,                     // D = ~D
    INCREMENT_AND_WRAP,         // D++
    DECREMENT_AND_WRAP          // D--
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkBlendFactor.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_blend
// S0 - source color 0
// S1 - source color 1
// D - destination color
// C - blend constants, set by "CmdSetBlendConstants"
NriEnum(BlendFactor, uint8_t,   // RGB                               ALPHA
    ZERO,                       // 0                                 0
    ONE,                        // 1                                 1
    SRC_COLOR,                  // S0.r, S0.g, S0.b                  S0.a
    ONE_MINUS_SRC_COLOR,        // 1 - S0.r, 1 - S0.g, 1 - S0.b      1 - S0.a
    DST_COLOR,                  // D.r, D.g, D.b                     D.a
    ONE_MINUS_DST_COLOR,        // 1 - D.r, 1 - D.g, 1 - D.b         1 - D.a
    SRC_ALPHA,                  // S0.a                              S0.a
    ONE_MINUS_SRC_ALPHA,        // 1 - S0.a                          1 - S0.a
    DST_ALPHA,                  // D.a                               D.a
    ONE_MINUS_DST_ALPHA,        // 1 - D.a                           1 - D.a
    CONSTANT_COLOR,             // C.r, C.g, C.b                     C.a
    ONE_MINUS_CONSTANT_COLOR,   // 1 - C.r, 1 - C.g, 1 - C.b         1 - C.a
    CONSTANT_ALPHA,             // C.a                               C.a
    ONE_MINUS_CONSTANT_ALPHA,   // 1 - C.a                           1 - C.a
    SRC_ALPHA_SATURATE,         // min(S0.a, 1 - D.a)                1
    SRC1_COLOR,                 // S1.r, S1.g, S1.b                  S1.a
    ONE_MINUS_SRC1_COLOR,       // 1 - S1.r, 1 - S1.g, 1 - S1.b      1 - S1.a
    SRC1_ALPHA,                 // S1.a                              S1.a
    ONE_MINUS_SRC1_ALPHA        // 1 - S1.a                          1 - S1.a
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkBlendOp.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_blend_op
// S - source color
// D - destination color
// Sf - source factor, produced by "BlendFactor"
// Df - destination factor, produced by "BlendFactor"
NriEnum(BlendOp, uint8_t,
    ADD,                        // S * Sf + D * Df
    SUBTRACT,                   // S * Sf - D * Df
    REVERSE_SUBTRACT,           // D * Df - S * Sf
    MIN,                        // min(S, D)
    MAX                         // max(S, D)
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkColorComponentFlagBits.html
NriBits(ColorWriteBits, uint8_t,
    NONE    = 0,
    R       = NriBit(0),
    G       = NriBit(1),
    B       = NriBit(2),
    A       = NriBit(3),

    RGB     = NriMember(ColorWriteBits, R) // "wingdi.h" must not be included after
            | NriMember(ColorWriteBits, G)
            | NriMember(ColorWriteBits, B),

    RGBA    = NriMember(ColorWriteBits, RGB)
            | NriMember(ColorWriteBits, A)
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkStencilOpState.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_depth_stencil_desc
NriStruct(StencilDesc) {
    Nri(CompareOp) compareOp; // "compareOp != NONE", expects "CmdSetStencilReference"
    Nri(StencilOp) failOp;
    Nri(StencilOp) passOp;
    Nri(StencilOp) depthFailOp;
    uint8_t writeMask;
    uint8_t compareMask;
};

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineDepthStencilStateCreateInfo.html
NriStruct(DepthAttachmentDesc) {
    Nri(CompareOp) compareOp;
    bool write;
    bool boundsTest; // requires "features.depthBoundsTest", expects "CmdSetDepthBounds"
};

NriStruct(StencilAttachmentDesc) {
    Nri(StencilDesc) front;
    Nri(StencilDesc) back; // requires "features.independentFrontAndBackStencilReferenceAndMasks" for "back.writeMask"
};

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineColorBlendAttachmentState.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_render_target_blend_desc
NriStruct(BlendDesc) {
    Nri(BlendFactor) srcFactor;
    Nri(BlendFactor) dstFactor;
    Nri(BlendOp) op;
};

NriStruct(ColorAttachmentDesc) {
    Nri(Format) format;
    Nri(BlendDesc) colorBlend;
    Nri(BlendDesc) alphaBlend;
    Nri(ColorWriteBits) colorWriteMask;
    bool blendEnabled;
};

NriStruct(OutputMergerDesc) {
    const NriPtr(ColorAttachmentDesc) colors;
    uint32_t colorNum;
    Nri(DepthAttachmentDesc) depth;
    Nri(StencilAttachmentDesc) stencil;
    Nri(Format) depthStencilFormat;
    Nri(LogicOp) logicOp;                   // requires "features.logicOp"
    NriOptional uint32_t viewMask;          // if non-0, requires "viewMaxNum > 1"
    NriOptional Nri(Multiview) multiview;   // if "viewMask != 0", requires "features.(xxx)Multiview"
};

NriStruct(AttachmentsDesc) {
    NriOptional const NriPtr(Descriptor) depthStencil;
    NriOptional const NriPtr(Descriptor) shadingRate; // requires "tiers.shadingRate >= 2"
    const NriPtr(Descriptor) const* colors;
    uint32_t colorNum;
    NriOptional uint32_t viewMask;          // if non-0, requires "viewMaxNum > 1"
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Pipelines ]
//============================================================================================================================================================================================

// https://docs.vulkan.org/guide/latest/robustness.html
NriEnum(Robustness, uint8_t,
    DEFAULT,        // don't care, follow device settings (VK level when used on a device)
    OFF,            // no overhead, no robust access (out-of-bounds access is not allowed)
    VK,             // minimal overhead, partial robust access
    D3D12           // moderate overhead, D3D12-level robust access (requires "VK_EXT_robustness2", soft fallback to VK mode)
);

// It's recommended to use "NRI.hlsl" in the shader code
NriStruct(ShaderDesc) {
    Nri(StageBits) stage;
    const void* bytecode;
    uint64_t size;
    NriOptional const char* entryPointName;
};

NriStruct(GraphicsPipelineDesc) {
    const NriPtr(PipelineLayout) pipelineLayout;
    NriOptional const NriPtr(VertexInputDesc) vertexInput;
    Nri(InputAssemblyDesc) inputAssembly;
    Nri(RasterizationDesc) rasterization;
    NriOptional const NriPtr(MultisampleDesc) multisample;
    Nri(OutputMergerDesc) outputMerger;
    const NriPtr(ShaderDesc) shaders;
    uint32_t shaderNum;
    NriOptional Nri(Robustness) robustness;
};

NriStruct(ComputePipelineDesc) {
    const NriPtr(PipelineLayout) pipelineLayout;
    Nri(ShaderDesc) shader;
    NriOptional Nri(Robustness) robustness;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Queries ]
//============================================================================================================================================================================================

// https://microsoft.github.io/DirectX-Specs/d3d/CountersAndQueries.html
// https://registry.khronos.org/vulkan/specs/latest/man/html/VkQueryType.html
NriEnum(QueryType, uint8_t,
    TIMESTAMP,                              // uint64_t
    TIMESTAMP_COPY_QUEUE,                   // uint64_t (requires "features.copyQueueTimestamp"), same as "TIMESTAMP" but for a "COPY" queue
    OCCLUSION,                              // uint64_t
    PIPELINE_STATISTICS,                    // see "PipelineStatisticsDesc" (requires "features.pipelineStatistics")
    ACCELERATION_STRUCTURE_SIZE,            // uint64_t, requires "features.rayTracing"
    ACCELERATION_STRUCTURE_COMPACTED_SIZE,  // uint64_t, requires "features.rayTracing"
    MICROMAP_COMPACTED_SIZE                 // uint64_t, requires "features.micromap"
);

NriStruct(QueryPoolDesc) {
    Nri(QueryType) queryType;
    uint32_t capacity;
};

// Data layout for QueryType::PIPELINE_STATISTICS
// https://registry.khronos.org/vulkan/specs/latest/man/html/VkQueryPipelineStatisticFlagBits.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_query_data_pipeline_statistics
NriStruct(PipelineStatisticsDesc) {
    // Common part
    uint64_t inputVertexNum;
    uint64_t inputPrimitiveNum;
    uint64_t vertexShaderInvocationNum;
    uint64_t geometryShaderInvocationNum;
    uint64_t geometryShaderPrimitiveNum;
    uint64_t rasterizerInPrimitiveNum;
    uint64_t rasterizerOutPrimitiveNum;
    uint64_t fragmentShaderInvocationNum;
    uint64_t tessControlShaderInvocationNum;
    uint64_t tessEvaluationShaderInvocationNum;
    uint64_t computeShaderInvocationNum;

    // If "features.meshShaderPipelineStats"
    uint64_t meshControlShaderInvocationNum;
    uint64_t meshEvaluationShaderInvocationNum;

    // D3D12: if "features.meshShaderPipelineStats"
    uint64_t meshEvaluationShaderPrimitiveNum;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Command signatures ]
//============================================================================================================================================================================================

// To fill commands for indirect drawing in a shader use one of "NRI_FILL_X_DESC" macros

// Command signatures (default)

NriStruct(DrawDesc) {                    // see NRI_FILL_DRAW_COMMAND
    uint32_t vertexNum;
    uint32_t instanceNum;
    uint32_t baseVertex;                    // vertex buffer offset = CmdSetVertexBuffers.offset + baseVertex * VertexStreamDesc::stride
    uint32_t baseInstance;
};

NriStruct(DrawIndexedDesc) {             // see NRI_FILL_DRAW_INDEXED_COMMAND
    uint32_t indexNum;
    uint32_t instanceNum;
    uint32_t baseIndex;                     // index buffer offset = CmdSetIndexBuffer.offset + baseIndex * sizeof(CmdSetIndexBuffer.indexType)
    int32_t baseVertex;                     // index += baseVertex
    uint32_t baseInstance;
};

NriStruct(DispatchDesc) {
    uint32_t x, y, z;
};

// D3D12: modified draw command signatures, if the bound pipeline layout has "PipelineLayoutBits::ENABLE_D3D12_DRAW_PARAMETERS_EMULATION"
//  - the following structs must be used instead
// - "NRI_ENABLE_DRAW_PARAMETERS_EMULATION" must be defined prior inclusion of "NRI.hlsl"

NriStruct(DrawBaseDesc) {                // see NRI_FILL_DRAW_COMMAND
    uint32_t shaderEmulatedBaseVertex;      // root constant
    uint32_t shaderEmulatedBaseInstance;    // root constant
    uint32_t vertexNum;
    uint32_t instanceNum;
    uint32_t baseVertex;                    // vertex buffer offset = CmdSetVertexBuffers.offset + baseVertex * VertexStreamDesc::stride
    uint32_t baseInstance;
};

NriStruct(DrawIndexedBaseDesc) {         // see NRI_FILL_DRAW_INDEXED_COMMAND
    int32_t shaderEmulatedBaseVertex;       // root constant
    uint32_t shaderEmulatedBaseInstance;    // root constant
    uint32_t indexNum;
    uint32_t instanceNum;
    uint32_t baseIndex;                     // index buffer offset = CmdSetIndexBuffer.offset + baseIndex * sizeof(CmdSetIndexBuffer.indexType)
    int32_t baseVertex;                     // index += baseVertex
    uint32_t baseInstance;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Other ]
//============================================================================================================================================================================================

// Copy
NriStruct(TextureRegionDesc) {
    Nri(Dim_t) x;
    Nri(Dim_t) y;
    Nri(Dim_t) z;
    Nri(Dim_t) width;       // can be "WHOLE_SIZE" (mip)
    Nri(Dim_t) height;      // can be "WHOLE_SIZE" (mip)
    Nri(Dim_t) depth;       // can be "WHOLE_SIZE" (mip)
    Nri(Dim_t) mipOffset;
    Nri(Dim_t) layerOffset;
    Nri(PlaneBits) planes;
};

NriStruct(TextureDataLayoutDesc) {
    uint64_t offset;        // a buffer offset must be a multiple of "uploadBufferTextureSliceAlignment" (data placement alignment)
    uint32_t rowPitch;      // must be a multiple of "uploadBufferTextureRowAlignment"
    uint32_t slicePitch;    // must be a multiple of "uploadBufferTextureSliceAlignment"
};

// Work submission
NriStruct(FenceSubmitDesc) {
    NriPtr(Fence) fence;
    uint64_t value;
    Nri(StageBits) stages;
};

NriStruct(QueueSubmitDesc) {
    const NriPtr(FenceSubmitDesc) waitFences;
    uint32_t waitFenceNum;
    const NriPtr(CommandBuffer) const* commandBuffers;
    uint32_t commandBufferNum;
    const NriPtr(FenceSubmitDesc) signalFences;
    uint32_t signalFenceNum;
    NriOptional const NriPtr(SwapChain) swapChain; // required if "NRILowLatency" is enabled in the swap chain
};

// Clear
NriStruct(ClearDesc) {
    Nri(ClearValue) value;
    Nri(PlaneBits) planes;
    uint32_t colorAttachmentIndex;
};

// Required synchronization
// - variant 1: "SHADER_RESOURCE_STORAGE" access ("SHADER_RESOURCE_STORAGE" layout) and "CLEAR_STORAGE" stage + any shader stage (or "ALL")
// - variant 2: "CLEAR_STORAGE" access ("SHADER_RESOURCE_STORAGE" layout) and "CLEAR_STORAGE" stage
NriStruct(ClearStorageDesc) {
    // For any buffers and textures with integer formats:
    //  - Clears a storage view with bit-precise values, copying the lower "N" bits from "value.[f/ui/i].channel"
    //    to the corresponding channel, where "N" is the number of bits in the "channel" of the resource format
    // For textures with non-integer formats:
    //  - Clears a storage view with float values with format conversion from "FLOAT" to "UNORM/SNORM" where appropriate
    // For buffers:
    //  - To avoid discrepancies in behavior between GAPIs use "R32f/ui/i" formats for views
    //  - D3D: structured buffers are unsupported!
    const NriPtr(Descriptor) storage; // a "STORAGE" descriptor
    Nri(Color) value; // avoid overflow
    uint32_t setIndex;
    uint32_t rangeIndex;
    uint32_t descriptorIndex;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Device description and capabilities ]
//============================================================================================================================================================================================

NriEnum(Vendor, uint8_t,
    UNKNOWN,
    NVIDIA,
    AMD,
    INTEL
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkPhysicalDeviceType.html
NriEnum(Architecture, uint8_t,
    UNKNOWN,    // CPU device, virtual GPU or other
    INTEGRATED, // UMA
    DESCRETE    // yes, please!
);

// https://registry.khronos.org/vulkan/specs/latest/man/html/VkQueueFlagBits.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_command_list_type
NriEnum(QueueType, uint8_t,
    GRAPHICS,
    COMPUTE,
    COPY
);

NriStruct(AdapterDesc) {
    char name[256];
    Nri(Uid_t) uid; // "LUID" (preferred) if "uid.high = 0", or "UUID" otherwise
    uint64_t videoMemorySize;
    uint64_t sharedSystemMemorySize;
    uint32_t deviceId;
    uint32_t queueNum[(uint32_t)NriScopedMember(QueueType, MAX_NUM)];
    Nri(Vendor) vendor;
    Nri(Architecture) architecture;
};

// Feature support coverage: https://vulkan.gpuinfo.org/ and https://d3d12infodb.boolka.dev/
NriStruct(DeviceDesc) {
    // Common
    Nri(AdapterDesc) adapterDesc; // "queueNum" reflects available number of queues per "QueueType"
    Nri(GraphicsAPI) graphicsAPI;
    uint16_t nriVersion;
    uint8_t shaderModel; // major * 10 + minor

    // Viewport
    struct {
        uint32_t maxNum;
        int16_t boundsMin;
        int16_t boundsMax;
    } viewport;

    // Dimensions
    struct {
        uint32_t typedBufferMaxDim;
        Nri(Dim_t) attachmentMaxDim;
        Nri(Dim_t) attachmentLayerMaxNum;
        Nri(Dim_t) texture1DMaxDim;
        Nri(Dim_t) texture2DMaxDim;
        Nri(Dim_t) texture3DMaxDim;
        Nri(Dim_t) textureLayerMaxNum;
    } dimensions;

    // Precision bits
    struct {
        uint32_t viewportBits;
        uint32_t subPixelBits;
        uint32_t subTexelBits;
        uint32_t mipmapBits;
    } precision;

    // Memory
    struct {
        uint64_t deviceUploadHeapSize; // ReBAR
        uint32_t allocationMaxNum;
        uint32_t samplerAllocationMaxNum;
        uint32_t constantBufferMaxRange;
        uint32_t storageBufferMaxRange;
        uint32_t bufferTextureGranularity;
        uint64_t bufferMaxSize;
    } memory;

    // Memory alignment
    struct {
        uint32_t uploadBufferTextureRow;
        uint32_t uploadBufferTextureSlice;
        uint32_t bufferShaderResourceOffset;
        uint32_t constantBufferOffset;
        uint32_t scratchBufferOffset;
        uint32_t shaderBindingTable;
        uint32_t accelerationStructureOffset;
        uint32_t micromapOffset;
    } memoryAlignment;

    // Pipeline layout
    // D3D12 only: rootConstantSize + descriptorSetNum * 4 + rootDescriptorNum * 8 <= 256 (see "FitPipelineLayoutSettingsIntoDeviceLimits")
    struct {
        uint32_t descriptorSetMaxNum;
        uint32_t rootConstantMaxSize;
        uint32_t rootDescriptorMaxNum;
    } pipelineLayout;

    // Descriptor set
    struct {
        uint32_t samplerMaxNum;
        uint32_t constantBufferMaxNum;
        uint32_t storageBufferMaxNum;
        uint32_t textureMaxNum;
        uint32_t storageTextureMaxNum;

        struct {
            uint32_t samplerMaxNum;
            uint32_t constantBufferMaxNum;
            uint32_t storageBufferMaxNum;
            uint32_t textureMaxNum;
            uint32_t storageTextureMaxNum;
        } updateAfterSet;
    } descriptorSet;

    // Shader stages
    struct {
        // Per stage resources
        uint32_t descriptorSamplerMaxNum;
        uint32_t descriptorConstantBufferMaxNum;
        uint32_t descriptorStorageBufferMaxNum;
        uint32_t descriptorTextureMaxNum;
        uint32_t descriptorStorageTextureMaxNum;
        uint32_t resourceMaxNum;

        struct {
            uint32_t descriptorSamplerMaxNum;
            uint32_t descriptorConstantBufferMaxNum;
            uint32_t descriptorStorageBufferMaxNum;
            uint32_t descriptorTextureMaxNum;
            uint32_t descriptorStorageTextureMaxNum;
            uint32_t resourceMaxNum;
        } updateAfterSet;

        // Vertex
        struct {
            uint32_t attributeMaxNum;
            uint32_t streamMaxNum;
            uint32_t outputComponentMaxNum;
        } vertex;

        // Tessellation control
        struct {
            float generationMaxLevel;
            uint32_t patchPointMaxNum;
            uint32_t perVertexInputComponentMaxNum;
            uint32_t perVertexOutputComponentMaxNum;
            uint32_t perPatchOutputComponentMaxNum;
            uint32_t totalOutputComponentMaxNum;
        } tesselationControl;

        // Tessellation evaluation
        struct {
            uint32_t inputComponentMaxNum;
            uint32_t outputComponentMaxNum;
        } tesselationEvaluation;

        // Geometry
        struct {
            uint32_t invocationMaxNum;
            uint32_t inputComponentMaxNum;
            uint32_t outputComponentMaxNum;
            uint32_t outputVertexMaxNum;
            uint32_t totalOutputComponentMaxNum;
        } geometry;

        // Fragment
        struct {
            uint32_t inputComponentMaxNum;
            uint32_t attachmentMaxNum;
            uint32_t dualSourceAttachmentMaxNum;
        } fragment;

        // Compute
        //  - a "dispatch" consists of "work groups" (aka "thread groups")
        //  - a "work group" consists of "waves" (aka "subgroups" or "warps")
        //  - a "wave" consists of "lanes", which can can be active, inactive or a helper:
        //    - active: the "lane" is performing its computations
        //    - inactive: the "lane" is part of the "wave" but is currently masked out
        //    - helper: these "lanes" are executed to provide auxiliary information (like derivatives) for active threads in the same 2x2 quad
        //  - "invocation" or "thread" is the more general term for a single shader instance, "lane" specifically refers to the position of that thread within a hardware "wave"
        //  - the concept of "wave/lane" execution applies to all shader stages
        struct {
            uint32_t workGroupMaxNum[3];
            uint32_t workGroupMaxDim[3];
            uint32_t workGroupInvocationMaxNum;
            uint32_t sharedMemoryMaxSize;
        } compute;

        // Ray tracing
        struct {
            uint32_t shaderGroupIdentifierSize;
            uint32_t tableMaxStride;
            uint32_t recursionMaxDepth;
        } rayTracing;

        // Mesh control
        struct {
            uint32_t sharedMemoryMaxSize;
            uint32_t workGroupInvocationMaxNum;
            uint32_t payloadMaxSize;
        } meshControl;

        // Mesh evaluation
        struct {
            uint32_t outputVerticesMaxNum;
            uint32_t outputPrimitiveMaxNum;
            uint32_t outputComponentMaxNum;
            uint32_t sharedMemoryMaxSize;
            uint32_t workGroupInvocationMaxNum;
        } meshEvaluation;
    } shaderStage;

    // Wave (subgroup)
    // https://github.com/microsoft/directxshadercompiler/wiki/wave-intrinsics
    // https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_Derivatives.html
    struct {
        uint32_t laneMinNum;
        uint32_t laneMaxNum;
        Nri(StageBits) waveOpsStages;       // SM 6.0+ (see "shaderFeatures.waveX")
        Nri(StageBits) quadOpsStages;       // SM 6.0+ (see "shaderFeatures.waveQuad")
        Nri(StageBits) derivativeOpsStages; // SM 6.6+ (https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_Derivatives.html#derivative-functions)
    } wave;

    // Other
    struct {
        uint64_t timestampFrequencyHz;
        uint32_t micromapSubdivisionMaxLevel;
        uint32_t drawIndirectMaxNum;
        float samplerLodBiasMax;
        float samplerAnisotropyMax;
        int8_t texelGatherOffsetMin;
        int8_t texelOffsetMin;
        uint8_t texelOffsetMax;
        uint8_t texelGatherOffsetMax;
        uint8_t clipDistanceMaxNum;
        uint8_t cullDistanceMaxNum;
        uint8_t combinedClipAndCullDistanceMaxNum;
        uint8_t viewMaxNum;                         // multiview is supported if > 1
        uint8_t shadingRateAttachmentTileSize;      // square size
    } other;

    // Tiers (0 - unsupported)
    struct {
        // https://microsoft.github.io/DirectX-Specs/d3d/ConservativeRasterization.html#tiered-support
        // 1 - 1/2 pixel uncertainty region and does not support post-snap degenerates
        // 2 - reduces the maximum uncertainty region to 1/256 and requires post-snap degenerates not be culled
        // 3 - maintains a maximum 1/256 uncertainty region and adds support for inner input coverage, aka "SV_InnerCoverage"
        uint8_t conservativeRaster;

        // https://microsoft.github.io/DirectX-Specs/d3d/ProgrammableSamplePositions.html#hardware-tiers
        // 1 - a single sample pattern can be specified to repeat for every pixel ("locationNum / sampleNum" ratio must be 1 in "CmdSetSampleLocations"),
        //     1x and 16x sample counts do not support programmable locations
        // 2 - four separate sample patterns can be specified for each pixel in a 2x2 grid ("locationNum / sampleNum" ratio can be 1 or 4 in "CmdSetSampleLocations"),
        //     all sample counts support programmable positions
        uint8_t sampleLocations;

        // https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#checkfeaturesupport-structures
        // 1 - DXR 1.0: full raytracing functionality, except features below
        // 2 - DXR 1.1: adds - ray query, "CmdDispatchRaysIndirect", "GeometryIndex()" intrinsic, additional ray flags & vertex formats
        // 3 - DXR 1.2: adds - micromap, shader execution reordering
        uint8_t rayTracing;

        // https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html#feature-tiering
        // 1 - shading rate can be specified only per draw
        // 2 - adds: per primitive shading rate, per "shadingRateAttachmentTileSize" shading rate, combiners, "SV_ShadingRate" support
        uint8_t shadingRate;

        // https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#limitations-on-static-samplers
        // 0 - ALL descriptors in range must be valid by the time the command list executes
        // 1 - only "CONSTANT_BUFFER" and "STORAGE" descriptors in range must be valid
        // 2 - only referenced descriptors must be valid
        uint8_t resourceBinding;

        // 1 - unbound arrays with dynamic indexing
        // 2 - D3D12 dynamic resources: https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html
        uint8_t bindless;

        // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_heap_tier
        // 1 - a "Memory" can support resources from all 3 categories: buffers, attachments, all other textures
        uint8_t memory;
    } tiers;

    // Features
    struct {
        // Bigger
        uint32_t getMemoryDesc2                                  : 1; // "GetXxxMemoryDesc2" support (VK: requires "maintenance4", D3D: supported)
        uint32_t enhancedBarriers                                : 1; // VK: supported, D3D12: requires "AgilitySDK", D3D11: unsupported
        uint32_t swapChain                                       : 1; // NRISwapChain
        uint32_t rayTracing                                      : 1; // NRIRayTracing
        uint32_t meshShader                                      : 1; // NRIMeshShader
        uint32_t lowLatency                                      : 1; // NRILowLatency
        uint32_t micromap                                        : 1; // see "Micromap"

        // Smaller
        uint32_t independentFrontAndBackStencilReferenceAndMasks : 1; // see "StencilAttachmentDesc::back"
        uint32_t textureFilterMinMax                             : 1; // see "ReductionMode"
        uint32_t logicOp                                         : 1; // see "LogicOp"
        uint32_t depthBoundsTest                                 : 1; // see "DepthAttachmentDesc::boundsTest"
        uint32_t drawIndirectCount                               : 1; // see "countBuffer" and "countBufferOffset"
        uint32_t lineSmoothing                                   : 1; // see "RasterizationDesc::lineSmoothing"
        uint32_t copyQueueTimestamp                              : 1; // see "QueryType::TIMESTAMP_COPY_QUEUE"
        uint32_t meshShaderPipelineStats                         : 1; // see "PipelineStatisticsDesc"
        uint32_t dynamicDepthBias                                : 1; // see "CmdSetDepthBias"
        uint32_t additionalShadingRates                          : 1; // see "ShadingRate"
        uint32_t viewportOriginBottomLeft                        : 1; // see "Viewport"
        uint32_t regionResolve                                   : 1; // see "CmdResolveTexture"
        uint32_t flexibleMultiview                               : 1; // see "Multiview::FLEXIBLE"
        uint32_t layerBasedMultiview                             : 1; // see "Multiview::LAYRED_BASED"
        uint32_t viewportBasedMultiview                          : 1; // see "Multiview::VIEWPORT_BASED"
        uint32_t presentFromCompute                              : 1; // see "SwapChainDesc::queue"
        uint32_t waitableSwapChain                               : 1; // see "SwapChainDesc::waitable"
        uint32_t resizableSwapChain                              : 1; // swap chain can be resized without triggering an "OUT_OF_DATE" error
        uint32_t pipelineStatistics                              : 1; // see "QueryType::PIPELINE_STATISTICS"
        uint32_t rootConstantsOffset                             : 1; // see "SetRootConstantsDesc" (unsupported only in D3D11)
        uint32_t nonConstantBufferRootDescriptorOffset           : 1; // see "SetRootDescriptorDesc" (unsupported only in D3D11)
        uint32_t mutableDescriptorType                           : 1; // see "DescriptorRangeBits::MUTABLE"
    } features;

    // Shader features
    // https://github.com/Microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst
    struct {
        uint32_t viewportIndex                                   : 1; // SV_ViewportArrayIndex, always can be used in geometry shaders
        uint32_t layerIndex                                      : 1; // SV_RenderTargetArrayIndex, always can be used in geometry shaders
        uint32_t unnormalizedCoordinates                         : 1; // https://microsoft.github.io/DirectX-Specs/d3d/VulkanOn12.html#non-normalized-texture-sampling-coordinates
        uint32_t clock                                           : 1; // https://github.com/Microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst#readclock
        uint32_t rasterizedOrderedView                           : 1; // https://microsoft.github.io/DirectX-Specs/d3d/RasterOrderViews.html (aka fragment shader interlock)
        uint32_t barycentric                                     : 1; // https://github.com/microsoft/DirectXShaderCompiler/wiki/SV_Barycentrics
        uint32_t rayTracingPositionFetch                         : 1; // https://docs.vulkan.org/features/latest/features/proposals/VK_KHR_ray_tracing_position_fetch.html

        // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-scalar
        // I32 and F32 are always supported
        uint32_t nativeI16                                       : 1; // "(u)int16_t"
        uint32_t nativeF16                                       : 1; // "float16_t"
        uint32_t nativeI64                                       : 1; // "(u)int64_t"
        uint32_t nativeF64                                       : 1; // "double"

        // https://learn.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-advanced-stages-cs-atomic-functions
        // https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_Int64_and_Float_Atomics.html
        // I32 atomics are always supported (for others it can be partial support of SMEM, texture or buffer atomics)
        uint32_t atomicsI16                                      : 1; // "(u)int16_t" atomics
        uint32_t atomicsF16                                      : 1; // "float16_t" atomics
        uint32_t atomicsF32                                      : 1; // "float" atomics
        uint32_t atomicsI64                                      : 1; // "(u)int64_t" atomics
        uint32_t atomicsF64                                      : 1; // "double" atomics

        // https://learn.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads#using-unorm-and-snorm-typed-uav-loads-from-hlsl
        uint32_t storageReadWithoutFormat                        : 1; // NRI_FORMAT("unknown") is allowed for storage reads
        uint32_t storageWriteWithoutFormat                       : 1; // NRI_FORMAT("unknown") is allowed for storage writes

        // https://github.com/microsoft/directxshadercompiler/wiki/wave-intrinsics
        uint32_t waveQuery                                       : 1; // WaveIsFirstLane, WaveGetLaneCount, WaveGetLaneIndex
        uint32_t waveVote                                        : 1; // WaveActiveAllTrue, WaveActiveAnyTrue, WaveActiveAllEqual
        uint32_t waveShuffle                                     : 1; // WaveReadLaneFirst, WaveReadLaneAt
        uint32_t waveArithmetic                                  : 1; // WaveActiveSum, WaveActiveProduct, WaveActiveMin, WaveActiveMax, WavePrefixProduct, WavePrefixSum
        uint32_t waveReduction                                   : 1; // WaveActiveCountBits, WaveActiveBitAnd, WaveActiveBitOr, WaveActiveBitXor, WavePrefixCountBits
        uint32_t waveQuad                                        : 1; // QuadReadLaneAt, QuadReadAcrossX, QuadReadAcrossY, QuadReadAcrossDiagonal
    } shaderFeatures;
};

#pragma endregion

NriNamespaceEnd
