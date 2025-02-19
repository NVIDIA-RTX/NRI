// © 2021 NVIDIA Corporation

#pragma once

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

// TIP: designated initializers are highly recommended

NriNamespaceBegin

// Entities
NriForwardStruct(Fence);
NriForwardStruct(Queue);
NriForwardStruct(Memory); // heap
NriForwardStruct(Buffer);
NriForwardStruct(Device);
NriForwardStruct(Texture);
NriForwardStruct(Pipeline);
NriForwardStruct(QueryPool);
NriForwardStruct(Descriptor);
NriForwardStruct(CommandBuffer); // command list
NriForwardStruct(DescriptorSet); // continuous set of descriptors in a descriptor heap
NriForwardStruct(DescriptorPool); // descriptor heap
NriForwardStruct(PipelineLayout); // root signature
NriForwardStruct(CommandAllocator);

// Types
typedef uint8_t Nri(Mip_t);
typedef uint8_t Nri(Sample_t);
typedef uint16_t Nri(Dim_t);
typedef uint32_t Nri(MemoryType);
typedef void Nri(Object);

// Aliases
static const uint32_t NriConstant(BGRA_UNUSED) = 0;         // only for "bgra" color for profiling
static const uint32_t NriConstant(ALL_SAMPLES) = 0;         // only for "sampleMask"
static const Nri(Dim_t) NriConstant(WHOLE_SIZE) = 0;        // only for "Dim_t" and "size"
static const Nri(Mip_t) NriConstant(REMAINING_MIPS) = 0;    // only for "mipNum"
static const Nri(Dim_t) NriConstant(REMAINING_LAYERS) = 0;  // only for "layerNum"

// Readability
#define NriOptional // i.e. can be 0 (keep an eye on comments)
#define NriOut      // highlights an output argument

//============================================================================================================================================================================================
#pragma region [ Common ]
//============================================================================================================================================================================================

NriEnum(GraphicsAPI, uint8_t,
    NONE,   // Supports everything, does nothing, returns dummy non-NULL objects and ~0-filled descs, available if "NRI_ENABLE_NONE_SUPPORT = ON" in CMake
    D3D11,  // Direct3D 11 (feature set 11.1), available if "NRI_ENABLE_D3D11_SUPPORT = ON" in CMake
    D3D12,  // Direct3D 12 (feature set 11.1+), available if "NRI_ENABLE_D3D12_SUPPORT = ON" in CMake
    VK      // Vulkan 1.3 or 1.2+ (can be used on MacOS via MoltenVK), available if "NRI_ENABLE_VK_SUPPORT = ON" in CMake
);

NriEnum(Result, uint8_t,
    SUCCESS,
    FAILURE,
    INVALID_ARGUMENT,
    OUT_OF_MEMORY,
    UNSUPPORTED,
    DEVICE_LOST,
    OUT_OF_DATE // VK only: swap chain is out of date
);

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

    RG8_UNORM,                           // +  +  +  -  +  -  +  +  +  -
    RG8_SNORM,                           // +  +  +  -  +  -  +  +  +  -
    RG8_UINT,                            // +  +  +  -  -  -  +  +  +  -
    RG8_SINT,                            // +  +  +  -  -  -  +  +  +  -

    BGRA8_UNORM,                         // +  +  +  -  +  -  +  +  +  -
    BGRA8_SRGB,                          // +  -  +  -  +  -  -  -  -  -

    RGBA8_UNORM,                         // +  +  +  -  +  -  +  +  +  -
    RGBA8_SRGB,                          // +  -  +  -  +  -  -  -  -  -
    RGBA8_SNORM,                         // +  +  +  -  +  -  +  +  +  -
    RGBA8_UINT,                          // +  +  +  -  -  -  +  +  +  -
    RGBA8_SINT,                          // +  +  +  -  -  -  +  +  +  -

    // Plain: 16 bits per channel
    R16_UNORM,                           // +  +  +  -  +  -  +  +  +  -
    R16_SNORM,                           // +  +  +  -  +  -  +  +  +  -
    R16_UINT,                            // +  +  +  -  -  -  +  +  +  -
    R16_SINT,                            // +  +  +  -  -  -  +  +  +  -
    R16_SFLOAT,                          // +  +  +  -  +  -  +  +  +  -

    RG16_UNORM,                          // +  +  +  -  +  -  +  +  +  -
    RG16_SNORM,                          // +  +  +  -  +  -  +  +  +  -
    RG16_UINT,                           // +  +  +  -  -  -  +  +  +  -
    RG16_SINT,                           // +  +  +  -  -  -  +  +  +  -
    RG16_SFLOAT,                         // +  +  +  -  +  -  +  +  +  -

    RGBA16_UNORM,                        // +  +  +  -  +  -  +  +  +  -
    RGBA16_SNORM,                        // +  +  +  -  +  -  +  +  +  -
    RGBA16_UINT,                         // +  +  +  -  -  -  +  +  +  -
    RGBA16_SINT,                         // +  +  +  -  -  -  +  +  +  -
    RGBA16_SFLOAT,                       // +  +  +  -  +  -  +  +  +  -

    // Plain: 32 bits per channel
    R32_UINT,                            // +  +  +  -  -  +  +  +  +  +
    R32_SINT,                            // +  +  +  -  -  +  +  +  +  +
    R32_SFLOAT,                          // +  +  +  -  +  +  +  +  +  +

    RG32_UINT,                           // +  +  +  -  -  -  +  +  +  -
    RG32_SINT,                           // +  +  +  -  -  -  +  +  +  -
    RG32_SFLOAT,                         // +  +  +  -  +  -  +  +  +  -

    RGB32_UINT,                          // +  -  -  -  -  -  +  -  +  -
    RGB32_SINT,                          // +  -  -  -  -  -  +  -  +  -
    RGB32_SFLOAT,                        // +  -  -  -  -  -  +  -  +  -

    RGBA32_UINT,                         // +  +  +  -  -  -  +  +  +  -
    RGBA32_SINT,                         // +  +  +  -  -  -  +  +  +  -
    RGBA32_SFLOAT,                       // +  +  +  -  +  -  +  +  +  -

    // Packed: 16 bits per pixel
    B5_G6_R5_UNORM,                      // +  -  +  -  +  -  -  -  -  -
    B5_G5_R5_A1_UNORM,                   // +  -  +  -  +  -  -  -  -  -
    B4_G4_R4_A4_UNORM,                   // +  -  +  -  +  -  -  -  -  -

    // Packed: 32 bits per pixel
    R10_G10_B10_A2_UNORM,                // +  +  +  -  +  -  +  +  +  -
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

