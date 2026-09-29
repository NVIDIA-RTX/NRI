// © 2026 NVIDIA Corporation

// Internal shaders of the Metal backend, precompiled into an embedded metallib at build time (see "InternalShadersMetal.h")

#include <metal_stdlib>

using namespace metal;

//============================================================================================================================================================================================
// "CmdClearAttachments": a full screen triangle per layer, constants at "buffer(3)"

struct ClearConstants {
    float4 color;
    float depth;
};

struct ClearVertex {
    float4 position [[position]];
    uint layer [[render_target_array_index]];
};

vertex ClearVertex nri_clear_vs(uint i [[vertex_id]], uint layer [[instance_id]], constant ClearConstants& c [[buffer(3)]]) {
    const float2 positions[3] = {float2(-1.0, -1.0), float2(3.0, -1.0), float2(-1.0, 3.0)};

    return {float4(positions[i], c.depth, 1.0), layer};
}

// "nri_clear_fs_<type>_<attachment>", type: 0 - float, 1 - uint, 2 - sint
#define CLEAR_FS(type, index, T, VALUE) \
    struct ClearOutput_##type##_##index { \
        T value [[color(index)]]; \
    }; \
    fragment ClearOutput_##type##_##index nri_clear_fs_##type##_##index(constant ClearConstants& c [[buffer(3)]]) { \
        return {VALUE}; \
    }

#define CLEAR_FS_TYPES(index) \
    CLEAR_FS(0, index, float4, c.color) \
    CLEAR_FS(1, index, uint4, as_type<uint4>(c.color)) \
    CLEAR_FS(2, index, int4, as_type<int4>(c.color))

CLEAR_FS_TYPES(0)
CLEAR_FS_TYPES(1)
CLEAR_FS_TYPES(2)
CLEAR_FS_TYPES(3)
CLEAR_FS_TYPES(4)
CLEAR_FS_TYPES(5)
CLEAR_FS_TYPES(6)
CLEAR_FS_TYPES(7)

//============================================================================================================================================================================================
// "CmdClearStorage": value at "buffer(4)", 3D slice or typed buffer element offset at "buffer(5)"

kernel void nri_clear_storage_buffer(device uint* d [[buffer(3)]], constant uint4& v [[buffer(4)]], uint i [[thread_position_in_grid]]) {
    d[i] = v.x;
}

#define WRITE_BUFFER(d, value) d.write(value, p.x + z)
#define WRITE_1D(d, value) d.write(value, p.x)
#define WRITE_1D_ARRAY(d, value) d.write(value, p.x, p.y)
#define WRITE_2D(d, value) d.write(value, p.xy)
#define WRITE_2D_ARRAY(d, value) d.write(value, p.xy, p.z)
#define WRITE_3D(d, value) d.write(value, uint3(p.xy, p.z + z))

// "nri_clear_storage_<dimension>_<type>", dimension: 0 - buffer, 1 - 1D, 2 - 1D array, 3 - 2D, 4 - 2D array, 5 - 3D
#define CLEAR_STORAGE(dimension, type, T, TEXTURE, WRITE, VALUE) \
    kernel void nri_clear_storage_##dimension##_##type(TEXTURE<T, access::write> d [[texture(0)]], constant uint4& v [[buffer(4)]], constant uint& z [[buffer(5)]], uint3 p [[thread_position_in_grid]]) { \
        WRITE(d, VALUE); \
    }

#define CLEAR_STORAGE_TYPES(dimension, TEXTURE, WRITE) \
    CLEAR_STORAGE(dimension, 0, float, TEXTURE, WRITE, as_type<float4>(v)) \
    CLEAR_STORAGE(dimension, 1, uint, TEXTURE, WRITE, v) \
    CLEAR_STORAGE(dimension, 2, int, TEXTURE, WRITE, as_type<int4>(v))

CLEAR_STORAGE_TYPES(0, texture_buffer, WRITE_BUFFER)
CLEAR_STORAGE_TYPES(1, texture1d, WRITE_1D)
CLEAR_STORAGE_TYPES(2, texture1d_array, WRITE_1D_ARRAY)
CLEAR_STORAGE_TYPES(3, texture2d, WRITE_2D)
CLEAR_STORAGE_TYPES(4, texture2d_array, WRITE_2D_ARRAY)
CLEAR_STORAGE_TYPES(5, texture3d, WRITE_3D)

