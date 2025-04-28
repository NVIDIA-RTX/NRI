#include "NRI.hlsl"

struct ImguiConstants {
    float invDisplayWidth;
    float invDisplayHeight;
    float hdrScale;
};

// Shader
#ifndef NRI_C

struct VS_INPUT {
    float2 pos : POSITION0;
    float2 uv : TEXCOORD0;
    float4 col : COLOR0;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 col : COLOR0;
};

NRI_ROOT_CONSTANTS(ImguiConstants, g_PushConstants, 0, 0);

PS_INPUT main(VS_INPUT input) {
    float2 p = input.pos * float2(g_PushConstants.invDisplayWidth, g_PushConstants.invDisplayHeight);
    p = NRI_UV_TO_CLIP(p);

    float4 col = input.col;
#if IMGUI_LINEAR_COLOR
    col.xyz = pow(saturate(col.xyz), 2.2);
#endif
    col.xyz *= g_PushConstants.hdrScale;

    PS_INPUT output;
    output.pos = float4(p, 0, 1);
    output.col = col;
    output.uv = input.uv;

    return output;
}

#endif
