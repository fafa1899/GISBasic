// Example2CreateRaster.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <gdal_priv.h>

#include <iostream>

using namespace std;

int main() {
  GDALAllRegister();  //注册格式

  string dstFile = getenv("GISBasic");
  dstFile = dstFile + "/../Data/Raster/dst.tif";

  GDALDriver* pDriver =
      GetGDALDriverManager()->GetDriverByName("GTiff");  //图像驱动
  char** ppszOptions = NULL;
  ppszOptions =
      CSLSetNameValue(ppszOptions, "BIGTIFF", "IF_NEEDED");  //配置图像信息

  int imgWidth = 256;
  int imgHeight = 256;
  int bandNum = 3;
  GDALDataset* dst = pDriver->Create(dstFile.c_str(), imgWidth, imgHeight,
                                     bandNum, GDT_Byte, ppszOptions);
  if (!dst) {
    printf("Can't Write Image!");
    return 1;
  }

  GDALClose(dst);
  return 0;
}