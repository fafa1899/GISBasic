// Example5DemResample.cpp: 定义应用程序的入口点。
//

#include <gdal_priv.h>

#include <iostream>

using namespace std;

double startX;
double startY;
double endX;
double endY;

int srcDemWidth;
int srcDemHeight;
double srcDx;
double srcDy;
vector<float> srcDemBuf;

double dstDx = 100;
double dstDy = -100;
int dstDemColumn;
int dstDemRow;
float dstNoDataValue = -32768;
vector<float> dstDemBuf;

void ReadDem() {
  string workDir = getenv("GISBasic");
  string demPath = workDir + "/../Data/Terrain/dem.tif";

  GDALDataset* dem = (GDALDataset*)GDALOpen(demPath.c_str(), GA_ReadOnly);
  if (!dem) {
    cout << "Can't Open Image!" << endl;
    return;
  }

  srcDemWidth = dem->GetRasterXSize();
  srcDemHeight = dem->GetRasterYSize();

  //坐标信息
  double geoTransform[6] = {0};
  dem->GetGeoTransform(geoTransform);
  srcDx = geoTransform[1];
  srcDy = geoTransform[5];
  startX = geoTransform[0] + 0.5 * srcDx;
  startY = geoTransform[3] + 0.5 * srcDy;
  endX = startX + (srcDemWidth - 1) * srcDx;
  endY = startY + (srcDemHeight - 1) * srcDy;

  size_t demBufNum = (size_t)srcDemWidth * srcDemHeight;
  srcDemBuf.resize(demBufNum, 0);

  int depth = sizeof(float);
  dem->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, srcDemWidth, srcDemHeight,
                                  srcDemBuf.data(), srcDemWidth, srcDemHeight,
                                  GDT_Float32, depth, srcDemWidth * depth);

  GDALClose(dem);
}

//双线性插值
void Bilinear(double lx, double ly, int xi, int yi) {
  int x0 = std::min(std::max((int)floor(lx), 0), srcDemWidth - 1);
  int y0 = std::min(std::max((int)floor(ly), 0), srcDemHeight - 1);
  int x1 = std::min(std::max(x0 + 1, 0), srcDemWidth - 1);
  int y1 = std::min(std::max(y0 + 1, 0), srcDemHeight - 1);

  double u = lx - x0;
  double v = ly - y0;

  size_t f00 = (size_t)srcDemWidth * y0 + x0;
  size_t f10 = (size_t)srcDemWidth * y0 + x1;
  size_t f01 = (size_t)srcDemWidth * y1 + x0;
  size_t f11 = (size_t)srcDemWidth * y1 + x1;

  double value = srcDemBuf[f00] * (1 - u) * (1 - v) +
                 srcDemBuf[f10] * u * (1 - v) + srcDemBuf[f01] * (1 - u) * v +
                 srcDemBuf[f11] * u * v;

  size_t m = (size_t)dstDemColumn * yi + xi;
  dstDemBuf[m] = (float)(value);
}

void ReSample() {
  dstDemColumn = (int)((endX - startX) / dstDx + 1);
  dstDemRow = (int)((endY - startY) / dstDy + 1);

  size_t demBufNum = (size_t)dstDemColumn * dstDemRow;
  dstDemBuf.resize(demBufNum, dstNoDataValue);

  for (int yi = 0; yi < dstDemRow; yi++) {
    for (int xi = 0; xi < dstDemColumn; xi++) {
      double lx = startX + dstDx * xi;
      double ly = startY + dstDy * yi;
      lx = (lx - startX) / srcDx;
      ly = (ly - startY) / srcDy;

      if (lx < 0 || lx > srcDemWidth - 1 || ly < 0 || ly > srcDemHeight - 1) {
        continue;
      }

      Bilinear(lx, ly, xi, yi);
    }
  }
}

void WriteDem() {
  string workDir = getenv("GISBasic");
  string demPath = workDir + "/../Data/Terrain/dst.tif";

  GDALDriver* pDriver =
      GetGDALDriverManager()->GetDriverByName("GTIFF");  //图像驱动
  char** ppszOptions = NULL;
  ppszOptions =
      CSLSetNameValue(ppszOptions, "BIGTIFF", "IF_NEEDED");  //配置图像信息
  GDALDataset* dem = pDriver->Create(demPath.c_str(), dstDemColumn, dstDemRow,
                                     1, GDT_Float32, ppszOptions);
  if (!dem) {
    printf("Can't Write Image!");
    return;
  }

  //坐标信息
  double padfTransform[6] = {
      startX - 0.5 * dstDx,  //左上角点坐标X
      dstDx,                 // X方向的分辨率
      0,  //旋转系数，如果为0，就是标准的正北向图像
      startY - 0.5 * dstDy,  //左上角点坐标Y
      0,      //旋转系数，如果为0，就是标准的正北向图像
      dstDy,  // Y方向的分辨率
  };
  dem->SetGeoTransform(padfTransform);

  dem->GetRasterBand(1)->SetNoDataValue(dstNoDataValue);

  size_t depth = sizeof(float);
  dem->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, dstDemColumn, dstDemRow,
                                  dstDemBuf.data(), dstDemColumn, dstDemRow,
                                  GDT_Float32, depth, dstDemColumn * depth);

  GDALClose(dem);
}

int main() {
  GDALAllRegister();  //注册格式

  ReadDem();

  ReSample();

  WriteDem();

  return 0;
}
