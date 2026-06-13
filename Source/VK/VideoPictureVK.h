// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct VideoPictureVK final : public DebugNameBase {
    inline VideoPictureVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~VideoPictureVK();

    //================================================================================================================    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)m_ImageView, name);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result Create(const VideoPictureDesc& videoPictureDesc);

    DeviceVK& m_Device;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkVideoPictureResourceInfoKHR m_Resource = {};
};

} // namespace nri
