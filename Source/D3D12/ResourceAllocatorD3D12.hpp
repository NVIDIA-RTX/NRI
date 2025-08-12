// © 2021 NVIDIA Corporation

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wswitch"
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#    pragma GCC diagnostic ignored "-Wunused-variable"
#    pragma GCC diagnostic ignored "-Wsometimes-uninitialized"
#elif defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wswitch"
#    pragma clang diagnostic ignored "-Wunused-parameter"
#    pragma clang diagnostic ignored "-Wunused-variable"
#    pragma clang diagnostic ignored "-Wsometimes-uninitialized"
#else
#    pragma warning(push)
#    pragma warning(disable : 4063) // case 'identifier' is not a valid value for switch of enum 'enumeration'
#    pragma warning(disable : 4100) // unreferenced formal parameter
#    pragma warning(disable : 4189) // local variable is initialized but not referenced
#    pragma warning(disable : 4505) // unreferenced function with internal linkage has been removed
#    pragma warning(disable : 4701) // potentially uninitialized local variable
#endif

#define D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED

#ifndef NDEBUG
#    define D3D12MA_DEBUG_LOG(format, ...) \
        do { \
            wprintf(format, __VA_ARGS__); \
            wprintf(L"\n"); \
        } while (false)
#endif

#include "D3D12MemAlloc.cpp"

#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(__clang__)
#    pragma clang diagnostic pop
#else
#    pragma warning(pop)
#endif

static void* vmaAllocate(size_t size, size_t alignment, void* pPrivateData) {
    const auto& allocationCallbacks = *(AllocationCallbacks*)pPrivateData;

    return allocationCallbacks.Allocate(allocationCallbacks.userArg, size, alignment);
}

static void vmaFree(void* pMemory, void* pPrivateData) {
    const auto& allocationCallbacks = *(AllocationCallbacks*)pPrivateData;

    return allocationCallbacks.Free(allocationCallbacks.userArg, pMemory);
}

HRESULT DeviceD3D12::CreateVma() {
    D3D12MA::ALLOCATION_CALLBACKS allocationCallbacks = {};
    allocationCallbacks.pPrivateData = (void*)&GetAllocationCallbacks();
    allocationCallbacks.pAllocate = vmaAllocate;
    allocationCallbacks.pFree = vmaFree;

    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.pDevice = m_Device;
    allocatorDesc.pAdapter = m_Adapter;
    allocatorDesc.Flags = (D3D12MA::ALLOCATOR_FLAGS)(D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DONT_PREFER_SMALL_BUFFERS_COMMITTED);
    allocatorDesc.PreferredBlockSize = VMA_PREFERRED_BLOCK_SIZE;

    if (!IsMemoryZeroInitializationEnabled())
        allocatorDesc.Flags = (D3D12MA::ALLOCATOR_FLAGS)(allocatorDesc.Flags | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);

    if (!GetAllocationCallbacks().disable3rdPartyAllocationCallbacks)
        allocatorDesc.pAllocationCallbacks = &allocationCallbacks;

    return D3D12MA::CreateAllocator(&allocatorDesc, &m_Vma);
}

Result BufferD3D12::Create(const AllocateBufferDesc& bufferDesc) {
    uint32_t flags = D3D12MA::ALLOCATION_FLAG_CAN_ALIAS | D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY;
    if (bufferDesc.dedicated)
        flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
    if (deviceDesc.tiers.memory == 0)
        heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = m_Device.GetHeapType(bufferDesc.memoryLocation);
    allocationDesc.Flags = (D3D12MA::ALLOCATION_FLAGS)flags;
    allocationDesc.ExtraHeapFlags = heapFlags;

#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
    D3D12_RESOURCE_DESC1 desc1 = {};
    m_Device.GetResourceDesc(bufferDesc.desc, (D3D12_RESOURCE_DESC&)desc1);

    const D3D12_BARRIER_LAYOUT initialLayout = D3D12_BARRIER_LAYOUT_UNDEFINED;

    HRESULT hr = m_Device.GetVma()->CreateResource3(&allocationDesc, &desc1, initialLayout, nullptr, NO_CASTABLE_FORMATS, &m_VmaAllocation, IID_PPV_ARGS(&m_Buffer));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "D3D12MA::CreateResource3");
#else
    D3D12_RESOURCE_DESC desc = {};
    m_Device.GetResourceDesc(bufferDesc.desc, desc);

    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
    if (bufferDesc.memoryLocation == MemoryLocation::HOST_UPLOAD || bufferDesc.memoryLocation == MemoryLocation::DEVICE_UPLOAD)
        initialState |= D3D12_RESOURCE_STATE_GENERIC_READ;
    else if (bufferDesc.memoryLocation == MemoryLocation::HOST_READBACK)
        initialState |= D3D12_RESOURCE_STATE_COPY_DEST;

    if (bufferDesc.desc.usage & BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE)
        initialState |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

    HRESULT hr = m_Device.GetVma()->CreateResource(&allocationDesc, &desc, initialState, nullptr, &m_VmaAllocation, IID_PPV_ARGS(&m_Buffer));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "D3D12MA::CreateResource");
