cmake_minimum_required(VERSION 3.30)

include(CMakeDependentOption)
include(FetchContent)

# Create project
project(NRI LANGUAGES C CXX)

# Test packages availability ("find_package" works only for X11)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_path(XLIB_HEADERS NAMES "X11/Xlib.h")
    find_path(WAYLAND_HEADERS NAMES "wayland-client.h")
endif()

# Options
set(NRI_AGILITY_SDK_DIR "AgilitySDK" CACHE STRING "Directory where Agility SDK will be copied relative to the directory with binaries")
set(NRI_AGILITY_SDK_VERSION_MAJOR "616" CACHE STRING "Agility SDK major version") # 717 (can cause D3D12_ERROR_INVALID_REDIST error?)
set(NRI_AGILITY_SDK_VERSION_MINOR "1" CACHE STRING "Agility SDK minor version") # 1-preview
set(NRI_SHADERS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/_Shaders" CACHE STRING "Shader output path override")

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
cmake_dependent_option(NRI_ENABLE_D3D_EXTENSIONS "Enable vendor specific extension libraries for D3D (NVAPI and AMD AGS)" ON "NRI_ENABLE_D3D11_SUPPORT OR NRI_ENABLE_D3D12_SUPPORT" OFF)
cmake_dependent_option(NRI_ENABLE_AGILITY_SDK_SUPPORT "Enable Agility SDK support to unlock access to latest D3D12 features" ON "NRI_ENABLE_D3D12_SUPPORT" OFF)
cmake_dependent_option(NRI_ENABLE_XLIB_SUPPORT "Enable X11 support" ON "XLIB_HEADERS" OFF)
cmake_dependent_option(NRI_ENABLE_WAYLAND_SUPPORT "Enable Wayland support" ON "WAYLAND_HEADERS" OFF)
cmake_dependent_option(NRI_ENABLE_NGX_SDK "Enable NVIDIA NGX (DLSS) SDK" OFF "NRI_ENABLE_D3D11_SUPPORT OR NRI_ENABLE_D3D12_SUPPORT OR NRI_ENABLE_VK_SUPPORT" OFF)
cmake_dependent_option(NRI_ENABLE_FFX_SDK "Enable AMD FidelityFX SDK" OFF "(NRI_ENABLE_VK_SUPPORT OR NRI_ENABLE_D3D12_SUPPORT) AND WIN32" OFF)
cmake_dependent_option(NRI_ENABLE_XESS_SDK "Enable INTEL XeSS SDK" OFF "NRI_ENABLE_D3D12_SUPPORT AND WIN32" OFF)

# Is submodule?
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    set(IS_SUBMODULE OFF)
else()
    set(IS_SUBMODULE ON)
endif()

# Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_POSITION_INDEPENDENT_BINARIES ON)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

if(NOT IS_SUBMODULE)
    get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

    if(IS_MULTI_CONFIG)
        set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "")
    endif()

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/_Bin" CACHE STRING "")
endif()

# Debugger markup
if(MSVC)
    set(NRI_NATVIS "nri.natvis")
else()
    set(NRI_NATVIS "")
endif()

# Enable grouping of source files into folders in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Compile options
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(COMPILE_OPTIONS -Wextra -Wno-missing-field-initializers)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(COMPILE_OPTIONS -Wextra -Wno-missing-field-initializers)
elseif(MSVC)
    set(COMPILE_OPTIONS
        /W4 /WX
        /wd4324 # padding was added at the end of a structure because you specified an alignment specifier
        $<$<CONFIG:Release>:/Zi> # generate a PDB for Release build
    )
else()
    message(WARNING "NRI: Unknown compiler!")
endif()

# Compile definitions
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Include/NRI.h" ver_h)
string(REGEX MATCH "NRI_VERSION ([0-9]*)" _ ${ver_h})
set(NRI_VERSION ${CMAKE_MATCH_1})

