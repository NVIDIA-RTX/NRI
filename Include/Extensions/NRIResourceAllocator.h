// © 2021 NVIDIA Corporation

#pragma once

#include "NRIRayTracing.h"

// Convenient creation of resources, which get returned already bound to memory.
// AMD Virtual Memory Allocator is used for memory allocations management:
//  https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
//  https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator

NriNamespaceBegin

NriStruct(AllocateBufferDesc) {
    Nri(BufferDesc) desc;
    Nri(MemoryLocation) memoryLocation;
    float memoryPriority; // [-1; 1]: low < 0, normal = 0, high > 0
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

NriStruct(ResourceAllocatorInterface) {
    Nri(Result) (NRI_CALL *AllocateBuffer)                  (NriRef(Device) device, const NriRef(AllocateBufferDesc) bufferDesc, NriOut NriRef(Buffer*) buffer);
    Nri(Result) (NRI_CALL *AllocateTexture)                 (NriRef(Device) device, const NriRef(AllocateTextureDesc) textureDesc, NriOut NriRef(Texture*) texture);
    Nri(Result) (NRI_CALL *AllocateAccelerationStructure)   (NriRef(Device) device, const NriRef(AllocateAccelerationStructureDesc) accelerationStructureDesc, NriOut NriRef(AccelerationStructure*) accelerationStructure);
};

NriNamespaceEnd
