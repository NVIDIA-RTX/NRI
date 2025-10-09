// © 2021 NVIDIA Corporation

#include <algorithm>

bool MemoryVal::HasBoundResources() {
    ExclusiveScope lock(m_Lock);

    return !m_Buffers.empty() || !m_Textures.empty() || !m_AccelerationStructures.empty() || !m_Micromaps.empty();
}

void MemoryVal::ReportBoundResources() {
    ExclusiveScope lock(m_Lock);

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

    for (size_t i = 0; i < m_Micromaps.size(); i++) {
        MicromapVal& micromap = *m_Micromaps[i];
        REPORT_ERROR(&m_Device, "Micromap (%p '%s') is still bound to the memory", &micromap, micromap.GetDebugName());
    }
}

void MemoryVal::Bind(BufferVal& buffer) {
    ExclusiveScope lock(m_Lock);

    m_Buffers.push_back(&buffer);
    buffer.SetBoundToMemory(this);
}

void MemoryVal::Bind(TextureVal& texture) {
    ExclusiveScope lock(m_Lock);

    m_Textures.push_back(&texture);
    texture.SetBoundToMemory(this);
}

void MemoryVal::Bind(AccelerationStructureVal& accelerationStructure) {
    ExclusiveScope lock(m_Lock);

    m_AccelerationStructures.push_back(&accelerationStructure);
    accelerationStructure.SetBoundToMemory(this);
}

void MemoryVal::Bind(MicromapVal& micromap) {
    ExclusiveScope lock(m_Lock);

    m_Micromaps.push_back(&micromap);
    micromap.SetBoundToMemory(this);
}

void MemoryVal::Unbind(BufferVal& buffer) {
    ExclusiveScope lock(m_Lock);

    const auto it = std::find(m_Buffers.begin(), m_Buffers.end(), &buffer);
    if (it == m_Buffers.end()) {
        REPORT_ERROR(&m_Device, "Unexpected error: Can't find the buffer in the list of bound resources");
        return;
    }

    m_Buffers.erase(it);
}

void MemoryVal::Unbind(TextureVal& texture) {
    ExclusiveScope lock(m_Lock);

    const auto it = std::find(m_Textures.begin(), m_Textures.end(), &texture);
    if (it == m_Textures.end()) {
        REPORT_ERROR(&m_Device, "Unexpected error: Can't find the texture in the list of bound resources");
        return;
    }

    m_Textures.erase(it);
}

void MemoryVal::Unbind(AccelerationStructureVal& accelerationStructure) {
    ExclusiveScope lock(m_Lock);

    const auto it = std::find(m_AccelerationStructures.begin(), m_AccelerationStructures.end(), &accelerationStructure);
    if (it == m_AccelerationStructures.end()) {
        REPORT_ERROR(&m_Device, "Unexpected error: Can't find the acceleration structure in the list of bound resources");
        return;
    }

    m_AccelerationStructures.erase(it);
}

void MemoryVal::Unbind(MicromapVal& micromap) {
    ExclusiveScope lock(m_Lock);

    const auto it = std::find(m_Micromaps.begin(), m_Micromaps.end(), &micromap);
    if (it == m_Micromaps.end()) {
        REPORT_ERROR(&m_Device, "Unexpected error: Can't find the micromap in the list of bound resources");
        return;
    }

    m_Micromaps.erase(it);
}
