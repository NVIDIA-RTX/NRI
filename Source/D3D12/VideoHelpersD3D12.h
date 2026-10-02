// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

constexpr uint32_t VIDEO_DECODE_MAX_PIC_ENTRY_SLOT = 127;
constexpr uint32_t VIDEO_D3D12_ENCODE_SUPPORT_PROBE_SIZE = 512;
constexpr uint32_t VIDEO_AV1_LEVEL_4_1 = 41;
constexpr uint32_t VIDEO_D3D12_ENCODE_AV1_MIN_Q_INDEX = 1;
constexpr uint32_t VIDEO_D3D12_ENCODE_AV1_MAX_Q_INDEX = 255;
constexpr VideoEncodeRateControlDesc VIDEO_ENCODE_DEFAULT_RATE_CONTROL = {VideoEncodeRateControlMode::CQP, 26, 28, 30, 0, 51, 30, 1, 0, 0, 0, 0, 0};

static inline GUID GetVideoDecodeProfile(VideoCodec codec, Format format) {
    switch (codec) {
        case VideoCodec::H264:
            return format == Format::NV12_UNORM ? D3D12_VIDEO_DECODE_PROFILE_H264 : GUID{};
        case VideoCodec::H265:
            return (format == Format::P010_UNORM || format == Format::P016_UNORM) ? D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN10 : D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN;
        case VideoCodec::AV1:
            return D3D12_VIDEO_DECODE_PROFILE_AV1_PROFILE0;
        default:
            return {};
    }
}

static inline void FillVideoCapabilities(VideoCapabilities& videoCapabilities, const VideoSessionDesc& videoSessionDesc) {
    videoCapabilities = {};
    videoCapabilities.widthMin = videoSessionDesc.width;
    videoCapabilities.heightMin = videoSessionDesc.height;
    videoCapabilities.widthMax = videoSessionDesc.width;
    videoCapabilities.heightMax = videoSessionDesc.height;
    videoCapabilities.pictureAccessGranularityWidth = 1;
    videoCapabilities.pictureAccessGranularityHeight = 1;
    videoCapabilities.maxReferenceNum = videoSessionDesc.maxReferenceNum;
    videoCapabilities.bitstreamOffsetAlignment = videoSessionDesc.type == VideoSessionType::DECODE ? D3D12_VIDEO_DECODE_MIN_BITSTREAM_OFFSET_ALIGNMENT : 1;
    videoCapabilities.bitstreamSizeAlignment = 1;
    videoCapabilities.bitstreamSizeMax = videoSessionDesc.type == VideoSessionType::DECODE ? UINT32_MAX : uint64_t(-1);
    videoCapabilities.metadataOffsetAlignment = 1;
    videoCapabilities.resolvedMetadataOffsetAlignment = 1;
    videoCapabilities.decodeBitstreamSourceMask = videoSessionDesc.type == VideoSessionType::DECODE ? VideoDecodeBitstreamSourceBits::BUFFER : VideoDecodeBitstreamSourceBits::NONE;
    videoCapabilities.decodeDpbAndOutputCoincide = videoSessionDesc.type == VideoSessionType::DECODE;
    videoCapabilities.decodeDpbAndOutputDistinct = false;
    videoCapabilities.decodeNativeArgumentsSupported = videoSessionDesc.type == VideoSessionType::DECODE;
    videoCapabilities.encodeBitstreamRangeSizeSupported = false;
}

static inline void FillVideoDecodeAV1Capabilities(VideoAV1Capabilities& videoAV1Capabilities) {
    videoAV1Capabilities = {};
    videoAV1Capabilities.av1MaxLevel = VIDEO_AV1_LEVEL_4_1;
}