//============================================================================================================================================================================================
// MIN/MAX resolves ("CmdResolveTexture", attachment resolves), constants at "buffer(3)"

struct ResolveConstants {
    uint2 origin;
    uint layer;
    uint samples;
    uint op; // 1 - min, 2 - max
};

vertex float4 nri_resolve_vs(uint i [[vertex_id]]) {
    const float2 positions[3] = {float2(-1.0, -1.0), float2(3.0, -1.0), float2(-1.0, 3.0)};

    return float4(positions[i], 0.0, 1.0);
}

#define READ_2D(s) t.read(q, s)
#define READ_ARRAY(s) t.read(q, c.layer, s)

// "nri_resolve_<2d/array>_<type>", type: 0 - float, 1 - uint, 2 - sint
#define RESOLVE(name, T, TEXTURE, READ) \
    fragment vec<T, 4> name(float4 p [[position]], TEXTURE<T, access::read> t [[texture(0)]], constant ResolveConstants& c [[buffer(3)]]) { \
        uint2 q = uint2(p.xy) + c.origin; \
        vec<T, 4> v = READ(0); \
        for (uint i = 1; i < c.samples; i++) \
            v = c.op == 1 ? min(v, READ(i)) : max(v, READ(i)); \
        return v; \
    }

RESOLVE(nri_resolve_2d_0, float, texture2d_ms, READ_2D)
RESOLVE(nri_resolve_2d_1, uint, texture2d_ms, READ_2D)
RESOLVE(nri_resolve_2d_2, int, texture2d_ms, READ_2D)
RESOLVE(nri_resolve_array_0, float, texture2d_ms_array, READ_ARRAY)
RESOLVE(nri_resolve_array_1, uint, texture2d_ms_array, READ_ARRAY)
RESOLVE(nri_resolve_array_2, int, texture2d_ms_array, READ_ARRAY)

//============================================================================================================================================================================================
// Indirect draw preparation, constants at "buffer(0)"

// Count buffer: draws at and after "count" become empty
struct FilterDrawsArgs {
    const device uint* source;
    const device uint* count;
    device uint* destination;
    uint drawNum;
    uint stride;
    uint words;
};

kernel void nri_filter_draws(constant FilterDrawsArgs& a [[buffer(0)]], uint i [[thread_position_in_grid]]) {
    if (i >= a.drawNum)
        return;

    for (uint j = 0; j < a.words; j++)
        a.destination[i * a.words + j] = i < *a.count ? a.source[i * a.stride + j] : 0;
}

// Draw parameters and draw index emulation: a root data copy per draw
struct PrepareDrawRootsArgs {
    const device uint* source;
    const device uint* root;
    device uint* roots;
    uint drawNum;
    uint stride;
    uint rootWords;
    uint parameterOffset;
    uint indexOffset;
};

kernel void nri_prepare_draw_roots(constant PrepareDrawRootsArgs& a [[buffer(0)]], uint i [[thread_position_in_grid]]) {
    if (i >= a.drawNum)
        return;

    for (uint j = 0; j < a.rootWords; j++)
        a.roots[i * a.rootWords + j] = a.root[j];

    if (a.parameterOffset != ~0u) {
        a.roots[i * a.rootWords + a.parameterOffset] = a.source[i * a.stride];
        a.roots[i * a.rootWords + a.parameterOffset + 1] = a.source[i * a.stride + 1];
    }

    if (a.indexOffset != ~0u)
        a.roots[i * a.rootWords + a.indexOffset] = i;
}

// Geometry and tessellation emulation: draw arguments to object threadgroup grids
struct EmulateDrawsArgs {
    const device uint* source;
    device packed_uint3* grids;
    uint drawNum;
    uint stride;
    uint verticesPerGroup;
    uint overlap;
};

