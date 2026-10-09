// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct AccelerationStructureVal final : public ObjectVal {
    AccelerationStructureVal(DeviceVal& device, AccelerationStructure* accelerationStructure, bool isBoundToMemory, const AccelerationStructureDesc* desc = nullptr)
        : ObjectVal(device, accelerationStructure)
        , m_IsBoundToMemory(isBoundToMemory) {
        if (desc) {
            m_Type = desc->type;
            m_Flags = desc->flags;
            m_GeometryOrInstanceNum = desc->geometryOrInstanceNum;
        }
    }

    ~AccelerationStructureVal();

    inline AccelerationStructure* GetImpl() const {
        return (AccelerationStructure*)m_Impl;
    }

    inline AccelerationStructureType GetType() const {
        return m_Type; // "MAX_NUM" if unknown (wrapped)
    }

    inline AccelerationStructureBits GetFlags() const {
        return m_Flags;
    }

    inline uint32_t GetGeometryOrInstanceNum() const {
        return m_GeometryOrInstanceNum;
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
    uint32_t m_GeometryOrInstanceNum = 0;
    AccelerationStructureType m_Type = AccelerationStructureType::MAX_NUM;
    AccelerationStructureBits m_Flags = AccelerationStructureBits::NONE;
    bool m_IsBoundToMemory = false;
};

} // namespace nri
