# 输出cmake版本提示
message(STATUS "The CMAKE_VERSION is ${CMAKE_VERSION}.")

# cmake的最低版本要求
cmake_minimum_required (VERSION 3.9)

# 工程名称、版本、语言
project ("Example5-12-Harris" VERSION 0.1 LANGUAGES CXX)

# 支持当前目录
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# cpp17支持
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 判断编译器类型
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(">> using Clang")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    message(">> using GCC")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    message(">> using Intel C++")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    message(">> using Visual Studio C++")
	add_compile_options(/wd4996 /wd4251) #禁用特定警告
    # 设置预编译头
    add_definitions("-DUNICODE" "-D_UNICODE" "-DNOMINMAX") 
else()
    message(">> unknow compiler.")
endif()

# 动态库前缀与后缀
IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(LibraryPrefix lib)
    set(LibraryPostfix so)
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(LibraryPrefix )
    set(LibraryPostfix lib)
ENDIF()

# 配置第三方库环境
find_package(OpenCV REQUIRED)

# 将源代码添加到此项目的可执行文件。
add_executable (${PROJECT_NAME} "Example5-12-Harris.cpp" )

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_world)

# TODO: 如有需要，请添加测试并安装目标。
