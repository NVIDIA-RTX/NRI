// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineCacheD3D12 final : public DebugNameBase {
    inline PipelineCacheD3D12(DeviceD3D12& device)
        : m_Device(device)
        , m_Blob(device.GetStdAllocator()) {
    }

    inline ~PipelineCacheD3D12() {
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline ID3D12PipelineLibrary1* GetLibrary() const {
        return m_Library.GetInterface();
    }

    inline Lock& GetStoreLock() {
        return m_StoreLock;
    }

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_Library, name);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result Create(const PipelineCacheDesc& pipelineCacheDesc);
    Result GetData(void* dst, uint64_t& size) const;

private:
    DeviceD3D12& m_Device;
    Vector<uint8_t> m_Blob; // do not sort, becaquse "m_Library" must be released before "m_Blob"
    ComPtr<ID3D12PipelineLibrary1> m_Library;
    Lock m_StoreLock;
};

} // namespace nri