kernel void nri_emulate_draws(constant EmulateDrawsArgs& a [[buffer(0)]], uint i [[thread_position_in_grid]]) {
    if (i >= a.drawNum)
        return;

    uint vertices = a.source[i * a.stride];
    uint instances = a.source[i * a.stride + 1];
    uint groups = vertices > a.overlap ? 1 + (vertices - a.overlap - 1) / a.verticesPerGroup : 0;

    a.grids[i] = packed_uint3(groups, instances, 1);
}

//============================================================================================================================================================================================
// Mesh pipelines require a fragment function even for depth-only draws

fragment void nri_depth_only() {
}

//============================================================================================================================================================================================
// Ray tracing helpers (see "InternalShadersMetal.h" for the host mirrors of the argument structures), arguments at "buffer(3)"

struct Instance {
    float transform[12];
    uint idMask;
    uint offsetFlags;
    ulong accelerationStructure;
};

struct MetalInstance {
    float transform[12]; // row major, "setInstanceTransformationMatrixLayout(RowMajor)"
    uint options;
    uint mask;
    uint intersectionFunctionTableOffset;
    uint userID;
    ulong accelerationStructure;
};

// Matches "IRRaytracingAccelerationStructureGPUHeader", instance contributions follow it
struct TopLevelHeader {
    ulong accelerationStructure;
    device uint* instanceContributions;
    ulong reserved[6];
};

inline void WriteTopLevelHeader(device TopLevelHeader* header, ulong accelerationStructure) {
    header->accelerationStructure = accelerationStructure;
    header->instanceContributions = (device uint*)(header + 1);

    for (uint i = 0; i < 6; i++)
        header->reserved[i] = 0;
}

struct ConvertInstancesArgs {
    device const Instance* src;
    device MetalInstance* dst;
    device TopLevelHeader* header;
    ulong accelerationStructure;
    uint instanceNum;
};

kernel void nri_convert_instances(constant ConvertInstancesArgs& args [[buffer(3)]], uint i [[thread_position_in_grid]]) {
    if (i == 0)
        WriteTopLevelHeader(args.header, args.accelerationStructure);

    if (i >= args.instanceNum)
        return;

    Instance src = args.src[i];

    MetalInstance dst;
    for (uint j = 0; j < 12; j++)
        dst.transform[j] = src.transform[j];
    dst.options = (src.offsetFlags >> 24) & 0xF;
    dst.mask = src.idMask >> 24;
    dst.intersectionFunctionTableOffset = 0;
    dst.userID = src.idMask & 0xFFFFFF;
    dst.accelerationStructure = src.accelerationStructure;
    args.dst[i] = dst;

    device uint* contributions = (device uint*)(args.header + 1);
    contributions[i] = src.offsetFlags & 0xFFFFFF;
}

struct CopyTopLevelHeaderArgs {
    device const uint* srcContributions;
    device TopLevelHeader* dst;
    ulong dstAccelerationStructure;
    uint num;
};

kernel void nri_copy_top_level_header(constant CopyTopLevelHeaderArgs& args [[buffer(3)]], uint i [[thread_position_in_grid]]) {
    if (i == 0)
        WriteTopLevelHeader(args.dst, args.dstAccelerationStructure);

    device uint* contributions = (device uint*)(args.dst + 1);
    if (i < args.num)
        contributions[i] = args.srcContributions[i];
}

struct CopyWordsArgs {
    device const uint* src;
    device uint* dst;
};

kernel void nri_copy_words(constant CopyWordsArgs& args [[buffer(3)]], uint i [[thread_position_in_grid]]) {
    args.dst[i] = args.src[i];
}

struct PrepareRaysIndirectArgs {
    device const uint* src;
    device uint* dst;
    device uint* dispatch;
};

kernel void nri_prepare_rays_indirect(constant PrepareRaysIndirectArgs& args [[buffer(3)]]) {
    for (uint i = 0; i < 26; i++)
        args.dst[i] = args.src[i];

    // "MTLDispatchThreadsIndirectArguments": width, height, depth + 8x8x1 threadgroups
    args.dispatch[0] = args.src[22];
    args.dispatch[1] = args.src[23];
    args.dispatch[2] = args.src[24];
    args.dispatch[3] = 8;
    args.dispatch[4] = 8;
    args.dispatch[5] = 1;
}
