// Example1RasterBase.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <gdal_priv.h>

#include <iostream>

using namespace std;

int main() {
  GDALAllRegister();

  string srcFile = getenv("GISBasic");
  srcFile = srcFile + "/../Data/Raster/berry_ali_2011127_crop_geo.tif";

  GDALDataset* img = (GDALDataset*)GDALOpen(srcFile.c_str(), GA_ReadOnly);
  if (!img) {
    return 1;
  }

  int imgWidth = img->GetRasterXSize();   //图像宽度
  int imgHeight = img->GetRasterYSize();  //图像高度
  int bandNum = img->GetRasterCount();    //波段数
  int depth = GDALGetDataTypeSize(img->GetRasterBand(1)->GetRasterDataType()) /
              8;  //图像深度

  cout << "宽度：" << imgWidth << '\n';
  cout << "高度：" << imgHeight << '\n';
  cout << "波段数：" << bandNum << '\n';
  cout << "深度：" << depth << '\n';

  GDALClose(img);
  img = nullptr;
}