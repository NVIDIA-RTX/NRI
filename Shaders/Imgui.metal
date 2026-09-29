// © 2026 NVIDIA Corporation

#include <metal_stdlib>

using namespace metal;

struct ImguiConstants {
    float invDisplayWidth;
    float invDisplayHeight;
    float hdrScale;
    float gamma;
};

// Match DescriptorMetal's 24-byte Converter argument-buffer ABI.
// Unused fields keep texture and sampler entries at the same stride.
struct ImguiTextureEntry {
    constant uchar* buffer [[id(0)]];
    texture2d<float> texture [[id(1)]];
    ulong metadata [[id(2)]];
};

struct ImguiSamplerEntry {
    sampler value [[id(0)]];
    ulong unused [[id(1)]];
    ulong metadata [[id(2)]];
};

struct ImguiRootData {
    ImguiConstants constants;
    constant ImguiSamplerEntry* samplers;
    constant ImguiTextureEntry* textures;
};

struct ImguiVertexIn {
    float2 position [[attribute(0)]];
    float2 uv [[attribute(1)]];
    float4 color [[attribute(2)]];
};

struct ImguiVertexOut {
    float4 position [[position]];
    float2 uv;
    float4 color;
};

vertex ImguiVertexOut ImguiVS(ImguiVertexIn input [[stage_in]], constant ImguiRootData& root [[buffer(2)]]) {
    ImguiVertexOut output;
    float2 position = input.position * float2(root.constants.invDisplayWidth, root.constants.invDisplayHeight);
    output.position = float4(position * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    output.color.rgb = pow(saturate(output.color.rgb), root.constants.gamma) * root.constants.hdrScale;

    return output;
}

fragment float4 ImguiFS(ImguiVertexOut input [[stage_in]], constant ImguiRootData& root [[buffer(2)]]) {
    return input.color * root.textures[0].texture.sample(root.samplers[0].value, input.uv);
}
