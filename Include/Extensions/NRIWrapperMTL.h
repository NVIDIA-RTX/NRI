// © 2021 NVIDIA Corporation

#pragma once

#include "NRIDeviceCreation.h"

NRI_NAMESPACE_BEGIN


NRI_ENUM
(
    MTLGPUFamily, uint8_t,

    Apple1,
    Apple2,
    Apple3,
    Apple4,
    Apple5,
    Apple6,
    Apple7,
    Mac1,
    Mac2,
    
    MAX_APPLE_FAMILY_NUM
);


NRI_STRUCT(DeviceCreationMTLDesc)
{
    bool enableNRIValidation;
    id<MTLDevice>     MtlDevice;
}

NRI_STRUCT(CommandBufferMTLDesc)
{

}

NRI_STRUCT(BufferMTLDesc)
{
  id<MTLBuffer> buffer;
  void* mappedMemory;
  MTLResourceOptions options; 
}
 
NRI_STRUCT(TextureMTLDesc)
{
  id<MTLTexture> mtlTexture;
  MTLTextureDescriptor* descriptor; 
}
 

NRI_STRUCT(MemoryMTLDesc)
{
  id<MTLBuffer> buffer;
  void* mappedMemory;
  MTLResourceOptions options; 
}

NRI_API NRI_NAME(Result) NRI_CALL nriCreateDeviceFromMtlDevice(const NRI_NAME_REF(DeviceCreationMTLDesc) deviceDesc, NRI_NAME_REF(Device*) device);

NRI_NAMESPACE_END

