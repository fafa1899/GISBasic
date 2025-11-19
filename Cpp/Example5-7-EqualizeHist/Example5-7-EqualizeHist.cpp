// Example7EqualizeHist.cpp : 此文件包含 "main"
// 函数。程序执行将在此处开始并结束。
//

#include <gdal_priv.h>

#include <algorithm>
#include <iostream>

using namespace std;

//直方图均衡化
void GetHistAvgLut(GUIntBig* anHistogram, int HistNum, vector<uint8_t>& lut) {
  //统计像素总的个数
  size_t sum = 0;
  for (int ci = 0; ci < HistNum; ci++) {
    sum = sum + anHistogram[ci];
  }

  //
  vector<double> funProbability(HistNum, 0.0);              //概率密度函数
  vector<double> funProbabilityDistribution(HistNum, 0.0);  //概率分布函数

  //计算概率分布函数
  double dsum = (double)sum;
  double accumulation = 0;
  for (int ci = 0; ci < HistNum; ci++) {
    funProbability[ci] = anHistogram[ci] / dsum;
    accumulation = accumulation + funProbability[ci];
    funProbabilityDistribution[ci] = accumulation;
  }

  //归一化的值扩展为0~255的像素值，存到颜色映射表
  lut.resize(HistNum, 0);
  for (int ci = 0; ci < HistNum; ci++) {
    double value = std::min<double>(
        std::max<double>(255 * funProbabilityDistribution[ci], 0), 255);
    lut[ci] = (unsigned char)value;
  }
}

//计算16位的颜色映射表
bool CalImgLut(GDALDataset* img, vector<vector<uint8_t>>& lut) {
  int bandNum = img->GetRasterCount();  //波段数
  lut.resize(bandNum);

  //
  for (int ib = 0; ib < bandNum; ib++) {
    //计算该通道的直方图
    int HistNum = 256;
    GUIntBig* anHistogram = new GUIntBig[HistNum];
    int bApproxOK = FALSE;
    img->GetRasterBand(ib + 1)->GetHistogram(-0.5, 255.5, HistNum, anHistogram,
                                             TRUE, bApproxOK, NULL, NULL);

    //直方图均衡化
    GetHistAvgLut(anHistogram, HistNum, lut[ib]);

    //
    delete[] anHistogram;
    anHistogram = nullptr;
  }

  return true;
}

int main() {          //
  GDALAllRegister();  // GDAL所有操作都需要先注册格式
  CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");  //支持中文路径

  string envDir = getenv("GISBasic");
  string imgPath = envDir + "/../Data/Raster/image1.jpg";

  //读取
  GDALDataset* img = (GDALDataset*)GDALOpen(imgPath.c_str(), GA_ReadOnly);
  if (!img) {
    cout << "Can't Open Image!" << endl;
    return 1;
  }

  //
  int imgWidth = img->GetRasterXSize();   //图像宽度
  int imgHeight = img->GetRasterYSize();  //图像高度
  int bandNum = img->GetRasterCount();    //波段数
  int depth = GDALGetDataTypeSize(img->GetRasterBand(1)->GetRasterDataType()) /
              8;  //图像深度

  //创建颜色映射表
  vector<vector<uint8_t>> lut;
  CalImgLut(img, lut);

  //创建
  GDALDriver* pDriver =
      GetGDALDriverManager()->GetDriverByName("BMP");  //图像驱动
  char** ppszOptions = NULL;
  string dstPath = envDir + "/../Data/Raster/dst.bmp";
  int bufWidth = imgWidth;
  int bufHeight = imgHeight;
  GDALDataset* dst = pDriver->Create(dstPath.c_str(), bufWidth, bufHeight,
                                     bandNum, GDT_Byte, ppszOptions);
  if (!dst) {
    printf("Can't Write Image!");
    return false;
  }

  //读取buf
  size_t imgBufNum = (size_t)bufWidth * bufHeight * bandNum * depth;
  GByte* imgBuf = new GByte[imgBufNum];
  img->RasterIO(GF_Read, 0, 0, bufWidth, bufHeight, imgBuf, bufWidth, bufHeight,
                GDT_Byte, bandNum, nullptr, bandNum * depth,
                bufWidth * bandNum * depth, depth);

  //迭代通过颜色映射表替换值
  for (int yi = 0; yi < bufHeight; yi++) {
    for (int xi = 0; xi < bufWidth; xi++) {
      for (int bi = 0; bi < bandNum; bi++) {
        size_t m = (size_t)bufWidth * bandNum * yi + bandNum * xi + bi;
        imgBuf[m] = lut[bi][imgBuf[m]];
      }
    }
  }

  //写入
  dst->RasterIO(GF_Write, 0, 0, bufWidth, bufHeight, imgBuf, bufWidth,
                bufHeight, GDT_Byte, bandNum, nullptr, bandNum * depth,
                bufWidth * bandNum * depth, depth);

  //释放
  delete[] imgBuf;
  imgBuf = nullptr;
  GDALClose(dst);
  dst = nullptr;
  GDALClose(img);
  img = nullptr;

  return 0;
}