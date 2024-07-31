﻿// Example3WebMercatorConvertor.cpp : 此文件包含 "main"
// 函数。程序执行将在此处开始并结束。
//

#include <ogr_spatialref.h>

#include <iostream>

OGRSpatialReference gcs;  //地理坐标系
OGRSpatialReference pcs;  //投影坐标系

void CreateSrs() {
  // CGCS2000
  gcs.importFromEPSG(4326);

  // Tm投影
  pcs.importFromEPSG(3857);

  //# GDAL 3 changes axis order : https://github.com/OSGeo/gdal/issues/1546
  gcs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
  pcs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
}

int main() {
  //设置Proj数据
  std::string projDataPath = getenv("GISBasic");
  projDataPath += "/share/proj";
  CPLSetConfigOption("PROJ_LIB", projDataPath.c_str());

  CreateSrs();

  OGRCoordinateTransformation* lonLat2XY =
      OGRCreateCoordinateTransformation(&gcs, &pcs);
  OGRCoordinateTransformation* xy2LonLat =
      OGRCreateCoordinateTransformation(&pcs, &gcs);
  if (!lonLat2XY || !xy2LonLat) {
    return 1;
  }

  double x = 113.6;
  double y = 38.8;
  printf("经纬度坐标：%.9lf\t%.9lf\n", x, y);
  if (!lonLat2XY->Transform(1, &x, &y)) {
    return 1;
  }
  printf("平面坐标：%.9lf\t%.9lf\n", x, y);

  if (!xy2LonLat->Transform(1, &x, &y)) {
    return 1;
  }
  printf("再次转换回的经纬度坐标：%.9lf\t%.9lf\n", x, y);

  OGRCoordinateTransformation::DestroyCT(lonLat2XY);
  lonLat2XY = nullptr;
  OGRCoordinateTransformation::DestroyCT(xy2LonLat);
  xy2LonLat = nullptr;
}
