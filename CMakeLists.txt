cmake_minimum_required(VERSION 3.27)
set(MODULE_PATH ${CMAKE_MODULE_PATH})

cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0087 NEW)
cmake_policy(SET CMP0148 NEW)

option(WITH_STATIC_ANALYSIS "Perform static analysis via clang-tidy" OFF)
option(WITH_ADDRESS_SANITIZER "Add additional memory checks" OFF)
option(WITH_WINDOWS_CI_BUILD "Set ON when do windows build on CI" OFF)

if(WIN32)
    set(MSVC_TOOLSET_VERSION "143" CACHE STRING MSVC_TOOLSET_VERSION)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_SYSTEM_VERSION "10.0" CACHE STRING INTERNAL FORCE)
    if(WITH_WINDOWS_CI_BUILD) # do not set /MP on CI build
        set(CMAKE_CXX_FLAGS "/GR /EHsc /DWIN32 /nologo /Zi /Gm- /GS /Gy- /Zc:wchar_t /Zc:inline /fp:precise /W3 /WX /Zc:forScope /utf-8 /D_CRT_RAND_S" CACHE STRING CMAKE_CXX_FLAGS)
        set(CMAKE_C_FLAGS "/DWIN32 /GR /EHsc /nologo /Zi /Gm- /GS /Gy- /Zc:wchar_t /Zc:inline /fp:precise /W3 /WX /Zc:forScope /utf-8" CACHE STRING CMAKE_C_FLAGS)
    else()
        set(CMAKE_CXX_FLAGS "/GR /EHsc /DWIN32 /nologo /Zi /Gm- /GS /Gy- /Zc:wchar_t /Zc:inline /fp:precise /W3 /WX /Zc:forScope /utf-8 /D_CRT_RAND_S /MP" CACHE STRING CMAKE_CXX_FLAGS)
        set(CMAKE_C_FLAGS "/DWIN32 /GR /EHsc /nologo /Zi /Gm- /GS /Gy- /Zc:wchar_t /Zc:inline /fp:precise /W3 /WX /Zc:forScope /utf-8 /MP" CACHE STRING CMAKE_C_FLAGS)
    endif()

    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Ob0 /Od /RTC1 /MTd" CACHE STRING CMAKE_CXX_FLAGS_DEBUG)
    set(CMAKE_C_FLAGS_DEBUG "/Zi /Ob0 /Od /RTC1 /MTd" CACHE STRING CMAKE_C_FLAGS_DEBUG)

    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Zi /O2 /Ob1 /DNDEBUG /MT" CACHE STRING CMAKE_CXX_FLAGS_RelWithDebInfo)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/Zi /O2 /Ob1 /DNDEBUG /MT" CACHE STRING CMAKE_C_FLAGS_RelWithDebInfo)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE WIN32_LEAN_AND_MEAN)
else()
    if(APPLE)
        set(CMAKE_CXX_FLAGS "-fPIC -Wno-multichar -Wno-enum-compare -Wno-nonportable-include-path -Wno-delete-abstract-non-virtual-dtor -Werror" CACHE STRING CMAKE_CXX_FLAGS)
        set(CMAKE_C_FLAGS "-fPIC -Wno-nonportable-include-path -Wno-delete-abstract-non-virtual-dtor -Werror" CACHE STRING CMAKE_C_FLAGS)
        set(CMAKE_INSTALL_RPATH "@loader_path")
    else()
        set(CMAKE_CXX_FLAGS "-fPIC -Wno-multichar -Wno-enum-compare -Werror" CACHE STRING CMAKE_CXX_FLAGS)
        set(CMAKE_C_FLAGS "-fPIC -Werror" CACHE STRING CMAKE_C_FLAGS)
        set(CMAKE_INSTALL_RPATH "$ORIGIN")
    endif()

    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

    set(CMAKE_CXX_FLAGS_DEBUG "-g -D_DEBUG" CACHE STRING CMAKE_CXX_FLAGS_DEBUG)
    set(CMAKE_C_FLAGS_DEBUG "-g -D_DEBUG" CACHE STRING CMAKE_C_FLAGS_DEBUG)

    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -DNDEBUG" CACHE STRING CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -O2 -DNDEBUG" CACHE STRING CMAKE_C_FLAGS_RELWITHDEBINFO)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type")
    set(CMAKE_CONFIGURATION_TYPES "RelWithDebInfo;Debug" CACHE STRING CMAKE_CONFIGURATION_TYPES)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES})
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(xbase)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

if(CMAKE_CONFIGURATION_TYPES)
    message("CMAKE_CONFIGURATION_TYPES=${CMAKE_CONFIGURATION_TYPES}")
    set(TYPES "${CMAKE_CONFIGURATION_TYPES}")
else()
    set(TYPES "${CMAKE_BUILD_TYPE}")
    message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif()

find_package(GTest REQUIRED)

if(WITH_STATIC_ANALYSIS AND NOT WIN32)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy" "-header-filter=." "-line-filter=[{'name':'.h'}, {'name':'.hpp'}]")
endif()

if(WITH_ADDRESS_SANITIZER AND NOT WIN32)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

add_subdirectory(src)
add_subdirectory(tests)

