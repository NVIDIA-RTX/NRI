// © 2021 NVIDIA Corporation

#pragma once

NriNamespaceBegin

NriForwardStruct(SwapChain);

static const uint32_t NriConstant(OUT_OF_DATE) = (uint32_t)(-1); // VK only: swap chain is out of date

// Color space:
//  - BT.709 - LDR https://en.wikipedia.org/wiki/Rec._709
//  - BT.2020 - HDR https://en.wikipedia.org/wiki/Rec._2020
// Transfer function:
//  - G10 - linear (gamma 1.0)
//  - G22 - sRGB (gamma ~2.2)
//  - G2084 - SMPTE ST.2084 (Perceptual Quantization)
// Bits per channel:
//  - 8, 10, 16 (float)
NriEnum(SwapChainFormat, uint8_t,
    BT709_G10_16BIT,
    BT709_G22_8BIT,
    BT709_G22_10BIT,
    BT2020_G2084_10BIT
);

NriStruct(WindowsWindow) {
    void* hwnd; // HWND
};

NriStruct(X11Window) {
    void* dpy; // Display*
    uint64_t window; // Window
};

NriStruct(WaylandWindow) {
    void* display; // wl_display*
    void* surface; // wl_surface*
};

NriStruct(MetalWindow) {
    void* caMetalLayer; // CAMetalLayer*
};

NriStruct(Window) {
    // Only one entity must be initialized
    Nri(WindowsWindow) windows;
    Nri(MetalWindow) metal;
    Nri(X11Window) x11;
    Nri(WaylandWindow) wayland;
};

// SwapChain textures will be created as "color attachment" resources
// queuedFrameNum = 0 - auto-selection between 1 (for waitable) or 2 (otherwise)
// queuedFrameNum = 2 - recommended if the GPU frame time is less than the desired frame time, but the sum of 2 frames is greater
NriStruct(SwapChainDesc) {
    Nri(Window) window;
    const NriPtr(Queue) queue;      // GRAPHICS or COMPUTE (requires "isPresentFromComputeSupported")
    Nri(Dim_t) width;
    Nri(Dim_t) height;
    uint8_t textureNum;
    Nri(SwapChainFormat) format;
    uint8_t verticalSyncInterval;   // 0 - vsync off
    uint8_t queuedFrameNum;         // aka "max frame latency", aka "number of frames in flight" (mostly for D3D11)
    bool waitable;                  // allows to use "WaitForPresent", which helps to reduce latency (requires "isWaitableSwapChainSupported")
    bool allowLowLatency;           // unlocks "NRILowLatency" functionality (requires "isLowLatencySupported")
};

NriStruct(ChromaticityCoords) {
    float x, y; // [0; 1]
};

// Describes color settings and capabilities of the closest display:
//  - Luminance provided in nits (cd/m2)
//  - SDR = standard dynamic range
//  - LDR = low dynamic range (in many cases LDR == SDR)
//  - HDR = high dynamic range, assumes G2084:
//      - BT709_G10_16BIT: HDR gets enabled and applied implicitly if Windows HDR is enabled
//      - BT2020_G2084_10BIT: HDR requires explicit color conversions and enabled HDR in Windows
//  - "SDR scale in HDR mode" = sdrLuminance / 80
NriStruct(DisplayDesc) {
    Nri(ChromaticityCoords) redPrimary;
    Nri(ChromaticityCoords) greenPrimary;
    Nri(ChromaticityCoords) bluePrimary;
    Nri(ChromaticityCoords) whitePoint;
    float minLuminance;
    float maxLuminance;
    float maxFullFrameLuminance;
    float sdrLuminance;
    bool isHDR;
};

// Threadsafe: no
NriStruct(SwapChainInterface) {
    Nri(Result)             (NRI_CALL *CreateSwapChain)             (NriRef(Device) device, const NriRef(SwapChainDesc) swapChainDesc, NriOut NriRef(SwapChain*) swapChain);
    void                    (NRI_CALL *DestroySwapChain)            (NriRef(SwapChain) swapChain);
    NriPtr(Texture) const*  (NRI_CALL *GetSwapChainTextures)        (const NriRef(SwapChain) swapChain, NriOut NonNriRef(uint32_t) textureNum);
    uint32_t                (NRI_CALL *AcquireNextSwapChainTexture) (NriRef(SwapChain) swapChain); // can return OUT_OF_DATE (VK only)
    Nri(Result)             (NRI_CALL *WaitForPresent)              (NriRef(SwapChain) swapChain); // call once right before input sampling (must be called starting from the 1st frame)
    Nri(Result)             (NRI_CALL *QueuePresent)                (NriRef(SwapChain) swapChain);
    Nri(Result)             (NRI_CALL *GetDisplayDesc)              (NriRef(SwapChain) swapChain, NriOut NriRef(DisplayDesc) displayDesc); // returns FAILURE if window is outside of all monitors
};

NriNamespaceEnd
