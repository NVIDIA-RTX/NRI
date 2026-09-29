// © 2026 NVIDIA Corporation

constexpr uint64_t UPLOAD_CHUNK_SIZE_METAL = 256 * 1024;
constexpr uint32_t TRANSIENT_MAX_UNUSED_RESETS_METAL = 8;

CommandAllocatorMetal::~CommandAllocatorMetal() {
    for (UploadChunkMetal& chunk : m_UploadChunks)
        Release(chunk.buffer);

    for (TransientResourceMetal& transient : m_TransientResources)
        Release(transient.resource);

    if (m_Allocator)
        m_Allocator->release();
}

void CommandAllocatorMetal::Release(MTL::Resource* resource) {
    m_Device.RemoveResidency(resource);
    resource->release();
}

Result CommandAllocatorMetal::Create() {
    m_Allocator = m_Device.GetNativeObject()->newCommandAllocator();

    return m_Allocator ? Result::SUCCESS : Result::FAILURE;
}

void CommandAllocatorMetal::Reset() {
    m_Allocator->reset();
    m_ResetIndex++;

    // The GPU is done with the previous recordings: rewind recently used memory, release memory unused for a while
    for (size_t i = m_UploadChunks.size(); i > 0; i--) {
        UploadChunkMetal& chunk = m_UploadChunks[i - 1];
        chunk.offset = 0;

        if (m_ResetIndex - chunk.lastUsed > TRANSIENT_MAX_UNUSED_RESETS_METAL) {
            Release(chunk.buffer);
            chunk = m_UploadChunks.back();
            m_UploadChunks.pop_back();
        }
    }

    for (size_t i = m_TransientResources.size(); i > 0; i--) {
        TransientResourceMetal& transient = m_TransientResources[i - 1];
        transient.isInUse = false;

        if (m_ResetIndex - transient.lastUsed > TRANSIENT_MAX_UNUSED_RESETS_METAL) {
            Release(transient.resource);
            transient = m_TransientResources.back();
            m_TransientResources.pop_back();
        }
    }

    m_UploadChunkIndex = 0;
}

MTL::Resource* CommandAllocatorMetal::AcquireTransientResource(const TransientResourceMetal& desc) {
    // A resource can't be reused before "Reset", since the GPU accesses it later. Buffers: the smallest sufficient one, textures: exact match
    TransientResourceMetal* best = nullptr;

    for (TransientResourceMetal& transient : m_TransientResources) {
        if (transient.isInUse || transient.format != desc.format || transient.size < desc.size || (best && transient.size >= best->size))
            continue;

        if (transient.width == desc.width && transient.height == desc.height)
            best = &transient;
    }

    if (best) {
        best->isInUse = true;
        best->lastUsed = m_ResetIndex;

        return best->resource;
    }

    MTL::Resource* resource = nullptr;

    if (desc.format == MTL::PixelFormatInvalid)
        resource = m_Device.GetNativeObject()->newBuffer(desc.size, MTL::ResourceStorageModePrivate | MTL::ResourceHazardTrackingModeUntracked);
    else {
        // Not "texture2DDescriptor", which returns an autoreleased object
        MTL::TextureDescriptor* textureDesc = MTL::TextureDescriptor::alloc()->init();
        textureDesc->setTextureType(MTL::TextureType2D);
        textureDesc->setPixelFormat(desc.format);
        textureDesc->setWidth(desc.width);
        textureDesc->setHeight(desc.height);
        textureDesc->setStorageMode(MTL::StorageModePrivate);
        textureDesc->setHazardTrackingMode(MTL::HazardTrackingModeUntracked);
        textureDesc->setUsage(MTL::TextureUsageRenderTarget);
        resource = m_Device.GetNativeObject()->newTexture(textureDesc);
        textureDesc->release();
    }

    if (!resource)
        return nullptr;

    m_Device.AddResidency(resource);

    TransientResourceMetal& transient = m_TransientResources.emplace_back(desc);
    transient.resource = resource;
    transient.lastUsed = m_ResetIndex;
    transient.isInUse = true;

    return resource;
}

MTL::Texture* CommandAllocatorMetal::CreateTransientTexture(MTL::PixelFormat format, uint32_t width, uint32_t height) {
    TransientResourceMetal desc = {};
    desc.format = format;
    desc.width = width;
    desc.height = height;

    return (MTL::Texture*)AcquireTransientResource(desc);
}

MTL::Buffer* CommandAllocatorMetal::CreateTransientBuffer(uint64_t size) {
    TransientResourceMetal desc = {};
    desc.size = size;

    return (MTL::Buffer*)AcquireTransientResource(desc);
}

MTL::GPUAddress CommandAllocatorMetal::Upload(const void* data, uint64_t size, uint64_t alignment) {
    if (!size)
        return 0;

    // Find a chunk with enough space, starting from the current one (chunks after the current one are empty)
    uint64_t offset = 0;

    for (; m_UploadChunkIndex < m_UploadChunks.size(); m_UploadChunkIndex++) {
        const UploadChunkMetal& chunk = m_UploadChunks[m_UploadChunkIndex];
        offset = Align(chunk.offset, alignment);

        if (offset + size <= chunk.size)
            break;
    }

    if (m_UploadChunkIndex == m_UploadChunks.size()) {
        const uint64_t chunkSize = std::max(UPLOAD_CHUNK_SIZE_METAL, Align(size, 4096));

        MTL::Buffer* buffer = m_Device.GetNativeObject()->newBuffer((NS::UInteger)chunkSize, MTL::ResourceStorageModeShared | MTL::ResourceCPUCacheModeWriteCombined | MTL::ResourceHazardTrackingModeUntracked);

        if (!buffer)
            return 0;

        m_Device.AddResidency(buffer);
        m_UploadChunks.push_back({buffer, (uint8_t*)buffer->contents(), buffer->gpuAddress(), 0, chunkSize, 0});
        offset = 0;
    }

    UploadChunkMetal& chunk = m_UploadChunks[m_UploadChunkIndex];
    chunk.lastUsed = m_ResetIndex;

    if (data)
        memcpy(chunk.data + offset, data, (size_t)size);

    chunk.offset = offset + size;

    return chunk.address + offset;
}

void CommandAllocatorMetal::SetDebugName(const char* name) {
    // "MTL4CommandAllocator::label" is read-only, it can only be set via "MTL4CommandAllocatorDescriptor" at creation
    MaybeUnused(name);
}
