// © 2021 NVIDIA Corporation

#pragma once

NriNamespaceBegin

NriForwardStruct(SwapChain);

// Special "initialValue" for "CreateFence" needed to create swap chain related semaphores
static const uint64_t NriConstant(SWAPCHAIN_SEMAPHORE) = (uint64_t)(-1);

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

NriStruct(WindowsWindow) {          // Expects "WIN32" platform macro
    void* hwnd;                     //    HWND
};

NriStruct(X11Window) {              // Expects "NRI_ENABLE_XLIB_SUPPORT"
    void* dpy;                      //    Display
    uint64_t window;                //    Window
};

NriStruct(WaylandWindow) {          // Expects "NRI_ENABLE_WAYLAND_SUPPORT"
    void* display;                  //    wl_display
    void* surface;                  //    wl_surface
};

NriStruct(MetalWindow) {            // Expects "APPLE" platform macro
    void* caMetalLayer;             //    CAMetalLayer
};

NriStruct(Window) {
    // Only one entity must be initialized
    Nri(WindowsWindow) windows;
    Nri(X11Window) x11;
    Nri(WaylandWindow) wayland;
    Nri(MetalWindow) metal;
};

// SwapChain textures will be created as "color attachment" resources
// queuedFrameNum = 0 - auto-selection between 1 (for waitable) or 2 (otherwise)
// queuedFrameNum = 2 - recommended if the GPU frame time is less than the desired frame time, but the sum of 2 frames is greater
NriStruct(SwapChainDesc) {
    Nri(Window) window;
    const NriPtr(Queue) queue;      // GRAPHICS or COMPUTE (requires "features.presentFromCompute")
    Nri(Dim_t) width;
    Nri(Dim_t) height;
    uint8_t textureNum;             // desired value, real value must be queried using "GetSwapChainTextures"
    Nri(SwapChainFormat) format;    // desired format, real must be queried using "GetTextureDesc" for one of swap chain textures
    uint8_t verticalSyncInterval;   // 0 - vsync off
    uint8_t queuedFrameNum;         // aka "max frame latency", aka "number of frames in flight" (mostly for D3D11)
    bool waitable;                  // allows to use "WaitForPresent", which helps to reduce latency (requires "features.waitableSwapChain")
    bool allowLowLatency;           // unlocks "NRILowLatency" functionality (requires "features.lowLatency")
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

// Threadsafe: yes
NriStruct(SwapChainInterface) {
    Nri(Result)             (NRI_CALL *CreateSwapChain)         (NriRef(Device) device, const NriRef(SwapChainDesc) swapChainDesc, NriOut NriRef(SwapChain*) swapChain);
    void                    (NRI_CALL *DestroySwapChain)        (NriRef(SwapChain) swapChain);
    NriPtr(Texture) const*  (NRI_CALL *GetSwapChainTextures)    (const NriRef(SwapChain) swapChain, NriOut NonNriRef(uint32_t) textureNum);

    // Returns "FAILURE" if swap chain's window is outside of all monitors
    Nri(Result)             (NRI_CALL *GetDisplayDesc)          (NriRef(SwapChain) swapChain, NriOut NriRef(DisplayDesc) displayDesc);

    // VK only: may return "OUT_OF_DATE", fences must be created with "SWAPCHAIN_SEMAPHORE" initial value
    Nri(Result)             (NRI_CALL *AcquireNextTexture)      (NriRef(SwapChain) swapChain, NriRef(Fence) textureAcquiredSemaphore, NriOut NonNriRef(uint32_t) textureIndex);
    Nri(Result)             (NRI_CALL *WaitForPresent)          (NriRef(SwapChain) swapChain); // call once right before input sampling (must be called starting from the 1st frame)
    Nri(Result)             (NRI_CALL *QueuePresent)            (NriRef(SwapChain) swapChain, NriRef(Fence) renderingFinishedSemaphore);
};

/*
Typical usage example:

// Creation:
    // Create swap chain
        SwapChainDesc swapChainDesc = {};
        swapChainDesc.textureNum = QUEUED_FRAME_NUM + 1;
        ...

    // Create in-flight data
        struct QueuedFrame {
            Fence* textureAcquiredSemaphore;
            Fence* renderingFinishedSemaphore;
            ...
        } queuedFrames[QUEUED_FRAME_NUM] = {};

        Fence* frameFence;
        NRI.CreateFence(device, 0, frameFence);

        for (int i = 0 ; i < QUEUED_FRAME_NUM; i++) {
            NRI.CreateFence(device, SWAPCHAIN_SEMAPHORE, queuedFrames[i].textureAcquiredSemaphore);
            NRI.CreateFence(device, SWAPCHAIN_SEMAPHORE, queuedFrames[i].renderingFinishedSemaphore);
            ...
        }

// Rendering:
    // Wait for the tail of enqueued frames
        NRI.Wait(frameFence, frameIndex >= QUEUED_FRAME_NUM ? 1 + frameIndex - QUEUED_FRAME_NUM : 0);

    // Current frame data
        const QueuedFrame& queuedFrame = queuedFrames[frameIndex % QUEUED_FRAME_NUM];

    // Acquire a swap chain texture
        NRI.AcquireNextTexture(swapChain, *queuedFrame.textureAcquiredSemaphore, currentSwapChainTextureIndex);

    // Record command buffers
        ...

    // Submit
        FenceSubmitDesc frameFence = {};
        frameFence.fence = m_FrameFence;
        frameFence.value = 1 + frameIndex;

        FenceSubmitDesc textureAcquiredFence = {};
        textureAcquiredFence.fence = queuedFrame.textureAcquiredSemaphore;
        textureAcquiredFence.stages = COLOR_ATTACHMENT; // COPY, ALL

        FenceSubmitDesc renderingFinishedFence = {};
        renderingFinishedFence.fence = queuedFrame.renderingFinishedSemaphore;

        FenceSubmitDesc signalFences[] = {renderingFinishedFence, frameFence};

        QueueSubmitDesc queueSubmitDesc = {};
        queueSubmitDesc.waitFences = &textureAcquiredFence;
        queueSubmitDesc.waitFenceNum = 1;
        queueSubmitDesc.commandBuffers = ...;
        queueSubmitDesc.commandBufferNum = ...;
        queueSubmitDesc.signalFences = signalFences;
        queueSubmitDesc.signalFenceNum = 2;

        NRI.QueueSubmit(queue, queueSubmitDesc);

    // Present
        NRI.QueuePresent(swapChain, *queuedFrame.renderingFinishedSemaphore);
*/

NriNamespaceEnd
