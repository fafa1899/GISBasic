// Example4ReadAndWriteRaster.cpp : 此文件包含 "main"
// 函数。程序执行将在此处开始并结束。
//

#include <gdal_priv.h>

#include <iostream>

using namespace std;

int main() {
  GDALAllRegister();  // GDAL所有操作都需要先注册格式
  CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");  //支持中文路径

  string baseDir = getenv("GISBasic");

  //打开数据集对象
  string srcFile = baseDir + "/../Data/Raster/image1.jpg";
  GDALDataset* img = (GDALDataset*)GDALOpen(srcFile.c_str(), GA_ReadOnly);
  if (!img) {
    cout << "Can't Open Image!" << endl;
    return 1;
  }

  int imgWidth = img->GetRasterXSize();   //图像宽度
  int imgHeight = img->GetRasterYSize();  //图像高度
  int bandNum = img->GetRasterCount();    //波段数
  int depth = GDALGetDataTypeSize(img->GetRasterBand(1)->GetRasterDataType()) /
              8;  //图像深度

  //创建数据集对象
  string dstFile = baseDir + "/../Data/Raster/dst.tif";
  GDALDriver* pDriver =
      GetGDALDriverManager()->GetDriverByName("GTIFF");  //图像驱动
  char** ppszOptions = NULL;
  ppszOptions =
      CSLSetNameValue(ppszOptions, "BIGTIFF", "IF_NEEDED");  //配置图像信息
  int bufWidth = 256;
  int bufHeight = 256;
  GDALDataset* dst = pDriver->Create(dstFile.c_str(), bufWidth, bufHeight,
                                     bandNum, GDT_Byte, ppszOptions);
  if (!dst) {
    printf("Can't Write Image!");
    return false;
  }

  //一般读写：
  //申请buf
  size_t imgBufNum = (size_t)bufWidth * bufHeight * bandNum;
  uint8_t* imgBuf = new uint8_t[imgBufNum];
  //读取
  img->RasterIO(GF_Read, 0, 0, bufWidth, bufHeight, imgBuf, bufWidth, bufHeight,
                GDT_Byte, bandNum, nullptr, bandNum * depth,
                bufWidth * bandNum * depth, depth);
  //写入
  dst->RasterIO(GF_Write, 0, 0, bufWidth, bufHeight, imgBuf, bufWidth,
                bufHeight, GDT_Byte, bandNum, nullptr, bandNum * depth,
                bufWidth * bandNum * depth, depth);
  //释放
  delete[] imgBuf;
  imgBuf = nullptr;

  ////读取特定波段
  ////波段索引
  // int panBandMap[3] = {3, 2, 1};
  ////申请buf
  // size_t imgBufNum = (size_t)bufWidth * bufHeight * bandNum;
  // uint8_t* imgBuf = new uint8_t[imgBufNum];
  ////读取
  // img->RasterIO(GF_Read, 0, 0, bufWidth, bufHeight, imgBuf, bufWidth,
  // bufHeight,
  //              GDT_Byte, bandNum, panBandMap, bandNum * depth,
  //              bufWidth * bandNum * depth, depth);
  ////写入
  // dst->RasterIO(GF_Write, 0, 0, bufWidth, bufHeight, imgBuf, bufWidth,
  //              bufHeight, GDT_Byte, bandNum, nullptr, bandNum * depth,
  //              bufWidth * bandNum * depth, depth);
  ////释放
  // delete[] imgBuf;
  // imgBuf = nullptr;

  ////左下角起点读写
  ////申请buf
  // size_t imgBufNum = (size_t)bufWidth * bufHeight * bandNum;
  // size_t imgBufOffset = (size_t)bufWidth * (bufHeight - 1) * bandNum;
  // uint8_t* imgBuf = new uint8_t[imgBufNum];
  ////读取
  // img->RasterIO(GF_Read, 0, 0, bufWidth, bufHeight, imgBuf + imgBufOffset,
  //              bufWidth, bufHeight, GDT_Byte, bandNum, nullptr,
  //              bandNum * depth, -bufWidth * bandNum * depth, depth);
  ////写入
  // dst->RasterIO(GF_Write, 0, 0, bufWidth, bufHeight, imgBuf + imgBufOffset,
  //              bufWidth, bufHeight, GDT_Byte, bandNum, nullptr,
  //              bandNum * depth, -bufWidth * bandNum * depth, depth);
  ////释放
  // delete[] imgBuf;
  // imgBuf = nullptr;

  ////重采样读写
  ////申请buf
  // size_t imgBufNum = (size_t)bufWidth * bufHeight * bandNum;
  // size_t imgBufOffset = (size_t)bufWidth * (bufHeight - 1) * bandNum;
  // uint8_t* imgBuf = new uint8_t[imgBufNum];
  ////读取
  // img->RasterIO(GF_Read, 0, 0, imgWidth, imgHeight, imgBuf + imgBufOffset,
  //              bufWidth, bufHeight, GDT_Byte, bandNum, nullptr,
  //              bandNum * depth, -bufWidth * bandNum * depth, depth);
  ////写入
  // dst->RasterIO(GF_Write, 0, 0, bufWidth, bufHeight, imgBuf + imgBufOffset,
  //              bufWidth, bufHeight, GDT_Byte, bandNum, nullptr,
  //              bandNum * depth, -bufWidth * bandNum * depth, depth);
  ////释放
  // delete[] imgBuf;
  // imgBuf = nullptr;

  GDALClose(img);
  GDALClose(dst);
}