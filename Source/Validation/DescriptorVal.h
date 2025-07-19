// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

enum class ResourceType {
    NONE,
    BUFFER,
    TEXTURE,
    SAMPLER,
    ACCELERATION_STRUCTURE
};

enum class ResourceViewType {
    NONE,
    COLOR_ATTACHMENT,
    DEPTH_STENCIL_ATTACHMENT,
    SHADER_RESOURCE,
    SHADER_RESOURCE_STORAGE,
    CONSTANT_BUFFER,
    SHADING_RATE_ATTACHMENT
};

struct DescriptorVal final : public ObjectVal {
    DescriptorVal(DeviceVal& device, Descriptor* descriptor, ResourceType resourceType);
    DescriptorVal(DeviceVal& device, Descriptor* descriptor, const BufferViewDesc& bufferViewDesc);
    DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture1DViewDesc& textureViewDesc);
    DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture2DViewDesc& textureViewDesc);
    DescriptorVal(DeviceVal& device, Descriptor* descriptor, const Texture3DViewDesc& textureViewDesc);
    DescriptorVal(DeviceVal& device, Descriptor* descriptor);

    inline Descriptor* GetImpl() const {
        return (Descriptor*)m_Impl;
    }

    inline uint64_t GetNativeObject() const {
        return GetCoreInterfaceImpl().GetDescriptorNativeObject(GetImpl());
    }

    inline bool IsBufferView() const {
        return m_ResourceType == ResourceType::BUFFER;
    }

    inline bool IsTextureView() const {
        return m_ResourceType == ResourceType::TEXTURE;
    }

    inline bool IsSampler() const {
        return m_ResourceType == ResourceType::SAMPLER;
    }

    inline bool IsAccelerationStructure() const {
        return m_ResourceType == ResourceType::ACCELERATION_STRUCTURE;
    }

    inline bool IsConstantBuffer() const {
        return m_ResourceType == ResourceType::BUFFER && m_ResourceViewType == ResourceViewType::CONSTANT_BUFFER;
    }

    inline bool IsColorAttachment() const {
        return IsTextureView() && m_ResourceViewType == ResourceViewType::COLOR_ATTACHMENT;
    }

    inline bool IsDepthStencilAttachment() const {
        return IsTextureView() && m_ResourceViewType == ResourceViewType::DEPTH_STENCIL_ATTACHMENT;
    }

    inline bool IsShaderResource() const {
        return m_ResourceType != ResourceType::NONE && !IsSampler() && m_ResourceViewType == ResourceViewType::SHADER_RESOURCE;
    }

    inline bool IsShaderResourceStorage() const {
        return m_ResourceType != ResourceType::NONE && !IsSampler() && m_ResourceViewType == ResourceViewType::SHADER_RESOURCE_STORAGE;
    }

    inline bool IsDepthReadonly() const {
        return m_IsDepthReadonly;
    }

    inline bool IsStencilReadonly() const {
        return m_IsStencilReadonly;
    }

private:
    ResourceType m_ResourceType = ResourceType::NONE;
    ResourceViewType m_ResourceViewType = ResourceViewType::NONE;
    bool m_IsDepthReadonly = false;
    bool m_IsStencilReadonly = false;
};

} // namespace nri
