#import <MetalKit/MetalKit.h>

struct DeviceMTL;
struct MemoryMTL;

struct BufferMTL {

    inline BufferMTL(DeviceMTL& device)
        : m_Device(device) {
    }

    inline id<MTLBuffer> GetHandle() const {
        return pBuffer;
    }

    inline VkDeviceAddress GetDeviceAddress() const {
        return m_DeviceAddress;
    }

    inline DeviceMTL& GetDevice() const {
        return m_Device;
    }

    inline const BufferDesc& GetDesc() const {
        return m_Desc;
    }

    ~BufferMTL();
    
    Result Create(const BufferDesc& bufferDesc);
    Result Create(const BufferVKDesc& bufferDesc);
    Result Create(const AllocateBufferDesc& bufferDesc);

private:
    DeviceVK& m_Device;
    id<MTLBuffer> pBuffer;
    uint8_t* m_MappedMemory = nullptr;
    uint64_t m_MappedMemoryOffset = 0;
    uint64_t m_MappedMemoryRangeSize = 0;
    uint64_t m_MappedMemoryRangeOffset = 0;
    BufferDesc m_Desc = {};
    bool m_OwnsNativeObjects = true;
};



