// © 2021 NVIDIA Corporation

#pragma once

struct ID3D12Device;
struct ID3D12CommandQueue;
enum D3D12_COMMAND_LIST_TYPE;

namespace nri {

struct QueueD3D12 final : public DebugNameBase {
    inline QueueD3D12(DeviceD3D12& device)
        : m_Device(device) {
    }

    inline ~QueueD3D12() {
    }

    inline operator ID3D12CommandQueue*() const {
        return m_Queue.GetInterface();
    }

    inline DeviceD3D12& GetDevice() const {
        return m_Device;
    }

    inline D3D12_COMMAND_LIST_TYPE GetType() const {
        return m_CommandListType;
    }

    Result Create(QueueType queueType, float priority);
    Result Create(ID3D12CommandQueue* queue);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE {
        SET_D3D_DEBUG_OBJECT_NAME(m_Queue, name);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    void BeginAnnotation(const char* name, uint32_t bgra);
    void EndAnnotation();
    void Annotation(const char* name, uint32_t bgra);
    Result Submit(const QueueSubmitDesc& queueSubmitDesc);
    Result WaitIdle();

private:
    DeviceD3D12& m_Device;
    ComPtr<ID3D12CommandQueue> m_Queue;
    D3D12_COMMAND_LIST_TYPE m_CommandListType = D3D12_COMMAND_LIST_TYPE(-1);
};

} // namespace nri
