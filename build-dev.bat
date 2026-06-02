@echo off
setlocal EnableExtensions

set "MODE=%~1"
set "CONFIG=%~2"
set "TARGET=%~3"
set "BUILD_NOW=%~4"

if "%MODE%"=="" (
    set "MODE=msvc"
)

if /I "%TARGET%"=="--build" (
    set "TARGET="
    set "BUILD_NOW=--build"
)

if /I "%MODE%"=="help" goto :usage
if /I "%MODE%"=="--help" goto :usage
if /I "%MODE%"=="/?" goto :usage

if "%CONFIG%"=="" set "CONFIG=RelWithDebInfo"

if /I not "%CONFIG%"=="Debug" if /I not "%CONFIG%"=="RelWithDebInfo" (
    echo Unsupported configuration "%CONFIG%".
    echo Expected Debug or RelWithDebInfo.
    exit /b 1
)

if not "%BUILD_NOW%"=="" if /I not "%BUILD_NOW%"=="--build" (
    echo Unsupported option "%BUILD_NOW%".
    echo Use --build to run the build after configure.
    exit /b 1
)

if /I "%MODE%"=="msvc" goto :build_msvc
if /I "%MODE%"=="clang-ninja" goto :build_clang_ninja
if /I "%MODE%"=="clang-vs" goto :build_clang_vs

echo Unknown mode "%MODE%".
echo.
goto :usage

:build_msvc
set "BUILD_DIR=build"
echo [configure] %MODE% ^| %CONFIG% ^| %BUILD_DIR%
cmake -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=xsdk_conan.cmake ^
  -DCONAN_BUILD_PROFILE=vs2022_17_x86_64 ^
  -DCONAN_HOST_PROFILE=vs2022_17_x86_64 ^
  -S . -B "%BUILD_DIR%"
if errorlevel 1 exit /b 1
goto :build_vs

:build_clang_ninja
where clang-cl >nul 2>nul
if errorlevel 1 (
    echo clang-cl was not found in PATH.
    echo Run this script from the VS x64 Native Tools prompt or after VsDevCmd.bat.
    exit /b 1
)

set "BUILD_DIR=build_clang"
echo [configure] %MODE% ^| %CONFIG% ^| %BUILD_DIR%
cmake -G Ninja ^
  -DUSE_CLANG=ON ^
  -DCMAKE_BUILD_TYPE=%CONFIG% ^
  -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=xsdk_conan.cmake ^
  -DCONAN_BUILD_PROFILE=vs2022_17_x86_64 ^
  -DCONAN_HOST_PROFILE=vs2022_17_x86_64 ^
  -S . -B "%BUILD_DIR%"
if errorlevel 1 exit /b 1

if /I not "%BUILD_NOW%"=="--build" (
    echo [done] configure only
    exit /b 0
)

if "%TARGET%"=="" (
    echo [build] %BUILD_DIR%
    cmake --build "%BUILD_DIR%"
) else (
    echo [build] %BUILD_DIR% ^| target=%TARGET%
    cmake --build "%BUILD_DIR%" --target "%TARGET%"
)
exit /b %errorlevel%

:build_clang_vs
set "BUILD_DIR=build_clang_vs"
echo [configure] %MODE% ^| %CONFIG% ^| %BUILD_DIR%
cmake -G "Visual Studio 17 2022" -A x64 -T ClangCL ^
  -DUSE_CLANG=ON ^
  -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=xsdk_conan.cmake ^
  -DCONAN_BUILD_PROFILE=vs2022_17_x86_64 ^
  -DCONAN_HOST_PROFILE=vs2022_17_x86_64 ^
  -S . -B "%BUILD_DIR%"
if errorlevel 1 exit /b 1
goto :build_vs

:build_vs
if /I not "%BUILD_NOW%"=="--build" (
    echo [done] configure only
    exit /b 0
)

if "%TARGET%"=="" (
    echo [build] %BUILD_DIR% ^| config=%CONFIG%
    cmake --build "%BUILD_DIR%" --config "%CONFIG%"
) else (
    echo [build] %BUILD_DIR% ^| config=%CONFIG% ^| target=%TARGET%
    cmake --build "%BUILD_DIR%" --config "%CONFIG%" --target "%TARGET%"
)
exit /b %errorlevel%

:usage
echo Usage:
echo   build-dev.bat [msvc^|clang-ninja^|clang-vs] [Debug^|RelWithDebInfo] [target] [--build]
echo.
echo Examples:
echo   build-dev.bat
echo   build-dev.bat msvc
echo   build-dev.bat msvc Debug xbase --build
echo   build-dev.bat clang-ninja
echo   build-dev.bat clang-ninja Debug xbase_tests --build
echo   build-dev.bat clang-vs Debug --build
echo.
echo Notes:
echo   - mode defaults to msvc when omitted
echo   - clang modes expect clang-cl to be available in PATH
echo   - run clang builds from the VS x64 Native Tools prompt or after VsDevCmd.bat
echo   - USE_CLANG switches the compiler, the generator is selected by the mode
echo   - build is not started automatically; pass --build when needed
exit /b 1
