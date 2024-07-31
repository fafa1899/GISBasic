# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.9)

project ("Example")

# 包含子项目。
add_subdirectory ("Example3-1-SRS")
add_subdirectory ("Example3-2-SRSTmConvertor")
add_subdirectory ("Example3-3-WebMercatorConvertor")
add_subdirectory ("Example3-4-BlhEcefGdal")
add_subdirectory ("Example3-5-BlhEcef")
add_subdirectory ("Example3-6-EnuConvertor")
add_subdirectory ("Example4-1-VectorConvert") 
add_subdirectory ("Example4-2-SpatialRelation") 
add_subdirectory ("Example4-3-CalPointFromLine")
add_subdirectory ("Example4-4-CalDistancePointAndLine")
add_subdirectory ("Example4-5-LineIntersection")
add_subdirectory ("Example4-6-LineOrthogonIntersection")
add_subdirectory ("Example4-7-PointInTriangle")
add_subdirectory ("Example4-8-PointInPolygon")
add_subdirectory ("Example4-9-Points2Tin")
add_subdirectory ("Example4-10-PolygonTriangulation")
add_subdirectory ("Example5-1-RasterBase")
add_subdirectory ("Example5-2-CreateRaster")
add_subdirectory ("Example5-3-GeoInfo")
add_subdirectory ("Example5-4-ReadAndWriteRaster")
