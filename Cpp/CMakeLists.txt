# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.9)

project ("Example")

# 包含子项目。
add_subdirectory ("Example3-1-SRS")
add_subdirectory ("Example3-2-SRSTmConvertor")

