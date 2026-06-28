// © 2021 NVIDIA Corporation

#ifdef _WIN32
#    include <windows.h> // OutputDebugStringA
#else
#    include <csignal> // raise
#    include <cstdlib> // malloc
#endif

#if NRI_ENABLE_NVTX_SUPPORT
#    include "nvtx3/nvToolsExt.h"
#endif

#if (NRI_ENABLE_D3D11_SUPPORT)
#    include <d3d11.h>
#    include <dxgidebug.h>
#endif

#if NRI_ENABLE_D3D12_SUPPORT
#    include <d3d12.h>
#    include <dxgidebug.h>
#    include <d3d12sdklayers.h>
#endif

#if NRI_ENABLE_VK_SUPPORT
#    include <vulkan/vulkan.h>
#endif

#if NRI_ENABLE_WGPU_SUPPORT
#    include <webgpu/wgpu.h>
#endif

#include "SharedExternal.h"

#define ADAPTER_MAX_NUM 32u

using namespace nri;

Result CreateDeviceNONE(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device);
Result CreateDeviceD3D11(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationD3D11Desc& deviceCreationDescD3D11, DeviceBase*& device);
Result CreateDeviceD3D12(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationD3D12Desc& deviceCreationDescD3D12, DeviceBase*& device);
Result CreateDeviceVK(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationVKDesc& deviceCreationDescVK, DeviceBase*& device);
Result CreateDeviceWGPU(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device);
DeviceBase* CreateDeviceValidation(const DeviceCreationDesc& deviceCreationDesc, DeviceBase& device);

constexpr uint64_t Hash(const char* name) {
    return *name != 0 ? *name ^ (33 * Hash(name + 1)) : 5381;
}

constexpr std::array<const char*, (size_t)Message::MAX_NUM> g_messageTypes = {
    "INFO",    // INFO,
    "WARNING", // WARNING,
    "ERROR",   // ERROR
};
NRI_VALIDATE_ARRAY_BY_PTR(g_messageTypes);

static void NRI_CALL MessageCallback(Message messageType, const char* file, uint32_t line, const char* message, void*) {
    const char* messageTypeName = g_messageTypes[(size_t)messageType];

    char buf[NRI_MAX_MESSAGE_LENGTH];
    snprintf(buf, sizeof(buf), "%s (%s:%u) - %s\n", messageTypeName, file, line, message);

    fprintf(stderr, "%s", buf);
#ifdef _WIN32
    OutputDebugStringA(buf);
#endif
}

static void NRI_CALL AbortExecution(void*) {
#ifdef _WIN32
    DebugBreak();
#else
    raise(SIGTRAP);
#endif
}

#define NRI_REPORT_DEVICE_FAULT_INFO(deviceBase, format, ...) \
    (deviceBase).ReportMessage(Message::INFO, Result::DEVICE_LOST, __FILE__, __LINE__, format, ##__VA_ARGS__)

#if NRI_ENABLE_D3D12_SUPPORT

constexpr uint32_t DRED_NODE_MAX_NUM = 16;
constexpr uint32_t DRED_BREADCRUMB_RADIUS = 4;

static const char* GetDredDeviceStateName(D3D12_DRED_DEVICE_STATE state) {
    switch (state) {
        case D3D12_DRED_DEVICE_STATE_UNKNOWN:
            return "Unknown";
        case D3D12_DRED_DEVICE_STATE_HUNG:
            return "Hung";
        case D3D12_DRED_DEVICE_STATE_FAULT:
            return "Fault";
        case D3D12_DRED_DEVICE_STATE_PAGEFAULT:
            return "PageFault";
    }

    return "Invalid";
}

static const char* GetDredBreadcrumbOpName(D3D12_AUTO_BREADCRUMB_OP op) {
    switch (op) {
        case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:
            return "SetMarker";
        case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:
            return "BeginEvent";
        case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:
            return "EndEvent";
        case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:
            return "DrawInstanced";
        case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:
            return "DrawIndexedInstanced";
        case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:
            return "ExecuteIndirect";
        case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:
            return "Dispatch";
        case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:
            return "CopyBufferRegion";
        case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:
            return "CopyTextureRegion";
        case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:
            return "CopyResource";
        case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:
            return "CopyTiles";
        case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:
            return "ResolveSubresource";
        case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:
            return "ClearRenderTargetView";
        case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:
            return "ClearUnorderedAccessView";
        case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:
            return "ClearDepthStencilView";
        case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:
            return "ResourceBarrier";
        case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:
            return "ExecuteBundle";
        case D3D12_AUTO_BREADCRUMB_OP_PRESENT:
            return "Present";
        case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:
            return "ResolveQueryData";
        case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:
            return "BeginSubmission";
        case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:
            return "EndSubmission";
        case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE:
            return "BuildRayTracingAccelerationStructure";
        case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:
            return "DispatchRays";
        case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:
            return "DispatchMesh";
        case D3D12_AUTO_BREADCRUMB_OP_BARRIER:
            return "Barrier";
        case D3D12_AUTO_BREADCRUMB_OP_BEGIN_COMMAND_LIST:
            return "BeginCommandList";
        default:
            return "Unknown";
    }
}

static const char* GetDredObjectName(const char* ansiName, const wchar_t* wideName, char* storage, size_t storageSize) {
    if (ansiName && ansiName[0] != '\0')
        return ansiName;

    if (wideName && wideName[0] != L'\0') {
        ConvertWcharToChar(wideName, storage, storageSize);
        return storage;
    }

    return "<unnamed>";
}

static void ReportDredAllocationList(const char* listName, const D3D12_DRED_ALLOCATION_NODE1* head, const DeviceBase& deviceBase) {
    uint32_t index = 0;
    for (const D3D12_DRED_ALLOCATION_NODE1* node = head; node && index < DRED_NODE_MAX_NUM; node = node->pNext, index++) {
        char allocationName[NRI_MAX_MESSAGE_LENGTH];
        NRI_REPORT_DEVICE_FAULT_INFO(deviceBase, "[D3D12][DRED] %s[%u]: type=%u name=%s", listName, index, (uint32_t)node->AllocationType, GetDredObjectName(node->ObjectNameA, node->ObjectNameW, allocationName, sizeof(allocationName)));
    }
}

static void ReportDredBreadcrumbs(const D3D12_AUTO_BREADCRUMB_NODE1* head, const DeviceBase& deviceBase) {
    uint32_t nodeIndex = 0;
    for (const D3D12_AUTO_BREADCRUMB_NODE1* node = head; node && nodeIndex < DRED_NODE_MAX_NUM; node = node->pNext, nodeIndex++) {
        uint32_t completedBreadcrumb = node->pLastBreadcrumbValue ? *node->pLastBreadcrumbValue : 0;
        char queueName[NRI_MAX_MESSAGE_LENGTH];
        char commandListName[NRI_MAX_MESSAGE_LENGTH];
        NRI_REPORT_DEVICE_FAULT_INFO(deviceBase,
            "[D3D12][DRED] BreadcrumbNode[%u]: queue=%s commandList=%s completed=%u/%u",
            nodeIndex,
            GetDredObjectName(node->pCommandQueueDebugNameA, node->pCommandQueueDebugNameW, queueName, sizeof(queueName)),
            GetDredObjectName(node->pCommandListDebugNameA, node->pCommandListDebugNameW, commandListName, sizeof(commandListName)),
            completedBreadcrumb,
            node->BreadcrumbCount);

        if (!node->pCommandHistory || node->BreadcrumbCount == 0)
            continue;

        uint32_t start = completedBreadcrumb > DRED_BREADCRUMB_RADIUS ? completedBreadcrumb - DRED_BREADCRUMB_RADIUS : 0;
        uint32_t end = node->BreadcrumbCount < completedBreadcrumb + DRED_BREADCRUMB_RADIUS + 1 ? node->BreadcrumbCount : completedBreadcrumb + DRED_BREADCRUMB_RADIUS + 1;
        for (uint32_t breadcrumbIndex = start; breadcrumbIndex < end; breadcrumbIndex++)
            NRI_REPORT_DEVICE_FAULT_INFO(deviceBase,
                "[D3D12][DRED]   op[%u]%s %s",
                breadcrumbIndex,
                breadcrumbIndex == completedBreadcrumb ? " <- last completed" : "",
                GetDredBreadcrumbOpName(node->pCommandHistory[breadcrumbIndex]));

        for (uint32_t contextIndex = 0; contextIndex < node->BreadcrumbContextsCount; contextIndex++) {
            const D3D12_DRED_BREADCRUMB_CONTEXT& context = node->pBreadcrumbContexts[contextIndex];
            if (context.BreadcrumbIndex >= start && context.BreadcrumbIndex < end) {
                char contextString[NRI_MAX_MESSAGE_LENGTH];
                const char* contextName = "<unnamed>";
                if (context.pContextString) {
                    ConvertWcharToChar(context.pContextString, contextString, sizeof(contextString));
                    contextName = contextString;
                }

                NRI_REPORT_DEVICE_FAULT_INFO(deviceBase, "[D3D12][DRED]   context[%u]: %s", context.BreadcrumbIndex, contextName);
            }
        }
    }
}

static void ReportD3D12DeviceRemovedExtendedData(ID3D12Device* nativeDevice, const DeviceBase& deviceBase) {
    ComPtr<ID3D12DeviceRemovedExtendedData2> dred2;
    if (SUCCEEDED(nativeDevice->QueryInterface(IID_PPV_ARGS(&dred2))) && dred2) {
        D3D12_DRED_DEVICE_STATE deviceState = dred2->GetDeviceState();
        NRI_REPORT_DEVICE_FAULT_INFO(deviceBase, "[D3D12][DRED] Device state: %s", GetDredDeviceStateName(deviceState));

        D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbs = {};
        if (SUCCEEDED(dred2->GetAutoBreadcrumbsOutput1(&breadcrumbs)))
            ReportDredBreadcrumbs(breadcrumbs.pHeadAutoBreadcrumbNode, deviceBase);

        D3D12_DRED_PAGE_FAULT_OUTPUT2 pageFault = {};
        if (SUCCEEDED(dred2->GetPageFaultAllocationOutput2(&pageFault))) {
            NRI_REPORT_DEVICE_FAULT_INFO(deviceBase, "[D3D12][DRED] PageFaultVA=0x%016llX flags=%u", (unsigned long long)pageFault.PageFaultVA, (uint32_t)pageFault.PageFaultFlags);
            ReportDredAllocationList("ExistingAllocation", pageFault.pHeadExistingAllocationNode, deviceBase);
            ReportDredAllocationList("RecentFreedAllocation", pageFault.pHeadRecentFreedAllocationNode, deviceBase);
        }

        return;
    }

    ComPtr<ID3D12DeviceRemovedExtendedData1> dred1;
    if (SUCCEEDED(nativeDevice->QueryInterface(IID_PPV_ARGS(&dred1))) && dred1) {
        D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbs = {};
        if (SUCCEEDED(dred1->GetAutoBreadcrumbsOutput1(&breadcrumbs)))
            ReportDredBreadcrumbs(breadcrumbs.pHeadAutoBreadcrumbNode, deviceBase);

        D3D12_DRED_PAGE_FAULT_OUTPUT1 pageFault = {};
        if (SUCCEEDED(dred1->GetPageFaultAllocationOutput1(&pageFault))) {
            NRI_REPORT_DEVICE_FAULT_INFO(deviceBase, "[D3D12][DRED] PageFaultVA=0x%016llX", (unsigned long long)pageFault.PageFaultVA);
            ReportDredAllocationList("ExistingAllocation", pageFault.pHeadExistingAllocationNode, deviceBase);
            ReportDredAllocationList("RecentFreedAllocation", pageFault.pHeadRecentFreedAllocationNode, deviceBase);
        }
    }
}

static Result EnableD3D12DeviceFaultInfo(DeviceFaultInfoLevel level) {
    if (level == DeviceFaultInfoLevel::NONE || level == DeviceFaultInfoLevel::BASIC)
        return Result::SUCCESS;

    ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dredSettings;
    HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings));
    if (FAILED(hr) || !dredSettings)
        return Result::UNSUPPORTED;

    dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    dredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

    return Result::SUCCESS;
}

