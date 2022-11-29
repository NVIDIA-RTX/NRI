cmake_minimum_required(VERSION 3.15)

# Options
option(NRI_DISABLE_XLIB_SUPPORT "Disable 'xlib' support" OFF)
option(NRI_DISABLE_WAYLAND_SUPPORT "Disable 'wayland' support" OFF)
option(NRI_STATIC_LIBRARY "Build static library" OFF)

# Cached
set(GLOBAL_BIN_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/_Build" CACHE STRING "")
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "")

# Create project
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Include/NRI.h" ver_h)
string(REGEX MATCH "NRI_VERSION_MAJOR ([0-9]*)" _ ${ver_h})
set(NRI_VERSION_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "NRI_VERSION_MINOR ([0-9]*)" _ ${ver_h})
set(NRI_VERSION_MINOR ${CMAKE_MATCH_1})

message("NRI v${NRI_VERSION_MAJOR}.${NRI_VERSION_MINOR}")
project(NRI LANGUAGES C CXX)

# Download dependencies using Packman
if(WIN32)
    set(NRI_PACKMAN_EXT ".cmd")
endif()

if((CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64") OR (CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64"))
    if(WIN32)
        set(PACKMAN_PLATFORM "windows-x86_64")
    else()
        set(PACKMAN_PLATFORM "linux-x86_64")
    endif()
elseif((CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64") OR (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64"))
    set(PACKMAN_PLATFORM "linux-aarch64")
else()
    message(FATAL_ERROR "Unsupported architecture: '${CMAKE_SYSTEM_PROCESSOR}'")
endif()
message("Packman platform: ${PACKMAN_PLATFORM}")

execute_process(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/External/Packman/Packman${NRI_PACKMAN_EXT}" pull "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies.xml" -p ${PACKMAN_PLATFORM}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE PACKMAN_RESULT)

if(NOT "${PACKMAN_RESULT}" STREQUAL "0")
    message(FATAL_ERROR "Packman failed(code = ${PACKMAN_RESULT})")
endif()

# Setup dependencies
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(NRI_BIN_ARCHITECTURE "x64")
        set(NRI_NVAPI_BIN_ARCHITECTURE "amd64")
    else()
        set(NRI_BIN_ARCHITECTURE "x86")
        set(NRI_NVAPI_BIN_ARCHITECTURE "x86")
    endif()

    # External
    find_library(NRI_INPUT_LIB_NVAPI NAMES nvapi64 nvapi PATHS "External/NVAPI/${NRI_NVAPI_BIN_ARCHITECTURE}" REQUIRED)
    find_library(NRI_INPUT_LIB_AGS NAMES amd_ags_${NRI_BIN_ARCHITECTURE} PATHS "External/AGS/lib" REQUIRED)

    # System
    if("$ENV{WIN10_SDK_PATH}$ENV{WIN10_SDK_VERSION}" STREQUAL "")
        get_filename_component(NRI_WINDOWS_SDK_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" ABSOLUTE CACHE)
        set(NRI_WINDOWS_SDK_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
        if("${NRI_WINDOWS_SDK_VERSION}" STREQUAL "")
            set(NRI_WINDOWS_SDK_VERSION ${CMAKE_SYSTEM_VERSION})
            message("Using CMAKE_SYSTEM_VERSION as WindowsSDK version.(${CMAKE_SYSTEM_VERSION})")
        endif()
    else()
        set(NRI_WINDOWS_SDK_PATH $ENV{WIN10_SDK_PATH})
        set(NRI_WINDOWS_SDK_VERSION $ENV{WIN10_SDK_VERSION})
    endif()

    if("${NRI_WINDOWS_SDK_PATH}" STREQUAL "")
        message(FATAL_ERROR "Failed to detect WindowsSDK path. Please add enviroment variables WIN10_SDK_PATH and WIN10_SDK_VERSION or use MSVC")
    endif()
    if("${NRI_WINDOWS_SDK_VERSION}" STREQUAL "")
        message(FATAL_ERROR "Failed to detect WindowsSDK version. Please add enviroment variables WIN10_SDK_PATH and WIN10_SDK_VERSION or use MSVC")
    endif()
    if(IS_DIRECTORY "${NRI_WINDOWS_SDK_PATH}/Include/${NRI_WINDOWS_SDK_VERSION}.0")
        set(NRI_WINDOWS_SDK_VERSION "${NRI_WINDOWS_SDK_VERSION}.0")
    endif()
    set(NRI_WINDOWS_SDK_LIB_PATH "${NRI_WINDOWS_SDK_PATH}/Lib/${NRI_WINDOWS_SDK_VERSION}/um/${NRI_BIN_ARCHITECTURE}")

    find_library(NRI_INPUT_LIB_D3D11 NAMES d3d11 HINTS ${NRI_WINDOWS_SDK_LIB_PATH} REQUIRED)
    find_library(NRI_INPUT_LIB_D3D12 NAMES d3d12 HINTS ${NRI_WINDOWS_SDK_LIB_PATH} REQUIRED)
    find_library(NRI_INPUT_LIB_DXGI NAMES dxgi HINTS ${NRI_WINDOWS_SDK_LIB_PATH} REQUIRED)
    find_library(NRI_INPUT_LIB_DXGUID NAMES dxguid HINTS ${NRI_WINDOWS_SDK_LIB_PATH} REQUIRED)
endif()

# Globals?
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_POSITION_INDEPENDENT_BINARIES ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# On Windows GLFW requires "/MT"
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

# Compile options
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(COMPILE_OPTIONS -Wextra)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(COMPILE_OPTIONS -Wextra)
elseif(MSVC)
    set(COMPILE_OPTIONS /W4 /WX /wd4324)
endif()

# Compile difinitions
set(COMPILE_DEFINITIONS NRI_USE_VULKAN=1)
if(WIN32)
    set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS _UNICODE UNICODE _ENFORCE_MATCHING_ALLOCATORS=0)
    set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} NRI_USE_D3D11=1 NRI_USE_D3D12=1)
