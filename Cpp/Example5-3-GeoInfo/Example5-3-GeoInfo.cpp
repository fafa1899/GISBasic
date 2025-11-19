// Example3GeoInfo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <gdal_priv.h>

#include <iostream>

using namespace std;

int main() {
  GDALAllRegister();  //注册格式

  //设置Proj数据
  string projDataPath = getenv("GISBasic");
  projDataPath += "/share/proj";
  CPLSetConfigOption("PROJ_LIB", projDataPath.c_str());

  string dstFile = getenv("GISBasic");
  dstFile = dstFile + "/../Data/Raster/dst.jpg";

  int imgWidth = 256;
  int imgHeight = 256;
  int bandNum = 3;

  //把数据保存到临时文件MEM
  GDALDriver *pDriverMEM = GetGDALDriverManager()->GetDriverByName("MEM");
  GDALDataset *pOutMEMDataset =
      pDriverMEM->Create("", imgWidth, imgHeight, bandNum, GDT_Byte, NULL);
  if (!pOutMEMDataset) {
    printf("Can't Write Image!");
    return false;
  }

  //空间参考
  OGRSpatialReference spatialReference;
  spatialReference.importFromEPSG(4326);  // wgs84地理坐标系
  char *pszWKT = nullptr;
  spatialReference.exportToWkt(&pszWKT);

  pOutMEMDataset->SetProjection(pszWKT);

  //坐标信息
  double padfTransform[6] = {
      114.0,      //左上角点坐标X
      0.000001,   // X方向的分辨率
      0,          //旋转系数，如果为0，就是标准的正北向图像
      34.0,       //左上角点坐标Y
      0,          //旋转系数，如果为0，就是标准的正北向图像
      -0.000001,  // Y方向的分辨率
  };

  pOutMEMDataset->SetGeoTransform(padfTransform);

  //以创建复制的方式，生成jpg文件
  // GDALDriver *pDriverPNG = GetGDALDriverManager()->GetDriverByName("PNG");
  GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName("JPEG");

  char **ppszOptions = NULL;
  ppszOptions =
      CSLSetNameValue(ppszOptions, "WORLDFILE", "YES");  //配置图像信息

  GDALDataset *dst = pDriver->CreateCopy(dstFile.c_str(), pOutMEMDataset, TRUE,
                                         ppszOptions, 0, 0);
  if (!dst) {
    printf("Can't Write Image!");
    return false;
  }

  dst->SetProjection(pszWKT);

  dst->SetGeoTransform(padfTransform);

  GDALClose(dst);
  dst = nullptr;

  GDALClose(pOutMEMDataset);
  pOutMEMDataset = nullptr;

  return 0;
}