static inline VideoAV1SequenceDesc GetDefaultVideoAV1SequenceDesc(uint32_t width, uint32_t height, Format format) {
    VideoAV1SequenceDesc desc = {};
    desc.flags = VideoAV1SequenceBits::ENABLE_ORDER_HINT | VideoAV1SequenceBits::ENABLE_CDEF | VideoAV1SequenceBits::ENABLE_RESTORATION | VideoAV1SequenceBits::COLOR_DESCRIPTION_PRESENT;
    desc.bitDepth = (format == Format::P010_UNORM || format == Format::P016_UNORM) ? 10 : 8;
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

#if NRI_ENABLE_AGILITY_SDK_SUPPORT

using VideoEncodeAV1TilesLayoutD3D12 = D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_TILES;
using VideoEncodeAV1PostEncodeValuesD3D12 = D3D12_VIDEO_ENCODER_AV1_POST_ENCODE_VALUES;

static constexpr D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAGS GetSupportedVideoEncodeAV1FeatureFlags() {
    return D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_ORDER_HINT_TOOLS | D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_RESTORATION_FILTER | D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FORCED_INTEGER_MOTION_VECTORS | D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_AUTO_SEGMENTATION | D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_CDEF_FILTERING | D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_QUANTIZATION_DELTAS | D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_FILTER_DELTAS;
}

static inline bool IsVideoEncodeAV1FeatureSetSupported(uint32_t featureFlags) {
    constexpr uint32_t supportedFeatureFlags = (uint32_t)GetSupportedVideoEncodeAV1FeatureFlags();

    return (featureFlags & ~supportedFeatureFlags) == 0;
}

static inline VideoAV1EncodeFeatureBits GetVideoEncodeAV1FeatureFlags(uint32_t d3d12FeatureFlags) {
    VideoAV1EncodeFeatureBits featureFlags = VideoAV1EncodeFeatureBits::NONE;
    if (d3d12FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_ORDER_HINT_TOOLS)
        featureFlags |= VideoAV1EncodeFeatureBits::ORDER_HINT_TOOLS;
    if (d3d12FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_RESTORATION_FILTER)
        featureFlags |= VideoAV1EncodeFeatureBits::LOOP_RESTORATION_FILTER;
    if (d3d12FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FORCED_INTEGER_MOTION_VECTORS)
        featureFlags |= VideoAV1EncodeFeatureBits::FORCED_INTEGER_MOTION_VECTORS;
    if (d3d12FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_AUTO_SEGMENTATION)
        featureFlags |= VideoAV1EncodeFeatureBits::AUTO_SEGMENTATION;
    if (d3d12FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_CDEF_FILTERING)
        featureFlags |= VideoAV1EncodeFeatureBits::CDEF_FILTERING;
    if (d3d12FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_QUANTIZATION_DELTAS)
        featureFlags |= VideoAV1EncodeFeatureBits::QUANTIZATION_DELTAS;
    if (d3d12FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_FILTER_DELTAS)
        featureFlags |= VideoAV1EncodeFeatureBits::LOOP_FILTER_DELTAS;

    return featureFlags;
}

static inline void FillVideoEncodeAV1Capabilities(VideoAV1Capabilities& videoAV1Capabilities, const VideoSessionDesc& videoSessionDesc, uint32_t requiredFeatureFlags, uint32_t supportedFeatureFlags) {
    videoAV1Capabilities = {};
    videoAV1Capabilities.av1MaxLevel = VIDEO_AV1_LEVEL_4_1;
    videoAV1Capabilities.av1MaxTileColumnNum = 1;
    videoAV1Capabilities.av1MaxTileRowNum = 1;
    videoAV1Capabilities.av1MinTileWidth = videoSessionDesc.width;
    videoAV1Capabilities.av1MinTileHeight = videoSessionDesc.height;
    videoAV1Capabilities.av1MaxTileWidth = videoSessionDesc.width;
    videoAV1Capabilities.av1MaxTileHeight = videoSessionDesc.height;
    videoAV1Capabilities.av1SuperblockSizeMask = 1;
    videoAV1Capabilities.av1MaxSingleReferenceNum = videoSessionDesc.maxReferenceNum ? 1 : 0;
    videoAV1Capabilities.av1SingleReferenceNameMask = videoSessionDesc.maxReferenceNum ? 0x7F : 0;
    videoAV1Capabilities.av1MaxUnidirectionalCompoundReferenceNum = 0;
    videoAV1Capabilities.av1UnidirectionalCompoundReferenceNameMask = 0;
    videoAV1Capabilities.av1MaxBidirectionalCompoundReferenceNum = 0;
    videoAV1Capabilities.av1BidirectionalCompoundReferenceNameMask = 0;
    videoAV1Capabilities.av1MaxTemporalLayerNum = 1;
    videoAV1Capabilities.av1MaxSpatialLayerNum = 1;
    videoAV1Capabilities.av1MaxOperatingPointNum = 1;
    videoAV1Capabilities.av1MinQIndex = VIDEO_D3D12_ENCODE_AV1_MIN_Q_INDEX;
    videoAV1Capabilities.av1MaxQIndex = VIDEO_D3D12_ENCODE_AV1_MAX_Q_INDEX;
    videoAV1Capabilities.av1EncodeRequiredFeatureFlags = GetVideoEncodeAV1FeatureFlags(requiredFeatureFlags);
    videoAV1Capabilities.av1EncodeSupportedFeatureFlags = GetVideoEncodeAV1FeatureFlags(supportedFeatureFlags);
}

struct VideoEncodeRateControlStateD3D12 {
    D3D12_VIDEO_ENCODER_RATE_CONTROL_CQP cqp = {};
    D3D12_VIDEO_ENCODER_RATE_CONTROL_CBR cbr = {};
    D3D12_VIDEO_ENCODER_RATE_CONTROL_VBR vbr = {};
    D3D12_VIDEO_ENCODER_RATE_CONTROL rateControl = {};
};

static inline D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE GetVideoEncodeRateControlMode(VideoEncodeRateControlMode mode) {
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

static inline uint32_t GetSupportedVideoEncodeRateControlModes(ID3D12VideoDevice* videoDevice, D3D12_VIDEO_ENCODER_CODEC codec) {
    uint32_t modes = 0;
    constexpr std::array<VideoEncodeRateControlMode, 3> rateControlModes = {VideoEncodeRateControlMode::CQP, VideoEncodeRateControlMode::CBR, VideoEncodeRateControlMode::VBR};

    for (VideoEncodeRateControlMode mode : rateControlModes) {
        D3D12_FEATURE_DATA_VIDEO_ENCODER_RATE_CONTROL_MODE support = {};
        support.Codec = codec;
        support.RateControlMode = GetVideoEncodeRateControlMode(mode);
        HRESULT hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_RATE_CONTROL_MODE, &support, sizeof(support));
        if (SUCCEEDED(hr) && support.IsSupported)
            modes |= video::GetEncodeRateControlModeMask(mode);
    }

    return modes;
}

static inline D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAGS GetVideoEncodeRateControlFlags(const VideoEncodeRateControlDesc& desc, D3D12_VIDEO_ENCODER_SUPPORT_FLAGS supportFlags) {
    D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAGS flags = D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_NONE;

    if ((desc.qpMin || desc.qpMax) && (supportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_RATE_CONTROL_ADJUSTABLE_QP_RANGE_AVAILABLE))
        flags |= D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_QP_RANGE;

    if (desc.qpP && (supportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_RATE_CONTROL_INITIAL_QP_AVAILABLE))
        flags |= D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_INITIAL_QP;

    if (desc.maxFrameBitSize && (supportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_RATE_CONTROL_MAX_FRAME_SIZE_AVAILABLE))
        flags |= D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_MAX_FRAME_SIZE;

    if ((desc.virtualBufferSizeMs || desc.initialVirtualBufferSizeMs) && (supportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_RATE_CONTROL_VBV_SIZE_CONFIG_AVAILABLE))
        flags |= D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_VBV_SIZES;

    return flags;
}

static inline void FillVideoEncodeRateControl(const VideoEncodeRateControlDesc& desc, D3D12_VIDEO_ENCODER_SUPPORT_FLAGS supportFlags, VideoEncodeRateControlStateD3D12& state) {
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

    state.rateControl.Mode = GetVideoEncodeRateControlMode(desc.mode);
    state.rateControl.TargetFrameRate = {frameRateNumerator, frameRateDenominator};

    switch (desc.mode) {
        case VideoEncodeRateControlMode::CQP:
            state.cqp = {desc.qpI, desc.qpP, desc.qpB};
            state.rateControl.ConfigParams.DataSize = sizeof(state.cqp);
            state.rateControl.ConfigParams.pConfiguration_CQP = &state.cqp;
            break;
        case VideoEncodeRateControlMode::CBR:
            state.cbr = {qpInitial, qpMin, qpMax, desc.maxFrameBitSize, desc.targetBitrate, vbvCapacity, initialVbvFullness};
            state.rateControl.Flags = GetVideoEncodeRateControlFlags(desc, supportFlags);
            state.rateControl.ConfigParams.DataSize = sizeof(state.cbr);
            state.rateControl.ConfigParams.pConfiguration_CBR = &state.cbr;
            break;
        case VideoEncodeRateControlMode::VBR:
            state.vbr = {qpInitial, qpMin, qpMax, desc.maxFrameBitSize, desc.targetBitrate, maxBitrate, vbvCapacity, initialVbvFullness};
            state.rateControl.Flags = GetVideoEncodeRateControlFlags(desc, supportFlags);
            state.rateControl.ConfigParams.DataSize = sizeof(state.vbr);
            state.rateControl.ConfigParams.pConfiguration_VBR = &state.vbr;
            break;
        default:
            break;
    }
}

// Rate-control capability bits in "SupportFlags" depend on the queried rate-control mode, so query them per mode
template <typename T>
static inline void GetVideoEncodeRateControlSupportFlags(ID3D12VideoDevice* videoDevice, D3D12_FEATURE_VIDEO feature, T encoderSupport, uint32_t rateControlModes, std::array<uint32_t, (size_t)VideoEncodeRateControlMode::MAX_NUM>& supportFlags) {
    constexpr std::array<VideoEncodeRateControlMode, 2> rateControlModesToQuery = {VideoEncodeRateControlMode::CBR, VideoEncodeRateControlMode::VBR};

    for (VideoEncodeRateControlMode mode : rateControlModesToQuery) {
        if ((rateControlModes & video::GetEncodeRateControlModeMask(mode)) == 0)
            continue;

        VideoEncodeRateControlDesc rateControlDesc = VIDEO_ENCODE_DEFAULT_RATE_CONTROL;
        rateControlDesc.mode = mode;
        rateControlDesc.targetBitrate = 1000000;
        VideoEncodeRateControlStateD3D12 rateControlState;
        FillVideoEncodeRateControl(rateControlDesc, D3D12_VIDEO_ENCODER_SUPPORT_FLAG_NONE, rateControlState);

        encoderSupport.RateControl = rateControlState.rateControl;
        HRESULT hr = videoDevice->CheckFeatureSupport(feature, &encoderSupport, sizeof(encoderSupport));

        if (SUCCEEDED(hr))
            supportFlags[(size_t)mode] = encoderSupport.SupportFlags;
    }
}

static inline D3D12_VIDEO_ENCODER_CODEC GetVideoEncodeCodec(VideoCodec codec) {
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

static inline bool FillVideoEncodeResourceCapabilities(ID3D12VideoDevice* videoDevice, const VideoSessionDesc& videoSessionDesc, D3D12_VIDEO_ENCODER_CODEC codec, const D3D12_VIDEO_ENCODER_PROFILE_DESC& profile, VideoCapabilities& videoCapabilities) {
    D3D12_FEATURE_DATA_VIDEO_ENCODER_RESOURCE_REQUIREMENTS requirements = {};
    requirements.Codec = codec;
    requirements.Profile = profile;
    requirements.InputFormat = GetDxgiFormat(videoSessionDesc.format).typed;
    requirements.PictureTargetResolution = {videoSessionDesc.width, videoSessionDesc.height};

    const HRESULT hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_RESOURCE_REQUIREMENTS, &requirements, sizeof(requirements));
    if (FAILED(hr) || !requirements.IsSupported)
        return false;

    videoCapabilities.bitstreamOffsetAlignment = requirements.CompressedBitstreamBufferAccessAlignment;
    videoCapabilities.bitstreamSizeAlignment = requirements.CompressedBitstreamBufferAccessAlignment;
    videoCapabilities.metadataOffsetAlignment = requirements.EncoderMetadataBufferAccessAlignment;
    videoCapabilities.resolvedMetadataOffsetAlignment = requirements.EncoderMetadataBufferAccessAlignment;
    videoCapabilities.encodeFeedbackMaxPendingNum = UINT32_MAX;
    videoCapabilities.metadataSize = requirements.MaxEncoderOutputMetadataBufferSize;
    videoCapabilities.resolvedMetadataSize = sizeof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA) + sizeof(D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA);
    videoCapabilities.resolvedMetadataState = {AccessBits::VIDEO_ENCODE_WRITE, StageBits::VIDEO_ENCODE};
    videoCapabilities.resolvedMetadataQueueType = QueueType::VIDEO_ENCODE;
    videoCapabilities.encodeFeedbackSupported = true;

    if (videoSessionDesc.codec == VideoCodec::AV1)
        videoCapabilities.resolvedMetadataSize += sizeof(VideoEncodeAV1TilesLayoutD3D12) + sizeof(VideoEncodeAV1PostEncodeValuesD3D12);

    return true;
}

