cmake_minimum_required(VERSION 3.30)

include(CMakeDependentOption)
include(FetchContent)

# Create project
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Include/NRI.h" ver_h)
string(REGEX MATCH "NRI_VERSION ([0-9]*)" _ ${ver_h})
set(NRI_VERSION ${CMAKE_MATCH_1})

project(NRI VERSION ${NRI_VERSION} LANGUAGES CXX)
message("NRI: v${NRI_VERSION}:")

# Test packages availability ("find_package" works only for X11)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_path(XLIB_HEADERS NAMES "X11/Xlib.h")
    find_path(WAYLAND_HEADERS NAMES "wayland-client.h")
endif()

# Architecture
if((CMAKE_GENERATOR_PLATFORM MATCHES "ARM64") OR (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64"))
    set(IS_ARM64 TRUE)
    set(BIN_ARCHITECTURE "arm64")
    set(NVAPI_BIN_ARCHITECTURE "arm64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(IS_ARM64 FALSE)
    set(BIN_ARCHITECTURE "x64")
    set(NVAPI_BIN_ARCHITECTURE "amd64")
else()
    set(IS_ARM64 FALSE)
    set(BIN_ARCHITECTURE "x86")
    set(NVAPI_BIN_ARCHITECTURE "x86")
endif()

# Options
set(NRI_AGILITY_SDK_DIR "AgilitySDK" CACHE STRING "Directory where Agility SDK will be copied relative to the directory with binaries")
set(NRI_AGILITY_SDK_VERSION_MAJOR "618" CACHE STRING "Agility SDK major version") # 717 (can cause D3D12_ERROR_INVALID_REDIST error?)
set(NRI_AGILITY_SDK_VERSION_MINOR "1" CACHE STRING "Agility SDK minor version") # 1-preview
set(NRI_SHADERS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/_Shaders" CACHE STRING "Shader output path override")
set(NRI_NVAPI_CUSTOM_PATH "" CACHE STRING "Path to a custom NVAPI library directory")

option(NRI_STATIC_LIBRARY "Build static library" OFF)
option(NRI_ENABLE_NVTX_SUPPORT "Annotations for NVIDIA Nsight Systems" ON)
option(NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS "Enable debug names, host and device annotations" ON)
option(NRI_ENABLE_NONE_SUPPORT "Enable NONE backend" ON)
option(NRI_ENABLE_VK_SUPPORT "Enable Vulkan backend" ON)
option(NRI_ENABLE_VALIDATION_SUPPORT "Enable Validation backend (otherwise 'enableNRIValidation' is ignored)" ON)
option(NRI_ENABLE_NIS_SDK "Enable NVIDIA Image Sharpening SDK" OFF)
option(NRI_ENABLE_IMGUI_EXTENSION "Enable 'NRIImgui' extension" OFF)

cmake_dependent_option(NRI_ENABLE_D3D11_SUPPORT "Enable D3D11 backend" ON "WIN32" OFF)
cmake_dependent_option(NRI_ENABLE_D3D12_SUPPORT "Enable D3D12 backend" ON "WIN32" OFF)
cmake_dependent_option(NRI_ENABLE_AMDAGS "Enable AMD AGS library for D3D" ON "(NRI_ENABLE_D3D11_SUPPORT OR NRI_ENABLE_D3D12_SUPPORT); NOT IS_ARM64" OFF)
cmake_dependent_option(NRI_ENABLE_NVAPI "Enable NVAPI library for D3D" ON "(NRI_ENABLE_D3D11_SUPPORT OR NRI_ENABLE_D3D12_SUPPORT); NOT IS_ARM64" OFF)
cmake_dependent_option(NRI_ENABLE_AGILITY_SDK_SUPPORT "Enable Agility SDK support to unlock access to latest D3D12 features" ON "NRI_ENABLE_D3D12_SUPPORT" OFF)
cmake_dependent_option(NRI_ENABLE_XLIB_SUPPORT "Enable X11 support" ON "XLIB_HEADERS" OFF)
cmake_dependent_option(NRI_ENABLE_WAYLAND_SUPPORT "Enable Wayland support" ON "WAYLAND_HEADERS" OFF)
cmake_dependent_option(NRI_ENABLE_NGX_SDK "Enable NVIDIA NGX (DLSS) SDK" OFF "(NRI_ENABLE_D3D11_SUPPORT OR NRI_ENABLE_D3D12_SUPPORT OR NRI_ENABLE_VK_SUPPORT); NOT IS_ARM64" OFF)
cmake_dependent_option(NRI_ENABLE_FFX_SDK "Enable AMD FidelityFX SDK" OFF "(NRI_ENABLE_VK_SUPPORT OR NRI_ENABLE_D3D12_SUPPORT); WIN32; NOT IS_ARM64" OFF)
cmake_dependent_option(NRI_ENABLE_XESS_SDK "Enable INTEL XeSS SDK" OFF "NRI_ENABLE_D3D12_SUPPORT; WIN32; NOT IS_ARM64" OFF)

# ShaderMake required?
if(NRI_ENABLE_NIS_SDK)
    set(NRI_ENABLE_SHADERMAKE ON)
else()
    set(NRI_ENABLE_SHADERMAKE OFF)
endif()

# Self-build?
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR}) # if not submodule
    get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

    if(IS_MULTI_CONFIG)
        set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "")
    endif()

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/_Bin" CACHE STRING "")
endif()

# Enable grouping of source files into folders in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Compile definitions
set(COMPILE_DEFINITIONS "")
foreach(opt IN ITEMS
    NRI_STATIC_LIBRARY
    NRI_ENABLE_NVTX_SUPPORT
    NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
    NRI_ENABLE_NONE_SUPPORT
    NRI_ENABLE_VK_SUPPORT
    NRI_ENABLE_VALIDATION_SUPPORT
    NRI_ENABLE_NIS_SDK
    NRI_ENABLE_IMGUI_EXTENSION
    NRI_ENABLE_D3D11_SUPPORT
    NRI_ENABLE_D3D12_SUPPORT
    NRI_ENABLE_NVAPI
    NRI_ENABLE_AMDAGS
    NRI_ENABLE_AGILITY_SDK_SUPPORT
    NRI_ENABLE_XLIB_SUPPORT
    NRI_ENABLE_WAYLAND_SUPPORT
    NRI_ENABLE_NGX_SDK
    NRI_ENABLE_FFX_SDK
    NRI_ENABLE_XESS_SDK
    NRI_ENABLE_SHADERMAKE
)
    if(${opt})
        message(STATUS "${opt}")
        list(APPEND COMPILE_DEFINITIONS "${opt}=1")
    endif()
endforeach()

# Construct list of dependencies # TODO: update to latest releases from time to time
set(DEPS)

# Dependencies: URL - good (no fat ".git" folder)
if(NRI_ENABLE_AGILITY_SDK_SUPPORT)
    # Agility SDK
    FetchContent_Declare(
        agilitysdk
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.${NRI_AGILITY_SDK_VERSION_MAJOR}.${NRI_AGILITY_SDK_VERSION_MINOR}
    )
    list(APPEND DEPS agilitysdk)
endif()

if(NRI_ENABLE_NVTX_SUPPORT)
    # NVTX
    FetchContent_Declare(
        nvtx
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL https://github.com/NVIDIA/NVTX/archive/refs/tags/v3.3.0.zip
    )
    list(APPEND DEPS nvtx)
endif()

if(NRI_ENABLE_VK_SUPPORT)
    # Vulkan headers
    FetchContent_Declare(
        vulkan_headers
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v1.4.329.zip
    )
    list(APPEND DEPS vulkan_headers)

    # Vulkan MA
    FetchContent_Declare(
        vma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG 6d36dca2c71a7f0468a129f664bc9b474da5a886
        GIT_SHALLOW 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
    )
    list(APPEND DEPS vma)
endif()

set(NGX_VERSION "310.4.0")
if(NRI_ENABLE_NGX_SDK)
    # NVIDIA NGX SDK
    FetchContent_Declare(
        ngx
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL https://github.com/NVIDIA/DLSS/archive/refs/tags/v${NGX_VERSION}.zip
    )
    list(APPEND DEPS ngx)
endif()

if(NRI_ENABLE_FFX_SDK)
    # AMD FidelityFX SDK
    FetchContent_Declare(
        ffx
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/releases/download/v1.1.4/FidelityFX-SDK-v1.1.4.zip
    )
    list(APPEND DEPS ffx)
endif()

if(NRI_ENABLE_XESS_SDK)
    # INTEL XeSS SDK
    FetchContent_Declare(
        xess
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL https://github.com/intel/xess/archive/refs/tags/v2.1.0.zip
    )
    list(APPEND DEPS xess)
endif()

if(NRI_ENABLE_D3D12_SUPPORT)
    # D3D12 MA
    FetchContent_Declare(
        d3d12ma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator.git
        GIT_TAG 2ac8a9bdada39ad75210be7b6da8b2b0f61e84f5
        GIT_SHALLOW 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
    )
    list(APPEND DEPS d3d12ma)
endif()

if(NRI_ENABLE_AMDAGS)
    # AMD AGS
    FetchContent_Declare(
        amdags
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK/archive/refs/tags/v6.3.0.zip
    )
    list(APPEND DEPS amdags)
endif()

# Dependencies: GIT_REPOSITORY - worse (because of ".git" folder)
if(NRI_ENABLE_NVAPI)
    # NV API
    if(NRI_NVAPI_CUSTOM_PATH)
        get_filename_component(nvapi_SOURCE_DIR "${NRI_NVAPI_CUSTOM_PATH}" ABSOLUTE)
        message("NRI: using custom NVAPI from '${nvapi_SOURCE_DIR}'")
    else()
        FetchContent_Declare(
            nvapi
            GIT_REPOSITORY https://github.com/NVIDIA/nvapi.git
            GIT_TAG main
            GIT_SHALLOW 1
            SOURCE_SUBDIR "ignore CMakeLists.txt"
        )
        list(APPEND DEPS nvapi)
    endif()
endif()

if(NRI_ENABLE_SHADERMAKE AND NOT TARGET ShaderMake)
    # ShaderMake
    option(SHADERMAKE_TOOL "" ON)

    FetchContent_Declare(
        shadermake
        GIT_REPOSITORY https://github.com/NVIDIA-RTX/ShaderMake.git
        GIT_TAG main
        GIT_SHALLOW 1
    )
    list(APPEND DEPS shadermake)
endif()

# Download dependencies
if(DEPS)
    message("NRI: downloading dependencies:")
    message(STATUS "${DEPS} ...")

    FetchContent_MakeAvailable(${DEPS})

    # Delete original archives
    foreach(DEP IN LISTS DEPS)
        file(REMOVE_RECURSE "${FETCHCONTENT_BASE_DIR}/${DEP}-tmp")
    endforeach()
endif()

# NVAPI
if(NRI_ENABLE_NVAPI)
    find_library(NVAPI_LIB
        NAMES
            nvapi64 nvapi
        PATHS
            "${nvapi_SOURCE_DIR}/${NVAPI_BIN_ARCHITECTURE}"
        REQUIRED
    ) # statically linked

    set(NVAPI_HEADERS
        "${nvapi_SOURCE_DIR}/nvapi.h"
        "${nvapi_SOURCE_DIR}/nvHLSLExtns.h"
        "${nvapi_SOURCE_DIR}/nvShaderExtnEnums.h"
    )
    source_group("External/nvapi"
        FILES
            ${NVAPI_HEADERS}
    )
endif()

# AMDAGS
if(NRI_ENABLE_AMDAGS)
    find_library(AGS_LIB
        NAMES
            amd_ags_${BIN_ARCHITECTURE}
        PATHS
            "${amdags_SOURCE_DIR}/ags_lib/lib"
        REQUIRED
    ) # dynamically loaded

    set(AMDAGS_HEADERS
        "${amdags_SOURCE_DIR}/ags_lib/inc/amd_ags.h"
        "${amdags_SOURCE_DIR}/ags_lib/hlsl/ags_shader_intrinsics_dx11.hlsl"
        "${amdags_SOURCE_DIR}/ags_lib/hlsl/ags_shader_intrinsics_dx12.hlsl"
    )
    source_group("External/amdags"
        FILES
            ${AMDAGS_HEADERS}
    )
    set_source_files_properties(${AMDAGS_HEADERS}
        PROPERTIES
            VS_SETTINGS "ExcludedFromBuild=true" # do not compile HLSL
    )
endif()

# Shared
if(NRI_ENABLE_NGX_SDK)
    if(WIN32)
        if((NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY) OR (${CMAKE_MSVC_RUNTIME_LIBRARY} MATCHES "DLL"))
            set(NGX_SUFFIX "d")
            message("NRI: using NGX LIB with dynamic runtime")
        else()
            set(NGX_SUFFIX "s")
            message("NRI: using NGX LIB with static runtime")
        endif()

        set(NGX_LIB "${ngx_SOURCE_DIR}/lib/Windows_x86_64/x64/nvsdk_ngx_${NGX_SUFFIX}$<$<CONFIG:Debug>:_dbg>.lib")
    else()
        set(NGX_LIB "${ngx_SOURCE_DIR}/lib/Linux_x86_64/libnvsdk_ngx.a")
    endif()
endif()

set(SHARED_SOURCE
    "Source/Shared/DeviceBase.h"
    "Source/Shared/HelperInterface.h"
    "Source/Shared/HelperInterface.hpp"
    "Source/Shared/ImguiInterface.h"
    "Source/Shared/ImguiInterface.hpp"
    "Source/Shared/Lock.h"
    "Source/Shared/NIS.h"
    "Source/Shared/Shared.cpp"
    "Source/Shared/SharedExternal.h"
    "Source/Shared/SharedExternal.hpp"
    "Source/Shared/SharedLibrary.hpp"
    "Source/Shared/StreamerInterface.h"
    "Source/Shared/StreamerInterface.hpp"
    "Source/Shared/UpscalerInterface.h"
    "Source/Shared/UpscalerInterface.hpp"
)

add_library(NRI_Shared STATIC)
target_sources(NRI_Shared
    PRIVATE
        ${SHARED_SOURCE}
)
target_include_directories(NRI_Shared
    PUBLIC
        "Include"
        "Source/Shared"
    PRIVATE
        $<$<BOOL:${NRI_ENABLE_AGILITY_SDK_SUPPORT}>:
            ${agilitysdk_SOURCE_DIR}/build/native/include
        >
        $<$<BOOL:${NRI_ENABLE_VK_SUPPORT}>:
            ${vulkan_headers_SOURCE_DIR}/include
        >
        $<$<BOOL:${NRI_ENABLE_NGX_SDK}>:
            ${ngx_SOURCE_DIR}/include
        >
        $<$<BOOL:${NRI_ENABLE_FFX_SDK}>:
            ${ffx_SOURCE_DIR}/ffx-api/include/ffx_api
        >
        $<$<BOOL:${NRI_ENABLE_XESS_SDK}>:
            ${xess_SOURCE_DIR}/inc/xess
        >
)
target_compile_definitions(NRI_Shared
    PUBLIC
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        ${COMPILE_DEFINITIONS}
)
target_compile_options(NRI_Shared
    PUBLIC
        # Options for Clang/GCC compilers
        # No "nullability-completeness" caused by VMA
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
            $<$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>:
                -Wextra
                -Werror
                -fvisibility=hidden
            >
            -Wno-missing-field-initializers
            -Wno-nullability-completeness
        >
        # Options for MSVC frontend
        $<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:
            /W4
            /WX
            /wd4324
            $<$<CONFIG:Release>:/Zi>
        >
)
target_compile_features(NRI_Shared
    PUBLIC
        cxx_std_17
)
target_link_libraries(NRI_Shared
    PRIVATE
        $<$<BOOL:${NRI_ENABLE_NGX_SDK}>:
            ${NGX_LIB}
        >
        $<$<BOOL:${NRI_ENABLE_XESS_SDK}>:
            ${xess_SOURCE_DIR}/lib/libxess.lib
        >
)
set_target_properties(NRI_Shared
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON           # enable PIC...
        INTERFACE_POSITION_INDEPENDENT_CODE ON # ...and propagate to all consumers
        FOLDER "NRI"
)

# NONE
if(NRI_ENABLE_NONE_SUPPORT)
    set(NONE_SOURCE "Source/NONE/ImplNONE.cpp")

    add_library(NRI_NONE STATIC)
    target_sources(NRI_NONE
        PRIVATE
            ${NONE_SOURCE}
    )
    target_link_libraries(NRI_NONE
        PRIVATE
            NRI_Shared
    )
    set_target_properties(NRI_NONE
        PROPERTIES
            FOLDER "NRI"
    )
endif()

# D3D11
if(NRI_ENABLE_D3D11_SUPPORT)
    set(D3D11_SOURCE
        "Source/D3D11/BufferD3D11.h"
        "Source/D3D11/BufferD3D11.hpp"
        "Source/D3D11/CommandAllocatorD3D11.h"
        "Source/D3D11/CommandAllocatorD3D11.hpp"
        "Source/D3D11/CommandBufferD3D11.h"
        "Source/D3D11/CommandBufferD3D11.hpp"
        "Source/D3D11/CommandBufferEmuD3D11.h"
        "Source/D3D11/CommandBufferEmuD3D11.hpp"
        "Source/D3D11/DescriptorD3D11.h"
        "Source/D3D11/DescriptorD3D11.hpp"
        "Source/D3D11/DescriptorPoolD3D11.h"
        "Source/D3D11/DescriptorPoolD3D11.hpp"
        "Source/D3D11/DescriptorSetD3D11.h"
        "Source/D3D11/DescriptorSetD3D11.hpp"
        "Source/D3D11/DeviceD3D11.h"
        "Source/D3D11/DeviceD3D11.hpp"
        "Source/D3D11/FenceD3D11.h"
        "Source/D3D11/FenceD3D11.hpp"
        "Source/D3D11/ImplD3D11.cpp"
        "Source/D3D11/MemoryD3D11.h"
        "Source/D3D11/PipelineD3D11.h"
        "Source/D3D11/PipelineD3D11.hpp"
        "Source/D3D11/PipelineLayoutD3D11.h"
        "Source/D3D11/PipelineLayoutD3D11.hpp"
        "Source/D3D11/QueryPoolD3D11.h"
        "Source/D3D11/QueryPoolD3D11.hpp"
        "Source/D3D11/QueueD3D11.h"
        "Source/D3D11/QueueD3D11.hpp"
        "Source/D3D11/SharedD3D11.h"
        "Source/D3D11/SharedD3D11.hpp"
        "Source/D3D11/SwapChainD3D11.h"
        "Source/D3D11/SwapChainD3D11.hpp"
        "Source/D3D11/TextureD3D11.h"
        "Source/D3D11/TextureD3D11.hpp"
    )

    add_library(NRI_D3D11 STATIC)
    target_sources(NRI_D3D11
        PRIVATE
            ${D3D11_SOURCE}
            $<$<BOOL:${NRI_ENABLE_NVAPI}>:
                ${NVAPI_HEADERS}
            >
            $<$<BOOL:${NRI_ENABLE_AMDAGS}>:
                ${AMDAGS_HEADERS}
            >
    )
    target_include_directories(NRI_D3D11
        PRIVATE
            "${nvapi_SOURCE_DIR}"
            "${amdags_SOURCE_DIR}/ags_lib/inc"
    )
    target_link_libraries(NRI_D3D11
        PRIVATE
            NRI_Shared
            d3d11
            dxgi
            dxguid
            $<$<BOOL:${NRI_ENABLE_NVAPI}>:
                ${NVAPI_LIB}
            >
    )
    set_target_properties(NRI_D3D11
        PROPERTIES
            FOLDER "NRI"
    )
endif()

# D3D12
if(NRI_ENABLE_D3D12_SUPPORT)
    message("NRI: using Windows SDK ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")

    set(D3D12_VMA "${d3d12ma_SOURCE_DIR}/include/D3D12MemAlloc.h")
    source_group("External/Vma"
        FILES
            ${D3D12_VMA}
    )

    set(D3D12_SOURCE
        "Source/D3D12/AccelerationStructureD3D12.h"
        "Source/D3D12/AccelerationStructureD3D12.hpp"
        "Source/D3D12/BufferD3D12.h"
        "Source/D3D12/BufferD3D12.hpp"
        "Source/D3D12/CommandAllocatorD3D12.h"
        "Source/D3D12/CommandAllocatorD3D12.hpp"
        "Source/D3D12/CommandBufferD3D12.h"
        "Source/D3D12/CommandBufferD3D12.hpp"
        "Source/D3D12/DescriptorD3D12.h"
        "Source/D3D12/DescriptorD3D12.hpp"
        "Source/D3D12/DescriptorPoolD3D12.h"
        "Source/D3D12/DescriptorPoolD3D12.hpp"
        "Source/D3D12/DescriptorSetD3D12.h"
        "Source/D3D12/DescriptorSetD3D12.hpp"
        "Source/D3D12/DeviceD3D12.h"
        "Source/D3D12/DeviceD3D12.hpp"
        "Source/D3D12/FenceD3D12.h"
        "Source/D3D12/FenceD3D12.hpp"
        "Source/D3D12/ImplD3D12.cpp"
        "Source/D3D12/MemoryAllocatorD3D12.h"
        "Source/D3D12/MemoryD3D12.h"
        "Source/D3D12/MemoryD3D12.hpp"
        "Source/D3D12/MicromapD3D12.h"
        "Source/D3D12/MicromapD3D12.hpp"
        "Source/D3D12/PipelineD3D12.h"
        "Source/D3D12/PipelineD3D12.hpp"
        "Source/D3D12/PipelineLayoutD3D12.h"
        "Source/D3D12/PipelineLayoutD3D12.hpp"
        "Source/D3D12/QueryPoolD3D12.h"
        "Source/D3D12/QueryPoolD3D12.hpp"
        "Source/D3D12/QueueD3D12.h"
        "Source/D3D12/QueueD3D12.hpp"
        "Source/D3D12/SharedD3D12.h"
        "Source/D3D12/SharedD3D12.hpp"
        "Source/D3D12/SwapChainD3D12.h"
        "Source/D3D12/SwapChainD3D12.hpp"
        "Source/D3D12/TextureD3D12.h"
        "Source/D3D12/TextureD3D12.hpp"
    )

    add_library(NRI_D3D12 STATIC)
    target_sources(NRI_D3D12
        PRIVATE
            ${D3D12_SOURCE}
            ${D3D12_VMA}
            $<$<BOOL:${NRI_ENABLE_NVAPI}>:
                ${NVAPI_HEADERS}
            >
            $<$<BOOL:${NRI_ENABLE_AMDAGS}>:
                ${AMDAGS_HEADERS}
            >
    )
    target_include_directories(NRI_D3D12
        PRIVATE
            "${d3d12ma_SOURCE_DIR}/include"
            "${d3d12ma_SOURCE_DIR}/src"
            "${nvapi_SOURCE_DIR}"
            "${amdags_SOURCE_DIR}/ags_lib/inc"
            $<$<BOOL:${NRI_ENABLE_AGILITY_SDK_SUPPORT}>:
                ${agilitysdk_SOURCE_DIR}/build/native/include
            >
    )
    target_compile_definitions(NRI_D3D12
        PRIVATE
            NRI_AGILITY_SDK_VERSION_MAJOR=$<IF:$<BOOL:${NRI_ENABLE_AGILITY_SDK_SUPPORT}>,${NRI_AGILITY_SDK_VERSION_MAJOR},D3D12_SDK_VERSION>
    )
    target_link_libraries(NRI_D3D12
        PRIVATE
            NRI_Shared
            d3d12
            dxgi
            dxguid
            $<$<BOOL:${NRI_ENABLE_NVAPI}>:
                ${NVAPI_LIB}
            >
    )
    set_target_properties(NRI_D3D12
        PROPERTIES
            FOLDER "NRI"
    )

    if(NRI_ENABLE_AGILITY_SDK_SUPPORT)
        message("NRI: using AgilitySDK v1.${NRI_AGILITY_SDK_VERSION_MAJOR}.${NRI_AGILITY_SDK_VERSION_MINOR}")

        # Generate "NRIAgilitySDK.h"
        file(WRITE Include/NRIAgilitySDK.h
            "// This file is auto-generated during project deployment. Do not modify!\n"
            "#ifdef __cplusplus\n"
            "extern \"C\" {\n"
            "#endif\n\n"
            "__declspec(dllexport) extern const uint32_t D3D12SDKVersion;\n"
            "const uint32_t D3D12SDKVersion = ${NRI_AGILITY_SDK_VERSION_MAJOR};\n\n"
            "__declspec(dllexport) extern const char* D3D12SDKPath;\n"
            "const char* D3D12SDKPath = \"${NRI_AGILITY_SDK_DIR}/\";\n\n"
            "#ifdef __cplusplus\n"
            "}\n"
            "#endif\n"
        )
    endif()
endif()

# VK
if(NRI_ENABLE_VK_SUPPORT)
    if(APPLE)
        find_package(Vulkan REQUIRED)
    endif()

    set(VK_VMA "${vma_SOURCE_DIR}/include/vk_mem_alloc.h")
    source_group("External/Vma"
        FILES
            ${VK_VMA}
    )

    set(VK_SOURCE
        "Source/VK/AccelerationStructureVK.h"
        "Source/VK/AccelerationStructureVK.hpp"
        "Source/VK/BufferVK.h"
        "Source/VK/BufferVK.hpp"
        "Source/VK/CommandAllocatorVK.h"
        "Source/VK/CommandAllocatorVK.hpp"
        "Source/VK/CommandBufferVK.h"
        "Source/VK/CommandBufferVK.hpp"
        "Source/VK/ConversionVK.h"
        "Source/VK/ConversionVK.hpp"
        "Source/VK/DescriptorPoolVK.h"
        "Source/VK/DescriptorPoolVK.hpp"
        "Source/VK/DescriptorSetVK.h"
        "Source/VK/DescriptorSetVK.hpp"
        "Source/VK/DescriptorVK.h"
        "Source/VK/DescriptorVK.hpp"
        "Source/VK/DeviceVK.h"
        "Source/VK/DeviceVK.hpp"
        "Source/VK/DispatchTable.h"
        "Source/VK/FenceVK.h"
        "Source/VK/FenceVK.hpp"
        "Source/VK/ImplVK.cpp"
        "Source/VK/MemoryAllocatorVK.h"
        "Source/VK/MemoryVK.h"
        "Source/VK/MemoryVK.hpp"
        "Source/VK/MicromapVK.h"
        "Source/VK/MicromapVK.hpp"
        "Source/VK/PipelineLayoutVK.h"
        "Source/VK/PipelineLayoutVK.hpp"
        "Source/VK/PipelineVK.h"
        "Source/VK/PipelineVK.hpp"
        "Source/VK/QueryPoolVK.h"
        "Source/VK/QueryPoolVK.hpp"
        "Source/VK/QueueVK.h"
        "Source/VK/QueueVK.hpp"
        "Source/VK/SharedVK.h"
        "Source/VK/SwapChainVK.h"
        "Source/VK/SwapChainVK.hpp"
        "Source/VK/TextureVK.h"
        "Source/VK/TextureVK.hpp"
    )

    add_library(NRI_VK STATIC)
    target_sources(NRI_VK
        PRIVATE
            ${VK_SOURCE}
            ${VK_VMA}
    )
    target_include_directories(NRI_VK
        PRIVATE
            "${vma_SOURCE_DIR}/include"
            "${vulkan_headers_SOURCE_DIR}/include"
    )
    target_compile_definitions(NRI_VK
        PRIVATE
            $<$<BOOL:${WIN32}>:
                VK_USE_PLATFORM_WIN32_KHR
            >
            $<$<BOOL:${APPLE}>:
                VK_USE_PLATFORM_METAL_EXT;VK_ENABLE_BETA_EXTENSIONS
            >
            $<$<BOOL:${NRI_ENABLE_XLIB_SUPPORT}>:
                VK_USE_PLATFORM_XLIB_KHR
            >
            $<$<BOOL:${NRI_ENABLE_WAYLAND_SUPPORT}>:
                VK_USE_PLATFORM_WAYLAND_KHR
            >
    )
    target_link_libraries(NRI_VK
        PRIVATE
            NRI_Shared
            $<$<BOOL:${APPLE}>:
                ${Vulkan_LIBRARIES}
            >
    )
    set_target_properties(NRI_VK
        PROPERTIES
            FOLDER "NRI"
    )
endif()

# Validation
if(NRI_ENABLE_VALIDATION_SUPPORT)
    set(NRI_VALIDATION_SOURCE
        "Source/Validation/AccelerationStructureVal.h"
        "Source/Validation/AccelerationStructureVal.hpp"
        "Source/Validation/BufferVal.h"
        "Source/Validation/BufferVal.hpp"
        "Source/Validation/CommandAllocatorVal.h"
        "Source/Validation/CommandAllocatorVal.hpp"
        "Source/Validation/CommandBufferVal.h"
        "Source/Validation/CommandBufferVal.hpp"
        "Source/Validation/ConversionVal.hpp"
        "Source/Validation/DescriptorPoolVal.h"
        "Source/Validation/DescriptorPoolVal.hpp"
        "Source/Validation/DescriptorSetVal.h"
        "Source/Validation/DescriptorSetVal.hpp"
        "Source/Validation/DescriptorVal.h"
        "Source/Validation/DescriptorVal.hpp"
        "Source/Validation/DeviceVal.h"
        "Source/Validation/DeviceVal.hpp"
        "Source/Validation/FenceVal.h"
        "Source/Validation/FenceVal.hpp"
        "Source/Validation/ImplVal.cpp"
        "Source/Validation/MemoryVal.h"
        "Source/Validation/MemoryVal.hpp"
        "Source/Validation/MicromapVal.h"
        "Source/Validation/MicromapVal.hpp"
        "Source/Validation/PipelineLayoutVal.h"
        "Source/Validation/PipelineLayoutVal.hpp"
        "Source/Validation/PipelineVal.h"
        "Source/Validation/PipelineVal.hpp"
        "Source/Validation/QueryPoolVal.h"
        "Source/Validation/QueryPoolVal.hpp"
        "Source/Validation/QueueVal.h"
        "Source/Validation/QueueVal.hpp"
        "Source/Validation/SharedVal.h"
        "Source/Validation/SwapChainVal.h"
        "Source/Validation/SwapChainVal.hpp"
        "Source/Validation/TextureVal.h"
        "Source/Validation/TextureVal.hpp"
    )

    add_library(NRI_Validation STATIC)
    target_sources(NRI_Validation
        PRIVATE
            ${NRI_VALIDATION_SOURCE}
    )
    target_link_libraries(NRI_Validation
        PRIVATE
            NRI_Shared
    )
    set_target_properties(NRI_Validation
        PROPERTIES
            FOLDER "NRI"
    )
endif()

# Core headers
set(NRI_HEADERS
    "Include/NRI.h"
    "Include/NRI.hlsl"
    "Include/NRIDescs.h"
    "Include/NRIMacro.h"
)
source_group("Include"
    FILES
        ${NRI_HEADERS}
)
set_source_files_properties(${NRI_HEADERS}
    PROPERTIES
        VS_SETTINGS "ExcludedFromBuild=true" # do not compile HLSL
)

# Extensions headers
set(NRI_EXTENSIONS
    "Include/Extensions/NRIDeviceCreation.h"
    "Include/Extensions/NRIHelper.h"
    "Include/Extensions/NRIImgui.h"
    "Include/Extensions/NRILowLatency.h"
    "Include/Extensions/NRIMeshShader.h"
    "Include/Extensions/NRIRayTracing.h"
    "Include/Extensions/NRIStreamer.h"
    "Include/Extensions/NRISwapChain.h"
    "Include/Extensions/NRIUpscaler.h"
    "Include/Extensions/NRIWrapperD3D11.h"
    "Include/Extensions/NRIWrapperD3D12.h"
    "Include/Extensions/NRIWrapperVK.h"
)
source_group("Include/Extensions"
    FILES
        ${NRI_EXTENSIONS}
)

# Resources
set(NRI_RESOURCES
    "Resources/NRI.rc"
    "Resources/Version.h"
)
source_group("Resources"
    FILES
        ${NRI_RESOURCES}
)

# NRI
set(NRI_SOURCE "Source/Creation/Creation.cpp")
source_group("Sources"
    FILES
        ${NRI_SOURCE}
)

if(NRI_STATIC_LIBRARY)
    add_library(NRI STATIC)
else()
    add_library(NRI SHARED)

    if(CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_link_options(NRI
            PRIVATE
                $<$<CONFIG:Release>:/DEBUG> # generate a PDB for Release build
                $<$<CONFIG:Release>:/OPT:ICF> # perform identical COMDAT folding (removes duplicate code/data)
        )
    endif()
endif()

target_sources(NRI
    PRIVATE
        ${NRI_SOURCE}
        ${NRI_HEADERS}
        ${NRI_RESOURCES}
        ${NRI_EXTENSIONS}
        $<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:
            nri.natvis
        > # debugger markup
)
target_include_directories(NRI
    PRIVATE
        "${nvtx_SOURCE_DIR}/c/include/"
        "${vulkan_headers_SOURCE_DIR}/include"
    PUBLIC
        "Include"
)
target_compile_definitions(NRI
    PRIVATE
        $<$<NOT:$<BOOL:${NRI_STATIC_LIBRARY}>>:
            $<$<BOOL:${WIN32}>:
                "NRI_API=extern \"C\" __declspec(dllexport)"
            >
            $<$<NOT:$<BOOL:${WIN32}>>:
                "NRI_API=extern \"C\" __attribute__((visibility(\"default\")))"
            >
        >
    PUBLIC
        $<$<BOOL:${NRI_ENABLE_AGILITY_SDK_SUPPORT}>:
            NRI_ENABLE_AGILITY_SDK_SUPPORT=1
        >
)
target_link_libraries(NRI
    PRIVATE
        NRI_Shared
        $<$<NOT:$<BOOL:${WIN32}>>:
            ${CMAKE_DL_LIBS}
        >
        $<$<BOOL:${NRI_ENABLE_NONE_SUPPORT}>:
            NRI_NONE
        >
        $<$<BOOL:${NRI_ENABLE_D3D11_SUPPORT}>:
            NRI_D3D11
        >
        $<$<BOOL:${NRI_ENABLE_D3D12_SUPPORT}>:
            NRI_D3D12
        >
        $<$<BOOL:${NRI_ENABLE_VK_SUPPORT}>:
            NRI_VK
        >
        $<$<BOOL:${NRI_ENABLE_VALIDATION_SUPPORT}>:
            NRI_Validation
        >
)
set_target_properties(NRI
    PROPERTIES
        FOLDER "NRI"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

message("NRI: output path '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}'")

# Copy to the output folder
if(NRI_ENABLE_AMDAGS)
    find_file(AMD_AGS_DLL
        NAMES
            amd_ags_${BIN_ARCHITECTURE}.dll
        PATHS "${amdags_SOURCE_DIR}/ags_lib/lib"
    )
    add_custom_command(TARGET NRI
        POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${AMD_AGS_DLL}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        VERBATIM
    )
endif()

if(NRI_ENABLE_AGILITY_SDK_SUPPORT)
    add_custom_command(TARGET NRI
        POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${agilitysdk_SOURCE_DIR}/build/native/bin/${BIN_ARCHITECTURE}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${NRI_AGILITY_SDK_DIR}"
        VERBATIM
    )
endif()

if(NRI_ENABLE_NGX_SDK)
    set(NGX_LIB_DIR $<IF:$<CONFIG:Debug>,dev,rel>)

    if(WIN32)
        set(DLSS_SR_DLL "${ngx_SOURCE_DIR}/lib/Windows_x86_64/${NGX_LIB_DIR}/nvngx_dlss.dll")
        set(DLSS_RR_DLL "${ngx_SOURCE_DIR}/lib/Windows_x86_64/${NGX_LIB_DIR}/nvngx_dlssd.dll")
    else()
        set(DLSS_SR_DLL "${ngx_SOURCE_DIR}/lib/Linux_x86_64/${NGX_LIB_DIR}/libnvidia-ngx-dlss.so.${NGX_VERSION}")
        set(DLSS_RR_DLL "${ngx_SOURCE_DIR}/lib/Linux_x86_64/${NGX_LIB_DIR}/libnvidia-ngx-dlssd.so.${NGX_VERSION}")
    endif()

    add_custom_command(TARGET NRI
        POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLSS_SR_DLL} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLSS_RR_DLL} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        VERBATIM
    )
endif()

if(NRI_ENABLE_FFX_SDK)
    if(NRI_ENABLE_D3D12_SUPPORT)
        find_file(FFX_DX12_DLL
            NAMES
                amd_fidelityfx_dx12.dll
            PATHS
                "${ffx_SOURCE_DIR}/PrebuiltSignedDLL"
        )
        add_custom_command(TARGET NRI
            POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FFX_DX12_DLL} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                VERBATIM
        )
    endif()

    if(NRI_ENABLE_VK_SUPPORT AND WIN32)
        find_file(FFX_VK_DLL
            NAMES
                amd_fidelityfx_vk.dll
            PATHS
                "${ffx_SOURCE_DIR}/PrebuiltSignedDLL"
        )
        add_custom_command(TARGET NRI
            POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FFX_VK_DLL} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()
