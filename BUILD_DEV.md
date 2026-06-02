# Developer Build Notes

This file documents the Conan-backed local build flows used by `xbase` developers.
It is intentionally separate from `README.md`, because external users often build
with preinstalled dependencies or consume already prepared artifacts.

## Prerequisites

- Windows developers should use the Visual Studio 2022 toolchain.
- Clang builds should be started from the `x64 Native Tools Command Prompt for VS 2022`
  or after running `VsDevCmd.bat`, so `clang-cl` is available in `PATH`.
- Conan-backed configure uses:
  - `-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=xsdk_conan.cmake`
  - `-DCONAN_BUILD_PROFILE=vs2022_17_x86_64`
  - `-DCONAN_HOST_PROFILE=vs2022_17_x86_64`

## Supported local modes

- `MSVC + Visual Studio/MSBuild`
- `clang-cl + Ninja`
- `clang-cl + Visual Studio/MSBuild`

`USE_CLANG=ON` selects the `clang-cl` compiler setup on Windows.
The generator is still chosen externally:

- `-G Ninja` for the Ninja flow
- `-G "Visual Studio 17 2022" -A x64` for MSVC
- `-G "Visual Studio 17 2022" -A x64 -T ClangCL` for `clang-cl` with MSBuild

Use separate build directories per mode. Do not reuse the same CMake cache when
switching generator or compiler.

## Convenience script

Use [build-dev.bat](E:/work/xbase/build-dev.bat) from the repository root:

```bat
build-dev.bat [msvc|clang-ninja|clang-vs] [Debug|RelWithDebInfo] [target] [--build]
```

By default the script only configures the build directory.
Pass `--build` to start compilation after configure.
If the mode is omitted, the script uses `msvc`.

Examples:

```bat
build-dev.bat
build-dev.bat msvc
build-dev.bat msvc Debug xbase --build
build-dev.bat clang-ninja
build-dev.bat clang-ninja Debug xbase_tests --build
build-dev.bat clang-vs Debug --build
```

## Notes

- `clang-cl + Visual Studio/MSBuild` uses `lld-link` under the hood.
- Conan `gtest` debug packages can emit `LNK4099` warnings with `lld-link`.
  Test targets suppress this warning locally via `/IGNORE:4099`.
- If you only need the library, build the `xbase` target explicitly:

```bat
build-dev.bat clang-vs Debug xbase --build
```
