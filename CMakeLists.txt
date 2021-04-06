cmake_minimum_required(VERSION 3.16)
project(cpp_challenge)

set(CMAKE_CXX_STANDARD 14 )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )
add_executable(app buggy.cxx)
target_link_libraries(app PUBLIC pthread)

