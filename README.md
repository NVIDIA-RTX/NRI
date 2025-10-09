# NVIDIA RENDER INTERFACE (NRI)

[![Status](https://github.com/NVIDIA-RTX/NRI/actions/workflows/build.yml/badge.svg)](https://github.com/NVIDIA-RTX/NRI/actions/workflows/build.yml)

*NRI* is a modular, extensible, low-level abstract rendering interface that was designed to support all low-level features of D3D12 and Vulkan GAPIs. At the same time, it aims to simplify usage and reduce the amount of code needed (especially compared with VK).

Goals:
- Generalization and unification of D3D12 ([spec](https://microsoft.github.io/DirectX-Specs/)) and VK ([spec](https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html))
- Explicitness (providing access to low-level features of modern GAPIs)
- Quality-of-life and high-level extensions (e.g., streaming and upscaling)
- Low overhead
- Cross-platform and platform independence (AMD/INTEL friendly)
- D3D11 support ([spec](https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm)) (as much as possible)

Non-goals:
- Exposing entities not existing in GAPIs
- High-level (D3D11-like) abstraction
- Hidden management of any kind (except for some high-level extensions where it's desired)
- Automatic barriers (better handled in a higher-level abstraction)

Supported GAPIs:
- Vulkan
- D3D12
- D3D11
- Metal (through [MoltenVK](https://github.com/KhronosGroup/MoltenVK))
- None / dummy (everything is supported but does nothing)

## WHY NRI?

There is [NVRHI](https://github.com/NVIDIA-RTX/NVRHI), which offers a D3D11-like abstraction layer, implying some overhead. There is [vkd3d-proton](https://github.com/HansKristian-Work/vkd3d-proton), which was designed for D3D12 emulation using Vulkan. There is [nvpro-samples](https://github.com/nvpro-samples), which explores all "dark corners" of Vulkan usage, but does not offer any cross-API support. There are some other good, unmentioned projects, but *NRI* was designed to offer a reasonably simple, low-overhead, and high-performance render interface suitable for game development, professional rendering and hobby projects. Additionally, *NRI* can serve as a middleware for integrations. For instance, the [NRD integration layer](https://github.com/NVIDIA-RTX/NRD/tree/master/Integration) is based on *NRI*, unifying multiple GAPI support within a single codebase.

## KEY FEATURES:
 - *C++* and *C* compatible interfaces
 - generalized common denominator for D3D12, VK and D3D11
 - low overhead
 - no memory allocations at runtime
 - descriptor indexing support, including directly indexed descriptor heaps
 - ray tracing support
 - mesh shaders support
 - D3D12 Ultimate features support, including enhanced barriers
 - *AMD Virtual Memory Allocator* support
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
 - `NRIImgui.h` - a light-weight ImGui renderer (no ImGui dependency)
 - `NRILowLatency.h` - low latency support (aka *NVIDIA REFLEX*)
 - `NRIMeshShader.h` - mesh shaders
 - `NRIRayTracing.h` - ray tracing
 - `NRIStreamer.h` - a convenient way to stream data into resources
 - `NRISwapChain.h` - swap chain and related functionality
 - `NRIUpscaler.h` - a configurable collection of common upscalers (NIS, FSR, DLSS-SR, DLSS-RR)

Repository organization:
- there is only `main` branch used for development
- stable versions are in `Releases` section

<details>
<summary>Required Vulkan extensions:</summary>

- for Vulkan < 1.3:
    - _VK_KHR_synchronization2_
    - _VK_KHR_dynamic_rendering_
    - _VK_KHR_copy_commands2_
    - _VK_KHR_maintenance4_ (optional, but recommended)
    - _VK_EXT_extended_dynamic_state_
- for Vulkan < 1.4:
    - _VK_KHR_push_descriptor
    - _VK_KHR_maintenance5 (optional, but recommended)
    - _VK_KHR_maintenance6 (optional, but recommended)
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
    - _VK_KHR_maintenance7_
    - _VK_KHR_maintenance8_
    - _VK_KHR_maintenance9_
    - _VK_KHR_fragment_shading_rate_
    - _VK_KHR_pipeline_library_
    - _VK_KHR_ray_tracing_pipeline_
    - _VK_KHR_acceleration_structure_ (depends on _VK_KHR_deferred_host_operations_)
    - _VK_KHR_ray_query_
    - _VK_KHR_ray_tracing_position_fetch_
    - _VK_KHR_ray_tracing_maintenance1_
    - _VK_KHR_line_rasterization_
    - _VK_KHR_fragment_shader_barycentric_
    - _VK_KHR_shader_clock_
    - _VK_KHR_compute_shader_derivatives_
    - _VK_EXT_mutable_descriptor_type_
    - _VK_EXT_subgroup_size_control_
    - _VK_EXT_swapchain_maintenance1_
    - _VK_EXT_present_mode_fifo_latest_ready_
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
    - _VK_EXT_image_robustness_
    - _VK_EXT_robustness2_
    - _VK_EXT_pipeline_robustness_
    - _VK_EXT_fragment_shader_interlock_
    - _VK_NV_low_latency2_
    - _VK_NVX_binary_import_
    - _VK_NVX_image_view_handle_

</details>

## BUILD INSTRUCTIONS

- Install [*Cmake*](https://cmake.org/download/) 3.30+
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

- `NRI_AGILITY_SDK_DIR` - Directory where Agility SDK will be copied relative to the directory with binaries
- `NRI_AGILITY_SDK_VERSION_MAJOR`- Agility SDK major version
- `NRI_AGILITY_SDK_VERSION_MINOR` - Agility SDK minor version
- `NRI_SHADERS_PATH` - Shader output path override
- `NRI_NVAPI_CUSTOM_PATH` - Path to a custom NVAPI library directory
- `NRI_STATIC_LIBRARY` - Build static library
- `NRI_ENABLE_NVTX_SUPPORT` - Annotations for NVIDIA Nsight Systems
- `NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS` - Enable debug names, host and device annotations
- `NRI_ENABLE_NONE_SUPPORT` - Enable NONE backend
- `NRI_ENABLE_VK_SUPPORT` - Enable Vulkan backend
- `NRI_ENABLE_VALIDATION_SUPPORT` - Enable Validation backend (otherwise `enableNRIValidation` is ignored)
- `NRI_ENABLE_NIS_SDK` - Enable NVIDIA Image Sharpening SDK
- `NRI_ENABLE_IMGUI_EXTENSION` - Enable `NRIImgui` extension
- `NRI_ENABLE_D3D11_SUPPORT` - Enable D3D11 backend
- `NRI_ENABLE_D3D12_SUPPORT` - Enable D3D12 backend
- `NRI_ENABLE_AMDAGS`- Enable AMD AGS library for D3D
- `NRI_ENABLE_NVAPI` - Enable NVAPI library for D3D
- `NRI_ENABLE_AGILITY_SDK_SUPPORT` - Enable Agility SDK support to unlock access to latest D3D12 features
- `NRI_ENABLE_XLIB_SUPPORT` - Enable X11 support
- `NRI_ENABLE_WAYLAND_SUPPORT` - Enable Wayland support
- `NRI_ENABLE_NGX_SDK` - Enable NVIDIA NGX (DLSS) SDK
- `NRI_ENABLE_FFX_SDK` - Enable AMD FidelityFX SDK
- `NRI_ENABLE_XESS_SDK` - Enable INTEL XeSS SDK

## AGILITY SDK

*Overview* and *Download* sections can be found [*here*](https://devblogs.microsoft.com/directx/directx12agility/).

D3D12 backend uses Agility SDK to get access to most recent D3D12 features and improved validation. It's highly recommended to use the latest Agility SDK.

Steps (already enabled by default):
- modify `NRI_AGILITY_SDK_VERSION_MAJOR` and `NRI_AGILITY_SDK_VERSION_MINOR` to the desired value
- enable or disable `NRI_ENABLE_AGILITY_SDK_SUPPORT`
- re-deploy project
- include auto-generated `NRIAgilitySDK.h` header in the code of your executable using *NRI*

## SAMPLES OVERVIEW

[*NRD sample*](https://github.com/NVIDIA-RTX/NRD-Sample):
- main sample demonstrating path tracing best practices

[*NRI samples*](https://github.com/NVIDIA-RTX/NRISamples):
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
- Wrapper - shows how to wrap native D3D11/D3D12/VK objects into *NRI* entities
- Resize - demonstrates window resize

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

## LICENSE

*NRI* is licensed under the MIT License.
