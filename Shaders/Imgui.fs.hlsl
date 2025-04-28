#include "NRI.hlsl"

#define IMGUI_SAMPLER_SET 0
#define IMGUI_TEXTURE_SET 1

// Shader
#ifndef NRI_C

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 col : COLOR0;
};

NRI_RESOURCE(SamplerState, gLinearClamp, s, 1, IMGUI_SAMPLER_SET);
NRI_RESOURCE(Texture2D<float4>, tex, t, 0, IMGUI_TEXTURE_SET);

void main(PS_INPUT input, out float4 color : SV_Target0) {
    color = input.col;
    color *= tex.Sample(gLinearClamp, input.uv);
}

#endif