endif()

if(NRI_ENABLE_XESS_SDK)
    if(NRI_ENABLE_D3D12_SUPPORT)
        find_file(XESS_DX12_DLL
            NAMES
                libxess.dll
            PATHS
                "${xess_SOURCE_DIR}/bin"
        )
        add_custom_command(TARGET NRI
            POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${XESS_DX12_DLL} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()
endif()

# Shaders
if(NRI_ENABLE_SHADERMAKE)
    set(SHADERS
        "Shaders/Imgui.fs.hlsl"
        "Shaders/Imgui.vs.hlsl"
        "Shaders/NIS.hlsl"
        "Shaders/NIS.cs.hlsl"
    )
    set_source_files_properties(${SHADERS}
        PROPERTIES
            VS_SETTINGS "ExcludedFromBuild=true" # do not compile HLSL
    )

    set(SHADERMAKE_GENERAL_ARGS
        --project "NRI"
        --compactProgress
        --flatten
        --stripReflection
        --WX
        --sRegShift 0 --tRegShift 0 --bRegShift 0 --uRegShift 0
        --headerBlob
        --sourceDir "Shaders"
        --ignoreConfigDir
        -c "Shaders/Shaders.cfg"
        -o "${NRI_SHADERS_PATH}"
        -I "Shaders"
        -I "Include"
    )

    set(SHADERMAKE_COMMANDS)
    if(NRI_ENABLE_D3D11_SUPPORT)
        list(APPEND SHADERMAKE_COMMANDS COMMAND ${SHADERMAKE_PATH} -p DXBC --compiler "${SHADERMAKE_FXC_PATH}" ${SHADERMAKE_GENERAL_ARGS})
    endif()
    if(NRI_ENABLE_D3D12_SUPPORT)
        list(APPEND SHADERMAKE_COMMANDS COMMAND ${SHADERMAKE_PATH} -p DXIL --compiler "${SHADERMAKE_DXC_PATH}" ${SHADERMAKE_GENERAL_ARGS})
    endif()
    if(NRI_ENABLE_VK_SUPPORT)
        list(APPEND SHADERMAKE_COMMANDS COMMAND ${SHADERMAKE_PATH} -p SPIRV --compiler "${SHADERMAKE_DXC_VK_PATH}" ${SHADERMAKE_GENERAL_ARGS})
    endif()
    list(APPEND SHADERMAKE_COMMANDS "") # fix for "The system cannot find the batch label specified - VCEnd"

    add_custom_target(NRI_Shaders
        ALL
            ${SHADERMAKE_COMMANDS}
        DEPENDS
            ShaderMake
        WORKING_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}"
        SOURCES
            ${SHADERS}
        VERBATIM
    )
    set_target_properties(NRI_Shaders
        PROPERTIES
            FOLDER "NRI"
    )
    add_dependencies(NRI_Shared NRI_Shaders)
    target_include_directories(NRI_Shared
        PRIVATE
            "${NRI_SHADERS_PATH}"
    )
    target_link_libraries(NRI_Shared
        PRIVATE
            ShaderMakeBlob
    )

    message("NRI: shaders path '${NRI_SHADERS_PATH}'")
endif()
