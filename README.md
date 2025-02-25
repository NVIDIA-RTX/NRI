# NVIDIA RENDER INTERFACE (NRI)

[![Status](https://github.com/NVIDIA-RTX/NRI/actions/workflows/build.yml/badge.svg)](https://github.com/NVIDIA-RTX/NRI/actions/workflows/build.yml)

*NRI* is a modular extensible low-level abstract render interface, which has been designed to support all low level features of D3D12 and Vulkan GAPIs, but at the same time to simplify usage and reduce the amount of code needed (especially compared with VK).

Goals:
- generalization of D3D12 ([spec](https://microsoft.github.io/DirectX-Specs/)) and VK ([spec](https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html)) GAPIs
- explicitness (providing access to low-level features of modern GAPIs)
- "quality of life" features (providing high-level improving utilities, organized as extensions)
- low overhead
- cross platform and platform independence (AMD/INTEL friendly)
- D3D11 ([spec](https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm)) support (as much as possible)

Non-goals (exceptions apply to some extensions, where high-level abstraction and hidden management are desired):
- high level (D3D11-like) abstraction
- exposing entities not existing in GAPIs
- hidden management of any kind

Currently supported GAPIs:
- Vulkan (VK 1.3 or extended 1.2 + additional features)
- D3D12
- D3D11
- Metal (through [MoltenVK](https://github.com/KhronosGroup/MoltenVK))
- None / dummy (everything is supported, but does nothing)

Key features:
 - *C++* and *C* compatible interfaces
 - generalized common denominator for D3D12, VK and D3D11 GAPIs
 - low overhead
 - no memory allocations at runtime
 - descriptor indexing support
 - ray tracing support
 - mesh shaders support
 - D3D12 Ultimate features support, including enhanced barriers
 - VK [printf](https://github.com/KhronosGroup/Vulkan-ValidationLayers/blob/main/docs/debug_printf.md) support
 - validation layers (GAPI- and NRI- provided)
 - user provided memory allocator support
 - default D3D11 behavior is changed to match D3D12/VK using *NVAPI* or *AMD AGS* libraries, where applicable
 - supporting as much as possible VK-enabled platforms: Windows, Linux, MacOS, Android
 - debug names and annotations for CPU and GPU timelines (GAPI, [NVTX](https://github.com/NVIDIA/NVTX) and [PIX](https://devblogs.microsoft.com/pix/winpixeventruntime/), if "WinPixEventRuntime.dll" is nearby)
 - can be used as a *shared* or *static* library

Available interfaces:
 - `NRI.h` - core functionality
 - `NRIDeviceCreation.h` - device creation and related functionality
 - `NRIHelper.h` - a collection of various helpers to ease use of the core interface
 - `NRILowLatency.h` - low latency support (aka *NVIDIA REFLEX*)
 - `NRIMeshShader.h` - mesh shaders
 - `NRIRayTracing.h` - ray tracing
 - `NRIResourceAllocator.h` - convenient creation of resources using *AMD Virtual Memory Allocator*, which get returned already bound to memory
 - `NRIStreamer.h` - a convenient way to stream data into resources
 - `NRISwapChain.h` - swap chain and related functionality
 - `NRIUpscaler.h` - a configurable collection of common upscalers (NIS, FSR, DLSS-SR, DLSS-RR)

Repository organization:
- there is only `main` branch used for development
- stable versions are in `Releases` section
- it's recommended to use the latest release

*NRI* sample code:
 - [*NRI samples*](https://github.com/NVIDIA-RTX/NRISamples)
 - [*NRD sample*](https://github.com/NVIDIA-RTX/NRD-Sample)

<details>
<summary>Required Vulkan extensions:</summary>

- for Vulkan 1.2:
    - _VK_KHR_synchronization2_
    - _VK_KHR_dynamic_rendering_
    - _VK_KHR_copy_commands2_
    - _VK_EXT_extended_dynamic_state_
- for APPLE:
    - _VK_KHR_portability_enumeration_ (instance extension)
    - _VK_KHR_get_physical_device_properties2_ (instance extension)
    - _VK_KHR_portability_subset_

</details>

<details>
<summary>Supported Vulkan extensions:</summary>

- Instance:
    - _VK_KHR_get_surface_capabilities2_
    - _VK_KHR_surface_
    - _VK_KHR_win32_surface_ (_VK_KHR_xlib_surface_, _VK_KHR_wayland_surface_, _VK_EXT_metal_surface_)
    - _VK_EXT_swapchain_colorspace_
    - _VK_EXT_debug_utils_
- Device:
    - _VK_KHR_swapchain_
    - _VK_KHR_present_id_
    - _VK_KHR_present_wait_
    - _VK_KHR_swapchain_mutable_format_
    - _VK_KHR_maintenance4_ (for Vulkan 1.2)
    - _VK_KHR_maintenance5_
    - _VK_KHR_maintenance6_
    - _VK_KHR_fragment_shading_rate_
    - _VK_KHR_push_descriptor_
    - _VK_KHR_pipeline_library_
    - _VK_KHR_ray_tracing_pipeline_
    - _VK_KHR_acceleration_structure_ (depends on _VK_KHR_deferred_host_operations_)
    - _VK_KHR_ray_query_
    - _VK_KHR_ray_tracing_position_fetch_
    - _VK_KHR_ray_tracing_maintenance1_
    - _VK_KHR_line_rasterization_
    - _VK_KHR_fragment_shader_barycentric_
    - _VK_KHR_shader_clock_
    - _VK_EXT_opacity_micromap_
    - _VK_EXT_sample_locations_
    - _VK_EXT_conservative_rasterization_
    - _VK_EXT_mesh_shader_
    - _VK_EXT_shader_atomic_float_
    - _VK_EXT_shader_atomic_float2_
    - _VK_EXT_memory_budget_
    - _VK_EXT_memory_priority_
    - _VK_EXT_image_sliced_view_of_3d_
    - _VK_EXT_custom_border_color_
    - _VK_EXT_image_robustness_ (for Vulkan 1.2)
    - _VK_EXT_robustness2_
    - _VK_EXT_pipeline_robustness_
    - _VK_EXT_fragment_shader_interlock_
    - _VK_NV_low_latency2_
    - _VK_NVX_binary_import_
    - _VK_NVX_image_view_handle_

</details>

## C/C++ INTERFACE DIFFERENCES

| C++                   | C                     |
|-----------------------|-----------------------|
| `nri::Interface`      | `NriInterface`        |
| `nri::Enum::MEMBER`   | `NriEnum_MEMBER`      |
| `nri::CONST`          | `NRI_CONST`           |
| `nri::nriFunction`    | `nriFunction`         |
| `nri::Function`       | `nriFunction`         |
| Reference `&`         | Pointer `*`           |

## ENTITIES

| NRI                     | D3D11                                 | D3D12                         | VK                           |
|-------------------------|---------------------------------------|-------------------------------|------------------------------|
| `Device`                | `ID3D11Device`                        | `ID3D12Device`                | `VkDevice`                   |
| `CommandBuffer`         | `ID3D11DeviceContext` (deferred)      | `ID3D12CommandList`           | `VkCommandBuffer`            |
| `CommandQueue`          | `ID3D11DeviceContext` (immediate)     | `ID3D12CommandQueue`          | `VkQueue`                    |
| `Fence`                 | `ID3D11Fence`                         | `ID3D12Fence`                 | `VkSemaphore` (timeline)     |
| `CommandAllocator`      | N/A                                   | `ID3D12CommandAllocator`      | `VkCommandPool`              |
| `Buffer`                | `ID3D11Buffer`                        | `ID3D12Resource`              | `VkBuffer`                   |
| `Texture`               | `ID3D11Texture`                       | `ID3D12Resource`              | `VkImage`                    |
| `Memory`                | N/A                                   | `ID3D12Heap`                  | `VkDeviceMemory`             |
| `Descriptor`            | `ID3D11*View` or `ID3D11SamplerState` | `D3D12_CPU_DESCRIPTOR_HANDLE` | `Vk*View` or `VkSampler`     |
| `DescriptorSet`         | N/A                                   | N/A                           | `VkDescriptorSet`            |
| `DescriptorPool`        | N/A                                   | `ID3D12DescriptorHeap`        | `VkDescriptorPool`           |
| `PipelineLayout`        | N/A                                   | `ID3D12RootSignature`         | `VkPipelineLayout`           |
| `Pipeline`              | `ID3D11*Shader` and `ID3D11*State`    | `ID3D12StateObject`           | `VkPipeline`                 |
| `AccelerationStructure` | N/A                                   | `ID3D12Resource`              | `VkAccelerationStructure`    |

## BUILD INSTRUCTIONS

- Install [*Cmake*](https://cmake.org/download/) 3.18+
- Install on
    - Windows: latest *WindowsSDK* and *VulkanSDK*
    - Linux (x86-64): latest *VulkanSDK* and optional *libx11-dev* and *libwayland-dev*
    - Linux (aarch64): optional *libx11-dev* and *libwayland-dev*
- Build (variant 1) - using *Git* and *CMake* explicitly
    - Clone project and init submodules
    - Generate and build the project using *CMake*
    - To build the binary with static MSVC runtime, add `-DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>"` parameter when deploying the project
- Build (variant 2) - by running scripts:
    - Run `1-Deploy`
    - Run `2-Build`

Notes:
- *Xlib* and *Wayland* can be both enabled
- Minimal supported client is Windows 8.1+. Windows 7 support requires minimal effort and can be added by request

## CMAKE OPTIONS

- `NRI_STATIC_LIBRARY` - build NRI as a static library (`off` by default)
- `NRI_ENABLE_NVTX_SUPPORT` - annotations for NVIDIA Nsight Systems (`on` by default)
- `NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS` - enable debug names, host and device annotations (`on` by default)
- `NRI_ENABLE_NONE_SUPPORT` - enable NONE backend (`on` by default)
- `NRI_ENABLE_VK_SUPPORT` - enable VULKAN backend (`on` by default)
- `NRI_ENABLE_VALIDATION_SUPPORT` - enable Validation backend (otherwise `enableNRIValidation` is ignored, `on` by default)
- `NRI_ENABLE_D3D11_SUPPORT` - enable D3D11 backend (`on` by default on Windows)
- `NRI_ENABLE_D3D12_SUPPORT` - enable D3D12 backend (`on` by default on Windows)
- `NRI_ENABLE_AGILITY_SDK_SUPPORT` - enable D3D12 Agility SDK (`on` by default)
- `NRI_ENABLE_XLIB_SUPPORT` - enable *Xlib* support (`on` by default)
- `NRI_ENABLE_WAYLAND_SUPPORT` - enable *Wayland* support (`on` by default)
- `NRI_ENABLE_D3D_EXTENSIONS` - enable vendor specific extension libraries for D3D (NVAPI and AMD AGS) (`on` by default if there is a D3D backend)
- `NRI_ENABLE_NIS_SDK` - enable NVIDIA Image Sharpening SDK (`off` by default)
- `NRI_ENABLE_NGX_SDK` - enable NVIDIA NGX (DLSS) SDK (`off` by default)
- `NRI_ENABLE_FFX_SDK` - enable AMD FidelityFX SDK (`off` by default)
- `NRI_AGILITY_SDK_DIR` - directory where Agility SDK binaries will be copied to relative to `CMAKE_RUNTIME_OUTPUT_DIRECTORY` (`AgilitySDK` by default)
- `NRI_AGILITY_SDK_VERSION` - Agility SDK version

## AGILITY SDK

*Overview* and *Download* sections can be found [*here*](https://devblogs.microsoft.com/directx/directx12agility/).

D3D12 backend uses Agility SDK to get access to most recent D3D12 features.

Installation steps:
- set `NRI_AGILITY_SDK_VERSION_MAJOR` and `NRI_AGILITY_SDK_VERSION_MINOR` to the desired value
- enable `NRI_ENABLE_AGILITY_SDK_SUPPORT`
- re-deploy project
- include auto-generated `NRIAgilitySDK.h` header in the code of your executable using NRI

## SAMPLES OVERVIEW

NRI samples can be found [*here*](https://github.com/NVIDIA-RTX/NRISamples).

Samples:
- DeviceInfo - queries and prints out information about device groups in the system
- Clear - minimal example of rendering using framebuffer clears only
- CTest - very simple example of C interface usage
- Triangle - simple textured triangle rendering (also multiview demonstration in _FLEXIBLE_ mode)
- SceneViewer - loading & rendering of meshes with materials (also tests programmable sample locations, shading rate and pipeline statistics)
- BindlessSceneViewer - bindless GPU-driven rendering test
- Readback - getting data from the GPU back to the CPU
- AsyncCompute - demonstrates parallel execution of graphic and compute workloads
- MultiThreading - shows advantages of multi-threaded command buffer recording
- Multiview - multiview demonstration in _LAYER_BASED_ mode (VK and D3D12 compatible)
- MultiGPU - multi GPU example
- RayTracingTriangle - simple triangle rendering through ray tracing
- RayTracingBoxes - a more advanced ray tracing example with many BLASes in TLAS
- Wrapper - shows how to wrap native D3D11/D3D12/VK objects into NRI entities
- Resize - demonstrates window resize

## LICENSE

NRI is licensed under the MIT License. This project includes NVAPI software. All uses of NVAPI software are governed by the license terms specified [*here*](https://github.com/NVIDIA/nvapi/blob/main/License.txt).
