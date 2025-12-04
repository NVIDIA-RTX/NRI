// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

enum class DescriptorTypeExt : uint8_t {
    // Must match "DescriptorType"
    MUTABLE,
    SAMPLER,
    TEXTURE,
    STORAGE_TEXTURE,
    BUFFER,
    STORAGE_BUFFER,
    CONSTANT_BUFFER,
    STRUCTURED_BUFFER,
    STORAGE_STRUCTURED_BUFFER,
    ACCELERATION_STRUCTURE,

    // Extra
    COLOR_ATTACHMENT,
    DEPTH_STENCIL_ATTACHMENT,
    SHADING_RATE_ATTACHMENT,

    MAX_NUM
};

struct DescriptorVal final : public ObjectVal {
    DescriptorVal(DeviceVal& device, Descriptor* descriptor, DescriptorType type);
    DescriptorVal(DeviceVal& device, Descriptor* descriptor, const BufferViewDesc& bufferViewDesc);
    DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture1DViewDesc& textureViewDesc);
    DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture2DViewDesc& textureViewDesc);
    DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture3DViewDesc& textureViewDesc);

    inline Descriptor* GetImpl() const {
        return (Descriptor*)m_Impl;
    }

    inline DescriptorType GetType() const {
        return (DescriptorType)m_Type;
    }

    inline DescriptorTypeExt GetTypeExt() const {
        return m_Type;
    }

    inline uint64_t GetNativeObject() const {
        return GetCoreInterfaceImpl().GetDescriptorNativeObject(GetImpl());
    }

    inline bool CanBeRoot() const {
        return m_Type == DescriptorTypeExt::CONSTANT_BUFFER
            || m_Type == DescriptorTypeExt::STRUCTURED_BUFFER
            || m_Type == DescriptorTypeExt::STORAGE_STRUCTURED_BUFFER
            || m_Type == DescriptorTypeExt::ACCELERATION_STRUCTURE;
    }

    inline bool IsConstantBuffer() const {
        return m_Type == DescriptorTypeExt::CONSTANT_BUFFER;
    }

    inline bool IsShaderResourceStorage() const {
        return m_Type == DescriptorTypeExt::STORAGE_TEXTURE
            || m_Type == DescriptorTypeExt::STORAGE_BUFFER
            || m_Type == DescriptorTypeExt::STORAGE_STRUCTURED_BUFFER;
    }

    inline bool IsDepthReadonly() const {
        return m_IsDepthReadonly;
    }

    inline bool IsStencilReadonly() const {
        return m_IsStencilReadonly;
    }

private:
    DescriptorTypeExt m_Type = DescriptorTypeExt::MAX_NUM;
    bool m_IsDepthReadonly = false;
    bool m_IsStencilReadonly = false;
};

} // namespace nri