NriBits(PlaneBits, uint8_t,
    ALL                             = 0,
    COLOR                           = NriBit(0),
    DEPTH                           = NriBit(1),
    STENCIL                         = NriBit(2)
);

NriBits(FormatSupportBits, uint16_t,
    UNSUPPORTED                     = 0,

    // Texture
    TEXTURE                         = NriBit(0),
    STORAGE_TEXTURE                 = NriBit(1),
    COLOR_ATTACHMENT                = NriBit(2),
    DEPTH_STENCIL_ATTACHMENT        = NriBit(3),
    BLEND                           = NriBit(4),
    STORAGE_TEXTURE_ATOMICS         = NriBit(5),  // other than Load / Store

    // Buffer
    BUFFER                          = NriBit(6),
    STORAGE_BUFFER                  = NriBit(7),
    VERTEX_BUFFER                   = NriBit(8),
    STORAGE_BUFFER_ATOMICS          = NriBit(9)   // other than Load / Store
);

NriBits(StageBits, uint32_t,
    // Special
    ALL                             = 0,          // Lazy default for barriers
    NONE                            = 0x7FFFFFFF,

    // Graphics                                   // Invoked by "CmdDraw*"
    INDEX_INPUT                     = NriBit(0),  //    Index buffer consumption
    VERTEX_SHADER                   = NriBit(1),  //    Vertex shader
    TESS_CONTROL_SHADER             = NriBit(2),  //    Tessellation control (hull) shader
    TESS_EVALUATION_SHADER          = NriBit(3),  //    Tessellation evaluation (domain) shader
    GEOMETRY_SHADER                 = NriBit(4),  //    Geometry shader
    MESH_CONTROL_SHADER             = NriBit(5),  //    Mesh control (task) shader
    MESH_EVALUATION_SHADER          = NriBit(6),  //    Mesh evaluation (amplification) shader
    FRAGMENT_SHADER                 = NriBit(7),  //    Fragment (pixel) shader
    DEPTH_STENCIL_ATTACHMENT        = NriBit(8),  //    Depth-stencil R/W operations
    COLOR_ATTACHMENT                = NriBit(9),  //    Color R/W operations

    // Compute                                    // Invoked by  "CmdDispatch*" (not Rays)
    COMPUTE_SHADER                  = NriBit(10), //    Compute shader

    // Ray tracing                                // Invoked by "CmdDispatchRays*"
    RAYGEN_SHADER                   = NriBit(11), //    Ray generation shader
    MISS_SHADER                     = NriBit(12), //    Miss shader
    INTERSECTION_SHADER             = NriBit(13), //    Intersection shader
    CLOSEST_HIT_SHADER              = NriBit(14), //    Closest hit shader
    ANY_HIT_SHADER                  = NriBit(15), //    Any hit shader
    CALLABLE_SHADER                 = NriBit(16), //    Callable shader

    ACCELERATION_STRUCTURE          = NriBit(17), // Invoked by "Cmd*AccelerationStructure*"

    // Copy
    COPY                            = NriBit(18), // Invoked by "CmdCopy*", "CmdUpload*" and "CmdReadback*"
    CLEAR_STORAGE                   = NriBit(19), // Invoked by "CmdClearStorage*"
    RESOLVE                         = NriBit(20), // Invoked by "CmdResolveTexture"

    // Modifiers
    INDIRECT                        = NriBit(21), // Invoked by "Indirect" command (used in addition to other bits)

    // Umbrella stages
    TESSELLATION_SHADERS            = NriMember(StageBits, TESS_CONTROL_SHADER) |
                                      NriMember(StageBits, TESS_EVALUATION_SHADER),

    MESH_SHADERS                    = NriMember(StageBits, MESH_CONTROL_SHADER) |
                                      NriMember(StageBits, MESH_EVALUATION_SHADER),

    GRAPHICS_SHADERS                = NriMember(StageBits, VERTEX_SHADER) |
                                      NriMember(StageBits, TESSELLATION_SHADERS) |
                                      NriMember(StageBits, GEOMETRY_SHADER) |
                                      NriMember(StageBits, MESH_SHADERS) |
                                      NriMember(StageBits, FRAGMENT_SHADER),

    // Invoked by "CmdDispatchRays"
    RAY_TRACING_SHADERS             = NriMember(StageBits, RAYGEN_SHADER) |
                                      NriMember(StageBits, MISS_SHADER) |
                                      NriMember(StageBits, INTERSECTION_SHADER) |
                                      NriMember(StageBits, CLOSEST_HIT_SHADER) |
                                      NriMember(StageBits, ANY_HIT_SHADER) |
                                      NriMember(StageBits, CALLABLE_SHADER),

    // Invoked by "CmdDraw*"
    DRAW                            = NriMember(StageBits, INDEX_INPUT) |
                                      NriMember(StageBits, GRAPHICS_SHADERS) |
                                      NriMember(StageBits, DEPTH_STENCIL_ATTACHMENT) |
                                      NriMember(StageBits, COLOR_ATTACHMENT)
);

// The viewport origin is top-left (D3D native) by default, but can be changed to bottom-left (VK native)
NriStruct(Viewport) {
    float x;
    float y;
    float width;
    float height;
    float depthMin;
    float depthMax;
    bool originBottomLeft; // expects "isViewportOriginBottomLeftSupported"
};

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

NriStruct(Dim2) {
    Nri(Dim_t) w, h;
};

NriStruct(Float2) {
    float x, y;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Creation ]
//============================================================================================================================================================================================

NriEnum(Robustness, uint8_t,
    DEFAULT,    // don't care, follow device settings (VK level when used on a device)
    OFF,        // no overhead, no robust access (out-of-bounds access is not allowed)
    VK,         // minimal overhead, partial robust access
    D3D12       // moderate overhead, D3D12-level robust access (requires "VK_EXT_robustness2", soft fallback to VK mode)
);

NriEnum(MemoryLocation, uint8_t,
    DEVICE,
    DEVICE_UPLOAD, // soft fallback to HOST_UPLOAD
    HOST_UPLOAD,
    HOST_READBACK
);

NriEnum(TextureType, uint8_t,
    TEXTURE_1D,
    TEXTURE_2D,
    TEXTURE_3D
);

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

