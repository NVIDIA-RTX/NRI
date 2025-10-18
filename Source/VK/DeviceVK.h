// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct QueueVK;

struct IsSupported {
    uint32_t deviceAddress          : 1;
    uint32_t swapChainMutableFormat : 1;
    uint32_t presentId              : 1;
    uint32_t memoryPriority         : 1;
    uint32_t memoryBudget           : 1;
    uint32_t maintenance4           : 1;
    uint32_t maintenance5           : 1;
    uint32_t maintenance6           : 1;
    uint32_t imageSlicedView        : 1;
    uint32_t customBorderColor      : 1;
    uint32_t robustness             : 1;
    uint32_t robustness2            : 1;
    uint32_t pipelineRobustness     : 1;
    uint32_t swapChainMaintenance1  : 1;
    uint32_t fifoLatestReady        : 1;
};

static_assert(sizeof(IsSupported) == sizeof(uint32_t), "4 bytes expected");

struct DeviceVK final : public DeviceBase {
    inline operator VkDevice() const {
        return m_Device;
    }

    inline operator VkPhysicalDevice() const {
        return m_PhysicalDevice;
    }

    inline operator VkInstance() const {
        return m_Instance;
    }

    inline const DispatchTable& GetDispatchTable() const {
        return m_VK;
    }

    inline const VkAllocationCallbacks* GetVkAllocationCallbacks() const {
        return m_AllocationCallbackPtr;
    }

    inline const VKBindingOffsets& GetBindingOffsets() const {
        return m_BindingOffsets;
    }

    inline const CoreInterface& GetCoreInterface() const {
        return m_iCore;
    }

