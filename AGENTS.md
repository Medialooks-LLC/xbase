# AGENTS.md

## Shared XSDK Code Rules

This repository can be used either as an XMedia submodule or as a standalone
sibling checkout. If it is checked out under `xmedia/submodules/<repo>`, agents
should read the nearest parent XMedia `coding_rules.md` and use it as the shared
C++ rule source. If it is checked out standalone next to XMedia, agents should
look for `../xmedia/coding_rules.md` from the repository root and use it when
present.

Local `AGENTS.md` notes remain authoritative for this repository's build and
workflow details. Shared C++ ownership, pointer, constness, and style rules
should stay aligned with XMedia `coding_rules.md` unless the project owner gives
a more specific instruction.

### C++ Ownership Snapshot

- Raw pointers are acceptable for short-lived non-owning function parameters
  only when the callee observes the object during that call and does not store,
  cache, queue, or retain the pointer.
- Do not store raw pointers in parameter, configuration, state, or message
  structs. Use `SPtr`/`SPtrC`, `std::weak_ptr`, a move-only owner, a clearly
  scoped reference-based call API, or an explicit UID/handle instead.
- When extracting a temporary non-owning pointer from `xbase::XResult`, use
  `GetPtr()` rather than `Result().get()`. Passing ownership or retaining the
  value should use `Result()` directly.
- Stable API/control identifiers should be declared as `constexpr`/static
  constants and reused from call sites instead of embedded directly in
  implementation code.

## Build

Use the Conan-backed CMake configure command that is known to work in this repository:

```powershell
cmake -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="xsdk_conan.cmake" -DCONAN_BUILD_PROFILE="vs2022_17_x86_64" -DCONAN_HOST_PROFILE="vs2022_17_x86_64" -S . -B build
```

Build with:

```powershell
cmake --build build
```

## Codex Desktop Notes

In Codex Desktop, both CMake commands above should be run with `require_escalated`.

Without escalation, this project may fail during CMake/Conan/MSBuild steps with sandbox-related errors such as:

- Conan cache write failures
- CMake timestamp/update failures
- MSBuild intermediate file access failures
