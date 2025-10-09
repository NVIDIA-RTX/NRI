// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferVal;
struct MemoryVal;

struct MicromapVal final : public ObjectVal {
    MicromapVal(DeviceVal& device, Micromap* micromap, bool isBoundToMemory)
        : ObjectVal(device, micromap)
        , m_IsBoundToMemory(isBoundToMemory) {
    }

    ~MicromapVal();

    inline Micromap* GetImpl() const {
        return (Micromap*)m_Impl;
    }

    inline bool IsBoundToMemory() const {
        return m_IsBoundToMemory;
    }

    inline void SetBoundToMemory(MemoryVal* memory) {
        m_Memory = memory;
        m_IsBoundToMemory = true;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    uint64_t GetBuildScratchBufferSize() const;
    uint64_t GetNativeObject() const;
    Buffer* GetBuffer();

private:
    MemoryVal* m_Memory = nullptr;
    BufferVal* m_Buffer = nullptr;
    bool m_IsBoundToMemory = false;
};

} // namespace nri
