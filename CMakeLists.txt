project(hasher)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++0x -g -Wall")
link_libraries(boost_thread boost_program_options pHash rt crypto)
add_executable(streamer streamer.cpp)

add_executable(capturer capturer.cpp)

