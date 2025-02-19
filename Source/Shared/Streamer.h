// © 2024 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferUpdateRequest {
    BufferUpdateRequestDesc desc;
    uint64_t offset;
};

struct TextureUpdateRequest {
    TextureUpdateRequestDesc desc;
    uint64_t offset;
};

struct GarbageInFlight {
    Buffer* buffer;
    Memory* memory;
    uint32_t frameNum;
};

struct StreamerImpl : public DebugNameBase {
    inline StreamerImpl(Device& device, const CoreInterface& NRI)
        : m_Device(device)
        , m_NRI(NRI)
        , m_BufferRequests(((DeviceBase&)device).GetStdAllocator())
        , m_BufferRequestsWithDst(((DeviceBase&)device).GetStdAllocator())
        , m_TextureRequests(((DeviceBase&)device).GetStdAllocator())
        , m_TextureRequestsWithDst(((DeviceBase&)device).GetStdAllocator())
        , m_GarbageInFlight(((DeviceBase&)device).GetStdAllocator()) {
    }

    inline Buffer* GetDynamicBuffer() {
        return m_DynamicBuffer;
    }

    inline Buffer* GetConstantBuffer() {
        return m_ConstantBuffer;
    }

    inline Device& GetDevice() {
        return m_Device;
    }

    ~StreamerImpl();

    Result Create(const StreamerDesc& desc);
    uint32_t UpdateConstantBuffer(const void* data, uint32_t dataSize);
    uint64_t AddBufferUpdateRequest(const BufferUpdateRequestDesc& bufferUpdateRequestDesc);
    uint64_t AddTextureUpdateRequest(const TextureUpdateRequestDesc& textureUpdateRequestDesc);
    Result CopyUpdateRequests();
    void CmdUploadUpdateRequests(CommandBuffer& commandBuffer);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        m_NRI.SetDebugName(m_ConstantBuffer, name);
        m_NRI.SetDebugName(m_ConstantBufferMemory, name);
        m_NRI.SetDebugName(m_DynamicBuffer, name);
        m_NRI.SetDebugName(m_DynamicBufferMemory, name);
    }

private:
    Device& m_Device;
    const CoreInterface& m_NRI;
    StreamerDesc m_Desc = {};
    Vector<BufferUpdateRequest> m_BufferRequests;
    Vector<BufferUpdateRequest> m_BufferRequestsWithDst;
    Vector<TextureUpdateRequest> m_TextureRequests;
    Vector<TextureUpdateRequest> m_TextureRequestsWithDst;
    Vector<GarbageInFlight> m_GarbageInFlight;
    Buffer* m_ConstantBuffer = nullptr;
    Memory* m_ConstantBufferMemory = nullptr;
    Buffer* m_DynamicBuffer = nullptr;
    Memory* m_DynamicBufferMemory = nullptr;
    uint32_t m_ConstantDataOffset = 0;
    uint64_t m_DynamicDataOffset = 0;
    uint64_t m_DynamicDataOffsetBase = 0;
    uint64_t m_DynamicBufferSize = 0;
    uint32_t m_FrameIndex = 0;
};

}