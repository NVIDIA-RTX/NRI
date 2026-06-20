// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorWGPU final : public DebugNameBase {
    inline DescriptorWGPU(DeviceWGPU& device)
        : m_Device(device) {
    }

    ~DescriptorWGPU();

    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    inline DescriptorType GetDescriptorType() const {
        return m_DescriptorType;
    }

    inline WGPUSampler GetSampler() const {
        return m_Sampler;
    }

    inline WGPUBuffer GetBuffer() const {
        return m_Buffer;
    }

    inline uint64_t GetOffset() const {
        return m_Offset;
    }

    inline uint64_t GetSize() const {
        return m_Size;
    }

    inline Format GetBufferFormat() const {
        return m_BufferFormat;
    }

    WGPUTextureView GetTextureView();
    Format GetFormat() const;
    const TextureDesc* GetTextureDesc() const;

    const TextureWGPU* GetTexture() const {
        return m_Texture;
    }

    const TextureViewDesc& GetTextureViewDesc() const {
        return m_TextureViewDesc;
    }

    Result Create(const SamplerDesc& samplerDesc);
    Result Create(const BufferViewDesc& bufferViewDesc);
    Result Create(const TextureViewDesc& textureViewDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        MaybeUnused(name);
    }

private:
    void ReleaseTextureView();

private:
    DeviceWGPU& m_Device;
    TextureWGPU* m_Texture = nullptr;
    WGPUSampler m_Sampler = nullptr;
    WGPUBuffer m_Buffer = nullptr;
    WGPUTextureView m_TextureView = nullptr;
    TextureViewDesc m_TextureViewDesc = {};
    uint64_t m_TextureVersion = 0;
    uint64_t m_Offset = 0;
    uint64_t m_Size = WGPU_WHOLE_SIZE;
    Format m_BufferFormat = Format::UNKNOWN;
    DescriptorType m_DescriptorType = DescriptorType::TEXTURE;
};

} // namespace nri