message("NRI: v${NRI_VERSION}:")

set(COMPILE_DEFINITIONS WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)

function(add_compile_definition DEFINITION)
    if(${DEFINITION})
        message(STATUS "${DEFINITION}")
        set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} ${DEFINITION}=1 PARENT_SCOPE)
    endif()
endfunction()

add_compile_definition(NRI_STATIC_LIBRARY)
add_compile_definition(NRI_ENABLE_NVTX_SUPPORT)
add_compile_definition(NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS)
add_compile_definition(NRI_ENABLE_NONE_SUPPORT)
add_compile_definition(NRI_ENABLE_VK_SUPPORT)
add_compile_definition(NRI_ENABLE_VALIDATION_SUPPORT)
add_compile_definition(NRI_ENABLE_NIS_SDK)
add_compile_definition(NRI_ENABLE_IMGUI_EXTENSION)
add_compile_definition(NRI_ENABLE_D3D11_SUPPORT)
add_compile_definition(NRI_ENABLE_D3D12_SUPPORT)
add_compile_definition(NRI_ENABLE_D3D_EXTENSIONS)
add_compile_definition(NRI_ENABLE_AGILITY_SDK_SUPPORT)
add_compile_definition(NRI_ENABLE_XLIB_SUPPORT)
add_compile_definition(NRI_ENABLE_WAYLAND_SUPPORT)
add_compile_definition(NRI_ENABLE_NGX_SDK)
add_compile_definition(NRI_ENABLE_FFX_SDK)
add_compile_definition(NRI_ENABLE_XESS_SDK)

if(NRI_ENABLE_NIS_SDK)
    set(NRI_REQUIRES_SHADERMAKE 1)
else()
    set(NRI_REQUIRES_SHADERMAKE 0)
endif()

add_compile_definition(NRI_REQUIRES_SHADERMAKE)

# Find Windows SDK
if(NRI_ENABLE_D3D11_SUPPORT OR NRI_ENABLE_D3D12_SUPPORT)
    message("NRI: looking for Windows SDK:")

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(BIN_ARCHITECTURE "x64")
        set(NVAPI_BIN_ARCHITECTURE "amd64")
    else()
        set(BIN_ARCHITECTURE "x86")
        set(NVAPI_BIN_ARCHITECTURE "x86")
    endif()

    if("$ENV{WIN10_SDK_PATH}$ENV{WIN10_SDK_VERSION}" STREQUAL "")
        get_filename_component(WINDOWS_SDK_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" ABSOLUTE CACHE)
        set(WINDOWS_SDK_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})

        if("${WINDOWS_SDK_VERSION}" STREQUAL "")
            set(WINDOWS_SDK_VERSION ${CMAKE_SYSTEM_VERSION})
            message(STATUS "Using CMAKE_SYSTEM_VERSION as WindowsSDK version.(${CMAKE_SYSTEM_VERSION})")
        endif()
    else()
        set(WINDOWS_SDK_PATH $ENV{WIN10_SDK_PATH})
        set(WINDOWS_SDK_VERSION $ENV{WIN10_SDK_VERSION})
    endif()

    if("${WINDOWS_SDK_PATH}" STREQUAL "")
        message(FATAL_ERROR "Failed to detect WindowsSDK path. Please add enviroment variables WIN10_SDK_PATH and WIN10_SDK_VERSION or use MSVC")
    endif()

    if("${WINDOWS_SDK_VERSION}" STREQUAL "")
        message(FATAL_ERROR "Failed to detect WindowsSDK version. Please add enviroment variables WIN10_SDK_PATH and WIN10_SDK_VERSION or use MSVC")
    endif()

    if(IS_DIRECTORY "${WINDOWS_SDK_PATH}/Include/${WINDOWS_SDK_VERSION}.0")
        set(WINDOWS_SDK_VERSION "${WINDOWS_SDK_VERSION}.0")
    endif()

    set(WINDOWS_SDK_LIB_PATH "${WINDOWS_SDK_PATH}/Lib/${WINDOWS_SDK_VERSION}/um/${BIN_ARCHITECTURE}")
    message(STATUS "${WINDOWS_SDK_LIB_PATH}")

    find_library(INPUT_LIB_DXGI NAMES dxgi HINTS ${WINDOWS_SDK_LIB_PATH} REQUIRED)
    find_library(INPUT_LIB_DXGUID NAMES dxguid HINTS ${WINDOWS_SDK_LIB_PATH} REQUIRED)
