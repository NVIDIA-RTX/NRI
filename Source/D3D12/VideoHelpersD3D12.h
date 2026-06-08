// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

constexpr uint32_t VIDEO_DECODE_MAX_PIC_ENTRY_SLOT = 127;
constexpr uint32_t VIDEO_HEVC_MAX_REFERENCE_NUM = 15;
constexpr uint32_t VIDEO_ENCODE_RATE_CONTROL_CQP = 1u << (uint32_t)VideoEncodeRateControlMode::CQP;
constexpr uint32_t VIDEO_ENCODE_RATE_CONTROL_CBR = 1u << (uint32_t)VideoEncodeRateControlMode::CBR;
constexpr uint32_t VIDEO_ENCODE_RATE_CONTROL_VBR = 1u << (uint32_t)VideoEncodeRateControlMode::VBR;

inline uint32_t GetVideoEncodeRateControlModeMask(VideoEncodeRateControlMode mode) {
    return 1u << (uint32_t)mode;
}

inline const VideoH265ReferenceDesc* FindVideoH265ReferenceDescD3D12(const VideoH265ReferenceDesc* references, uint32_t referenceNum, uint32_t slot) {
    if (!references)
        return nullptr;

    for (uint32_t i = 0; i < referenceNum; i++) {
        if (references[i].slot == slot)
            return &references[i];
    }

    return nullptr;
}

inline GUID GetVideoDecodeProfileD3D12(VideoCodec codec, Format format) {
    switch (codec) {
        case VideoCodec::H264:
            return D3D12_VIDEO_DECODE_PROFILE_H264;
        case VideoCodec::H265:
            return format == Format::P010_UNORM || format == Format::P016_UNORM ? D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN10 : D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN;
        case VideoCodec::AV1:
            return D3D12_VIDEO_DECODE_PROFILE_AV1_PROFILE0;
        default:
            return {};
    }
}

inline void FillVideoCapabilitiesD3D12(VideoCapabilities& videoCapabilities, const VideoSessionDesc& videoSessionDesc) {
    videoCapabilities = {};
    videoCapabilities.widthMin = videoSessionDesc.width;
    videoCapabilities.heightMin = videoSessionDesc.height;
    videoCapabilities.widthMax = videoSessionDesc.width;
    videoCapabilities.heightMax = videoSessionDesc.height;
    videoCapabilities.pictureAccessGranularityWidth = 1;
    videoCapabilities.pictureAccessGranularityHeight = 1;
    videoCapabilities.maxReferenceNum = videoSessionDesc.maxReferenceNum;
    videoCapabilities.bitstreamOffsetAlignment = 1;
    videoCapabilities.bitstreamSizeAlignment = 1;
    videoCapabilities.bitstreamSizeMax = uint64_t(-1);
}

#if NRI_ENABLE_AGILITY_SDK_SUPPORT

inline VideoAV1SequenceDesc GetDefaultVideoAV1SequenceDescD3D12(uint32_t width, uint32_t height, Format format) {
    VideoAV1SequenceDesc desc = {};
    desc.flags = VideoAV1SequenceBits::ENABLE_ORDER_HINT | VideoAV1SequenceBits::ENABLE_CDEF | VideoAV1SequenceBits::ENABLE_RESTORATION | VideoAV1SequenceBits::COLOR_DESCRIPTION_PRESENT;
    desc.bitDepth = format == Format::P010_UNORM || format == Format::P016_UNORM ? 10 : 8;
    desc.subsamplingX = 1;
    desc.subsamplingY = 1;
    desc.maxFrameWidthMinus1 = (uint16_t)(width - 1);
    desc.maxFrameHeightMinus1 = (uint16_t)(height - 1);
    desc.frameWidthBitsMinus1 = 15;
    desc.frameHeightBitsMinus1 = 15;
    desc.orderHintBitsMinus1 = 7;
    desc.seqForceIntegerMv = 2;
    desc.seqForceScreenContentTools = 2;
    desc.colorPrimaries = 1;
    desc.transferCharacteristics = 1;
    desc.matrixCoefficients = 1;
    desc.chromaSamplePosition = 1;

    return desc;
}

inline constexpr VideoAV1PictureBits GetDefaultVideoAV1PictureFlags() {
    return VideoAV1PictureBits::ERROR_RESILIENT_MODE | VideoAV1PictureBits::DISABLE_CDF_UPDATE | VideoAV1PictureBits::ALLOW_SCREEN_CONTENT_TOOLS | VideoAV1PictureBits::FORCE_INTEGER_MV;
}

