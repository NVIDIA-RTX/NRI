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
    "SAMPLER",                   // SAMPLER,
    "CONSTANT_BUFFER",           // CONSTANT_BUFFER,
    "TEXTURE",                   // TEXTURE,
    "STORAGE_TEXTURE",           // STORAGE_TEXTURE,
    "BUFFER",                    // BUFFER,
    "STORAGE_BUFFER",            // STORAGE_BUFFER,
    "STRUCTURED_BUFFER",         // STRUCTURED_BUFFER,
    "STORAGE_STRUCTURED_BUFFER", // STORAGE_STRUCTURED_BUFFER,
    "ACCELERATION_STRUCTURE",    // ACCELERATION_STRUCTURE
};
VALIDATE_ARRAY_BY_PTR(g_descriptorTypeNames);

constexpr const char* GetDescriptorTypeName(DescriptorType descriptorType) {
    return g_descriptorTypeNames[(uint32_t)descriptorType];
}

constexpr bool IsAccessMaskSupported(BufferUsageBits usage, AccessBits accessMask) {
    bool isSupported = true;
    if (accessMask & AccessBits::INDEX_BUFFER)
        isSupported = isSupported && (usage & BufferUsageBits::INDEX_BUFFER) != 0;
    if (accessMask & AccessBits::VERTEX_BUFFER)
        isSupported = isSupported && (usage & BufferUsageBits::VERTEX_BUFFER) != 0;
    if (accessMask & AccessBits::CONSTANT_BUFFER)
        isSupported = isSupported && (usage & BufferUsageBits::CONSTANT_BUFFER) != 0;
    if (accessMask & AccessBits::ARGUMENT_BUFFER)
        isSupported = isSupported && (usage & BufferUsageBits::ARGUMENT_BUFFER) != 0;
    if (accessMask & AccessBits::SCRATCH_BUFFER)
        isSupported = isSupported && (usage & BufferUsageBits::SCRATCH_BUFFER) != 0;
    if (accessMask & (AccessBits::COLOR_ATTACHMENT | AccessBits::SHADING_RATE_ATTACHMENT | AccessBits::DEPTH_STENCIL_ATTACHMENT_READ | AccessBits::DEPTH_STENCIL_ATTACHMENT_WRITE))
        isSupported = false;
    if (accessMask & (AccessBits::ACCELERATION_STRUCTURE_READ | AccessBits::ACCELERATION_STRUCTURE_WRITE))
        isSupported = isSupported && (usage & BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE) != 0;
    if (accessMask & (AccessBits::MICROMAP_READ | AccessBits::MICROMAP_WRITE))
        isSupported = isSupported && (usage & BufferUsageBits::MICROMAP_STORAGE) != 0;
    if (accessMask & AccessBits::SHADER_BINDING_TABLE)
        isSupported = isSupported && (usage & BufferUsageBits::SHADER_BINDING_TABLE) != 0;
    if (accessMask & AccessBits::SHADER_RESOURCE)
        isSupported = isSupported && (usage & (BufferUsageBits::SHADER_RESOURCE | BufferUsageBits::ACCELERATION_STRUCTURE_BUILD_INPUT)) != 0;
    if (accessMask & AccessBits::SHADER_RESOURCE_STORAGE)
        isSupported = isSupported && (usage & BufferUsageBits::SHADER_RESOURCE_STORAGE) != 0;
    if (accessMask & (AccessBits::RESOLVE_SOURCE | AccessBits::RESOLVE_DESTINATION))
        isSupported = false;

    return isSupported;
}

constexpr bool IsAccessMaskSupported(TextureUsageBits usage, AccessBits accessMask) {
    bool isSupported = true;
    if (accessMask & (AccessBits::INDEX_BUFFER | AccessBits::VERTEX_BUFFER | AccessBits::CONSTANT_BUFFER | AccessBits::ARGUMENT_BUFFER | AccessBits::SCRATCH_BUFFER))
        isSupported = false;
    if (accessMask & AccessBits::COLOR_ATTACHMENT)
        isSupported = isSupported && (usage & TextureUsageBits::COLOR_ATTACHMENT) != 0;
    if (accessMask & AccessBits::SHADING_RATE_ATTACHMENT)
        isSupported = isSupported && (usage & TextureUsageBits::SHADING_RATE_ATTACHMENT) != 0;
    if (accessMask & (AccessBits::DEPTH_STENCIL_ATTACHMENT_READ | AccessBits::DEPTH_STENCIL_ATTACHMENT_WRITE))
        isSupported = isSupported && (usage & TextureUsageBits::DEPTH_STENCIL_ATTACHMENT) != 0;
    if (accessMask & (AccessBits::ACCELERATION_STRUCTURE_READ | AccessBits::ACCELERATION_STRUCTURE_WRITE))
        isSupported = false;
    if (accessMask & (AccessBits::MICROMAP_READ | AccessBits::MICROMAP_WRITE))
        isSupported = false;
    if (accessMask & AccessBits::SHADER_BINDING_TABLE)
        isSupported = false;
    if (accessMask & AccessBits::SHADER_RESOURCE)
        isSupported = isSupported && (usage & TextureUsageBits::SHADER_RESOURCE) != 0;
    if (accessMask & (AccessBits::SHADER_RESOURCE_STORAGE | AccessBits::CLEAR_STORAGE))
        isSupported = isSupported && (usage & TextureUsageBits::SHADER_RESOURCE_STORAGE) != 0;

    return isSupported;
}

constexpr bool IsTextureLayoutSupported(TextureUsageBits usage, Layout layout) {
    if (layout == Layout::COLOR_ATTACHMENT)
        return (usage & TextureUsageBits::COLOR_ATTACHMENT) != 0;
    if (layout == Layout::SHADING_RATE_ATTACHMENT)
        return (usage & TextureUsageBits::SHADING_RATE_ATTACHMENT) != 0;
    if (layout == Layout::DEPTH_STENCIL_ATTACHMENT || layout == Layout::DEPTH_STENCIL_READONLY)
        return (usage & TextureUsageBits::DEPTH_STENCIL_ATTACHMENT) != 0;
    if (layout == Layout::SHADER_RESOURCE)
        return (usage & TextureUsageBits::SHADER_RESOURCE) != 0;
    if (layout == Layout::SHADER_RESOURCE_STORAGE)
        return (usage & TextureUsageBits::SHADER_RESOURCE_STORAGE) != 0;

    return true;
}

void ConvertBotomLevelGeometries(const BottomLevelGeometryDesc* geometries, uint32_t geometryNum, BottomLevelGeometryDesc*& outGeometries, BottomLevelMicromapDesc*& outMicromaps);

QueryType GetQueryTypeVK(uint32_t queryTypeVK);

} // namespace nri