#endif

#if NRI_ENABLE_VK_SUPPORT

static Result GetResultFromDeviceFaultVkResult(VkResult vkResult) {
    if (vkResult >= 0)
        return Result::SUCCESS;

    switch (vkResult) {
        case VK_ERROR_DEVICE_LOST:
            return Result::DEVICE_LOST;

        case VK_ERROR_FEATURE_NOT_PRESENT:
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return Result::UNSUPPORTED;

        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return Result::OUT_OF_MEMORY;

        default:
            return Result::FAILURE;
    }
}

#    ifdef VK_KHR_device_fault
static const char* GetVkDeviceFaultAddressTypeName(VkDeviceFaultAddressTypeKHR type) {
    switch (type) {
        case VK_DEVICE_FAULT_ADDRESS_TYPE_NONE_KHR:
            return "None";
        case VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_KHR:
            return "ReadInvalid";
        case VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_KHR:
            return "WriteInvalid";
        case VK_DEVICE_FAULT_ADDRESS_TYPE_EXECUTE_INVALID_KHR:
            return "ExecuteInvalid";
        case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_UNKNOWN_KHR:
            return "InstructionPointerUnknown";
        case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_INVALID_KHR:
            return "InstructionPointerInvalid";
        case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_KHR:
            return "InstructionPointerFault";
        default:
            return "Unknown";
    }
}

static void ReportVKDeviceFaultAddressInfo(const DeviceBase& deviceBase, const char* name, const VkDeviceFaultAddressInfoKHR& addressInfo) {
    NRI_REPORT_DEVICE_FAULT_INFO(deviceBase,
        "[VK][DeviceFault]   %s: type=%s address=0x%016" PRIX64 " precision=0x%016" PRIX64,
        name,
        GetVkDeviceFaultAddressTypeName(addressInfo.addressType),
        addressInfo.reportedAddress,
        addressInfo.addressPrecision);
}

