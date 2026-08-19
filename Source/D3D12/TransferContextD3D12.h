// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct TransferContextD3D12 {
    inline TransferContextD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    ~TransferContextD3D12();

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline D3D12_COMMAND_LIST_TYPE GetType() const {
        return m_CommandListType;
    }

    inline bool IsInUse() const {
        return m_IsInUse;
    }

    inline void SetInUse(bool isInUse) {
        m_IsInUse = isInUse;
    }

    inline CommandBufferD3D12& GetCommandBuffer() const {
        return *m_CommandBuffer;
    }

    inline BufferD3D12& GetUploadBuffer() const {
        return *m_UploadBuffer;
    }

    inline BufferD3D12& GetReadbackBuffer() const {
        return *m_ReadbackBuffer;
    }

    Result Prepare(QueueD3D12& queue);
    Result EnsureUploadBuffer(uint64_t size);
    Result EnsureReadbackBuffer(uint64_t size);
    Result SubmitAndWait(QueueD3D12& queue);

private:
    Result EnsureBuffer(MemoryLocation memoryLocation, uint64_t size, BufferD3D12*& buffer, uint64_t& capacity);

private:
    DeviceD3D12& m_Device;
    CommandBufferD3D12* m_CommandBuffer = nullptr;
    FenceD3D12* m_Fence = nullptr;
    CommandAllocatorD3D12* m_CommandAllocator = nullptr;
    BufferD3D12* m_UploadBuffer = nullptr;
    BufferD3D12* m_ReadbackBuffer = nullptr;
    uint64_t m_UploadBufferSize = 0;
    uint64_t m_ReadbackBufferSize = 0;
    uint64_t m_FenceValue = 1;
    D3D12_COMMAND_LIST_TYPE m_CommandListType = D3D12_COMMAND_LIST_TYPE(-1);
    bool m_IsInUse = false;
};

} // namespace nri
