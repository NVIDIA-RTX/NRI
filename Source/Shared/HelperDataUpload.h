// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

constexpr size_t BASE_UPLOAD_BUFFER_SIZE = 1 * 1024 * 1024;

struct HelperDataUpload {
    inline HelperDataUpload(const CoreInterface& NRI, Device& device, Queue& queue)
        : m_NRI(NRI)
        , m_Device(device)
        , m_Queue(queue)
        , m_UploadBufferSize(BASE_UPLOAD_BUFFER_SIZE) {
    }

    Result UploadData(const TextureUploadDesc* textureDataDescs, uint32_t textureDataDescNum, const BufferUploadDesc* bufferDataDescs, uint32_t bufferDataDescNum);

private:
    Result Create();
    Result UploadTextures(const TextureUploadDesc* textureDataDescs, uint32_t textureDataDescNum);
    Result UploadBuffers(const BufferUploadDesc* bufferDataDescs, uint32_t bufferDataDescNum);
    Result EndCommandBuffersAndSubmit();
    bool CopyTextureContent(const TextureUploadDesc& textureDataDesc, Dim_t& layerOffset, Mip_t& mipOffset, bool& isCapacityInsufficient);
    void CopyTextureSubresourceContent(const TextureSubresourceUploadDesc& subresource, uint64_t alignedRowPitch, uint64_t alignedSlicePitch);
    bool CopyBufferContent(const BufferUploadDesc& bufferDataDesc, uint64_t& bufferContentOffset);

    const CoreInterface& m_NRI;
    Device& m_Device;
    Queue& m_Queue;
    CommandBuffer* m_CommandBuffer = nullptr;
    Fence* m_Fence = nullptr;
    CommandAllocator* m_CommandAllocators = nullptr;
    Buffer* m_UploadBuffer = nullptr;
    Memory* m_UploadBufferMemory = nullptr;
    uint8_t* m_MappedMemory = nullptr;
    uint64_t m_UploadBufferSize = 0;
    uint64_t m_UploadBufferOffset = 0;
    uint64_t m_FenceValue = 1;
};

} // namespace nri