static Result ReportVKDeviceFaultInfo(const Device& device, const DeviceBase& deviceBase) {
    CoreInterface core = {};
    Result result = deviceBase.FillFunctionTable(core);
    if (result != Result::SUCCESS || !core.GetDeviceNativeObject)
        return Result::UNSUPPORTED;

    WrapperVKInterface wrapperVK = {};
    result = deviceBase.FillFunctionTable(wrapperVK);
    if (result != Result::SUCCESS || !wrapperVK.GetDeviceProcAddrVK)
        return Result::UNSUPPORTED;

    VkDevice nativeDevice = (VkDevice)core.GetDeviceNativeObject(&device);
    if (!nativeDevice)
        return Result::INVALID_ARGUMENT;

    PFN_vkGetDeviceProcAddr getDeviceProcAddr = (PFN_vkGetDeviceProcAddr)wrapperVK.GetDeviceProcAddrVK(device);
    if (!getDeviceProcAddr)
        return Result::UNSUPPORTED;

    PFN_vkGetDeviceFaultReportsKHR getDeviceFaultReports = (PFN_vkGetDeviceFaultReportsKHR)getDeviceProcAddr(nativeDevice, "vkGetDeviceFaultReportsKHR");
    if (!getDeviceFaultReports)
        return Result::UNSUPPORTED;

    uint32_t faultReportNum = 0;
    VkResult vkResult = getDeviceFaultReports(nativeDevice, 0, &faultReportNum, nullptr);
    if (vkResult < 0)
        return GetResultFromDeviceFaultVkResult(vkResult);

    StdAllocator<uint8_t>& allocator = ((DeviceBase&)deviceBase).GetStdAllocator();
    Vector<VkDeviceFaultInfoKHR> faultReports(faultReportNum, {VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_KHR}, allocator);

    vkResult = getDeviceFaultReports(nativeDevice, 0, &faultReportNum, faultReports.data());
    if (vkResult < 0)
        return GetResultFromDeviceFaultVkResult(vkResult);

    NRI_REPORT_DEVICE_FAULT_INFO(deviceBase, "[VK][DeviceFault] reportCount=%u", faultReportNum);
    for (uint32_t i = 0; i < faultReportNum; i++) {
        const VkDeviceFaultInfoKHR& faultReport = faultReports[i];
        NRI_REPORT_DEVICE_FAULT_INFO(deviceBase,
            "[VK][DeviceFault] Report[%u]: flags=0x%08X groupId=0x%016" PRIX64 " description=%s",
            i,
            faultReport.flags,
            faultReport.groupId,
            faultReport.description);
        ReportVKDeviceFaultAddressInfo(deviceBase, "FaultAddress", faultReport.faultAddressInfo);
        ReportVKDeviceFaultAddressInfo(deviceBase, "InstructionAddress", faultReport.instructionAddressInfo);

        NRI_REPORT_DEVICE_FAULT_INFO(deviceBase,
            "[VK][DeviceFault]   VendorInfo: code=0x%016" PRIX64 " data=0x%016" PRIX64 " description=%s",
            faultReport.vendorInfo.vendorFaultCode,
            faultReport.vendorInfo.vendorFaultData,
            faultReport.vendorInfo.description);
    }

    PFN_vkGetDeviceFaultDebugInfoKHR getDeviceFaultDebugInfo = (PFN_vkGetDeviceFaultDebugInfoKHR)getDeviceProcAddr(nativeDevice, "vkGetDeviceFaultDebugInfoKHR");
    if (getDeviceFaultDebugInfo) {
        VkDeviceFaultDebugInfoKHR debugInfo = {VK_STRUCTURE_TYPE_DEVICE_FAULT_DEBUG_INFO_KHR};
        VkResult debugResult = getDeviceFaultDebugInfo(nativeDevice, &debugInfo);
        if (debugResult >= 0)
            NRI_REPORT_DEVICE_FAULT_INFO(deviceBase, "[VK][DeviceFault] vendorBinarySize=%u", debugInfo.vendorBinarySize);
    }

    return Result::SUCCESS;
}
#    endif

#endif

#ifdef _WIN32

static void* NRI_CALL AlignedMalloc(void*, size_t size, size_t alignment) {
    return _aligned_malloc(size, alignment);
}

static void* NRI_CALL AlignedRealloc(void*, void* memory, size_t size, size_t alignment) {
    return _aligned_realloc(memory, size, alignment);
}

static void NRI_CALL AlignedFree(void*, void* memory) {
    _aligned_free(memory);
}

#else

static void* NRI_CALL AlignedMalloc(void*, size_t size, size_t alignment) {
    uint8_t* memory = (uint8_t*)malloc(size + sizeof(uint8_t*) + alignment - 1);
    if (!memory)
        return nullptr;

    uint8_t* alignedMemory = Align(memory + sizeof(uint8_t*), alignment);
    uint8_t** memoryHeader = (uint8_t**)alignedMemory - 1;
    *memoryHeader = memory;

    return alignedMemory;
}

static void* NRI_CALL AlignedRealloc(void* userArg, void* memory, size_t size, size_t alignment) {
    if (!memory)
        return AlignedMalloc(userArg, size, alignment);

    uint8_t** memoryHeader = (uint8_t**)memory - 1;
    uint8_t* oldMemory = *memoryHeader;

    uint8_t* newMemory = (uint8_t*)realloc(oldMemory, size + sizeof(uint8_t*) + alignment - 1);
    if (!newMemory)
        return nullptr;

    if (newMemory == oldMemory)
        return memory;

    uint8_t* alignedMemory = Align(newMemory + sizeof(uint8_t*), alignment);
    memoryHeader = (uint8_t**)alignedMemory - 1;
    *memoryHeader = newMemory;

    return alignedMemory;
}

static void NRI_CALL AlignedFree(void*, void* memory) {
    if (!memory)
        return;

    uint8_t** memoryHeader = (uint8_t**)memory - 1;
    uint8_t* oldMemory = *memoryHeader;
    free(oldMemory);
}

#endif

static void CheckAndSetDefaultCallbacks(DeviceCreationDesc& deviceCreationDesc) {
    if (!deviceCreationDesc.callbackInterface.MessageCallback)
        deviceCreationDesc.callbackInterface.MessageCallback = MessageCallback;

    if (!deviceCreationDesc.callbackInterface.AbortExecution)
        deviceCreationDesc.callbackInterface.AbortExecution = AbortExecution;

    if (!deviceCreationDesc.allocationCallbacks.Allocate || !deviceCreationDesc.allocationCallbacks.Reallocate || !deviceCreationDesc.allocationCallbacks.Free) {
        deviceCreationDesc.allocationCallbacks.Allocate = AlignedMalloc;
        deviceCreationDesc.allocationCallbacks.Reallocate = AlignedRealloc;
        deviceCreationDesc.allocationCallbacks.Free = AlignedFree;
    }
}

static Result EnableDeviceFaultInfo(const DeviceCreationDesc& deviceCreationDesc) {
    if (deviceCreationDesc.deviceFaultInfoLevel == DeviceFaultInfoLevel::NONE)
        return Result::SUCCESS;

#if NRI_ENABLE_D3D12_SUPPORT
    if (deviceCreationDesc.graphicsAPI == GraphicsAPI::D3D12)
        return EnableD3D12DeviceFaultInfo(deviceCreationDesc.deviceFaultInfoLevel);
#endif

#if NRI_ENABLE_VK_SUPPORT
    if (deviceCreationDesc.graphicsAPI == GraphicsAPI::VK)
        return Result::SUCCESS;
#endif

    return Result::UNSUPPORTED;
}

static void ReportDeviceFaultInfoEnableWarning(const DeviceCreationDesc& deviceCreationDesc, Result result) {
    if (!deviceCreationDesc.callbackInterface.MessageCallback)
        return;

    char message[NRI_MAX_MESSAGE_LENGTH];
    snprintf(message, sizeof(message), "Device fault info level %u is not available for the selected backend, result=%u.", (uint32_t)deviceCreationDesc.deviceFaultInfoLevel, (uint32_t)result);
    deviceCreationDesc.callbackInterface.MessageCallback(Message::WARNING, __FILE__, __LINE__, message, deviceCreationDesc.callbackInterface.userArg);
}

static int SortAdapters(const void* pa, const void* pb) {
    constexpr uint64_t SHIFT = 60ull;
    static_assert((uint64_t)Architecture::MAX_NUM <= 1ull << (64ull - SHIFT), "Adjust SHIFT");

    const AdapterDesc* a = (AdapterDesc*)pa;
    uint64_t sa = a->videoMemorySize + a->sharedSystemMemorySize;
    sa |= (uint64_t)(a->architecture) << SHIFT;

    const AdapterDesc* b = (AdapterDesc*)pb;
    uint64_t sb = b->videoMemorySize + b->sharedSystemMemorySize;
    sb |= (uint64_t)(b->architecture) << SHIFT;

    if (sa > sb)
        return -1;

    if (sa < sb)
        return 1;

    return 0;
}

#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)

