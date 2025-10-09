// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferVal;
struct MemoryVal;

struct AccelerationStructureVal final : public ObjectVal {
    AccelerationStructureVal(DeviceVal& device, AccelerationStructure* accelerationStructure, bool isBoundToMemory)
        : ObjectVal(device, accelerationStructure)
        , m_IsBoundToMemory(isBoundToMemory) {
    }

    ~AccelerationStructureVal();

    inline AccelerationStructure* GetImpl() const {
        return (AccelerationStructure*)m_Impl;
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

    uint64_t GetUpdateScratchBufferSize() const;
    uint64_t GetBuildScratchBufferSize() const;
    uint64_t GetHandle() const;
    uint64_t GetNativeObject() const;
    Buffer* GetBuffer();
    Result CreateDescriptor(Descriptor*& descriptor);

private:
    MemoryVal* m_Memory = nullptr;
    BufferVal* m_Buffer = nullptr;
    bool m_IsBoundToMemory = false;
};

} // namespace nri
