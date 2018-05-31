cmake_minimum_required(VERSION 3.8)
project(OS_Ex3)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES MapReduceClient.h MapReduceFramework.h MapReduceFramework.cpp)
set(SAMPLE_CLIENT MapReduceClient.h MapReduceFramework.h MapReduceFramework.cpp SampleClient.cpp)
add_executable(OS_Ex3 ${SOURCE_FILES})
add_executable(client ${SAMPLE_CLIENT})