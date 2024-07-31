// Example4BlhEcefGdal.cpp : 此文件包含 "main"
// 函数。程序执行将在此处开始并结束。
//

#include <ogr_spatialref.h>

#include <iostream>

OGRSpatialReference gcs;   //地理坐标系
OGRSpatialReference ecef;  //投影坐标系

void CreateSrs() {
  // CGCS2000
  gcs.importFromEPSG(4326);

  // Tm投影
  ecef.importFromEPSG(4978);

  //# GDAL 3 changes axis order : https://github.com/OSGeo/gdal/issues/1546
  gcs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
  ecef.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
}

int main() {
  //设置Proj数据
  std::string projDataPath = getenv("GISBasic");
  projDataPath += "/share/proj";
  CPLSetConfigOption("PROJ_LIB", projDataPath.c_str());

  CreateSrs();

  OGRCoordinateTransformation* gcs2Ecef =
      OGRCreateCoordinateTransformation(&gcs, &ecef);
  OGRCoordinateTransformation* ecef2Gcs =
      OGRCreateCoordinateTransformation(&ecef, &gcs);
  if (!gcs2Ecef || !ecef2Gcs) {
    return 1;
  }

  double x = 113.6;
  double y = 38.8;
  double z = 100;
  printf("大地坐标：%.9lf\t%.9lf\t%.9lf\n", x, y, z);
  if (!gcs2Ecef->Transform(1, &x, &y, &z)) {
    return 1;
  }
  printf("地心地固坐标：%.9lf\t%.9lf\t%.9lf\n", x, y, z);

  if (!ecef2Gcs->Transform(1, &x, &y, &z)) {
    return 1;
  }
  printf("再次转换回的经纬度坐标：%.9lf\t%.9lf\t%.9lf\n", x, y, z);

  OGRCoordinateTransformation::DestroyCT(gcs2Ecef);
  gcs2Ecef = nullptr;
  OGRCoordinateTransformation::DestroyCT(ecef2Gcs);
  ecef2Gcs = nullptr;
}
