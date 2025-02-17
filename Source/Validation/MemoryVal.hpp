// © 2021 NVIDIA Corporation

#include <algorithm>

MemoryVal::MemoryVal(DeviceVal& device, Memory* memory, uint64_t size, MemoryLocation memoryLocation)
    : ObjectVal(device, memory)
    , m_Buffers(device.GetStdAllocator())
    , m_Textures(device.GetStdAllocator())
    , m_AccelerationStructures(device.GetStdAllocator())
    , m_Size(size)
    , m_MemoryLocation(memoryLocation) {
}

bool MemoryVal::HasBoundResources() {
    ExclusiveScope lockScope(m_Lock);
    return !m_Buffers.empty() || !m_Textures.empty() || !m_AccelerationStructures.empty();
}

void MemoryVal::ReportBoundResources() {
    ExclusiveScope lockScope(m_Lock);

    for (size_t i = 0; i < m_Buffers.size(); i++) {
        BufferVal& buffer = *m_Buffers[i];
        REPORT_ERROR(&m_Device, "Buffer (%p '%s') is still bound to the memory", &buffer, buffer.GetDebugName());
    }

    for (size_t i = 0; i < m_Textures.size(); i++) {
        TextureVal& texture = *m_Textures[i];
        REPORT_ERROR(&m_Device, "Texture (%p '%s') is still bound to the memory", &texture, texture.GetDebugName());
    }

    for (size_t i = 0; i < m_AccelerationStructures.size(); i++) {
        AccelerationStructureVal& accelerationStructure = *m_AccelerationStructures[i];
        REPORT_ERROR(&m_Device, "AccelerationStructure (%p '%s') is still bound to the memory", &accelerationStructure, accelerationStructure.GetDebugName());
    }
}

void MemoryVal::UnbindBuffer(BufferVal& buffer) {
    ExclusiveScope lockScope(m_Lock);

    const auto it = std::find(m_Buffers.begin(), m_Buffers.end(), &buffer);
    if (it == m_Buffers.end()) {
        REPORT_ERROR(&m_Device, "Unexpected error: Can't find the buffer in the list of bound resources");
        return;
    }

    m_Buffers.erase(it);
}

void MemoryVal::UnbindTexture(TextureVal& texture) {
    ExclusiveScope lockScope(m_Lock);

    const auto it = std::find(m_Textures.begin(), m_Textures.end(), &texture);
    if (it == m_Textures.end()) {
        REPORT_ERROR(&m_Device, "Unexpected error: Can't find the texture in the list of bound resources");
        return;
    }

    m_Textures.erase(it);
}

void MemoryVal::UnbindAccelerationStructure(AccelerationStructureVal& accelerationStructure) {
    ExclusiveScope lockScope(m_Lock);

    const auto it = std::find(m_AccelerationStructures.begin(), m_AccelerationStructures.end(), &accelerationStructure);
    if (it == m_AccelerationStructures.end()) {
        REPORT_ERROR(&m_Device, "Unexpected error: Can't find the acceleration structure in the list of bound resources");
        return;
    }

    m_AccelerationStructures.erase(it);
}

void MemoryVal::BindBuffer(BufferVal& buffer) {
    ExclusiveScope lockScope(m_Lock);
    m_Buffers.push_back(&buffer);
    buffer.SetBoundToMemory(this);
}

void MemoryVal::BindTexture(TextureVal& texture) {
    ExclusiveScope lockScope(m_Lock);
    m_Textures.push_back(&texture);
    texture.SetBoundToMemory(this);
}

void MemoryVal::BindAccelerationStructure(AccelerationStructureVal& accelerationStructure) {
    ExclusiveScope lockScope(m_Lock);
    m_AccelerationStructures.push_back(&accelerationStructure);
    accelerationStructure.SetBoundToMemory(*this);
}
