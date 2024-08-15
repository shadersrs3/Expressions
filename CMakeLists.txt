cmake_minimum_required(VERSION 3.6)
project(expression_evaluator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -fjump-tables -O3)

set(SOURCE_FILES src/main.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})