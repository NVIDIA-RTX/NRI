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

#ifndef _WIN32
#    include <alloca.h>
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
#include "Extensions/NRIVideo.h"
#include "Extensions/NRIWrapperD3D11.h"
#include "Extensions/NRIWrapperD3D12.h"
#include "Extensions/NRIWrapperVK.h"

#include "Lock.h"
#include "VideoAV1.h"
#include "VideoAnnexB.h"

// NRI default settings (if not provided in "NRIConfig.h")
#include "../NRIConfig.h"

#ifndef NRI_TIMEOUT_PRESENT
#    define NRI_TIMEOUT_PRESENT 1000u // 1 sec
#endif

#ifndef NRI_TIMEOUT_FENCE
#    define NRI_TIMEOUT_FENCE 5000u // 5 sec
#endif

#ifndef NRI_MAX_MESSAGE_LENGTH
#    define NRI_MAX_MESSAGE_LENGTH 2048u // 2 Kb
#endif

#ifndef NRI_ZERO_BUFFER_SIZE
#    define NRI_ZERO_BUFFER_SIZE 4194304u // 4 Mb
#endif

#ifndef NRI_MAX_STACK_ALLOC_SIZE
#    define NRI_MAX_STACK_ALLOC_SIZE 32768u // 32 Kb
#endif

#ifndef NRI_FILE_SEPARATOR
#    ifdef _WIN32
#        define NRI_FILE_SEPARATOR '\\'
#    else
#        define NRI_FILE_SEPARATOR '/'
#    endif
#endif

#ifndef NRI_CHECK
#    ifdef NDEBUG
#        define NRI_CHECK(condition, message)
#    else
#        define NRI_CHECK(condition, message) assert((condition) && message)
#    endif
#endif

#ifndef NRI_INLINE
#    define NRI_INLINE inline
#endif

// D3D12MA default settings (if not provided in "NRIConfig.h")
#ifndef D3D12MA_DEBUG_LOG
#    define D3D12MA_DEBUG_LOG(format, ...) \
        do { \
            wprintf(format, __VA_ARGS__); \
            wprintf(L"\n"); \
        } while (false)
#endif

#ifndef D3D12MA_DEFAULT_BLOCK_SIZE
#    define D3D12MA_DEFAULT_BLOCK_SIZE 67108864u // 64 Mb
#endif

#ifndef D3D12MA_ASSERT
#    define D3D12MA_ASSERT(cond) NRI_CHECK(cond, "D3D12MA assert failed!")
#endif

#ifndef D3D12MA_HEAVY_ASSERT
#    define D3D12MA_HEAVY_ASSERT(expr)
#endif

// VMA default settings (if not provided in "NRIConfig.h")
#ifndef VMA_DEBUG_LOG_FORMAT
#    define VMA_DEBUG_LOG_FORMAT(format, ...) \
        do { \
            printf((format), __VA_ARGS__); \
            printf("\n"); \
        } while (false)
#endif

#ifndef VMA_DEFAULT_LARGE_HEAP_BLOCK_SIZE
#    define VMA_DEFAULT_LARGE_HEAP_BLOCK_SIZE 67108864u // 64 Mb
#endif

#ifndef VMA_ASSERT
#    define VMA_ASSERT(expr) NRI_CHECK(expr, "VMA assert failed!")
#endif

#ifndef VMA_ASSERT_LEAK
#    define VMA_ASSERT_LEAK(expr) VMA_ASSERT(expr)
#endif

#ifndef VMA_DEBUG_LOG
#    define VMA_DEBUG_LOG(str) VMA_DEBUG_LOG_FORMAT("%s", (str))
#endif

#ifndef VMA_LEAK_LOG_FORMAT
#    define VMA_LEAK_LOG_FORMAT(format, ...) VMA_DEBUG_LOG_FORMAT(format, __VA_ARGS__)
#endif