endif()

# Shared
file(GLOB_RECURSE NRI_SHARED_SOURCE "Source/Shared/*.cpp" "Source/Shared/*.h" "Source/Shared/*.hpp")
source_group("" FILES ${NRI_SHARED_SOURCE})
add_library(NRI_Shared STATIC ${NRI_SHARED_SOURCE})
target_include_directories(NRI_Shared PRIVATE "Include" "Source/Shared")
target_compile_definitions(NRI_Shared PRIVATE ${COMPILE_DEFINITIONS})
target_compile_options(NRI_Shared PRIVATE ${COMPILE_OPTIONS})
set_property(TARGET NRI_Shared PROPERTY FOLDER ${PROJECT_NAME})

# D3D11
if(WIN32)
    file(GLOB_RECURSE NRI_D3D11_SOURCE "Source/D3D11/*.cpp" "Source/D3D11/*.h" "Source/D3D11/*.hpp")
    source_group("" FILES ${NRI_D3D11_SOURCE})
    file(GLOB_RECURSE NRI_D3D11_NVAPI "External/NVAPI/*.h")
    source_group("External/NVAPI" FILES ${NRI_D3D11_NVAPI})
    file(GLOB_RECURSE NRI_D3D11_AMDAGS "External/AGS/inc/*.h")
    source_group("External/AMDAGS" FILES ${NRI_D3D11_AMDAGS})
    add_library(NRI_D3D11 STATIC ${NRI_D3D11_SOURCE} ${NRI_D3D11_NVAPI} ${NRI_D3D11_AMDAGS})
    target_include_directories(NRI_D3D11 PRIVATE "Include" "Source/Shared" "External")
    target_compile_definitions(NRI_D3D11 PRIVATE ${COMPILE_DEFINITIONS})
    target_compile_options(NRI_D3D11 PRIVATE ${COMPILE_OPTIONS})
    target_link_libraries(NRI_D3D11 PRIVATE NRI_Shared ${NRI_INPUT_LIB_D3D11} ${NRI_INPUT_LIB_DXGI} ${NRI_INPUT_LIB_DXGUID} ${NRI_INPUT_LIB_NVAPI})
    set_property(TARGET NRI_D3D11 PROPERTY FOLDER ${PROJECT_NAME})
endif()

# D3D12
if(WIN32)
    file(GLOB_RECURSE NRI_D3D12_SOURCE "Source/D3D12/*.cpp" "Source/D3D12/*.h" "Source/D3D12/*.hpp")
    source_group("" FILES ${NRI_D3D12_SOURCE})
    add_library(NRI_D3D12 STATIC ${NRI_D3D12_SOURCE})
    target_include_directories(NRI_D3D12 PRIVATE "Include" "Source/Shared")
    target_compile_definitions(NRI_D3D12 PRIVATE ${COMPILE_DEFINITIONS})
    target_compile_options(NRI_D3D12 PRIVATE ${COMPILE_OPTIONS})
    target_link_libraries(NRI_D3D12 PRIVATE NRI_Shared ${NRI_INPUT_LIB_D3D12} ${NRI_INPUT_LIB_DXGI})
    set_property(TARGET NRI_D3D12 PROPERTY FOLDER ${PROJECT_NAME})
endif()

