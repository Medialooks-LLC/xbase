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

add_library(${PROJECT_NAME}
    STATIC
    ${FILES}
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME}
                          PUBLIC
                          IPHLPAPI
    )
else()
    target_link_libraries(${PROJECT_NAME}
                          PUBLIC
    )
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} stdc++fs)
endif()

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ../include
        ..
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${FILES})