endif()

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
        URL https://github.com/NVIDIA/NVTX/archive/refs/tags/v3.2.1.zip
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
        URL https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v1.4.323.zip
    )
    list(APPEND DEPS vulkan_headers)

    # Vulkan MA
    FetchContent_Declare(
        vma
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/refs/tags/v3.3.0.zip
    )
    list(APPEND DEPS vma)
endif()

if(NRI_ENABLE_NGX_SDK)
    # NVIDIA NGX SDK
    FetchContent_Declare(
        ngx
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL https://github.com/NVIDIA/DLSS/archive/refs/tags/v310.3.0.zip
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
        URL https://github.com/intel/xess/releases/download/v2.0.1/XeSS_SDK_2.0.1.zip
    )
    list(APPEND DEPS xess)
endif()

# Dependencies: GIT_REPOSITORY - worse (because of ".git" folder)
if(NRI_ENABLE_D3D12_SUPPORT)
    # D3D12 MA
    FetchContent_Declare(
        d3d12ma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator.git
        GIT_TAG v3.0.1
        GIT_SHALLOW 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
    )
    list(APPEND DEPS d3d12ma)
endif()

if(NRI_ENABLE_D3D_EXTENSIONS)
    # AMD AGS
    FetchContent_Declare(
        amdags
        DOWNLOAD_EXTRACT_TIMESTAMP 1
        DOWNLOAD_NO_PROGRESS 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
        URL https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK/archive/refs/tags/v6.3.0.zip
    )
    list(APPEND DEPS amdags)

    # NV API
    FetchContent_Declare(
        nvapi
        GIT_REPOSITORY https://github.com/NVIDIA/nvapi.git
        GIT_TAG main
        GIT_SHALLOW 1
        SOURCE_SUBDIR "ignore CMakeLists.txt"
    )
    list(APPEND DEPS nvapi)
endif()

