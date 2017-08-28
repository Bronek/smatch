cmake_minimum_required(VERSION 3.6)
# The OLD behavior for CMP0037 policy is to allow creating targets with reserved names, such as "test"
cmake_policy(SET CMP0037 OLD)
project(smatch)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra")

add_subdirectory(app)
add_subdirectory(test)
add_subdirectory(lib)
