#pragma once

#include "xbase/symbols.h"
#include "xbase/xresult.hpp"

#include <filesystem>

namespace xsdk::xbase::platform {

XBASE_API XResult<std::filesystem::path> CurrentProcessPath();

XBASE_API XResult<std::filesystem::path> CurrentProcessFolder();

XBASE_API XResult<std::filesystem::path> CurrentProcessFilename();

XBASE_API XResult<std::filesystem::path> LibraryPath();

XBASE_API XResult<std::filesystem::path> LibraryFolder();

} // namespace xsdk::xbase::platform
