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

// Pre-converted DXIL: a ShaderMake Metal converter bundle ("ShaderMake -p METAL --metalFromDXIL", "SMMB", see "ShaderMake/ShaderBlob.h") can be passed
// to "ShaderDesc::bytecode" in any Metal backend build, the function name and thread group sizes come from its reflection.
// Required "--metalShaderConverterOptions": "--root-signature=<file>" with "GetRootSignatureMetal" output for the pipeline layout (checked against
// the reflection, except descriptor ranges and flags), "--framebuffer-fetch-register-space=998", "--minimum-gpu-family=<family>" supported by the device,
// "--samplerLODBias" if "MTLGPUFamilyApple10" is unsupported, "--dual-source-blending-support=forceEnabled" for fragment shaders used with "SRC1" blending.
// Metal vertex fetch must be used (no "--vertex-stage-in"). Not supported with geometry or tessellation shaders, point topologies, a sample mask and in ray tracing pipelines.
// Only NRI validation checks the bundle structure, untrusted data must be validated

// Threadsafe: yes
NriStruct(WrapperMetalInterface) {
    Nri(Result) (NRI_CALL *CreateBufferMetal)       (NriRef(Device) device, const NriRef(BufferMetalDesc) bufferMetalDesc, NriOut NriRef(Buffer*) buffer);
    Nri(Result) (NRI_CALL *CreateTextureMetal)      (NriRef(Device) device, const NriRef(TextureMetalDesc) textureMetalDesc, NriOut NriRef(Texture*) texture);
    Nri(Result) (NRI_CALL *CreateFenceMetal)        (NriRef(Device) device, const NriRef(FenceMetalDesc) fenceMetalDesc, NriOut NriRef(Fence*) fence);

    // Converter root signature of a pipeline layout as "metal-shaderconverter --root-signature" JSON (null-terminated, "size" includes the terminator).
    // Requires "shaderBytecodeDXIL", otherwise "size" is set to 0 if "json" is NULL, or "UNSUPPORTED" is returned
    Nri(Result) (NRI_CALL *GetRootSignatureMetal)   (const NriRef(PipelineLayout) pipelineLayout, NriOut char* json, NonNriRef(uint64_t) size);
};

NRI_API Nri(Result) NRI_CALL nriCreateDeviceFromMetalDevice(const NriRef(DeviceCreationMetalDesc) deviceDesc, NriOut NriRef(Device*) device);

NriNamespaceEnd