#ifndef VMA_HEAVY_ASSERT
#    define VMA_HEAVY_ASSERT(expr)
#endif

// ComPtr
#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)

struct IUnknown;

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

// Macro stuff
#define NRI_STRINGIFY_(token) #token
#define NRI_STRINGIFY(token)  NRI_STRINGIFY_(token)

#if defined(_WIN32)
#    define NRI_VULKAN_LOADER_NAME "vulkan-1.dll"
#elif defined(__APPLE__)
#    define NRI_VULKAN_LOADER_NAME "libvulkan.1.dylib"
#elif defined(__ANDROID__)
#    define NRI_VULKAN_LOADER_NAME "libvulkan.so"
#else
#    define NRI_VULKAN_LOADER_NAME "libvulkan.so.1"
#endif

// Message reporting
#define NRI_RETURN_ON_BAD_HRESULT(deviceBase, hr, funcName) \
    if (hr < 0) { \
        Result _result = GetResultFromHRESULT(hr); \
        (deviceBase)->ReportMessage(Message::ERROR, _result, __FILE__, __LINE__, funcName "(): failed, result = 0x%08X (%d)!", hr, hr); \
        return _result; \
    }

#define NRI_RETURN_VOID_ON_BAD_HRESULT(deviceBase, hr, funcName) \
    if (hr < 0) { \
        Result _result = GetResultFromHRESULT(hr); \
        (deviceBase)->ReportMessage(Message::ERROR, _result, __FILE__, __LINE__, funcName "(): failed, result = 0x%08X (%d)!", hr, hr); \
        return; \
    }

#define NRI_RETURN_ON_BAD_VKRESULT(deviceBase, vkResult, funcName) \
    if (vkResult < 0) { \
        Result _result = GetResultFromVkResult(vkResult); \
        (deviceBase)->ReportMessage(Message::ERROR, _result, __FILE__, __LINE__, funcName "(): failed, result = 0x%08X (%d)!", vkResult, vkResult); \
        return _result; \
    }

#define NRI_RETURN_VOID_ON_BAD_VKRESULT(deviceBase, vkResult, funcName) \
    if (vkResult < 0) { \
        Result _result = GetResultFromVkResult(vkResult); \
        (deviceBase)->ReportMessage(Message::ERROR, _result, __FILE__, __LINE__, funcName "(): failed, result = 0x%08X (%d)!", vkResult, vkResult); \
        return; \
    }

#define NRI_REPORT_ERROR_ON_BAD_NVAPI_STATUS(deviceBase, expression) \
    if ((expression) != 0) { \
        (deviceBase)->ReportMessage(Message::ERROR, Result::FAILURE, __FILE__, __LINE__, "%s: " NRI_STRINGIFY(expression) " failed!", __FUNCTION__); \
    }

#define NRI_RETURN_ON_FAILURE(deviceBase, condition, returnCode, format, ...) \
    if (!(condition)) { \
        (deviceBase)->ReportMessage(Message::ERROR, Result::FAILURE, __FILE__, __LINE__, "%s: " format, __FUNCTION__, ##__VA_ARGS__); \
        return returnCode; \
    }

#define NRI_REPORT_INFO(deviceBase, format, ...)    (deviceBase)->ReportMessage(Message::INFO, Result::SUCCESS, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define NRI_REPORT_WARNING(deviceBase, format, ...) (deviceBase)->ReportMessage(Message::WARNING, Result::SUCCESS, __FILE__, __LINE__, "%s(): " format, __FUNCTION__, ##__VA_ARGS__)
#define NRI_REPORT_ERROR(deviceBase, format, ...)   (deviceBase)->ReportMessage(Message::ERROR, Result::FAILURE, __FILE__, __LINE__, "%s(): " format, __FUNCTION__, ##__VA_ARGS__)

// Array validation
#define NRI_VALIDATE_ARRAY(x)                 static_assert((size_t)x[x.size() - 1] != 0, "Some elements are missing in '" NRI_STRINGIFY(x) "'");
#define NRI_VALIDATE_ARRAY_BY_PTR(x)          static_assert(x[x.size() - 1] != nullptr, "Some elements are missing in '" NRI_STRINGIFY(x) "'");
#define NRI_VALIDATE_ARRAY_BY_FIELD(x, field) static_assert(x[x.size() - 1].field != 0, "Some elements are missing in '" NRI_STRINGIFY(x) "'");

// D3D
#define NRI_SET_D3D_DEBUG_OBJECT_NAME(obj, name) \
    if (obj) \
    obj->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)std::strlen(name), name)

