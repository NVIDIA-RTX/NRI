// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferVal;
struct TextureVal;
struct AccelerationStructureVal;

struct MemoryVal final : public ObjectVal {
    MemoryVal(DeviceVal& device, Memory* memory, uint64_t size, MemoryLocation memoryLocation);

    inline Memory* GetImpl() const {
        return (Memory*)m_Impl;
    }

    inline uint64_t GetSize() const {
        return m_Size;
    }

    inline MemoryLocation GetMemoryLocation() const {
        return m_MemoryLocation;
    }

    bool HasBoundResources();
    void ReportBoundResources();
    void UnbindBuffer(BufferVal& buffer);
    void UnbindTexture(TextureVal& texture);
    void UnbindAccelerationStructure(AccelerationStructureVal& accelerationStructure);
    void BindBuffer(BufferVal& buffer);
    void BindTexture(TextureVal& texture);
    void BindAccelerationStructure(AccelerationStructureVal& accelerationStructure);

private:
    Vector<BufferVal*> m_Buffers;
    Vector<TextureVal*> m_Textures;
    Vector<AccelerationStructureVal*> m_AccelerationStructures;
    uint64_t m_Size = 0;
    MemoryLocation m_MemoryLocation = MemoryLocation::MAX_NUM; // wrapped object
    Lock m_Lock;
};

} // namespace nri
