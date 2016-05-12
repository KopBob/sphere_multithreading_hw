cmake_minimum_required(VERSION 3.5)
project(p5)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#set(SOURCE_FILES tests.cpp)
#set(SOURCE_FILES fork_test.cpp)
#set(SOURCE_FILES main.cpp)
#set(SOURCE_FILES cpp_main.cpp)
set(SOURCE_FILES shell.cpp)
#set(SOURCE_FILES tets2.cpp)
#set(SOURCE_FILES pipe_test.cpp)
add_executable(p5 ${SOURCE_FILES})

