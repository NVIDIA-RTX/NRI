# NVIDIA Render Interface v1.87 (NRI)

*NRI* is a low-level abstract render interface which currently supports three backends: D3D11, D3D12 and Vulkan. *NRI* has been designed to support all (at least major) low level features of D3D12 and Vulkan APIs, but at the same time to simplify usage and reduce the amount of code needed (especially compared with Vulkan).

 Key features:
 - common denominator for D3D11, D3D12 and Vulkan APIs
 - low overhead
 - ray tracing support
 - API-provided and/or own validation layer
 - default D3D11 behavior is changed to match D3D12/VK where applicable using *NVAPI* or *AMD AGS* libraries

 *NRI* is used in:
 - [*NRI samples*](https://github.com/NVIDIAGameWorks/NRISamples)
 - [*NRD Sample*](https://github.com/NVIDIAGameWorks/NRDSample)

## Build instructions

- Install [*Cmake*](https://cmake.org/download/) 3.15+
- Install on
    - Windows: *WindowsSDK*, *VulkanSDK*
    - Linux (x86-64): *VulkanSDK* and optional *libx11-dev* and *libwayland-dev*
    - Linux (aarch64): optional *libx11-dev* and *libwayland-dev*
- Build (variant 1) - using *Git* and *CMake* explicitly
    - Clone project and init submodules
    - Generate and build the project using *CMake*
- Build (variant 2) - by running scripts:
    - Run `1-Deploy`
    - Run `2-Build`

Note: *Xlib* and *Wayland* can be both enabled.

### CMake options

- `NRI_DISABLE_INTERPROCEDURAL_OPTIMIZATION=ON` - disable interprocedural optimization
- `NRI_DISABLE_XLIB_SUPPORT=ON` - disable *Xlib* support
- `NRI_DISABLE_WAYLAND_SUPPORT=ON` - disable *Wayland* support
- `NRI_STATIC_LIBRARY=ON` - build NRI as a static library
