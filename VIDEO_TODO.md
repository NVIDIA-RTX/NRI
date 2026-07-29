# NRI Video TODO

## Vulkan AV1 Encode

- Decide whether Vulkan AV1 segmentation should remain explicitly unsupported or be implemented when driver capabilities make it viable.

## Vulkan AV1 Feedback

- Keep query feedback handling aligned with `VideoEncodeFeedback::encodedBitstreamOffset` being relative to `VideoEncodeDesc::dstBitstream.offset`.
- Preserve the current split between host query reads and explicit command-buffer resolves; Vulkan query-copy resolves must stay on graphics or compute queues.

## Video Backend Discrepancies

- NRISamples `VideoEncodeDecode` still has VK/D3D12 branches because some video requirements are not represented as backend-neutral NRI state yet.
- Vulkan AV1 encode currently uses a reduced descriptor shape: CDEF/restoration/screen-content sequence options are disabled, several AV1 picture sub-descriptor pointers are omitted, and render size follows coded size. These differences come from the Vulkan AV1 encode path and driver-supported feature set not matching the D3D12 descriptor expectations one-to-one.
- D3D12 AV1 encode currently enables segmentation where Vulkan does not. The sample carries this as a backend branch because the two backends require different AV1 picture descriptor details for otherwise similar encode requests.
- Encode feedback metadata is consumed differently. Vulkan needs the resolved metadata readback buffer and, for AV1 decode-info construction, may need bytes from the encoded payload header. D3D12 uses the resolved metadata buffer directly in the current path.
- H.264/H.265 Vulkan decode currently needs explicit DPB-style picture states for some decoded/reference pictures, while D3D12 follows the generic states returned by the current helper path. The sample branches because these layout/state requirements are observable at command recording time.
- Decode bitstream buffer barriers differ: Vulkan records the bitstream buffer transition before decode, while D3D12 skips it in the current sample path. This reflects backend command-list/resource-state handling differences.
- Decoded-picture readback queue selection differs: Vulkan readback currently uses the decode queue, while D3D12 uses the graphics queue. The sample branches because copy/readback support from video-used pictures is not currently handled identically across backends.
