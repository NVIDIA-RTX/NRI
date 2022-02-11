# NVIDIA Rendering Interface v1.77 (NRI)

NRI is a low-level abstract render interface which supports three backends: D3D11, D3D12, Vulkan.

## Build instructions

- Install [*Cmake*](https://cmake.org/download/) 3.15+
- Install on
    - Windows: *WindowsSDK*, *VulkanSDK*
    - Linux (x86-64): *VulkanSDK* and optional *libx11-dev* and *libwayland-dev*
    - Linux (aarch64): optional *libx11-dev* and *libwayland-dev*
- Build (variant 1) - using *Git* and *CMake* explicitly
    - Clone project and init submodules
    - Generate and build project using *CMake*
- Build (variant 2) - by running scripts:
    - Run `1-Deploy`
    - Run `2-Build`

Note: *Xlib* and *Wayland* can be both enabled.

### CMake options

- `NRI_DISABLE_INTERPROCEDURAL_OPTIMIZATION=ON` - disable interprocedural optimization
- `NRI_DISABLE_XLIB_SUPPORT=ON` - disable *Xlib* support
- `NRI_DISABLE_WAYLAND_SUPPORT=ON` - disable *Wayland* support
- `NRI_STATIC_LIBRARY=ON` - build NRI as a static library
