---
name: nri-review
description: Review NRI changes or the current tree for graphics-API correctness, backend parity, validation coverage, duplication, simplification, and build risk.
---

# NRI Review

Read `../nri-coding-standard/SKILL.md` completely before reviewing. Treat it as the coding-policy checklist; this skill defines the review method.

When the review touches WGPU, read `../../todo-wgpu.md` completely. When it touches video, read `../../todo-video.md` completely. Treat these files as maintained investigation context, not as pre-established findings; verify every relevant item against the review target.

Review exactly the target and comparison basis specified by the user. Clarify only when genuine ambiguity could change the findings.

Preserve the intentional formatting of NRI interface files, including public headers under `Include/` and internal `*Interface.h` declarations. Do not run clang-format on these files; make focused manual edits instead.

When adding or editing code comments, do not add a period at the end of a single-sentence comment. Keep periods that separate sentences in multi-sentence comments:

```cpp
// This is good
// That was bad. This is good
```

Record the exact target and comparison basis, including refs and commits when applicable. Include committed and uncommitted changes when reviewing a diff. If the review is against current `main`, verify that the local ref matches the remote before comparing.

## Map the Feature

Trace each changed public concept through:

`public API -> capabilities -> Validation -> backend entry point -> native objects/commands -> result/feedback -> destruction`

Find all declarations and call sites. Check function-table wiring, supported and unsupported backends, neutral and native paths, and the resources, indices, offsets, states, queues, ownership, and lifetimes represented by the API.

For public pointers or binary blobs, check ownership, lifetime, size/null invariants, valid call timing, repeated-call behavior, allocation, caching, and destruction.

Review the whole feature neighborhood, not only the last commit or added lines.

## Correctness and Backend Parity

- Every valid advertised path must execute safely with the documented meaning.
- Capabilities, Validation, and implementation behavior must agree.
- For optional native features, check extension selection, feature chaining, dispatch resolution, stored support, and the unsupported result as one path.
- Apply scratch initialization requirements to native input and input/output structures. Pure output arrays without `sType`/`pNext` do not need redundant initialization.
- Keep direct native API mirrors in native terminology even when the NRI-facing concept uses different terminology.
- Compare D3D12 and Vulkan resource states, access/stages, queues, ownership, synchronization, setup/reference/reconstructed pictures, offsets, feedback, and object lifetimes.
- Different native requirements are valid only when the NRI contract lets callers discover and satisfy them. Treat cross-backend differences as investigation leads, not proof of a defect; confirm the contract and units of each native API before deciding that one backend is wrong.
- Check zero and maximum counts, sparse or duplicate slots, aliasing, optional pointers, nonzero offsets, narrowing, partial construction, rollback, and destruction.
- Check relevant combinations of codec, encode/decode, frame type, neutral/native arguments, host/buffer sources, coincident/distinct pictures, Validation/direct use, and Agility/non-Agility.
- Verify uncertain GAPI rules with primary documentation or primary implementation sources; identify inferences. Distinguish quantities such as logical and physical or block-rounded extents, texels and blocks, row size and row pitch, and resources and subresources. Include relevant padding, edge, feature, and version rules before declaring a native violation.

## Duplication and Simplification

Use this pass for substantial implementation changes or when maintainability is in scope. Correctness and regression risk come first.

- Compare file-local helpers and substantial workflows across backends.
- Look for equivalent bodies with the same or different names, terminology, capitalization, or backend suffixes.
- Use backend suffixes to locate helper counterparts; backend classes and native-facing types keep their suffixes.
- Check duplication within one backend, especially capability probing versus creation, descriptor construction, codec branches, and count/check/write workflows.
- Share only genuinely backend-independent reused logic. Keep one-off and native-specific helpers local.
- Retain duplication when abstraction would worsen pointer lifetime, diagnostics, performance, or native clarity; state that reason.

## Review Verdict

A blocker is a supported valid path that can fail, crash, access missing storage, use invalid native data, violate an undiscoverable resource-state contract, advertise unsupported behavior, break Validation or interface wiring, leak or misuse lifetime, fail a required build, or otherwise make the reviewed state unacceptable.

Incomplete functionality is non-blocking when explicitly unsupported, accurately reported, and harmless to existing NRI. ABI changes alone are not findings for v181 when recompilation is accepted.

One reachable blocker controls the verdict.

## Verification and Report

- Run `git diff --check`, check for accidental edits, and verify CRLF.
- Build every affected backend and feature gate. For generic verification, cover every backend available through CMake: D3D12 (both Agility and non-Agility), Vulkan, D3D11, and WGPU, in both Debug and Release configurations.
- For non-Agility D3D12 verification, confirm that the build selected the Windows SDK 10.0.20348 headers; a downlevel target alone may still compile against newer installed headers.
- For findings that predict a native validation failure or incorrect output, run the smallest relevant existing test on the unchanged reviewed tree when practical and record whether it reproduces. A test that passes only after a proposed fix does not establish that the baseline fails; if both pass, reconcile the behavior with the exact native rule before retaining the finding.
- Distinguish compilation from runtime coverage and list important untested paths.
- Review the exact final tree, not an earlier iteration.

Report findings first by severity. Give exact file/line, reachable path, violated NRI or native rule, affected backend/mode, why Validation or capabilities do not prevent it, and a concrete fix.

End with blocker and useful non-blocker counts, verification performed and omitted, and an explicit `PASS` or `FAIL` verdict.
