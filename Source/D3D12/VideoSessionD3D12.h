// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoSessionD3D12 final : public DebugNameBase {
    inline VideoSessionD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_Decoder, name);
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_DecoderHeap, name);
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_Encoder, name);
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_EncoderHeap, name);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result Create(const VideoSessionDesc& videoSessionDesc);

    DeviceD3D12& m_Device;
    ComPtr<ID3D12VideoDecoder> m_Decoder;
    ComPtr<ID3D12VideoDecoderHeap> m_DecoderHeap;
    ComPtr<ID3D12VideoEncoder> m_Encoder;
    ComPtr<ID3D12VideoEncoderHeap> m_EncoderHeap;
#if NRI_D3D12_HAS_VIDEO_ENCODE_AV1
    ComPtr<ID3D12VideoEncoderHeap1> m_EncoderHeap1;
#endif
    uint32_t m_AV1FeatureFlags = 0;
    uint32_t m_RateControlModes = 0;
    VideoSessionDesc m_Desc = {};
};

} // namespace nri
