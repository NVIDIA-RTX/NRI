// © 2026 NVIDIA Corporation

#pragma once

namespace nri {

struct PipelineCacheD3D12 final : public DebugNameBase {
    inline PipelineCacheD3D12(DeviceD3D12& device)
        : m_Device(device)
        , m_Blob(device.GetStdAllocator()) {
    }

    inline ~PipelineCacheD3D12() {
        // m_Library must be released BEFORE m_Blob is freed - the library's lifetime depends on the blob it was created from.
        // ComPtr's destruction order follows reverse declaration order, but be explicit to make the dependency obvious.
        m_Library = nullptr;
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

    Result Create(const PipelineCacheDesc& pipelineCacheDesc);
    Result GetData(void* dst, uint64_t& size) const;

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE {
        NRI_SET_D3D_DEBUG_OBJECT_NAME(m_Library, name);
    }

private:
    DeviceD3D12& m_Device;
    Vector<uint8_t> m_Blob;                       // owned copy - "ID3D12Device1::CreatePipelineLibrary" requires the source data to outlive the library
    ComPtr<ID3D12PipelineLibrary1> m_Library;
    Lock m_StoreLock;
};

} // namespace nri
