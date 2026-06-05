// © 2026 NVIDIA Corporation

Result VideoSessionD3D12::Create(const VideoSessionDesc& videoSessionDesc) {
    if (videoSessionDesc.width == 0 || videoSessionDesc.height == 0 || videoSessionDesc.format == Format::UNKNOWN)
        return Result::INVALID_ARGUMENT;

    if (videoSessionDesc.type == VideoSessionType::DECODE) {
        ComPtr<ID3D12VideoDevice> videoDevice;
        HRESULT hr = m_Device->QueryInterface(IID_PPV_ARGS(&videoDevice));  // TODO: use "QueryLatestInterface"
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::QueryInterface(ID3D12VideoDevice)");

        D3D12_VIDEO_DECODE_CONFIGURATION configuration = {};
        configuration.DecodeProfile = GetVideoDecodeProfileD3D12(videoSessionDesc.codec, videoSessionDesc.format);
        configuration.BitstreamEncryption = D3D12_BITSTREAM_ENCRYPTION_TYPE_NONE;
        configuration.InterlaceType = D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE;
        if (configuration.DecodeProfile == GUID{})
            return Result::UNSUPPORTED;

        D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT decodeSupport = {};
        decodeSupport.Configuration = configuration;
        decodeSupport.Width = videoSessionDesc.width;
        decodeSupport.Height = videoSessionDesc.height;
        decodeSupport.DecodeFormat = GetDxgiFormat(videoSessionDesc.format).typed;
        decodeSupport.FrameRate = {30, 1};
        hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_SUPPORT, &decodeSupport, sizeof(decodeSupport));
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice::CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_SUPPORT)");
        if ((decodeSupport.SupportFlags & D3D12_VIDEO_DECODE_SUPPORT_FLAG_SUPPORTED) == 0) {
            NRI_REPORT_WARNING(&m_Device, "D3D12 video decode support rejected: supportFlags=0x%X configurationFlags=0x%X decodeTier=0x%X", decodeSupport.SupportFlags,
                decodeSupport.ConfigurationFlags, decodeSupport.DecodeTier);
            return Result::UNSUPPORTED;
        }
        if (decodeSupport.ConfigurationFlags & D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_REFERENCE_ONLY_ALLOCATIONS_REQUIRED) {
            NRI_REPORT_WARNING(&m_Device, "D3D12 video decode support requires reference-only allocations, which are not exposed by the current NRIVideo texture usage flags");
            return Result::UNSUPPORTED;
        }

        D3D12_VIDEO_DECODER_DESC decoderDesc = {};
        decoderDesc.Configuration = configuration;

        hr = videoDevice->CreateVideoDecoder(&decoderDesc, __uuidof(ID3D12VideoDecoderBest), (void**)&m_Session); // TODO-VIDEO: use "QueryLatestInterface"
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice::CreateVideoDecoder");

        D3D12_VIDEO_DECODER_HEAP_DESC heapDesc = {};
        heapDesc.Configuration = configuration;
        heapDesc.DecodeWidth = videoSessionDesc.width;
        heapDesc.DecodeHeight = videoSessionDesc.height;
        heapDesc.Format = GetDxgiFormat(videoSessionDesc.format).typed;
        heapDesc.MaxDecodePictureBufferCount = videoSessionDesc.maxReferenceNum ? videoSessionDesc.maxReferenceNum : 1;

        hr = videoDevice->CreateVideoDecoderHeap(&heapDesc, __uuidof(ID3D12VideoDecoderHeapBest), (void**)&m_Heap); // TODO-VIDEO: use "QueryLatestInterface"
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice::CreateVideoDecoderHeap");
    } else if (videoSessionDesc.type == VideoSessionType::ENCODE) {
        ComPtr<ID3D12VideoDevice3> videoDevice;
        HRESULT hr = m_Device->QueryInterface(IID_PPV_ARGS(&videoDevice));  // TODO-VIDEO: use "QueryLatestInterface", merge with "decoder" code path
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::QueryInterface(ID3D12VideoDevice3)");

        D3D12_VIDEO_ENCODER_CODEC codec = GetVideoEncodeCodecD3D12(videoSessionDesc.codec);
        if (codec == (D3D12_VIDEO_ENCODER_CODEC)-1)
            return Result::UNSUPPORTED;

        D3D12_VIDEO_ENCODER_PROFILE_H264 h264Profile = D3D12_VIDEO_ENCODER_PROFILE_H264_HIGH;
        D3D12_VIDEO_ENCODER_PROFILE_HEVC hevcProfile = videoSessionDesc.format == Format::P010_UNORM || videoSessionDesc.format == Format::P016_UNORM ? D3D12_VIDEO_ENCODER_PROFILE_HEVC_MAIN10 : D3D12_VIDEO_ENCODER_PROFILE_HEVC_MAIN;
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        D3D12_VIDEO_ENCODER_AV1_PROFILE av1Profile = D3D12_VIDEO_ENCODER_AV1_PROFILE_MAIN;
#endif
        D3D12_VIDEO_ENCODER_PROFILE_DESC profile = {};
        if (videoSessionDesc.codec == VideoCodec::H264) {
            profile.DataSize = sizeof(h264Profile);
            profile.pH264Profile = &h264Profile;
        } else if (videoSessionDesc.codec == VideoCodec::H265) {
            profile.DataSize = sizeof(hevcProfile);
            profile.pHEVCProfile = &hevcProfile;
        } else {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
            profile.DataSize = sizeof(av1Profile);
            profile.pAV1Profile = &av1Profile;
#else
            return Result::UNSUPPORTED;
#endif
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
        if (videoSessionDesc.codec == VideoCodec::H265) {
            D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC hevcCaps = {};
            hevcCaps.MinLumaCodingUnitSize = hevcConfig.MinLumaCodingUnitSize;
            hevcCaps.MaxLumaCodingUnitSize = hevcConfig.MaxLumaCodingUnitSize;
            hevcCaps.MinLumaTransformUnitSize = hevcConfig.MinLumaTransformUnitSize;
            hevcCaps.MaxLumaTransformUnitSize = hevcConfig.MaxLumaTransformUnitSize;
            hevcCaps.max_transform_hierarchy_depth_inter = hevcConfig.max_transform_hierarchy_depth_inter;
            hevcCaps.max_transform_hierarchy_depth_intra = hevcConfig.max_transform_hierarchy_depth_intra;

            D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT hevcConfigSupport = {};
            hevcConfigSupport.Codec = codec;
            hevcConfigSupport.Profile = profile;
            hevcConfigSupport.CodecSupportLimits.DataSize = sizeof(hevcCaps);
            hevcConfigSupport.CodecSupportLimits.pHEVCSupport = &hevcCaps;
            hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &hevcConfigSupport, sizeof(hevcConfigSupport));
            NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice3::CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT)");
            if (!hevcConfigSupport.IsSupported)
                return Result::UNSUPPORTED;

            if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_ASYMETRIC_MOTION_PARTITION_SUPPORT || hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_ASYMETRIC_MOTION_PARTITION_REQUIRED)
                hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_USE_ASYMETRIC_MOTION_PARTITION;
            if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_SAO_FILTER_SUPPORT)
                hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ENABLE_SAO_FILTER;
            if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_DISABLING_LOOP_FILTER_ACROSS_SLICES_SUPPORT)
                hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_DISABLE_LOOP_FILTER_ACROSS_SLICES;
            if (hevcCaps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_TRANSFORM_SKIP_SUPPORT)
                hevcConfig.ConfigurationFlags |= D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ENABLE_TRANSFORM_SKIPPING;
        }

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION av1Config = {};
        av1Config.FeatureFlags = D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_NONE;
        av1Config.OrderHintBitsMinus1 = 7;
