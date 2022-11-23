/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#define NRI_DECLARE_ENUM_CLASS_WITH_OPS(EnumClass, Type) \
    enum class EnumClass : Type; \
    constexpr Type operator & (EnumClass val0, EnumClass val1) { return (Type)val0 & (Type)val1; } \
    constexpr EnumClass operator | (EnumClass val0, EnumClass val1) { return (EnumClass)((Type)val0 | (Type)val1); } \
    constexpr EnumClass& operator &= (EnumClass& val0, EnumClass val1) { val0 = (EnumClass)(val0 & val1); return val0; } \
    constexpr EnumClass& operator |= (EnumClass& val0, EnumClass val1) { val0 = (EnumClass)(val0 | val1); return val0; } \
    enum class EnumClass : Type

namespace nri
{
    struct Buffer;
    struct CommandQueue;
    struct CommandBuffer;
    struct CommandAllocator;
    struct Device;
    struct Descriptor;
    struct DescriptorPool;
    struct DescriptorSet;
    struct DeviceSemaphore;
    struct FrameBuffer;
    struct Memory;
    struct PipelineLayout;
    struct Pipeline;
    struct QueueSemaphore;
    struct QueryPool;
    struct Texture;

    constexpr uint32_t SetBit(uint32_t index)
    {
        return 1 << index;
    }

    constexpr uint16_t REMAINING_ARRAY_LAYERS = 0;
    constexpr uint16_t REMAINING_MIP_LEVELS = 0;
    constexpr uint16_t WHOLE_SIZE = 0;
    constexpr uint32_t WHOLE_DEVICE_GROUP = 0;
    constexpr bool VARIABLE_DESCRIPTOR_NUM = true;
    constexpr bool DESCRIPTOR_ARRAY = true;

    enum class Result : uint8_t
    {
        SUCCESS,
        FAILURE,
        INVALID_ARGUMENT,
        OUT_OF_MEMORY,
        UNSUPPORTED,
        DEVICE_LOST,
        SWAPCHAIN_RESIZE,

        MAX_NUM
    };

    enum class Vendor : uint8_t
    {
        UNKNOWN,
        NVIDIA,
        AMD,
        INTEL,

        MAX_NUM
    };

    enum class GraphicsAPI : uint8_t
    {
        D3D11,
        D3D12,
        VULKAN,

        MAX_NUM
    };

    enum class CommandQueueType : uint8_t
    {
        GRAPHICS,
        COMPUTE,
        COPY,

        MAX_NUM
    };

    enum class MemoryLocation : uint8_t
    {
        DEVICE,
        HOST_UPLOAD,
        HOST_READBACK,

        MAX_NUM
    };

    enum class TextureType : uint8_t
    {
        TEXTURE_1D,
        TEXTURE_2D,
        TEXTURE_3D,

        MAX_NUM
    };

    enum class Texture1DViewType : uint8_t
    {
        SHADER_RESOURCE_1D,
        SHADER_RESOURCE_1D_ARRAY,
        SHADER_RESOURCE_STORAGE_1D,
        SHADER_RESOURCE_STORAGE_1D_ARRAY,
        COLOR_ATTACHMENT,
        DEPTH_STENCIL_ATTACHMENT,

        MAX_NUM
    };

    enum class Texture2DViewType : uint8_t
    {
        SHADER_RESOURCE_2D,
        SHADER_RESOURCE_2D_ARRAY,
        SHADER_RESOURCE_CUBE,
        SHADER_RESOURCE_CUBE_ARRAY,
        SHADER_RESOURCE_STORAGE_2D,
        SHADER_RESOURCE_STORAGE_2D_ARRAY,
        COLOR_ATTACHMENT,
        DEPTH_STENCIL_ATTACHMENT,

        MAX_NUM
    };

    enum class Texture3DViewType : uint8_t
    {
        SHADER_RESOURCE_3D,
        SHADER_RESOURCE_STORAGE_3D,
        COLOR_ATTACHMENT,

        MAX_NUM
    };

    enum class BufferViewType : uint8_t
    {
        SHADER_RESOURCE,
        SHADER_RESOURCE_STORAGE,
        CONSTANT,

        MAX_NUM
    };

    enum class DescriptorType : uint8_t
    {
        SAMPLER,
        CONSTANT_BUFFER,
        TEXTURE,
        STORAGE_TEXTURE,
        BUFFER,
        STORAGE_BUFFER,
        STRUCTURED_BUFFER,
        STORAGE_STRUCTURED_BUFFER,
        ACCELERATION_STRUCTURE,

        MAX_NUM
    };

