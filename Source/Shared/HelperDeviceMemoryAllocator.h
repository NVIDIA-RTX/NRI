// © 2021 NVIDIA Corporation

#pragma once

template <typename U, typename T>
using Map = std::map<U, T, std::less<U>, StdAllocator<std::pair<const U, T>>>;

namespace nri {

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
    MemoryHeap& FindOrCreateHeap(MemoryDesc& memoryDesc, uint64_t preferredMemorySize);
    void GroupByMemoryType(MemoryLocation memoryLocation, const ResourceGroupDesc& resourceGroupDesc);
    void FillMemoryBindingDescs(Buffer* const* buffers, const uint64_t* bufferOffsets, uint32_t bufferNum, Memory& memory);
    void FillMemoryBindingDescs(Texture* const* texture, const uint64_t* textureOffsets, uint32_t textureNum, Memory& memory);

    const CoreInterface& m_NRI;
    Device& m_Device;

    Vector<MemoryHeap> m_Heaps;
    Vector<Buffer*> m_DedicatedBuffers;
    Vector<Texture*> m_DedicatedTextures;
    Vector<BufferMemoryBindingDesc> m_BufferBindingDescs;
    Vector<TextureMemoryBindingDesc> m_TextureBindingDescs;
};

}
