// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorMetal final : public DebugNameBase {
    DescriptorMetal(DeviceMetal& device);
    ~DescriptorMetal();

    Result Create(const BufferViewDesc& desc);
    Result Create(const TextureViewDesc& desc);
    Result Create(const SamplerDesc& desc);
    Result Create(const AccelerationStructureMetal& accelerationStructure);

    DeviceMetal& GetDevice() const;
    MTL::Buffer* GetBuffer() const;
    MTL::Texture* GetTexture() const;
    uint64_t GetBufferOffset() const;
    uint64_t GetBufferSize() const;
    const TextureViewDesc& GetTextureViewDesc() const;
    uint64_t GetNativeObject() const;
    Format GetFormat() const; // texture or typed buffer view format
    void WriteEntry(void* dst) const;

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    DeviceMetal& m_Device;
    BufferMetal* m_Buffer = nullptr;
    const AccelerationStructureMetal* m_AccelerationStructure = nullptr;
    TextureMetal* m_Texture = nullptr;
    MTL::Texture* m_TextureView = nullptr;
    MTL::SamplerState* m_Sampler = nullptr;
    TextureViewDesc m_TextureViewDesc = {};
    Format m_Format = Format::UNKNOWN;
    uint64_t m_BufferOffset = 0;
    uint64_t m_BufferSize = 0;
    float m_SamplerBias = 0.0f;
    uint8_t m_TextureViewOffset = 0; // in elements
    bool m_TypedBuffer = false;
};

} // namespace nri