    enum class VertexStreamStepRate : uint8_t
    {
        PER_VERTEX,
        PER_INSTANCE,

        MAX_NUM
    };

    enum class TextureLayout : uint8_t
    {
        GENERAL,
        COLOR_ATTACHMENT,
        DEPTH_STENCIL,
        DEPTH_STENCIL_READONLY,
        DEPTH_READONLY,
        STENCIL_READONLY,
        SHADER_RESOURCE,
        PRESENT,
        UNKNOWN,

        MAX_NUM
    };

    enum class ShaderStage : uint8_t
    {
        ALL,
        VERTEX,
        TESS_CONTROL,
        TESS_EVALUATION,
        GEOMETRY,
        FRAGMENT,
        COMPUTE,
        RAYGEN,
        MISS,
        INTERSECTION,
        CLOSEST_HIT,
        ANY_HIT,
        CALLABLE,
        MESH_CONTROL,
        MESH_EVALUATION,

        MAX_NUM
    };

    enum class BarrierDependency : uint8_t
    {
        ALL_STAGES,
        GRAPHICS_STAGE,
        COMPUTE_STAGE,
        COPY_STAGE,
        RAYTRACING_STAGE,

        MAX_NUM
    };

    enum class Topology : uint8_t
    {
        POINT_LIST,
        LINE_LIST,
        LINE_STRIP,
        TRIANGLE_LIST,
        TRIANGLE_STRIP,
        LINE_LIST_WITH_ADJACENCY,
        LINE_STRIP_WITH_ADJACENCY,
        TRIANGLE_LIST_WITH_ADJACENCY,
        TRIANGLE_STRIP_WITH_ADJACENCY,
        PATCH_LIST,

        MAX_NUM
    };

    enum class FillMode : uint8_t
    {
        SOLID,
        WIREFRAME,

        MAX_NUM
    };

    enum class CullMode : uint8_t
    {
        NONE,
        FRONT,
        BACK,

        MAX_NUM
    };

    enum class LogicFunc : uint8_t
    {
        NONE,
        CLEAR,
        AND,
        AND_REVERSE,
        COPY,
        AND_INVERTED,
        XOR,
        OR,
        NOR,
        EQUIVALENT,
        INVERT,
        OR_REVERSE,
        COPY_INVERTED,
        OR_INVERTED,
        NAND,
        SET,

        MAX_NUM
    };

    enum class CompareFunc : uint8_t
    {
        NONE,
        ALWAYS,
        NEVER,
        LESS,
        LESS_EQUAL,
        EQUAL,
        GREATER_EQUAL,
        GREATER,
        NOT_EQUAL,

        MAX_NUM
    };

    enum class StencilFunc : uint8_t
    {
        KEEP,
        ZERO,
        REPLACE,
        INCREMENT_AND_CLAMP,
        DECREMENT_AND_CLAMP,
        INVERT,
        INCREMENT_AND_WRAP,
        DECREMENT_AND_WRAP,

        MAX_NUM
    };

    enum class BlendFactor : uint8_t
    {
        ZERO,
        ONE,
        SRC_COLOR,
        ONE_MINUS_SRC_COLOR,
        DST_COLOR,
        ONE_MINUS_DST_COLOR,
        SRC_ALPHA,
        ONE_MINUS_SRC_ALPHA,
        DST_ALPHA,
        ONE_MINUS_DST_ALPHA,
        CONSTANT_COLOR,
        ONE_MINUS_CONSTANT_COLOR,
        CONSTANT_ALPHA,
        ONE_MINUS_CONSTANT_ALPHA,
        SRC_ALPHA_SATURATE,
        SRC1_COLOR,
        ONE_MINUS_SRC1_COLOR,
        SRC1_ALPHA,
        ONE_MINUS_SRC1_ALPHA,

        MAX_NUM
    };

    enum class BlendFunc : uint8_t
    {
        ADD,
        SUBTRACT,
        REVERSE_SUBTRACT,
        MIN,
        MAX,

        MAX_NUM
    };

    enum class IndexType : uint8_t
    {
        UINT16,
        UINT32,

        MAX_NUM
    };

    enum class QueryType : uint8_t
    {
        TIMESTAMP,
        OCCLUSION,
        PIPELINE_STATISTICS,
        ACCELERATION_STRUCTURE_COMPACTED_SIZE,

        MAX_NUM
    };

    enum class Filter : uint8_t
    {
        NEAREST,
        LINEAR,

        MAX_NUM
    };

    enum class FilterExt : uint8_t
    {
        NONE,
        MIN,
        MAX,

        MAX_NUM
    };

