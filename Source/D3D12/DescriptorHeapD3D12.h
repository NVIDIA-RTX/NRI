// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct DescriptorHeapD3D12 final : public DebugNameBase {
    inline DescriptorHeapD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    Result Create(const DescriptorHeapDesc& descriptorHeapDesc);
    Result WriteResourceDescriptors(const WriteResourceDescriptorsDesc* writeDescs, uint32_t writeDescNum);
    Result WriteSamplerDescriptors(const WriteSamplerDescriptorsDesc* writeDescs, uint32_t writeDescNum);
    void Bind(ID3D12GraphicsCommandList* commandList) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    Result CreateHeap(DescriptorHeapType type, uint32_t descriptorNum);

private:
    DeviceD3D12& m_Device;
    std::array<ComPtr<ID3D12DescriptorHeap>, DescriptorHeapType::MAX_NUM> m_Heaps;
    std::array<ID3D12DescriptorHeap*, DescriptorHeapType::MAX_NUM> m_HeapHandles = {};
    std::array<DescriptorHandleCPU, DescriptorHeapType::MAX_NUM> m_BaseHandles = {};
    std::array<uint32_t, DescriptorHeapType::MAX_NUM> m_DescriptorSizes = {};
    uint32_t m_HeapNum = 0;
};

} // namespace nri