inline bool IsVideoEncodeAV1RequiredFeatureSetSupportedD3D12(uint32_t featureFlags) {
    constexpr uint32_t supportedFeatureFlags = D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_ORDER_HINT_TOOLS | D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_RESTORATION_FILTER | D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FORCED_INTEGER_MOTION_VECTORS | D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_AUTO_SEGMENTATION | D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_CDEF_FILTERING | D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_QUANTIZATION_DELTAS | D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_FILTER_DELTAS;

    return (featureFlags & ~supportedFeatureFlags) == 0;
}

struct VideoEncodeHEVCReferenceListsD3D12 {
    std::array<uint32_t, VIDEO_HEVC_MAX_REFERENCE_NUM> list0 = {};
    std::array<uint32_t, VIDEO_HEVC_MAX_REFERENCE_NUM> list1 = {};
    uint32_t list0Num = 0;
    uint32_t list1Num = 0;
    uint32_t failingReference = 0;
    bool missingDescriptor = false;
    bool invalidPictureOrderCount = false;
};

inline bool BuildVideoEncodeHEVCReferenceListsD3D12(const VideoReference* references, const VideoH265ReferenceDesc* referenceDescs, uint32_t referenceNum,
    VideoEncodeFrameType frameType, int32_t currentPictureOrderCount, VideoEncodeHEVCReferenceListsD3D12& lists) {
    lists = {};

    if (referenceNum > VIDEO_HEVC_MAX_REFERENCE_NUM) {
        lists.failingReference = VIDEO_HEVC_MAX_REFERENCE_NUM;
        return false;
    }

    if (referenceNum && !referenceDescs) {
        lists.missingDescriptor = true;
        return false;
    }

    for (uint32_t i = 0; i < referenceNum; i++) {
        const VideoH265ReferenceDesc* referenceDesc = FindVideoH265ReferenceDescD3D12(referenceDescs, referenceNum, references[i].slot);
        if (!referenceDesc) {
            lists.failingReference = i;
            lists.missingDescriptor = true;
            return false;
        }

        if (referenceDesc->listIndex == 0) {
            if (referenceDesc->pictureOrderCount >= currentPictureOrderCount) {
                lists.failingReference = i;
                lists.invalidPictureOrderCount = true;
                return false;
            }

            lists.list0[lists.list0Num++] = i;
        } else if (referenceDesc->listIndex == 1) {
            if (frameType != VideoEncodeFrameType::B) {
                lists.failingReference = i;
                lists.invalidPictureOrderCount = true;
                return false;
            }
            if (referenceDesc->pictureOrderCount <= currentPictureOrderCount) {
                lists.failingReference = i;
                lists.invalidPictureOrderCount = true;
                return false;
            }

            lists.list1[lists.list1Num++] = i;
        } else {
            lists.failingReference = i;
            lists.invalidPictureOrderCount = true;
            return false;
        }
    }

    if (referenceNum && !lists.list0Num) {
        lists.invalidPictureOrderCount = true;
        return false;
    }

    return true;
}

struct VideoEncodeRateControlStateD3D12 {
    D3D12_VIDEO_ENCODER_RATE_CONTROL_CQP cqp = {};
    D3D12_VIDEO_ENCODER_RATE_CONTROL_CBR cbr = {};
    D3D12_VIDEO_ENCODER_RATE_CONTROL_VBR vbr = {};
    D3D12_VIDEO_ENCODER_RATE_CONTROL rateControl = {};
};

inline D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE GetVideoEncodeRateControlModeD3D12(VideoEncodeRateControlMode mode) {
    switch (mode) {
        case VideoEncodeRateControlMode::CQP:
            return D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CQP;
        case VideoEncodeRateControlMode::CBR:
            return D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CBR;
        case VideoEncodeRateControlMode::VBR:
            return D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_VBR;
        default:
            return D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_ABSOLUTE_QP_MAP;
    }
}

