// © 2025 NVIDIA Corporation

#define NIS_HLSL                        1
#define NIS_SCALER                      1 // upscaling needed
#define NIS_CLAMP_OUTPUT                1 // it's for free
#define NIS_VIEWPORT_SUPPORT            0 // not needed
#define NIS_BLOCK_WIDTH                 32
#define kContrastBoost                  1.0

#ifdef __hlsl_dx_compiler
    #define NIS_DXC                     1
    #define NIS_HLSL_6_2                NIS_FP16
#else
    #define NIS_DXC                     0
    #define NIS_HLSL_6_2                0
#endif

#if NIS_HLSL_6_2
    #define NIS_USE_HALF_PRECISION      1
    #define NIS_BLOCK_HEIGHT            32
#else
    #define NIS_BLOCK_HEIGHT            24
#endif

#include "../Include/NRICompatibility.hlsli"

struct Constants { // see NIS.h
    float detectRatio;
    float detectThres;
    float minContrastRatio;
    float ratioNorm;

    float sharpStartY;
    float sharpScaleY;
    float sharpStrengthMin;
    float sharpStrengthScale;

    float sharpLimitMin;
    float sharpLimitScale;
    float scaleX;
    float scaleY;

    float dstNormX;
    float dstNormY;
    float srcNormX;
    float srcNormY;
};

#define kDetectRatio                    constants.detectRatio
#define kDetectThres                    constants.detectThres
#define kMinContrastRatio               constants.minContrastRatio
#define kRatioNorm                      constants.ratioNorm
#define kSharpStartY                    constants.sharpStartY
#define kSharpScaleY                    constants.sharpScaleY
#define kSharpStrengthMin               constants.sharpStrengthMin
#define kSharpStrengthScale             constants.sharpStrengthScale
#define kSharpLimitMin                  constants.sharpLimitMin
#define kSharpLimitScale                constants.sharpLimitScale
#define kScaleX                         constants.scaleX
#define kScaleY                         constants.scaleY
#define kDstNormX                       constants.dstNormX
#define kDstNormY                       constants.dstNormY
#define kSrcNormX                       constants.srcNormX
#define kSrcNormY                       constants.srcNormY

NRI_ROOT_CONSTANTS(Constants, constants,           0, 0); // "NIS::" omitted
NRI_RESOURCE(SamplerState, samplerLinearClamp,  s, 1, 0);
NRI_RESOURCE(Texture2D<float4>, in_texture,     t, 2, 0);
NRI_RESOURCE(Texture2D<float4>, coef_scaler,    t, 3, 0);
NRI_RESOURCE(Texture2D<float4>, coef_usm,       t, 4, 0);
NRI_RESOURCE(RWTexture2D<float4>, out_texture,  u, 5, 0);

// https://github.com/NVIDIAGameWorks/NVIDIAImageScaling/blob/main/NIS/NIS_Scaler.h
// Add this line in "CalcLTI" function:
//    const float kEps = 1.0 / 255.0;
#include "NIS.hlsli"

[numthreads(NIS_THREAD_GROUP_SIZE, 1, 1)]
void main(uint3 blockIdx : SV_GroupID, uint3 threadIdx : SV_GroupThreadID) {
    NVScaler(blockIdx.xy, threadIdx.x);
}