    enum class AddressMode : uint8_t
    {
        REPEAT,
        MIRRORED_REPEAT,
        CLAMP_TO_EDGE,
        CLAMP_TO_BORDER,

        MAX_NUM
    };

    enum class BorderColor : uint8_t
    {
        FLOAT_TRANSPARENT_BLACK,
        FLOAT_OPAQUE_BLACK,
        FLOAT_OPAQUE_WHITE,
        INT_TRANSPARENT_BLACK,
        INT_OPAQUE_BLACK,
        INT_OPAQUE_WHITE,

        MAX_NUM
    };

    enum class Format : uint8_t
    {
        UNKNOWN,

        R8_UNORM,
        R8_SNORM,
        R8_UINT,
        R8_SINT,

        RG8_UNORM,
        RG8_SNORM,
        RG8_UINT,
        RG8_SINT,

        BGRA8_UNORM,
        BGRA8_SRGB,

        RGBA8_UNORM,
        RGBA8_SNORM,
        RGBA8_UINT,
        RGBA8_SINT,
        RGBA8_SRGB,

        R16_UNORM,
        R16_SNORM,
        R16_UINT,
        R16_SINT,
        R16_SFLOAT,

        RG16_UNORM,
        RG16_SNORM,
        RG16_UINT,
        RG16_SINT,
        RG16_SFLOAT,

        RGBA16_UNORM,
        RGBA16_SNORM,
        RGBA16_UINT,
        RGBA16_SINT,
        RGBA16_SFLOAT,

        R32_UINT,
        R32_SINT,
        R32_SFLOAT,

        RG32_UINT,
        RG32_SINT,
        RG32_SFLOAT,

        RGB32_UINT,
        RGB32_SINT,
        RGB32_SFLOAT,

        RGBA32_UINT,
        RGBA32_SINT,
        RGBA32_SFLOAT,

        R10_G10_B10_A2_UNORM,
        R10_G10_B10_A2_UINT,
        R11_G11_B10_UFLOAT,
        R9_G9_B9_E5_UFLOAT,

        BC1_RGBA_UNORM,
        BC1_RGBA_SRGB,
        BC2_RGBA_UNORM,
        BC2_RGBA_SRGB,
        BC3_RGBA_UNORM,
        BC3_RGBA_SRGB,
        BC4_R_UNORM,
        BC4_R_SNORM,
        BC5_RG_UNORM,
        BC5_RG_SNORM,
        BC6H_RGB_UFLOAT,
        BC6H_RGB_SFLOAT,
        BC7_RGBA_UNORM,
        BC7_RGBA_SRGB,

        // DEPTH_STENCIL_ATTACHMENT views
        D16_UNORM,
        D24_UNORM_S8_UINT,
        D32_SFLOAT,
        D32_SFLOAT_S8_UINT_X24,

        // Depth-stencil specific SHADER_RESOURCE views
        R24_UNORM_X8,
        X24_R8_UINT,
        X32_R8_UINT_X24,
        R32_SFLOAT_X8_X24,

        MAX_NUM
    };

    typedef uint32_t MemoryType;

    enum class AttachmentContentType : uint8_t
    {
        COLOR,
        DEPTH,
        STENCIL,
        DEPTH_STENCIL,

        MAX_NUM
    };

    enum class RenderPassBeginFlag : uint8_t
    {
        NONE,
        SKIP_FRAME_BUFFER_CLEAR,

        MAX_NUM
    };

    enum class PrimitiveRestart : uint8_t
    {
        DISABLED,
        INDICES_UINT16,
        INDICES_UINT32,

        MAX_NUM
    };

    NRI_DECLARE_ENUM_CLASS_WITH_OPS(TextureUsageBits, uint16_t)
    {
        NONE                                = 0,
        SHADER_RESOURCE                     = SetBit(0),
        SHADER_RESOURCE_STORAGE             = SetBit(1),
        COLOR_ATTACHMENT                    = SetBit(2),
        DEPTH_STENCIL_ATTACHMENT            = SetBit(3)
    };

    NRI_DECLARE_ENUM_CLASS_WITH_OPS(BufferUsageBits, uint16_t)
    {
        NONE                                = 0,
        SHADER_RESOURCE                     = SetBit(0),
        SHADER_RESOURCE_STORAGE             = SetBit(1),
        VERTEX_BUFFER                       = SetBit(2),
        INDEX_BUFFER                        = SetBit(3),
        CONSTANT_BUFFER                     = SetBit(4),
        ARGUMENT_BUFFER                     = SetBit(5),
        RAY_TRACING_BUFFER                  = SetBit(6),
        ACCELERATION_STRUCTURE_BUILD_READ   = SetBit(7)
    };

