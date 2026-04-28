// © 2021 NVIDIA Corporation

// Goal: hardware video encode/decode command submission
// D3D12: wraps ID3D12Video*CommandList encode/decode calls
// Vulkan: wraps vkCmdDecodeVideoKHR / vkCmdEncodeVideoKHR
// Video formats: https://learn.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering

#pragma once

#define NRI_VIDEO_H 1

NriNamespaceBegin

NriStruct(VideoDecodeD3D12Desc) {
    void* d3d12Decoder;                 // ID3D12VideoDecoder*
    const void* d3d12OutputArguments;   // D3D12_VIDEO_DECODE_OUTPUT_STREAM_ARGUMENTS*
    const void* d3d12InputArguments;    // D3D12_VIDEO_DECODE_INPUT_STREAM_ARGUMENTS*
};

NriStruct(VideoEncodeD3D12Desc) {
    void* d3d12Encoder;                 // ID3D12VideoEncoder*
    void* d3d12Heap;                    // ID3D12VideoEncoderHeap*
    const void* d3d12InputArguments;    // D3D12_VIDEO_ENCODER_ENCODEFRAME_INPUT_ARGUMENTS*
    const void* d3d12OutputArguments;   // D3D12_VIDEO_ENCODER_ENCODEFRAME_OUTPUT_ARGUMENTS*
};

NriStruct(VideoDecodeVKDesc) {
    const void* vkDecodeInfo;           // VkVideoDecodeInfoKHR*
};

NriStruct(VideoEncodeVKDesc) {
    const void* vkEncodeInfo;           // VkVideoEncodeInfoKHR*
};

NriStruct(VideoDecodeDesc) {
    NriOptional const NriPtr(VideoDecodeD3D12Desc) d3d12;
    NriOptional const NriPtr(VideoDecodeVKDesc) vk;
};

NriStruct(VideoEncodeDesc) {
    NriOptional const NriPtr(VideoEncodeD3D12Desc) d3d12;
    NriOptional const NriPtr(VideoEncodeVKDesc) vk;
};

// Threadsafe: no
NriStruct(VideoInterface) {
    // Command buffer
    // {
        // Video decode/encode command buffers must be created from "QueueType::VIDEO_DECODE" or "QueueType::VIDEO_ENCODE" queues.
        void (NRI_CALL *CmdDecodeVideo) (NriRef(CommandBuffer) commandBuffer, const NriRef(VideoDecodeDesc) videoDecodeDesc);
        void (NRI_CALL *CmdEncodeVideo) (NriRef(CommandBuffer) commandBuffer, const NriRef(VideoEncodeDesc) videoEncodeDesc);
    // }
};

NriNamespaceEnd
