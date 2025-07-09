// © 2021 NVIDIA Corporation

// Goal: convenient creation of resources, which get returned already bound to a memory.
// AMD Virtual Memory Allocator is used for "under the hood" memory allocations management:
//  https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
//  https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator

#pragma once

#define NRI_RESOURCE_ALLOCATOR_H 1

#include "NRIRayTracing.h"

NriNamespaceBegin

// See "Resources: binding to memory" section for more information
// "memoryPriority" - [-1; 1]: low < 0, normal = 0, high > 0
// "dedicated" - put a resource into a dedicated memory heap, containing only 1 object with offset = 0

NriStruct(AllocateBufferDesc) {
    Nri(BufferDesc) desc;
    Nri(MemoryLocation) memoryLocation;
    float memoryPriority;
    bool dedicated;
};

NriStruct(AllocateTextureDesc) {
    Nri(TextureDesc) desc;
    Nri(MemoryLocation) memoryLocation;
    float memoryPriority;
    bool dedicated;
};

NriStruct(AllocateAccelerationStructureDesc) {
    Nri(AccelerationStructureDesc) desc;
    Nri(MemoryLocation) memoryLocation;
    float memoryPriority;
    bool dedicated;
};

NriStruct(AllocateMicromapDesc) {
    Nri(MicromapDesc) desc;
    Nri(MemoryLocation) memoryLocation;
    float memoryPriority;
    bool dedicated;
};

// Threadsafe: yes
NriStruct(ResourceAllocatorInterface) {
    Nri(Result) (NRI_CALL *AllocateBuffer)                  (NriRef(Device) device, const NriRef(AllocateBufferDesc) bufferDesc, NriOut NriRef(Buffer*) buffer);
    Nri(Result) (NRI_CALL *AllocateTexture)                 (NriRef(Device) device, const NriRef(AllocateTextureDesc) textureDesc, NriOut NriRef(Texture*) texture);
    Nri(Result) (NRI_CALL *AllocateAccelerationStructure)   (NriRef(Device) device, const NriRef(AllocateAccelerationStructureDesc) accelerationStructureDesc, NriOut NriRef(AccelerationStructure*) accelerationStructure);
    Nri(Result) (NRI_CALL *AllocateMicromap)                (NriRef(Device) device, const NriRef(AllocateMicromapDesc) micromapDesc, NriOut NriRef(Micromap*) micromap);
};

NriNamespaceEnd
