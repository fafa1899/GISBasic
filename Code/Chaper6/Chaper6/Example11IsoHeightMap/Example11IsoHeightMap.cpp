#include <gdal_priv.h>

#include <array>
#include <iostream>
#include <vector>

using namespace std;

using F_RGB = std::array<double, 3>;

int demWidth;
int demHeight;

double geoTransform[6] = {0};
double startX;  //左上角点坐标X
double dx;      // X方向的分辨率
double startY;  //左上角点坐标Y
double dy;      // Y方向的分辨率

vector<float> demBuf;

int dstBandNum = 4;
vector<uint8_t> dstBuf;

double startHeight = 550;
double endHeight = 2815;
double heightInterval = 500;

vector<F_RGB> tableRGB(256);         //颜色映射表
vector<double> heightThresholdList;  //高度区间
vector<F_RGB> heightRGBList;         //高度区间对应的颜色

//生成渐变色
void Gradient(F_RGB &start, F_RGB &end, vector<F_RGB> &RGBList) {
  F_RGB d;
  for (int i = 0; i < 3; i++) {
    d[i] = (end[i] - start[i]) / RGBList.size();
  }

  for (size_t i = 0; i < RGBList.size(); i++) {
    for (int j = 0; j < 3; j++) {
      RGBList[i][j] = start[j] + d[j] * i;
    }
  }
}

//初始化颜色查找表
void InitColorTable() {
  F_RGB blue({17, 60, 235});   //蓝色
  F_RGB green({17, 235, 86});  //绿色
  vector<F_RGB> RGBList(60);
  Gradient(blue, green, RGBList);
  for (int i = 0; i < 60; i++) {
    tableRGB[i] = RGBList[i];
  }

  F_RGB yellow({235, 173, 17});  //黄色
  RGBList.clear();
  RGBList.resize(60);
  Gradient(green, yellow, RGBList);
  for (int i = 0; i < 60; i++) {
    tableRGB[i + 60] = RGBList[i];
  }

  F_RGB red({235, 60, 17});  //红色
  RGBList.clear();
  RGBList.resize(60);
  Gradient(yellow, red, RGBList);
  for (int i = 0; i < 60; i++) {
    tableRGB[i + 120] = RGBList[i];
  }

  F_RGB white({235, 17, 235});  //紫色
  RGBList.clear();
  RGBList.resize(76);
  Gradient(red, white, RGBList);
  for (int i = 0; i < 76; i++) {
    tableRGB[i + 180] = RGBList[i];
  }
}

void ReadDem() {
  string workDir = getenv("GISBasic");
  string demPath = workDir + "/../Data/Terrain/dem.tif";

  GDALDataset *dem = (GDALDataset *)GDALOpen(demPath.c_str(), GA_ReadOnly);
  if (!dem) {
    cout << "Can't Open Image!" << endl;
    return;
  }

  demWidth = dem->GetRasterXSize();
  demHeight = dem->GetRasterYSize();

  dem->GetGeoTransform(geoTransform);
  startX = geoTransform[0];  //左上角点坐标X
  dx = geoTransform[1];      // X方向的分辨率
  startY = geoTransform[3];  //左上角点坐标Y
  dy = geoTransform[5];      // Y方向的分辨率

  // noValue = dem->GetRasterBand(1)->GetNoDataValue();

  size_t demBufNum = (size_t)demWidth * demHeight;
  demBuf.resize(demBufNum, 0);

  int depth = sizeof(float);
  dem->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, demWidth, demHeight,
                                  demBuf.data(), demWidth, demHeight,
                                  GDT_Float32, depth, demWidth * depth);

  GDALClose(dem);
  dem = nullptr;
}

void HandleDem() {
  size_t dstBufNum = (size_t)demWidth * demHeight * dstBandNum;
  dstBuf.resize(dstBufNum, 255);

  for (size_t i = 0; i < heightThresholdList.size(); i++) {
    double heightThreshold = heightThresholdList[i];
    F_RGB thresholdRgb = heightRGBList[i];

    for (int yi = 0; yi < demHeight; yi++) {
      for (int xi = 0; xi < demWidth; xi++) {
        size_t m = (size_t)demWidth * yi + xi;

        if (demBuf[m] > heightThreshold) {
          size_t n = (size_t)demWidth * dstBandNum * yi + dstBandNum * xi;
          for (int bi = 0; bi < 3; bi++) {
            dstBuf[n + bi] = (uint8_t)thresholdRgb[bi];
          }
        }
      }
    }
  }
}

void WriteDst() {
  string workDir = getenv("GISBasic");
  string demPath = workDir + "/../Data/Terrain/dst.tif";

  GDALDriver *pDriver =
      GetGDALDriverManager()->GetDriverByName("GTIFF");  //图像驱动
  char **ppszOptions = NULL;
  ppszOptions =
      CSLSetNameValue(ppszOptions, "BIGTIFF", "IF_NEEDED");  //配置图像信息
  GDALDataset *dst = pDriver->Create(demPath.c_str(), demWidth, demHeight, 4,
                                     GDT_Byte, ppszOptions);
  if (!dst) {
    printf("Can't Write Image!");
    return;
  }

  dst->SetGeoTransform(geoTransform);

  int depth = sizeof(uint8_t);
  dst->RasterIO(GF_Write, 0, 0, demWidth, demHeight, dstBuf.data(), demWidth,
                demHeight, GDT_Byte, dstBandNum, nullptr, dstBandNum * depth,
                demWidth * dstBandNum * depth, depth);

  GDALClose(dst);
  dst = nullptr;
}

int main() {
  GDALAllRegister();  // GDAL所有操作都需要先注册格式

  ReadDem();

  InitColorTable();

  double heightThreshold = startHeight;
  while (heightThreshold < endHeight) {
    heightThresholdList.push_back(heightThreshold);
    heightThreshold = heightThreshold + heightInterval;
  }

  if (heightThresholdList.size() == 1) {
    heightRGBList.push_back(tableRGB[0]);
  } else {
    size_t step = tableRGB.size() / (heightThresholdList.size() - 1);
    size_t index = 0;
    for (size_t i = 0; i < heightThresholdList.size() - 1; i++) {
      heightRGBList.push_back(tableRGB[index]);
      index = index + step;
    }
    heightRGBList.push_back(tableRGB[tableRGB.size() - 1]);
  }

  HandleDem();

  WriteDst();

  return 0;
}