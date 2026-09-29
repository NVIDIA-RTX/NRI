// © 2026 NVIDIA Corporation

#pragma once

#include <atomic>
#include <chrono>
#include <thread>

namespace nri {

struct QueueMetal final : public DebugNameBase {
    inline QueueMetal(DeviceMetal& device)
        : m_Device(device) {
    }

    ~QueueMetal();

    inline DeviceMetal& GetDevice() const {
        return m_Device;
    }

    inline MTL4::CommandQueue* GetNativeObject() const {
        return m_Queue;
    }

    Result Create();
    Result Create(MTL4::CommandQueue* queue);
    Result Submit(const QueueSubmitDesc& queueSubmitDesc);
    Result WaitIdle();
    void WaitForDrawable(CA::MetalDrawable* drawable, FenceMetal& acquireFence);
    void SignalDrawable(CA::MetalDrawable* drawable, FenceMetal& releaseFence);
    void GetCalibratedTimestamps(uint64_t& timestampGPU, uint64_t& timestampCPU);
    Result UploadHostMemoryToTexture(const UploadHostMemoryToTextureDesc* copyDescs, uint32_t copyDescNum);
    Result ReadbackTextureToHostMemory(const ReadbackTextureToHostMemoryDesc* copyDescs, uint32_t copyDescNum);
    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

private:
    Result Finalize();
    Result Commit(const MTL4::CommandBuffer* const* commandBuffers, uint32_t commandBufferNum); // under "m_Lock"
    uint64_t SignalIdle(uint64_t& commitNum);                                                   // under "m_Lock"
    Result WaitForSignal(uint64_t value, uint64_t commitNum);
    Result BeginTransfer(uint64_t stagingSize, MTL4::ComputeCommandEncoder*& encoder);
    Result EndTransfer();
    void TrimTransfer();

    DeviceMetal& m_Device;
    MTL4::CommandQueue* m_Queue = nullptr;
    MTL::SharedEvent* m_IdleEvent = nullptr;
    std::atomic<NS::Error*> m_Error = nullptr;   // the first commit error, set by feedback handlers
    dispatch_group_t m_PendingCommits = nullptr; // commits with feedback handlers not called yet (handlers access "this")
    std::atomic_uint64_t m_FeedbackNum = 0;      // called feedback handlers

    // Submissions are atomic: waits, commits and signals of different threads don't interleave (also keeps "m_IdleEvent" values monotonic)
    std::mutex m_Lock;
    uint64_t m_IdleValue = 0;
    uint64_t m_CommitNum = 0;

    // "UploadHostMemoryToTexture" and "ReadbackTextureToHostMemory" (reused, the staging buffer only grows)
    std::mutex m_TransferLock;
    MTL4::CommandAllocator* m_TransferAllocator = nullptr;
    MTL4::CommandBuffer* m_TransferCommandBuffer = nullptr;
    MTL::Buffer* m_TransferBuffer = nullptr;
    uint64_t m_TransferValue = 0; // "m_IdleEvent" value signaled after the last transfer
};

} // namespace nri
