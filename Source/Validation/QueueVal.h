// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct QueueVal final : public ObjectVal {
    inline QueueVal(DeviceVal& device, Queue* queue)
        : ObjectVal(device, queue) {
    }

    inline Queue* GetImpl() const {
        return (Queue*)m_Impl;
    }

    inline void* GetNativeObject() const {
        return m_Device.GetCoreInterfaceImpl().GetQueueNativeObject(GetImpl());
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    void BeginAnnotation(const char* name, uint32_t bgra);
    void EndAnnotation();
    void Annotation(const char* name, uint32_t bgra);
    Result Submit(const QueueSubmitDesc& queueSubmitDesc);
    Result WaitIdle();
};

} // namespace nri
