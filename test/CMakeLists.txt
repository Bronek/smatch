cmake_minimum_required(VERSION 3.6)
# The OLD behavior for CMP0037 policy is to allow creating targets with reserved names, such as "test"
cmake_policy(SET CMP0037 OLD)
project(test)

set(SOURCE_FILES
    main.cpp
    catch.hpp
    core.cpp
    book.cpp
    )

add_subdirectory(../lib lib)
include_directories(${LIB_INCLUDE})

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} lib)
