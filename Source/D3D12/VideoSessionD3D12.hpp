// © 2026 NVIDIA Corporation

static bool CanCreateVideoDecodeSession(ID3D12VideoDevice* videoDevice, const VideoSessionDesc& videoSessionDesc, const D3D12_VIDEO_DECODE_CONFIGURATION& configuration) {
    D3D12_VIDEO_DECODER_DESC decoderDesc = {};
    decoderDesc.Configuration = configuration;

    ComPtr<ID3D12VideoDecoderBest> decoder;
    HRESULT hr = videoDevice->CreateVideoDecoder(&decoderDesc, __uuidof(ID3D12VideoDecoderBest), (void**)&decoder);
    if (FAILED(hr))
        return false;

    D3D12_VIDEO_DECODER_HEAP_DESC heapDesc = {};
    heapDesc.Configuration = configuration;
    heapDesc.DecodeWidth = videoSessionDesc.width;
    heapDesc.DecodeHeight = videoSessionDesc.height;
    heapDesc.Format = GetDxgiFormat(videoSessionDesc.format).typed;
    heapDesc.MaxDecodePictureBufferCount = videoSessionDesc.maxReferenceNum + 1;

    ComPtr<ID3D12VideoDecoderHeapBest> heap;
    hr = videoDevice->CreateVideoDecoderHeap(&heapDesc, __uuidof(ID3D12VideoDecoderHeapBest), (void**)&heap);

    return SUCCEEDED(hr);
}

static Result GetVideoCapabilities(DeviceD3D12& device, const VideoSessionDesc& videoSessionDesc, VideoCapabilities& videoCapabilities) {
    FillVideoCapabilities(videoCapabilities, videoSessionDesc);

    ComPtr<ID3D12VideoDevice> videoDevice;
    HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&videoDevice));
    NRI_RETURN_ON_BAD_HRESULT(&device, hr, "ID3D12Device::QueryInterface(ID3D12VideoDevice)");

    if (videoSessionDesc.type == VideoSessionType::DECODE) {
        D3D12_VIDEO_DECODE_CONFIGURATION configuration = {};
        configuration.DecodeProfile = GetVideoDecodeProfile(videoSessionDesc.codec, videoSessionDesc.format);
        if (configuration.DecodeProfile == GUID{})
            return Result::UNSUPPORTED;

        configuration.BitstreamEncryption = D3D12_BITSTREAM_ENCRYPTION_TYPE_NONE;
        configuration.InterlaceType = D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE;

        D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT decodeSupport = {};
        decodeSupport.Configuration = configuration;
        decodeSupport.Width = videoSessionDesc.width;
        decodeSupport.Height = videoSessionDesc.height;
        decodeSupport.DecodeFormat = GetDxgiFormat(videoSessionDesc.format).typed;
        decodeSupport.FrameRate = {30, 1};

        hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_SUPPORT, &decodeSupport, sizeof(decodeSupport));
        NRI_RETURN_ON_BAD_HRESULT(&device, hr, "ID3D12VideoDevice::CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_SUPPORT)");

        if ((decodeSupport.SupportFlags & D3D12_VIDEO_DECODE_SUPPORT_FLAG_SUPPORTED) == 0)
            return Result::UNSUPPORTED;

        if (decodeSupport.ConfigurationFlags & D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_REFERENCE_ONLY_ALLOCATIONS_REQUIRED)
            return Result::UNSUPPORTED;

        return CanCreateVideoDecodeSession(videoDevice.GetInterface(), videoSessionDesc, configuration) ? Result::SUCCESS : Result::UNSUPPORTED;
    }

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    if (videoSessionDesc.type == VideoSessionType::ENCODE)
        return IsVideoEncodeSessionSupported(videoDevice, videoSessionDesc, &videoCapabilities) ? Result::SUCCESS : Result::UNSUPPORTED;
#endif

    return Result::UNSUPPORTED;
}

