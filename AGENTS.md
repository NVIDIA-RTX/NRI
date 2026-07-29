## Scope

These instructions apply to the NRI repository. NRI is a low-overhead graphics API layer with public C/C++ headers in `Include/`, implementation backends in `Source/D3D11`, `Source/D3D12`, `Source/VK`, a validation wrapper in `Source/Validation`, shared helpers in `Source/Shared`, and device creation/export glue in `Source/Creation`.

## Required Style

- Use the repo `.clang-format`. Do not hand-format against a different style.
- `.clang-format` is Google-based with 4-space indentation, no tabs, attached braces, left pointer/reference alignment, `ColumnLimit: 0`, preserved include blocks, and case-sensitive include sorting.
- Keep C++17 compatibility. CMake builds with warnings as errors (`/WX` or `-Werror`).
- Preserve the existing file split: declarations and entity layout in `.h`, inline/implementation bodies in `.hpp`, backend interface tables and exported C-call wrappers in `Impl*.cpp`.
- Use `nri` namespace patterns and existing macros (`NRI_CALL`, `NRI_INLINE`, `NRI_RETURN_ON_FAILURE`, `NRI_RETURN_ON_BAD_HRESULT`, `NRI_RETURN_ON_BAD_VKRESULT`, `NRI_CHECK`) instead of inventing local substitutes.
- Public headers must keep the C/C++ macro facade (`NriStruct`, `NriEnum`, `NriBits`, `NriRef`, `NriPtr`, `NriNamespaceBegin/End`, `NriOptional`, `NriOut`). Do not replace public API declarations with plain C++-only forms.
- Leave one empty line before control-flow keywords such as `return`, `if`, `for`, `while`, and `switch` in code you touch, matching the maintainer preference.
- Keep helper functions appearing only once, in the file where they are used. Prefer file-local `static inline` helpers in the relevant `.hpp`/`.cpp`; do not move one-off helpers into shared headers.
- In `switch` statements over enums, never add `case <Enum>::MAX_NUM`. Use `default` for unexpected values.
- Prefer extending existing conversion tables and `NRI_VALIDATE_ARRAY*` coverage over ad hoc switch logic for enum mappings.
- Avoid unnecessary changes. If a task-unrelated change is absolutely required to unblock the requested work, annotate that change with a `FIXED BY AI` comment so it can be audited later.

## Entity Layout

- Primary entity structs with implementations, such as `Device*`, `Descriptor*`, `VideoSession*`, `Buffer*`, `Texture*`, `Pipeline*`, `Queue*`, and validation counterparts, should keep data members behind `private:`.
- Public surface in these structs should be constructors/destructors, cheap accessors, creation/destruction methods, interface-fill methods, and NRI entry point methods.
- Private sections should contain helper methods first, then member storage. Keep existing comments such as `.natvis`, lock ownership notes, and destructor-order notes.
- Avoid adding public mutable state to implementation entities. If existing code exposes a specific public compatibility field, do not expand that pattern without a strong local reason.

## Validation Boundary

- Do input validation in `Source/Validation`, not in backend implementations (`Source/D3D11`, `Source/D3D12`, `Source/VK`).
- Validation code should reject invalid public inputs with `NRI_RETURN_ON_FAILURE`, keep validation wrapper state consistent, unwrap validation objects with existing helpers/macros, and then call the implementation interface.
- Null checks must happen before casting or dereferencing validation wrappers in new validation code.
- Mutate validation-side bookkeeping only after backend success, or provide clear rollback.
- Backend implementations may assume validated NRI descriptors and object relationships. They may use `NRI_CHECK` for critical internal assumptions, impossible states, unsupported backend-specific paths, or defensive crash/debug checks.
- Do not duplicate validation logic in D3D12/VK/D3D11 just because a backend dereferences a descriptor. Move or add the public input check in `Validation` instead.

## Backend Patterns

- Backend `Impl*.cpp` files build function tables from file-local `static NRI_CALL` wrappers. Keep wrappers thin: cast to backend entity, delegate to an entity method, and return the backend result.
- D3D11, D3D12, and VK backends use `CreateImplementation<T>` allocation flow: allocate, call `Create`, destroy and null the interface on failure, cast on success.
- Backend resource creation should use native result macros (`NRI_RETURN_ON_BAD_HRESULT`, `NRI_RETURN_ON_BAD_VKRESULT`) for native API failures.
- Descriptor, barrier, pipeline, and format conversion code should follow existing table/helper patterns in `Shared*` and local descriptor/pipeline files.
- Public or extension API additions must stay aligned across the public header table, `DeviceBase::FillFunctionTable`, validation `FillFunctionTable`, every supported backend `FillFunctionTable`, `nriGetInterface` size/name behavior, and extension support flags.
- When adding enum values, update mapping arrays, validation checks, descriptor/type conversion helpers, and any `NRI_VALIDATE_ARRAY*` assertions together.
- Preserve the low-overhead goal: do not add hidden management, automatic barriers, broad synchronization, or high-level abstraction behavior unless the task explicitly requires it.

## D3D12 Rules

- Minimize `#if/#ifdef/#endif` blocks for `NRI_ENABLE_AGILITY_SDK_SUPPORT`. Prefer the existing pattern of tight feature-specific blocks and shared fallback code.
- Respect current D3D12 assumptions: NRI uses latest Agility SDK support, currently v1.619.x in CMake, or Windows SDK 10.0.20348 as the minimum spec when Agility support is off.
- Do not create local fallback constants, enums, structs, or magic values for "old Agility SDK just in case". Use SDK-provided names available under the supported SDK assumptions.
- Keep `ID3D12DeviceBest` and version-gated calls consistent with `DeviceD3D12` patterns.

## Build And Verification

- Prefer targeted builds/checks for the changed backend or layer, but know CI runs:
  - Windows: `.\1-Deploy.bat`, `.\2-Build.bat`, `.\3-PrepareSDK.bat`
  - Linux: `bash 1-Deploy.sh`, `bash 2-Build.sh`, `bash 3-PrepareSDK.sh`
- CMake source lists are explicit. Add new files to the relevant backend/source list and preserve existing `target_sources`, `source_group`, and generator-expression style.
- For noisy commands, redirect logs to files and inspect the first unique error plus the summary. Do not paste repeated build errors or long logs into conversation context.
- Before finishing, run `.clang-format` on touched C/C++ files when available and inspect the diff for unintended churn.
