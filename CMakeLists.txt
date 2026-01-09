cmake_minimum_required(VERSION 3.10)

# 项目定义
project(StellarX VERSION 2.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 为了支持 out-of-source builds，创建构建目录
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)

# 设置生成的二进制文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含头文件目录（目前头文件都在根目录）
include_directories(${CMAKE_SOURCE_DIR})

# 通过选项设置是否启用调试信息
option(USE_DEBUG "Build with debug information" OFF)
if(USE_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
else()
    set(CMAKE_BUILD_TYPE Release)
endif()

# 查找源文件
file(GLOB_RECURSE SOURCES
    "${CMAKE_SOURCE_DIR}/*.cpp"
)

# 生成可执行文件
add_executable(StellarX ${SOURCES})

# 可以选择性地查找外部库并链接（例如 Boost，SDL2等）
# FindPackage(Boost REQUIRED)
# target_link_libraries(StellarX Boost::Boost)

# 为外部依赖配置路径
# set(Boost_DIR "path/to/boost")
# find_package(Boost REQUIRED)

# 如果有额外的库需要链接，继续在此处添加
# target_link_libraries(StellarX Boost::Boost)