    NRI_DECLARE_ENUM_CLASS_WITH_OPS(AccessBits, uint16_t)
    {
        UNKNOWN                             = 0,
        VERTEX_BUFFER                       = SetBit(0),
        INDEX_BUFFER                        = SetBit(1),
        CONSTANT_BUFFER                     = SetBit(2),
        ARGUMENT_BUFFER                     = SetBit(3),
        SHADER_RESOURCE                     = SetBit(4),
        SHADER_RESOURCE_STORAGE             = SetBit(5),
        COLOR_ATTACHMENT                    = SetBit(6),
        DEPTH_STENCIL_WRITE                 = SetBit(7),
        DEPTH_STENCIL_READ                  = SetBit(8),
        COPY_SOURCE                         = SetBit(9),
        COPY_DESTINATION                    = SetBit(10),
        ACCELERATION_STRUCTURE_READ         = SetBit(11),
        ACCELERATION_STRUCTURE_WRITE        = SetBit(12)
    };

    NRI_DECLARE_ENUM_CLASS_WITH_OPS(PipelineLayoutShaderStageBits, uint16_t)
    {
        NONE                                = 0,
        VERTEX                              = SetBit(1),
        TESS_CONTROL                        = SetBit(2),
        TESS_EVALUATION                     = SetBit(3),
        GEOMETRY                            = SetBit(4),
        FRAGMENT                            = SetBit(5),
        COMPUTE                             = SetBit(6),
        RAYGEN                              = SetBit(7),
        MISS                                = SetBit(8),
        INTERSECTION                        = SetBit(9),
        CLOSEST_HIT                         = SetBit(10),
        ANY_HIT                             = SetBit(11),
        CALLABLE                            = SetBit(12),
        MESH_CONTROL                        = SetBit(13),
        MESH_EVALUATION                     = SetBit(14),
        ALL_GRAPHICS                        = VERTEX | TESS_CONTROL | TESS_EVALUATION | GEOMETRY | FRAGMENT | MESH_CONTROL | MESH_EVALUATION,
        ALL_RAY_TRACING                     = RAYGEN | MISS | INTERSECTION | CLOSEST_HIT | ANY_HIT | CALLABLE
    };

    NRI_DECLARE_ENUM_CLASS_WITH_OPS(PipelineStatsBits, uint16_t)
    {
        INPUT_ASSEMBLY_VERTICES             = SetBit(0),
        INPUT_ASSEMBLY_PRIMITIVES           = SetBit(1),
        VERTEX_SHADER_INVOCATIONS           = SetBit(2),
        GEOMETRY_SHADER_INVOCATIONS         = SetBit(3),
        GEOMETRY_SHADER_PRIMITIVES          = SetBit(4),
        CLIPPING_INVOCATIONS                = SetBit(5),
        CLIPPING_PRIMITIVES                 = SetBit(6),
        FRAGMENT_SHADER_INVOCATIONS         = SetBit(7),
        TESS_CONTROL_SHADER_PATCHES         = SetBit(8),
        TESS_EVALUATION_SHADER_INVOCATIONS  = SetBit(9),
        COMPUTE_SHADER_INVOCATIONS          = SetBit(10)
    };

    NRI_DECLARE_ENUM_CLASS_WITH_OPS(ColorWriteBits, uint8_t)
    {
        R                                   = SetBit(0),
        G                                   = SetBit(1),
        B                                   = SetBit(2),
        A                                   = SetBit(3),
        RGBA                                = R | G | B | A
    };

    NRI_DECLARE_ENUM_CLASS_WITH_OPS(ResourceViewBits, uint8_t)
    {
        READONLY_DEPTH                      = SetBit(0),
        READONLY_STENCIL                    = SetBit(1)
    };

    NRI_DECLARE_ENUM_CLASS_WITH_OPS(FormatSupportBits, uint8_t)
    {
        UNSUPPORTED                         = 0,
        TEXTURE                             = SetBit(0),
        STORAGE_TEXTURE                     = SetBit(1),
        BUFFER                              = SetBit(2),
        STORAGE_BUFFER                      = SetBit(3),
        COLOR_ATTACHMENT                    = SetBit(4),
        DEPTH_STENCIL_ATTACHMENT            = SetBit(5),
        VERTEX_BUFFER                       = SetBit(6)
    };

    struct Rect
    {
        int32_t left;
        int32_t top;
        uint32_t width;
        uint32_t height;
    };

