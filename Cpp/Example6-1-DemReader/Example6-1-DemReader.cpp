// Example1DemReader.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <gdal_priv.h>

#include <iostream>

using namespace std;

int main() {
  GDALAllRegister();  //注册格式

  //设置Proj数据
  std::string projDataPath = getenv("GISBasic");
  projDataPath += "/share/proj";
  CPLSetConfigOption("PROJ_LIB", projDataPath.c_str());

  string workDir = getenv("GISBasic");
  string demPath = workDir + "/../Data/Terrain/dem.tif";

  GDALDataset* dem = (GDALDataset*)GDALOpen(demPath.c_str(), GA_ReadOnly);
  if (!dem) {
    cout << "Can't Open Image!" << endl;
    return 1;
  }

  int demWidth = dem->GetRasterXSize();
  int demHeight = dem->GetRasterYSize();
  int bandNum = dem->GetRasterCount();

  cout << "DEM列数：" << demWidth << endl;
  cout << "DEM行数：" << demHeight << endl;
  cout << "数据集波段数：" << bandNum << endl;

  const char* stringDataType =
      GDALGetDataTypeName(dem->GetRasterBand(1)->GetRasterDataType());
  cout << "DEM数据类型：" << stringDataType << endl;

  int success = 0;
  cout << "DEM无效值：" << dem->GetRasterBand(1)->GetNoDataValue(&success)
       << endl;

  cout << "DEM空间参考坐标系：\n";
  const char* wktString = dem->GetProjectionRef();
  cout << wktString << endl;

  //坐标信息
  double geoTransform[6] = {0};
  dem->GetGeoTransform(geoTransform);
  double dx = geoTransform[1];
  double dy = geoTransform[5];
  double startX = geoTransform[0] + 0.5 * dx;
  double startY = geoTransform[3] + 0.5 * dy;

  cout << "DEM间距：" << dx << '\t' << dy << '\n';
  cout << fixed << "DEM左上角起点位置：" << startX << '\t' << startY << endl;

  cout << "读取DEM高程..." << endl;
  size_t demBufNum = (size_t)demWidth * demHeight;
  vector<float> demBuf(demBufNum, 0);

  int depth = sizeof(float);
  dem->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, demWidth, demHeight,
                                  demBuf.data(), demWidth, demHeight,
                                  GDT_Float32, depth, demWidth * depth);
  cout << "完成" << endl;

  GDALClose(dem);
}