#endif

    m_Desc = bufferDesc.desc;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = allocationDesc.HeapType;

    return SetPriorityAndPersistentlyMap(bufferDesc.memoryPriority, heapProps);
}

Result TextureD3D12::Create(const AllocateTextureDesc& textureDesc) {
    D3D12_CLEAR_VALUE clearValue = {GetDxgiFormat(textureDesc.desc.format).typed};

    const FormatProps& formatProps = GetFormatProps(textureDesc.desc.format);
    if (formatProps.isDepth || formatProps.isStencil) {
        clearValue.DepthStencil.Depth = textureDesc.desc.optimizedClearValue.depthStencil.depth;
        clearValue.DepthStencil.Stencil = textureDesc.desc.optimizedClearValue.depthStencil.stencil;
    } else {
        clearValue.Color[0] = textureDesc.desc.optimizedClearValue.color.f.x;
        clearValue.Color[1] = textureDesc.desc.optimizedClearValue.color.f.y;
        clearValue.Color[2] = textureDesc.desc.optimizedClearValue.color.f.z;
        clearValue.Color[3] = textureDesc.desc.optimizedClearValue.color.f.w;
    }

    uint32_t flags = D3D12MA::ALLOCATION_FLAG_CAN_ALIAS | D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_MEMORY;
    if (textureDesc.dedicated)
        flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;

    const DeviceDesc& deviceDesc = m_Device.GetDesc();
    D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
    if (deviceDesc.tiers.memory == 0) {
        if (textureDesc.desc.usage & (TextureUsageBits::COLOR_ATTACHMENT | TextureUsageBits::DEPTH_STENCIL_ATTACHMENT) )
            heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
        else
            heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
    }

    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = m_Device.GetHeapType(textureDesc.memoryLocation);
    allocationDesc.Flags = (D3D12MA::ALLOCATION_FLAGS)flags;
    allocationDesc.ExtraHeapFlags = heapFlags;

#ifdef NRI_ENABLE_AGILITY_SDK_SUPPORT
    D3D12_RESOURCE_DESC1 desc1 = {};
    m_Device.GetResourceDesc(textureDesc.desc, (D3D12_RESOURCE_DESC&)desc1);

    bool isRenderableSurface = desc1.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    HRESULT hr = m_Device.GetVma()->CreateResource3(&allocationDesc, &desc1, D3D12_BARRIER_LAYOUT_COMMON, isRenderableSurface ? &clearValue : nullptr, NO_CASTABLE_FORMATS, &m_VmaAllocation, IID_PPV_ARGS(&m_Texture));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "D3D12MA::CreateResource3");
#else
    D3D12_RESOURCE_DESC desc = {};
    m_Device.GetResourceDesc(textureDesc.desc, desc);

    bool isRenderableSurface = desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    HRESULT hr = m_Device.GetVma()->CreateResource(&allocationDesc, &desc, D3D12_RESOURCE_STATE_COMMON, isRenderableSurface ? &clearValue : nullptr, &m_VmaAllocation, IID_PPV_ARGS(&m_Texture));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "D3D12MA::CreateResource");
#endif

    // Priority
    D3D12_RESIDENCY_PRIORITY residencyPriority = (D3D12_RESIDENCY_PRIORITY)ConvertPriority(textureDesc.memoryPriority);
    if (residencyPriority != 0) {
        ID3D12Pageable* obj = m_Texture.GetInterface();
        hr = m_Device->SetResidencyPriority(1, &obj, &residencyPriority);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "ID3D12Device1::SetResidencyPriority");
    }

    m_Desc = FixTextureDesc(textureDesc.desc);

    return Result::SUCCESS;
}

Result AccelerationStructureD3D12::Create(const AllocateAccelerationStructureDesc& accelerationStructureDesc) {
    m_Device.GetAccelerationStructurePrebuildInfo(accelerationStructureDesc.desc, m_PrebuildInfo);
    m_Flags = accelerationStructureDesc.desc.flags;

    AllocateBufferDesc bufferDesc = {};
    bufferDesc.memoryLocation = accelerationStructureDesc.memoryLocation;
    bufferDesc.memoryPriority = accelerationStructureDesc.memoryPriority;
    bufferDesc.desc.size = m_PrebuildInfo.ResultDataMaxSizeInBytes;
    bufferDesc.desc.usage = BufferUsageBits::ACCELERATION_STRUCTURE_STORAGE;

    return m_Device.CreateImplementation<BufferD3D12>(m_Buffer, bufferDesc);
}

Result MicromapD3D12::Create(const AllocateMicromapDesc& micromapDesc) {
    m_Device.GetMicromapPrebuildInfo(micromapDesc.desc, m_PrebuildInfo);
    m_Flags = micromapDesc.desc.flags;

    AllocateBufferDesc bufferDesc = {};
    bufferDesc.memoryLocation = micromapDesc.memoryLocation;
    bufferDesc.memoryPriority = micromapDesc.memoryPriority;
    bufferDesc.desc.size = m_PrebuildInfo.ResultDataMaxSizeInBytes;
    bufferDesc.desc.usage = BufferUsageBits::MICROMAP_STORAGE;

    return m_Device.CreateImplementation<BufferD3D12>(m_Buffer, bufferDesc);
}