# Add dependencies for dependencies
if(NRI_REQUIRES_SHADERMAKE AND NOT TARGET ShaderMake)
    # ShaderMake
    option(SHADERMAKE_TOOL "" ON)

    FetchContent_Declare(
        shadermake
        GIT_REPOSITORY https://github.com/NVIDIA-RTX/ShaderMake.git
        GIT_TAG 757594f0539314467f74950c8d898e815ecbd383
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

# D3D extensions
if(NRI_ENABLE_D3D_EXTENSIONS)
    find_library(NVAPI_LIB NAMES nvapi64 nvapi PATHS "${nvapi_SOURCE_DIR}/${NVAPI_BIN_ARCHITECTURE}" REQUIRED) # statically linked
    find_library(AGS_LIB NAMES amd_ags_${BIN_ARCHITECTURE} PATHS "${amdags_SOURCE_DIR}/ags_lib/lib" REQUIRED) # dynamically loaded

    file(GLOB NVAPI_HEADERS
        "${nvapi_SOURCE_DIR}/nvapi.h"
        "${nvapi_SOURCE_DIR}/nvHLSLExtns.h"
        "${nvapi_SOURCE_DIR}/nvShaderExtnEnums.h"
    )
    source_group("External/nvapi" FILES ${NVAPI_HEADERS})

    file(GLOB AMDAGS_HEADERS
        "${amdags_SOURCE_DIR}/ags_lib/inc/*.h"
        "${amdags_SOURCE_DIR}/ags_lib/hlsl/*.hlsl"
    )
    source_group("External/amdags" FILES ${AMDAGS_HEADERS})
    set_source_files_properties(${AMDAGS_HEADERS} PROPERTIES VS_TOOL_OVERRIDE "None")
endif()

# NONE
if(NRI_ENABLE_NONE_SUPPORT)
    file(GLOB NONE_SOURCE "Source/NONE/*")
    source_group("" FILES ${NONE_SOURCE})
    add_library(NRI_NONE STATIC ${NONE_SOURCE})
    target_include_directories(NRI_NONE PRIVATE
        "Include"
        "Source/Shared"
    )
    target_compile_definitions(NRI_NONE PRIVATE ${COMPILE_DEFINITIONS})
    target_compile_options(NRI_NONE PRIVATE ${COMPILE_OPTIONS})
    set_target_properties(NRI_NONE PROPERTIES FOLDER ${PROJECT_NAME})
endif()

# D3D11
if(NRI_ENABLE_D3D11_SUPPORT)
    file(GLOB D3D11_SOURCE "Source/D3D11/*")
    source_group("" FILES ${D3D11_SOURCE})

    if(NRI_ENABLE_D3D_EXTENSIONS)
        add_library(NRI_D3D11 STATIC ${D3D11_SOURCE} ${NVAPI_HEADERS} ${AMDAGS_HEADERS})
        target_link_libraries(NRI_D3D11 PRIVATE ${NVAPI_LIB})
    else()
        add_library(NRI_D3D11 STATIC ${D3D11_SOURCE})
    endif()

    find_library(INPUT_LIB_D3D11 NAMES d3d11 HINTS ${WINDOWS_SDK_LIB_PATH} REQUIRED)

    target_include_directories(NRI_D3D11 PRIVATE
        "Include"
        "Source/Shared"
        "${nvapi_SOURCE_DIR}"
        "${amdags_SOURCE_DIR}/ags_lib/inc"
    )
    target_compile_definitions(NRI_D3D11 PRIVATE ${COMPILE_DEFINITIONS})
    target_compile_options(NRI_D3D11 PRIVATE ${COMPILE_OPTIONS})
    target_link_libraries(NRI_D3D11 PRIVATE NRI_Shared ${INPUT_LIB_D3D11} ${INPUT_LIB_DXGI} ${INPUT_LIB_DXGUID})
    set_target_properties(NRI_D3D11 PROPERTIES FOLDER ${PROJECT_NAME})
endif()

# D3D12
if(NRI_ENABLE_D3D12_SUPPORT)
    file(GLOB D3D12_VMA "${d3d12ma_SOURCE_DIR}/include/D3D12MemAlloc.h")
    source_group("External/vma" FILES ${D3D12_VMA})

    file(GLOB D3D12_SOURCE "Source/D3D12/*")
    source_group("" FILES ${D3D12_SOURCE})

    if(NRI_ENABLE_D3D_EXTENSIONS)
        add_library(NRI_D3D12 STATIC ${D3D12_SOURCE} ${D3D12_VMA} ${NVAPI_HEADERS} ${AMDAGS_HEADERS})
        target_link_libraries(NRI_D3D12 PRIVATE ${NVAPI_LIB})
    else()
        add_library(NRI_D3D12 STATIC ${D3D12_SOURCE})
    endif()

    find_library(INPUT_LIB_D3D12 NAMES d3d12 HINTS ${WINDOWS_SDK_LIB_PATH} REQUIRED)

    target_include_directories(NRI_D3D12 PRIVATE
        "Include"
        "Source/Shared"
        "${d3d12ma_SOURCE_DIR}/include"
        "${d3d12ma_SOURCE_DIR}/src"
        "${nvapi_SOURCE_DIR}"
        "${amdags_SOURCE_DIR}/ags_lib/inc"
    )
    target_compile_options(NRI_D3D12 PRIVATE ${COMPILE_OPTIONS})
    target_link_libraries(NRI_D3D12 PRIVATE NRI_Shared ${INPUT_LIB_D3D12} ${INPUT_LIB_DXGI} ${INPUT_LIB_DXGUID})
    set_target_properties(NRI_D3D12 PROPERTIES FOLDER ${PROJECT_NAME})

    if(NRI_ENABLE_AGILITY_SDK_SUPPORT)
        message("NRI: using AgilitySDK v1.${NRI_AGILITY_SDK_VERSION_MAJOR}.${NRI_AGILITY_SDK_VERSION_MINOR}")

        target_compile_definitions(NRI_D3D12 PRIVATE ${COMPILE_DEFINITIONS} NRI_AGILITY_SDK_VERSION_MAJOR=${NRI_AGILITY_SDK_VERSION_MAJOR})
        target_include_directories(NRI_D3D12 PRIVATE "${agilitysdk_SOURCE_DIR}/build/native/include")

        # Generate "NRIAgilitySDK.h"
        file(WRITE Include/NRIAgilitySDK.h
            "// This file is auto-generated during project deployment. Do not modify!\n"
            "#ifdef __cplusplus\n"
            "extern \"C\" {\n"
            "#endif\n\n"
            "__declspec(dllexport) extern const uint32_t D3D12SDKVersion = ${NRI_AGILITY_SDK_VERSION_MAJOR};\n"
            "__declspec(dllexport) extern const char* D3D12SDKPath = \"${NRI_AGILITY_SDK_DIR}/\";\n\n"
            "#ifdef __cplusplus\n"
            "}\n"
            "#endif\n"
        )
    else()
        target_compile_definitions(NRI_D3D12 PRIVATE ${COMPILE_DEFINITIONS} NRI_AGILITY_SDK_VERSION_MAJOR=D3D12_SDK_VERSION)
    endif()
endif()

# VK
if(NRI_ENABLE_VK_SUPPORT)
    file(GLOB VK_VMA "${vma_SOURCE_DIR}/include/vk_mem_alloc.h")
    source_group("External/vma" FILES ${VK_VMA})

    file(GLOB VK_SOURCE "Source/VK/*")
    source_group("" FILES ${VK_SOURCE})

    add_library(NRI_VK STATIC ${VK_SOURCE} ${VK_VMA})
    target_include_directories(NRI_VK PRIVATE
        "Include"
        "Source/Shared"
        "${vma_SOURCE_DIR}/include"
        "${vulkan_headers_SOURCE_DIR}/include"
    )
    target_compile_definitions(NRI_VK PRIVATE ${COMPILE_DEFINITIONS})
    target_compile_options(NRI_VK PRIVATE ${COMPILE_OPTIONS})
    target_link_libraries(NRI_VK PRIVATE NRI_Shared)
    set_target_properties(NRI_VK PROPERTIES FOLDER ${PROJECT_NAME})

    if(WIN32)
        target_compile_definitions(NRI_VK PRIVATE VK_USE_PLATFORM_WIN32_KHR)
    elseif(APPLE)
        find_package(Vulkan REQUIRED)
        target_link_libraries(NRI_VK PRIVATE ${Vulkan_LIBRARIES})
        target_compile_definitions(NRI_VK PRIVATE VK_USE_PLATFORM_METAL_EXT VK_ENABLE_BETA_EXTENSIONS)
    else()
        if(NRI_ENABLE_XLIB_SUPPORT)
            target_compile_definitions(NRI_VK PRIVATE VK_USE_PLATFORM_XLIB_KHR)
        endif()

        if(NRI_ENABLE_WAYLAND_SUPPORT)
            target_compile_definitions(NRI_VK PRIVATE VK_USE_PLATFORM_WAYLAND_KHR)
        endif()
    endif()
endif()

# Validation
if(NRI_ENABLE_VALIDATION_SUPPORT)
    file(GLOB NRI_VALIDATION_SOURCE
        "Source/Validation/*.cpp"
        "Source/Validation/*.h"
        "Source/Validation/*.hpp"
    )
    source_group("" FILES ${NRI_VALIDATION_SOURCE})

    add_library(NRI_Validation STATIC ${NRI_VALIDATION_SOURCE})
    target_include_directories(NRI_Validation PRIVATE
        "Include"
        "Source/Shared"
    )
    target_compile_definitions(NRI_Validation PRIVATE ${COMPILE_DEFINITIONS})
    target_compile_options(NRI_Validation PRIVATE ${COMPILE_OPTIONS})
    target_link_libraries(NRI_Validation PRIVATE NRI_Shared)
    set_target_properties(NRI_Validation PROPERTIES FOLDER ${PROJECT_NAME})
endif()

# Shared
file(GLOB SHARED_SOURCE
    "Source/Shared/*.cpp"
    "Source/Shared/*.h"
    "Source/Shared/*.hpp"
)
source_group("" FILES ${SHARED_SOURCE})
add_library(NRI_Shared STATIC ${SHARED_SOURCE})

target_include_directories(NRI_Shared PRIVATE
    "Include"
    "Source/Shared"
)

if(NRI_ENABLE_AGILITY_SDK_SUPPORT)
    target_include_directories(NRI_Shared PRIVATE "${agilitysdk_SOURCE_DIR}/build/native/include")
endif()

if(NRI_ENABLE_VK_SUPPORT)
    target_include_directories(NRI_Shared PRIVATE "${vulkan_headers_SOURCE_DIR}/include")
endif()

if(NRI_ENABLE_NGX_SDK)
    target_include_directories(NRI_Shared PRIVATE "${ngx_SOURCE_DIR}/include")

    if(WIN32)
        if((NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY) OR (${CMAKE_MSVC_RUNTIME_LIBRARY} MATCHES "DLL"))
            set(NGX_SUFFIX "d")
            message("NRI: using NGX LIB with dynamic runtime")
        else()
            set(NGX_SUFFIX "s")
            message("NRI: using NGX LIB with static runtime")
        endif()

        set(NGX_LIB "${ngx_SOURCE_DIR}/lib/Windows_x86_64/x64/nvsdk_ngx_${NGX_SUFFIX}")

        target_link_libraries(NRI_Shared PRIVATE debug ${NGX_LIB}_dbg.lib)
        target_link_libraries(NRI_Shared PRIVATE optimized ${NGX_LIB}.lib)
    else()
        target_link_libraries(NRI_Shared PRIVATE "${ngx_SOURCE_DIR}/lib/Linux_x86_64/libnvsdk_ngx.a")
    endif()
endif()

if(NRI_ENABLE_FFX_SDK)
    target_include_directories(NRI_Shared PRIVATE "${ffx_SOURCE_DIR}/ffx-api/include/ffx_api")
endif()

if(NRI_ENABLE_XESS_SDK)
    target_include_directories(NRI_Shared PRIVATE "${xess_SOURCE_DIR}/inc/xess")
    target_link_libraries(NRI_Shared PRIVATE "${xess_SOURCE_DIR}/lib/libxess.lib")
endif()

target_compile_definitions(NRI_Shared PRIVATE ${COMPILE_DEFINITIONS})
target_compile_options(NRI_Shared PRIVATE ${COMPILE_OPTIONS})
set_target_properties(NRI_Shared PROPERTIES FOLDER ${PROJECT_NAME})

# NRI
file(GLOB NRI_HEADERS
    "Include/*.h"
    "Include/*.hlsl"
)
set_source_files_properties(${NRI_HEADERS} PROPERTIES VS_TOOL_OVERRIDE "None")
source_group("Include" FILES ${NRI_HEADERS})

file(GLOB NRI_EXTENSIONS
    "Include/Extensions/*.h"
    "Include/Extensions/*.hpp"
)
source_group("Include/Extensions" FILES ${NRI_EXTENSIONS})

file(GLOB NRI_SOURCE
    "Source/Creation/*.cpp"
    "Source/Creation/*.h"
)
source_group("Sources" FILES ${NRI_SOURCE})

file(GLOB NRI_RESOURCES "Resources/*")
source_group("Resources" FILES ${NRI_RESOURCES})

if(NRI_STATIC_LIBRARY)
    add_library(${PROJECT_NAME} STATIC ${NRI_SOURCE} ${NRI_HEADERS} ${NRI_RESOURCES} ${NRI_EXTENSIONS} ${NRI_NATVIS})
else()
    add_library(${PROJECT_NAME} SHARED ${NRI_SOURCE} ${NRI_HEADERS} ${NRI_RESOURCES} ${NRI_EXTENSIONS} ${NRI_NATVIS})

    if(WIN32)
        target_compile_definitions(${PROJECT_NAME} PRIVATE "NRI_API=extern \"C\" __declspec(dllexport)")
    else()
        target_compile_definitions(${PROJECT_NAME} PRIVATE "NRI_API=extern \"C\" __attribute__((visibility(\"default\")))")
    endif()

    if(MSVC)
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:/DEBUG> # generate a PDB for Release build
            $<$<CONFIG:Release>:/OPT:ICF> # perform identical COMDAT folding (removes duplicate code/data)
        )
    endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC "Include")