static bool IsVideoEncodeSessionSupported(ID3D12VideoDevice* videoDevice, const VideoSessionDesc& videoSessionDesc, VideoCapabilities* videoCapabilities = nullptr, VideoAV1Capabilities* videoAV1Capabilities = nullptr) {
    if (videoSessionDesc.type != VideoSessionType::ENCODE || videoSessionDesc.width == 0 || videoSessionDesc.height == 0 || videoSessionDesc.format == Format::UNKNOWN)
        return false;

    const VideoCodec codec = videoSessionDesc.codec;
    if (codec == VideoCodec::H264 && videoSessionDesc.format != Format::NV12_UNORM)
        return false;

    D3D12_VIDEO_ENCODER_CODEC d3d12Codec = GetVideoEncodeCodec(codec);
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
    uint32_t av1RequiredFeatureFlags = 0;
    uint32_t av1SupportedFeatureFlags = 0;
    if (codec == VideoCodec::AV1) {
        D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION_SUPPORT av1Caps = {};
        D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT av1ConfigSupport = {};
        av1ConfigSupport.Codec = d3d12Codec;
        av1ConfigSupport.Profile = profile;
        av1ConfigSupport.CodecSupportLimits.DataSize = sizeof(av1Caps);
        av1ConfigSupport.CodecSupportLimits.pAV1Support = &av1Caps;

        HRESULT hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &av1ConfigSupport, sizeof(av1ConfigSupport));
        if (FAILED(hr) || !av1ConfigSupport.IsSupported || !IsVideoEncodeAV1FeatureSetSupported(av1Caps.RequiredFeatureFlags))
            return false;

        av1RequiredFeatureFlags = av1Caps.RequiredFeatureFlags;
        av1SupportedFeatureFlags = (av1Caps.RequiredFeatureFlags | av1Caps.SupportedFeatureFlags) & (uint32_t)GetSupportedVideoEncodeAV1FeatureFlags();
        av1Config.FeatureFlags = (D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAGS)(av1Caps.RequiredFeatureFlags | av1SupportedFeatureFlags);
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

    const uint32_t rateControlModes = GetSupportedVideoEncodeRateControlModes(videoDevice, d3d12Codec);
    if ((rateControlModes & video::ENCODE_RATE_CONTROL_CQP) == 0)
        return false;

    const VideoEncodeRateControlDesc& defaultRateControl = VIDEO_ENCODE_DEFAULT_RATE_CONTROL;
    VideoEncodeRateControlStateD3D12 rateControlState;
    FillVideoEncodeRateControl(defaultRateControl, D3D12_VIDEO_ENCODER_SUPPORT_FLAG_NONE, rateControlState);

    D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_H264 h264Gop = {};
    h264Gop.GOPLength = videoSessionDesc.maxReferenceNum ? 60 : 1;
    h264Gop.PPicturePeriod = videoSessionDesc.maxReferenceNum > 1 ? 2 : 1;

    D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_HEVC hevcGop = {};
    hevcGop.GOPLength = videoSessionDesc.maxReferenceNum ? 60 : 1;
    hevcGop.PPicturePeriod = videoSessionDesc.maxReferenceNum > 1 ? 2 : 1;

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

    D3D12_VIDEO_ENCODER_LEVELS_H264 suggestedH264Level = {};
    D3D12_VIDEO_ENCODER_LEVEL_TIER_CONSTRAINTS_HEVC suggestedHevcLevel = {};
    D3D12_VIDEO_ENCODER_AV1_LEVEL_TIER_CONSTRAINTS suggestedAv1Level = {};
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
    const auto canCreateEncoder = [&]() {
        ComPtr<ID3D12VideoDevice3> videoDevice3;
        HRESULT hr = videoDevice->QueryInterface(IID_PPV_ARGS(&videoDevice3));
        if (FAILED(hr))
            return false;

        D3D12_VIDEO_ENCODER_DESC encoderDesc = {};
        encoderDesc.EncodeCodec = d3d12Codec;
        encoderDesc.EncodeProfile = profile;
        encoderDesc.InputFormat = GetDxgiFormat(videoSessionDesc.format).typed;
        encoderDesc.CodecConfiguration = codecConfig;
        encoderDesc.MaxMotionEstimationPrecision = D3D12_VIDEO_ENCODER_MOTION_ESTIMATION_PRECISION_MODE_MAXIMUM;

        ComPtr<ID3D12VideoEncoder> encoder;
        hr = videoDevice3->CreateVideoEncoder(&encoderDesc, __uuidof(ID3D12VideoEncoder), (void**)&encoder);
        if (FAILED(hr))
            return false;

        D3D12_VIDEO_ENCODER_HEAP_DESC heapDesc = {};
        heapDesc.EncodeCodec = d3d12Codec;
        heapDesc.EncodeProfile = profile;
        heapDesc.EncodeLevel = suggestedLevel;
        heapDesc.ResolutionsListCount = 1;
        heapDesc.pResolutionList = &resolution;

        ComPtr<ID3D12VideoEncoderHeap1> heap;
        hr = videoDevice3->CreateVideoEncoderHeap(&heapDesc, __uuidof(ID3D12VideoEncoderHeap1), (void**)&heap);
        return SUCCEEDED(hr);
    };

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

        if (!canCreateEncoder())
            return false;

        if (videoCapabilities) {
            FillVideoCapabilities(*videoCapabilities, videoSessionDesc);
            if (!FillVideoEncodeResourceCapabilities(videoDevice, videoSessionDesc, d3d12Codec, profile, *videoCapabilities))
                return false;
        }
        if (videoAV1Capabilities) {
            if (codec != VideoCodec::AV1)
                return false;

            FillVideoEncodeAV1Capabilities(*videoAV1Capabilities, videoSessionDesc, av1RequiredFeatureFlags, av1SupportedFeatureFlags);
        }

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

    if (!canCreateEncoder())
        return false;

    if (videoCapabilities) {
        FillVideoCapabilities(*videoCapabilities, videoSessionDesc);
        if (!FillVideoEncodeResourceCapabilities(videoDevice, videoSessionDesc, d3d12Codec, profile, *videoCapabilities))
            return false;
    }

    return true;
}

static inline bool IsVideoEncodeCodecSupported(ID3D12VideoDevice* videoDevice, VideoCodec codec) {
    VideoSessionDesc videoSessionDesc = {};
    videoSessionDesc.type = VideoSessionType::ENCODE;
    videoSessionDesc.codec = codec;
    videoSessionDesc.format = Format::NV12_UNORM;
    videoSessionDesc.width = VIDEO_D3D12_ENCODE_SUPPORT_PROBE_SIZE;
    videoSessionDesc.height = VIDEO_D3D12_ENCODE_SUPPORT_PROBE_SIZE;

    return IsVideoEncodeSessionSupported(videoDevice, videoSessionDesc);
}

#endif

} // namespace nri
