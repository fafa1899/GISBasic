# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

project ("Chaper6")

# 包含子项目。
add_subdirectory ("Example5DemResample")
add_subdirectory ("Example6Tin2Dem")
add_subdirectory ("Example7HillshadeMap")
add_subdirectory ("Example8HillshadeMap")
add_subdirectory ("Example9HillshadeMap")


