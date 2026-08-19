// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct TransferContextVK {
    inline TransferContextVK(DeviceVK& device)
        : m_Device(device) {
    }

    ~TransferContextVK();

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline uint32_t GetFamilyIndex() const {
        return m_FamilyIndex;
    }

    inline bool IsInUse() const {
        return m_IsInUse;
    }

    inline void SetInUse(bool isInUse) {
        m_IsInUse = isInUse;
    }

    inline CommandBufferVK& GetCommandBuffer() const {
        return *m_CommandBuffer;
    }

    inline BufferVK& GetUploadBuffer() const {
        return *m_UploadBuffer;
    }

    inline BufferVK& GetReadbackBuffer() const {
        return *m_ReadbackBuffer;
    }

    Result Prepare(QueueVK& queue);
    Result EnsureUploadBuffer(uint64_t size);
    Result EnsureReadbackBuffer(uint64_t size);
    Result SubmitAndWait(QueueVK& queue);
    bool TryRecover();

private:
    Result EnsureBuffer(MemoryLocation memoryLocation, uint64_t size, BufferVK*& buffer, uint64_t& capacity);
    void Reset();

private:
    DeviceVK& m_Device;
    CommandBufferVK* m_CommandBuffer = nullptr;
    FenceVK* m_Fence = nullptr;
    CommandAllocatorVK* m_CommandAllocator = nullptr;
    BufferVK* m_UploadBuffer = nullptr;
    BufferVK* m_ReadbackBuffer = nullptr;
    uint64_t m_UploadBufferSize = 0;
    uint64_t m_ReadbackBufferSize = 0;
    uint64_t m_FenceValue = 1;
    uint32_t m_FamilyIndex = INVALID_FAMILY_INDEX;
    bool m_IsInUse = false;
    bool m_IsReusable = true;
};

} // namespace nri
