// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineCacheVal final : public ObjectVal {
    PipelineCacheVal(DeviceVal& device, PipelineCache* pipelineCache);

    inline PipelineCache* GetImpl() const {
        return (PipelineCache*)m_Impl;
    }

    Result GetData(void* dst, uint64_t& size);
};

} // namespace nri