NriBits(TextureUsageBits, uint8_t,
    NONE                                = 0,
    SHADER_RESOURCE                     = NriBit(0),
    SHADER_RESOURCE_STORAGE             = NriBit(1),
    COLOR_ATTACHMENT                    = NriBit(2),
    DEPTH_STENCIL_ATTACHMENT            = NriBit(3),
    SHADING_RATE_ATTACHMENT             = NriBit(4)
);

NriBits(BufferUsageBits, uint16_t,
    NONE                                = 0,
    SHADER_RESOURCE                     = NriBit(0),
    SHADER_RESOURCE_STORAGE             = NriBit(1),
    VERTEX_BUFFER                       = NriBit(2),
    INDEX_BUFFER                        = NriBit(3),
    CONSTANT_BUFFER                     = NriBit(4),
    ARGUMENT_BUFFER                     = NriBit(5),

    // Ray tracing
    SCRATCH_BUFFER                      = NriBit(6),
    SHADER_BINDING_TABLE                = NriBit(7),
    ACCELERATION_STRUCTURE_BUILD_INPUT  = NriBit(8), // read-only input for an acceleration structure build or update
    ACCELERATION_STRUCTURE_STORAGE      = NriBit(9)  // (INTERNAL) acceleration structure storage
);

// Resources
NriStruct(TextureDesc) {
    Nri(TextureType) type;
    Nri(TextureUsageBits) usage;
    Nri(Format) format;
    Nri(Dim_t) width;
    NriOptional Nri(Dim_t) height;
    NriOptional Nri(Dim_t) depth;
    NriOptional Nri(Mip_t) mipNum;
    NriOptional Nri(Dim_t) layerNum;
    NriOptional Nri(Sample_t) sampleNum;
};

NriStruct(BufferDesc) {
    uint64_t size;
    NriOptional uint32_t structureStride; // use 4 to allow "byte address" (raw) views
    Nri(BufferUsageBits) usage;
};

// Descriptors (Views)
NriStruct(Texture1DViewDesc) {
    const NriPtr(Texture) texture;
    Nri(Texture1DViewType) viewType;
    Nri(Format) format;
    Nri(Mip_t) mipOffset;
    Nri(Mip_t) mipNum;
    Nri(Dim_t) layerOffset;
    Nri(Dim_t) layerNum;
};

NriStruct(Texture2DViewDesc) {
    const NriPtr(Texture) texture;
    Nri(Texture2DViewType) viewType;
    Nri(Format) format;
    Nri(Mip_t) mipOffset;
    Nri(Mip_t) mipNum;
    Nri(Dim_t) layerOffset;
    Nri(Dim_t) layerNum;
};

NriStruct(Texture3DViewDesc) {
    const NriPtr(Texture) texture;
    Nri(Texture3DViewType) viewType;
    Nri(Format) format;
    Nri(Mip_t) mipOffset;
    Nri(Mip_t) mipNum;
    Nri(Dim_t) sliceOffset;
    Nri(Dim_t) sliceNum;
};

NriStruct(BufferViewDesc) {
    const NriPtr(Buffer) buffer;
    Nri(BufferViewType) viewType;
    Nri(Format) format;
    uint64_t offset;
    uint64_t size;
};

// Descriptor pool
NriStruct(DescriptorPoolDesc) {
    uint32_t descriptorSetMaxNum;
    uint32_t samplerMaxNum;
    uint32_t constantBufferMaxNum;
    uint32_t dynamicConstantBufferMaxNum;
    uint32_t textureMaxNum;
    uint32_t storageTextureMaxNum;
    uint32_t bufferMaxNum;
    uint32_t storageBufferMaxNum;
    uint32_t structuredBufferMaxNum;
    uint32_t storageStructuredBufferMaxNum;
    uint32_t accelerationStructureMaxNum;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Pipeline layout and descriptors management ]
//============================================================================================================================================================================================

/*
All indices are local in the currently bound pipeline layout.

Pipeline layout example:
    Descriptor set                  #0          // "setIndex" - a descriptor set index in the pipeline layout, provided as an argument or bound to the pipeline
        Descriptor range                #0      // "rangeIndex" and "baseRange" - a descriptor range (base) index in the descriptor set
            Descriptor                      #0  // "descriptorIndex" and "baseDescriptor" - a descriptor (base) index in the descriptor range
            Descriptor                      #1
            Descriptor                      #2
        Descriptor range                #1
            Descriptor                      #0
            Descriptor                      #1
        Dynamic constant buffer         #0      // "baseDynamicConstantBuffer" - an offset in "dynamicConstantBuffers" in the currently bound pipeline layout for the provided descriptor set
        Dynamic constant buffer         #1

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
        Dynamic constant buffer         #0

    RootConstantDesc                #0          // "rootConstantIndex" - an index in "rootConstants" in the currently bound pipeline layout

    RootDescriptorDesc              #0          // "rootDescriptorIndex" - an index in "rootDescriptors" in the currently bound pipeline layout
    RootDescriptorDesc              #1
*/

// "DescriptorRange" consists of "Descriptor" entities
NriBits(DescriptorRangeBits, uint8_t,
    NONE                    = 0,
    PARTIALLY_BOUND         = NriBit(0), // descriptors in range may not contain valid descriptors at the time the descriptors are consumed (but referenced descriptors must be valid)
    ARRAY                   = NriBit(1), // descriptors in range are organized into an array
    VARIABLE_SIZED_ARRAY    = NriBit(2)  // descriptors in range are organized into a variable-sized array, whose size is specified via "variableDescriptorNum" argument of "AllocateDescriptorSets" function
);

NriStruct(DescriptorRangeDesc) {
    uint32_t baseRegisterIndex;
    uint32_t descriptorNum; // treated as max size if "VARIABLE_SIZED_ARRAY" flag is set
    Nri(DescriptorType) descriptorType;
    Nri(StageBits) shaderStages;
    Nri(DescriptorRangeBits) flags;
};

// "DescriptorSet" consists of "DescriptorRange" entities
NriStruct(DynamicConstantBufferDesc) {
    uint32_t registerIndex;
    Nri(StageBits) shaderStages;
};

NriStruct(DescriptorSetDesc) {
    uint32_t registerSpace; // must be unique, avoid big gaps
    const NriPtr(DescriptorRangeDesc) ranges;
    uint32_t rangeNum;
    const NriPtr(DynamicConstantBufferDesc) dynamicConstantBuffers; // a dynamic constant buffer allows to dynamically specify an offset in the buffer via "CmdSetDescriptorSet" call
    uint32_t dynamicConstantBufferNum;
};