    struct Viewport
    {
        float offset[2];
        float size[2];
        float depthRangeMin;
        float depthRangeMax;
    };

    struct Color32f
    {
        float x, y, z, w;
    };

    struct Color32ui
    {
        uint32_t x, y, z, w;
    };

    struct Color32i
    {
        int32_t x, y, z, w;
    };

    struct DepthStencilClearValue
    {
        float depth;
        uint8_t stencil;
    };

    union ClearValueDesc
    {
        DepthStencilClearValue depthStencil;
        Color32f color32f;
        Color32ui color32ui;
        Color32i color32i;
    };

    struct ClearDesc
    {
        ClearValueDesc value;
        AttachmentContentType attachmentContentType;
        uint32_t colorAttachmentIndex;
    };

    struct ClearStorageBufferDesc
    {
        const Descriptor* storageBuffer;
        uint32_t value;
        uint32_t setIndex;
        uint32_t rangeIndex;
        uint32_t offsetInRange;
    };

    struct ClearStorageTextureDesc
    {
        const Descriptor* storageTexture;
        ClearValueDesc value;
        uint32_t setIndex;
        uint32_t rangeIndex;
        uint32_t offsetInRange;
    };

    struct TextureRegionDesc
    {
        uint16_t offset[3];
        uint16_t size[3];
        uint16_t mipOffset;
        uint16_t arrayOffset;
    };

    struct TextureDataLayoutDesc
    {
        uint64_t offset;
        uint32_t rowPitch;
        uint32_t slicePitch;
    };

    struct WorkSubmissionDesc
    {
        const CommandBuffer* const* commandBuffers;
        const QueueSemaphore* const* wait;
        const QueueSemaphore* const* signal;
        uint32_t commandBufferNum;
        uint32_t waitNum;
        uint32_t signalNum;
        uint32_t physicalDeviceIndex;
    };

    struct BufferMemoryBindingDesc
    {
        Memory* memory;
        Buffer* buffer;
        uint64_t offset;
        uint32_t physicalDeviceMask;
    };

    struct TextureMemoryBindingDesc
    {
        Memory* memory;
        Texture* texture;
        uint64_t offset;
        uint32_t physicalDeviceMask;
    };

    struct MemoryDesc
    {
        uint64_t size;
        uint32_t alignment;
        MemoryType type;
        bool mustBeDedicated;
    };

    struct AddressModes
    {
        AddressMode u;
        AddressMode v;
        AddressMode w;
    };

    struct SamplerDesc
    {
        Filter magnification;
        Filter minification;
        Filter mip;
        FilterExt filterExt;
        uint32_t anisotropy;
        float mipBias;
        float mipMin;
        float mipMax;
        AddressModes addressModes;
        CompareFunc compareFunc;
        BorderColor borderColor;
        bool unnormalizedCoordinates : 1;
    };

    struct TextureDesc
    {
        TextureType type;
        TextureUsageBits usageMask;
        Format format;
        uint16_t size[3];
        uint16_t mipNum;
        uint16_t arraySize;
        uint8_t sampleNum;
        uint32_t physicalDeviceMask;
    };

    struct BufferDesc
    {
        uint64_t size;
        uint32_t structureStride;
        BufferUsageBits usageMask;
        uint32_t physicalDeviceMask;
    };

    struct Texture1DViewDesc
    {
        const Texture* texture;
        Texture1DViewType viewType;
        Format format;
        uint16_t mipOffset;
        uint16_t mipNum;
        uint16_t arrayOffset;
        uint16_t arraySize;
        uint32_t physicalDeviceMask;
        ResourceViewBits flags;
    };

    struct Texture2DViewDesc
    {
        const Texture* texture;
        Texture2DViewType viewType;
        Format format;
        uint16_t mipOffset;
        uint16_t mipNum;
        uint16_t arrayOffset;
        uint16_t arraySize;
        uint32_t physicalDeviceMask;
        ResourceViewBits flags;
    };

    struct Texture3DViewDesc
    {
        const Texture* texture;
        Texture3DViewType viewType;
        Format format;
        uint16_t mipOffset;
        uint16_t mipNum;
        uint16_t sliceOffset;
        uint16_t sliceNum;
        uint32_t physicalDeviceMask;
        ResourceViewBits flags;
    };

    struct BufferViewDesc
    {
        const Buffer* buffer;
        BufferViewType viewType;
        Format format;
        uint64_t offset;
        uint64_t size;
        uint32_t physicalDeviceMask;
    };

    struct DescriptorPoolDesc
    {
        uint32_t physicalDeviceMask;
        uint32_t descriptorSetMaxNum;
        uint32_t samplerMaxNum;
        uint32_t staticSamplerMaxNum;
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

