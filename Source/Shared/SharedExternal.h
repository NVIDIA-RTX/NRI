// © 2021 NVIDIA Corporation

#pragma once

#include <cassert>   // assert
#include <cinttypes> // PRIu64
#include <cstring>   // memcpy
#include <numeric>   // lcm

#include <array>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)
#    include <dxgi1_6.h>
#else
typedef uint32_t DXGI_FORMAT;
#endif

// IMPORTANT: "SharedExternal.h" must be included after inclusion of "windows.h" (can be implicit) because ERROR gets undef-ed below
#include "NRI.h"
#include "NRI.hlsl"

#include "Extensions/NRIDeviceCreation.h"
#include "Extensions/NRIHelper.h"
#include "Extensions/NRIImgui.h"
#include "Extensions/NRILowLatency.h"
#include "Extensions/NRIMeshShader.h"
#include "Extensions/NRIRayTracing.h"
#include "Extensions/NRIStreamer.h"
#include "Extensions/NRISwapChain.h"
#include "Extensions/NRIUpscaler.h"
#include "Extensions/NRIWrapperD3D11.h"
#include "Extensions/NRIWrapperD3D12.h"
#include "Extensions/NRIWrapperVK.h"

#include "Lock.h"

// ComPtr
#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)

struct IUnknown;

// This struct acts as a smart pointer for IUnknown pointers making sure to call AddRef() and Release() as needed.
template <typename T>
struct ComPtr {
    inline ComPtr(T* lComPtr = nullptr)
        : m_ComPtr(lComPtr) {
        static_assert(std::is_base_of<IUnknown, T>::value, "T needs to be IUnknown based");

        if (m_ComPtr)
            m_ComPtr->AddRef();
    }

    inline ComPtr(const ComPtr<T>& lComPtrObj) {
        static_assert(std::is_base_of<IUnknown, T>::value, "T needs to be IUnknown based");

        m_ComPtr = lComPtrObj.m_ComPtr;

        if (m_ComPtr)
            m_ComPtr->AddRef();
    }

    inline ComPtr(ComPtr<T>&& lComPtrObj) {
        m_ComPtr = lComPtrObj.m_ComPtr;
        lComPtrObj.m_ComPtr = nullptr;
    }

    inline T* operator=(T* lComPtr) {
        if (m_ComPtr)
            m_ComPtr->Release();

        m_ComPtr = lComPtr;

        if (m_ComPtr)
            m_ComPtr->AddRef();

        return m_ComPtr;
    }

    inline T* operator=(const ComPtr<T>& lComPtrObj) {
        if (m_ComPtr)
            m_ComPtr->Release();

        m_ComPtr = lComPtrObj.m_ComPtr;

        if (m_ComPtr)
            m_ComPtr->AddRef();

        return m_ComPtr;
    }

    inline ~ComPtr() {
        if (m_ComPtr) {
            m_ComPtr->Release();
            m_ComPtr = nullptr;
        }
    }

    inline T** operator&() {
        // The assert on operator& usually indicates a bug. Could be a potential memory leak.
        // If this really what is needed, however, use GetInterface() explicitly.
        assert(m_ComPtr == nullptr);
        return &m_ComPtr;
    }

    inline operator T*() const {
        return m_ComPtr;
    }

    inline T* GetInterface() const {
        return m_ComPtr;
    }

    inline T& operator*() const {
        return *m_ComPtr;
    }

    inline T* operator->() const {
        return m_ComPtr;
    }

    inline bool operator!() const {
        return (nullptr == m_ComPtr);
    }

    inline bool operator<(T* lComPtr) const {
        return m_ComPtr < lComPtr;
    }

    inline bool operator!=(T* lComPtr) const {
        return !operator==(lComPtr);
    }

    inline bool operator==(T* lComPtr) const {
        return m_ComPtr == lComPtr;
    }

protected:
    T* m_ComPtr;
};

#endif

