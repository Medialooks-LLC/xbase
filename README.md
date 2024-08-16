# xbase library

The `xbase` library is a part of the `xsdk` framework to create high performance video processing and management tools.

The `xbase` library provides a set of C++ classes and utilities for object-oriented programming and cross-platform development which used in `xsdk`.

## Features

- `IObject`: A base interface for creating unique and shared objects.
- `xobject`: A namespace providing utilities for working with `IObject` via smart pointers.
- `IData`:   A base interface for data handling, such as data adding, data getting, data removing, and data resetting.
- `xdata`:   A namespace providing utilities for working with `IData` via smart pointers.
- `PtrBase`: A base class template for managing smart pointers of a derived classes.
- `TypeUid`: A template classs to obtain a compile-time constant UID for a given C++ type.


## Usage

Examples of usage you can find in the tests.

## Build and Dependencies

The xbase library can be built using a C++17 compatible compiler. There are only one external dependency - Gtest for execute unit tests.

The library can be built with following command:
 ```shell
 cmake -S . -B build
 cmake --build build
 ```

To resolve dependency with custom build of GTest need to set necessary environment and run `cmake` something like follow:
```
cmake -DGTEST_INCLUDE_DIR='../googletest/googletest/include' -DGTEST_LIBRARY='../googletest/build/lib/Debug/gtest.lib' -DGTEST_MAIN_LIBRARY='../googletest/build/lib/Debug/gtest_main.lib' -S . -B build
cmake --build build
```

Also it can be build with using Conan:
```
cmake -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="xsdk_conan.cmake" -S . -B build
cmake --build build
```

## License

The xbase library is licensed under the [GPL v3 License](LICENSE).
