// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorHeapVK final : public DebugNameBase {
    inline DescriptorHeapVK(DeviceVK& device)
        : m_Device(device)
        , m_CustomBorderColorIndices(device.GetStdAllocator()) {
    }

    ~DescriptorHeapVK();

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    Result Create(const DescriptorHeapDesc& descriptorHeapDesc);
    Result WriteResourceDescriptors(const WriteResourceDescriptorsDesc* writeDescs, uint32_t writeDescNum);
    Result WriteSamplerDescriptors(const WriteSamplerDescriptorsDesc* writeDescs, uint32_t writeDescNum);
    void Bind(VkCommandBuffer commandBuffer) const;

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    Result CreateHeap(BufferVK*& heap, VkBindHeapInfoEXT& bindInfo, uint64_t descriptorNum, uint64_t descriptorSize, uint64_t reservedSize, uint64_t alignment);
    Result WriteResourceDescriptorsInternal(const WriteResourceDescriptorsDesc* writeDescs, uint32_t writeDescNum);
    Result WriteSamplerDescriptorsInternal(const WriteSamplerDescriptorsDesc* writeDescs, uint32_t writeDescNum);

private:
    DeviceVK& m_Device;
    BufferVK* m_ResourceHeap = nullptr;
    BufferVK* m_SamplerHeap = nullptr;
    VkBindHeapInfoEXT m_ResourceBindInfo = {VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT};
    VkBindHeapInfoEXT m_SamplerBindInfo = {VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT};
    Vector<uint32_t> m_CustomBorderColorIndices;
    uint64_t m_ResourceDescriptorSize = 0;
    uint64_t m_SamplerDescriptorSize = 0;
    Lock m_ResourceLock;
    Lock m_SamplerLock;
};

} // namespace nri
