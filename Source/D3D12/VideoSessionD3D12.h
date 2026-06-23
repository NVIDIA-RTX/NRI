// © 2026 NVIDIA Corporation

#pragma once

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
typedef ID3D12VideoDecoder1 ID3D12VideoDecoderBest;
typedef ID3D12VideoDecoderHeap1 ID3D12VideoDecoderHeapBest;
typedef ID3D12VideoEncoder ID3D12VideoEncoderBest;
typedef ID3D12VideoEncoderHeap1 ID3D12VideoEncoderHeapBest;
#else
typedef ID3D12VideoDecoder ID3D12VideoDecoderBest;
typedef ID3D12VideoDecoderHeap ID3D12VideoDecoderHeapBest;
#endif

namespace nri {

struct VideoSessionD3D12 final : public DebugNameBase {
    inline VideoSessionD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline const VideoSessionDesc& GetDesc() const {
        return m_Desc;
    }

    inline uint32_t GetAV1FeatureFlags() const {
        return m_AV1FeatureFlags;
    }

    inline uint32_t GetRateControlModes() const {
        return m_RateControlModes;
    }

    inline bool IsBFrameSupported() const {
        return m_BFrameSupported;
    }

    inline ID3D12VideoDecoderBest* GetDecoder() const {
        NRI_CHECK(m_Desc.type == VideoSessionType::DECODE, "Unexpected");

        return static_cast<ID3D12VideoDecoderBest*>(m_Session.GetInterface());
    }

    inline ID3D12VideoDecoderHeapBest* GetDecoderHeap() const {
        NRI_CHECK(m_Desc.type == VideoSessionType::DECODE, "Unexpected");

        return static_cast<ID3D12VideoDecoderHeapBest*>(m_Heap.GetInterface());
    }

#if NRI_ENABLE_AGILITY_SDK_SUPPORT
    inline ID3D12VideoEncoderBest* GetEncoder() const {
        NRI_CHECK(m_Desc.type == VideoSessionType::ENCODE, "Unexpected");

        return static_cast<ID3D12VideoEncoderBest*>(m_Session.GetInterface());
    }

    inline ID3D12VideoEncoderHeapBest* GetEncoderHeap() const {
        NRI_CHECK(m_Desc.type == VideoSessionType::ENCODE, "Unexpected");

        return static_cast<ID3D12VideoEncoderHeapBest*>(m_Heap.GetInterface());
    }
#endif

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_Session, name);
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_Heap, name);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result Create(const VideoSessionDesc& videoSessionDesc);

private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12Pageable> m_Session; // decoder or encoder
    ComPtr<ID3D12Pageable> m_Heap;
    VideoSessionDesc m_Desc = {};
    uint32_t m_AV1FeatureFlags = 0;
    uint32_t m_RateControlModes = 0;
    bool m_BFrameSupported = false;
};

} // namespace nri