// clang-format off
#define NRI_ALLOCATE_SCRATCH(device, T, elementNum) { \
        (device).GetAllocationCallbacks(), \
        !(elementNum) ? nullptr : ( \
            ((elementNum) * sizeof(T) + alignof(T)) > NRI_MAX_STACK_ALLOC_SIZE \
                ? (T*)(device).GetAllocationCallbacks().Allocate((device).GetAllocationCallbacks().userArg, (elementNum) * sizeof(T), alignof(T)) \
                : (T*)Align((T*)alloca((elementNum) * sizeof(T) + alignof(T)), alignof(T)) \
        ), \
        (elementNum) \
    }
// clang-format on

namespace nri {

// Internal consts
constexpr uint32_t NODE_MASK = 0x1;               // mGPU is not planned
constexpr uint32_t ROOT_SIGNATURE_DWORD_NUM = 64; // https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signature-limits
constexpr uint64_t PRESENT_INDEX_BIT_NUM = 56ull;

// Scratch
template <typename T>
class Scratch {
public:
    Scratch(const AllocationCallbacks& allocator, T* mem, size_t num)
        : m_Allocator(allocator)
        , m_Mem(mem)
        , m_Num(num) {
        m_IsHeap = (num * sizeof(T) + alignof(T)) > NRI_MAX_STACK_ALLOC_SIZE;
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
    const AllocationCallbacks& m_Allocator;
    T* m_Mem = nullptr;
    size_t m_Num = 0;
    bool m_IsHeap = false;
};

// Shared library
struct Library;

Library* LoadSharedLibrary(const char* path);
void* GetSharedLibraryFunction(Library& library, const char* name);
void UnloadSharedLibrary(Library& library);

// Helpers
template <typename T>
inline T Align(T x, size_t alignment) {
    return (T)((size_t(x) + alignment - 1) & ~(alignment - 1));
}

template <typename... Args>
constexpr void MaybeUnused([[maybe_unused]] const Args&... args) {
}

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

constexpr uint64_t MsToUs(uint32_t x) {
    return x * 1000000ull;
}

constexpr void ReturnVoid() {
}

inline bool IsAligned(uint64_t value, uint64_t alignment) {
    return alignment <= 1 || value % alignment == 0;
}

// Allocator
template <typename T>
struct StdAllocator {
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef std::true_type propagate_on_container_move_assignment;
    typedef std::false_type is_always_equal;

    StdAllocator(const AllocationCallbacks& allocationCallbacks)
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

    const AllocationCallbacks& GetInterface() const {
        return m_Interface;
    }

