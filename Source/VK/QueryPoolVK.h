// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct QueryPoolVK final : public DebugNameBase {
    inline QueryPoolVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline VkQueryPool GetHandle() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline VkQueryType GetType() const {
        return m_Type;
    }

    ~QueryPoolVK();

    Result Create(const QueryPoolDesc& queryPoolDesc);
    Result Create(const QueryPoolVKDesc& queryPoolVKDesc);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    void Reset(uint32_t offset, uint32_t num);

    inline uint32_t GetQuerySize() const {
        return m_QuerySize;
    }

private:
    DeviceVK& m_Device;
    VkQueryPool m_Handle = VK_NULL_HANDLE;
    VkQueryType m_Type = (VkQueryType)0;
    uint32_t m_QuerySize = 0;
    bool m_OwnsNativeObjects = true;
};

} // namespace nri