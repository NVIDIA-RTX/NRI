# WGPU Backend Notes

Last updated: 2026-06-22

## Repository State

- Root `NRISamples` checkout is currently on branch `main`.
- Nested NRI checkout at `External/NRIFramework/External/NRI` is currently on branch `WebGPU`.
- Nested NRI checkout has local WGPU review fixes. Root `NRISamples` still has local sample/config changes.
- WGPU backend source lives in `External/NRIFramework/External/NRI/Source/WGPU`.
- `NotesWGPU.md` lives next to the WGPU backend source.
- Keep Windows CRLF line endings.
- Avoid changing samples, prefer fixing WGPU backend.

## Dependency And Build

- CMake option: `NRI_ENABLE_WGPU_SUPPORT`.
- Current dependency source: `gfx-rs/wgpu-native` release `v29.0.0.0`, exposed as `NRI_WGPU_VERSION` in NRI CMake.
- WGPU support is enabled by default when `WGPU_BIN_ARCHITECTURE` is known.
- CMake fetches the matching wgpu-native binary package:
  - Windows: `wgpu-windows-${WGPU_BIN_ARCHITECTURE}-msvc-release.zip`
  - Linux: `wgpu-linux-${WGPU_BIN_ARCHITECTURE}-release.zip`
  - macOS: `wgpu-macos-${WGPU_BIN_ARCHITECTURE}-release.zip`
- WGPU package architecture mapping:
  - NRI `arm64` -> wgpu `aarch64`
  - NRI `x64` -> wgpu `x86_64`
  - NRI `x86` -> wgpu `i686` on Windows only
- WGPU swapchain surface creation supports NRI window handles for Windows, X11/Xlib, Wayland, and Metal.
- Quick backend build:

```powershell
cmake --build _Build --config Debug --target NRI_WGPU -- /nr:false
```

- Quick sample run pattern:

```powershell
cd _Bin\Debug
Triangle.exe -a WGPU --timeLimit=8 --debugNRI --debugAPI
```

## Design Rules

- WGPU backend files use `WGPU` suffix and live under `Source/WGPU`.
- Backend implementations are in `.h` and `.hpp` files, included from `ImplWGPU.cpp`, matching the other backends.
- Use the nested NRI `.clang-format` for WGPU backend files. Keep a blank line after a completed `if` / `if-else` block before starting the next statement, if it's not a closing bracket.
- `std::vector` must not be used in WGPU backend code. Use only NRI `Vector` with the custom allocator for persistent storage.
- Never use `Vector` for runtime temporary storage. Use `NRI_ALLOCATE_SCRATCH` instead.
- Use "std::array" in cases where out-of-bounds behavior may be trapped in Debug mode. C-style fixed size arrays are fine if they are passed in functions as "raw data pointers".
- NRI GAPI backends must not do validity checks. Validation must remain feature-driven and must not special-case `GraphicsAPI::WGPU`.
- Validation must not inspect shader code to identify SPIR-V/WGSL. Shader bytecode format handling is backend responsibility.
- WebGPU exposes a single device queue. NRI reports:
  - `GRAPHICS = 1`
  - `COMPUTE = 0`
  - `COPY = 0`
- Compute pipelines and compute dispatch are supported on the graphics queue. Async compute must be disabled in samples.
- Unsupported WebGPU features should be exposed through `DeviceDesc` fields and tiers, not through backend-side rejection.

## TODOs

- Clarify or replace `DeviceDesc::shaderModel = NriShaderModel(6, 0)`
  - WebGPU/WGSL does not expose a D3D-style shader model, so the current value is only a compatibility placeholder.
- Implement occlusion queries:
  - WebGPU supports occlusion query sets and begin/end commands;
  - WGPU render pass creation needs to pass `occlusionQuerySet`;
  - set `features.occlusion = true` only after the render-pass path is wired correctly.
- Rework WGSL reflection:
  - current texture reflection is a small source parser for direct `@group`, `@binding`, `texture_*`, and `texture_storage_*` declarations;
  - this is enough for simple direct declarations, but not a full WGSL reflection solution;
  - keep Validation out of shader-code parsing.
- Rework descriptor sampler layout metadata:
  - descriptor sampler ranges currently default to filtering samplers;
  - descriptor-set comparison samplers need shader reflection or explicit metadata.
- Rework sampled texture format metadata:
  - shader reflection can identify scalar type, view dimension, and depth, but not the concrete NRI texture format;
  - `UnfilterableFloat` layouts for 32-bit float sampled textures need explicit metadata or a more complete binding model;
  - keep ordinary sampled float declarations as `Float` so normalized textures remain filterable.
- Add WGSL sample or test coverage:
  - No sample currently exercises WGSL shaders.
- Finish descriptor array support:
  - fixed-size `DescriptorRangeBits::ARRAY` maps to one native binding with an array size, matching the Vulkan backend's `descriptorCount = range.descriptorNum`;
  - WGPU requests native binding-array features opportunistically;
  - `PARTIALLY_BOUND` and `VARIABLE_SIZED_ARRAY` are rejected by Validation through existing `DeviceDesc` tiers;
  - fixed arrays still need a clear capability story because WGPU support depends on optional native binding-array features and NRI currently has no dedicated cap for this.
- Decide how to represent true update-after-set descriptors:
  - WGPU bind groups are immutable after command encoding;
  - same-set rebind after a descriptor update is handled by tracking descriptor-set update versions during command recording;
  - descriptor updates after `CmdSetDescriptorSet` without rebinding still need a `DeviceDesc` cap or API convention before `ALLOW_UPDATE_AFTER_SET` can be advertised accurately.
- Decide how to represent indirect-count draws:
  - WGPU native count-buffer draw calls do not expose NRI's indirect command stride;
  - keep `features.drawIndirectCount = false` until the API mismatch is solved or an emulation path is added.
- Evaluate `NRI_DRAW_ID` / `PipelineLayoutBits::ENABLE_DRAW_INDEX_EMULATION` support:
  - WGPU does not expose a native equivalent;
  - keep `shaderFeatures.drawIndex = false` until an emulation path is implemented.
- Move draw-emulated clear pipeline caching from command-buffer-local to device-level if clear pipeline creation shows up in profiles.
- Install WGPU uncaptured error/device-lost callbacks and route messages into the NRI callback where possible.
- Improve `MultiThreading` performance by reducing remaining draw-time WGPU command encoding overhead, mainly repeated per-box bind-group binds for descriptor set 0 and the root descriptor group.
