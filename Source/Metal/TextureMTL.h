// © 2021 NVIDIA Corporation
#pragma once

#import <MetalKit/MetalKit.h>

namespace nri {

struct DeviceMTL;

class MemoryMTL;

void fillMTLTextureDescriptor(const TextureDesc& textureDesc, MTLTextureDescriptor* mtlDescriptor);

struct TextureMTL {
public:
    inline TextureMTL(DeviceMTL& device)
        : m_Device(device) {
    }
    ~TextureMTL();
    
    inline id<MTLTexture> GetHandle() const {
         return m_Handle;
    }

    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }

    inline const TextureDesc& GetDesc() const {
        return m_Desc;
    }

    Result Create(const TextureDesc& textureDesc);
    void FinishMemoryBinding(MemoryMTL& memory, uint64_t memoryOffset);
    
    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    
private:
    
    void UpdateLabel();
    NSString* m_Label = nullptr;
    DeviceMTL& m_Device;
    TextureDesc m_Desc = {};
    id<MTLTexture> m_Handle;
    NSString* m_label;
};

} // namespace nri