static void UpdateAdaptersD3D(AdapterDesc* adapterDescs, uint32_t& adapterDescNum, LUID* precreatedLuid) {
    ComPtr<IDXGIFactory4> dxgifactory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgifactory))))
        return;

    for (uint32_t i = 0; i < 256 && adapterDescNum < ADAPTER_MAX_NUM; i++) {
        ComPtr<IDXGIAdapter1> adapter;
        HRESULT hr = dxgifactory->EnumAdapters1(i, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 desc = {};
        if (FAILED(adapter->GetDesc1(&desc)))
            continue;

        // Logic: append unique or wait for "precreated"
        Uid_t uid = {};
        memcpy(&uid.low, &desc.AdapterLuid, sizeof(uid.low));

        uint32_t n = 0;
        if (precreatedLuid) {
            Uid_t uidNeeded = {};
            memcpy(&uidNeeded.low, precreatedLuid, sizeof(uidNeeded.low));

            if (!CompareUid(uid, uidNeeded))
                continue;
        } else {
            for (; n < adapterDescNum; n++) {
                if (CompareUid(adapterDescs[n].uid, uid))
                    break;
            }
        }

        AdapterDesc& adapterDesc = adapterDescs[n];

        // Update GAPI support
#    if (NRI_ENABLE_D3D11_SUPPORT)
        D3D_FEATURE_LEVEL levels[2] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
        uint32_t levelNum = GetCountOf(levels);
        ComPtr<ID3D11Device> deviceD3D11;
        hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, levels, levelNum, D3D11_SDK_VERSION, &deviceD3D11, nullptr, nullptr);
        if (SUCCEEDED(hr))
            adapterDesc.supportedGraphicsAPIs |= GraphicsAPI::D3D11;
#    endif

#    if (NRI_ENABLE_D3D12_SUPPORT)
        hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr);
        if (SUCCEEDED(hr))
            adapterDesc.supportedGraphicsAPIs |= GraphicsAPI::D3D12;
#    endif

        // Logic: advance or skip
        if (n != adapterDescNum)
            continue;

        adapterDescNum++;

        { // Driver version
            LARGE_INTEGER driverVersion;
            if (SUCCEEDED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &driverVersion)))
                adapterDesc.driverVersion = driverVersion.LowPart;
        }

        // Architecture
        if (desc.Flags == DXGI_ADAPTER_FLAG_NONE) {
#    if (NRI_ENABLE_D3D11_SUPPORT)
            // Simpler and faster
            if (deviceD3D11) {
                D3D11_FEATURE_DATA_D3D11_OPTIONS2 options2 = {};
                hr = deviceD3D11->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &options2, sizeof(options2));
                if (SUCCEEDED(hr))
                    adapterDesc.architecture = options2.UnifiedMemoryArchitecture ? Architecture::INTEGRATED : Architecture::DISCRETE;
            }
#    else
            ComPtr<ID3D12Device> deviceD3D12;
            hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&deviceD3D12);
            if (SUCCEEDED(hr)) {
                D3D12_FEATURE_DATA_ARCHITECTURE architecture = {};
                hr = deviceD3D12->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &architecture, sizeof(architecture));
                if (SUCCEEDED(hr))
                    adapterDesc.architecture = architecture.UMA ? Architecture::INTEGRATED : Architecture::DISCRETE;
            }
#    endif
        } else if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            adapterDesc.architecture = Architecture::SOFTWARE;
        else if (desc.Flags & DXGI_ADAPTER_FLAG_REMOTE)
            adapterDesc.architecture = Architecture::VIRTUAL;

        // Advertise reasonable values for queue counts (can't be queried in D3D)
        adapterDesc.queueNum[(uint32_t)QueueType::GRAPHICS] = 4;
        adapterDesc.queueNum[(uint32_t)QueueType::COMPUTE] = 4;
        adapterDesc.queueNum[(uint32_t)QueueType::COPY] = 4;

        // Other fields
        adapterDesc.uid = uid;
        adapterDesc.deviceId = desc.DeviceId;
        adapterDesc.vendor = GetVendorFromID(desc.VendorId);
        adapterDesc.videoMemorySize = desc.DedicatedVideoMemory + desc.DedicatedSystemMemory;
        adapterDesc.sharedSystemMemorySize = desc.SharedSystemMemory;

        wcstombs(adapterDesc.name, desc.Description, GetCountOf(adapterDesc.name) - 1);

        // Logic: only one "adapterDesc" is needed for "precreated"
        if (precreatedLuid)
            break;
    }
}

#endif

#if NRI_ENABLE_VK_SUPPORT