    template <typename U>
    using other = StdAllocator<U>;

private:
    const AllocationCallbacks& m_Interface = {}; // IMPORTANT: yes, it's a pointer to the real location (DeviceBase)
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

// VK related
#if NRI_ENABLE_VK_SUPPORT

struct QueueFamilyProps {
    uint32_t queueCount;
    uint32_t videoDecodeCodecNum;
    uint32_t videoEncodeCodecNum;
    bool graphics;
    bool compute;
    bool copy;
    bool sparse;
    bool videoDecode;
    bool videoEncode;
    bool protect;
    bool opticalFlow;
};

inline QueueType TrySelectPreferredQueueType(const QueueFamilyProps& props, std::array<uint32_t, (size_t)QueueType::MAX_NUM>& scores) {
    { // Prefer as much features as possible
        size_t index = (size_t)QueueType::GRAPHICS;
        uint32_t score = ((props.graphics ? 100 : 0) + (props.compute ? 10 : 0) + (props.copy ? 10 : 0) + (props.sparse ? 5 : 0) + (props.videoDecode ? 2 : 0) + (props.videoEncode ? 2 : 0) + (props.protect ? 1 : 0) + (props.opticalFlow ? 1 : 0));

        if (props.graphics && score > scores[index]) {
            scores[index] = score;
            return QueueType::GRAPHICS;
        }
    }

    { // Prefer compute-only
        size_t index = (size_t)QueueType::COMPUTE;
        uint32_t score = ((!props.graphics ? 10 : 0) + (props.compute ? 100 : 0) + (!props.copy ? 10 : 0) + (props.sparse ? 5 : 0) + (!props.videoDecode ? 2 : 0) + (!props.videoEncode ? 2 : 0) + (props.protect ? 1 : 0) + (!props.opticalFlow ? 1 : 0));

        if (props.compute && score > scores[index]) {
            scores[index] = score;
            return QueueType::COMPUTE;
        }
    }

    { // Prefer copy-only
        size_t index = (size_t)QueueType::COPY;
        uint32_t score = ((!props.graphics ? 10 : 0) + (!props.compute ? 10 : 0) + (props.copy ? 100 * props.queueCount : 0) + (props.sparse ? 5 : 0) + (!props.videoDecode ? 2 : 0) + (!props.videoEncode ? 2 : 0) + (props.protect ? 1 : 0) + (!props.opticalFlow ? 1 : 0));

        if (props.copy && score > scores[index]) {
            scores[index] = score;
            return QueueType::COPY;
        }
    }

    { // Prefer the most video decode codecs, then more queues
        size_t index = (size_t)QueueType::VIDEO_DECODE;
        uint32_t score = props.videoDecodeCodecNum * 100000 + props.queueCount * 100 + (!props.graphics ? 10 : 0) + (!props.compute ? 10 : 0) + (!props.copy ? 10 : 0) + (props.sparse ? 5 : 0) + (props.videoDecode ? 100 * props.queueCount : 0) + (!props.videoEncode ? 2 : 0) + (props.protect ? 1 : 0) + (!props.opticalFlow ? 1 : 0);

        if (props.videoDecode && score > scores[index]) {
            scores[index] = score;
            return QueueType::VIDEO_DECODE;
        }
    }

    { // Prefer the most video encode codecs, then more queues
        size_t index = (size_t)QueueType::VIDEO_ENCODE;
        uint32_t score = props.videoEncodeCodecNum * 100000 + props.queueCount * 100 + (!props.graphics ? 10 : 0) + (!props.compute ? 10 : 0) + (!props.copy ? 10 : 0) + (props.sparse ? 5 : 0) + (!props.videoDecode ? 2 : 0) + (props.videoEncode ? 100 * props.queueCount : 0) + (props.protect ? 1 : 0) + (!props.opticalFlow ? 1 : 0);

        if (props.videoEncode && score > scores[index]) {
            scores[index] = score;
            return QueueType::VIDEO_ENCODE;
        }
    }

    return QueueType::MAX_NUM;
}

inline Uid_t ConstructUid(uint8_t luid[8], uint8_t uuid[16], bool isLuidValid) {
    Uid_t out = {};

    if (isLuidValid)
        memcpy(&out.low, luid, sizeof(out.low));
    else {
        memcpy(&out.low, uuid, sizeof(out.low));
        memcpy(&out.high, uuid + 8, sizeof(out.high));
    }

    return out;
}

#endif

} // namespace nri

#include "DeviceBase.h" // requires "StdAllocator"
