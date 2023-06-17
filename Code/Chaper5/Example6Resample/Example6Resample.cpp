// Example6Resample.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <gdal_priv.h>

#include <Eigen/Eigen>
#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

using namespace std;
using namespace Eigen;

//插值方式，改变此值以使用不同的插值方式重采样
int interpolationMethod = 1;

OGRSpatialReference srcSpatialReference;
GDALDataset* srcImage = nullptr;
int srcWidth = 0;
int srcHeight = 0;
double srcStartX = 0;
double srcStartY = 0;
vector<GByte> srcImgBuf;

array<double, 4> dstBound;

OGRSpatialReference dstSpatialReference;
GDALDataset* dstImage = nullptr;
int dstWidth = 0;
int dstHeight = 0;
double dstStartX = 0;
double dstStartY = 0;
vector<GByte> dstImgBuf;

int bandNum;
double dx = 0;
double dy = 0;
int depth = sizeof(GByte);

void GetDstImageBound() {
  double geoTransform[6];
  srcImage->GetGeoTransform(geoTransform);

  srcStartX = geoTransform[0];
  srcStartY = geoTransform[3];
  dx = geoTransform[1];
  dy = geoTransform[5];
  double endX = srcStartX + dx * srcWidth;
  double endY = srcStartY + dy * srcHeight;

  double geoBoundX[4] = {srcStartX, endX, endX, srcStartX};
  double geoBoundY[4] = {srcStartY, srcStartY, endY, endY};

  srcSpatialReference.importFromWkt(srcImage->GetProjectionRef());
  dstSpatialReference.importFromEPSG(3857);

  OGRCoordinateTransformation* src2Dst = OGRCreateCoordinateTransformation(
      &srcSpatialReference, &dstSpatialReference);

  src2Dst->Transform(4, geoBoundX, geoBoundY);

  auto xMinMax = std::minmax_element(geoBoundX, geoBoundX + 4);
  auto yMinMax = std::minmax_element(geoBoundY, geoBoundY + 4);
  dstBound = {*xMinMax.first, *yMinMax.first, *xMinMax.second, *yMinMax.second};

  OGRCoordinateTransformation::DestroyCT(src2Dst);
  src2Dst = nullptr;
}

void ReadImage() {
  string imgPath = getenv("GISBasic");
  imgPath = imgPath + "/../Data/Raster/berry_ali_2011127_crop_geo.tif";

  srcImage = (GDALDataset*)GDALOpen(imgPath.c_str(), GA_ReadOnly);
  if (!srcImage) {
    cout << "Can't Open Image!" << endl;
    return;
  }
  srcWidth = srcImage->GetRasterXSize();   //图像宽度
  srcHeight = srcImage->GetRasterYSize();  //图像高度
  bandNum = srcImage->GetRasterCount();    //波段数

  GetDstImageBound();

  //申请buf
  size_t srcImgBufNum = (size_t)srcWidth * srcHeight * bandNum;
  srcImgBuf.resize(srcImgBufNum, 0);

  //读取
  srcImage->RasterIO(GF_Read, 0, 0, srcWidth, srcHeight, srcImgBuf.data(),
                     srcWidth, srcHeight, GDT_Byte, bandNum, nullptr,
                     bandNum * depth, srcWidth * bandNum * depth, depth);

  GDALClose(srcImage);
}

void WriteImage() {
  GDALDriver* pDriver =
      GetGDALDriverManager()->GetDriverByName("GTIFF");  //图像驱动
  char** ppszOptions = NULL;
  ppszOptions =
      CSLSetNameValue(ppszOptions, "BIGTIFF", "IF_NEEDED");  //配置图像信息

  string imgPath = getenv("GISBasic");
  imgPath = imgPath + "/../Data/Raster/dst.tif";
  dstImage = pDriver->Create(imgPath.c_str(), dstWidth, dstHeight, bandNum,
                             GDT_Byte, ppszOptions);
  if (!dstImage) {
    printf("Can't Write Image!");
    return;
  }

  double geoTransform[6] = {dstStartX, dx, 0, dstStartY, 0, dy};
  dstImage->SetGeoTransform(geoTransform);

  char* pszWKT = nullptr;
  dstSpatialReference.exportToWkt(&pszWKT);
  dstImage->SetProjection(pszWKT);
  CPLFree(pszWKT);
  pszWKT = nullptr;

  //写入
  dstImage->RasterIO(GF_Write, 0, 0, dstWidth, dstHeight, dstImgBuf.data(),
                     dstWidth, dstHeight, GDT_Byte, bandNum, nullptr,
                     bandNum * depth, dstWidth * bandNum * depth, depth);

  GDALClose(dstImage);
}

double BicubicFunction(double x) {
  double y;
  double a = -0.5;
  if (fabs(x) >= 0 && fabs(x) <= 1) {
    y = 1 - (a + 3) * x * x + (a + 2) * (fabs(x) * fabs(x) * fabs(x));
  } else if (fabs(x) > 1 && fabs(x) <= 2) {
    y = -4 * a + 8 * a * fabs(x) - 5 * a * x * x +
        a * (fabs(x) * fabs(x) * fabs(x));
  } else {
    y = 0;
  }
  return y;
}

//最邻近插值
void Nearest(double lx, double ly, int xi, int yi) {
  int x = (int)round(lx);
  x = std::min(std::max(x, 0), srcWidth - 1);
  int y = (int)round(ly);
  y = std::min(std::max(y, 0), srcHeight - 1);

  for (int bi = 0; bi < bandNum; bi++) {
    size_t m = (size_t)dstWidth * bandNum * yi + bandNum * xi + bi;
    size_t n = (size_t)srcWidth * bandNum * y + bandNum * x + bi;
    dstImgBuf[m] = srcImgBuf[n];
  }
}