static void UpdateAdaptersVK(AdapterDesc* adapterDescs, uint32_t& adapterDescNum, VkPhysicalDevice precreatedPhysicalDevice) {
    // Variables first
    VkApplicationInfo applicationInfo = {};
    applicationInfo.apiVersion = VK_API_VERSION_1_2; // 1.3 not needed here

    VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    VkInstance instance = VK_NULL_HANDLE;
    Uid_t uidNeeded = {};
    VkResult vkResult = VK_SUCCESS;
    uint32_t deviceGroupNum = 0;
    uint32_t maxFamilyNum = 1;
    VkPhysicalDeviceGroupProperties* deviceGroupProperties = nullptr;
    VkQueueFamilyProperties2* familyProps2 = nullptr;

    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
    PFN_vkCreateInstance vkCreateInstance = nullptr;
    PFN_vkDestroyInstance vkDestroyInstance = nullptr;
    PFN_vkEnumeratePhysicalDeviceGroups vkEnumeratePhysicalDeviceGroups = nullptr;
    PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 = nullptr;
    PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties2 vkGetPhysicalDeviceQueueFamilyProperties2 = nullptr;

#    ifdef __APPLE__
    std::array<const char*, 2> instanceExtensions = {"VK_KHR_get_physical_device_properties2", VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};

    instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#    endif

    // Get loader
    Library* loader = LoadSharedLibrary(NRI_VULKAN_LOADER_NAME);
    if (!loader)
        goto CLEANUP;

    // Get the entry point
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetSharedLibraryFunction(*loader, "vkGetInstanceProcAddr");
    if (!vkGetInstanceProcAddr)
        goto CLEANUP;

    // Create instance
    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
    if (!vkCreateInstance)
        goto CLEANUP;

    vkResult = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    if (vkResult != VK_SUCCESS)
        goto CLEANUP;

    // Get needed functions
    vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
    vkEnumeratePhysicalDeviceGroups = (PFN_vkEnumeratePhysicalDeviceGroups)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDeviceGroups");
    vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2");
    vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties");
    vkGetPhysicalDeviceQueueFamilyProperties2 = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties2");

    if (!vkDestroyInstance || !vkEnumeratePhysicalDeviceGroups || !vkGetPhysicalDeviceProperties2)
        goto CLEANUP;

    vkResult = vkEnumeratePhysicalDeviceGroups(instance, &deviceGroupNum, nullptr);
    if (vkResult != VK_SUCCESS)
        goto CLEANUP;

    // Query device groups
    deviceGroupProperties = (VkPhysicalDeviceGroupProperties*)alloca(sizeof(VkPhysicalDeviceGroupProperties) * deviceGroupNum);
    for (uint32_t i = 0; i < deviceGroupNum; i++) {
        deviceGroupProperties[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
        deviceGroupProperties[i].pNext = nullptr;
    }
    vkEnumeratePhysicalDeviceGroups(instance, &deviceGroupNum, deviceGroupProperties);

    // Max queue families
    if (vkGetPhysicalDeviceQueueFamilyProperties2) {
        for (uint32_t i = 0; i < deviceGroupNum; i++) {
            VkPhysicalDevice physicalDevice = deviceGroupProperties[i].physicalDevices[0];

            uint32_t familyNum = 0;
            vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &familyNum, nullptr);

            maxFamilyNum = std::max(maxFamilyNum, familyNum);
        }
    }

    familyProps2 = (VkQueueFamilyProperties2*)alloca(sizeof(VkQueueFamilyProperties2) * maxFamilyNum);
    for (uint32_t i = 0; i < maxFamilyNum; i++)
        familyProps2[i] = {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2};

    // Precreated physical device
    if (precreatedPhysicalDevice) {
        VkPhysicalDeviceProperties2 deviceProps2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

        VkPhysicalDeviceIDProperties deviceIDProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
        deviceProps2.pNext = &deviceIDProperties;

        vkGetPhysicalDeviceProperties2(precreatedPhysicalDevice, &deviceProps2);

        uidNeeded = ConstructUid(deviceIDProperties.deviceLUID, deviceIDProperties.deviceUUID, deviceIDProperties.deviceLUIDValid);
    }

    // Query device groups properties
    for (uint32_t i = 0; i < deviceGroupNum && adapterDescNum < ADAPTER_MAX_NUM; i++) {
        VkPhysicalDevice physicalDevice = deviceGroupProperties[i].physicalDevices[0];

        VkPhysicalDeviceProperties2 deviceProps2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

        VkPhysicalDeviceIDProperties deviceIDProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
        deviceProps2.pNext = &deviceIDProperties;

        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProps2);

        // Logic: append unique or wait for "precreated"
        Uid_t uid = ConstructUid(deviceIDProperties.deviceLUID, deviceIDProperties.deviceUUID, deviceIDProperties.deviceLUIDValid);

        uint32_t n = 0;
        if (precreatedPhysicalDevice) {
            if (!CompareUid(uid, uidNeeded))
                continue;
        } else {
            for (; n < adapterDescNum; n++) {
                if (CompareUid(adapterDescs[n].uid, uid))
                    break;
            }
        }

        AdapterDesc& adapterDesc = adapterDescs[n];

        // Update GAPI support
        adapterDesc.supportedGraphicsAPIs |= GraphicsAPI::VK;

        // Logic: advance or skip
        if (n != adapterDescNum)
            continue;

        adapterDescNum++;

        // Architecture
        const VkPhysicalDeviceProperties& deviceProps = deviceProps2.properties;

        if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            adapterDesc.architecture = Architecture::DISCRETE;
        else if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            adapterDesc.architecture = Architecture::INTEGRATED;
        else if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
            adapterDesc.architecture = Architecture::SOFTWARE;
        else if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
            adapterDesc.architecture = Architecture::VIRTUAL;

        // Memory size
        if (vkGetPhysicalDeviceMemoryProperties) {
            VkPhysicalDeviceMemoryProperties memoryProperties = {};
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceMemoryProperties.html
            for (uint32_t j = 0; j < memoryProperties.memoryHeapCount; j++) {
                // From spec: In UMA systems ... implementation must advertise the heap as device-local
                if ((memoryProperties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0 && deviceProps.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                    adapterDesc.videoMemorySize += memoryProperties.memoryHeaps[j].size;
                else
                    adapterDesc.sharedSystemMemorySize += memoryProperties.memoryHeaps[j].size;
            }
        }

        // Queues
        if (vkGetPhysicalDeviceQueueFamilyProperties2) {
            uint32_t familyNum = maxFamilyNum;
            vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &familyNum, familyProps2);

            std::array<uint32_t, (size_t)QueueType::MAX_NUM> scores = {};
            for (uint32_t j = 0; j < familyNum; j++) {
                const VkQueueFamilyProperties& familyProps = familyProps2[j].queueFamilyProperties;

                QueueFamilyProps props = {};
                props.queueCount = familyProps.queueCount;
                props.graphics = familyProps.queueFlags & VK_QUEUE_GRAPHICS_BIT;
                props.compute = familyProps.queueFlags & VK_QUEUE_COMPUTE_BIT;
                props.copy = familyProps.queueFlags & VK_QUEUE_TRANSFER_BIT;
                props.sparse = familyProps.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
                props.videoDecode = familyProps.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR;
                props.videoEncode = familyProps.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR;
                props.protect = familyProps.queueFlags & VK_QUEUE_PROTECTED_BIT;
                props.opticalFlow = familyProps.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV;

                QueueType queueType = TrySelectPreferredQueueType(props, scores);
                if (queueType != QueueType::MAX_NUM)
                    adapterDesc.queueNum[(size_t)queueType] = familyProps.queueCount;
            }
        }

        // Other fields
        adapterDesc.uid = uid;
        adapterDesc.deviceId = deviceProps.deviceID;
        adapterDesc.driverVersion = deviceProps.driverVersion;
        adapterDesc.vendor = GetVendorFromID(deviceProps.vendorID);

        strncpy(adapterDesc.name, deviceProps.deviceName, sizeof(adapterDesc.name));

        // Logic: only one "adapterDesc" is needed for "precreated"
        if (precreatedPhysicalDevice)
            break;
    }

CLEANUP:
    if (vkDestroyInstance && instance)
        vkDestroyInstance(instance, nullptr);

    if (loader)
        UnloadSharedLibrary(*loader);
}

#endif

#if NRI_ENABLE_WGPU_SUPPORT

static Architecture GetArchitecture(WGPUAdapterType adapterType) {
    switch (adapterType) {
        case WGPUAdapterType_DiscreteGPU: return Architecture::DISCRETE;
        case WGPUAdapterType_IntegratedGPU: return Architecture::INTEGRATED;
        case WGPUAdapterType_CPU: return Architecture::SOFTWARE;
        default: return Architecture::UNKNOWN;
    }
}

static void CopyString(char* dst, size_t dstSize, WGPUStringView src) {
    if (!dstSize)
        return;

    size_t n = src.data ? std::min(src.length, dstSize - 1) : 0;
    if (n)
        memcpy(dst, src.data, n);

    dst[n] = 0;
}

static void UpdateAdaptersWGPU(AdapterDesc* adapterDescs, uint32_t& adapterDescNum) {
    WGPUInstanceExtras instanceExtras = {};
    instanceExtras.chain.sType = (WGPUSType)WGPUSType_InstanceExtras;
    instanceExtras.backends = WGPUInstanceBackend_Primary;

    WGPUInstanceDescriptor instanceDesc = WGPU_INSTANCE_DESCRIPTOR_INIT;
    instanceDesc.nextInChain = &instanceExtras.chain;

    WGPUInstance instance = wgpuCreateInstance(&instanceDesc);
    if (!instance)
        return;

    WGPUInstanceEnumerateAdapterOptions options = {};
    options.backends = WGPUInstanceBackend_Primary;

    size_t wgpuAdapterNum = wgpuInstanceEnumerateAdapters(instance, &options, nullptr);
    WGPUAdapter* wgpuAdapters = (WGPUAdapter*)alloca(sizeof(WGPUAdapter) * wgpuAdapterNum);
    wgpuAdapterNum = wgpuInstanceEnumerateAdapters(instance, &options, wgpuAdapters);

    for (size_t i = 0; i < wgpuAdapterNum; i++) {
        WGPUAdapterInfo adapterInfo = WGPU_ADAPTER_INFO_INIT;
        if (wgpuAdapterGetInfo(wgpuAdapters[i], &adapterInfo) != WGPUStatus_Success) {
            wgpuAdapterRelease(wgpuAdapters[i]);
            continue;
        }

        if (wgpuAdapterHasFeature(wgpuAdapters[i], (WGPUFeatureName)WGPUNativeFeature_Immediates) != WGPU_TRUE) {
            wgpuAdapterInfoFreeMembers(adapterInfo);
            wgpuAdapterRelease(wgpuAdapters[i]);
            continue;
        }

        WGPUNativeLimits nativeLimits = {};
        nativeLimits.chain.sType = (WGPUSType)WGPUSType_NativeLimits;

        WGPULimits limits = WGPU_LIMITS_INIT;
        limits.nextInChain = &nativeLimits.chain;
        if (wgpuAdapterGetLimits(wgpuAdapters[i], &limits) != WGPUStatus_Success || nativeLimits.maxImmediateSize < 256) {
            wgpuAdapterInfoFreeMembers(adapterInfo);
            wgpuAdapterRelease(wgpuAdapters[i]);
            continue;
        }

        Vendor vendor = GetVendorFromID(adapterInfo.vendorID);

        uint32_t n = 0;
        for (; n < adapterDescNum; n++) {
            bool isSameDevice = adapterInfo.vendorID != 0 && adapterInfo.deviceID != 0 && adapterDescs[n].vendor == vendor && adapterDescs[n].deviceId == adapterInfo.deviceID;
            if (isSameDevice)
                break;
        }

        if (n == adapterDescNum) {
            if (adapterDescNum == ADAPTER_MAX_NUM) {
                wgpuAdapterInfoFreeMembers(adapterInfo);
                wgpuAdapterRelease(wgpuAdapters[i]);
                continue;
            }

            AdapterDesc& adapterDesc = adapterDescs[adapterDescNum++];
            adapterDesc.sharedSystemMemorySize = 128ull << 30;
            adapterDesc.queueNum[(uint32_t)QueueType::GRAPHICS] = 1;
            adapterDesc.queueNum[(uint32_t)QueueType::COMPUTE] = 0;
            adapterDesc.queueNum[(uint32_t)QueueType::COPY] = 0;
            adapterDesc.vendor = vendor;
            adapterDesc.architecture = GetArchitecture(adapterInfo.adapterType);
            adapterDesc.deviceId = adapterInfo.deviceID;

            CopyString(adapterDesc.name, sizeof(adapterDesc.name), adapterInfo.device);
            if (!adapterDesc.name[0])
                strncpy(adapterDesc.name, "WGPU", sizeof(adapterDesc.name));
        }

        adapterDescs[n].supportedGraphicsAPIs |= GraphicsAPI::WGPU;

        wgpuAdapterInfoFreeMembers(adapterInfo);
        wgpuAdapterRelease(wgpuAdapters[i]);
    }

    wgpuInstanceRelease(instance);
}