target_include_directories(${PROJECT_NAME} PRIVATE
    "Source/Shared"
    "${nvtx_SOURCE_DIR}/c/include/"
    "${vulkan_headers_SOURCE_DIR}/include"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE ${COMPILE_DEFINITIONS})

if(NRI_ENABLE_AGILITY_SDK_SUPPORT)
    target_compile_definitions(${PROJECT_NAME} PUBLIC NRI_ENABLE_AGILITY_SDK_SUPPORT=1) # make it available in the parent project
endif()

target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILE_OPTIONS})

target_link_libraries(${PROJECT_NAME} PRIVATE NRI_Shared)

if(NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
endif()

if(NRI_ENABLE_NONE_SUPPORT)
    target_link_libraries(${PROJECT_NAME} PRIVATE NRI_NONE)
endif()

if(NRI_ENABLE_D3D11_SUPPORT)
    target_link_libraries(${PROJECT_NAME} PRIVATE NRI_D3D11)
endif()

if(NRI_ENABLE_D3D12_SUPPORT)
    target_link_libraries(${PROJECT_NAME} PRIVATE NRI_D3D12)
endif()

if(NRI_ENABLE_VK_SUPPORT)
    target_link_libraries(${PROJECT_NAME} PRIVATE NRI_VK)
endif()

if(NRI_ENABLE_VALIDATION_SUPPORT)
    target_link_libraries(${PROJECT_NAME} PRIVATE NRI_Validation)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    FOLDER ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

message("NRI: output path '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}'")

# Copy to the output folder
if(NRI_ENABLE_D3D_EXTENSIONS)
    find_file(AMD_AGS_DLL NAMES amd_ags_x64.dll PATHS "${amdags_SOURCE_DIR}/ags_lib/lib")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${AMD_AGS_DLL}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        VERBATIM)