#endif
        if (videoSessionDesc.codec == VideoCodec::AV1) {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
            D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION_SUPPORT av1Caps = {};
            D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT av1ConfigSupport = {};
            av1ConfigSupport.Codec = codec;
            av1ConfigSupport.Profile = profile;
            av1ConfigSupport.CodecSupportLimits.DataSize = sizeof(av1Caps);
            av1ConfigSupport.CodecSupportLimits.pAV1Support = &av1Caps;
            hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &av1ConfigSupport, sizeof(av1ConfigSupport));
            NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice3::CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT)");
            if (!av1ConfigSupport.IsSupported)
                return Result::UNSUPPORTED;

            if (!IsVideoEncodeAV1RequiredFeatureSetSupportedD3D12(av1Caps.RequiredFeatureFlags)) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 AV1 encoder requires unsupported feature flags: required=0x%X", av1Caps.RequiredFeatureFlags);
                return Result::UNSUPPORTED;
            }

            av1Config.FeatureFlags = av1Caps.RequiredFeatureFlags;
            m_AV1FeatureFlags = av1Config.FeatureFlags;
#else
            return Result::UNSUPPORTED;
#endif
        }

        D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION codecConfig = {};
        if (videoSessionDesc.codec == VideoCodec::H264) {
            codecConfig.DataSize = sizeof(h264Config);
            codecConfig.pH264Config = &h264Config;
        } else if (videoSessionDesc.codec == VideoCodec::H265) {
            codecConfig.DataSize = sizeof(hevcConfig);
            codecConfig.pHEVCConfig = &hevcConfig;
        } else {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
            codecConfig.DataSize = sizeof(av1Config);
            codecConfig.pAV1Config = &av1Config;
#else
            return Result::UNSUPPORTED;
#endif
        }

        m_RateControlModes = GetSupportedVideoEncodeRateControlModesD3D12(*videoDevice, codec);
        if ((m_RateControlModes & VIDEO_ENCODE_RATE_CONTROL_CQP) == 0)
            return Result::UNSUPPORTED;

        const VideoEncodeRateControlDesc defaultRateControl = {VideoEncodeRateControlMode::CQP, 26, 28, 30, 0, 51, 30, 1, 0, 0, 0, 0, 0};
        VideoEncodeRateControlStateD3D12 rateControlState;
        FillVideoEncodeRateControlD3D12(defaultRateControl, rateControlState);

        D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_H264 h264Gop = {};
        h264Gop.GOPLength = videoSessionDesc.maxReferenceNum ? 60 : 1;
        h264Gop.PPicturePeriod = 1;

        D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_HEVC hevcGop = {};
        hevcGop.GOPLength = videoSessionDesc.maxReferenceNum ? 60 : 1;
        hevcGop.PPicturePeriod = 1;

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        D3D12_VIDEO_ENCODER_AV1_SEQUENCE_STRUCTURE av1Sequence = {};
        av1Sequence.IntraDistance = videoSessionDesc.maxReferenceNum ? 60 : 1;
        av1Sequence.InterFramePeriod = videoSessionDesc.maxReferenceNum ? 1 : 0;
