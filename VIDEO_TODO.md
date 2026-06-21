# NRI Video TODO

## Vulkan AV1 Encode

- Decide whether Vulkan AV1 segmentation should remain explicitly unsupported or be implemented when driver capabilities make it viable.
- Validate and expose the supported AV1 encode Q index range. NVIDIA Vulkan reports `baseQIndex` / Q index 0 outside the supported 1..255 range, and the same zero-Q AV1 encode request can device-remove on D3D12 instead of failing cleanly.

## Vulkan AV1 Feedback

- Keep query feedback handling aligned with `VideoEncodeFeedback::encodedBitstreamOffset` being relative to `VideoEncodeDesc::dstBitstream.offset`.
- Preserve the current split between host query reads and explicit command-buffer resolves; Vulkan query-copy resolves must stay on graphics or compute queues.

## H.265 Encode Roundtrip Coverage

- Add a dedicated H.265 encode/decode roundtrip test.
- Generate valid VPS/SPS/PPS descriptors and serialize Annex-B headers with `WriteVideoAnnexBParameterSetsShared`.
- Do not treat H.265 as covered by the H.264 FFmpeg smoke path; it needs codec-specific session parameters, picture syntax, and decode input assembly.

## Vulkan H.265 Encode Follow-Ups

- Investigate non-fatal NVIDIA parser warnings from NRISamples `VideoEncodeDecode` Vulkan H.265 inter-frame smoke runs. `Invalid slice segment address` is still printed for some P/B CQP and lossless cases even though encode/decode completes and the smoke script passes.
- Clean up the debug-validation preview layout warning where `VideoReconstructedTexture` can remain in `VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR` while a graphics descriptor expects `VK_IMAGE_LAYOUT_GENERAL`.

## Vulkan Video Test Isolation

- Re-check whether repeated video device/session creation and teardown leaves process-global Vulkan driver state behind.
- If device initialization becomes order-dependent, isolate Vulkan video tests by process or tighten cleanup between video contexts.