inline uint32_t GetSupportedVideoEncodeRateControlModesD3D12(ID3D12VideoDevice* videoDevice, D3D12_VIDEO_ENCODER_CODEC codec) {
    uint32_t modes = 0;
    constexpr std::array<VideoEncodeRateControlMode, 3> rateControlModes = {VideoEncodeRateControlMode::CQP, VideoEncodeRateControlMode::CBR, VideoEncodeRateControlMode::VBR};

    for (VideoEncodeRateControlMode mode : rateControlModes) {
        D3D12_FEATURE_DATA_VIDEO_ENCODER_RATE_CONTROL_MODE support = {};
        support.Codec = codec;
        support.RateControlMode = GetVideoEncodeRateControlModeD3D12(mode);
        HRESULT hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_RATE_CONTROL_MODE, &support, sizeof(support));
        if (SUCCEEDED(hr) && support.IsSupported)
            modes |= GetVideoEncodeRateControlModeMask(mode);
    }

    return modes;
}

inline void FillVideoEncodeRateControlD3D12(const VideoEncodeRateControlDesc& desc, VideoEncodeRateControlStateD3D12& state) {
    state = {};

    const uint32_t frameRateNumerator = desc.frameRateNumerator ? desc.frameRateNumerator : 30;
    const uint32_t frameRateDenominator = desc.frameRateDenominator ? desc.frameRateDenominator : 1;
    const uint32_t qpMin = desc.qpMin;
    const uint32_t qpMax = desc.qpMax ? desc.qpMax : 51;
    const uint32_t qpInitial = desc.qpP;
    const uint64_t maxBitrate = desc.maxBitrate ? desc.maxBitrate : desc.targetBitrate;
    const uint32_t virtualBufferSizeMs = desc.virtualBufferSizeMs ? desc.virtualBufferSizeMs : 1000;
    const uint32_t initialVirtualBufferSizeMs = desc.initialVirtualBufferSizeMs ? desc.initialVirtualBufferSizeMs : virtualBufferSizeMs;
    const uint64_t vbvCapacity = desc.targetBitrate * virtualBufferSizeMs / 1000;
    const uint64_t initialVbvFullness = desc.targetBitrate * initialVirtualBufferSizeMs / 1000;

    state.rateControl.Mode = GetVideoEncodeRateControlModeD3D12(desc.mode);
    state.rateControl.TargetFrameRate = {frameRateNumerator, frameRateDenominator};

    switch (desc.mode) {
        case VideoEncodeRateControlMode::CQP:
            state.cqp = {desc.qpI, desc.qpP, desc.qpB};
            state.rateControl.ConfigParams.DataSize = sizeof(state.cqp);
            state.rateControl.ConfigParams.pConfiguration_CQP = &state.cqp;
            break;
        case VideoEncodeRateControlMode::CBR:
            state.cbr = {qpInitial, qpMin, qpMax, desc.maxFrameBitSize, desc.targetBitrate, vbvCapacity, initialVbvFullness};
            state.rateControl.ConfigParams.DataSize = sizeof(state.cbr);
            state.rateControl.ConfigParams.pConfiguration_CBR = &state.cbr;
            break;
        case VideoEncodeRateControlMode::VBR:
            state.vbr = {qpInitial, qpMin, qpMax, desc.maxFrameBitSize, desc.targetBitrate, maxBitrate, vbvCapacity, initialVbvFullness};
            state.rateControl.ConfigParams.DataSize = sizeof(state.vbr);
            state.rateControl.ConfigParams.pConfiguration_VBR = &state.vbr;
            break;
        default:
            break;
    }
}

inline D3D12_VIDEO_ENCODER_CODEC GetVideoEncodeCodecD3D12(VideoCodec codec) {
    switch (codec) {
        case VideoCodec::H264:
            return D3D12_VIDEO_ENCODER_CODEC_H264;
        case VideoCodec::H265:
            return D3D12_VIDEO_ENCODER_CODEC_HEVC;
        case VideoCodec::AV1:
            return D3D12_VIDEO_ENCODER_CODEC_AV1;
        default:
            return (D3D12_VIDEO_ENCODER_CODEC)-1;
    }
}

