---
name: nri-review
description: Review NRI repository changes with repo-specific C++/graphics-backend rules. Use when Codex reviews local diffs, pull requests, merge requests, or proposed patches touching Include, Source/Validation, Source/D3D11, Source/D3D12, Source/VK, Source/Shared, CMake, build scripts, or NRI extension headers.
---

# NRI Review

## Overview

Review NRI changes as a graphics API/backend reviewer. Prioritize correctness, validation-layer boundaries, backend SDK assumptions, ABI/API compatibility, and maintainer style rules over generic C++ preferences.

## Workflow

1. Start from the actual diff or review target. Use `git status --short --branch`, `git diff --stat`, and targeted `git diff -- <path>` reads.
2. If pattern context is needed, study `main` without switching branches: use `git show main:<path>` and `git grep <pattern> main -- <paths>`.
3. Classify each touched file by layer: public API (`Include`), validation (`Source/Validation`), backend implementation (`Source/D3D11`, `Source/D3D12`, `Source/VK`), shared helpers (`Source/Shared`), creation/glue (`Source/Creation`), or build/deploy.
4. Review for the NRI-specific checks below before generic code quality.
5. Report findings first, ordered by severity, with file and line references. If there are no findings, say so and note any unrun verification.

## Review Checks

Layering:
- Ensure public input validation lives in `Source/Validation`, not backend implementations.
- Treat backend `NRI_CHECK` as acceptable for critical internal assumptions, impossible states, and native/backend failure points, not as public argument validation.
- Check validation wrappers unwrap `*Val` objects consistently, update validation-side state only after implementation success, and preserve object lifetime/destruction behavior.

Style and structure:
- Require `.clang-format` formatting: Google-based, 4-space indent, no tabs, attached braces, left pointer/reference alignment, `ColumnLimit: 0`, preserved include blocks, and case-sensitive include sorting.
- Require the maintainer preference for one empty line before control-flow keywords in touched code (`return`, `if`, `for`, `while`, `switch`, etc.).
- Check primary implementation entities (`Device*`, `Descriptor*`, `VideoSession*`, `Buffer*`, `Texture*`, `Pipeline*`, `Queue*`, validation entities) keep data members behind `private:`.
- Keep one-off helper functions in the single file where they are used, usually as file-local `static inline`; flag duplicated or unnecessarily shared helpers.
- Never add `case <Enum>::MAX_NUM` in an enum switch. Use `default` for unexpected enum values.
- Prefer existing conversion tables plus `NRI_VALIDATE_ARRAY*` checks over ad hoc enum switch mappings.
- Avoid making unnecessary changes to existing code.
- Flag unrelated churn. If an unrelated fix was required, it should be commented `FIXED BY AI` for maintainer audit.

Backend implementation:
- Check `Impl*.cpp` function-table wrappers stay thin: cast to the backend/validation entity and delegate.
- Check D3D11, D3D12, and VK create paths preserve `CreateImplementation<T>` behavior: allocate, call `Create`, destroy and null on failure, cast on success.
- For native API failures, expect `NRI_RETURN_ON_BAD_HRESULT`, `NRI_RETURN_ON_BAD_VKRESULT`, or existing native-result macros.
- Check backend parity when public API, shared descriptors, enum values, or interface methods change: D3D11, D3D12, VK, Validation, and Creation may all need edits.
- Preserve NRI's low-overhead model. Flag hidden management, automatic barriers, broad synchronization, or high-level abstraction behavior unless explicitly requested.

D3D12-specific:
- Minimize `#if/#ifdef/#endif` use for `NRI_ENABLE_AGILITY_SDK_SUPPORT`; prefer tight feature-specific blocks and shared fallback code.
- Respect current SDK assumptions: latest Agility SDK support, currently v1.619.x in CMake, or Windows SDK 10.0.20348 as the minimum spec without Agility.
- Flag locally invented D3D12 constants/enums/structs or magic values justified as "old Agility SDK" compatibility.
- Keep `ID3D12DeviceBest`, version checks, and Agility-only calls consistent with `DeviceD3D12` and existing descriptor/pipeline patterns.

API and build:
- Public headers must preserve C/C++ compatibility macros (`NriStruct`, `NriEnum`, `NriBits`, `NriRef`, `NriPtr`, `NriNamespaceBegin/End`, `NriOptional`, `NriOut`, `NRI_CALL`) and ABI-sensitive layout.
- Public or extension API additions must update the header function-pointer table, `DeviceBase::FillFunctionTable`, validation table fill, supported backend table fills, extension support flags, and `nriGetInterface` compatibility behavior.
- New enum values must update mapping arrays, validation checks, conversion helpers, and `NRI_VALIDATE_ARRAY*` coverage together.
- CMake changes must preserve option names, dependency gating, warnings-as-errors behavior, explicit source lists, `target_sources`, `source_group`, and generator-expression style.
- CI deploy/build/SDK preparation commands are the baseline verification path; prefer targeted local verification when a full build is not practical.

## Finding Priorities

- High severity: validation moved into backends, invalid public API behavior, native resource lifetime bugs, ABI-breaking public header changes, D3D12 SDK assumptions violated, or changes likely to break a backend build.
- Medium severity: wrong helper placement, newly added enum `MAX_NUM` switch cases, missing failure cleanup, inconsistent wrapper unwrapping, bad version/feature gating, missing interface-table/source-list updates, or meaningful style violations in touched code.
- Low severity: minor local style drift, unclear comments, or missed formatting when behavior is unaffected.

## Verification

For C/C++ changes, check formatting with the repo `.clang-format` when possible. For build verification, use the narrowest relevant build first; full CI equivalents are `.\1-Deploy.bat`, `.\2-Build.bat`, `.\3-PrepareSDK.bat` on Windows and `bash 1-Deploy.sh`, `bash 2-Build.sh`, `bash 3-PrepareSDK.sh` on Linux. For noisy logs, save output to disk and inspect only the first unique error and summary.
