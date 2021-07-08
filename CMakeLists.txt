cmake_minimum_required(VERSION 3.10)

# NRI_CUSTOM_VULKAN_SDK_PATH
option(NRI_DISABLE_INTERPROCEDURAL_OPTIMIZATION "disable interprocedural optimization" OFF)
option(NRI_DISABLE_XLIB_SUPPORT "disable xlib support" OFF)
option(NRI_DISABLE_WAYLAND_SUPPORT "disable wayland support" OFF)
option(NRI_STATIC_LIBRARY "build static lib" OFF)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Include/NRI.h" ver_h)
string(REGEX MATCH "NRI_VERSION_MAJOR ([0-9]*)" _ ${ver_h})
set(ver_major ${CMAKE_MATCH_1})
string(REGEX MATCH "NRI_VERSION_MINOR ([0-9]*)" _ ${ver_h})
set(ver_minor ${CMAKE_MATCH_1})

project(NRI VERSION ${ver_major}.${ver_minor} LANGUAGES CXX)
message(STATUS "${PROJECT_NAME} version: ${ver_major}.${ver_minor}")

if (WIN32)
    set(NRI_PACKMAN_PLATFORM "windows-x86_64")

    execute_process(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/External/Packman/packman.cmd" pull "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies.xml" -p ${NRI_PACKMAN_PLATFORM}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE NRI_PACKMAN_RESULT)

    if (${NRI_PACKMAN_RESULT} EQUAL 0)
        message(STATUS "Packman result: success")
    else()
        message(FATAL_ERROR "Packman result: ${NRI_PACKMAN_RESULT}")
    endif()
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_POSITION_INDEPENDENT_BINARIES ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

if (NOT NRI_DISABLE_INTERPROCEDURAL_OPTIMIZATION)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

file(GLOB_RECURSE NRI_SHARED_SOURCE "Source/Shared/*.cpp" "Source/Shared/*.h" "Source/Shared/*.hpp")
file(GLOB_RECURSE NRI_D3D11_SOURCE "Source/D3D11/*.cpp" "Source/D3D11/*.h" "Source/D3D11/*.hpp")
file(GLOB_RECURSE NRI_D3D12_SOURCE "Source/D3D12/*.cpp" "Source/D3D12/*.h" "Source/D3D12/*.hpp")
file(GLOB_RECURSE NRI_VK_SOURCE "Source/VK/*.cpp" "Source/VK/*.h" "Source/VK/*.hpp")
file(GLOB_RECURSE NRI_VALIDATION_SOURCE "Source/Validation/*.cpp" "Source/Validation/*.h" "Source/Validation/*.hpp")
file(GLOB_RECURSE NRI_SOURCE "Source/Creation/*.cpp" "Source/Creation/*.h")

source_group("" FILES ${NRI_SHARED_SOURCE})
source_group("" FILES ${NRI_D3D11_SOURCE})
source_group("" FILES ${NRI_D3D12_SOURCE})
source_group("" FILES ${NRI_VK_SOURCE})
source_group("" FILES ${NRI_VALIDATION_SOURCE})
source_group("" FILES ${NRI_SOURCE})

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # -Wmissing-braces
    # warning: suggest braces around initialization of subobject [-Wmissing-braces]
    # D:\git\nri\Source\D3D12\CommandBufferD3D12.cpp(344,75) initialization of anonymous union
    add_compile_options(-Wno-switch -Wno-missing-braces)
endif()

# defines and VulkanSDK path for Windows
if (WIN32)
    set(NRI_VULKAN_SDK_INCLUDE_PATH "$ENV{VULKAN_SDK}/Include")
    set(NRI_VULKAN_SDK_LIB_PATH "$ENV{VULKAN_SDK}/Lib")
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS _UNICODE UNICODE _ENFORCE_MATCHING_ALLOCATORS=0)
    add_compile_definitions(NRI_USE_D3D11=1 NRI_USE_D3D12=1)
