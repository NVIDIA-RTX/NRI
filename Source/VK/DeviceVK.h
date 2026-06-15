// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct QueueVK;

struct RenderPassAttachmentDesc {
    bool operator==(const RenderPassAttachmentDesc& other) const {
        return format == other.format
            && sampleNum == other.sampleNum
            && loadOp == other.loadOp
            && storeOp == other.storeOp
            && stencilLoadOp == other.stencilLoadOp
            && stencilStoreOp == other.stencilStoreOp
            && layout == other.layout;
    }

    VkFormat format = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits sampleNum = VK_SAMPLE_COUNT_1_BIT;
    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
};

struct RenderPassDesc {
    RenderPassDesc(const StdAllocator<uint8_t>& allocator)
        : colors(allocator)
        , colorResolves(allocator)
        , inputAttachmentIndices(allocator) {
    }

    bool operator==(const RenderPassDesc& other) const {
        return viewMask == other.viewMask
            && hasDepth == other.hasDepth
            && hasStencil == other.hasStencil
            && hasDepthResolve == other.hasDepthResolve
            && hasStencilResolve == other.hasStencilResolve
            && hasShadingRate == other.hasShadingRate
            && depthResolveMode == other.depthResolveMode
            && stencilResolveMode == other.stencilResolveMode
            && colors == other.colors
            && colorResolves == other.colorResolves
            && inputAttachmentIndices == other.inputAttachmentIndices
            && depth == other.depth
            && stencil == other.stencil
            && depthResolve == other.depthResolve
            && stencilResolve == other.stencilResolve
            && shadingRate == other.shadingRate;
    }

    Vector<RenderPassAttachmentDesc> colors;
    Vector<RenderPassAttachmentDesc> colorResolves;
    Vector<uint32_t> inputAttachmentIndices;
    RenderPassAttachmentDesc depth = {};
    RenderPassAttachmentDesc stencil = {};
    RenderPassAttachmentDesc depthResolve = {};
    RenderPassAttachmentDesc stencilResolve = {};
    RenderPassAttachmentDesc shadingRate = {};
    VkResolveModeFlagBits depthResolveMode = VK_RESOLVE_MODE_NONE;
    VkResolveModeFlagBits stencilResolveMode = VK_RESOLVE_MODE_NONE;
    uint32_t viewMask = 0;
    bool hasDepth = false;
    bool hasStencil = false;
    bool hasDepthResolve = false;
    bool hasStencilResolve = false;
    bool hasShadingRate = false;
};

struct FramebufferDesc {
    FramebufferDesc(const StdAllocator<uint8_t>& allocator)
        : attachments(allocator) {
    }

    bool operator==(const FramebufferDesc& other) const {
        return renderPass == other.renderPass && width == other.width && height == other.height && layerNum == other.layerNum && attachments == other.attachments;
    }

    Vector<VkImageView> attachments;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layerNum = 0;
};

struct RenderPassCacheEntry {
    RenderPassCacheEntry(const StdAllocator<uint8_t>& allocator)
        : desc(allocator) {
    }

    RenderPassDesc desc;
    VkRenderPass handle = VK_NULL_HANDLE;
};

struct FramebufferCacheEntry {
    FramebufferCacheEntry(const StdAllocator<uint8_t>& allocator)
        : desc(allocator) {
    }

    FramebufferDesc desc;
    VkFramebuffer handle = VK_NULL_HANDLE;
};

struct IsSupported {
    uint32_t deviceAddress                : 1;
    uint32_t dynamicRendering             : 1;
    uint32_t swapChainMutableFormat       : 1;
    uint32_t presentId                    : 1;
    uint32_t memoryPriority               : 1;
    uint32_t memoryBudget                 : 1;
    uint32_t maintenance4                 : 1;
    uint32_t maintenance5                 : 1;
    uint32_t maintenance6                 : 1;
    uint32_t maintenance7                 : 1;
    uint32_t maintenance8                 : 1;
    uint32_t maintenance9                 : 1;
    uint32_t maintenance10                : 1;
    uint32_t imageSlicedView              : 1;
    uint32_t customBorderColor            : 1;
    uint32_t robustness                   : 1;
    uint32_t robustness2                  : 1;
    uint32_t pipelineRobustness           : 1;
    uint32_t swapChainMaintenance1        : 1;
    uint32_t fifoLatestReady              : 1;
    uint32_t unifiedImageLayoutsVideo     : 1;
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
    VkRenderPass GetOrCreateRenderPass(const RenderPassDesc& desc);
    VkFramebuffer GetOrCreateFramebuffer(const FramebufferDesc& desc);
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
    void DestroyFramebuffers(VkImageView imageView);

    //================================================================================================================
    // DebugNameBase
    //================================================================================================================

    void SetDebugName(const char* name) NRI_DEBUG_NAME_OVERRIDE;

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
    Vector<RenderPassCacheEntry> m_RenderPasses;
    Vector<FramebufferCacheEntry> m_Framebuffers;
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
