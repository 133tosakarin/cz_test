cmake_minimum_required(VERSION 3.10)
project(MyProject)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/include)
message("source_dir is: ${SOURCE_DIR}")

include_directories(${SOURCE_DIR})

# 包含子目录
add_subdirectory(src/concurrency)

# 添加测试目录
add_subdirectory(test)  
