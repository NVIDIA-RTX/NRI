// © 2021 NVIDIA Corporation

#pragma once

#include "SharedExternal.h"

#include "DeviceVal.h"

#define NRI_OBJECT_SIGNATURE 0x1234567887654321ull // TODO: 32-bit platform support? not needed, I believe

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

#define NRI_GET_IMPL(className, object) (object ? ((className##Val*)object)->GetImpl() : nullptr)

template <typename T>
inline DeviceVal& GetDeviceVal(T& object) {
    return ((ObjectVal&)object).GetDevice();
}

uint64_t GetMemorySizeD3D12(const MemoryD3D12Desc& memoryD3D12Desc);

constexpr std::array<const char*, (size_t)DescriptorType::MAX_NUM> g_descriptorTypeNames = {
    "MUTABLE",                   // MUTABLE
    "SAMPLER",                   // SAMPLER
    "CONSTANT_BUFFER",           // CONSTANT_BUFFER
    "TEXTURE",                   // TEXTURE
    "STORAGE_TEXTURE",           // STORAGE_TEXTURE
    "BUFFER",                    // BUFFER
    "STORAGE_BUFFER",            // STORAGE_BUFFER
    "STRUCTURED_BUFFER",         // STRUCTURED_BUFFER
    "STORAGE_STRUCTURED_BUFFER", // STORAGE_STRUCTURED_BUFFER
    "ACCELERATION_STRUCTURE",    // ACCELERATION_STRUCTURE
};
VALIDATE_ARRAY_BY_PTR(g_descriptorTypeNames);

constexpr const char* GetDescriptorTypeName(DescriptorType descriptorType) {
    return g_descriptorTypeNames[(uint32_t)descriptorType];
}

void ConvertBotomLevelGeometries(const BottomLevelGeometryDesc* geometries, uint32_t geometryNum, BottomLevelGeometryDesc*& outGeometries, BottomLevelMicromapDesc*& outMicromaps);
QueryType GetQueryTypeVK(uint32_t queryTypeVK);

} // namespace nri
