// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct QueueVK final : public DebugNameBase {
    inline QueueVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline operator VkQueue() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline uint32_t GetFamilyIndex() const {
        return m_FamilyIndex;
    }

    inline QueueType GetType() const {
        return m_Type;
    }

    inline Lock& GetLock() {
        return m_Lock;
    }

    Result Create(QueueType type, uint32_t familyIndex, VkQueue handle);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // NRI
    //================================================================================================================

    void BeginAnnotation(const char* name, uint32_t bgra);
    void EndAnnotation();
    void Annotation(const char* name, uint32_t bgra);
    Result Submit(const QueueSubmitDesc& queueSubmitDesc);
    Result WaitIdle();

private:
    DeviceVK& m_Device;
    VkQueue m_Handle = VK_NULL_HANDLE;
    uint32_t m_FamilyIndex = INVALID_FAMILY_INDEX;
    QueueType m_Type = QueueType(-1);
    Lock m_Lock;
};

} // namespace nri
