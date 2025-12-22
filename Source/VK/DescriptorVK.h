// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct TextureVK;

struct TexViewDesc {
    const TextureVK* texture;
    VkImageLayout expectedLayout;
    Dim_t layerOrSliceOffset; // this is valid, because it's used only for https://docs.vulkan.org/refpages/latest/refpages/source/VkImageSubresourceRange.html
    Dim_t layerOrSliceNum;
    Dim_t mipOffset;
    Dim_t mipNum;
};

struct DescriptorVK final : public DebugNameBase {
    inline DescriptorVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline DescriptorType GetType() const {
        return m_Type;
    }

    inline Format GetFormat() const {
        return m_Format;
    }

    inline const TexViewDesc& GetTexViewDesc() const {
        return m_ViewDesc.texture;
    }

    inline const VkDescriptorBufferInfo& GetBufferInfo() const {
        return m_ViewDesc.buffer;
    }

    inline VkBufferView GetBufferView() const {
        return m_View.buffer;
    }

    inline VkImageView GetImageView() const {
        return m_View.image;
    }

    inline const VkSampler& GetSampler() const {
        return m_View.sampler;
    }

    inline VkAccelerationStructureKHR GetAccelerationStructure() const {
        return m_View.accelerationStructure;
    }

    inline bool IsDepthWritable() const {
        return m_ViewDesc.texture.expectedLayout != VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL && m_ViewDesc.texture.expectedLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }

    inline bool IsStencilWritable() const {
        return m_ViewDesc.texture.expectedLayout != VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL && m_ViewDesc.texture.expectedLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }

    ~DescriptorVK();

    Result Create(const BufferViewDesc& bufferViewDesc);
    Result Create(const Texture1DViewDesc& textureViewDesc);
    Result Create(const Texture2DViewDesc& textureViewDesc);
    Result Create(const Texture3DViewDesc& textureViewDesc);
    Result Create(const SamplerDesc& samplerDesc);
    Result Create(VkAccelerationStructureKHR accelerationStructure);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

private:
    Result CreateTextureView(const Texture1DViewDesc& textureViewDesc);
    Result CreateTextureView(const Texture2DViewDesc& textureViewDesc);
    Result CreateTextureView(const Texture3DViewDesc& textureViewDesc);

private:
    DeviceVK& m_Device;

    union View {
        VkImageView image = VK_NULL_HANDLE;
        VkBufferView buffer;
        VkSampler sampler;
        VkAccelerationStructureKHR accelerationStructure;
    } m_View;

    union ViewDesc {
        TexViewDesc texture = {}; // larger first
        VkDescriptorBufferInfo buffer;
    } m_ViewDesc;

    DescriptorType m_Type = DescriptorType::MAX_NUM;
    Format m_Format = Format::UNKNOWN;
};

} // namespace nri