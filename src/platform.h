#ifndef CLOUD_SDK_PLATFORM_H
#define CLOUD_SDK_PLATFORM_H

#ifdef _WIN32
    #ifndef HAVE_INET_PTON
        #define HAVE_INET_PTON // Do we need this ? Move to CMakeList.txt ?
    #endif

    #ifndef NOMINMAX
        #define NOMINMAX // Move to CMakeList.txt ?
    #endif

    #include <atlbase.h> // For CA2W, _T()
    #include <windows.h>
#elif defined(__linux__)
    #define GCC_COMPILER (defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER))
#elif defined(__APPLE__)
    #define APPLE_COMPILER TRUE
#endif // _WIN32
#endif // CLOUD_SDK_PLATFORM_H