endif()

if(NRI_ENABLE_AGILITY_SDK_SUPPORT)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${agilitysdk_SOURCE_DIR}/build/native/bin/x64" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${NRI_AGILITY_SDK_DIR}"
        VERBATIM)
endif()

if(NRI_ENABLE_NGX_SDK)
    set(NGX_LIB_DIR $<IF:$<CONFIG:Debug>,dev,rel>)

    if(WIN32)
        set(DLSS_SR_DLL "${ngx_SOURCE_DIR}/lib/Windows_x86_64/${NGX_LIB_DIR}/nvngx_dlss.dll")
        set(DLSS_RR_DLL "${ngx_SOURCE_DIR}/lib/Windows_x86_64/${NGX_LIB_DIR}/nvngx_dlssd.dll")
    else()
        set(DLSS_SR_DLL "${ngx_SOURCE_DIR}/lib/Linux_x86_64/${NGX_LIB_DIR}/libnvidia-ngx-dlss.so.310.3.0")
        set(DLSS_RR_DLL "${ngx_SOURCE_DIR}/lib/Linux_x86_64/${NGX_LIB_DIR}/libnvidia-ngx-dlssd.so.310.3.0")
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLSS_SR_DLL} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLSS_RR_DLL} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        VERBATIM)
endif()

