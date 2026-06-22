// © 2021 NVIDIA Corporation

#pragma once

#include "SharedExternal.h"

#define NRI_OBJECT_SIGNATURE 0x1234567887654321ull // TODO: 32-bit platform support? not needed, I believe

#define NRI_GET_IMPL(className, object) (object ? ((className##Val*)object)->GetImpl() : nullptr)

namespace nri {

struct AccelerationStructureVal;
struct BufferVal;
struct CommandAllocatorVal;
struct CommandBufferVal;
struct DescriptorPoolVal;
struct DescriptorSetVal;
struct DescriptorVal;
struct DeviceVal;
struct FenceVal;
struct MemoryVal;
struct MicromapVal;
struct PipelineCacheVal;
struct PipelineLayoutVal;
struct PipelineVal;
struct QueryPoolVal;
struct QueueVal;
struct SwapChainVal;
struct TextureVal;

uint64_t GetMemorySizeD3D12(const MemoryD3D12Desc& memoryD3D12Desc);
QueryType GetQueryTypeVK(uint32_t queryTypeVK);
const char* GetDescriptorTypeName(DescriptorType descriptorType);
void ConvertBotomLevelGeometries(const BottomLevelGeometryDesc* geometries, uint32_t geometryNum, BottomLevelGeometryDesc*& outGeometries, BottomLevelMicromapDesc*& outMicromaps);

} // namespace nri

#include "DeviceVal.h"

namespace nri {

struct ObjectVal : public DebugNameBaseVal {
    inline ObjectVal(DeviceVal& device, Object* object = nullptr)
        : m_Impl(object)
        , m_Device(device) {
    }

    inline ~ObjectVal() {
        if (m_Name) {
            const auto& allocationCallbacks = m_Device.GetAllocationCallbacks();
            allocationCallbacks.Free(allocationCallbacks.userArg, m_Name);
        }
    }

    inline const char* GetDebugName() const {
        return m_Name ? m_Name : "unnamed";
    }

    inline DeviceVal& GetDevice() const {
        return m_Device;
    }

    inline const CoreInterface& GetCoreInterfaceImpl() const {
        return m_Device.GetCoreInterfaceImpl();
    }

    inline const HelperInterface& GetHelperInterfaceImpl() const {
        return m_Device.GetHelperInterfaceImpl();
    }

    inline const LowLatencyInterface& GetLowLatencyInterfaceImpl() const {
        return m_Device.GetLowLatencyInterfaceImpl();
    }

    inline const MeshShaderInterface& GetMeshShaderInterfaceImpl() const {
        return m_Device.GetMeshShaderInterfaceImpl();
    }

    inline const RayTracingInterface& GetRayTracingInterfaceImpl() const {
        return m_Device.GetRayTracingInterfaceImpl();
    }

    inline const SwapChainInterface& GetSwapChainInterfaceImpl() const {
        return m_Device.GetSwapChainInterfaceImpl();
    }

    inline const WrapperD3D11Interface& GetWrapperD3D11InterfaceImpl() const {
        return m_Device.GetWrapperD3D11InterfaceImpl();
    }

    inline const WrapperD3D12Interface& GetWrapperD3D12InterfaceImpl() const {
        return m_Device.GetWrapperD3D12InterfaceImpl();
    }

    inline const WrapperVKInterface& GetWrapperVKInterfaceImpl() const {
        return m_Device.GetWrapperVKInterfaceImpl();
    }

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) override {
        const auto& allocationCallbacks = m_Device.GetAllocationCallbacks();
        if (m_Name)
            allocationCallbacks.Free(allocationCallbacks.userArg, m_Name);

        size_t len = strlen(name);
        m_Name = (char*)allocationCallbacks.Allocate(allocationCallbacks.userArg, len + 1, sizeof(size_t));
        strcpy(m_Name, name);

        m_Device.GetCoreInterfaceImpl().SetDebugName(m_Impl, name);
    }

protected:
#ifndef NDEBUG
    uint64_t m_Signature = NRI_OBJECT_SIGNATURE; // .natvis
#endif
    char* m_Name = nullptr; // .natvis
    Object* m_Impl = nullptr;
    DeviceVal& m_Device;
};

template <typename T>
inline DeviceVal& GetDeviceVal(T& object) {
    return ((ObjectVal&)object).GetDevice();
}

} // namespace nri
