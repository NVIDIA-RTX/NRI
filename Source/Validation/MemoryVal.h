// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct BufferVal;
struct TextureVal;
struct AccelerationStructureVal;
struct MicromapVal;

struct MemoryVal final : public ObjectVal {
    inline MemoryVal(DeviceVal& device, Memory* memory, uint64_t size, MemoryLocation memoryLocation)
        : ObjectVal(device, memory)
        , m_Buffers(device.GetStdAllocator())
        , m_Textures(device.GetStdAllocator())
        , m_AccelerationStructures(device.GetStdAllocator())
        , m_Micromaps(device.GetStdAllocator())
        , m_Size(size)
        , m_MemoryLocation(memoryLocation) {
    }

    inline Memory* GetImpl() const {
        return (Memory*)m_Impl;
    }

    inline uint64_t GetSize() const {
        return m_Size;
    }

    inline MemoryLocation GetMemoryLocation() const {
        return m_MemoryLocation;
    }

    inline bool IsWrapped() const {
        return m_MemoryLocation == MemoryLocation::MAX_NUM;
    }

    bool HasBoundResources();
    void ReportBoundResources();
    void Bind(BufferVal& buffer);
    void Bind(TextureVal& texture);
    void Bind(AccelerationStructureVal& accelerationStructure);
    void Bind(MicromapVal& micromap);
    void Unbind(BufferVal& buffer);
    void Unbind(TextureVal& texture);
    void Unbind(AccelerationStructureVal& accelerationStructure);
    void Unbind(MicromapVal& micromap);

private:
    Vector<BufferVal*> m_Buffers;
    Vector<TextureVal*> m_Textures;
    Vector<AccelerationStructureVal*> m_AccelerationStructures;
    Vector<MicromapVal*> m_Micromaps;
    uint64_t m_Size = 0;
    MemoryLocation m_MemoryLocation = MemoryLocation::MAX_NUM; // wrapped object
    Lock m_Lock;
};

} // namespace nri