// Prerequisites // TODO: improve
template <typename T>
inline T Align(T x, size_t alignment) {
    return (T)((size_t(x) + alignment - 1) & ~(alignment - 1));
}

template <typename... Args>
constexpr void MaybeUnused([[maybe_unused]] const Args&... args) {
}

// Allocator
template <typename T>
struct StdAllocator {
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef std::true_type propagate_on_container_move_assignment;
    typedef std::false_type is_always_equal;

    StdAllocator(const nri::AllocationCallbacks& allocationCallbacks)
        : m_Interface(allocationCallbacks) {
    }

    StdAllocator(const StdAllocator<T>& allocator)
        : m_Interface(allocator.GetInterface()) {
    }

    template <class U>
    StdAllocator(const StdAllocator<U>& allocator)
        : m_Interface(allocator.GetInterface()) {
    }

    StdAllocator<T>& operator=(const StdAllocator<T>& allocator) {
        m_Interface = allocator.GetInterface();
        return *this;
    }

    T* allocate(size_t n) noexcept {
        return (T*)m_Interface.Allocate(m_Interface.userArg, n * sizeof(T), alignof(T));
    }

    void deallocate(T* memory, size_t) noexcept {
        m_Interface.Free(m_Interface.userArg, memory);
    }

    const nri::AllocationCallbacks& GetInterface() const {
        return m_Interface;
    }

    template <typename U>
    using other = StdAllocator<U>;

private:
    const nri::AllocationCallbacks& m_Interface = {}; // IMPORTANT: yes, it's a pointer to the real location (DeviceBase)
};

template <typename T>
bool operator==(const StdAllocator<T>& left, const StdAllocator<T>& right) {
    return left.GetInterface() == right.GetInterface();
}

template <typename T>
bool operator!=(const StdAllocator<T>& left, const StdAllocator<T>& right) {
    return !operator==(left, right);
}

// Types with "StdAllocator"
template <typename T>
using Vector = std::vector<T, StdAllocator<T>>;

template <typename U, typename T>
using UnorderedMap = std::unordered_map<U, T, std::hash<U>, std::equal_to<U>, StdAllocator<std::pair<const U, T>>>;

template <typename U, typename T>
using Map = std::map<U, T, std::less<U>, StdAllocator<std::pair<const U, T>>>;

using String = std::basic_string<char, std::char_traits<char>, StdAllocator<char>>;

// Scratch
#ifndef _WIN32
#    include <alloca.h>
#endif

constexpr size_t MAX_STACK_ALLOC_SIZE = 32 * 1024;

template <typename T>
class Scratch {
public:
    Scratch(const nri::AllocationCallbacks& allocator, T* mem, size_t num)
        : m_Allocator(allocator)
        , m_Mem(mem)
        , m_Num(num) {
        m_IsHeap = (num * sizeof(T) + alignof(T)) > MAX_STACK_ALLOC_SIZE;
    }

    ~Scratch() {
        if (m_IsHeap)
            m_Allocator.Free(m_Allocator.userArg, m_Mem);
    }

    inline operator T*() const {
        return m_Mem;
    }

    inline T& operator[](size_t i) const {
        assert(i < m_Num);
        return m_Mem[i];
    }

private:
    const nri::AllocationCallbacks& m_Allocator;
    T* m_Mem = nullptr;
    size_t m_Num = 0;
    bool m_IsHeap = false;
};

#define AllocateScratch(device, T, elementNum) \
    {(device).GetAllocationCallbacks(), \
        ((elementNum) * sizeof(T) + alignof(T)) > MAX_STACK_ALLOC_SIZE \
            ? (T*)(device).GetAllocationCallbacks().Allocate((device).GetAllocationCallbacks().userArg, (elementNum) * sizeof(T), alignof(T)) \
            : (T*)Align((elementNum) ? (T*)alloca(((elementNum) * sizeof(T) + alignof(T))) : nullptr, alignof(T)), \
        (elementNum)}

// Base classes
#include "DeviceBase.h"

