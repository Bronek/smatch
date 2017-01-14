cmake_minimum_required(VERSION 3.6)
project(smatch)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

set(SOURCE_FILES
        lib/book.cpp
        lib/book.hpp
        lib/engine.hpp
        lib/input.cpp
        lib/input.hpp
        main.cpp
        lib/streams.hpp
        lib/types.hpp)

add_executable(smatch ${SOURCE_FILES})
