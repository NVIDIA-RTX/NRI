// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct HelperDataUpload {
    inline HelperDataUpload(const CoreInterface& NRI, Device& device, Queue& queue)
        : m_iCore(NRI)
        , m_Device(device)
        , m_Queue(queue) {
    }

    Result UploadData(const TextureUploadDesc* textureDataDescs, uint32_t textureDataDescNum, const BufferUploadDesc* bufferDataDescs, uint32_t bufferDataDescNum);

private:
    Result Create(const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum);
    Result UploadTextures(const TextureUploadDesc* textureDataDescs, uint32_t textureDataDescNum);
    Result UploadBuffers(const BufferUploadDesc* bufferDataDescs, uint32_t bufferDataDescNum);
    Result EndCommandBuffersAndSubmit();
    bool CopyTextureContent(const TextureUploadDesc& textureDataDesc, Dim_t& layerOffset, Dim_t& mipOffset);
    bool CopyBufferContent(const BufferUploadDesc& bufferDataDesc, uint64_t& bufferContentOffset);

    const CoreInterface& m_iCore;
    Device& m_Device;
    Queue& m_Queue;
    CommandBuffer* m_CommandBuffer = nullptr;
    Fence* m_Fence = nullptr;
    CommandAllocator* m_CommandAllocator = nullptr;
    Buffer* m_UploadBuffer = nullptr;
    Memory* m_UploadBufferMemory = nullptr;
    uint8_t* m_MappedMemory = nullptr;
    uint64_t m_UploadBufferSize = 0;
    uint64_t m_UploadBufferOffset = 0;
    uint64_t m_FenceValue = 1;
};

struct HelperDeviceMemoryAllocator {
    HelperDeviceMemoryAllocator(const CoreInterface& NRI, Device& device);

    uint32_t CalculateAllocationNumber(const ResourceGroupDesc& resourceGroupDesc);
    Result AllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations);

private:
    struct MemoryHeap {
        MemoryHeap(MemoryType memoryType, const StdAllocator<uint8_t>& stdAllocator);

        Vector<Buffer*> buffers;
        Vector<uint64_t> bufferOffsets;
        Vector<Texture*> textures;
        Vector<uint64_t> textureOffsets;
        uint64_t size;
        MemoryType type;
    };

    Result TryToAllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations, size_t& allocationNum);
    Result ProcessDedicatedResources(MemoryLocation memoryLocation, Memory** allocations, size_t& allocationNum);
    MemoryHeap& FindOrCreateHeap(const MemoryDesc& memoryDesc, uint64_t preferredMemorySize);
    void GroupByMemoryType(MemoryLocation memoryLocation, const ResourceGroupDesc& resourceGroupDesc);
    void FillMemoryBindingDescs(Buffer* const* buffers, const uint64_t* bufferOffsets, uint32_t bufferNum, Memory& memory);
    void FillMemoryBindingDescs(Texture* const* texture, const uint64_t* textureOffsets, uint32_t textureNum, Memory& memory);

    const CoreInterface& m_iCore;
    Device& m_Device;

    Vector<MemoryHeap> m_Heaps;
    Vector<Buffer*> m_DedicatedBuffers;
    Vector<Texture*> m_DedicatedTextures;
    Vector<BindBufferMemoryDesc> m_BufferBindingDescs;
    Vector<BindTextureMemoryDesc> m_TextureBindingDescs;
};

} // namespace nri