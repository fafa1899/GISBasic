# CMakeList.txt: Example5DemResample 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 判断编译器类型
message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")

# 判断编译器类型
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(">> using Clang")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    message(">> using GCC")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    message(">> using Intel C++")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    message(">> using Visual Studio C++")
	add_compile_options(/wd4996) #禁用特定警告
    add_definitions("-DUNICODE" "-D_UNICODE")
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

# 配置第三方库环境，否则定义第三方库位置 
IF(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(GDAL_INCLUDE_DIR $ENV{GISBasic}/include/gdal)
    set(GDAL_LIBRARY $ENV{GISBasic}/lib/gdal.lib)
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(GDAL REQUIRED)   
ENDIF()

# config 3rdParty
message(STATUS "GISBasic is $ENV{GISBasic}/include.")
include_directories($ENV{GISBasic}/include)

include_directories(${GDAL_INCLUDE_DIR})

# 将源代码添加到此项目的可执行文件。
add_executable (Example9Osgb "Example9Osgb.cpp" )

# 链接库
target_link_libraries(Example9Osgb PRIVATE ${GDAL_LIBRARY})
target_link_libraries(Example9Osgb PRIVATE $ENV{GISBasic}/lib/osg.lib)
target_link_libraries(Example9Osgb PRIVATE $ENV{GISBasic}/lib/osgDB.lib)
target_link_libraries(Example9Osgb PRIVATE $ENV{GISBasic}/lib/osgViewer.lib)

# TODO: 如有需要，请添加测试并安装目标。
