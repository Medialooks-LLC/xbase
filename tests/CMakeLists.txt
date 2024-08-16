cmake_minimum_required(VERSION 3.10)

project(xbase_tests)

FILE(GLOB FILES
    ../../include/*.h
    *.cpp
	*.hpp
	*.h
)

include_directories(
        ../../include
)

if(MSVC)
    add_compile_options(/bigobj)
endif()

add_executable(${PROJECT_NAME}
               ${FILES}
)

target_link_libraries(${PROJECT_NAME}
                        GTest::gtest_main
                        xbase
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${FILES})
