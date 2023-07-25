// Example2DemWriter.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <gdal_priv.h>

#include <iostream>

using namespace std;

int demColumn = 512;
int demRow = 512;

int main() {
  GDALAllRegister();  //注册格式

  string workDir = getenv("GISBasic");
  string demPath = workDir + "/../Data/Terrain/dst.tif";

  GDALDriver* pDriver =
      GetGDALDriverManager()->GetDriverByName("GTIFF");  //图像驱动
  char** ppszOptions = NULL;
  ppszOptions =
      CSLSetNameValue(ppszOptions, "BIGTIFF", "IF_NEEDED");  //配置图像信息
  GDALDataset* dem = pDriver->Create(demPath.c_str(), demColumn, demRow, 1,
                                     GDT_Float32, ppszOptions);
  if (!dem) {
    printf("Can't Write Image!");
    return false;
  }

  //空间参考
  OGRSpatialReference spatialReference;
  spatialReference.importFromEPSG(3857);  // web墨卡托坐标系
  char* pszWKT = nullptr;
  spatialReference.exportToWkt(&pszWKT);
  dem->SetProjection(pszWKT);
  CPLFree(pszWKT);
  pszWKT = nullptr;

  //坐标信息
  double padfTransform[6] = {
      5,    //左上角点坐标X
      10,   // X方向的分辨率
      0,    //旋转系数，如果为0，就是标准的正北向图像
      95,   //左上角点坐标Y
      0,    //旋转系数，如果为0，就是标准的正北向图像
      -10,  // Y方向的分辨率
  };
  dem->SetGeoTransform(padfTransform);

  double noDataValue = -99;
  dem->GetRasterBand(1)->SetNoDataValue(noDataValue);

  size_t demBufNum = (size_t)demColumn * demRow;
  vector<float> demBuf(demBufNum, 0);

  for (auto& height : demBuf) {
    height = rand() % 100;
  }

  for (int yi = 0; yi < 100; yi++) {
    for (int xi = 400; xi < demColumn; xi++) {
      size_t m = (size_t)demColumn * yi + xi;
      demBuf[m] = noDataValue;
    }
  }

  size_t depth = sizeof(float);
  dem->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, demColumn, demRow,
                                  demBuf.data(), demColumn, demRow, GDT_Float32,
                                  depth, demColumn * depth);

  GDALClose(dem);
}