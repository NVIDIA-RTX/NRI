// © 2026 NVIDIA Corporation

#pragma once

typedef ID3D12VideoEncoder ID3D12VideoEncoderBest;
#if NRI_ENABLE_AGILITY_SDK_SUPPORT
typedef ID3D12VideoDecoder1 ID3D12VideoDecoderBest;
typedef ID3D12VideoDecoderHeap1 ID3D12VideoDecoderHeapBest;
typedef ID3D12VideoEncoderHeap1 ID3D12VideoEncoderHeapBest;
#else
typedef ID3D12VideoDecoder ID3D12VideoDecoderBest;
typedef ID3D12VideoDecoderHeap ID3D12VideoDecoderHeapBest;
typedef ID3D12VideoEncoderHeap ID3D12VideoEncoderHeapBest;
#endif

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
    ComPtr<ID3D12VideoDecoderBest> m_Decoder;
    ComPtr<ID3D12VideoDecoderHeapBest> m_DecoderHeap;
    ComPtr<ID3D12VideoEncoderBest> m_Encoder;
    ComPtr<ID3D12VideoEncoderHeapBest> m_EncoderHeap;
    uint32_t m_AV1FeatureFlags = 0;
    uint32_t m_RateControlModes = 0;
    VideoSessionDesc m_Desc = {};
};

} // namespace nri
