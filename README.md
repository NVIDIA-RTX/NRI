# NVIDIA RENDER INTERFACE (NRI)

[![Status](https://github.com/NVIDIA-RTX/NRI/actions/workflows/build.yml/badge.svg)](https://github.com/NVIDIA-RTX/NRI/actions/workflows/build.yml)

*NRI* is a modular, extensible, low-level rendering interface for *D3D12*, *Vulkan*, *D3D11*, *WebGPU* and *Metal*, with an API suitable for console backends. It exposes low-level GAPI features while simplifying usage and reducing the amount of code needed (especially compared with *Vulkan*).

Goals:
- A consistent programming model for modern GAPIs, building on the explicit models of *D3D12* ([spec](https://microsoft.github.io/DirectX-Specs/)) and *Vulkan* ([spec](https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html))
- Explicitness (providing access to low-level features of modern GAPIs) with less code
- Practical helpers and extensions that simplify rendering tasks (e.g., data streaming) and feature integration (e.g., upscalers)
- Low CPU overhead
- Cross-platform and vendor-independent support (AMD/Intel and console friendly)
- *D3D11* ([spec](https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm)) and *WGPU* support where practical

Non-goals:
- Exposing entities that do not exist in GAPIs
- High-level (*D3D11*-like) abstraction
- Hidden management (except where explicitly provided by high-level extensions)
- Automatic barriers (better handled by a higher-level abstraction)

Supported GAPIs:
- *Vulkan 1.4+, 1.3++ or 1.2+++* - older core versions require more extensions, with a *Vulkan 1.2* + _VK_KHR_synchronization2_ baseline
- *D3D12*
- *D3D11*
- *WebGPU* (through [wgpu-native](https://github.com/gfx-rs/wgpu-native))
- *Metal* (through [MoltenVK](https://github.com/KhronosGroup/MoltenVK))
- *Metal 4* (native, through [metal-cpp](https://developer.apple.com/metal/cpp/))
- None / dummy (everything is supported but does nothing)

## WHY NRI?

There is [NVRHI](https://github.com/NVIDIA-RTX/NVRHI), which offers a higher-level abstraction with optional automatic barriers and automatic resource lifetime management, reducing application-side bookkeeping. In our measured workloads, NVRHI ([`2e21b39`](https://github.com/NVIDIA-RTX/NVRHI/commit/2e21b39c1eddd9661cf6728b165c132412708869), 2026.10.27) was 3–8× slower on the CPU than *NRI*. There is [vkd3d-proton](https://github.com/HansKristian-Work/vkd3d-proton), which implements *D3D12* using *Vulkan*. There is [nvpro-samples](https://github.com/nvpro-samples), which provides *Vulkan*-focused samples and tutorials. These projects address different needs.

*NRI* was designed to offer a reasonably simple, low-overhead, and high-performance render interface suitable for game development, professional rendering and hobby projects. Additionally, *NRI* can serve as middleware for integrations. For instance, the [NRD integration layer](https://github.com/NVIDIA-RTX/NRD/tree/master/Integration) is based on *NRI*, providing multi-GAPI support within a single codebase.

*NRI* is console friendly. Console backends for *Nintendo Switch (NVN)*, *PlayStation 5 (AGC)* and *Xbox (D3D12.x)* are possible. These are not included in this public repository and cannot be maintained by NVIDIA. Authorized developers may maintain them in access-controlled repositories. Generic API fixes for console portability are welcome here.

## KEY FEATURES
 - *Multi-API support* - a unified interface for *D3D12*, *Vulkan*, *D3D11*, *WebGPU*, and native *Metal 4* (default *D3D11* behavior is changed to match *D3D12*/VK using *NVAPI* or **AMD AGS** libraries, where applicable)
 - *C/C++ compatible* - native interface, compatible with both languages (can be used as a *shared* or *static* library)
 - *High performance* - low overhead, with support for user-provided memory allocators
 - *Modern hardware tech* - support for Ray Tracing, Mesh Shaders, descriptor indexing (including directly indexed descriptor heaps) and more
 - *Mobile-ready* - optimized for Tile-Based Rendering (TBR/TBDR) architectures and friendly to mobile-oriented API designs
 - *Portable API shape* - explicit resource, command and synchronization models suitable for console GAPIs, with capability reporting for optional features
 - *D3D12 Ultimate* - full support, including Enhanced Barriers
 - *Platform Flexibility* - Windows, Linux, macOS, iOS, and Android support
 - *Developer Experience* - integrated GAPI and NRI validation, VK [printf](https://github.com/KhronosGroup/Vulkan-ValidationLayers/blob/main/docs/debug_printf.md), and timeline annotations (GAPI, [NVTX](https://github.com/NVIDIA/NVTX) and [PIX](https://devblogs.microsoft.com/pix/winpixeventruntime/), if "WinPixEventRuntime.dll" is nearby)
 - *Memory Management* - integration with *Vulkan Memory Allocator* and *D3D12 Memory Allocator* to ease memory management and memory reuse

Available interfaces:
 - `NRI.h` - core functionality
 - `NRIDeviceCreation.h` - device creation and related functionality
 - `NRIHelper.h` - a collection of various helpers to ease use of the core interface
 - `NRIDescriptorHeap.h` - directly indexed descriptor heaps
 - `NRIImgui.h` - a lightweight *ImGui* renderer (no *ImGui* dependency)
 - `NRILowLatency.h` - low latency support (aka *NVIDIA REFLEX*)
 - `NRIMeshShader.h` - mesh shaders
 - `NRIRayTracing.h` - ray tracing
 - `NRIStreamer.h` - a convenient way to stream data into resources
 - `NRISwapChain.h` - swap chain and related functionality
 - `NRIUpscaler.h` - a configurable collection of common upscalers (*NIS*, *FSR*, *XeSS*, *DLSS-SR*, *DLSS-RR*, *MetalFX*)
 - `NRIVideo.h` - hardware video encoding and decoding
 - `NRIWrapperD3D11.h`, `NRIWrapperD3D12.h`, `NRIWrapperVK.h`, `NRIWrapperMetal.h` - interoperability with native GAPI devices and resources

Repository organization:
- development takes place on the `main` branch
- stable versions are available in the `Releases` section

## BUILD INSTRUCTIONS

- Install [*CMake*](https://cmake.org/download/) 3.30+
- macOS: install prerequisites
    - Xcode command line tools
    - Ninja
    - [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#mac), then source its `setup-env.sh`
- Build (variant 1) - using *Git* and *CMake* explicitly
    - Clone the project and initialize submodules
    - Generate and build the project using *CMake*
    - To build the binary with the static MSVC runtime, add `-DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>"` parameter when configuring the project
- Build (variant 2) - by running scripts:
    - To deploy the project, run `Scripts/<Platform>/1-Deploy`
    - To build the project, run `Scripts/<Platform>/2-Build`
    - To package the SDK, run `Scripts/<Platform>/3-PrepareSDK`
    - To clean generated files, run `Scripts/<Platform>/4-Clean`

`<Platform>` is `Windows`, `Linux` or `MacOS`. Use `.bat` on Windows; run `.sh` scripts with `bash` on Linux and macOS.

Scripts resolve the NRI root from their own location, so they can be launched from any working directory.

Notes:
- *Xlib* and *Wayland* can both be enabled
- Minimum supported Windows version is Windows 8.1 (backend requirements may be higher)
- *Metal 4* backend requires macOS 26+ or iOS 26+ (iPadOS included) and Xcode 26+ (with `MetalToolchain` component), *metal-cpp* is fetched automatically, native `.metallib` shaders must follow `NRI.metal` ABI, *DXIL* support requires *Metal Shader Converter* 4.0.1 (macOS and iOS devices, no simulator), offline converted *DXIL* (*ShaderMake* `--metalFromDXIL` bundles) works everywhere (see `NRIWrapperMetal.h`)
- *iOS*: `-DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos|iphonesimulator -DCMAKE_OSX_ARCHITECTURES=arm64` (a separate build folder per SDK, with the *Xcode* generator *MetalFX* linking follows `CMAKE_OSX_SYSROOT`, static library by default, no *VK*, *WebGPU*, *NVTX* and *NIS*, no *MetalFX* and *Metal Shader Converter* in the simulator)
- static `NRI` provides *metal-cpp* private implementation (`NS/MTL/CA/MTLFX_PRIVATE_IMPLEMENTATION`), set `NRI_METAL_CPP_PRIVATE_IMPLEMENTATION = OFF` if the app compiles it itself
- *Metal Shader Converter* is required for building if `NRI_ENABLE_METAL_SHADER_CONVERTER` is on (`/usr/local/lib` on macOS, `/usr/local/lib_iOS` for iOS devices). Apple's *Metal Shader Converter* license permits distributing its dynamic libraries for shader conversion:
    - shared `NRI` copies `libmetalirconverter.dylib` next to itself and finds it via `@loader_path` or the app `Frameworks` folder, `3-PrepareSDK` puts it into `Lib` (without Apple's license text, which comes with the *Metal Shader Converter* installer)
    - static `NRI` leaves bundling to the app, for example:
    ```cmake
    set_target_properties(App PROPERTIES BUILD_RPATH "@executable_path" INSTALL_RPATH "@executable_path")
    add_custom_command(TARGET App POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${NRI_METAL_SHADER_CONVERTER_LIBRARY}" "$<TARGET_FILE_DIR:App>")
    ```
    - iOS: embed `libmetalirconverter.dylib` into the app `Frameworks` folder (`@executable_path/Frameworks`) and sign it, the *App Store* may require wrapping it into a `.framework`

## CMAKE OPTIONS

- `NRI_AGILITY_SDK_DIR` - Directory where *Agility SDK* will be copied relative to the directory with binaries
- `NRI_AGILITY_SDK_VERSION_MAJOR` - *Agility SDK* major version
- `NRI_AGILITY_SDK_VERSION_MINOR` - *Agility SDK* minor version
- `NRI_SHADERS_PATH` - Shader output path override
- `NRI_NVAPI_CUSTOM_PATH` - Path to a custom NVAPI library directory
- `NRI_METAL_SHADER_CONVERTER_INCLUDE_PATH` - Path to a custom *Metal Shader Converter* include directory
- `NRI_METAL_SHADER_CONVERTER_LIBRARY` - Path to a custom *Metal Shader Converter* library
- `NRI_STATIC_LIBRARY` - Build static library (`ON` by default on iOS)
- `NRI_ENABLE_NVTX_SUPPORT` - Annotations for NVIDIA Nsight Systems
- `NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS` - Enable debug names, host and device annotations
- `NRI_ENABLE_NONE_SUPPORT` - Enable NONE backend
- `NRI_ENABLE_VK_SUPPORT` - Enable *Vulkan* backend
- `NRI_ENABLE_WGPU_SUPPORT` - Enable *WebGPU* backend through *wgpu-native*
- `NRI_ENABLE_METAL_SUPPORT` - Enable *Metal 4* backend (enabled by default with Xcode 26+ and its `MetalToolchain` component)
- `NRI_ENABLE_METAL_SHADER_CONVERTER` - Enable *DXIL* support in *Metal* backend through *Metal Shader Converter* (enabled by default if installed)
- `NRI_METAL_CPP_PRIVATE_IMPLEMENTATION` - Compile *metal-cpp* private implementation into `NRI` (can be disabled only for a static library)
- `NRI_ENABLE_VALIDATION_SUPPORT` - Enable Validation backend (otherwise `enableNRIValidation` is ignored)
- `NRI_ENABLE_NIS_SDK` - Enable NVIDIA Image Scaling SDK
- `NRI_ENABLE_IMGUI_EXTENSION` - Enable `NRIImgui` extension
- `NRI_STREAMER_THREAD_SAFE` - `NRIStreamer` thread safety (`OFF` is faster)
- `NRI_ENABLE_D3D11_SUPPORT` - Enable *D3D11* backend
- `NRI_ENABLE_D3D12_SUPPORT` - Enable *D3D12* backend
- `NRI_ENABLE_AMDAGS` - Enable *AMD AGS* library for D3D
- `NRI_ENABLE_NVAPI` - Enable NVAPI library for D3D
- `NRI_ENABLE_AGILITY_SDK_SUPPORT` - Enable *Agility SDK* support to unlock access to latest *D3D12* features
- `NRI_ENABLE_XLIB_SUPPORT` - Enable X11 support
- `NRI_ENABLE_WAYLAND_SUPPORT` - Enable Wayland support
- `NRI_ENABLE_NGX_SDK` - Enable *NVIDIA NGX (DLSS)* SDK
- `NRI_ENABLE_FFX_SDK` - Enable *AMD FidelityFX* SDK
- `NRI_ENABLE_XESS_SDK` - Enable *INTEL XeSS* SDK

## AGILITY SDK

The minimum build requirement for the *D3D12* backend is *Windows SDK 10.0.20348* (with *D3D12 Ultimate* headers, the last pre-*Agility SDK* release). *NRI* can be compiled for this *Windows SDK* using the `Scripts/Windows/1-Deploy.bat -DNRI_ENABLE_AGILITY_SDK_SUPPORT=OFF -A "x64,version=10.0.20348.0"` command line. But using the latest *Agility SDK* is highly recommended to get access to the most recent *D3D12* features and improved validation. See *Overview* and *Download* sections [*here*](https://devblogs.microsoft.com/directx/directx12agility/) for more details about *Agility SDK*.

*Agility SDK* configuration (support is enabled by default):
- modify `NRI_AGILITY_SDK_VERSION_MAJOR` and `NRI_AGILITY_SDK_VERSION_MINOR` to the desired value
- enable or disable `NRI_ENABLE_AGILITY_SDK_SUPPORT`
- re-deploy the project
- (optional) include the auto-generated `NRIAgilitySDK.h` header in the code of your executable using *NRI*

It's safe to compile *NRI* using the default *Agility SDK* and export a specific `D3D12SDKVersion` in the executable using *NRI*. In this case, *NRI* queries the latest interfaces available in the loaded `D3D12Core.dll`.

## SUPPORTED VULKAN EXTENSIONS

Required for Vulkan 1.2 (core in 1.3+):
- _VK_KHR_synchronization2_

Required (Apple):
- _VK_KHR_portability_enumeration_, _VK_KHR_get_physical_device_properties2_, _VK_KHR_portability_subset_

Supported (instance):
- _VK_KHR_get_surface_capabilities2_, _VK_KHR_surface_, _VK_KHR_win32_surface_, _VK_KHR_xlib_surface_, _VK_KHR_wayland_surface_, _VK_KHR_android_surface_, _VK_EXT_metal_surface_, _VK_EXT_swapchain_colorspace_, _VK_EXT_debug_utils_, _VK_EXT_surface_maintenance1_

Supported:
- _VK_KHR_external_memory_win32_, _VK_KHR_external_semaphore_win32_, _VK_KHR_external_memory_fd_, _VK_KHR_external_semaphore_fd_, _VK_KHR_copy_commands2_, _VK_KHR_dynamic_rendering_, _VK_KHR_format_feature_flags2_, _VK_KHR_swapchain_, _VK_KHR_present_id_, _VK_KHR_present_wait_, _VK_KHR_swapchain_mutable_format_, _VK_KHR_push_descriptor_, _VK_KHR_maintenance4_, _VK_KHR_maintenance5_, _VK_KHR_maintenance6_, _VK_KHR_maintenance7_, _VK_KHR_maintenance8_, _VK_KHR_maintenance9_, _VK_KHR_maintenance10_, _VK_KHR_extended_flags_, _VK_KHR_line_rasterization_, _VK_KHR_fragment_shading_rate_, _VK_KHR_pipeline_library_, _VK_KHR_ray_tracing_pipeline_, _VK_KHR_acceleration_structure_ (depends on _VK_KHR_deferred_host_operations_), _VK_KHR_ray_query_, _VK_KHR_ray_tracing_position_fetch_, _VK_KHR_ray_tracing_maintenance1_, _VK_KHR_fragment_shader_barycentric_, _VK_KHR_shader_clock_, _VK_KHR_compute_shader_derivatives_, _VK_KHR_unified_image_layouts_, _VK_KHR_shader_integer_dot_product_, _VK_KHR_shader_untyped_pointers_, _VK_KHR_sampler_ycbcr_conversion_, _VK_KHR_load_store_op_none_, _VK_KHR_dynamic_rendering_local_read_, _VK_EXT_extended_dynamic_state_, _VK_EXT_calibrated_timestamps_, _VK_EXT_pipeline_creation_cache_control_, _VK_EXT_pipeline_robustness_, _VK_EXT_host_image_copy_, _VK_EXT_load_store_op_none_, _VK_EXT_image_robustness_, _VK_EXT_subgroup_size_control_, _VK_EXT_mutable_descriptor_type_, _VK_EXT_descriptor_heap_, _VK_EXT_swapchain_maintenance1_, _VK_EXT_present_mode_fifo_latest_ready_, _VK_EXT_opacity_micromap_, _VK_EXT_sample_locations_, _VK_EXT_conservative_rasterization_, _VK_EXT_mesh_shader_, _VK_EXT_shader_atomic_float_, _VK_EXT_shader_atomic_float2_, _VK_EXT_memory_budget_, _VK_EXT_memory_priority_, _VK_EXT_image_sliced_view_of_3d_, _VK_EXT_custom_border_color_, _VK_EXT_robustness2_, _VK_EXT_fragment_shader_interlock_, _VK_EXT_zero_initialize_device_memory_, _VK_EXT_device_fault_, _VK_KHR_video_queue_, _VK_KHR_video_decode_queue_, _VK_KHR_video_encode_queue_, _VK_KHR_video_decode_h264_, _VK_KHR_video_decode_h265_, _VK_KHR_video_decode_av1_, _VK_KHR_video_encode_h264_, _VK_KHR_video_encode_h265_, _VK_KHR_video_encode_av1_, _VK_KHR_video_maintenance1_, _VK_KHR_video_maintenance2_, _VK_NV_low_latency2_, _VK_NVX_binary_import_, _VK_NVX_image_view_handle_

## SAMPLES OVERVIEW

[*NRD sample*](https://github.com/NVIDIA-RTX/NRD-Sample):
- main sample demonstrating path tracing best practices

[*NRI samples*](https://github.com/NVIDIA-RTX/NRISamples):
- samples demonstrating basic usage and exercising individual features

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

| NRI                     | *D3D11*                                 | *D3D12*                         | VK                           | WGPU                                      | *Metal 4*                                               |
|-------------------------|-----------------------------------------|---------------------------------|------------------------------|-------------------------------------------|---------------------------------------------------------|
| `Device`                | `ID3D11Device`                          | `ID3D12Device`                  | `VkDevice`                   | `WGPUDevice`                              | `MTL::Device`                                           |
| `CommandBuffer`         | `ID3D11DeviceContext` (deferred)        | `ID3D12CommandList`             | `VkCommandBuffer`            | `WGPUCommandBuffer`                       | `MTL4::CommandBuffer`                                   |
| `Queue`                 | `ID3D11DeviceContext` (immediate)       | `ID3D12CommandQueue`            | `VkQueue`                    | `WGPUQueue`                               | `MTL4::CommandQueue`                                    |
| `Fence`                 | `ID3D11Fence`                           | `ID3D12Fence`                   | `VkSemaphore` (timeline)     | N/A                                       | `MTL::SharedEvent`                                      |
| `CommandAllocator`      | N/A                                     | `ID3D12CommandAllocator`        | `VkCommandPool`              | N/A                                       | `MTL4::CommandAllocator`                                |
| `Buffer`                | `ID3D11Buffer`                          | `ID3D12Resource`                | `VkBuffer`                   | `WGPUBuffer`                              | `MTL::Buffer`                                           |
| `Texture`               | `ID3D11Texture`                         | `ID3D12Resource`                | `VkImage`                    | `WGPUTexture`                             | `MTL::Texture`                                          |
| `Memory`                | N/A                                     | `ID3D12Heap`                    | `VkDeviceMemory`             | N/A                                       | `MTL::Heap`                                             |
| `Descriptor`            | `ID3D11*View` or `ID3D11SamplerState`   | `D3D12_CPU_DESCRIPTOR_HANDLE`   | `Vk*View` or `VkSampler`     | `WGPUTextureView`, `WGPUBuffer`, `WGPUSampler` | `MTL::Texture`, `MTL::Buffer`, `MTL::SamplerState`      |
| `DescriptorSet`         | N/A                                     | N/A                             | `VkDescriptorSet`            | `WGPUBindGroup`                           | N/A                                                     |
| `DescriptorPool`        | N/A                                     | `ID3D12DescriptorHeap`          | `VkDescriptorPool`           | N/A                                       | `MTL::Buffer`                                           |
| `PipelineLayout`        | N/A                                     | `ID3D12RootSignature`           | `VkPipelineLayout`           | `WGPUPipelineLayout`                      | N/A                                                     |
| `Pipeline`              | `ID3D11*Shader` and `ID3D11*State`      | `ID3D12PipelineState` / `ID3D12StateObject` | `VkPipeline`                 | `WGPURenderPipeline` / `WGPUComputePipeline` | `MTL::RenderPipelineState` / `MTL::ComputePipelineState` |
| `AccelerationStructure` | N/A                                     | `ID3D12Resource`                | `VkAccelerationStructure`    | N/A                                       | `MTL::AccelerationStructure`                            |

## LICENSE

*NRI* is licensed under the MIT License.