#endif

static Result FinalizeDeviceCreation(const DeviceCreationDesc& deviceCreationDesc, DeviceBase& deviceImpl, Device*& device) {
    MaybeUnused(deviceCreationDesc);
#if NRI_ENABLE_VALIDATION_SUPPORT
    if (deviceCreationDesc.enableNRIValidation && deviceCreationDesc.graphicsAPI != GraphicsAPI::NONE) {
        Device* deviceVal = (Device*)CreateDeviceValidation(deviceCreationDesc, deviceImpl);
        if (!deviceVal) {
            nriDestroyDevice((Device*)&deviceImpl);
            return Result::FAILURE;
        }

        device = deviceVal;
    } else
#endif
        device = (Device*)&deviceImpl;

#if NRI_ENABLE_NVTX_SUPPORT
    nvtxInitialize(nullptr); // needed only to avoid stalls on the first use
#endif

    return Result::SUCCESS;
}

NRI_API Result NRI_CALL nriGetInterface(const Device& device, const char* interfaceName, size_t interfaceSize, void* interfacePtr) {
    if (strstr(interfaceName, "nri::") == interfaceName)
        interfaceName += 5;
    else if (strstr(interfaceName, "Nri") == interfaceName)
        interfaceName += 3;

    uint64_t hash = Hash(interfaceName);
    size_t realInterfaceSize = size_t(-1);
    Result result = Result::INVALID_ARGUMENT;
    const DeviceBase& deviceBase = (DeviceBase&)device;

    memset(interfacePtr, 0, interfaceSize);

    if (hash == Hash(NRI_STRINGIFY(CoreInterface))) {
        realInterfaceSize = sizeof(CoreInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(CoreInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(ImguiInterface))) {
        realInterfaceSize = sizeof(ImguiInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(ImguiInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(HelperInterface))) {
        realInterfaceSize = sizeof(HelperInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(HelperInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(LowLatencyInterface))) {
        realInterfaceSize = sizeof(LowLatencyInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(LowLatencyInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(MeshShaderInterface))) {
        realInterfaceSize = sizeof(MeshShaderInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(MeshShaderInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(RayTracingInterface))) {
        realInterfaceSize = sizeof(RayTracingInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(RayTracingInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(StreamerInterface))) {
        realInterfaceSize = sizeof(StreamerInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(StreamerInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(SwapChainInterface))) {
        realInterfaceSize = sizeof(SwapChainInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(SwapChainInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(UpscalerInterface))) {
        realInterfaceSize = sizeof(UpscalerInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(UpscalerInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(WrapperD3D11Interface))) {
        realInterfaceSize = sizeof(WrapperD3D11Interface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperD3D11Interface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(WrapperD3D12Interface))) {
        realInterfaceSize = sizeof(WrapperD3D12Interface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperD3D12Interface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(WrapperVKInterface))) {
        realInterfaceSize = sizeof(WrapperVKInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperVKInterface*)interfacePtr);
    }

    if (result == Result::INVALID_ARGUMENT)
        NRI_REPORT_ERROR(&deviceBase, "Unknown interface '%s'!", interfaceName);
    else if (interfaceSize != realInterfaceSize)
        NRI_REPORT_ERROR(&deviceBase, "Interface '%s' has invalid size=%u bytes, while %u bytes expected by the implementation", interfaceName, interfaceSize, realInterfaceSize);
    else if (result == Result::UNSUPPORTED)
        NRI_REPORT_WARNING(&deviceBase, "Interface '%s' is not supported by the device!", interfaceName);
    else {
        const void* const* const begin = (void**)interfacePtr;
        const void* const* const end = begin + realInterfaceSize / sizeof(void*);

        for (const void* const* current = begin; current != end; current++) {
            if (!(*current)) {
                NRI_REPORT_ERROR(&deviceBase, "Invalid function table: function #%u is NULL!", uint32_t(current - begin));
                return Result::FAILURE;
            }
        }
    }

    return result;
}

NRI_API void NRI_CALL nriBeginAnnotation(const char* name, uint32_t bgra) {
    MaybeUnused(name, bgra);

#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
#    if NRI_ENABLE_NVTX_SUPPORT

    nvtxEventAttributes_t eventAttrib = {};
    eventAttrib.version = NVTX_VERSION;
    eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    eventAttrib.colorType = bgra == BGRA_UNUSED ? NVTX_COLOR_UNKNOWN : NVTX_COLOR_ARGB;
    eventAttrib.color = bgra;
    eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
    eventAttrib.message.ascii = name;

    nvtxRangePushEx(&eventAttrib);

#    else

    // TODO: add PIX

#    endif
#endif
}

NRI_API void NRI_CALL nriEndAnnotation() {
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
#    if NRI_ENABLE_NVTX_SUPPORT

    nvtxRangePop();

#    else

    // TODO: add PIX

#    endif
#endif
}

NRI_API void NRI_CALL nriAnnotation(const char* name, uint32_t bgra) {
    MaybeUnused(name, bgra);

#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
#    if NRI_ENABLE_NVTX_SUPPORT

    nvtxEventAttributes_t eventAttrib = {};
    eventAttrib.version = NVTX_VERSION;
    eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    eventAttrib.colorType = bgra == BGRA_UNUSED ? NVTX_COLOR_UNKNOWN : NVTX_COLOR_ARGB;
    eventAttrib.color = bgra;
    eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
    eventAttrib.message.ascii = name;

    nvtxMarkEx(&eventAttrib);

#    else

    // TODO: add PIX

#    endif
#endif
}

#if NRI_ENABLE_NVTX_SUPPORT
#    if (defined __linux__)
#        include <sys/syscall.h>
#    elif (defined __APPLE__)
#        include <sys/syscall.h>
#    elif (defined __ANDROID__)
#        include <unistd.h>
#    elif (defined _WIN64)
#        include <processthreadsapi.h>
#    endif

NRI_API void NRI_CALL nriSetThreadName(const char* name) {
    uint64_t tid = 0;
#    if (defined __linux__)
    tid = syscall(SYS_gettid);
#    elif (defined __APPLE__)
    pthread_threadid_np(nullptr, &tid);
#    elif (defined __ANDROID__)
    tid = gettid();
#    elif (defined _WIN64)
    tid = GetCurrentThreadId();
#    endif

    nvtxNameOsThreadA((uint32_t)tid, name);
}

#else

NRI_API void NRI_CALL nriSetThreadName(const char*) {
}

#endif

NRI_API Result NRI_CALL nriCreateDevice(const DeviceCreationDesc& deviceCreationDesc, Device*& device) {
    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

    DeviceCreationDesc modifiedDeviceCreationDesc = deviceCreationDesc;
    CheckAndSetDefaultCallbacks(modifiedDeviceCreationDesc);

    Result deviceFaultInfoResult = EnableDeviceFaultInfo(modifiedDeviceCreationDesc);
    if (deviceFaultInfoResult != Result::SUCCESS)
        ReportDeviceFaultInfoEnableWarning(modifiedDeviceCreationDesc, deviceFaultInfoResult);

    // Valid adapter expected (take 1st compatible)
    uint32_t adapterDescNum = ADAPTER_MAX_NUM;
    std::array<AdapterDesc, ADAPTER_MAX_NUM> adapterDescs = {};
    AdapterDesc adapterDesc = {};
    if (!modifiedDeviceCreationDesc.adapterDesc) {
        nriEnumerateAdapters(adapterDescs.data(), adapterDescNum);
        for (uint32_t i = 0; i < adapterDescNum; i++) {
            if (adapterDescs[i].supportedGraphicsAPIs & modifiedDeviceCreationDesc.graphicsAPI) {
                adapterDesc = adapterDescs[i];
                break;
            }
        }
        modifiedDeviceCreationDesc.adapterDesc = &adapterDesc;
    }

    if ((modifiedDeviceCreationDesc.adapterDesc->supportedGraphicsAPIs & modifiedDeviceCreationDesc.graphicsAPI) == 0)
        return Result::UNSUPPORTED;

    // Valid queue families expected
    QueueFamilyDesc qraphicsQueue = {};
    qraphicsQueue.queueNum = 1;
    qraphicsQueue.queueType = QueueType::GRAPHICS;

    if (!modifiedDeviceCreationDesc.queueFamilyNum) {
        modifiedDeviceCreationDesc.queueFamilyNum = 1;
        modifiedDeviceCreationDesc.queueFamilies = &qraphicsQueue;
    }

    for (uint32_t i = 0; i < modifiedDeviceCreationDesc.queueFamilyNum; i++) {
        QueueFamilyDesc& queueFamily = (QueueFamilyDesc&)modifiedDeviceCreationDesc.queueFamilies[i];

        uint32_t queueType = (uint32_t)queueFamily.queueType;
        if (queueType >= (uint32_t)QueueType::MAX_NUM)
            return Result::INVALID_ARGUMENT;

        uint32_t supportedQueueNum = modifiedDeviceCreationDesc.adapterDesc->queueNum[queueType];
        if (queueFamily.queueNum > supportedQueueNum)
            queueFamily.queueNum = supportedQueueNum;
    }

#if NRI_ENABLE_NONE_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::NONE)
        result = CreateDeviceNONE(modifiedDeviceCreationDesc, deviceImpl);
#endif

#if NRI_ENABLE_D3D11_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::D3D11)
        result = CreateDeviceD3D11(modifiedDeviceCreationDesc, {}, deviceImpl);
#endif

#if NRI_ENABLE_D3D12_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::D3D12)
        result = CreateDeviceD3D12(modifiedDeviceCreationDesc, {}, deviceImpl);
#endif

#if NRI_ENABLE_VK_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::VK)
        result = CreateDeviceVK(modifiedDeviceCreationDesc, {}, deviceImpl);
#endif

#if NRI_ENABLE_WGPU_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::WGPU)
        result = CreateDeviceWGPU(modifiedDeviceCreationDesc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(modifiedDeviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromD3D11Device(const DeviceCreationD3D11Desc& deviceCreationD3D11Desc, Device*& device) {
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D11;

    AdapterDesc adapterDesc = {};
    deviceCreationDesc.adapterDesc = &adapterDesc;

    // Copy what is possible to the main "desc"
    deviceCreationDesc.callbackInterface = deviceCreationD3D11Desc.callbackInterface;
    deviceCreationDesc.allocationCallbacks = deviceCreationD3D11Desc.allocationCallbacks;
    deviceCreationDesc.d3dShaderExtRegister = deviceCreationD3D11Desc.d3dShaderExtRegister;
    deviceCreationDesc.d3dZeroBufferSize = deviceCreationD3D11Desc.d3dZeroBufferSize;
    deviceCreationDesc.enableNRIValidation = deviceCreationD3D11Desc.enableNRIValidation;
    deviceCreationDesc.enableD3D11CommandBufferEmulation = deviceCreationD3D11Desc.enableD3D11CommandBufferEmulation;

    CheckAndSetDefaultCallbacks(deviceCreationDesc);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if NRI_ENABLE_D3D11_SUPPORT
    { // Valid adapter expected
        if (!deviceCreationD3D11Desc.d3d11Device)
            return Result::INVALID_ARGUMENT;

        ComPtr<IDXGIDevice> dxgiDevice;
        HRESULT hr = deviceCreationD3D11Desc.d3d11Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
        if (SUCCEEDED(hr)) {
            ComPtr<IDXGIAdapter> adapter;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr)) {
                DXGI_ADAPTER_DESC desc = {};
                hr = adapter->GetDesc(&desc);
                if (SUCCEEDED(hr)) {
                    uint32_t unused = 0;
                    UpdateAdaptersD3D(&adapterDesc, unused, &desc.AdapterLuid);
                }
            }
        }
    }

    // Valid queue families expected
    QueueFamilyDesc qraphicsQueue = {};
    qraphicsQueue.queueNum = 1;
    qraphicsQueue.queueType = QueueType::GRAPHICS;

    if (!deviceCreationDesc.queueFamilyNum) {
        deviceCreationDesc.queueFamilyNum = 1;
        deviceCreationDesc.queueFamilies = &qraphicsQueue;
    }

    result = CreateDeviceD3D11(deviceCreationDesc, deviceCreationD3D11Desc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromD3D12Device(const DeviceCreationD3D12Desc& deviceCreationD3D12Desc, Device*& device) {
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D12;

    AdapterDesc adapterDesc = {};
    deviceCreationDesc.adapterDesc = &adapterDesc;

    // Copy what is possible to the main "desc"
    deviceCreationDesc.callbackInterface = deviceCreationD3D12Desc.callbackInterface;
    deviceCreationDesc.allocationCallbacks = deviceCreationD3D12Desc.allocationCallbacks;
    deviceCreationDesc.d3dShaderExtRegister = deviceCreationD3D12Desc.d3dShaderExtRegister;
    deviceCreationDesc.d3dZeroBufferSize = deviceCreationD3D12Desc.d3dZeroBufferSize;
    deviceCreationDesc.enableNRIValidation = deviceCreationD3D12Desc.enableNRIValidation;
    deviceCreationDesc.enableMemoryZeroInitialization = deviceCreationD3D12Desc.enableMemoryZeroInitialization;
    deviceCreationDesc.disableD3D12EnhancedBarriers = deviceCreationD3D12Desc.disableD3D12EnhancedBarriers;

    CheckAndSetDefaultCallbacks(deviceCreationDesc);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if NRI_ENABLE_D3D12_SUPPORT
    // Valid adapter expected
    if (!deviceCreationD3D12Desc.d3d12Device)
        return Result::INVALID_ARGUMENT;

    LUID luid = deviceCreationD3D12Desc.d3d12Device->GetAdapterLuid();
    uint32_t unused = 0;
    UpdateAdaptersD3D(&adapterDesc, unused, &luid);

    // Valid queue families expected
    for (uint32_t i = 0; i < deviceCreationD3D12Desc.queueFamilyNum; i++) {
        QueueFamilyD3D12Desc& queueFamilyD3D12Desc = (QueueFamilyD3D12Desc&)deviceCreationD3D12Desc.queueFamilies[i];

        uint32_t queueType = (uint32_t)queueFamilyD3D12Desc.queueType;
        if (queueType >= (uint32_t)QueueType::MAX_NUM)
            return Result::INVALID_ARGUMENT;

        uint32_t supportedQueueNum = deviceCreationDesc.adapterDesc->queueNum[queueType];
        if (queueFamilyD3D12Desc.queueNum > supportedQueueNum)
            queueFamilyD3D12Desc.queueNum = supportedQueueNum;
    }

    result = CreateDeviceD3D12(deviceCreationDesc, deviceCreationD3D12Desc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromVKDevice(const DeviceCreationVKDesc& deviceCreationVKDesc, Device*& device) {
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.graphicsAPI = GraphicsAPI::VK;

    AdapterDesc adapterDesc = {};
    deviceCreationDesc.adapterDesc = &adapterDesc;

    // Copy what is possible to the main "desc"
    deviceCreationDesc.callbackInterface = deviceCreationVKDesc.callbackInterface;
    deviceCreationDesc.allocationCallbacks = deviceCreationVKDesc.allocationCallbacks;
    deviceCreationDesc.enableNRIValidation = deviceCreationVKDesc.enableNRIValidation;
    deviceCreationDesc.enableMemoryZeroInitialization = deviceCreationVKDesc.enableMemoryZeroInitialization;
    deviceCreationDesc.vkBindingOffsets = deviceCreationVKDesc.vkBindingOffsets;
    deviceCreationDesc.vkExtensions = deviceCreationVKDesc.vkExtensions;

    CheckAndSetDefaultCallbacks(deviceCreationDesc);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if NRI_ENABLE_VK_SUPPORT
    // Valid adapter expected
    if (!deviceCreationVKDesc.vkPhysicalDevice)
        return Result::INVALID_ARGUMENT;

    uint32_t unused = 0;
    UpdateAdaptersVK(&adapterDesc, unused, (VkPhysicalDevice)deviceCreationVKDesc.vkPhysicalDevice);

    // Valid queue families expected
    for (uint32_t i = 0; i < deviceCreationVKDesc.queueFamilyNum; i++) {
        QueueFamilyVKDesc& queueFamilyVKDesc = (QueueFamilyVKDesc&)deviceCreationVKDesc.queueFamilies[i];

        uint32_t queueType = (uint32_t)queueFamilyVKDesc.queueType;
        if (queueType >= (uint32_t)QueueType::MAX_NUM)
            return Result::INVALID_ARGUMENT;

        uint32_t supportedQueueNum = deviceCreationDesc.adapterDesc->queueNum[queueType];
        if (queueFamilyVKDesc.queueNum > supportedQueueNum)
            queueFamilyVKDesc.queueNum = supportedQueueNum;
    }

    result = CreateDeviceVK(deviceCreationDesc, deviceCreationVKDesc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API void NRI_CALL nriDestroyDevice(Device* device) {
    if (device)
        ((DeviceBase*)device)->Destruct();
}

NRI_API Format NRI_CALL nriConvertVKFormatToNRI(uint32_t vkFormat) {
    return VKFormatToNRIFormat(vkFormat);
}

NRI_API Format NRI_CALL nriConvertDXGIFormatToNRI(uint32_t dxgiFormat) {
    return DXGIFormatToNRIFormat(dxgiFormat);
}

NRI_API uint32_t NRI_CALL nriConvertNRIFormatToVK(Format format) {
    MaybeUnused(format);

#if NRI_ENABLE_VK_SUPPORT
    return NRIFormatToVKFormat(format);
#else
    return 0;
#endif
}

NRI_API uint32_t NRI_CALL nriConvertNRIFormatToDXGI(Format format) {
    MaybeUnused(format);

#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)
    return NRIFormatToDXGIFormat(format);
#else
    return 0;
#endif
}

NRI_API const FormatProps* NRI_CALL nriGetFormatProps(Format format) {
    return &GetFormatProps(format);
}

NRI_API const char* NRI_CALL nriGetGraphicsAPIString(GraphicsAPI graphicsAPI) {
    switch (graphicsAPI) {
        case GraphicsAPI::NONE:
            return "NONE";
        case GraphicsAPI::D3D11:
            return "D3D11";
        case GraphicsAPI::D3D12:
            return "D3D12";
        case GraphicsAPI::VK:
            return "VK";
        case GraphicsAPI::WGPU:
            return "WGPU";
        default:
            return "UNKNOWN";
    }
}

NRI_API Result NRI_CALL nriEnumerateAdapters(AdapterDesc* outAdapterDescs, uint32_t& outAdapterDescNum) {
    std::array<AdapterDesc, ADAPTER_MAX_NUM> adapterDescs = {};
    uint32_t adapterDescNum = 0;

#if NRI_ENABLE_VK_SUPPORT
    UpdateAdaptersVK(adapterDescs.data(), adapterDescNum, nullptr);
#endif

#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)
    UpdateAdaptersD3D(adapterDescs.data(), adapterDescNum, nullptr);
#endif

#if NRI_ENABLE_WGPU_SUPPORT
    UpdateAdaptersWGPU(adapterDescs.data(), adapterDescNum);
#endif

#if NRI_ENABLE_NONE_SUPPORT
    if (!adapterDescNum) {
        AdapterDesc& adapterDesc = adapterDescs[adapterDescNum++];

        adapterDesc.videoMemorySize = 128ull << 30;
        adapterDesc.sharedSystemMemorySize = 128ull << 30;

        for (uint32_t i = 0; i < GetCountOf(adapterDesc.queueNum); i++)
            adapterDesc.queueNum[i] = 32;

        strncpy(adapterDesc.name, "NONE", sizeof(adapterDesc.name));
    }
#endif

    // Sort by video memory size and arhitecture (DISCRETE first)
    qsort(adapterDescs.data(), adapterDescNum, sizeof(adapterDescs[0]), SortAdapters);

    // Copy to output
    if (outAdapterDescs) {
        outAdapterDescNum = std::min(outAdapterDescNum, adapterDescNum);
        for (uint32_t i = 0; i < outAdapterDescNum; i++) {
            outAdapterDescs[i] = adapterDescs[i];
#if NRI_ENABLE_NONE_SUPPORT
            outAdapterDescs[i].supportedGraphicsAPIs |= GraphicsAPI::NONE;
#endif
        }
    } else
        outAdapterDescNum = adapterDescNum;

    return outAdapterDescNum == 0 ? Result::UNSUPPORTED : Result::SUCCESS;
}

NRI_API void NRI_CALL nriReportLiveObjects() {
#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)
    ComPtr<IDXGIDebug1> pDebug;
    HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug));
    if (SUCCEEDED(hr))
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)((uint32_t)DXGI_DEBUG_RLO_DETAIL | (uint32_t)DXGI_DEBUG_RLO_IGNORE_INTERNAL));
#endif
}

NRI_API Result NRI_CALL nriReportDeviceFaultInfo(const Device& device) {
    const DeviceBase& deviceBase = (DeviceBase&)device;
    GraphicsAPI graphicsAPI = deviceBase.GetDesc().graphicsAPI;

#if NRI_ENABLE_D3D12_SUPPORT
    if (graphicsAPI == GraphicsAPI::D3D12) {
        CoreInterface core = {};
        Result result = deviceBase.FillFunctionTable(core);
        if (result != Result::SUCCESS || !core.GetDeviceNativeObject)
            return Result::UNSUPPORTED;

        ID3D12Device* nativeDevice = (ID3D12Device*)core.GetDeviceNativeObject(&device);
        if (!nativeDevice)
            return Result::INVALID_ARGUMENT;

        HRESULT deviceRemovedReason = nativeDevice->GetDeviceRemovedReason();
        NRI_REPORT_DEVICE_FAULT_INFO(deviceBase, "[D3D12] GetDeviceRemovedReason: 0x%08X, result=%u", (uint32_t)deviceRemovedReason, (uint32_t)GetResultFromHRESULT(deviceRemovedReason));
        ReportD3D12DeviceRemovedExtendedData(nativeDevice, deviceBase);

        return Result::SUCCESS;
    }
#endif

#if NRI_ENABLE_VK_SUPPORT && defined(VK_KHR_device_fault)
    if (graphicsAPI == GraphicsAPI::VK)
        return ReportVKDeviceFaultInfo(device, deviceBase);
#endif

    return Result::UNSUPPORTED;
}

#undef NRI_REPORT_DEVICE_FAULT_INFO