#endif

        D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE gop = {};
        if (videoSessionDesc.codec == VideoCodec::H264) {
            gop.DataSize = sizeof(h264Gop);
            gop.pH264GroupOfPictures = &h264Gop;
        } else if (videoSessionDesc.codec == VideoCodec::H265) {
            gop.DataSize = sizeof(hevcGop);
            gop.pHEVCGroupOfPictures = &hevcGop;
        } else {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
            gop.DataSize = sizeof(av1Sequence);
            gop.pAV1SequenceStructure = &av1Sequence;
#else
            return Result::UNSUPPORTED;
#endif
        }

        D3D12_VIDEO_ENCODER_LEVELS_H264 suggestedH264Level = D3D12_VIDEO_ENCODER_LEVELS_H264_41;
        D3D12_VIDEO_ENCODER_LEVEL_TIER_CONSTRAINTS_HEVC suggestedHevcLevel = {D3D12_VIDEO_ENCODER_LEVELS_HEVC_41, D3D12_VIDEO_ENCODER_TIER_HEVC_MAIN};
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
        D3D12_VIDEO_ENCODER_AV1_LEVEL_TIER_CONSTRAINTS suggestedAv1Level = {D3D12_VIDEO_ENCODER_AV1_LEVELS_4_1, D3D12_VIDEO_ENCODER_AV1_TIER_MAIN};
