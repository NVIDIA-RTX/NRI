# NRI Video TODO

## Vulkan AV1 Encode

- Add a proper AV1 OBU header writer or public helper equivalent to the H.264/H.265 Annex-B helpers. Consumers still need a temporal delimiter and sequence header before raw AV1 frame payloads can be decoded standalone.
- Decide whether Vulkan AV1 segmentation should remain explicitly unsupported or be implemented when driver capabilities make it viable.

## Vulkan AV1 Feedback

- Keep query feedback handling aligned with `VideoEncodeFeedback::encodedBitstreamOffset` being relative to `VideoEncodeDesc::dstBitstream.offset`.
- Preserve the current split between host query reads and explicit command-buffer resolves; Vulkan query-copy resolves must stay on graphics or compute queues.

## H.265 Encode Roundtrip Coverage

- Add a dedicated H.265 encode/decode roundtrip test.
- Generate valid VPS/SPS/PPS descriptors and serialize Annex-B headers with `WriteVideoAnnexBParameterSetsShared`.
- Do not treat H.265 as covered by the H.264 FFmpeg smoke path; it needs codec-specific session parameters, picture syntax, and decode input assembly.

## Vulkan Video Test Isolation

- Re-check whether repeated video device/session creation and teardown leaves process-global Vulkan driver state behind.
- If device initialization becomes order-dependent, isolate Vulkan video tests by process or tighten cleanup between video contexts.