static Result GetVideoAV1Capabilities(DeviceD3D12& device, const VideoSessionDesc& videoSessionDesc, VideoAV1Capabilities& videoAV1Capabilities) {
    videoAV1Capabilities = {};
    if (videoSessionDesc.codec != VideoCodec::AV1)
        return Result::UNSUPPORTED;

    ComPtr<ID3D12VideoDevice> videoDevice;
    HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&videoDevice));
    NRI_RETURN_ON_BAD_HRESULT(&device, hr, "ID3D12Device::QueryInterface(ID3D12VideoDevice)");

    if (videoSessionDesc.type == VideoSessionType::DECODE) {
        VideoCapabilities videoCapabilities = {};
        Result result = GetVideoCapabilities(device, videoSessionDesc, videoCapabilities);
        if (result == Result::SUCCESS)
            FillVideoDecodeAV1Capabilities(videoAV1Capabilities);

        return result;
    }

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    if (videoSessionDesc.type == VideoSessionType::ENCODE)
        return IsVideoEncodeSessionSupported(videoDevice, videoSessionDesc, nullptr, &videoAV1Capabilities) ? Result::SUCCESS : Result::UNSUPPORTED;
#endif

    return Result::UNSUPPORTED;
}

static Result GetVideoEncodeFeedback(BufferD3D12& resolvedMetadataReadback, uint64_t resolvedMetadataOffset, VideoEncodeFeedback& feedback) {
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    NRI_CHECK(resolvedMetadataReadback.IsMapped(), "'resolvedMetadataReadback' must be CPU-visible");

    const void* metadata = resolvedMetadataReadback.Map(resolvedMetadataOffset);
    const auto& d3d12Feedback = *(const D3D12_VIDEO_ENCODER_OUTPUT_METADATA*)metadata;
    const auto* subregions = (const D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA*)((const uint8_t*)metadata + sizeof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA));

    feedback = {};
    feedback.errorFlags = d3d12Feedback.EncodeErrorFlags;
    feedback.averageQp = d3d12Feedback.EncodeStats.AverageQP;
    feedback.intraCodingUnitNum = d3d12Feedback.EncodeStats.IntraCodingUnitsCount;
    feedback.interCodingUnitNum = d3d12Feedback.EncodeStats.InterCodingUnitsCount;
    feedback.skipCodingUnitNum = d3d12Feedback.EncodeStats.SkipCodingUnitsCount;
    feedback.averageMotionEstimationX = d3d12Feedback.EncodeStats.AverageMotionEstimationXDirection;
    feedback.averageMotionEstimationY = d3d12Feedback.EncodeStats.AverageMotionEstimationYDirection;
    feedback.encodedBitstreamWrittenBytes = d3d12Feedback.EncodedBitstreamWrittenBytesCount;
    feedback.writtenSubregionNum = d3d12Feedback.WrittenSubregionsCount;
    feedback.encodedBitstreamOffset = (subregions && feedback.writtenSubregionNum) ? subregions[0].bStartOffset : 0;

    return Result::SUCCESS;
#else
    MaybeUnused(resolvedMetadataReadback, resolvedMetadataOffset, feedback);

    return Result::UNSUPPORTED;
#endif
}