// Macro stuff
#ifdef _WIN32
#    define FILE_SEPARATOR '\\'
#else
#    define FILE_SEPARATOR '/'
#endif

#ifdef NDEBUG
#    define CHECK(condition, message) MaybeUnused(condition)
#else
#    define CHECK(condition, message) assert((condition) && message)
#endif

#define NRI_INLINE inline // we want to inline all functions, which are actually wrappers for the interface functions

#define NRI_STRINGIFY_(token) #token
#define NRI_STRINGIFY(token)  NRI_STRINGIFY_(token)

// Message reporting
#define RETURN_ON_BAD_HRESULT(deviceBase, hr, funcName) \
    if (hr < 0) { \
        Result _result = GetResultFromHRESULT(hr); \
        (deviceBase)->ReportMessage(Message::ERROR, _result, __FILE__, __LINE__, funcName "(): failed, result = 0x%08X (%d)!", __FUNCTION__, hr, hr); \
        return _result; \
    }

#define RETURN_VOID_ON_BAD_HRESULT(deviceBase, hr, funcName) \
    if (hr < 0) { \
        Result _result = GetResultFromHRESULT(hr); \
        (deviceBase)->ReportMessage(Message::ERROR, _result, __FILE__, __LINE__, funcName "(): failed, result = 0x%08X (%d)!", __FUNCTION__, hr, hr); \
        return; \
    }

#define RETURN_ON_BAD_VKRESULT(deviceBase, vkResult, funcName) \
    if (vkResult < 0) { \
        Result _result = GetResultFromVkResult(vkResult); \
        (deviceBase)->ReportMessage(Message::ERROR, _result, __FILE__, __LINE__, funcName "(): failed, result = 0x%08X (%d)!", __FUNCTION__, vkResult, vkResult); \
        return _result; \
    }

#define RETURN_VOID_ON_BAD_VKRESULT(deviceBase, vkResult, funcName) \
    if (vkResult < 0) { \
        Result _result = GetResultFromVkResult(vkResult); \
        (deviceBase)->ReportMessage(Message::ERROR, _result, __FILE__, __LINE__, funcName "(): failed, result = 0x%08X (%d)!", __FUNCTION__, vkResult, vkResult); \
        return; \
    }

#define REPORT_ERROR_ON_BAD_NVAPI_STATUS(deviceBase, expression) \
    if ((expression) != 0) { \
        (deviceBase)->ReportMessage(Message::ERROR, Result::FAILURE, __FILE__, __LINE__, "%s: " NRI_STRINGIFY(expression) " failed!", __FUNCTION__); \
    }

