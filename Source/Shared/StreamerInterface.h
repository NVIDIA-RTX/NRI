// © 2024 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferUpdateRequest {
    Buffer* dstBuffer;
    uint64_t dstOffset;
    Buffer* srcBuffer;
    uint64_t srcOffset;
    uint64_t size;
};

struct TextureUpdateRequest {
    Texture* dstTexture;
    TextureRegionDesc dstRegion;
    Buffer* srcBuffer;
    TextureDataLayoutDesc srcDataLayout;
};

struct GarbageInFlight {
    Buffer* buffer;
    uint32_t frameNum;
};

struct StreamerImpl final : public DebugNameBase {
    inline StreamerImpl(Device& device, const CoreInterface& NRI)
        : m_Device(device)
        , m_iCore(NRI)
        , m_BufferRequestsWithDst(((DeviceBase&)device).GetStdAllocator())
        , m_TextureRequestsWithDst(((DeviceBase&)device).GetStdAllocator())
        , m_GarbageInFlight(((DeviceBase&)device).GetStdAllocator()) {
    }

    inline Buffer* GetConstantBuffer() {
        return m_ConstantBuffer;
    }

    inline Device& GetDevice() {
        return m_Device;
    }

    ~StreamerImpl();

    Result Create(const StreamerDesc& desc);
    uint32_t StreamConstantData(const void* data, uint32_t dataSize);
    BufferOffset StreamBufferData(const StreamBufferDataDesc& streamBufferDataDesc);
    BufferOffset StreamTextureData(const StreamTextureDataDesc& streamTextureDataDesc);
    void CmdCopyStreamedData(CommandBuffer& commandBuffer);
    void EndFrame();

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        m_iCore.SetDebugName(m_ConstantBuffer, name);
        m_iCore.SetDebugName(m_DynamicBuffer, name);
    }

private:
    bool Grow();

private:
    Device& m_Device;
    const CoreInterface& m_iCore;
    StreamerDesc m_Desc = {};
    Vector<BufferUpdateRequest> m_BufferRequestsWithDst;
    Vector<TextureUpdateRequest> m_TextureRequestsWithDst;
    Vector<GarbageInFlight> m_GarbageInFlight;
    Buffer* m_DynamicBuffer = nullptr;
    Buffer* m_ConstantBuffer = nullptr;
    uint64_t m_DynamicBufferOffset = 0;
    uint64_t m_DynamicBufferSizePerFrame = 0;
    uint32_t m_ConstantBufferOffset = 0;
    uint32_t m_FrameIndex = 0;
    Lock m_Lock;
};

}