static Result GetVideoAV1EncodeDecodeInfo(BufferD3D12& resolvedMetadataReadback, uint64_t resolvedMetadataOffset, const VideoAV1EncodeDecodeInfoDesc& desc, VideoAV1EncodeDecodeInfo& info) {
    info = {};
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    NRI_CHECK(desc.feedback && desc.sequence, "'feedback' and 'sequence' must be valid");

    if (desc.feedback->errorFlags || !desc.feedback->encodedBitstreamWrittenBytes || !desc.feedback->writtenSubregionNum)
        return Result::FAILURE;

    if (desc.encodedPayloadHeader && desc.encodedPayloadHeaderSize)
        return video::av1::GetEncodeDecodeInfoFromHeader(desc, info);

    // Metadata only: picture header fields not reported by the resolved metadata come from the submitted picture description
    NRI_CHECK(desc.pictureDesc && desc.av1PictureDesc, "'pictureDesc' and 'av1PictureDesc' are required without 'encodedPayloadHeader'");
    NRI_CHECK(resolvedMetadataReadback.IsMapped(), "'resolvedMetadataReadback' must be CPU-visible");

    const void* metadata = resolvedMetadataReadback.Map(resolvedMetadataOffset);
    const auto* bytes = (const uint8_t*)metadata;
    const auto& output = *(const D3D12_VIDEO_ENCODER_OUTPUT_METADATA*)bytes;
    const auto& subregion = *(const D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA*)(bytes + sizeof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA));
    const auto& tilesLayout = *(const VideoEncodeAV1TilesLayoutD3D12*)(bytes + sizeof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA) + sizeof(D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA));
    const auto& post = *(const VideoEncodeAV1PostEncodeValuesD3D12*)(bytes + sizeof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA) + sizeof(D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA) + sizeof(VideoEncodeAV1TilesLayoutD3D12));

    if (output.EncodeErrorFlags || output.WrittenSubregionsCount != 1 || subregion.bSize <= subregion.bStartOffset)
        return Result::FAILURE;

    if (tilesLayout.ColCount != 1 || tilesLayout.RowCount != 1)
        return Result::FAILURE;

    const uint64_t tilePayloadSize = subregion.bSize - subregion.bStartOffset;
    if (tilePayloadSize > std::numeric_limits<uint32_t>::max())
        return Result::FAILURE;

    info.sequence = *desc.sequence;
    info.sequence.flags |= VideoAV1SequenceBits::ENABLE_CDEF | VideoAV1SequenceBits::ENABLE_RESTORATION;
    const uint32_t width = info.sequence.maxFrameWidthMinus1 + 1;
    const uint32_t height = info.sequence.maxFrameHeightMinus1 + 1;
    video::av1::BindPointers(info);
    video::av1::FillSingleTileLayout(info, width, height);
    info.tileLayout.contextUpdateTileId = (uint16_t)tilesLayout.ContextUpdateTileId;

    info.bitstreamOffset = 0; // "feedback.encodedBitstreamOffset" already includes "bStartOffset"
    info.bitstreamSize = tilePayloadSize;
    info.tiles[0] = {0, (uint32_t)tilePayloadSize, 0, 0, 0xFF};

    info.quantization.deltaQYDc = (int8_t)post.Quantization.YDCDeltaQ;
    info.quantization.deltaQUDc = (int8_t)post.Quantization.UDCDeltaQ;
    info.quantization.deltaQUAc = (int8_t)post.Quantization.UACDeltaQ;
    info.quantization.deltaQVDc = (int8_t)post.Quantization.VDCDeltaQ;
    info.quantization.deltaQVAc = (int8_t)post.Quantization.VACDeltaQ;
    info.quantization.usingQmatrix = (uint8_t)post.Quantization.UsingQMatrix;
    info.quantization.qmY = (uint8_t)post.Quantization.QMY;
    info.quantization.qmU = (uint8_t)post.Quantization.QMU;
    info.quantization.qmV = (uint8_t)post.Quantization.QMV;

    info.loopFilter.level[0] = (uint8_t)post.LoopFilter.LoopFilterLevel[0];
    info.loopFilter.level[1] = (uint8_t)post.LoopFilter.LoopFilterLevel[1];
    info.loopFilter.level[2] = (uint8_t)post.LoopFilter.LoopFilterLevelU;
    info.loopFilter.level[3] = (uint8_t)post.LoopFilter.LoopFilterLevelV;
    info.loopFilter.sharpness = (uint8_t)post.LoopFilter.LoopFilterSharpnessLevel;
    info.loopFilter.deltaEnabled = (uint8_t)post.LoopFilter.LoopFilterDeltaEnabled;
    info.loopFilter.deltaUpdate = (uint8_t)post.LoopFilter.UpdateRefDelta;
    info.loopFilter.updateModeDelta = (uint8_t)post.LoopFilter.UpdateModeDelta;
    for (uint32_t i = 0; i < 8; i++)
        info.loopFilter.refDeltas[i] = (int8_t)post.LoopFilter.RefDeltas[i];
    for (uint32_t i = 0; i < 2; i++)
        info.loopFilter.modeDeltas[i] = (int8_t)post.LoopFilter.ModeDeltas[i];

    const VideoAV1EncodePictureDesc& av1PictureDesc = *desc.av1PictureDesc;
    const bool isKeyFrame = desc.pictureDesc->frameType == VideoFrameType::IDR || desc.pictureDesc->frameType == VideoFrameType::I;

    info.picture.frameType = isKeyFrame ? VideoFrameType::IDR : VideoFrameType::P;
    info.picture.orderHint = av1PictureDesc.orderHint;
    info.picture.refreshFrameFlags = isKeyFrame ? 0xFF : av1PictureDesc.refreshFrameFlags;
    info.picture.primaryReferenceName = VideoAV1ReferenceName::NONE;
    info.picture.currentFrameId = av1PictureDesc.currentFrameId;
    info.picture.flags = VideoAV1PictureBits::ERROR_RESILIENT_MODE | VideoAV1PictureBits::FORCE_INTEGER_MV | VideoAV1PictureBits::SHOW_FRAME;

    if (!isKeyFrame) {
        std::array<uint8_t, 7> refFrameIndices = {};

        for (uint32_t i = 0; i < refFrameIndices.size(); i++) {
            if (post.ReferenceIndices[i] > std::numeric_limits<uint8_t>::max())
                return Result::FAILURE;

            refFrameIndices[i] = (uint8_t)post.ReferenceIndices[i];
        }

        if (post.PrimaryRefFrame > 7 || !video::av1::BuildInterFrameReferences(desc, refFrameIndices, info))
            return Result::FAILURE;

        info.picture.primaryReferenceName = video::av1::GetReferenceNameFromReferenceIndex((uint32_t)post.PrimaryRefFrame); // driver-reported
        info.picture.flags = VideoAV1PictureBits::SHOW_FRAME;
    }

    if (post.QuantizationDelta.DeltaQPresent) {
        info.picture.flags |= VideoAV1PictureBits::DELTA_Q_PRESENT;
        info.picture.deltaQRes = (uint8_t)post.QuantizationDelta.DeltaQRes;
    }

    if (post.LoopFilterDelta.DeltaLFPresent) {
        info.picture.flags |= VideoAV1PictureBits::DELTA_LF_PRESENT;
        info.picture.deltaLfRes = (uint8_t)post.LoopFilterDelta.DeltaLFRes;
    }

    if (post.LoopFilterDelta.DeltaLFMulti)
        info.picture.flags |= VideoAV1PictureBits::DELTA_LF_MULTI;

    if (post.SegmentationConfig.NumSegments) {
        info.picture.flags |= VideoAV1PictureBits::SEGMENTATION_ENABLED;

        if (post.SegmentationConfig.UpdateMap)
            info.picture.flags |= VideoAV1PictureBits::SEGMENTATION_UPDATE_MAP;

        if (post.SegmentationConfig.UpdateData)
            info.picture.flags |= VideoAV1PictureBits::SEGMENTATION_UPDATE_DATA;

        if (post.SegmentationConfig.TemporalUpdate)
            info.picture.flags |= VideoAV1PictureBits::SEGMENTATION_TEMPORAL_UPDATE;

        info.picture.segmentation = &info.segmentation;

        for (uint32_t i = 0; i < 8; i++) {
            info.segmentation.featureEnabled[i] = (uint8_t)post.SegmentationConfig.SegmentsData[i].EnabledFeatures;

            for (uint32_t j = 0; j < 8; j++)
                info.segmentation.featureData[i][j] = (int16_t)post.SegmentationConfig.SegmentsData[i].FeatureValue[j];
        }
    }

    info.picture.renderWidthMinus1 = (uint16_t)(width - 1);
    info.picture.renderHeightMinus1 = (uint16_t)(height - 1);
    info.picture.baseQIndex = (uint8_t)post.Quantization.BaseQIndex;
    info.picture.interpolationFilter = av1PictureDesc.interpolationFilter;
    info.picture.txMode = video::av1::TX_MODE_SELECT;
    info.picture.cdefDampingMinus3 = (uint8_t)post.CDEF.CdefDampingMinus3;
    info.picture.cdefBits = (uint8_t)post.CDEF.CdefBits;
    info.picture.tileNum = 1;

    for (uint32_t i = 0; i < 8; i++) {
        info.cdef.yPrimaryStrength[i] = (uint8_t)post.CDEF.CdefYPriStrength[i];
        info.cdef.ySecondaryStrength[i] = (uint8_t)post.CDEF.CdefYSecStrength[i];
        info.cdef.uvPrimaryStrength[i] = (uint8_t)post.CDEF.CdefUVPriStrength[i];
        info.cdef.uvSecondaryStrength[i] = (uint8_t)post.CDEF.CdefUVSecStrength[i];
    }

    video::av1::FillIdentityGlobalMotion(info.globalMotion);
    video::av1::BindPointers(info);

    return Result::SUCCESS;
