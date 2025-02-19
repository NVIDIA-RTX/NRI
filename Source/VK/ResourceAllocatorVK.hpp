// © 2021 NVIDIA Corporation

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-variable"
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunused-variable"
#    pragma clang diagnostic ignored "-Wunused-parameter"
#else
#    pragma warning(push)
#    pragma warning(disable : 4100) // unreferenced formal parameter
#    pragma warning(disable : 4189) // local variable is initialized but not referenced
#    pragma warning(disable : 4127) // conditional expression is constant
#endif

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION

#include "vk_mem_alloc.h"

#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(__clang__)
#    pragma clang diagnostic pop
#else
#    pragma warning(pop)
#endif


Result DeviceVK::CreateVma() {
    if (m_Vma)
        return Result::SUCCESS;

    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = m_VK.GetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = m_VK.GetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.vulkanApiVersion = m_MinorVersion >= 3 ? VK_API_VERSION_1_3 : VK_API_VERSION_1_2;
    allocatorCreateInfo.physicalDevice = m_PhysicalDevice;
    allocatorCreateInfo.device = m_Device;
    allocatorCreateInfo.instance = m_Instance;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
    allocatorCreateInfo.pAllocationCallbacks = m_AllocationCallbackPtr;
    allocatorCreateInfo.preferredLargeHeapBlockSize = VMA_PREFERRED_BLOCK_SIZE;

    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT;
    if (m_IsSupported.memoryBudget)
        allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    if (m_IsSupported.deviceAddress)
        allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    if (m_IsSupported.memoryPriority)
        allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
    if (m_IsSupported.maintenance5)
        allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT;

    VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &m_Vma);
    RETURN_ON_FAILURE(this, result == VK_SUCCESS, GetReturnCode(result), "vmaCreateAllocator returned %d", (int32_t)result);

    return Result::SUCCESS;
}

