#include <gdal_priv.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

using namespace std;

using F_RGB = std::array<double, 3>;

const double pi = 3.14159265358979323846;
const double d2r = pi / 180;
const double r2d = 180 / pi;

int demWidth;
int demHeight;
int dstBandNum = 4;

double geoTransform[6] = {0};
double startX;  //左上角点坐标X
double dx;      // X方向的分辨率
double startY;  //左上角点坐标Y
double dy;      // Y方向的分辨率

double noValue;

vector<float> demBuf;
vector<uint8_t> dstBuf;

//颜色查找表
vector<F_RGB> tableRGB(256);

//生成渐变色
void Gradient(F_RGB& start, F_RGB& end, vector<F_RGB>& RGBList) {
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

//根据高程选颜色
inline int GetColorIndex(double z, double min_z, double max_z) {
  int temp = (int)floor((z - min_z) * 255 / (max_z - min_z) + 0.6);
  return temp;
}

void ReadDem() {
  string workDir = getenv("GISBasic");
  string demPath = workDir + "/../Data/Terrain/dem.tif";

  GDALDataset* dem = (GDALDataset*)GDALOpen(demPath.c_str(), GA_ReadOnly);
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

  noValue = dem->GetRasterBand(1)->GetNoDataValue();

  size_t demBufNum = (size_t)demWidth * demHeight;
  demBuf.resize(demBufNum, 0);

  int depth = sizeof(float);
  dem->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, demWidth, demHeight,
                                  demBuf.data(), demWidth, demHeight,
                                  GDT_Float32, depth, demWidth * depth);

  GDALClose(dem);
  dem = nullptr;
}

// a b c
// d e f
// g h i
double CalHillshade(float* tmpBuf, double Zenith_rad, double Azimuth_rad,
                    double dx, double dy, double z_factor) {
  double dzdx = ((tmpBuf[2] + 2 * tmpBuf[5] + tmpBuf[8]) -
                 (tmpBuf[0] + 2 * tmpBuf[3] + tmpBuf[6])) /
                (8 * dx);
  double dzdy = ((tmpBuf[6] + 2 * tmpBuf[7] + tmpBuf[8]) -
                 (tmpBuf[0] + 2 * tmpBuf[1] + tmpBuf[2])) /
                (8 * dy);

  double Slope_rad = atan(z_factor * sqrt(dzdx * dzdx + dzdy * dzdy));
  double Aspect_rad = 0;
  if (abs(dzdx) > 1e-9) {
    Aspect_rad = atan2(dzdy, -dzdx);
    if (Aspect_rad < 0) {
      Aspect_rad = 2 * pi + Aspect_rad;
    }
  } else {
    if (dzdy > 0) {
      Aspect_rad = pi / 2;
    } else if (dzdy < 0) {
      Aspect_rad = 2 * pi - pi / 2;
    } else {
      Aspect_rad = Aspect_rad;
    }
  }

  double Hillshade =
      255.0 *
      ((cos(Zenith_rad) * cos(Slope_rad)) +
       (sin(Zenith_rad) * sin(Slope_rad) * cos(Azimuth_rad - Aspect_rad)));
  return Hillshade;
}

void Hillshade() {
  //设置方向：平行光
  double solarAltitude = 45.0;
  double solarAzimuth = 315.0;

  //
  double Zenith_rad = (90 - solarAltitude) * d2r;
  double Azimuth_math = 360.0 - solarAzimuth + 90;
  if (Azimuth_math >= 360.0) {
    Azimuth_math = Azimuth_math - 360.0;
  }
  double Azimuth_rad = (Azimuth_math)*d2r;

  size_t dstBufNum = (size_t)demWidth * demHeight * dstBandNum;
  dstBuf.resize(dstBufNum, 0);

  double z_factor = 1;
  double alpha = 0.3;  // A不透明度 α*A+(1-α)*B
  double minZ = *(std::min_element(demBuf.begin(), demBuf.end()));
  double maxZ = *(std::max_element(demBuf.begin(), demBuf.end()));

  // a b c
  // d e f
  // g h i
  for (int yi = 1; yi < demHeight - 1; yi++) {
    for (int xi = 1; xi < demWidth - 1; xi++) {
      size_t e = (size_t)demWidth * yi + xi;
      size_t f = e + 1;
      size_t d = e - 1;

      size_t b = e - demWidth;
      size_t c = b + 1;
      size_t a = b - 1;

      size_t h = e + demWidth;
      size_t i = h + 1;
      size_t g = h - 1;

      float tmpBuf[9] = {demBuf[a], demBuf[b], demBuf[c], demBuf[d], demBuf[e],
                         demBuf[f], demBuf[g], demBuf[h], demBuf[i]};
      double Hillshade =
          CalHillshade(tmpBuf, Zenith_rad, Azimuth_rad, dx, -dy, z_factor);

      size_t m = (size_t)demWidth * dstBandNum * yi + dstBandNum * xi;

      int index = GetColorIndex(demBuf[e], minZ, maxZ);

      for (int bi = 0; bi < 3; bi++) {
        double v = Hillshade * alpha + (1 - alpha) * (GByte)tableRGB[index][bi];
        dstBuf[m + bi] = (GByte)(std::min)((std::max)(v, 0.0), 255.0);
      }
      dstBuf[m + 3] = 255;
    }
  }
}

void WriteDst() {
  string workDir = getenv("GISBasic");
  string demPath = workDir + "/../Data/Terrain/dst.tif";

  GDALDriver* pDriver =
      GetGDALDriverManager()->GetDriverByName("GTIFF");  //图像驱动
  char** ppszOptions = NULL;
  ppszOptions =
      CSLSetNameValue(ppszOptions, "BIGTIFF", "IF_NEEDED");  //配置图像信息
  GDALDataset* dst = pDriver->Create(demPath.c_str(), demWidth, demHeight, 4,
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

  //设置Proj数据
  std::string projDataPath = getenv("GISBasic");
  projDataPath += "/share/proj";
  CPLSetConfigOption("PROJ_LIB", projDataPath.c_str());

  InitColorTable();

  ReadDem();

  Hillshade();

  WriteDst();

  return 0;
}
