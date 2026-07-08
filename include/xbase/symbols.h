#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
    #define XSDK_SYMBOL_EXPORT __declspec(dllexport)
    #define XSDK_SYMBOL_IMPORT __declspec(dllimport)
    #define XSDK_SYMBOL_HIDDEN
#elif defined(__GNUC__) || defined(__clang__)
    #define XSDK_SYMBOL_EXPORT __attribute__((visibility("default")))
    #define XSDK_SYMBOL_IMPORT __attribute__((visibility("default")))
    #define XSDK_SYMBOL_HIDDEN __attribute__((visibility("hidden")))
#else
    #define XSDK_SYMBOL_EXPORT
    #define XSDK_SYMBOL_IMPORT
    #define XSDK_SYMBOL_HIDDEN
#endif

#define XSDK_SYMBOL_LOCAL XSDK_SYMBOL_HIDDEN

#if defined(XBASE_STATIC_DEFINE)
    #define XBASE_API
#elif defined(XBASE_BUILDING_DLL)
    #define XBASE_API XSDK_SYMBOL_EXPORT
#else
    #define XBASE_API XSDK_SYMBOL_IMPORT
#endif

#define XBASE_PRIVATE XSDK_SYMBOL_LOCAL