# VK
file(GLOB_RECURSE NRI_VK_SOURCE "Source/VK/*.cpp" "Source/VK/*.h" "Source/VK/*.hpp")
source_group("" FILES ${NRI_VK_SOURCE})
add_library(NRI_VK STATIC ${NRI_VK_SOURCE})
target_include_directories(NRI_VK PRIVATE "Include" "Source/Shared" "External/vulkan/include")
target_compile_definitions(NRI_VK PRIVATE ${COMPILE_DEFINITIONS})
target_compile_options(NRI_VK PRIVATE ${COMPILE_OPTIONS})
target_link_libraries(NRI_VK PRIVATE NRI_Shared)
set_property(TARGET NRI_VK PROPERTY FOLDER ${PROJECT_NAME})

if(WIN32)
    target_compile_definitions(NRI_VK PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif(APPLE)
    target_compile_definitions(NRI_VK PRIVATE VK_USE_PLATFORM_METAL_EXT)
else()
    if(NOT NRI_DISABLE_XLIB_SUPPORT)
        find_path(NRI_XLIB_HEADERS NAMES "X11/Xlib.h")
        if(NOT NRI_XLIB_HEADERS)
            message(FATAL_ERROR "Can't find 'X11/xlib.h'.(libx11-dev is not installed?)")
        endif()
        target_compile_definitions(NRI_VK PRIVATE VK_USE_PLATFORM_XLIB_KHR)
    endif()
    if(NOT NRI_DISABLE_WAYLAND_SUPPORT)
        find_path(NRI_WAYLAND_HEADERS NAMES "wayland-client.h")
        if(NOT NRI_WAYLAND_HEADERS)
            message(FATAL_ERROR "Can't find 'wayland-client.h'.(libwayland-dev is not installed?)")
        endif()
        target_compile_definitions(NRI_VK PRIVATE VK_USE_PLATFORM_WAYLAND_KHR)
    endif()
endif()

# Validation
file(GLOB_RECURSE NRI_VALIDATION_SOURCE "Source/Validation/*.cpp" "Source/Validation/*.h" "Source/Validation/*.hpp")
source_group("" FILES ${NRI_VALIDATION_SOURCE})
add_library(NRI_Validation STATIC ${NRI_VALIDATION_SOURCE})
target_include_directories(NRI_Validation PRIVATE "Include" "Source/Shared")
target_compile_definitions(NRI_Validation PRIVATE ${COMPILE_DEFINITIONS})
target_compile_options(NRI_Validation PRIVATE ${COMPILE_OPTIONS})
target_link_libraries(NRI_Validation PRIVATE NRI_Shared)
set_property(TARGET NRI_Validation PROPERTY FOLDER ${PROJECT_NAME})

# NRI
file(GLOB NRI_HEADERS "Include/*.h" "Include/*.hpp")
source_group("Include" FILES ${NRI_HEADERS})
file(GLOB NRI_HEADERS_EXTENSIONS "Include/Extensions/*.h" "Include/Extensions/*.hpp")
source_group("Include/Extensions" FILES ${NRI_HEADERS_EXTENSIONS})
file(GLOB_RECURSE NRI_SOURCE "Source/Creation/*.cpp" "Source/Creation/*.h")
source_group("Sources" FILES ${NRI_SOURCE})
file(GLOB NRI_RESOURCE "Resources/*")
source_group("Resources" FILES ${NRI_RESOURCE})

if(NRI_STATIC_LIBRARY)
    add_library(${PROJECT_NAME} STATIC ${NRI_SOURCE} ${NRI_HEADERS} ${NRI_RESOURCE} ${NRI_HEADERS_EXTENSIONS})
else()
    add_library(${PROJECT_NAME} SHARED ${NRI_SOURCE} ${NRI_HEADERS} ${NRI_RESOURCE} ${NRI_HEADERS_EXTENSIONS})
    if(WIN32)
        target_compile_definitions(${PROJECT_NAME} PRIVATE "NRI_API=extern \"C\" __declspec(dllexport)")
        target_compile_definitions(${PROJECT_NAME} PRIVATE "NRIC_API=extern \"C\" __declspec(dllexport)")
    else()
        target_compile_definitions(${PROJECT_NAME} PRIVATE "NRI_API=extern \"C\" __attribute__((visibility(\"default\")))")
        target_compile_definitions(${PROJECT_NAME} PRIVATE "NRIC_API=extern \"C\" __attribute__((visibility(\"default\")))")
    endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE "Include" "Source/Shared" "External/vulkan/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE ${COMPILE_DEFINITIONS})
target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILE_OPTIONS})

target_link_libraries(${PROJECT_NAME} PRIVATE NRI_Shared NRI_VK NRI_Validation)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE NRI_D3D11 NRI_D3D12)
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${GLOBAL_BIN_OUTPUT_PATH}/$<CONFIG>")
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${GLOBAL_BIN_OUTPUT_PATH}/$<CONFIG>")
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${GLOBAL_BIN_OUTPUT_PATH}/$<CONFIG>")
message("NRI output path: '${GLOBAL_BIN_OUTPUT_PATH}'")