if(NRI_ENABLE_FFX_SDK)
    if(NRI_ENABLE_D3D12_SUPPORT)
        find_file(FFX_DX12_DLL NAMES amd_fidelityfx_dx12.dll PATHS "${ffx_SOURCE_DIR}/PrebuiltSignedDLL")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FFX_DX12_DLL} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            VERBATIM)
    endif()

    if(NRI_ENABLE_VK_SUPPORT AND WIN32)
        find_file(FFX_VK_DLL NAMES amd_fidelityfx_vk.dll PATHS "${ffx_SOURCE_DIR}/PrebuiltSignedDLL")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FFX_VK_DLL} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            VERBATIM)
    endif()
endif()

if(NRI_ENABLE_XESS_SDK)
    if(NRI_ENABLE_D3D12_SUPPORT)
        find_file(XESS_DX12_DLL NAMES libxess.dll PATHS "${xess_SOURCE_DIR}/bin")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${XESS_DX12_DLL} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            VERBATIM)
    endif()
endif()

# Shaders
if(NRI_REQUIRES_SHADERMAKE)
    file(GLOB_RECURSE SHADERS
        "Shaders/*.hlsl"
        "Shaders/*.hlsli"
    )
    set_source_files_properties(${SHADERS} PROPERTIES VS_TOOL_OVERRIDE "None")

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

    set(SHADERMAKE_COMMANDS "")

    if(NRI_ENABLE_D3D11_SUPPORT)
        set(SHADERMAKE_COMMANDS ${SHADERMAKE_COMMANDS} COMMAND ${SHADERMAKE_PATH} -p DXBC --compiler "${SHADERMAKE_FXC_PATH}" ${SHADERMAKE_GENERAL_ARGS})
    endif()

    if(NRI_ENABLE_D3D12_SUPPORT)
        set(SHADERMAKE_COMMANDS ${SHADERMAKE_COMMANDS} COMMAND ${SHADERMAKE_PATH} -p DXIL --compiler "${SHADERMAKE_DXC_PATH}" ${SHADERMAKE_GENERAL_ARGS})
    endif()

    if(NRI_ENABLE_VK_SUPPORT)
        set(SHADERMAKE_COMMANDS ${SHADERMAKE_COMMANDS} COMMAND ${SHADERMAKE_PATH} -p SPIRV --compiler "${SHADERMAKE_DXC_VK_PATH}" ${SHADERMAKE_GENERAL_ARGS})
    endif()

    add_custom_target(NRI_Shaders ALL
        ${SHADERMAKE_COMMANDS}
        DEPENDS ShaderMake
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        VERBATIM
        SOURCES ${SHADERS}
    )

    set_target_properties(NRI_Shaders PROPERTIES FOLDER ${PROJECT_NAME})

    add_dependencies(NRI_Shared NRI_Shaders)
    target_include_directories(NRI_Shared PRIVATE "${NRI_SHADERS_PATH}")
    target_link_libraries(NRI_Shared PRIVATE ShaderMakeBlob)

    message("NRI: shaders path '${NRI_SHADERS_PATH}'")
endif()
