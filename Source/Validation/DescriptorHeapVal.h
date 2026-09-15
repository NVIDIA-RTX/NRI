// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorHeapVal final : public ObjectVal {
    inline DescriptorHeapVal(DeviceVal& device, DescriptorHeap* descriptorHeap, const DescriptorHeapDesc& desc)
        : ObjectVal(device, (Object*)descriptorHeap)
        , m_Desc(desc) {
    }

    inline DescriptorHeap* GetImpl() const {
        return (DescriptorHeap*)m_Impl;
    }

    Result WriteResourceDescriptors(const WriteResourceDescriptorsDesc* writeDescs, uint32_t writeDescNum);
    Result WriteSamplerDescriptors(const WriteSamplerDescriptorsDesc* writeDescs, uint32_t writeDescNum);

private:
    DescriptorHeapDesc m_Desc = {};
};

} // namespace nri