#else
    MaybeUnused(resolvedMetadataReadback, resolvedMetadataOffset, desc);

    return Result::UNSUPPORTED;
#endif
}

Result VideoSessionD3D12::Create(const VideoSessionDesc& videoSessionDesc) {
    m_BFrameSupported = false;

    if (videoSessionDesc.type == VideoSessionType::DECODE) {
        ComPtr<ID3D12VideoDevice> videoDevice;
        HRESULT hr = m_Device->QueryInterface(IID_PPV_ARGS(&videoDevice)); // TODO: use "QueryLatestInterface"
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::QueryInterface(ID3D12VideoDevice)");

        D3D12_VIDEO_DECODE_CONFIGURATION configuration = {};
        configuration.DecodeProfile = GetVideoDecodeProfile(videoSessionDesc.codec, videoSessionDesc.format);
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
            NRI_REPORT_WARNING(&m_Device, "D3D12 video decode support rejected: supportFlags=0x%X configurationFlags=0x%X decodeTier=0x%X", decodeSupport.SupportFlags, decodeSupport.ConfigurationFlags, decodeSupport.DecodeTier);
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
        heapDesc.MaxDecodePictureBufferCount = videoSessionDesc.maxReferenceNum + 1;

        hr = videoDevice->CreateVideoDecoderHeap(&heapDesc, __uuidof(ID3D12VideoDecoderHeapBest), (void**)&m_Heap); // TODO-VIDEO: use "QueryLatestInterface"
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice::CreateVideoDecoderHeap");
    }
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    else if (videoSessionDesc.type == VideoSessionType::ENCODE) {
        if (videoSessionDesc.codec == VideoCodec::H264 && videoSessionDesc.format != Format::NV12_UNORM)
            return Result::UNSUPPORTED;

        ComPtr<ID3D12VideoDevice3> videoDevice;
        HRESULT hr = m_Device->QueryInterface(IID_PPV_ARGS(&videoDevice)); // TODO-VIDEO: use "QueryLatestInterface", merge with "decoder" code path
        NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device::QueryInterface(ID3D12VideoDevice3)");

        D3D12_VIDEO_ENCODER_CODEC codec = GetVideoEncodeCodec(videoSessionDesc.codec);
        if (codec == (D3D12_VIDEO_ENCODER_CODEC)-1)
            return Result::UNSUPPORTED;

        D3D12_VIDEO_ENCODER_PROFILE_H264 h264Profile = D3D12_VIDEO_ENCODER_PROFILE_H264_HIGH;
        D3D12_VIDEO_ENCODER_PROFILE_HEVC hevcProfile = (videoSessionDesc.format == Format::P010_UNORM || videoSessionDesc.format == Format::P016_UNORM) ? D3D12_VIDEO_ENCODER_PROFILE_HEVC_MAIN10 : D3D12_VIDEO_ENCODER_PROFILE_HEVC_MAIN;
        D3D12_VIDEO_ENCODER_AV1_PROFILE av1Profile = D3D12_VIDEO_ENCODER_AV1_PROFILE_MAIN;
        D3D12_VIDEO_ENCODER_PROFILE_DESC profile = {};
        if (videoSessionDesc.codec == VideoCodec::H264) {
            profile.DataSize = sizeof(h264Profile);
            profile.pH264Profile = &h264Profile;
        } else if (videoSessionDesc.codec == VideoCodec::H265) {
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
            NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice::CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT)");
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

        D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION av1Config = {};
        av1Config.FeatureFlags = D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_NONE;
        av1Config.OrderHintBitsMinus1 = 7;

        if (videoSessionDesc.codec == VideoCodec::AV1) {
            D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION_SUPPORT av1Caps = {};
            D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT av1ConfigSupport = {};
            av1ConfigSupport.Codec = codec;
            av1ConfigSupport.Profile = profile;
            av1ConfigSupport.CodecSupportLimits.DataSize = sizeof(av1Caps);
            av1ConfigSupport.CodecSupportLimits.pAV1Support = &av1Caps;
            hr = videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &av1ConfigSupport, sizeof(av1ConfigSupport));
            NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice::CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT)");
            if (!av1ConfigSupport.IsSupported)
                return Result::UNSUPPORTED;

            if (!IsVideoEncodeAV1FeatureSetSupported(av1Caps.RequiredFeatureFlags)) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 AV1 encoder requires unsupported feature flags: required=0x%X", av1Caps.RequiredFeatureFlags);
                return Result::UNSUPPORTED;
            }

            const uint32_t supportedFeatureFlags = (av1Caps.RequiredFeatureFlags | av1Caps.SupportedFeatureFlags) & (uint32_t)GetSupportedVideoEncodeAV1FeatureFlags();
            av1Config.FeatureFlags = (D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAGS)(av1Caps.RequiredFeatureFlags | supportedFeatureFlags);
            m_AV1FeatureFlags = av1Config.FeatureFlags;
            m_AV1RequiredFeatureFlags = av1Caps.RequiredFeatureFlags;
        }

        D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION codecConfig = {};
        if (videoSessionDesc.codec == VideoCodec::H264) {
            codecConfig.DataSize = sizeof(h264Config);
            codecConfig.pH264Config = &h264Config;
        } else if (videoSessionDesc.codec == VideoCodec::H265) {
            codecConfig.DataSize = sizeof(hevcConfig);
            codecConfig.pHEVCConfig = &hevcConfig;
        } else {
            codecConfig.DataSize = sizeof(av1Config);
            codecConfig.pAV1Config = &av1Config;
        }

        m_RateControlModes = GetSupportedVideoEncodeRateControlModes(videoDevice, codec);
        if ((m_RateControlModes & video::ENCODE_RATE_CONTROL_CQP) == 0)
            return Result::UNSUPPORTED;

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
        if (videoSessionDesc.codec == VideoCodec::H264) {
            gop.DataSize = sizeof(h264Gop);
            gop.pH264GroupOfPictures = &h264Gop;
        } else if (videoSessionDesc.codec == VideoCodec::H265) {
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
        if (videoSessionDesc.codec == VideoCodec::H264) {
            suggestedLevel.DataSize = sizeof(suggestedH264Level);
            suggestedLevel.pH264LevelSetting = &suggestedH264Level;
        } else if (videoSessionDesc.codec == VideoCodec::H265) {
            suggestedLevel.DataSize = sizeof(suggestedHevcLevel);
            suggestedLevel.pHEVCLevelSetting = &suggestedHevcLevel;
        } else {
            suggestedLevel.DataSize = sizeof(suggestedAv1Level);
            suggestedLevel.pAV1LevelSetting = &suggestedAv1Level;
        }

        D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC resolution = {videoSessionDesc.width, videoSessionDesc.height};
        D3D12_FEATURE_DATA_VIDEO_ENCODER_RESOLUTION_SUPPORT_LIMITS resolutionLimits = {};
        if (videoSessionDesc.codec == VideoCodec::AV1) {
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
            NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice::CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1)");
            if ((encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_GENERAL_SUPPORT_OK) == 0) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 video encoder support rejected: validationFlags=0x%X supportFlags=0x%X", encoderSupport.ValidationFlags, encoderSupport.SupportFlags);
                return Result::UNSUPPORTED;
            }
            if ((encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_READABLE_RECONSTRUCTED_PICTURE_LAYOUT_AVAILABLE) == 0) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 video encoder support requires reference-only reconstructed pictures, which are not exposed by the current NRIVideo texture usage flags");
                return Result::UNSUPPORTED;
            }

            m_BFrameSupported = videoSessionDesc.maxReferenceNum > 1;
            m_EncodeSupportFlags[(size_t)VideoEncodeRateControlMode::CQP] = encoderSupport.SupportFlags;
            GetVideoEncodeRateControlSupportFlags(videoDevice, D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1, encoderSupport, m_RateControlModes, m_EncodeSupportFlags);
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
            NRI_RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12VideoDevice::CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT)");
            if ((encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_GENERAL_SUPPORT_OK) == 0) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 video encoder support rejected: validationFlags=0x%X supportFlags=0x%X", encoderSupport.ValidationFlags, encoderSupport.SupportFlags);
                return Result::UNSUPPORTED;
            }
            if ((encoderSupport.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_READABLE_RECONSTRUCTED_PICTURE_LAYOUT_AVAILABLE) == 0) {
                NRI_REPORT_WARNING(&m_Device, "D3D12 video encoder support requires reference-only reconstructed pictures, which are not exposed by the current NRIVideo texture usage flags");
                return Result::UNSUPPORTED;
            }

            m_BFrameSupported = videoSessionDesc.maxReferenceNum > 1;
            m_EncodeSupportFlags[(size_t)VideoEncodeRateControlMode::CQP] = encoderSupport.SupportFlags;
            GetVideoEncodeRateControlSupportFlags(videoDevice, D3D12_FEATURE_VIDEO_ENCODER_SUPPORT, encoderSupport, m_RateControlModes, m_EncodeSupportFlags);
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
    }
#endif
    else
        return Result::UNSUPPORTED;

    m_Desc = videoSessionDesc;

    return Result::SUCCESS;
}