endif()

if (NRI_CUSTOM_VULKAN_SDK_PATH)
    set(NRI_VULKAN_SDK_INCLUDE_PATH "${NRI_CUSTOM_VULKAN_SDK_PATH}/include")
    set(NRI_VULKAN_SDK_LIB_PATH "${NRI_CUSTOM_VULKAN_SDK_PATH}/lib")
    message("NRI: using custom VulkanSDK path: '${NRI_CUSTOM_VULKAN_SDK_PATH}'")
endif()

add_compile_definitions(NRI_USE_VULKAN=1)

# Dependencies
find_library(NRI_INPUT_LIB_VULKAN vulkan-1 vulkan PATHS "${NRI_VULKAN_SDK_LIB_PATH}" REQUIRED)
if (WIN32)
    if ("$ENV{WIN10_SDK_PATH}$ENV{WIN10_SDK_VERSION}" STREQUAL "" )
        get_filename_component(NRI_WINDOWS_SDK_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" ABSOLUTE CACHE)
        set(NRI_WINDOWS_SDK_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
    elseif()
        set(NRI_WINDOWS_SDK_PATH $ENV{WIN10_SDK_PATH})
        set(NRI_WINDOWS_SDK_VERSION $ENV{WIN10_SDK_VERSION})
    endif()
    if (IS_DIRECTORY "${NRI_WINDOWS_SDK_PATH}/Include/${NRI_WINDOWS_SDK_VERSION}.0")
        set(NRI_WINDOWS_SDK_VERSION "${NRI_WINDOWS_SDK_VERSION}.0")
    endif()
    set(NRI_WINDOWS_SDK_LIB_PATH "${NRI_WINDOWS_SDK_PATH}/Lib/${NRI_WINDOWS_SDK_VERSION}/um/x64")

    find_library(NRI_INPUT_LIB_D3D11 NAMES d3d11 HINTS ${NRI_WINDOWS_SDK_LIB_PATH} REQUIRED)
    find_library(NRI_INPUT_LIB_D3D12 NAMES d3d12 HINTS ${NRI_WINDOWS_SDK_LIB_PATH} REQUIRED)
    find_library(NRI_INPUT_LIB_DXGI NAMES dxgi HINTS ${NRI_WINDOWS_SDK_LIB_PATH} REQUIRED)
    find_library(NRI_INPUT_LIB_DXGUID NAMES dxguid HINTS ${NRI_WINDOWS_SDK_LIB_PATH} REQUIRED)
    find_library(NRI_INPUT_LIB_NVAPI NAMES nvapi64 PATHS "External/NVAPI/amd64" REQUIRED)
    find_library(NRI_INPUT_LIB_AGS NAMES amd_ags_x64 PATHS "External/AGS/lib" REQUIRED)
endif()

# Shared
add_library(NRI_Shared STATIC ${NRI_SHARED_SOURCE})
target_include_directories(NRI_Shared PRIVATE "Include" "Source/Shared")
set_property(TARGET NRI_Shared PROPERTY FOLDER "${NRI_PROJECT_FOLDER}") 

if (WIN32)
    #D3D11
    add_library(NRI_D3D11 STATIC ${NRI_D3D11_SOURCE})
    target_include_directories(NRI_D3D11 PRIVATE "Include" "Source/Shared" "External")
    target_link_libraries(NRI_D3D11 PRIVATE NRI_Shared ${NRI_INPUT_LIB_D3D11} ${NRI_INPUT_LIB_DXGI} ${NRI_INPUT_LIB_DXGUID} ${NRI_INPUT_LIB_NVAPI})
    set_property(TARGET NRI_D3D11 PROPERTY FOLDER "${NRI_PROJECT_FOLDER}") 

    # D3D12
    add_library(NRI_D3D12 STATIC ${NRI_D3D12_SOURCE})
    target_include_directories(NRI_D3D12 PRIVATE "Include" "Source/Shared")
    target_link_libraries(NRI_D3D12 PRIVATE NRI_Shared ${NRI_INPUT_LIB_D3D12} ${NRI_INPUT_LIB_DXGI})
    set_property(TARGET NRI_D3D12 PROPERTY FOLDER "${NRI_PROJECT_FOLDER}") 
endif()

# Vulkan
add_library(NRI_VK STATIC ${NRI_VK_SOURCE})
target_include_directories(NRI_VK PRIVATE "Include" "Source/Shared" "${NRI_VULKAN_SDK_INCLUDE_PATH}")
target_link_libraries(NRI_VK PRIVATE NRI_Shared ${NRI_INPUT_LIB_VULKAN})
set_property(TARGET NRI_VK PROPERTY FOLDER "${NRI_PROJECT_FOLDER}") 

# Window system defines
if (WIN32)
    target_compile_definitions(NRI_VK PRIVATE VK_USE_PLATFORM_WIN32_KHR)
else()
    if (NOT NRI_CUSTOM_VULKAN_SDK_PATH)
        find_path(NRI_VULKANSDK_HEADERS NAMES "vulkan/vulkan.h" REQUIRED)
        if (NOT NRI_VULKANSDK_HEADERS)
            message(FATAL_ERROR "Can't find 'vulkan/vulkan.h'. (vulkansdk is not installed?)")
        endif()
    endif()
    if (NOT NRI_DISABLE_XLIB_SUPPORT)
        find_path(NRI_XLIB_HEADERS NAMES "X11/Xlib.h" REQUIRED)
        if (NOT NRI_XLIB_HEADERS)
            message(FATAL_ERROR "Can't find 'X11/xlib.h'. (libx11-dev is not installed?)")
        endif()
        target_compile_definitions(NRI_VK PRIVATE VK_USE_PLATFORM_XLIB_KHR)
    endif()
    if (NOT NRI_DISABLE_WAYLAND_SUPPORT)
        find_path(NRI_WAYLAND_HEADERS NAMES "wayland-client.h" REQUIRED)
        if (NOT NRI_WAYLAND_HEADERS)
            message(FATAL_ERROR "Can't find 'wayland-client.h'. (libwayland-dev is not installed?)")
        endif()
        target_compile_definitions(NRI_VK PRIVATE VK_USE_PLATFORM_WAYLAND_KHR)
    endif()
endif()

# Validation
add_library(NRI_Validation STATIC ${NRI_VALIDATION_SOURCE})
target_include_directories(NRI_Validation PRIVATE "Include" "Source/Shared")
target_link_libraries(NRI_Validation PRIVATE NRI_Shared)
set_property(TARGET NRI_Validation PROPERTY FOLDER "${NRI_PROJECT_FOLDER}") 

# NRI
if (NRI_STATIC_LIBRARY)
    add_library(${PROJECT_NAME} STATIC ${NRI_SOURCE})
else()
    add_library(${PROJECT_NAME} SHARED ${NRI_SOURCE})
endif()

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE NRI_D3D11 NRI_D3D12)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE NRI_Shared NRI_VK NRI_Validation ${NRI_INPUT_LIB_VULKAN})
target_include_directories(${PROJECT_NAME} PRIVATE "Include" "Source/Shared" "${NRI_VULKAN_SDK_INCLUDE_PATH}")
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "${NRI_PROJECT_FOLDER}") 

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "NRI_API=extern \"C\" __declspec(dllexport)")
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE "NRI_API=extern \"C\" __attribute__((visibility(\"default\")))")
endif()

set(OUTPUT_BIN_PATH "${PROJECT_BINARY_DIR}/Output/${CMAKE_CXX_COMPILER_ID}_${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR}")
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_BIN_PATH}/Release")
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_BIN_PATH}/Debug")
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_BIN_PATH}/Release")
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_BIN_PATH}/Debug")
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_BIN_PATH}/Release")
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_BIN_PATH}/Debug")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

