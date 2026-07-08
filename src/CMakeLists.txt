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
target_compile_definitions(${PROJECT_NAME} PRIVATE XBASE_BUILDING_DLL)
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
else()
add_library(${PROJECT_NAME}
    STATIC
    ${FILES}
)
target_compile_definitions(${PROJECT_NAME} PUBLIC XBASE_STATIC_DEFINE)
endif()
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
                          PUBLIC
                          IPHLPAPI
    )
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
