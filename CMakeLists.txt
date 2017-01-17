cmake_minimum_required(VERSION 3.6)
project(smatch)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

add_subdirectory(app)
add_subdirectory(lib)
