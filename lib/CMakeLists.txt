cmake_minimum_required(VERSION 3.6)
project(lib)

set(LIB_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

if (TARGET ${PROJECT_NAME})
    message("")
else()
    set(SOURCE_FILES
        book.cpp
        book.hpp
        engine.hpp
        runner.hpp
        input.hpp
        stream.cpp
        stream.hpp
        types.hpp
        )

   add_library(${PROJECT_NAME} ${SOURCE_FILES})
endif()