    inline bool IsHostCoherentMemory(MemoryTypeIndex memoryTypeIndex) const {
        return (m_MemoryProps.memoryTypes[memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
    }

    inline bool IsMemoryZeroInitializationEnabled() const {
        return m_IsMemoryZeroInitializationEnabled;
    }

    inline VmaAllocator_T* GetVma() const {
        return m_Vma;
    }

    template <typename Implementation, typename Interface, typename... Args>
    inline Result CreateImplementation(Interface*& entity, const Args&... args) {
        Implementation* impl = Allocate<Implementation>(GetAllocationCallbacks(), *this);
        Result result = impl->Create(args...);

        if (result != Result::SUCCESS) {
            Destroy(GetAllocationCallbacks(), impl);
            entity = nullptr;
        } else
            entity = (Interface*)impl;

        return result;
    }

    DeviceVK(const CallbackInterface& callbacks, const AllocationCallbacks& allocationCallbacks);
    ~DeviceVK();

    Result Create(const DeviceCreationDesc& desc, const DeviceCreationVKDesc& descVK);
    void FillCreateInfo(const BufferDesc& bufferDesc, VkBufferCreateInfo& info) const;
    void FillCreateInfo(const TextureDesc& bufferDesc, VkImageCreateInfo& info) const;
    void FillCreateInfo(const SamplerDesc& samplerDesc, VkSamplerCreateInfo& info, VkSamplerReductionModeCreateInfo& reductionModeInfo, VkSamplerCustomBorderColorCreateInfoEXT& borderColorInfo) const;
    void GetMemoryDesc2(const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    void GetMemoryDesc2(const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    void GetMemoryDesc2(const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc);
    void GetMemoryDesc2(const MicromapDesc& micromapDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc);
    bool GetMemoryDesc(MemoryLocation memoryLocation, const VkMemoryRequirements& memoryRequirements, const VkMemoryDedicatedRequirements& memoryDedicatedRequirements, MemoryDesc& memoryDesc) const;
    bool GetMemoryTypeByIndex(uint32_t index, MemoryTypeInfo& memoryTypeInfo) const;
    void GetAccelerationStructureBuildSizesInfo(const AccelerationStructureDesc& accelerationStructureDesc, VkAccelerationStructureBuildSizesInfoKHR& sizesInfo);
    void GetMicromapBuildSizesInfo(const MicromapDesc& micromapDesc, VkMicromapBuildSizesInfoEXT& sizesInfo);
    void SetDebugNameToTrivialObject(VkObjectType objectType, uint64_t handle, const char* name);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) DEBUG_NAME_OVERRIDE;

    //================================================================================================================
    // DeviceBase
    //================================================================================================================

    inline const DeviceDesc& GetDesc() const override {
        return m_Desc;
    }

    void Destruct() override;
    Result FillFunctionTable(CoreInterface& table) const override;
    Result FillFunctionTable(HelperInterface& table) const override;
    Result FillFunctionTable(LowLatencyInterface& table) const override;
    Result FillFunctionTable(MeshShaderInterface& table) const override;
    Result FillFunctionTable(RayTracingInterface& table) const override;
    Result FillFunctionTable(StreamerInterface& table) const override;
    Result FillFunctionTable(SwapChainInterface& table) const override;
    Result FillFunctionTable(UpscalerInterface& table) const override;
    Result FillFunctionTable(WrapperVKInterface& table) const override;

#if NRI_ENABLE_IMGUI_EXTENSION
    Result FillFunctionTable(ImguiInterface& table) const override;
#endif

    //================================================================================================================
    // NRI
    //================================================================================================================

    void CopyDescriptorRanges(const CopyDescriptorRangeDesc* copyDescriptorRangeDescs, uint32_t copyDescriptorRangeDescNum);
    void UpdateDescriptorRanges(const UpdateDescriptorRangeDesc* updateDescriptorRangeDescs, uint32_t updateDescriptorRangeDescNum);
    Result GetQueue(QueueType queueType, uint32_t queueIndex, Queue*& queue);
    Result WaitIdle();
    Result BindBufferMemory(const BindBufferMemoryDesc* bindBufferMemoryDescs, uint32_t bindBufferMemoryDescNum);
    Result BindTextureMemory(const BindTextureMemoryDesc* bindTextureMemoryDescs, uint32_t bindTextureMemoryDescNum);
    Result QueryVideoMemoryInfo(MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) const;
    Result BindAccelerationStructureMemory(const BindAccelerationStructureMemoryDesc* bindAccelerationStructureMemoryDescs, uint32_t bindAccelerationStructureMemoryDescNum);
    Result BindMicromapMemory(const BindMicromapMemoryDesc* bindMicromapMemoryDescs, uint32_t bindMicromapMemoryDescNum);
    FormatSupportBits GetFormatSupport(Format format) const;

private:
    VkResult CreateVma();
    void FilterInstanceLayers(Vector<const char*>& layers);
    void ProcessInstanceExtensions(Vector<const char*>& desiredInstanceExts);
    void ProcessDeviceExtensions(Vector<const char*>& desiredDeviceExts, bool disableRayTracing);
    void ReportMemoryTypes();
    Result CreateInstance(bool enableGraphicsAPIValidation, const Vector<const char*>& desiredInstanceExts);
    Result ResolvePreInstanceDispatchTable();
    Result ResolveInstanceDispatchTable(const Vector<const char*>& desiredInstanceExts);
    Result ResolveDispatchTable(const Vector<const char*>& desiredDeviceExts);

public:
    union {
        uint32_t m_IsSupportedStorage = 0;
        IsSupported m_IsSupported;
    };

private:
    VkPhysicalDevice m_PhysicalDevice = nullptr;
    std::array<uint32_t, (size_t)QueueType::MAX_NUM> m_ActiveQueueFamilyIndices = {};
    std::array<Vector<QueueVK*>, (size_t)QueueType::MAX_NUM> m_QueueFamilies;
    DispatchTable m_VK = {};
    VkPhysicalDeviceMemoryProperties m_MemoryProps = {};
    VkAllocationCallbacks m_AllocationCallbacks = {};
    VKBindingOffsets m_BindingOffsets = {};
    CoreInterface m_iCore = {};
    DeviceDesc m_Desc = {};
    Library* m_Loader = nullptr;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkAllocationCallbacks* m_AllocationCallbackPtr = nullptr;
    VkDebugUtilsMessengerEXT m_Messenger = VK_NULL_HANDLE;
    VmaAllocator_T* m_Vma = nullptr;
    uint32_t m_NumActiveFamilyIndices = 0;
    uint32_t m_MinorVersion = 0;
    bool m_OwnsNativeObjects = true;
    bool m_IsMemoryZeroInitializationEnabled = false;

    Lock m_Lock;
};

} // namespace nri
