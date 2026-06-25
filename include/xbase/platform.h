#pragma once

#include "xbase/xresult.hpp"

#include <filesystem>

namespace xsdk::xbase::platform {

XResult<std::filesystem::path> CurrentProcessPath();

XResult<std::filesystem::path> CurrentProcessFolder();

XResult<std::filesystem::path> CurrentProcessFilename();

XResult<std::filesystem::path> LibraryPath();

XResult<std::filesystem::path> LibraryFolder();

} // namespace xsdk::xbase::platform
