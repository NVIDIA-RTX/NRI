---
name: nri-coding-standard
description: Implement or modify NRI C/C++ using its source layout, validation boundary, backend patterns, low-overhead rules, CMake conventions, and verification requirements.
---

# NRI Coding Standard

Apply these rules to changes under `Include`, `Source`, CMake, and build scripts. Explicit user instructions take precedence.

## General

- Preserve CRLF and C++17 compatibility.
- Use the repository `.clang-format`: 4 spaces, no tabs, attached braces, left pointer/reference alignment, unlimited columns, preserved include blocks, and case-sensitive include sorting.
- Leave an empty line before `return`, `if`, `for`, `while`, and `switch` in touched code.
- Parenthesize compound conditions used by the conditional (`?:`) operator, for example: `(isEnabled && value != 0) ? enabled : disabled`.
- Parenthesize arithmetic subexpressions used as comparison operands in compound conditions, for example: `offsetA < (offsetB + numB) && offsetB < (offsetA + numA)`.
- Use existing NRI macros and patterns.
- Preserve the public C/C++ facade: `NriStruct`, `NriEnum`, `NriBits`, `NriRef`, `NriPtr`, `NriNamespaceBegin/End`, `NriOptional`, `NriOut`, and `NRI_CALL`.
- Avoid unrelated churn and preserve user changes. Mark an unavoidable unrelated fix with `FIXED BY AI`.

## Source Organization

- Keep declarations and entity layout in `.h`, non-trivial method bodies in the corresponding `.hpp`, and `Impl*.cpp` focused on interface wiring, dispatch, and short entry points.
- Functions that do not use class state are file-local `static inline` helpers in the helper section near the top of the file.
- Backend interface implementations called by wrappers in `Impl*.cpp` are not ordinary helpers: keep their GAPI suffixes and define them with `NRI_INLINE`.
- Wrapper-interface entry points defined directly in `Impl*.cpp` also keep GAPI suffixes to match the corresponding wrapper API function-table members; keep them file-local with `static` and `NRI_CALL`.
- Omit redundant GAPI suffixes from ordinary file-local helpers in single-GAPI files.
- Keep GAPI suffixes in `Creation.cpp` and other multi-GAPI files where they disambiguate backend-specific helpers.
- Keep one-off helpers local. Put genuinely reused backend-independent logic in the relevant Shared header or codec namespace.
- Put backend-wide constants and declarations in the corresponding `Shared*.h`.
- Keep implementation-entity data members private. Put private helpers before storage and preserve lock, lifetime, destructor-order, and debugger comments.
- Do not expand public mutable implementation state.
- In enum switches, use `default`; do not add `case <Enum>::MAX_NUM`.

## Temporary Storage and Performance

- Use `Scratch<T>` with `NRI_ALLOCATE_SCRATCH` for variable-sized temporary arrays.
- Explicitly initialize native structures allocated in scratch memory.
- Use stack arrays for small fixed-size storage and persistent containers for persistent state.
- Zero-sized scratch allocations are valid; do not allocate dummy elements unless code indexes them.
- Ensure pointers stored in native structures outlive the native call. Be careful when copying pointer-bearing aggregates.
- Preserve NRI's low-overhead model. Do not add hidden barriers, synchronization, ownership, allocation, caching, or high-level management unless requested.

## Validation Boundary

- Public-input validation belongs in `Source/Validation`, not D3D11, D3D12, Vulkan, WGPU, or NONE.
- Use `NRI_RETURN_ON_FAILURE` to reject invalid public inputs in Validation.
- Validation may check required pointers, object relationships, simple ranges, counts, alignment, and advertised capability limits.
- Null-check before casting or dereferencing validation wrappers; unwrap `*Val` objects consistently.
- Update validation bookkeeping only after backend success or provide rollback.
- Validation enforces the NRI usage contract. Do not duplicate complex GAPI-specific checks or state tracking handled by native debug or validation layers.
- Backends may assume validated descriptors. Use debug-only `NRI_CHECK` for critical internal assumptions, impossible states, unsupported native paths, or defensive crash checks.
- Cover new NRI functionality in Validation.

## Backend and API Patterns

- Keep `Impl*.cpp` function-table wrappers file-local and limited to casting and delegation.
- Preserve `CreateImplementation<T>`: allocate, call `Create`, destroy and null on failure, cast on success.
- Use `NRI_RETURN_ON_BAD_HRESULT`, `NRI_RETURN_ON_BAD_VKRESULT`, and existing native-result macros.
- Follow existing tables and helpers for formats, descriptors, barriers, pipelines, and conversions.
- Prefer extending conversion tables and `NRI_VALIDATE_ARRAY*` coverage over parallel switch logic.
- Public API changes must align the public table, `DeviceBase::FillFunctionTable`, Validation, every supported backend, Creation/export glue, `nriGetInterface`, support flags, and capabilities.
- Enum changes must update mapping arrays, validation ranges, native conversions, and `NRI_VALIDATE_ARRAY*` assertions together.
- NRI v181 is work in progress: recompilation and ABI/layout/enum-number changes are acceptable unless the user says otherwise. Preserve the C/C++ facade and public semantics.

## D3D12

- Keep `NRI_ENABLE_AGILITY_SDK_SUPPORT` blocks tight and feature-specific.
- Assume the supported Agility SDK, currently v1.619.x, or Windows SDK 10.0.20348 as the non-Agility minimum.
- Do not invent fallback D3D12 constants, enums, structures, or magic values for unsupported SDKs.
- Keep `ID3D12DeviceBest`, interface queries, and version-gated calls consistent with `DeviceD3D12`.

## CMake and Verification

- Preserve option names, dependency gates, warnings-as-errors, explicit source lists, `target_sources`, `source_group`, and generator-expression style.
- Add new files to every applicable explicit source list.
- Prefer targeted builds first. CI baselines are `.\Scripts\Windows\1-Deploy.bat`, `.\Scripts\Windows\2-Build.bat`, and `.\Scripts\Windows\3-PrepareSDK.bat` on Windows, with corresponding shell scripts in `Scripts/Linux/` and `Scripts/MacOS/`. Scripts resolve paths relative to the NRI repository root.
- Keep noisy logs under `_Tmp` when requested and report the first unique error plus the summary.
- Run `.clang-format` on touched C/C++ when available, inspect the diff, run `git diff --check`, and verify CRLF.
