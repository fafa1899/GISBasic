# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

project ("Chaper7")

# 包含子项目。
add_subdirectory ("Example1DemModel")
add_subdirectory ("Example2DemModel")
add_subdirectory ("Example3DemModel")
add_subdirectory ("Example8CreateBuilding")
add_subdirectory ("Example9Osgb")
add_subdirectory ("Example10ObliqueIndexes")

