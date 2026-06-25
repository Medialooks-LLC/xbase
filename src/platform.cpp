#include "xbase/platform.h"

#include <string>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
    #include <limits.h>
    #include <unistd.h>
    #ifdef __APPLE__
        #include <mach-o/dyld.h>
    #endif
#endif

namespace xsdk::xbase::platform {

XResult<std::filesystem::path> CurrentProcessPath()
{
#ifdef _WIN32
    wchar_t path[MAX_PATH] = {};
    if (!GetModuleFileNameW(nullptr, path, MAX_PATH))
        return std::error_code(::GetLastError(), std::system_category());
#else
    char path[PATH_MAX] = {};
    #if defined(__linux__)
    const int len = readlink("/proc/self/exe", path, PATH_MAX);
    if (len <= 0 || len >= PATH_MAX)
        return std::make_error_code(std::errc::filename_too_long);
    #elif defined(__APPLE__)
    uint32_t max_size = PATH_MAX;
    const int len = _NSGetExecutablePath(path, &max_size);
    if (len == -1)
        return std::make_error_code(std::errc::filename_too_long);
    #endif
#endif

    return std::filesystem::path(path);
}

XResult<std::filesystem::path> CurrentProcessFolder()
{
    auto path = CurrentProcessPath();
    if (path.HasError())
        return path;
    return path.MoveResult().remove_filename();
}

XResult<std::filesystem::path> CurrentProcessFilename()
{
    auto path = CurrentProcessPath();
    if (path.HasError())
        return path;
    return path.MoveResult().filename();
}

XResult<std::filesystem::path> LibraryPath()
{
#ifdef _WIN32
    wchar_t path[MAX_PATH] = {};
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(LibraryPath),
                            &module)) {
        return std::error_code(::GetLastError(), std::system_category());
    }

    const DWORD len = GetModuleFileNameW(module, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
        return std::make_error_code(std::errc::filename_too_long);
#else
    char path[PATH_MAX] = {};
    Dl_info info = {};
    if (!dladdr(reinterpret_cast<void*>(LibraryPath), &info))
        return std::make_error_code(std::errc::bad_address);

    const auto len = std::char_traits<char>::length(info.dli_fname);
    if (len == 0 || len >= PATH_MAX)
        return std::make_error_code(std::errc::filename_too_long);

    std::char_traits<char>::copy(path, info.dli_fname, len);
    path[len] = '\0';
#endif

    return std::filesystem::path(path);
}

XResult<std::filesystem::path> LibraryFolder()
{
    auto path = LibraryPath();
    if (path.HasError())
        return path;
    return path.MoveResult().remove_filename();
}

} // namespace xsdk::xbase::platform