    struct TextureTransitionBarrierDesc
    {
        const Texture* texture;
        uint16_t mipOffset;
        uint16_t mipNum;
        uint16_t arrayOffset;
        uint16_t arraySize;
        AccessBits prevAccess;
        AccessBits nextAccess;
        TextureLayout prevLayout;
        TextureLayout nextLayout;
    };

    struct BufferTransitionBarrierDesc
    {
        const Buffer* buffer;
        AccessBits prevAccess;
        AccessBits nextAccess;
    };

    struct BufferAliasingBarrierDesc
    {
        const Buffer* before;
        const Buffer* after;
        AccessBits nextAccess;
    };

    struct TextureAliasingBarrierDesc
    {
        const Texture* before;
        const Texture* after;
        AccessBits nextAccess;
        TextureLayout nextLayout;
    };

    struct TransitionBarrierDesc
    {
        const BufferTransitionBarrierDesc* buffers;
        const TextureTransitionBarrierDesc* textures;
        uint32_t bufferNum;
        uint32_t textureNum;
    };

    struct AliasingBarrierDesc
    {
        const BufferAliasingBarrierDesc* buffers;
        const TextureAliasingBarrierDesc* textures;
        uint32_t bufferNum;
        uint32_t textureNum;
    };

    struct DescriptorRangeDesc
    {
        uint32_t baseRegisterIndex;
        uint32_t descriptorNum;
        DescriptorType descriptorType;
        ShaderStage visibility;
        bool isDescriptorNumVariable;
        bool isArray;
    };

    struct DynamicConstantBufferDesc
    {
        uint32_t registerIndex;
        ShaderStage visibility;
    };

    struct StaticSamplerDesc
    {
        SamplerDesc samplerDesc;
        uint32_t registerIndex;
        ShaderStage visibility;
    };

    struct DescriptorSetDesc
    {
        const DescriptorRangeDesc* ranges;
        uint32_t rangeNum;
        const StaticSamplerDesc* staticSamplers;
        uint32_t staticSamplerNum;
        const DynamicConstantBufferDesc* dynamicConstantBuffers;
        uint32_t dynamicConstantBufferNum;
    };

    struct DescriptorRangeUpdateDesc
    {
        const Descriptor* const* descriptors;
        uint32_t descriptorNum;
        uint32_t offsetInRange;
    };

    struct DescriptorSetCopyDesc
    {
        const DescriptorSet* srcDescriptorSet;
        uint32_t baseSrcRange;
        uint32_t baseDstRange;
        uint32_t rangeNum;
        uint32_t baseSrcDynamicConstantBuffer;
        uint32_t baseDstDynamicConstantBuffer;
        uint32_t dynamicConstantBufferNum;
        uint32_t physicalDeviceMask;
    };

    struct PushConstantDesc
    {
        uint32_t registerIndex;
        uint32_t size;
        ShaderStage visibility;
    };

    struct SPIRVBindingOffsets
    {
        uint32_t samplerOffset;
        uint32_t textureOffset;
        uint32_t constantBufferOffset;
        uint32_t storageTextureAndBufferOffset;
    };

    struct ShaderDesc
    {
        ShaderStage stage;
        const void* bytecode;
        uint64_t size;
        const char* entryPointName;
    };

    struct VertexAttributeD3D
    {
        const char* semanticName;
        uint32_t semanticIndex;
    };

    struct VertexAttributeVK
    {
        uint32_t location;
    };

    struct VertexAttributeDesc
    {
        VertexAttributeD3D d3d;
        VertexAttributeVK vk;
        uint32_t offset;
        Format format;
        uint16_t streamIndex;
    };

    struct VertexStreamDesc
    {
        uint32_t stride;
        uint16_t bindingSlot;
        VertexStreamStepRate stepRate;
    };

   struct InputAssemblyDesc
    {
        const VertexAttributeDesc* attributes;
        const VertexStreamDesc* streams;
        uint8_t attributeNum;
        uint8_t streamNum;
        Topology topology;
        uint8_t tessControlPointNum;
        PrimitiveRestart primitiveRestart;
    };

    struct SamplePosition
    {
        int8_t x;
        int8_t y;
    };

    struct RasterizationDesc
    {
        uint32_t viewportNum;
        int32_t depthBiasConstantFactor;
        float depthBiasClamp;
        float depthBiasSlopeFactor;
        FillMode fillMode;
        CullMode cullMode;
        uint16_t sampleMask;
        uint8_t sampleNum;
        bool alphaToCoverage : 1;
        bool frontCounterClockwise : 1;
        bool depthClamp : 1;
        bool antialiasedLines : 1;
        bool rasterizerDiscard : 1;
        bool conservativeRasterization : 1;
    };

