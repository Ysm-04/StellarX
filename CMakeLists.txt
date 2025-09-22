cmake_minimum_required(VERSION 3.10)

# 项目定义
project(StellarX VERSION 2.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 包含头文件目录（目前头文件都在根目录）
include_directories(${CMAKE_SOURCE_DIR})

# 源文件收集
file(GLOB_RECURSE SOURCES
    "${CMAKE_SOURCE_DIR}/*.cpp"
)

# 生成可执行文件
add_executable(StellarX ${SOURCES})
