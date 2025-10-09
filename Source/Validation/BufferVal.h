// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct MemoryVal;

struct BufferVal final : public ObjectVal {
    BufferVal(DeviceVal& device, Buffer* buffer, bool isBoundToMemory)
        : ObjectVal(device, buffer)
        , m_IsBoundToMemory(isBoundToMemory) {
        m_Desc = GetCoreInterfaceImpl().GetBufferDesc(*buffer);
    }

    ~BufferVal();

    inline Buffer* GetImpl() const {
        return (Buffer*)m_Impl;
    }

    inline const BufferDesc& GetDesc() const {
        return m_Desc;
    }

    inline uint64_t GetNativeObject() const {
        return GetCoreInterfaceImpl().GetBufferNativeObject(GetImpl());
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

    void* Map(uint64_t offset, uint64_t size);
    void Unmap();

private:
    BufferDesc m_Desc = {}; // (only for) .natvis
    MemoryVal* m_Memory = nullptr;
    bool m_IsBoundToMemory = false;
    bool m_IsMapped = false;
};

} // namespace nri