inline bool IsVideoEncodeSessionSupportedD3D12(ID3D12VideoDevice* videoDevice, const VideoSessionDesc& videoSessionDesc, VideoCapabilities* videoCapabilities = nullptr) {
    if (videoSessionDesc.type != VideoSessionType::ENCODE || videoSessionDesc.width == 0 || videoSessionDesc.height == 0 || videoSessionDesc.format == Format::UNKNOWN)
        return false;

    const VideoCodec codec = videoSessionDesc.codec;
    D3D12_VIDEO_ENCODER_CODEC d3d12Codec = GetVideoEncodeCodecD3D12(codec);
    if (d3d12Codec == (D3D12_VIDEO_ENCODER_CODEC)-1)
        return false;

    D3D12_VIDEO_ENCODER_PROFILE_H264 h264Profile = D3D12_VIDEO_ENCODER_PROFILE_H264_HIGH;
    const bool is10Bit = videoSessionDesc.format == Format::P010_UNORM || videoSessionDesc.format == Format::P016_UNORM;
    D3D12_VIDEO_ENCODER_PROFILE_HEVC hevcProfile = is10Bit ? D3D12_VIDEO_ENCODER_PROFILE_HEVC_MAIN10 : D3D12_VIDEO_ENCODER_PROFILE_HEVC_MAIN;
    D3D12_VIDEO_ENCODER_AV1_PROFILE av1Profile = D3D12_VIDEO_ENCODER_AV1_PROFILE_MAIN;
    D3D12_VIDEO_ENCODER_PROFILE_DESC profile = {};

    if (codec == VideoCodec::H264) {
        profile.DataSize = sizeof(h264Profile);
        profile.pH264Profile = &h264Profile;
    } else if (codec == VideoCodec::H265) {
        profile.DataSize = sizeof(hevcProfile);
        profile.pHEVCProfile = &hevcProfile;
    } else {
        profile.DataSize = sizeof(av1Profile);
        profile.pAV1Profile = &av1Profile;
    }

    D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264 h264Config = {};
    h264Config.DirectModeConfig = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264_DIRECT_MODES_DISABLED;
    h264Config.DisableDeblockingFilterConfig = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264_SLICES_DEBLOCKING_MODE_0_ALL_LUMA_CHROMA_SLICE_BLOCK_EDGES_ALWAYS_FILTERED;

    D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC hevcConfig = {};
    hevcConfig.MinLumaCodingUnitSize = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_8x8;
    hevcConfig.MaxLumaCodingUnitSize = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_32x32;
    hevcConfig.MinLumaTransformUnitSize = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_4x4;
    hevcConfig.MaxLumaTransformUnitSize = D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_32x32;
    hevcConfig.max_transform_hierarchy_depth_inter = 3;
    hevcConfig.max_transform_hierarchy_depth_intra = 3;

    if (codec == VideoCodec::H265) {
        D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC hevcCaps = {};
        hevcCaps.MinLumaCodingUnitSize = hevcConfig.MinLumaCodingUnitSize;
        hevcCaps.MaxLumaCodingUnitSize = hevcConfig.MaxLumaCodingUnitSize;
        hevcCaps.MinLumaTransformUnitSize = hevcConfig.MinLumaTransformUnitSize;
        hevcCaps.MaxLumaTransformUnitSize = hevcConfig.MaxLumaTransformUnitSize;
        hevcCaps.max_transform_hierarchy_depth_inter = hevcConfig.max_transform_hierarchy_depth_inter;
        hevcCaps.max_transform_hierarchy_depth_intra = hevcConfig.max_transform_hierarchy_depth_intra;

        D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT hevcConfigSupport = {};
        hevcConfigSupport.Codec = d3d12Codec;
        hevcConfigSupport.Profile = profile;
        hevcConfigSupport.CodecSupportLimits.DataSize = sizeof(hevcCaps);
        hevcConfigSupport.CodecSupportLimits.pHEVCSupport = &hevcCaps;
        HRESULT hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &hevcConfigSupport, sizeof(hevcConfigSupport));
        if (FAILED(hr) || !hevcConfigSupport.IsSupported)
            return false;

        if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_ASYMETRIC_MOTION_PARTITION_SUPPORT || hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_ASYMETRIC_MOTION_PARTITION_REQUIRED)
            hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_USE_ASYMETRIC_MOTION_PARTITION;
        if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_SAO_FILTER_SUPPORT)
            hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ENABLE_SAO_FILTER;
        if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_DISABLING_LOOP_FILTER_ACROSS_SLICES_SUPPORT)
            hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_DISABLE_LOOP_FILTER_ACROSS_SLICES;
        if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_TRANSFORM_SKIP_SUPPORT)
            hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ENABLE_TRANSFORM_SKIPPING;
    }

    D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION av1Config = {};
    av1Config.FeatureFlags = D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_NONE;
    av1Config.OrderHintBitsMinus1 = 7;
    if (codec == VideoCodec::AV1) {
        D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION_SUPPORT av1Caps = {};
        D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT av1ConfigSupport = {};
        av1ConfigSupport.Codec = d3d12Codec;
        av1ConfigSupport.Profile = profile;
        av1ConfigSupport.CodecSupportLimits.DataSize = sizeof(av1Caps);
        av1ConfigSupport.CodecSupportLimits.pAV1Support = &av1Caps;

        HRESULT hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &av1ConfigSupport, sizeof(av1ConfigSupport));
        if (FAILED(hr) || !av1ConfigSupport.IsSupported || !IsVideoEncodeAV1RequiredFeatureSetSupportedD3D12(av1Caps.RequiredFeatureFlags))
            return false;

        av1Config.FeatureFlags = av1Caps.RequiredFeatureFlags;
    }

    D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION codecConfig = {};
    if (codec == VideoCodec::H264) {
        codecConfig.DataSize = sizeof(h264Config);
        codecConfig.pH264Config = &h264Config;
    } else if (codec == VideoCodec::H265) {
        codecConfig.DataSize = sizeof(hevcConfig);
        codecConfig.pHEVCConfig = &hevcConfig;
    } else {
        codecConfig.DataSize = sizeof(av1Config);
        codecConfig.pAV1Config = &av1Config;
    }

    const uint32_t rateControlModes = GetSupportedVideoEncodeRateControlModesD3D12(videoDevice, d3d12Codec);
    if ((rateControlModes & VIDEO_ENCODE_RATE_CONTROL_CQP) == 0)
        return false;

    const VideoEncodeRateControlDesc defaultRateControl = {VideoEncodeRateControlMode::CQP, 26, 28, 30, 0, 51, 30, 1, 0, 0, 0, 0, 0};
    VideoEncodeRateControlStateD3D12 rateControlState;
    FillVideoEncodeRateControlD3D12(defaultRateControl, rateControlState);

    D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_H264 h264Gop = {};
    h264Gop.GOPLength = videoSessionDesc.maxReferenceNum ? 60 : 1;
    h264Gop.PPicturePeriod = 1;

    D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_HEVC hevcGop = {};
    hevcGop.GOPLength = videoSessionDesc.maxReferenceNum ? 60 : 1;
    hevcGop.PPicturePeriod = 1;

    D3D12_VIDEO_ENCODER_AV1_SEQUENCE_STRUCTURE av1Sequence = {};
    av1Sequence.IntraDistance = videoSessionDesc.maxReferenceNum ? 60 : 1;
    av1Sequence.InterFramePeriod = videoSessionDesc.maxReferenceNum ? 1 : 0;

    D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE gop = {};
    if (codec == VideoCodec::H264) {
        gop.DataSize = sizeof(h264Gop);
        gop.pH264GroupOfPictures = &h264Gop;
    } else if (codec == VideoCodec::H265) {
        gop.DataSize = sizeof(hevcGop);
        gop.pHEVCGroupOfPictures = &hevcGop;
    } else {
        gop.DataSize = sizeof(av1Sequence);
        gop.pAV1SequenceStructure = &av1Sequence;
    }

    D3D12_VIDEO_ENCODER_LEVELS_H264 suggestedH264Level = D3D12_VIDEO_ENCODER_LEVELS_H264_41;
    D3D12_VIDEO_ENCODER_LEVEL_TIER_CONSTRAINTS_HEVC suggestedHevcLevel = {D3D12_VIDEO_ENCODER_LEVELS_HEVC_41, D3D12_VIDEO_ENCODER_TIER_HEVC_MAIN};
    D3D12_VIDEO_ENCODER_AV1_LEVEL_TIER_CONSTRAINTS suggestedAv1Level = {D3D12_VIDEO_ENCODER_AV1_LEVELS_4_1, D3D12_VIDEO_ENCODER_AV1_TIER_MAIN};
    D3D12_VIDEO_ENCODER_LEVEL_SETTING suggestedLevel = {};

    if (codec == VideoCodec::H264) {
        suggestedLevel.DataSize = sizeof(suggestedH264Level);
        suggestedLevel.pH264LevelSetting = &suggestedH264Level;
    } else if (codec == VideoCodec::H265) {
        suggestedLevel.DataSize = sizeof(suggestedHevcLevel);
        suggestedLevel.pHEVCLevelSetting = &suggestedHevcLevel;
    } else {
        suggestedLevel.DataSize = sizeof(suggestedAv1Level);
        suggestedLevel.pAV1LevelSetting = &suggestedAv1Level;
    }

    D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC resolution = {videoSessionDesc.width, videoSessionDesc.height};
    D3D12_FEATURE_DATA_VIDEO_ENCODER_RESOLUTION_SUPPORT_LIMITS resolutionLimits = {};
    if (codec == VideoCodec::AV1) {
        D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_TILES tiles = {};
        tiles.RowCount = 1;
        tiles.ColCount = 1;

        D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1 encoderSupport = {};
        encoderSupport.Codec = d3d12Codec;
        encoderSupport.InputFormat = GetDxgiFormat(videoSessionDesc.format).typed;
        encoderSupport.CodecConfiguration = codecConfig;
        encoderSupport.CodecGopSequence = gop;
        encoderSupport.RateControl = rateControlState.rateControl;
        encoderSupport.IntraRefresh = D3D12_VIDEO_ENCODER_INTRA_REFRESH_MODE_NONE;
        encoderSupport.SubregionFrameEncoding = D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_FULL_FRAME;
        encoderSupport.ResolutionsListCount = 1;
        encoderSupport.pResolutionList = &resolution;
        encoderSupport.MaxReferenceFramesInDPB = 8;
        encoderSupport.SuggestedProfile = profile;
        encoderSupport.SuggestedLevel = suggestedLevel;
        encoderSupport.pResolutionDependentSupport = &resolutionLimits;
        encoderSupport.SubregionFrameEncodingData.DataSize = sizeof(tiles);
        encoderSupport.SubregionFrameEncodingData.pTilesPartition_AV1 = &tiles;

        HRESULT hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1, &encoderSupport, sizeof(encoderSupport));
        if (FAILED(hr) || (encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_GENERAL_SUPPORT_OK) == 0)
            return false;

        if ((encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_READABLE_RECONSTRUCTED_PICTURE_LAYOUT_AVAILABLE) == 0)
            return false;

        if (videoCapabilities)
            FillVideoCapabilitiesD3D12(*videoCapabilities, videoSessionDesc);

        return true;
    }

    D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT encoderSupport = {};
    encoderSupport.Codec = d3d12Codec;
    encoderSupport.InputFormat = GetDxgiFormat(videoSessionDesc.format).typed;
    encoderSupport.CodecConfiguration = codecConfig;
    encoderSupport.CodecGopSequence = gop;
    encoderSupport.RateControl = rateControlState.rateControl;
    encoderSupport.IntraRefresh = D3D12_VIDEO_ENCODER_INTRA_REFRESH_MODE_NONE;
    encoderSupport.SubregionFrameEncoding = D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_FULL_FRAME;
    encoderSupport.ResolutionsListCount = 1;
    encoderSupport.pResolutionList = &resolution;
    encoderSupport.MaxReferenceFramesInDPB = videoSessionDesc.maxReferenceNum;
    encoderSupport.SuggestedProfile = profile;
    encoderSupport.SuggestedLevel = suggestedLevel;
    encoderSupport.pResolutionDependentSupport = &resolutionLimits;

    HRESULT hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT, &encoderSupport, sizeof(encoderSupport));
    if (FAILED(hr) || (encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_GENERAL_SUPPORT_OK) == 0)
        return false;

    if ((encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_READABLE_RECONSTRUCTED_PICTURE_LAYOUT_AVAILABLE) == 0)
        return false;

    if (videoCapabilities)
        FillVideoCapabilitiesD3D12(*videoCapabilities, videoSessionDesc);

    return true;
}

inline bool IsVideoEncodeCodecSupportedD3D12(ID3D12VideoDevice* videoDevice, VideoCodec codec) {
    VideoSessionDesc videoSessionDesc = {};
    videoSessionDesc.type = VideoSessionType::ENCODE;
    videoSessionDesc.codec = codec;
    videoSessionDesc.format = Format::NV12_UNORM;
    videoSessionDesc.width = 128;
    videoSessionDesc.height = 128;

    return IsVideoEncodeSessionSupportedD3D12(videoDevice, videoSessionDesc);
}

#endif

} // namespace nri
