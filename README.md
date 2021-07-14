# NRI

## Description

NRI is a low-level abstract render interface which supports three backends: D3D11, D3D12, Vulkan.

## Build instructions
### Windows
1. Install **WindowsSDK**
2. Clone project and init submodules
3. Generate and build project using **cmake**

### Linux (x86-64/aarch64)
1. Install **libwayland-dev** or **libx11-dev** with **libxrandr-dev**
2. Clone project and init submodules
3. if **libwayland-dev** is not installed, add `-DNRI_DISABLE_WAYLAND_SUPPORT=ON` on step5
4. if **libx11-dev** is not installed, add `-DNRI_DISABLE_XLIB_SUPPORT=ON` on step5
5. Generate and build project using **cmake**

Note: xlib and wayland can be both enabled.

### CMake options
`-DNRI_DISABLE_INTERPROCEDURAL_OPTIMIZATION=ON` - disable interprocedural optimization

`-DNRI_DISABLE_XLIB_SUPPORT=ON` - disable xlib support

`-DNRI_DISABLE_WAYLAND_SUPPORT=ON` - disable wayland support

`-DNRI_STATIC_LIBRARY=ON` - build NRI as a static library
