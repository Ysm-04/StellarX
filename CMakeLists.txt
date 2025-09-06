cmake_minimum_required(VERSION 3.15)
project(StellarX VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows特定设置
if(WIN32)
    add_definitions(-DWIN32 -D_WINDOWS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    
    # 设置Windows子系统
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()

# 设置输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 包含目录
include_directories(include)

# Windows API库
if(WIN32)
    find_library(GDI32_LIBRARY gdi32)
    find_library(USER32_LIBRARY user32)
    find_library(KERNEL32_LIBRARY kernel32)
    set(WIN32_LIBS ${GDI32_LIBRARY} ${USER32_LIBRARY} ${KERNEL32_LIBRARY})
endif()

# 创建库
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_library(StellarX STATIC ${SOURCES})

# 链接Windows库
if(WIN32)
    target_link_libraries(StellarX ${WIN32_LIBS})
endif()

# 设置库属性
set_target_properties(StellarX PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "include/StellarX/StellarX.h"
    OUTPUT_NAME "StellarX"
)

# 安装规则
install(DIRECTORY include/StellarX DESTINATION include)
install(TARGETS StellarX
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/StellarX
)

# 示例程序
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples)
    file(GLOB EXAMPLE_SOURCES "examples/*.cpp")
    foreach(example_source ${EXAMPLE_SOURCES})
        get_filename_component(example_name ${example_source} NAME_WE)
        add_executable(${example_name} ${example_source})
        target_link_libraries(${example_name} StellarX ${WIN32_LIBS})
        
        # 设置Windows子系统
        if(WIN32)
            set_target_properties(${example_name} PROPERTIES
                LINK_FLAGS "/SUBSYSTEM:WINDOWS"
            )
        endif()
        
        # 安装示例
        install(TARGETS ${example_name} DESTINATION bin)
    endforeach()
endif()

# 文档生成选项
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    find_package(Doxygen)
    if(Doxygen_FOUND)
        set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)
        set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
        set(DOXYGEN_PROJECT_NAME "StellarX GUI Framework")
        set(DOXYGEN_PROJECT_NUMBER ${PROJECT_VERSION})
        set(DOXYGEN_PROJECT_BRIEF "A lightweight, modular C++ GUI framework for Windows")
        set(DOXYGEN_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
        set(DOXYGEN_RECURSIVE YES)
        set(DOXYGEN_EXTRACT_ALL YES)
        set(DOXYGEN_EXTRACT_PRIVATE YES)
        set(DOXYGEN_EXTRACT_STATIC YES)
        set(DOXYGEN_SOURCE_BROWSER YES)
        set(DOXYGEN_GENERATE_TREEVIEW YES)
        set(DOXYGEN_HAVE_DOT YES)
        set(DOXYGEN_CALL_GRAPH YES)
        set(DOXYGEN_CALLER_GRAPH YES)
        
        doxygen_add_docs(
            docs
            ${DOXYGEN_INPUT}
            COMMENT "Generate HTML documentation"
        )
        
        install(DIRECTORY ${DOXYGEN_OUTPUT_DIRECTORY}/html DESTINATION docs)
    endif()
endif()

# 打包配置
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION "A lightweight, modular C++ GUI framework for Windows")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "星垣 (StellarX) - A lightweight Windows GUI framework")
set(CPACK_PACKAGE_VENDOR "StellarX Development Team")
set(CPACK_PACKAGE_CONTACT "contact@stellarx.dev")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "StellarX")
set(CPACK_NSIS_MODIFY_PATH ON)

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
else()
    set(CPACK_GENERATOR "ZIP")
endif()

include(CPack)