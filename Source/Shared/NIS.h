// © 2025 NVIDIA Corporation
// Based on "NIS_Config.h" from https://github.com/NVIDIAGameWorks/NVIDIAImageScaling

#pragma once

namespace NIS {

typedef uint32_t uint;

#include "../Shaders/NIS.cs.hlsl"

enum class HDRMode {
    None,
    Linear,
    PQ
};

inline bool UpdateConstants(Constants& constants, float sharpness,
    uint32_t inputViewportWidth, uint32_t inputViewportHeight,
    uint32_t inputTextureWidth, uint32_t inputTextureHeight,
    uint32_t outputViewportWidth, uint32_t outputViewportHeight,
    uint32_t outputTextureWidth, uint32_t outputTextureHeight,
    HDRMode hdrMode = HDRMode::None) {
    // adjust params based on value from sharpness slider
    sharpness = std::max<float>(std::min<float>(1.f, sharpness), 0.f);
    float sharpen_slider = sharpness - 0.5f; // Map 0 to 1 to -0.5 to +0.5

    // Different range for 0 to 50% vs 50% to 100%
    // The idea is to make sure sharpness of 0% map to no-sharpening,
    // while also ensuring that sharpness of 100% doesn't cause too much over-sharpening.
    const float MaxScale = (sharpen_slider >= 0.0f) ? 1.25f : 1.75f;
    const float MinScale = (sharpen_slider >= 0.0f) ? 1.25f : 1.0f;
    const float LimitScale = (sharpen_slider >= 0.0f) ? 1.25f : 1.0f;

    float kDetectRatio = 2 * 1127.f / 1024.f;

    // Params for SDR
    float kDetectThres = 64.0f / 1024.0f;
    float kMinContrastRatio = 2.0f;
    float kMaxContrastRatio = 10.0f;

    float kSharpStartY = 0.45f;
    float kSharpEndY = 0.9f;
    float kSharpStrengthMin = std::max<float>(0.0f, 0.4f + sharpen_slider * MinScale * 1.2f);
    float kSharpStrengthMax = 1.6f + sharpen_slider * MaxScale * 1.8f;
    float kSharpLimitMin = std::max<float>(0.1f, 0.14f + sharpen_slider * LimitScale * 0.32f);
    float kSharpLimitMax = 0.5f + sharpen_slider * LimitScale * 0.6f;

    if (hdrMode == HDRMode::Linear || hdrMode == HDRMode::PQ) {
        kDetectThres = 32.0f / 1024.0f;

        kMinContrastRatio = 1.5f;
        kMaxContrastRatio = 5.0f;

        kSharpStrengthMin = std::max<float>(0.0f, 0.4f + sharpen_slider * MinScale * 1.1f);
        kSharpStrengthMax = 2.2f + sharpen_slider * MaxScale * 1.8f;
        kSharpLimitMin = std::max<float>(0.06f, 0.10f + sharpen_slider * LimitScale * 0.28f);
        kSharpLimitMax = 0.6f + sharpen_slider * LimitScale * 0.6f;

        if (hdrMode == HDRMode::PQ) {
            kSharpStartY = 0.35f;
            kSharpEndY = 0.55f;
        } else {
            kSharpStartY = 0.3f;
            kSharpEndY = 0.5f;
        }
    }

    float kRatioNorm = 1.0f / (kMaxContrastRatio - kMinContrastRatio);
    float kSharpScaleY = 1.0f / (kSharpEndY - kSharpStartY);
    float kSharpStrengthScale = kSharpStrengthMax - kSharpStrengthMin;
    float kSharpLimitScale = kSharpLimitMax - kSharpLimitMin;

    uint32_t kInputViewportWidth = inputViewportWidth == 0 ? inputTextureWidth : inputViewportWidth;
    uint32_t kInputViewportHeight = inputViewportHeight == 0 ? inputTextureHeight : inputViewportHeight;
    uint32_t kOutputViewportWidth = outputViewportWidth == 0 ? outputTextureWidth : outputViewportWidth;
    uint32_t kOutputViewportHeight = outputViewportHeight == 0 ? outputTextureHeight : outputViewportHeight;

    constants.srcNormX = 1.f / inputTextureWidth;
    constants.srcNormY = 1.f / inputTextureHeight;
    constants.dstNormX = 1.f / outputTextureWidth;
    constants.dstNormY = 1.f / outputTextureHeight;
    constants.scaleX = kInputViewportWidth / float(kOutputViewportWidth);
    constants.scaleY = kInputViewportHeight / float(kOutputViewportHeight);
    constants.detectRatio = kDetectRatio;
    constants.detectThres = kDetectThres;
    constants.minContrastRatio = kMinContrastRatio;
    constants.ratioNorm = kRatioNorm;
    constants.sharpStartY = kSharpStartY;
    constants.sharpScaleY = kSharpScaleY;
    constants.sharpStrengthMin = kSharpStrengthMin;
    constants.sharpStrengthScale = kSharpStrengthScale;
    constants.sharpLimitMin = kSharpLimitMin;
    constants.sharpLimitScale = kSharpLimitScale;

#if (NIS_VIEWPORT_SUPPORT == 1)
    constants.inputViewportW = kInputViewportWidth;
    constants.inputViewportH = kInputViewportHeight;
    constants.outputViewportW = kOutputViewportWidth;
    constants.outputViewportH = kOutputViewportHeight;
#endif

    if (constants.scaleX < 0.5f || constants.scaleX > 1.f || constants.scaleY < 0.5f || constants.scaleY > 1.f || kOutputViewportWidth == 0 || kOutputViewportHeight == 0)
        return false;

    return true;
}

constexpr size_t kPhaseCount = 64;
constexpr size_t kFilterSize = 8;

constexpr uint16_t coef_scale_fp16[kPhaseCount][kFilterSize] = {
    {0, 0, 15360, 0, 0, 0, 0, 0},
    {6640, 41601, 15360, 8898, 39671, 0, 0, 0},
    {7796, 42592, 15357, 9955, 40695, 0, 0, 0},
    {8321, 43167, 15351, 10576, 41286, 4121, 0, 0},
    {8702, 43537, 15346, 11058, 41797, 4121, 0, 0},
    {9029, 43871, 15339, 11408, 42146, 4121, 0, 0},
    {9280, 44112, 15328, 11672, 42402, 5145, 0, 0},
    {9411, 44256, 15316, 11944, 42690, 5669, 0, 0},
    {9535, 44401, 15304, 12216, 42979, 6169, 0, 0},
    {9667, 44528, 15288, 12396, 43137, 6378, 0, 0},
    {9758, 44656, 15273, 12540, 43282, 6640, 0, 0},
    {9857, 44768, 15255, 12688, 43423, 6903, 0, 0},
    {9922, 44872, 15235, 12844, 43583, 7297, 0, 0},
    {10014, 44959, 15213, 13000, 43744, 7429, 0, 0},
    {10079, 45048, 15190, 13156, 43888, 7691, 0, 0},
    {10112, 45092, 15167, 13316, 44040, 7796, 0, 0},
    {10178, 45124, 15140, 13398, 44120, 8058, 0, 0},
    {10211, 45152, 15112, 13482, 44201, 8256, 0, 0},
    {10211, 45180, 15085, 13566, 44279, 8387, 0, 0},
    {10242, 45200, 15054, 13652, 44360, 8518, 0, 0},
    {10242, 45216, 15023, 13738, 44440, 8636, 0, 0},
    {10242, 45228, 14990, 13826, 44520, 8767, 0, 0},
    {10242, 45236, 14955, 13912, 44592, 8964, 0, 0},
    {10211, 45244, 14921, 14002, 44673, 9082, 0, 0},
    {10178, 45244, 14885, 14090, 44745, 9213, 0, 0},
    {10145, 45244, 14849, 14178, 44817, 9280, 0, 0},
    {10112, 45236, 14810, 14266, 44887, 9378, 0, 0},
    {10079, 45228, 14770, 14346, 44953, 9437, 0, 0},
    {10014, 45216, 14731, 14390, 45017, 9503, 0, 0},
    {9981, 45204, 14689, 14434, 45064, 9601, 0, 0},
    {9922, 45188, 14649, 14478, 45096, 9667, 0, 0},
    {9857, 45168, 14607, 14521, 45120, 9726, 0, 0},
    {9791, 45144, 14564, 14564, 45144, 9791, 0, 0},
    {9726, 45120, 14521, 14607, 45168, 9857, 0, 0},
    {9667, 45096, 14478, 14649, 45188, 9922, 0, 0},
    {9601, 45064, 14434, 14689, 45204, 9981, 0, 0},
    {9503, 45017, 14390, 14731, 45216, 10014, 0, 0},
    {9437, 44953, 14346, 14770, 45228, 10079, 0, 0},
    {9378, 44887, 14266, 14810, 45236, 10112, 0, 0},
    {9280, 44817, 14178, 14849, 45244, 10145, 0, 0},
    {9213, 44745, 14090, 14885, 45244, 10178, 0, 0},
    {9082, 44673, 14002, 14921, 45244, 10211, 0, 0},
    {8964, 44592, 13912, 14955, 45236, 10242, 0, 0},
    {8767, 44520, 13826, 14990, 45228, 10242, 0, 0},
    {8636, 44440, 13738, 15023, 45216, 10242, 0, 0},
    {8518, 44360, 13652, 15054, 45200, 10242, 0, 0},
    {8387, 44279, 13566, 15085, 45180, 10211, 0, 0},
    {8256, 44201, 13482, 15112, 45152, 10211, 0, 0},
    {8058, 44120, 13398, 15140, 45124, 10178, 0, 0},
    {7796, 44040, 13316, 15167, 45092, 10112, 0, 0},
    {7691, 43888, 13156, 15190, 45048, 10079, 0, 0},
    {7429, 43744, 13000, 15213, 44959, 10014, 0, 0},
    {7297, 43583, 12844, 15235, 44872, 9922, 0, 0},
    {6903, 43423, 12688, 15255, 44768, 9857, 0, 0},
    {6640, 43282, 12540, 15273, 44656, 9758, 0, 0},
    {6378, 43137, 12396, 15288, 44528, 9667, 0, 0},
    {6169, 42979, 12216, 15304, 44401, 9535, 0, 0},
    {5669, 42690, 11944, 15316, 44256, 9411, 0, 0},
    {5145, 42402, 11672, 15328, 44112, 9280, 0, 0},
    {4121, 42146, 11408, 15339, 43871, 9029, 0, 0},
    {4121, 41797, 11058, 15346, 43537, 8702, 0, 0},
    {4121, 41286, 10576, 15351, 43167, 8321, 0, 0},
    {0, 40695, 9955, 15357, 42592, 7796, 0, 0},
    {0, 39671, 8898, 15360, 41601, 6640, 0, 0},
};

constexpr uint16_t coef_usm_fp16[kPhaseCount][kFilterSize] = {
    {0, 47309, 15565, 47309, 0, 0, 0, 0},
    {6640, 47326, 15563, 47289, 39408, 0, 0, 0},
    {7429, 47339, 15560, 47266, 40695, 4121, 0, 0},
    {8058, 47349, 15554, 47239, 41286, 0, 0, 0},
    {8387, 47357, 15545, 47209, 41915, 0, 0, 0},
    {8636, 47363, 15534, 47176, 42238, 4121, 0, 0},
    {8767, 47364, 15522, 47141, 42657, 4121, 0, 0},
    {9029, 47367, 15509, 47105, 43023, 4121, 0, 0},
    {9213, 47363, 15490, 47018, 43249, 4121, 0, 0},
    {9280, 47357, 15472, 46928, 43472, 5145, 0, 0},
    {9345, 47347, 15450, 46836, 43727, 5145, 0, 0},
    {9378, 47337, 15427, 46736, 43999, 5669, 0, 0},
    {9437, 47323, 15401, 46630, 44152, 5669, 0, 0},
    {9470, 47310, 15376, 46520, 44312, 6169, 0, 0},
    {9503, 47294, 15338, 46402, 44479, 6378, 0, 0},
    {9503, 47272, 15274, 46280, 44648, 6640, 0, 0},
    {9503, 47253, 15215, 46158, 44817, 6903, 0, 0},
    {9503, 47231, 15150, 45972, 45017, 7165, 0, 0},
    {9535, 47206, 15082, 45708, 45132, 7297, 0, 0},
    {9503, 47180, 15012, 45432, 45232, 7429, 0, 0},
    {9470, 47153, 14939, 45152, 45332, 7560, 0, 0},
    {9470, 47126, 14868, 44681, 45444, 7691, 0, 0},
    {9437, 47090, 14793, 44071, 45560, 7796, 0, 0},
    {9411, 47030, 14714, 42847, 45668, 7927, 0, 0},
    {9411, 46968, 14635, 8387, 45788, 8058, 0, 0},
    {9345, 46902, 14552, 10786, 45908, 8256, 0, 0},
    {9313, 46846, 14478, 11647, 46036, 8321, 0, 0},
    {9247, 46776, 14394, 12292, 46120, 8453, 0, 0},
    {9247, 46714, 14288, 12620, 46184, 8518, 0, 0},
    {9147, 46648, 14130, 12936, 46248, 8570, 0, 0},
    {9029, 46576, 13956, 13268, 46312, 8702, 0, 0},
    {8964, 46512, 13792, 13456, 46378, 8767, 0, 0},
    {8898, 46446, 13624, 13624, 46446, 8898, 0, 0},
    {8767, 46378, 13456, 13792, 46512, 8964, 0, 0},
    {8702, 46312, 13268, 13956, 46576, 9029, 0, 0},
    {8570, 46248, 12936, 14130, 46648, 9147, 0, 0},
    {8518, 46184, 12620, 14288, 46714, 9247, 0, 0},
    {8453, 46120, 12292, 14394, 46776, 9247, 0, 0},
    {8321, 46036, 11647, 14478, 46846, 9313, 0, 0},
    {8256, 45908, 10786, 14552, 46902, 9345, 0, 0},
    {8058, 45788, 8387, 14635, 46968, 9411, 0, 0},
    {7927, 45668, 42847, 14714, 47030, 9411, 0, 0},
    {7796, 45560, 44071, 14793, 47090, 9437, 0, 0},
    {7691, 45444, 44681, 14868, 47126, 9470, 0, 0},
    {7560, 45332, 45152, 14939, 47153, 9470, 0, 0},
    {7429, 45232, 45432, 15012, 47180, 9503, 0, 0},
    {7297, 45132, 45708, 15082, 47206, 9535, 0, 0},
    {7165, 45017, 45972, 15150, 47231, 9503, 0, 0},
    {6903, 44817, 46158, 15215, 47253, 9503, 0, 0},
    {6640, 44648, 46280, 15274, 47272, 9503, 0, 0},
    {6378, 44479, 46402, 15338, 47294, 9503, 0, 0},
    {6169, 44312, 46520, 15376, 47310, 9470, 0, 0},
    {5669, 44152, 46630, 15401, 47323, 9437, 0, 0},
    {5669, 43999, 46736, 15427, 47337, 9378, 0, 0},
    {5145, 43727, 46836, 15450, 47347, 9345, 0, 0},
    {5145, 43472, 46928, 15472, 47357, 9280, 0, 0},
    {4121, 43249, 47018, 15490, 47363, 9213, 0, 0},
    {4121, 43023, 47105, 15509, 47367, 9029, 0, 0},
    {4121, 42657, 47141, 15522, 47364, 8767, 0, 0},
    {4121, 42238, 47176, 15534, 47363, 8636, 0, 0},
    {0, 41915, 47209, 15545, 47357, 8387, 0, 0},
    {0, 41286, 47239, 15554, 47349, 8058, 0, 0},
    {4121, 40695, 47266, 15560, 47339, 7429, 0, 0},
    {0, 39408, 47289, 15563, 47326, 6640, 0, 0},
};

} // namespace NIS