//双线性插值
void Bilinear(double lx, double ly, int xi, int yi) {
  int x0 = std::min(std::max((int)floor(lx), 0), srcWidth - 1);
  int y0 = std::min(std::max((int)floor(ly), 0), srcHeight - 1);
  int x1 = std::min(std::max(x0 + 1, 0), srcWidth - 1);
  int y1 = std::min(std::max(y0 + 1, 0), srcHeight - 1);

  double u = lx - x0;
  double v = ly - y0;

  for (int bi = 0; bi < bandNum; bi++) {
    size_t f00 = (size_t)srcWidth * bandNum * y0 + bandNum * x0 + bi;
    size_t f10 = (size_t)srcWidth * bandNum * y0 + bandNum * x1 + bi;
    size_t f01 = (size_t)srcWidth * bandNum * y1 + bandNum * x0 + bi;
    size_t f11 = (size_t)srcWidth * bandNum * y1 + bandNum * x1 + bi;

    double value = srcImgBuf[f00] * (1 - u) * (1 - v) +
                   srcImgBuf[f10] * u * (1 - v) + srcImgBuf[f01] * (1 - u) * v +
                   srcImgBuf[f11] * u * v;
    value = std::min(std::max(value, 0.0), 255.0);

    size_t m = (size_t)dstWidth * bandNum * yi + bandNum * xi + bi;
    dstImgBuf[m] = (GByte)(value);
  }
}

//三次卷积插值
void Bicubic(double lx, double ly, int xi, int yi) {
  int x0 = (int)floor(lx);
  int y0 = (int)floor(ly);
  double u = lx - x0;
  double v = ly - y0;

  int x[4] = {std::min(std::max(x0 - 1, 0), srcWidth - 1),
              std::min(std::max(x0, 0), srcWidth - 1),
              std::min(std::max(x0 + 1, 0), srcWidth - 1),
              std::min(std::max(x0 + 2, 0), srcWidth - 1)};
  int y[4] = {std::min(std::max(y0 - 1, 0), srcHeight - 1),
              std::min(std::max(y0, 0), srcHeight - 1),
              std::min(std::max(y0 + 1, 0), srcHeight - 1),
              std::min(std::max(y0 + 2, 0), srcHeight - 1)};
  size_t f[16] = {0};
  for (int ri = 0; ri < 4; ri++) {
    for (int ci = 0; ci < 4; ci++) {
      f[ri * 4 + ci] = (size_t)srcWidth * bandNum * y[ri] + bandNum * x[ci];
    }
  }

  Vector4d A(BicubicFunction(1 + u), BicubicFunction(u), BicubicFunction(1 - u),
             BicubicFunction(2 - u));
  Vector4d C(BicubicFunction(1 + v), BicubicFunction(v), BicubicFunction(1 - v),
             BicubicFunction(2 - v));

  for (int bi = 0; bi < bandNum; bi++) {
    Eigen::Matrix4d B;
    B << srcImgBuf[f[0] + bi], srcImgBuf[f[1] + bi], srcImgBuf[f[2] + bi],
        srcImgBuf[f[3] + bi], srcImgBuf[f[4] + bi], srcImgBuf[f[5] + bi],
        srcImgBuf[f[6] + bi], srcImgBuf[f[7] + bi], srcImgBuf[f[8] + bi],
        srcImgBuf[f[9] + bi], srcImgBuf[f[10] + bi], srcImgBuf[f[11] + bi],
        srcImgBuf[f[12] + bi], srcImgBuf[f[13] + bi], srcImgBuf[f[14] + bi],
        srcImgBuf[f[15] + bi];

    double value = round(A.dot(B * C));
    value = std::min(std::max(value, 0.0), 255.0);

    size_t m = (size_t)dstWidth * bandNum * yi + bandNum * xi + bi;
    dstImgBuf[m] = (GByte)(value);
  }
}

void ReSample() {
  dstStartX = floor(dstBound[0] / dx) * dx;
  dstStartY = ceil(dstBound[3] / dy) * dy;
  double endX = ceil(dstBound[2] / dx) * dx;
  double endY = floor(dstBound[1] / dy) * dy;
  dstWidth = (int)((endX - dstStartX) / dx);
  dstHeight = (int)((endY - dstStartY) / dy);

  //申请buf
  size_t dstImgBufNum = (size_t)dstWidth * dstHeight * bandNum;
  dstImgBuf.resize(dstImgBufNum, 0);

  OGRCoordinateTransformation* dst2Src = OGRCreateCoordinateTransformation(
      &dstSpatialReference, &srcSpatialReference);

  for (int yi = 0; yi < dstHeight; yi++) {
    for (int xi = 0; xi < dstWidth; xi++) {
      double lx = dstStartX + dx * xi + 0.5 * dx;
      double ly = dstStartY + dy * yi + 0.5 * dy;
      dst2Src->Transform(1, &lx, &ly);
      lx = (lx - (srcStartX + 0.5 * dx)) / dx;
      ly = (ly - (srcStartY + 0.5 * dy)) / dy;

      if (lx < 0 || lx > srcWidth - 1 || ly < 0 || ly > srcHeight - 1) {
        continue;
      }

      switch (interpolationMethod) {
        case 0: {
          Nearest(lx, ly, xi, yi);
          break;
        }
        case 1:
        default: {
          Bilinear(lx, ly, xi, yi);
          break;
        }
        case 2: {
          Bicubic(lx, ly, xi, yi);
          break;
        }
      }
    }
  }

  OGRCoordinateTransformation::DestroyCT(dst2Src);
  dst2Src = nullptr;
}

int main() {
  GDALAllRegister();  //注册格式

  ReadImage();

  ReSample();

  WriteImage();
}