    struct StencilDesc
    {
        CompareFunc compareFunc;
        StencilFunc fail;
        StencilFunc pass;
        StencilFunc depthFail;
    };

    struct BlendingDesc
    {
        BlendFactor srcFactor;
        BlendFactor dstFactor;
        BlendFunc func;
    };

    struct ColorAttachmentDesc
    {
        Format format;
        BlendingDesc colorBlend;
        BlendingDesc alphaBlend;
        ColorWriteBits colorWriteMask;
        bool blendEnabled : 1;
    };

    // CompareFunc::NONE = depth/stencil test disabled
    struct DepthAttachmentDesc
    {
        CompareFunc compareFunc;
        bool write;
    };

    struct StencilAttachmentDesc
    {
        StencilDesc front;
        StencilDesc back;
        uint8_t reference;
        uint8_t compareMask;
        uint8_t writeMask;
    };

    struct OutputMergerDesc
    {
        const ColorAttachmentDesc* color;
        DepthAttachmentDesc depth;
        StencilAttachmentDesc stencil;
        Format depthStencilFormat;
        LogicFunc colorLogicFunc;
        uint32_t colorNum;
        Color32f blendConsts;
    };

    struct PipelineLayoutDesc
    {
        const DescriptorSetDesc* descriptorSets;
        const PushConstantDesc* pushConstants;
        uint32_t descriptorSetNum;
        uint32_t pushConstantNum;
        PipelineLayoutShaderStageBits stageMask;
        bool ignoreGlobalSPIRVOffsets;
    };

    struct GraphicsPipelineDesc
    {
        const PipelineLayout* pipelineLayout;
        const InputAssemblyDesc* inputAssembly;
        const RasterizationDesc* rasterization;
        const OutputMergerDesc* outputMerger;
        const ShaderDesc* shaderStages;
        uint32_t shaderStageNum;
    };

    struct ComputePipelineDesc
    {
        const PipelineLayout* pipelineLayout;
        ShaderDesc computeShader;
    };

    struct FrameBufferDesc
    {
        const Descriptor* const* colorAttachments;
        const Descriptor* depthStencilAttachment;
        const ClearValueDesc* colorClearValues;
        const ClearValueDesc* depthStencilClearValue;
        uint32_t colorAttachmentNum;
        uint32_t physicalDeviceMask;
        uint16_t size[2];
        uint16_t layerNum;
    };

    struct QueryPoolDesc
    {
        QueryType queryType;
        uint32_t capacity;
        PipelineStatsBits pipelineStatsMask;
        uint32_t physicalDeviceMask;
    };

    struct PipelineStatisticsDesc
    {
        uint64_t inputVertices;
        uint64_t inputPrimitives;
        uint64_t vertexShaderInvocations;
        uint64_t geometryShaderInvocations;
        uint64_t geometryShaderPrimitives;
        uint64_t rasterizerInPrimitives;
        uint64_t rasterizerOutPrimitives;
        uint64_t fragmentShaderInvocations;
        uint64_t tessControlInvocations;
        uint64_t tessEvaluationInvocations;
        uint64_t computeShaderInvocations;
    };

    struct DeviceDesc
    {
        // Common
        GraphicsAPI graphicsAPI;
        Vendor vendor;
        uint16_t nriVersionMajor;
        uint16_t nriVersionMinor;

        // Viewports
        uint32_t viewportMaxNum;
        uint32_t viewportSubPixelBits;
        int32_t viewportBoundsRange[2];

        // Framebuffer
        uint32_t frameBufferMaxDim;
        uint32_t frameBufferLayerMaxNum;
        uint32_t framebufferColorAttachmentMaxNum;

        // Multi-sampling
        uint8_t frameBufferColorSampleMaxNum;
        uint8_t frameBufferDepthSampleMaxNum;
        uint8_t frameBufferStencilSampleMaxNum;
        uint8_t frameBufferNoAttachmentsSampleMaxNum;
        uint8_t textureColorSampleMaxNum;
        uint8_t textureIntegerSampleMaxNum;
        uint8_t textureDepthSampleMaxNum;
        uint8_t textureStencilSampleMaxNum;
        uint8_t storageTextureSampleMaxNum;

        // Resource dimensions
        uint32_t texture1DMaxDim;
        uint32_t texture2DMaxDim;
        uint32_t texture3DMaxDim;
        uint32_t textureArrayMaxDim;
        uint32_t texelBufferMaxDim;

