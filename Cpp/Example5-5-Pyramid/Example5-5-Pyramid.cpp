// Example5Pyramid.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <gdal_priv.h>
#include <iostream>

using namespace std;

int main() {
  GDALAllRegister();  // GDAL所有操作都需要先注册格式

  //打开数据集对象
  string baseDir = getenv("GISBasic");
  string srcFilePath =
      baseDir + "/../Data/Raster/berry_ali_2011127_crop_geo.tif";

  GDALDataset* img = (GDALDataset*)GDALOpen(srcFilePath.c_str(), GA_ReadOnly);
  if (!img) {
    cout << "Can't Open Image!" << endl;
    return 1;
  }

  const char* pszResampling = "nearest";
  int nWidth = img->GetRasterXSize();
  int nHeight = img->GetRasterYSize();
  int nPixNumMax = nWidth * nHeight;
  int nPixNumMin = 128 * 128;
  int nPixNumCur = nPixNumMax;

  int LevelArray[1024] = {0};
  int nLevelCount = 0;

  do {
    LevelArray[nLevelCount] = static_cast<int>(pow(2.0, nLevelCount + 1));
    nPixNumCur = static_cast<int>(nPixNumCur / 4);
    nLevelCount++;
  } while (nPixNumCur > nPixNumMin && nLevelCount < 1024);

  if (nLevelCount > 0) {
    img->BuildOverviews(pszResampling, nLevelCount, LevelArray, 0, nullptr,
                        nullptr, nullptr);
  }

  GDALClose(img);

  return 0;
}
