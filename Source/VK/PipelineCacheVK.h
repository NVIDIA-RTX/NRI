// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineCacheVK final : public DebugNameBase {
    inline PipelineCacheVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline operator VkPipelineCache() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    ~PipelineCacheVK();

    Result Create(const PipelineCacheDesc& pipelineCacheDesc);
    Result GetData(void* dst, uint64_t& size) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    DeviceVK& m_Device;
    VkPipelineCache m_Handle = VK_NULL_HANDLE;
};

} // namespace nri
