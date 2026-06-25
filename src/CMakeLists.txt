cmake_minimum_required(VERSION 3.10)

project(xbase)

file(GLOB_RECURSE FILES
    ../include/*.*
    *.h
    *.cpp
)

include_directories(
        ../include
)
if(DYNAMIC_BUILD)
add_library(${PROJECT_NAME}
    SHARED
    ${FILES}
)
else()
add_library(${PROJECT_NAME}
    STATIC
    ${FILES}
)
endif()
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
                          PUBLIC
                          IPHLPAPI
    )
    set_target_properties(xbase PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME}
                          PUBLIC
                          stdc++fs
    )
endif()

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ../include
        ..
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${FILES})
