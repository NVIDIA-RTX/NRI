// © 2024 NVIDIA Corporation

// Goal: minimizing latency between input sampling and frame presentation

#pragma once

#define NRI_LOW_LATENCY_H 1

NriNamespaceBegin

NriForwardStruct(SwapChain);
NriForwardStruct(Queue);

// us = microseconds

NriEnum(LatencyMarker, uint8_t,

                                        // Should be called:
    SIMULATION_START    = 0,            // at the start of the simulation execution each frame, but after the call to "LatencySleep"
    SIMULATION_END      = 1,            // at the end of the simulation execution each frame
    RENDER_SUBMIT_START = 2,            // at the beginning of the render submission execution each frame (must not span into asynchronous rendering)
    RENDER_SUBMIT_END   = 3,            // at the end of the render submission execution each frame
    INPUT_SAMPLE        = 6             // just before the application gathers input data, but between SIMULATION_START and SIMULATION_END (yes, 6!)
);

NriStruct(LatencySleepMode) {
    uint32_t minIntervalUs;             // minimum allowed frame interval (0 - no frame rate limit)
    bool lowLatencyMode;                // low latency mode enablement
    bool lowLatencyBoost;               // hint to increase performance to provide additional latency savings at a cost of increased power consumption
};

NriStruct(LatencyReport) {
                                        // The time stamp written:
    uint64_t inputSampleTimeUs;         // when "SetLatencyMarker(INPUT_SAMPLE)" is called
    uint64_t simulationStartTimeUs;     // when "SetLatencyMarker(SIMULATION_START)" is called
    uint64_t simulationEndTimeUs;       // when "SetLatencyMarker(SIMULATION_END)" is called
    uint64_t renderSubmitStartTimeUs;   // when "SetLatencyMarker(RENDER_SUBMIT_START)" is called
    uint64_t renderSubmitEndTimeUs;     // when "SetLatencyMarker(RENDER_SUBMIT_END)" is called
    uint64_t presentStartTimeUs;        // right before "Present"
    uint64_t presentEndTimeUs;          // right after "Present"
    uint64_t driverStartTimeUs;         // when the first "QueueSubmitTrackable" is called
    uint64_t driverEndTimeUs;           // when the final "QueueSubmitTrackable" hands off from the driver
    uint64_t osRenderQueueStartTimeUs;
    uint64_t osRenderQueueEndTimeUs;
    uint64_t gpuRenderStartTimeUs;      // when the first submission reaches the GPU
    uint64_t gpuRenderEndTimeUs;        // when the final submission finishes on the GPU
};

// Multi-swapchain is supported only by VK
// Threadsafe: no
NriStruct(LowLatencyInterface) {
    Nri(Result)     (NRI_CALL   *SetLatencySleepMode)   (NriRef(SwapChain) swapChain, const NriRef(LatencySleepMode) latencySleepMode);
    Nri(Result)     (NRI_CALL   *SetLatencyMarker)      (NriRef(SwapChain) swapChain, Nri(LatencyMarker) latencyMarker);
    Nri(Result)     (NRI_CALL   *LatencySleep)          (NriRef(SwapChain) swapChain); // call once before INPUT_SAMPLE
    Nri(Result)     (NRI_CALL   *GetLatencyReport)      (const NriRef(SwapChain) swapChain, NriOut NriRef(LatencyReport) latencyReport);

    // This function must be used in "low latency" mode instead of "QueueSubmit"
    // Threadsafe: yes
    Nri(Result)     (NRI_CALL   *QueueSubmitTrackable)  (NriRef(Queue) queue, const NriRef(QueueSubmitDesc) queueSubmitDesc, const NriRef(SwapChain) swapChain);
};

NriNamespaceEnd
