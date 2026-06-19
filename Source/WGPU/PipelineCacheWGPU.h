// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineCacheWGPU final : public DebugNameBase {
    inline PipelineCacheWGPU(DeviceWGPU& device)
        : m_Device(device)
        , m_Data(device.GetStdAllocator()) {
    }

    inline DeviceWGPU& GetDevice() const {
        return m_Device;
    }

    Result Create(const PipelineCacheDesc& pipelineCacheDesc);
    Result GetData(void* dst, uint64_t& size) const;

private:
    DeviceWGPU& m_Device;
    Vector<uint8_t> m_Data;
};

} // namespace nri
