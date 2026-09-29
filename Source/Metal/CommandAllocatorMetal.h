// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct UploadChunkMetal {
    MTL::Buffer* buffer = nullptr;
    uint8_t* data = nullptr;
    MTL::GPUAddress address = 0;
    uint64_t offset = 0;
    uint64_t size = 0;
    uint32_t lastUsed = 0; // "Reset" index
};

struct TransientResourceMetal {
    MTL::Resource* resource = nullptr;
    uint64_t size = 0;                                 // buffers
    MTL::PixelFormat format = MTL::PixelFormatInvalid; // textures
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t lastUsed = 0; // "Reset" index
    bool isInUse = false;
};

// Upload chunks and transient resources are owned by the allocator: "Reset" rewinds and recycles them. Memory unused during the last
// "TRANSIENT_MAX_UNUSED_RESETS_METAL" resets gets released. "ResetCommandAllocator" (as "ID3D12CommandAllocator::Reset" and "vkResetCommandPool")
// requires the GPU to be done with command buffers of the allocator
struct CommandAllocatorMetal final : public DebugNameBase {
    inline CommandAllocatorMetal(DeviceMetal& device) : m_Device(device), m_UploadChunks(device.GetStdAllocator()), m_TransientResources(device.GetStdAllocator()) {
    }

    ~CommandAllocatorMetal();

    inline DeviceMetal& GetDevice() const {
        return m_Device;
    }

    inline MTL4::CommandAllocator* GetNativeObject() const {
        return m_Allocator;
    }

    Result Create();
    void Reset();
    MTL::GPUAddress Upload(const void* data, uint64_t size, uint64_t alignment = 16);
    MTL::Texture* CreateTransientTexture(MTL::PixelFormat format, uint32_t width, uint32_t height);
    MTL::Buffer* CreateTransientBuffer(uint64_t size);
    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    MTL::Resource* AcquireTransientResource(const TransientResourceMetal& desc);
    void Release(MTL::Resource* resource);

    DeviceMetal& m_Device;
    MTL4::CommandAllocator* m_Allocator = nullptr;
    Vector<UploadChunkMetal> m_UploadChunks;
    Vector<TransientResourceMetal> m_TransientResources;
    size_t m_UploadChunkIndex = 0; // the current chunk
    uint32_t m_ResetIndex = 0;
};

} // namespace nri
