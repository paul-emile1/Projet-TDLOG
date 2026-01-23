cmake_minimum_required(VERSION 3.10)
project(Puissance4_AI)

set(CMAKE_CXX_STANDARD 17)

add_library(ai_lib SHARED src/ai_core.cpp)


add_executable(run_ai_test src/main.cpp src/ai_core.cpp)
