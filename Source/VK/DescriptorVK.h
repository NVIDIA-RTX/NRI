// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct TexViewDescVK {
    const TextureVK* texture;
    VkImageLayout expectedLayout;
    VkImageAspectFlags aspectMask;
    Dim_t layerOrSliceOffset; // this is valid, because it's used only for https://docs.vulkan.org/refpages/latest/refpages/source/VkImageSubresourceRange.html
    Dim_t layerOrSliceNum;
    Dim_t mipOffset;
    Dim_t mipNum;
};

struct BufferViewDescVK {
    const BufferVK* buffer;
    VkDeviceSize offset;
    VkDeviceSize range;
};

struct SamplerViewDescVK {
    Color customBorderColor;
    float mipBias;
    float mipMin;
    float mipMax;
    uint32_t packed;
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

    inline const TexViewDescVK& GetTexViewDesc() const {
        return m_ViewDesc.texture;
    }

    inline VkDescriptorBufferInfo GetBufferInfo() const {
        if (m_Type == DescriptorType::ACCELERATION_STRUCTURE)
            return {};

        return {m_ViewDesc.buffer.buffer->GetHandle(), m_ViewDesc.buffer.offset, m_ViewDesc.buffer.range};
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
        return m_Type == DescriptorType::ACCELERATION_STRUCTURE ? m_View.accelerationStructure : VK_NULL_HANDLE;
    }

    inline VkDeviceAddress GetDeviceAddress() const {
        if (m_Type == DescriptorType::ACCELERATION_STRUCTURE)
            return m_ViewDesc.accelerationStructureAddress;

        return m_ViewDesc.buffer.buffer->GetDeviceAddress() + m_ViewDesc.buffer.offset;
    }

    inline bool IsDepthWritable() const {
        return m_ViewDesc.texture.expectedLayout != VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL && m_ViewDesc.texture.expectedLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }

    inline bool IsStencilWritable() const {
        return m_ViewDesc.texture.expectedLayout != VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL && m_ViewDesc.texture.expectedLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }

    ~DescriptorVK();

    Result Create(const BufferViewDesc& bufferViewDesc);
    Result Create(const TextureViewDesc& textureViewDesc);
    Result Create(const SamplerDesc& samplerDesc);
    Result Create(const AccelerationStructureVK& accelerationStructure);
    void FillImageViewCreateInfo(VkImageViewCreateInfo& createInfo, VkImageViewUsageCreateInfo& usageInfo, VkImageViewSlicedCreateInfoEXT& slicesInfo) const;
    void FillSamplerDesc(SamplerDesc& samplerDesc) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    DeviceVK& m_Device;

    union View {
        VkImageView image = VK_NULL_HANDLE;
        VkBufferView buffer;
        VkSampler sampler;
        VkAccelerationStructureKHR accelerationStructure;
    } m_View;

    union ViewDesc {
        SamplerViewDescVK sampler = {}; // larger first
        TexViewDescVK texture;
        BufferViewDescVK buffer;
        VkDeviceAddress accelerationStructureAddress;
    } m_ViewDesc;

    ComponentMapping m_TextureComponents = {};
    DescriptorType m_Type = DescriptorType::MAX_NUM;
    Format m_Format = Format::UNKNOWN;
    TextureView m_TextureView = TextureView::TEXTURE;
};

} // namespace nri
