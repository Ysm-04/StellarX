cmake_minimum_required(VERSION 3.15)

project(StellarX VERSION 4.0.0 LANGUAGES CXX)

option(STELLARX_BUILD_EXAMPLES "Build the standalone StellarX examples" OFF)
option(STELLARX_USE_STATIC_MSVC_RUNTIME "Use the static MSVC runtime (/MT or /MTd)" OFF)

if(MSVC AND STELLARX_USE_STATIC_MSVC_RUNTIME)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(STELLARX_SOURCES
    src/Button.cpp
    src/Canvas.cpp
    src/Control.cpp
    src/Dialog.cpp
    src/Label.cpp
    src/MessageBox.cpp
    src/SxLog.cpp
    src/TabControl.cpp
    src/Table.cpp
    src/TextBox.cpp
    src/Window.cpp
)

set(STELLARX_PRIVATE_HEADERS
    src/internal/SxEventLoopInternal.h
    src/internal/SxTextInternal.h
)

add_library(StellarX STATIC ${STELLARX_SOURCES} ${STELLARX_PRIVATE_HEADERS})
target_include_directories(StellarX
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/StellarX"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/internal"
)
target_compile_features(StellarX PUBLIC cxx_std_17)

if(MSVC)
    # 源文件统一保存为 UTF-8；当前 StellarX/EasyX 的窄字符串绘制链路按
    # Windows 中文 ANSI 解释，因此执行字符集显式转换为 CP936。
    set(STELLARX_MSVC_TEXT_OPTIONS
        /source-charset:utf-8
        /execution-charset:.936
    )
    target_compile_options(StellarX PRIVATE ${STELLARX_MSVC_TEXT_OPTIONS})

    # EasyX 通常安装在 Visual Studio 的 VC/Auxiliary/VS 目录，但该目录不一定
    # 自动加入 LIB。允许通过 EASYX_ROOT 覆盖，并在找不到时给出明确错误。
    set(EASYX_ROOT "" CACHE PATH "EasyX installation root")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(EASYX_ARCH x64)
    else()
        set(EASYX_ARCH x86)
    endif()
    find_library(STELLARX_EASYX_LIBRARY
        NAMES EasyXa
        HINTS
            "${EASYX_ROOT}/lib/${EASYX_ARCH}"
            "$ENV{VCToolsInstallDir}/../../../Auxiliary/VS/lib/${EASYX_ARCH}"
    )
    if(NOT STELLARX_EASYX_LIBRARY)
        message(FATAL_ERROR "EasyXa.lib not found. Install EasyX for Visual Studio or set EASYX_ROOT.")
    endif()

    target_link_libraries(StellarX PUBLIC "${STELLARX_EASYX_LIBRARY}" user32 gdi32 shell32)
endif()

if(STELLARX_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
