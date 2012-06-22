project(hasher)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++0x -g -Wall")
link_libraries(boost_thread pHash)
add_executable(${PROJECT_NAME} ${SRC_LIST})

