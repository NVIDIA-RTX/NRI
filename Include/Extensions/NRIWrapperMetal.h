// © 2026 NVIDIA Corporation

// Goal: wrapping native Metal objects into NRI objects

#pragma once

#define NRI_WRAPPER_METAL_H 1

#include "NRIDeviceCreation.h"

NriNamespaceBegin

// Native pointers use these exact metal-cpp types: MTL::Device*, MTL4::CommandQueue*, MTL::Buffer*, MTL::Texture*, and MTL::SharedEvent*.
// NRI retains every supplied native object until the corresponding NRI object is destroyed.

// A collection of queues of the same type
NriStruct(QueueFamilyMetalDesc) {
    NriOptional void* const* mtl4Queues; // MTL4::CommandQueue*, if not provided, will be created
    uint32_t queueNum;
    Nri(QueueType) queueType;
};

NriStruct(DeviceCreationMetalDesc) {
    void* mtlDevice; // MTL::Device*
    const NriPtr(QueueFamilyMetalDesc) queueFamilies;
    uint32_t queueFamilyNum;
    NriOptional Nri(CallbackInterface) callbackInterface;
    NriOptional Nri(AllocationCallbacks) allocationCallbacks;

    // Switches (disabled by default)
    bool enableNRIValidation;
    bool enableMemoryZeroInitialization; // Metal memory is always zero-initialized
};

NriStruct(BufferMetalDesc) {
    void* mtlBuffer;      // MTL::Buffer*
    Nri(BufferDesc) desc; // complete NRI metadata for the native buffer
};

NriStruct(TextureMetalDesc) {
    void* mtlTexture;      // MTL::Texture*
    Nri(TextureDesc) desc; // complete NRI metadata for the native texture
};

NriStruct(FenceMetalDesc) {
    void* mtlSharedEvent; // MTL::SharedEvent*
};

// Threadsafe: yes
NriStruct(WrapperMetalInterface) {
    Nri(Result) (NRI_CALL *CreateBufferMetal)       (NriRef(Device) device, const NriRef(BufferMetalDesc) bufferMetalDesc, NriOut NriRef(Buffer*) buffer);
    Nri(Result) (NRI_CALL *CreateTextureMetal)      (NriRef(Device) device, const NriRef(TextureMetalDesc) textureMetalDesc, NriOut NriRef(Texture*) texture);
    Nri(Result) (NRI_CALL *CreateFenceMetal)        (NriRef(Device) device, const NriRef(FenceMetalDesc) fenceMetalDesc, NriOut NriRef(Fence*) fence);
};

NRI_API Nri(Result) NRI_CALL nriCreateDeviceFromMetalDevice(const NriRef(DeviceCreationMetalDesc) deviceDesc, NriOut NriRef(Device*) device);

NriNamespaceEnd
