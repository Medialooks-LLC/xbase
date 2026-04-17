# AGENTS.md

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