// "PipelineLayout" consists of "DescriptorSet" descriptions and root parameters
NriStruct(RootConstantDesc) { // aka push constants block
    uint32_t registerIndex;
    uint32_t size;
    Nri(StageBits) shaderStages;
};

NriStruct(RootDescriptorDesc) { // aka push descriptor
    uint32_t registerIndex;
    Nri(DescriptorType) descriptorType; // CONSTANT_BUFFER, STRUCTURED_BUFFER or STORAGE_STRUCTURED_BUFFER
    Nri(StageBits) shaderStages;
};

NriStruct(PipelineLayoutDesc) {
    uint32_t rootRegisterSpace;
    const NriPtr(RootConstantDesc) rootConstants;
    uint32_t rootConstantNum;
    const NriPtr(RootDescriptorDesc) rootDescriptors;
    uint32_t rootDescriptorNum;
    const NriPtr(DescriptorSetDesc) descriptorSets;
    uint32_t descriptorSetNum;
    Nri(StageBits) shaderStages;
    bool ignoreGlobalSPIRVOffsets;
    bool enableD3D12DrawParametersEmulation; // enable draw parameters emulation, not needed if all vertex shaders for this layout compiled with SM 6.8 (native support)
};

// Updating descriptors
NriStruct(DescriptorRangeUpdateDesc) {
    const NriPtr(Descriptor) const* descriptors;
    uint32_t descriptorNum;
    uint32_t baseDescriptor;
};

NriStruct(DescriptorSetCopyDesc) {
    const NriPtr(DescriptorSet) srcDescriptorSet;
    uint32_t srcBaseRange;
    uint32_t dstBaseRange;
    uint32_t rangeNum;
    uint32_t srcBaseDynamicConstantBuffer;
    uint32_t dstBaseDynamicConstantBuffer;
    uint32_t dynamicConstantBufferNum;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Input assembly ]
//============================================================================================================================================================================================

NriEnum(VertexStreamStepRate, uint8_t,
    PER_VERTEX,
    PER_INSTANCE
);

NriEnum(IndexType, uint8_t,
    UINT16,
    UINT32
);

NriEnum(PrimitiveRestart, uint8_t,
    DISABLED,
    INDICES_UINT16,
    INDICES_UINT32
);

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
    uint16_t stride; // TODO: the code is ready to accept "stride" as a dynamic parameter of "CmdSetVertexBuffers"
    uint16_t bindingSlot;
    Nri(VertexStreamStepRate) stepRate;
};

NriStruct(VertexInputDesc) {
    const NriPtr(VertexAttributeDesc) attributes;
    uint8_t attributeNum;
    const NriPtr(VertexStreamDesc) streams;
    uint8_t streamNum;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Rasterization ]
//============================================================================================================================================================================================

NriEnum(FillMode, uint8_t,
    SOLID,
    WIREFRAME
);

NriEnum(CullMode, uint8_t,
    NONE,
    FRONT,
    BACK
);

NriEnum(ShadingRate, uint8_t,
    FRAGMENT_SIZE_1X1,
    FRAGMENT_SIZE_1X2,
    FRAGMENT_SIZE_2X1,
    FRAGMENT_SIZE_2X2,

    // Require "isAdditionalShadingRatesSupported"
    FRAGMENT_SIZE_2X4,
    FRAGMENT_SIZE_4X2,
    FRAGMENT_SIZE_4X4
);

NriEnum(ShadingRateCombiner, uint8_t,
    REPLACE,
    KEEP,
    MIN,
    MAX,
    SUM
);