#define RETURN_ON_FAILURE(deviceBase, condition, returnCode, format, ...) \
    if (!(condition)) { \
        (deviceBase)->ReportMessage(Message::ERROR, Result::FAILURE, __FILE__, __LINE__, "%s: " format, __FUNCTION__, ##__VA_ARGS__); \
        return returnCode; \
    }

#define REPORT_INFO(deviceBase, format, ...)    (deviceBase)->ReportMessage(Message::INFO, Result::SUCCESS, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define REPORT_WARNING(deviceBase, format, ...) (deviceBase)->ReportMessage(Message::WARNING, Result::SUCCESS, __FILE__, __LINE__, "%s(): " format, __FUNCTION__, ##__VA_ARGS__)
#define REPORT_ERROR(deviceBase, format, ...)   (deviceBase)->ReportMessage(Message::ERROR, Result::FAILURE, __FILE__, __LINE__, "%s(): " format, __FUNCTION__, ##__VA_ARGS__)

// Queue scores // TODO: improve?
#define GRAPHICS_QUEUE_SCORE ((graphics ? 100 : 0) + (compute ? 10 : 0) + (copy ? 10 : 0) + (sparse ? 5 : 0) + (videoDecode ? 2 : 0) + (videoEncode ? 2 : 0) + (protect ? 1 : 0) + (opticalFlow ? 1 : 0))
#define COMPUTE_QUEUE_SCORE  ((!graphics ? 10 : 0) + (compute ? 100 : 0) + (!copy ? 10 : 0) + (sparse ? 5 : 0) + (!videoDecode ? 2 : 0) + (!videoEncode ? 2 : 0) + (protect ? 1 : 0) + (!opticalFlow ? 1 : 0))
#define COPY_QUEUE_SCORE     ((!graphics ? 10 : 0) + (!compute ? 10 : 0) + (copy ? 100 * familyProps.queueCount : 0) + (sparse ? 5 : 0) + (!videoDecode ? 2 : 0) + (!videoEncode ? 2 : 0) + (protect ? 1 : 0) + (!opticalFlow ? 1 : 0))

// Array validation
#define VALIDATE_ARRAY(x)                 static_assert((size_t)x[x.size() - 1] != 0, "Some elements are missing in '" NRI_STRINGIFY(x) "'");
#define VALIDATE_ARRAY_BY_PTR(x)          static_assert(x[x.size() - 1] != nullptr, "Some elements are missing in '" NRI_STRINGIFY(x) "'");
#define VALIDATE_ARRAY_BY_FILED(x, field) static_assert(x[x.size() - 1].field != 0, "Some elements are missing in '" NRI_STRINGIFY(x) "'");

// Shared library
struct Library;
Library* LoadSharedLibrary(const char* path);
void* GetSharedLibraryFunction(Library& library, const char* name);
void UnloadSharedLibrary(Library& library);
extern const char* VULKAN_LOADER_NAME;

namespace nri {

// Consts
constexpr uint32_t NODE_MASK = 0x1;        // mGPU is not planned
constexpr uint32_t TIMEOUT_PRESENT = 1000; // 1 sec
constexpr uint32_t TIMEOUT_FENCE = 5000;   // 5 sec
constexpr uint64_t PRESENT_INDEX_BIT_NUM = 56ull;
constexpr uint32_t MAX_MESSAGE_LENGTH = 2048;
constexpr uint64_t VMA_PREFERRED_BLOCK_SIZE = 64 * 1024 * 1024;
constexpr uint32_t ROOT_SIGNATURE_DWORD_NUM = 64; // https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signature-limits
constexpr uint32_t ZERO_BUFFER_DEFAULT_SIZE = 4 * 1024 * 1024;

// Helpers
template <typename T, uint32_t N>
constexpr uint32_t GetCountOf(T const (&)[N]) {
    return N;
}

template <typename T, size_t N>
constexpr uint32_t GetCountOf(const std::array<T, N>& v) {
    return (uint32_t)v.size();
}

template <typename T, typename... Args>
constexpr void Construct(T* objects, size_t number, Args&&... args) {
    for (size_t i = 0; i < number; i++)
        new (objects + i) T(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
inline T* Allocate(const AllocationCallbacks& allocationCallbacks, Args&&... args) {
    T* object = (T*)allocationCallbacks.Allocate(allocationCallbacks.userArg, sizeof(T), alignof(T));
    if (object)
        new (object) T(std::forward<Args>(args)...);

    return object;
}

template <typename T>
inline void Destroy(const AllocationCallbacks& allocationCallbacks, T* object) {
    if (object) {
        object->~T();
        allocationCallbacks.Free(allocationCallbacks.userArg, object);
    }
}

template <typename T>
inline void Destroy(T* object) {
    if (object) {
        object->~T();

        const auto& allocationCallbacks = ((DeviceBase&)(object->GetDevice())).GetAllocationCallbacks();
        allocationCallbacks.Free(allocationCallbacks.userArg, object);
    }
}

constexpr uint64_t MsToUs(uint32_t x) {
    return x * 1000000ull;
}

constexpr void ReturnVoid() {
}

// Format conversion
struct DxgiFormat {
    DXGI_FORMAT typeless;
    DXGI_FORMAT typed;
};

const DxgiFormat& GetDxgiFormat(Format format);
const FormatProps& GetFormatProps(Format format);

Format DXGIFormatToNRIFormat(uint32_t dxgiFormat);
Format VKFormatToNRIFormat(uint32_t vkFormat);

uint32_t NRIFormatToDXGIFormat(Format format);
uint32_t NRIFormatToVKFormat(Format format);

// Misc
Result GetResultFromHRESULT(long result);

inline Vendor GetVendorFromID(uint32_t vendorID) {
    switch (vendorID) {
        case 0x10DE:
            return Vendor::NVIDIA;
        case 0x1002:
            return Vendor::AMD;
        case 0x8086:
            return Vendor::INTEL;
    }

    return Vendor::UNKNOWN;
}

inline Dim_t GetDimension(GraphicsAPI api, const TextureDesc& textureDesc, Dim_t dimensionIndex, Dim_t mip) {
    assert(dimensionIndex < 3);

    Dim_t dim = textureDesc.depth;
    if (dimensionIndex == 0)
        dim = textureDesc.width;
    else if (dimensionIndex == 1)
        dim = textureDesc.height;

    dim = (Dim_t)std::max(dim >> mip, 1);

    // TODO: VK doesn't require manual alignment, but probably we should use it here and during texture creation
    if (api != GraphicsAPI::VK)
        dim = Align(dim, dimensionIndex < 2 ? GetFormatProps(textureDesc.format).blockWidth : 1);

    return dim;
}

inline bool IsDepthBiasEnabled(const DepthBiasDesc& depthBiasDesc) {
    return depthBiasDesc.constant != 0.0f || depthBiasDesc.slope != 0.0f;
}

inline TextureDesc FixTextureDesc(const TextureDesc& textureDesc) {
    TextureDesc desc = textureDesc;
    desc.height = std::max(desc.height, (Dim_t)1);
    desc.depth = std::max(desc.depth, (Dim_t)1);
    desc.mipNum = std::max(desc.mipNum, (Dim_t)1);
    desc.layerNum = std::max(desc.layerNum, (Dim_t)1);
    desc.sampleNum = std::max(desc.sampleNum, (Sample_t)1);

    return desc;
}

inline Uid_t ConstructUid(uint8_t luid[8], uint8_t uuid[16], bool isLuidValid) {
    Uid_t out = {};

    if (isLuidValid)
        out.low = *(uint64_t*)luid;
    else {
        out.low = *(uint64_t*)uuid;
        out.high = *(uint64_t*)(uuid + 8);
    }

    return out;
}

inline bool CompareUid(const Uid_t& a, const Uid_t& b) {
    return a.low == b.low && a.high == b.high;
}

// Strings
void ConvertCharToWchar(const char* in, wchar_t* out, size_t outLen);
void ConvertWcharToChar(const wchar_t* in, char* out, size_t outLen);

// Swap chain ID
uint64_t GetSwapChainId();

inline uint64_t GetPresentIndex(uint64_t presentId) {
    return presentId & ((1ull << PRESENT_INDEX_BIT_NUM) - 1ull);
}

// Windows/D3D specific
#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)

#    define SET_D3D_DEBUG_OBJECT_NAME(obj, name) \
        if (obj) \
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)std::strlen(name), name)

bool HasOutput();
Result QueryVideoMemoryInfoDXGI(uint64_t luid, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo);

struct DisplayDescHelper {
    Result GetDisplayDesc(void* hwnd, DisplayDesc& displayDesc);

    ComPtr<IDXGIFactory2> m_DxgiFactory2;
    DisplayDesc m_DisplayDesc = {};
    bool m_HasDisplayDesc = false;
};

#else

struct DisplayDescHelper {
    inline Result GetDisplayDesc(void*, DisplayDesc& displayDesc) { // TODO: non-Windows - query somehow? Windows - allow DXGI usage even if D3D is disabled?
        displayDesc = {};
        displayDesc.sdrLuminance = 80.0f;
        displayDesc.maxLuminance = 80.0f;

        return Result::UNSUPPORTED;
    }
};

#endif

} // namespace nri