#endif
        D3D12_VIDEO_ENCODER_LEVEL_SETTING suggestedLevel = {};
        if (videoSessionDesc.codec == VideoCodec::H264) {
            suggestedLevel.DataSize = sizeof(suggestedH264Level);
            suggestedLevel.pH264LevelSetting = &suggestedH264Level;
        } else if (videoSessionDesc.codec == VideoCodec::H265) {
            suggestedLevel.DataSize = sizeof(suggestedHevcLevel);
            suggestedLevel.pHEVCLevelSetting = &suggestedHevcLevel;
        } else {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
            suggestedLevel.DataSize = sizeof(suggestedAv1Level);
            suggestedLevel.pAV1LevelSetting = &suggestedAv1Level;
#else
            return Result::UNSUPPORTED;
#endif
        }

        D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC resolution = {videoSessionDesc.width, videoSessionDesc.height};
        D3D12_FEATURE_DATA_VIDEO_ENCODER_RESOLUTION_SUPPORT_LIMITS resolutionLimits = {};
        if (videoSessionDesc.codec == VideoCodec::AV1) {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
            D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_TILES tiles = {};
            tiles.RowCount = 1;
            tiles.ColCount = 1;

            D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1 encoderSupport = {};
            encoderSupport.Codec = codec;
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
            hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1, &encoderSupport, sizeof(encoderSupport));
            NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice3::CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1)");
            if ((encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_GENERAL_SUPPORT_OK) == 0) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 video encoder support rejected: validationFlags=0x%X supportFlags=0x%X", encoderSupport.ValidationFlags, encoderSupport.SupportFlags);
                return Result::UNSUPPORTED;
            }
            if ((encoderSupport.SupportFlags & VIDEO_D3D12_ENCODER_SUPPORT_FLAG_READABLE_RECONSTRUCTED_PICTURE_LAYOUT_AVAILABLE) == 0) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 video encoder support requires reference-only reconstructed pictures, which are not exposed by the current NRIVideo texture usage flags");
                return Result::UNSUPPORTED;
            }
#else
            return Result::UNSUPPORTED;
#endif
        } else {
            D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT encoderSupport = {};
            encoderSupport.Codec = codec;
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
            hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT, &encoderSupport, sizeof(encoderSupport));
            NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice3::CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT)");
            if ((encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_GENERAL_SUPPORT_OK) == 0) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 video encoder support rejected: validationFlags=0x%X supportFlags=0x%X", encoderSupport.ValidationFlags, encoderSupport.SupportFlags);
                return Result::UNSUPPORTED;
            }
            if ((encoderSupport.SupportFlags & VIDEO_D3D12_ENCODER_SUPPORT_FLAG_READABLE_RECONSTRUCTED_PICTURE_LAYOUT_AVAILABLE) == 0) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 video encoder support requires reference-only reconstructed pictures, which are not exposed by the current NRIVideo texture usage flags");
                return Result::UNSUPPORTED;
            }
        }

        D3D12_VIDEO_ENCODER_DESC encoderDesc = {};
        encoderDesc.EncodeCodec = codec;
        encoderDesc.EncodeProfile = profile;
        encoderDesc.InputFormat = GetDxgiFormat(videoSessionDesc.format).typed;
        encoderDesc.CodecConfiguration = codecConfig;
        encoderDesc.MaxMotionEstimationPrecision = D3D12_VIDEO_ENCODER_MOTION_ESTIMATION_PRECISION_MODE_MAXIMUM;

        hr = videoDevice->CreateVideoEncoder(&encoderDesc, __uuidof(ID3D12VideoEncoderBest), (void**)&m_Session); // TODO-VIDEO: use "QueryLatestInterface"
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice3::CreateVideoEncoder");

        D3D12_VIDEO_ENCODER_HEAP_DESC heapDesc = {};
        heapDesc.EncodeCodec = codec;
        heapDesc.EncodeProfile = profile;
        heapDesc.EncodeLevel = suggestedLevel;
        heapDesc.ResolutionsListCount = 1;
        heapDesc.pResolutionList = &resolution;

        hr = videoDevice->CreateVideoEncoderHeap(&heapDesc, __uuidof(ID3D12VideoEncoderHeapBest), (void**)&m_Heap); // TODO-VIDEO: use "QueryLatestInterface"
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice3::CreateVideoEncoderHeap");
    } else
        return Result::UNSUPPORTED;

    m_Desc = videoSessionDesc;

    return Result::SUCCESS;
}