        // Memory
        uint32_t memoryAllocationMaxNum;
        uint32_t samplerAllocationMaxNum;
        uint32_t uploadBufferTextureRowAlignment;
        uint32_t uploadBufferTextureSliceAlignment;
        uint32_t typedBufferOffsetAlignment;
        uint32_t constantBufferOffsetAlignment;
        uint32_t constantBufferMaxRange;
        uint32_t storageBufferOffsetAlignment;
        uint32_t storageBufferMaxRange;
        uint32_t bufferTextureGranularity;
        uint64_t bufferMaxSize;
        uint32_t pushConstantsMaxSize;

        // Shader resources
        uint32_t boundDescriptorSetMaxNum;
        uint32_t perStageDescriptorSamplerMaxNum;
        uint32_t perStageDescriptorConstantBufferMaxNum;
        uint32_t perStageDescriptorStorageBufferMaxNum;
        uint32_t perStageDescriptorTextureMaxNum;
        uint32_t perStageDescriptorStorageTextureMaxNum;
        uint32_t perStageResourceMaxNum;

        // Descriptor set
        uint32_t descriptorSetSamplerMaxNum;
        uint32_t descriptorSetConstantBufferMaxNum;
        uint32_t descriptorSetStorageBufferMaxNum;
        uint32_t descriptorSetTextureMaxNum;
        uint32_t descriptorSetStorageTextureMaxNum;

        // Vertex shader
        uint32_t vertexShaderAttributeMaxNum;
        uint32_t vertexShaderStreamMaxNum;
        uint32_t vertexShaderOutputComponentMaxNum;

        // Tessellation control shader
        float tessControlShaderGenerationMaxLevel;
        uint32_t tessControlShaderPatchPointMaxNum;
        uint32_t tessControlShaderPerVertexInputComponentMaxNum;
        uint32_t tessControlShaderPerVertexOutputComponentMaxNum;
        uint32_t tessControlShaderPerPatchOutputComponentMaxNum;
        uint32_t tessControlShaderTotalOutputComponentMaxNum;

        // Tessellation evaluation shader
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
        uint32_t fragmentShaderCombinedOutputResourceMaxNum;

        // Compute shader
        uint32_t computeShaderSharedMemoryMaxSize;
        uint32_t computeShaderWorkGroupMaxNum[3];
        uint32_t computeShaderWorkGroupInvocationMaxNum;
        uint32_t computeShaderWorkGroupMaxDim[3];

        // Ray tracing
        uint64_t rayTracingShaderGroupIdentifierSize;
        uint64_t rayTracingShaderTableAligment;
        uint64_t rayTracingShaderTableMaxStride;
        uint32_t rayTracingShaderRecursionMaxDepth;
        uint32_t rayTracingGeometryObjectMaxNum;

        // Mesh shader
        uint32_t meshTaskMaxNum;
        uint32_t meshTaskWorkGroupInvocationMaxNum;
        uint32_t meshTaskWorkGroupMaxDim[3];
        uint32_t meshTaskTotalMemoryMaxSize;
        uint32_t meshTaskOutputMaxNum;
        uint32_t meshWorkGroupInvocationMaxNum;
        uint32_t meshWorkGroupMaxDim[3];
        uint32_t meshTotalMemoryMaxSize;
        uint32_t meshOutputVertexMaxNum;
        uint32_t meshOutputPrimitiveMaxNum;
        uint32_t meshMultiviewViewMaxNum;
        uint32_t meshOutputPerVertexGranularity;
        uint32_t meshOutputPerPrimitiveGranularity;

        // Other
        uint32_t subPixelPrecisionBits;
        uint32_t subTexelPrecisionBits;
        uint32_t mipmapPrecisionBits;
        uint32_t drawIndexedIndex16ValueMax;
        uint32_t drawIndexedIndex32ValueMax;
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
        uint8_t conservativeRasterTier;
        uint32_t phyiscalDeviceGroupSize;
        uint64_t timestampFrequencyHz;

        // Features support
        bool isAPIValidationEnabled : 1;
        bool isTextureFilterMinMaxSupported : 1;
        bool isLogicOpSupported : 1;
        bool isDepthBoundsTestSupported : 1;
        bool isProgrammableSampleLocationsSupported : 1;
        bool isComputeQueueSupported : 1;
        bool isCopyQueueSupported : 1;
        bool isCopyQueueTimestampSupported : 1;
        bool isRegisterAliasingSupported : 1;
        bool isSubsetAllocationSupported : 1;
        bool isFloat16Supported : 1;
    };
}