Result BufferVK::Create(const AllocateBufferDesc& bufferDesc) {
    Result nriResult = m_Device.CreateVma();
    if (nriResult != Result::SUCCESS)
        return nriResult;

    // Fill info
    VkBufferCreateInfo bufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    m_Device.FillCreateInfo(bufferDesc.desc, bufferCreateInfo);

    // Create
    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT | VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
    allocationCreateInfo.priority = bufferDesc.memoryPriority * 0.5f + 0.5f;
    allocationCreateInfo.usage = IsHostMemory(bufferDesc.memoryLocation) ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    if (bufferDesc.dedicated)
        allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    if (IsHostVisibleMemory(bufferDesc.memoryLocation)) {
        allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

        if (bufferDesc.memoryLocation == MemoryLocation::HOST_READBACK)
            allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        else
            allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    uint32_t alignment = 1;
    if (bufferDesc.desc.usage & (BufferUsageBits::SHADER_RESOURCE | BufferUsageBits::SHADER_RESOURCE_STORAGE))
        alignment = std::max(alignment, deviceDesc.bufferShaderResourceOffsetAlignment);
    if (bufferDesc.desc.usage & BufferUsageBits::CONSTANT_BUFFER)
        alignment = std::max(alignment, deviceDesc.constantBufferOffsetAlignment);
    if (bufferDesc.desc.usage & BufferUsageBits::SHADER_BINDING_TABLE)
        alignment = std::max(alignment, deviceDesc.shaderBindingTableAlignment);
    if (bufferDesc.desc.usage & BufferUsageBits::SCRATCH_BUFFER)
        alignment = std::max(alignment, deviceDesc.scratchBufferOffsetAlignment);

    VmaAllocationInfo allocationInfo = {};
    VkResult result = vmaCreateBufferWithAlignment(m_Device.GetVma(), &bufferCreateInfo, &allocationCreateInfo, alignment, &m_Handle, &m_VmaAllocation, &allocationInfo);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vmaCreateBufferWithAlignment returned %d", (int32_t)result);

    // Mapped memory
    if (IsHostVisibleMemory(bufferDesc.memoryLocation)) {
        m_MappedMemory = (uint8_t*)allocationInfo.pMappedData - allocationInfo.offset;
        m_MappedMemoryOffset = allocationInfo.offset;

        uint32_t memoryTypeIndex = 0;
        result = vmaFindMemoryTypeIndexForBufferInfo(m_Device.GetVma(), &bufferCreateInfo, &allocationCreateInfo, &memoryTypeIndex);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vmaFindMemoryTypeIndexForBufferInfo returned %d", (int32_t)result);

        if (!m_Device.IsHostCoherentMemory((MemoryTypeIndex)memoryTypeIndex))
            m_NonCoherentDeviceMemory = allocationInfo.deviceMemory;
    }

    // Device address
    if (m_Device.m_IsSupported.deviceAddress) {
        VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        bufferDeviceAddressInfo.buffer = m_Handle;

        const auto& vk = m_Device.GetDispatchTable();
        m_DeviceAddress = vk.GetBufferDeviceAddress(m_Device, &bufferDeviceAddressInfo);
    }

    m_Desc = bufferDesc.desc;

    return Result::SUCCESS;
}

Result TextureVK::Create(const AllocateTextureDesc& textureDesc) {
    Result nriResult = m_Device.CreateVma();
    if (nriResult != Result::SUCCESS)
        return nriResult;

    // Fill info
    VkImageCreateInfo imageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    m_Device.FillCreateInfo(textureDesc.desc, imageCreateInfo);

    // Create
    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT | VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
    allocationCreateInfo.priority = textureDesc.memoryPriority * 0.5f + 0.5f;
    allocationCreateInfo.usage = IsHostMemory(textureDesc.memoryLocation) ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    if (textureDesc.dedicated)
        allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    VkResult result = vmaCreateImage(m_Device.GetVma(), &imageCreateInfo, &allocationCreateInfo, &m_Handle, &m_VmaAllocation, nullptr);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vmaCreateImage returned %d", (int32_t)result);

    m_Desc = textureDesc.desc;

    return Result::SUCCESS;
}

Result AccelerationStructureVK::Create(const AllocateAccelerationStructureDesc& accelerationStructureDesc) {
    Result nriResult = m_Device.CreateVma();
    if (nriResult != Result::SUCCESS)
        return nriResult;

    VkAccelerationStructureBuildSizesInfoKHR sizesInfo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    m_Device.GetAccelerationStructureBuildSizesInfo(accelerationStructureDesc.desc, sizesInfo);

    AllocateBufferDesc bufferDesc = {};
    bufferDesc.memoryLocation = accelerationStructureDesc.memoryLocation;
    bufferDesc.memoryPriority = accelerationStructureDesc.memoryPriority;
    bufferDesc.desc.size = sizesInfo.accelerationStructureSize;
    bufferDesc.desc.usage = BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE;

    Buffer* buffer = nullptr;
    Result result = m_Device.CreateImplementation<BufferVK>(buffer, bufferDesc);
    if (result == Result::SUCCESS) {
        m_Buffer = (BufferVK*)buffer;
        m_BuildScratchSize = sizesInfo.buildScratchSize;
        m_UpdateScratchSize = sizesInfo.updateScratchSize;
        m_Type = GetAccelerationStructureType(accelerationStructureDesc.desc.type);
        m_AccelerationStructureSize = sizesInfo.accelerationStructureSize;

        FinishCreation();
    }

    return result;
}

void DeviceVK::DestroyVma() {
    if (m_Vma)
        vmaDestroyAllocator(m_Vma);
}

void BufferVK::DestroyVma() {
    CHECK(m_VmaAllocation, "Not a VMA allocation");
    vmaDestroyBuffer(m_Device.GetVma(), m_Handle, m_VmaAllocation);
}

void TextureVK::DestroyVma() {
    CHECK(m_VmaAllocation, "Not a VMA allocation");
    vmaDestroyImage(m_Device.GetVma(), m_Handle, m_VmaAllocation);
}
