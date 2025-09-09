// © 2025 NVIDIA Corporation

#include "NRI.hlsl"

#define NIS_HLSL                        1
#define NIS_SCALER                      1 // upscaling needed
#define NIS_CLAMP_OUTPUT                1 // it's for free
#define NIS_VIEWPORT_SUPPORT            1 // needed for proper resolution scaling support
#define NIS_BLOCK_WIDTH                 32
#define kContrastBoost                  1.0

#if (defined(NRI_DXIL) || defined(NRI_SPIRV))
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

#if (NIS_VIEWPORT_SUPPORT == 1)
    uint inputViewportW;
    uint inputViewportH;
    uint outputViewportW;
    uint outputViewportH;
#endif
};

#define NRI_NIS_SET_STATIC              0
#define NRI_NIS_SET_DYNAMIC             1

//Shader
#ifndef NRI_C

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

#if (NIS_VIEWPORT_SUPPORT == 1)
    #define kInputViewportWidth         constants.inputViewportW
    #define kInputViewportHeight        constants.inputViewportH
    #define kOutputViewportWidth        constants.outputViewportW
    #define kOutputViewportHeight       constants.outputViewportH
#endif

NRI_ROOT_CONSTANTS(Constants, constants,           0, NRI_NIS_SET_STATIC);
NRI_RESOURCE(SamplerState, samplerLinearClamp,  s, 1, NRI_NIS_SET_STATIC);
NRI_RESOURCE(Texture2D<float4>, coef_scaler,    t, 2, NRI_NIS_SET_STATIC);
NRI_RESOURCE(Texture2D<float4>, coef_usm,       t, 3, NRI_NIS_SET_STATIC);

NRI_RESOURCE(Texture2D<float4>, in_texture,     t, 0, NRI_NIS_SET_DYNAMIC);
NRI_FORMAT("unknown") NRI_RESOURCE(RWTexture2D<float4>, out_texture,  u, 1, NRI_NIS_SET_DYNAMIC);

// https://github.com/NVIDIAGameWorks/NVIDIAImageScaling/blob/main/NIS/NIS_Scaler.h
// Add this line in "CalcLTI" function:
//    const float kEps = 1.0 / 255.0;
#include "NIS.hlsl"

[numthreads(NIS_THREAD_GROUP_SIZE, 1, 1)]
void main(uint3 blockIdx : SV_GroupID, uint3 threadIdx : SV_GroupThreadID) {
    NVScaler(blockIdx.xy, threadIdx.x);
}

#endif
