// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct QueueWGPU final : public DebugNameBase {
    inline QueueWGPU(DeviceWGPU& device)
        : m_Device(device) {
    }

    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    Result Create(QueueType queueType, uint32_t queueIndex);

    void BeginAnnotation(const char* name, uint32_t bgra);
    void EndAnnotation();
    void Annotation(const char* name, uint32_t bgra);
    void GetCalibratedTimestamps(uint64_t& timestampGPU, uint64_t& timestampCPU);
    Result Submit(const QueueSubmitDesc& queueSubmitDesc);
    Result WaitIdle();

private:
    DeviceWGPU& m_Device;
    WGPUSubmissionIndex m_LastSubmissionIndex = 0;
    QueueType m_Type = QueueType::MAX_NUM;
    uint32_t m_Index = 0;
};

} // namespace nri