/*
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
    bool lineSmoothing;         // requires "isLineSmoothingSupported"
    bool conservativeRaster;    // requires "conservativeRasterTier != 0"
    bool shadingRate;           // requires "shadingRateTier != 0", expects "CmdSetShadingRate" and optionally "AttachmentsDesc::shadingRate"
};

NriStruct(MultisampleDesc) {
    uint32_t sampleMask;
    Nri(Sample_t) sampleNum;
    bool alphaToCoverage;
    bool sampleLocations; // requires "sampleLocationsTier != 0", expects "CmdSetSampleLocations"
};

NriStruct(ShadingRateDesc) {
    Nri(ShadingRate) shadingRate;
    Nri(ShadingRateCombiner) primitiveCombiner;
    Nri(ShadingRateCombiner) attachmentCombiner;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Output merger ]
//============================================================================================================================================================================================

NriEnum(Multiview, uint8_t,
    FLEXIBLE,                   // destination "viewport" and/or "layer" must be set in shaders explicitly, "viewMask" for rendering can be < than the one used for pipeline creation (D3D12 style)
    LAYER_BASED,                // view instances go to statically assigned corresponding attachment layers, "viewMask" for rendering must match the one used for pipeline creation (VK style)
    VIEWPORT_BASED              // view instances go to statically assigned corresponding viewports, "viewMask" for pipeline creation is unused (D3D11 style)
);

// S - source color 0
// D - destination color
NriEnum(LogicFunc, uint8_t,
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

// R - fragment's depth or stencil reference
// D - depth or stencil buffer
NriEnum(CompareFunc, uint8_t,
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

// R - reference, set by "CmdSetStencilReference"
// D - stencil buffer
NriEnum(StencilFunc, uint8_t,
    KEEP,                       // D = D
    ZERO,                       // D = 0
    REPLACE,                    // D = R
    INCREMENT_AND_CLAMP,        // D = min(D++, 255)
    DECREMENT_AND_CLAMP,        // D = max(D--, 0)
    INVERT,                     // D = ~D
    INCREMENT_AND_WRAP,         // D++
    DECREMENT_AND_WRAP          // D--
);

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

// S - source color
// D - destination color
// Sf - source factor, produced by "BlendFactor"
// Df - destination factor, produced by "BlendFactor"
NriEnum(BlendFunc, uint8_t,
    ADD,                        // S * Sf + D * Df
    SUBTRACT,                   // S * Sf - D * Df
    REVERSE_SUBTRACT,           // D * Df - S * Sf
    MIN,                        // min(S, D)
    MAX                         // max(S, D)
);

NriBits(ColorWriteBits, uint8_t,
    NONE    = 0,
    R       = NriBit(0),
    G       = NriBit(1),
    B       = NriBit(2),
    A       = NriBit(3),

    RGB     = NriMember(ColorWriteBits, R) | // "wingdi.h" must not be included after
              NriMember(ColorWriteBits, G) |
              NriMember(ColorWriteBits, B),

    RGBA    = NriMember(ColorWriteBits, RGB) |
              NriMember(ColorWriteBits, A)
);

NriStruct(ClearDesc) {
    Nri(ClearValue) value;
    Nri(PlaneBits) planes;
    uint32_t colorAttachmentIndex;
};

NriStruct(StencilDesc) {
    Nri(CompareFunc) compareFunc; // compareFunc != NONE, expects "CmdSetStencilReference"
    Nri(StencilFunc) fail;
    Nri(StencilFunc) pass;
    Nri(StencilFunc) depthFail;
    uint8_t writeMask;
    uint8_t compareMask;
};

NriStruct(BlendingDesc) {
    Nri(BlendFactor) srcFactor;
    Nri(BlendFactor) dstFactor;
    Nri(BlendFunc) func;
};

NriStruct(ColorAttachmentDesc) {
    Nri(Format) format;
    Nri(BlendingDesc) colorBlend;
    Nri(BlendingDesc) alphaBlend;
    Nri(ColorWriteBits) colorWriteMask;
    bool blendEnabled;
};

NriStruct(DepthAttachmentDesc) {
    Nri(CompareFunc) compareFunc;
    bool write;
    bool boundsTest; // requires "isDepthBoundsTestSupported", expects "CmdSetDepthBounds"
};

NriStruct(StencilAttachmentDesc) {
    Nri(StencilDesc) front;
    Nri(StencilDesc) back; // requires "isIndependentFrontAndBackStencilReferenceAndMasksSupported" for "back.writeMask"
};

NriStruct(OutputMergerDesc) {
    const NriPtr(ColorAttachmentDesc) colors;
    uint32_t colorNum;
    Nri(DepthAttachmentDesc) depth;
    Nri(StencilAttachmentDesc) stencil;
    Nri(Format) depthStencilFormat;
    Nri(LogicFunc) logicFunc; // requires "isLogicFuncSupported"
    NriOptional uint32_t viewMask; // if non-0, requires "viewMaxNum > 1"
    NriOptional Nri(Multiview) multiview; // if viewMask != 0, requires "is(multiview)MultiviewSupported"
};

NriStruct(AttachmentsDesc) {
    NriOptional const NriPtr(Descriptor) depthStencil;
    NriOptional const NriPtr(Descriptor) shadingRate; // requires "shadingRateTier >= 2"
    const NriPtr(Descriptor) const* colors;
    uint32_t colorNum;
    NriOptional uint32_t viewMask;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Sampler ]
//============================================================================================================================================================================================

NriEnum(Filter, uint8_t,
    NEAREST,
    LINEAR
);

NriEnum(FilterExt, uint8_t,
    NONE,
    MIN,
    MAX
);

NriEnum(AddressMode, uint8_t,
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER,
    MIRROR_CLAMP_TO_EDGE
);

NriStruct(AddressModes) {
    Nri(AddressMode) u, v, w;
};

NriStruct(Filters) {
    Nri(Filter) min, mag, mip;
    Nri(FilterExt) ext; // requires "isTextureFilterMinMaxSupported"
};

NriStruct(SamplerDesc) {
    Nri(Filters) filters;
    uint8_t anisotropy;
    float mipBias;
    float mipMin;
    float mipMax;
    Nri(AddressModes) addressModes;
    Nri(CompareFunc) compareFunc;
    Nri(Color) borderColor;
    bool isInteger;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Pipeline ]
//============================================================================================================================================================================================

// It's recommended to use "NRICompatibility.hlsli" in the shader code
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
#pragma region [ Barrier ]
//============================================================================================================================================================================================

// If AgilitySDK is not available, "UNKNOWN" will be silently mapped to "COMMON", leading to discrepancies with VK
NriBits(AccessBits, uint16_t,                       // Compatible "StageBits" (including ALL):
    UNKNOWN                         = 0,
    INDEX_BUFFER                    = NriBit(0),    // INDEX_INPUT
    VERTEX_BUFFER                   = NriBit(1),    // VERTEX_SHADER
    CONSTANT_BUFFER                 = NriBit(2),    // GRAPHICS_SHADERS, COMPUTE_SHADER, RAY_TRACING_SHADERS
    SHADER_RESOURCE                 = NriBit(3),    // GRAPHICS_SHADERS, COMPUTE_SHADER, RAY_TRACING_SHADERS
    SHADER_RESOURCE_STORAGE         = NriBit(4),    // GRAPHICS_SHADERS, COMPUTE_SHADER, RAY_TRACING_SHADERS, CLEAR_STORAGE
    ARGUMENT_BUFFER                 = NriBit(5),    // INDIRECT
    COLOR_ATTACHMENT                = NriBit(6),    // COLOR_ATTACHMENT
    DEPTH_STENCIL_ATTACHMENT_WRITE  = NriBit(7),    // DEPTH_STENCIL_ATTACHMENT
    DEPTH_STENCIL_ATTACHMENT_READ   = NriBit(8),    // DEPTH_STENCIL_ATTACHMENT
    COPY_SOURCE                     = NriBit(9),    // COPY
    COPY_DESTINATION                = NriBit(10),   // COPY
    RESOLVE_SOURCE                  = NriBit(11),   // RESOLVE
    RESOLVE_DESTINATION             = NriBit(12),   // RESOLVE
    ACCELERATION_STRUCTURE_READ     = NriBit(13),   // COMPUTE_SHADER, RAY_TRACING_SHADERS, ACCELERATION_STRUCTURE
    ACCELERATION_STRUCTURE_WRITE    = NriBit(14),   // COMPUTE_SHADER, RAY_TRACING_SHADERS, ACCELERATION_STRUCTURE
    SHADING_RATE_ATTACHMENT         = NriBit(15)    // FRAGMENT_SHADER
);

// Not used if "isEnchancedBarrierSupported" is "false", i.e. no AgilitySDK
NriEnum(Layout, uint8_t,        // Compatible "AccessBits":
    UNKNOWN,
    COLOR_ATTACHMENT,           // COLOR_ATTACHMENT
    DEPTH_STENCIL_ATTACHMENT,   // DEPTH_STENCIL_ATTACHMENT_WRITE
    DEPTH_STENCIL_READONLY,     // DEPTH_STENCIL_ATTACHMENT_READ, SHADER_RESOURCE
    SHADER_RESOURCE,            // SHADER_RESOURCE
    SHADER_RESOURCE_STORAGE,    // SHADER_RESOURCE_STORAGE
    COPY_SOURCE,                // COPY_SOURCE
    COPY_DESTINATION,           // COPY_DESTINATION
    RESOLVE_SOURCE,             // RESOLVE_SOURCE
    RESOLVE_DESTINATION,        // RESOLVE_DESTINATION
    PRESENT,                    // UNKNOWN
    SHADING_RATE_ATTACHMENT     // SHADING_RATE_ATTACHMENT
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
    NriPtr(Buffer) buffer;
    Nri(AccessStage) before;
    Nri(AccessStage) after;
};

NriStruct(TextureBarrierDesc) {
    NriPtr(Texture) texture;
    Nri(AccessLayoutStage) before;
    Nri(AccessLayoutStage) after;
    Nri(Mip_t) mipOffset;
    Nri(Mip_t) mipNum;
    Nri(Dim_t) layerOffset;
    Nri(Dim_t) layerNum;
    Nri(PlaneBits) planes;
};

NriStruct(BarrierGroupDesc) {
    const NriPtr(GlobalBarrierDesc) globals;
    uint32_t globalNum;
    const NriPtr(BufferBarrierDesc) buffers;
    uint32_t bufferNum;
    const NriPtr(TextureBarrierDesc) textures;
    uint32_t textureNum;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Other ]
//============================================================================================================================================================================================

// Copy
NriStruct(TextureRegionDesc) {
    uint16_t x;
    uint16_t y;
    uint16_t z;
    Nri(Dim_t) width;
    Nri(Dim_t) height;
    Nri(Dim_t) depth;
    Nri(Mip_t) mipOffset;
    Nri(Dim_t) layerOffset;
};

NriStruct(TextureDataLayoutDesc) {
    uint64_t offset;
    uint32_t rowPitch; // must respect "uploadBufferTextureRowAlignment"
    uint32_t slicePitch; // must respect "uploadBufferTextureSliceAlignment"
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
};

// Memory
NriStruct(MemoryDesc) {
    uint64_t size;
    uint32_t alignment;
    Nri(MemoryType) type;
    bool mustBeDedicated; // must be put into a dedicated Memory, containing only 1 object with offset = 0
};

NriStruct(AllocateMemoryDesc) {
    uint64_t size;
    Nri(MemoryType) type;
    float priority; // [-1; 1]: low < 0, normal = 0, high > 0
};

NriStruct(BufferMemoryBindingDesc) {
    NriPtr(Memory) memory;
    NriPtr(Buffer) buffer;
    uint64_t offset;
};

NriStruct(TextureMemoryBindingDesc) {
    NriPtr(Memory) memory;
    NriPtr(Texture) texture;
    uint64_t offset;
};

// Clear storage
NriStruct(ClearStorageBufferDesc) {
    const NriPtr(Descriptor) storageBuffer;
    uint32_t value;
    uint32_t setIndex;
    uint32_t rangeIndex;
    uint32_t descriptorIndex;
};

NriStruct(ClearStorageTextureDesc) {
    const NriPtr(Descriptor) storageTexture;
    Nri(ClearValue) value;
    uint32_t setIndex;
    uint32_t rangeIndex;
    uint32_t descriptorIndex;
};

// Command signatures (default)
// To fill commands for indirect drawing in a shader use one of "NRI_FILL_X_DESC" macros
NriStruct(DrawDesc) { // see NRI_FILL_DRAW_COMMAND
    uint32_t vertexNum;
    uint32_t instanceNum;
    uint32_t baseVertex; // vertex buffer offset = CmdSetVertexBuffers.offset + baseVertex * VertexStreamDesc::stride
    uint32_t baseInstance;
};

NriStruct(DrawIndexedDesc) { // see NRI_FILL_DRAW_INDEXED_COMMAND
    uint32_t indexNum;
    uint32_t instanceNum;
    uint32_t baseIndex; // index buffer offset = CmdSetIndexBuffer.offset + baseIndex * sizeof(CmdSetIndexBuffer.indexType)
    int32_t baseVertex; // index += baseVertex
    uint32_t baseInstance;
};

NriStruct(DispatchDesc) {
    uint32_t x, y, z;
};

// Modified draw command signatures (D3D12 only)
//  If "DeviceDesc::isDrawParametersEmulationEnabled = true" (emulation globally enabled and allowed) and if a shader has "PipelineLayout::enableDrawParametersEmulation = true" (emulation requested)
//  - the following structs must be used instead
// - "NRI_ENABLE_DRAW_PARAMETERS_EMULATION" must be defined prior inclusion of "NRICompatibility.hlsli"
NriStruct(DrawBaseDesc) { // see NRI_FILL_DRAW_COMMAND
    uint32_t shaderEmulatedBaseVertex; // root constant
    uint32_t shaderEmulatedBaseInstance; // root constant
    uint32_t vertexNum;
    uint32_t instanceNum;
    uint32_t baseVertex; // vertex buffer offset = CmdSetVertexBuffers.offset + baseVertex * VertexStreamDesc::stride
    uint32_t baseInstance;
};

NriStruct(DrawIndexedBaseDesc) { // see NRI_FILL_DRAW_INDEXED_COMMAND
    int32_t shaderEmulatedBaseVertex; // root constant
    uint32_t shaderEmulatedBaseInstance; // root constant
    uint32_t indexNum;
    uint32_t instanceNum;
    uint32_t baseIndex; // index buffer offset = CmdSetIndexBuffer.offset + baseIndex * sizeof(CmdSetIndexBuffer.indexType)
    int32_t baseVertex; // index += baseVertex
    uint32_t baseInstance;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Queries ]
//============================================================================================================================================================================================

NriEnum(QueryType, uint8_t,
    TIMESTAMP,
    TIMESTAMP_COPY_QUEUE, // requires "isCopyQueueTimestampSupported"
    OCCLUSION,
    PIPELINE_STATISTICS,
    ACCELERATION_STRUCTURE_COMPACTED_SIZE
);

NriStruct(QueryPoolDesc) {
    Nri(QueryType) queryType;
    uint32_t capacity;
};

// Data layout for QueryType::PIPELINE_STATISTICS
// Never used, only describes the data layout for various cases
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

    // If "isMeshShaderPipelineStatsSupported"
    uint64_t meshControlShaderInvocationNum;
    uint64_t meshEvaluationShaderInvocationNum;

    // If "isMeshShaderPipelineStatsSupported" and D3D12
    uint64_t meshEvaluationShaderPrimitiveNum;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Device desc ]
//============================================================================================================================================================================================

NriEnum(Vendor, uint8_t,
    UNKNOWN,
    NVIDIA,
    AMD,
    INTEL
);

NriEnum(Architecture, uint8_t,
    UNKNOWN,
    DESCRETE,
    INTEGRATED // UMA
);

NriEnum(QueueType, uint8_t,
    GRAPHICS,
    COMPUTE,
    COPY
);

NriStruct(AdapterDesc) {
    char name[256];
    uint64_t luid;
    uint64_t videoMemorySize;
    uint64_t sharedSystemMemorySize;
    uint32_t deviceId;
    uint32_t queueNum[(uint32_t)NriScopedMember(QueueType, MAX_NUM)];
    Nri(Vendor) vendor;
    Nri(Architecture) architecture;
};

NriStruct(DeviceDesc) {
    // Common
    Nri(AdapterDesc) adapterDesc; // "queueNum" reflects available number of queues
    Nri(GraphicsAPI) graphicsAPI;
    uint16_t nriVersionMajor;
    uint16_t nriVersionMinor;

    // Viewports
    uint32_t viewportMaxNum;
    int32_t viewportBoundsRange[2];

    // Attachments
    Nri(Dim_t) attachmentMaxDim;
    Nri(Dim_t) attachmentLayerMaxNum;
    Nri(Dim_t) colorAttachmentMaxNum;

    // Multi-sampling
    Nri(Sample_t) colorSampleMaxNum;
    Nri(Sample_t) depthSampleMaxNum;
    Nri(Sample_t) stencilSampleMaxNum;
    Nri(Sample_t) zeroAttachmentsSampleMaxNum;
    Nri(Sample_t) textureColorSampleMaxNum;
    Nri(Sample_t) textureIntegerSampleMaxNum;
    Nri(Sample_t) textureDepthSampleMaxNum;
    Nri(Sample_t) textureStencilSampleMaxNum;
    Nri(Sample_t) storageTextureSampleMaxNum;

    // Resource dimensions
    Nri(Dim_t) texture1DMaxDim;
    Nri(Dim_t) texture2DMaxDim;
    Nri(Dim_t) texture3DMaxDim;
    Nri(Dim_t) textureArrayLayerMaxNum;
    uint32_t typedBufferMaxDim;

    // Memory
    uint64_t deviceUploadHeapSize; // ReBAR
    uint32_t memoryAllocationMaxNum;
    uint32_t samplerAllocationMaxNum;
    uint32_t constantBufferMaxRange;
    uint32_t storageBufferMaxRange;
    uint32_t bufferTextureGranularity;
    uint64_t bufferMaxSize;

    // Memory alignment
    uint32_t uploadBufferTextureRowAlignment;
    uint32_t uploadBufferTextureSliceAlignment;
    uint32_t bufferShaderResourceOffsetAlignment;
    uint32_t constantBufferOffsetAlignment;
    uint32_t scratchBufferOffsetAlignment;
    uint32_t shaderBindingTableAlignment;

    // Pipeline layout
    // D3D12 only: rootConstantSize + descriptorSetNum * 4 + rootDescriptorNum * 8 <= 256 (see "FitPipelineLayoutSettingsIntoDeviceLimits")
    uint32_t pipelineLayoutDescriptorSetMaxNum;
    uint32_t pipelineLayoutRootConstantMaxSize;
    uint32_t pipelineLayoutRootDescriptorMaxNum;

    // Descriptor set
    uint32_t descriptorSetSamplerMaxNum;
    uint32_t descriptorSetConstantBufferMaxNum;
    uint32_t descriptorSetStorageBufferMaxNum;
    uint32_t descriptorSetTextureMaxNum;
    uint32_t descriptorSetStorageTextureMaxNum;

    // Shader resources
    uint32_t perStageDescriptorSamplerMaxNum;
    uint32_t perStageDescriptorConstantBufferMaxNum;
    uint32_t perStageDescriptorStorageBufferMaxNum;
    uint32_t perStageDescriptorTextureMaxNum;
    uint32_t perStageDescriptorStorageTextureMaxNum;
    uint32_t perStageResourceMaxNum;

    // Vertex shader
    uint32_t vertexShaderAttributeMaxNum;
    uint32_t vertexShaderStreamMaxNum;
    uint32_t vertexShaderOutputComponentMaxNum;

    // Tessellation shaders
    float tessControlShaderGenerationMaxLevel;
    uint32_t tessControlShaderPatchPointMaxNum;
    uint32_t tessControlShaderPerVertexInputComponentMaxNum;
    uint32_t tessControlShaderPerVertexOutputComponentMaxNum;
    uint32_t tessControlShaderPerPatchOutputComponentMaxNum;
    uint32_t tessControlShaderTotalOutputComponentMaxNum;
    uint32_t tessEvaluationShaderInputComponentMaxNum;
    uint32_t tessEvaluationShaderOutputComponentMaxNum;

    // Geometry shader
    uint32_t geometryShaderInvocationMaxNum;
    uint32_t geometryShaderInputComponentMaxNum;
    uint32_t geometryShaderOutputComponentMaxNum;
    uint32_t geometryShaderOutputVertexMaxNum;
    uint32_t geometryShaderTotalOutputComponentMaxNum;

    // Fragment shader
    uint32_t fragmentShaderInputComponentMaxNum;
    uint32_t fragmentShaderOutputAttachmentMaxNum;
    uint32_t fragmentShaderDualSourceAttachmentMaxNum;

    // Compute shader
    uint32_t computeShaderSharedMemoryMaxSize;
    uint32_t computeShaderWorkGroupMaxNum[3];
    uint32_t computeShaderWorkGroupInvocationMaxNum;
    uint32_t computeShaderWorkGroupMaxDim[3];

    // Ray tracing
    uint32_t rayTracingShaderGroupIdentifierSize;
    uint32_t rayTracingShaderTableMaxStride;
    uint32_t rayTracingShaderRecursionMaxDepth;
    uint32_t rayTracingGeometryObjectMaxNum;

    // Mesh shaders
    uint32_t meshControlSharedMemoryMaxSize;
    uint32_t meshControlWorkGroupInvocationMaxNum;
    uint32_t meshControlPayloadMaxSize;
    uint32_t meshEvaluationOutputVerticesMaxNum;
    uint32_t meshEvaluationOutputPrimitiveMaxNum;
    uint32_t meshEvaluationOutputComponentMaxNum;
    uint32_t meshEvaluationSharedMemoryMaxSize;
    uint32_t meshEvaluationWorkGroupInvocationMaxNum;

    // Precision bits
    uint32_t viewportPrecisionBits;
    uint32_t subPixelPrecisionBits;
    uint32_t subTexelPrecisionBits;
    uint32_t mipmapPrecisionBits;

    // Other
    uint64_t timestampFrequencyHz;
    uint32_t drawIndirectMaxNum;
    float samplerLodBiasMin;
    float samplerLodBiasMax;
    float samplerAnisotropyMax;
    int32_t texelOffsetMin;
    uint32_t texelOffsetMax;
    int32_t texelGatherOffsetMin;
    uint32_t texelGatherOffsetMax;
    uint32_t clipDistanceMaxNum;
    uint32_t cullDistanceMaxNum;
    uint32_t combinedClipAndCullDistanceMaxNum;
    uint32_t viewMaxNum;                            // aka multiview
    uint8_t shadingRateAttachmentTileSize;          // square size
    uint8_t shaderModel;                            // major * 10 + minor

    // Tiers (0 - unsupported)

    // 1 - 1/2 pixel uncertainty region and does not support post-snap degenerates
    // 2 - reduces the maximum uncertainty region to 1/256 and requires post-snap degenerates not be culled
    // 3 - maintains a maximum 1/256 uncertainty region and adds support for inner input coverage, aka "SV_InnerCoverage"
    uint8_t conservativeRasterTier;

    // 1 - a single sample pattern can be specified to repeat for every pixel ("locationNum / sampleNum" must be 1 in "CmdSetSampleLocations")
    // 2 - four separate sample patterns can be specified for each pixel in a 2x2 grid ("locationNum / sampleNum" can be up to 4 in "CmdSetSampleLocations")
    uint8_t sampleLocationsTier;

    // 1 - DXR 1.0: full raytracing functionality, except features below
    // 2 - DXR 1.1: adds - ray query, "CmdDispatchRaysIndirect", "GeometryIndex()" intrinsic, additional ray flags & vertex formats
    uint8_t rayTracingTier;

    // 1 - shading rate can be specified only per draw
    // 2 - adds: per primitive shading rate, per "shadingRateAttachmentTileSize" shading rate, combiners, "SV_ShadingRate" support
    uint8_t shadingRateTier;

    // 1 - unbound arrays with dynamic indexing
    // 2 - D3D12 dynamic resources: https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html
    uint8_t bindlessTier;

    // Features
    uint32_t isGetMemoryDesc2Supported : 1;             // D3D: always supported, VK: requires "maintenance4" support
    uint32_t isEnchancedBarrierSupported : 1;           // aka can "Layout" be ignored?
    uint32_t isMemoryTier2Supported : 1;                // a memory object can support resources from all 3 categories (buffers, attachments, all other textures)

    uint32_t isIndependentFrontAndBackStencilReferenceAndMasksSupported : 1; // see "StencilAttachmentDesc::back"
    uint32_t isTextureFilterMinMaxSupported : 1;        // see "FilterExt"
    uint32_t isLogicFuncSupported : 1;                  // see "LogicFunc"
    uint32_t isDepthBoundsTestSupported : 1;            // see "DepthAttachmentDesc::boundsTest"
    uint32_t isDrawIndirectCountSupported : 1;          // see "countBuffer" and "countBufferOffset"
    uint32_t isLineSmoothingSupported : 1;              // see "RasterizationDesc::lineSmoothing"
    uint32_t isCopyQueueTimestampSupported : 1;         // see "QueryType::TIMESTAMP_COPY_QUEUE"
    uint32_t isMeshShaderPipelineStatsSupported : 1;    // see "PipelineStatisticsDesc"
    uint32_t isDynamicDepthBiasSupported : 1;           // see "CmdSetDepthBias"
    uint32_t isAdditionalShadingRatesSupported : 1;     // see "ShadingRate"
    uint32_t isViewportOriginBottomLeftSupported : 1;   // see "Viewport"
    uint32_t isRegionResolveSupported : 1;              // see "CmdResolveTexture"
    uint32_t isFlexibleMultiviewSupported : 1;          // see "Multiview::FLEXIBLE"
    uint32_t isLayerBasedMultiviewSupported : 1;        // see "Multiview::LAYRED_BASED"
    uint32_t isViewportBasedMultiviewSupported : 1;     // see "Multiview::VIEWPORT_BASED"
    uint32_t isPresentFromComputeSupported : 1;         // see "SwapChainDesc::queue"
    uint32_t isWaitableSwapChainSupported : 1;          // see "SwapChainDesc::waitable"

    // Shader features (I32 + atomics and F32 are always supported)
    uint32_t isShaderNativeI16Supported : 1;
    uint32_t isShaderNativeF16Supported : 1;
    uint32_t isShaderNativeI64Supported : 1;
    uint32_t isShaderNativeF64Supported : 1;
    uint32_t isShaderAtomicsI16Supported : 1;
    uint32_t isShaderAtomicsF16Supported : 1;
    uint32_t isShaderAtomicsF32Supported : 1;
    uint32_t isShaderAtomicsI64Supported : 1;
    uint32_t isShaderAtomicsF64Supported : 1;
    uint32_t isShaderViewportIndexSupported : 1;        // always can be used from geometry shaders
    uint32_t isShaderLayerSupported : 1;                // always can be used from geometry shaders
    uint32_t isShaderClockSupported : 1;
    uint32_t isRasterizedOrderedViewSupported : 1;      // ROV, aka fragment shader interlock
    uint32_t isBarycentricSupported : 1;
    uint32_t isRayTracingPositionFetchSupported : 1;    // currently VK only

    // Extensions (unexposed are always supported)
    uint32_t isSwapChainSupported : 1;                  // NRISwapChain
    uint32_t isRayTracingSupported : 1;                 // NRIRayTracing
    uint32_t isMeshShaderSupported : 1;                 // NRIMeshShader
    uint32_t isLowLatencySupported : 1;                 // NRILowLatency
};

#pragma endregion

NriNamespaceEnd
