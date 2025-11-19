#include <gdal_priv.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

struct VertexProperty {
  double x;
  double y;
  double z;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

size_t vertexCount;
vector<VertexProperty> vertexData;
size_t faceCount;
vector<int> indices;

//颜色查找表
using F_RGB = std::array<double, 3>;
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
  string demPath = workDir + "/../Data/Model/dem.tif";

  GDALDataset* dem = (GDALDataset*)GDALOpen(demPath.c_str(), GA_ReadOnly);
  if (!dem) {
    cout << "Can't Open Image!" << endl;
    return;
  }

  int srcDemWidth = dem->GetRasterXSize();
  int srcDemHeight = dem->GetRasterYSize();

  //坐标信息
  double geoTransform[6] = {0};
  dem->GetGeoTransform(geoTransform);
  double srcDx = geoTransform[1];
  double srcDy = geoTransform[5];
  double startX = geoTransform[0] + 0.5 * srcDx;
  double startY = geoTransform[3] + 0.5 * srcDy;
  double endX = startX + (srcDemWidth - 1) * srcDx;
  double endY = startY + (srcDemHeight - 1) * srcDy;

  size_t demBufNum = (size_t)srcDemWidth * srcDemHeight;
  vector<float> srcDemBuf(demBufNum, 0);

  int depth = sizeof(float);
  dem->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, srcDemWidth, srcDemHeight,
                                  srcDemBuf.data(), srcDemWidth, srcDemHeight,
                                  GDT_Float32, depth, srcDemWidth * depth);

  GDALClose(dem);

  double minZ = *(std::min_element(srcDemBuf.begin(), srcDemBuf.end()));
  double maxZ = *(std::max_element(srcDemBuf.begin(), srcDemBuf.end()));

  vertexCount = (size_t)srcDemWidth * srcDemHeight;
  vertexData.resize(vertexCount);
  for (int yi = 0; yi < srcDemHeight; yi++) {
    for (int xi = 0; xi < srcDemWidth; xi++) {
      size_t m = (size_t)srcDemWidth * yi + xi;
      vertexData[m].x = startX + xi * srcDx;
      vertexData[m].y = startY + yi * srcDy;
      vertexData[m].z = srcDemBuf[m];

      int index = GetColorIndex(srcDemBuf[m], minZ, maxZ);
      vertexData[m].red = (uint8_t)(tableRGB[index][0] + 0.5);
      vertexData[m].green = (uint8_t)(tableRGB[index][1] + 0.5);
      vertexData[m].blue = (uint8_t)(tableRGB[index][2] + 0.5);
    }
  }

  faceCount = (size_t)(srcDemHeight - 1) * (srcDemWidth - 1) * 2;
  // indices.resize(faceCount);
  for (int yi = 0; yi < srcDemHeight - 1; yi++) {
    for (int xi = 0; xi < srcDemWidth - 1; xi++) {
      size_t m = (size_t)srcDemWidth * yi + xi;
      indices.push_back(m);
      indices.push_back(m + srcDemWidth);
      indices.push_back(m + srcDemWidth + 1);

      indices.push_back(m + srcDemWidth + 1);
      indices.push_back(m + 1);
      indices.push_back(m);
    }
  }
}

void WriteDemModel() {
  string workDir = getenv("GISBasic");
  string demPath = workDir + "/../Data/Model/dst.ply";

  ofstream outfile(demPath);
  if (!outfile) {
    printf("write file error %s\n", demPath.c_str());
    return;
  }

  outfile << "ply\n";
  outfile << "format ascii 1.0\n";
  outfile << "comment CL generated\n";
  outfile << "element vertex " << to_string(vertexCount) << '\n';
  outfile << "property double x\n";
  outfile << "property double y\n";
  outfile << "property double z\n";
  outfile << "property uchar red\n";
  outfile << "property uchar green\n";
  outfile << "property uchar blue\n";
  outfile << "element face " << to_string(faceCount) << '\n';
  outfile << "property list uchar int vertex_indices\n";
  outfile << "end_header\n";

  outfile << fixed;
  for (int vi = 0; vi < vertexCount; vi++) {
    outfile << vertexData[vi].x << ' ';
    outfile << vertexData[vi].y << ' ';
    outfile << vertexData[vi].z << ' ';
    outfile << (int)vertexData[vi].red << ' ';
    outfile << (int)vertexData[vi].green << ' ';
    outfile << (int)vertexData[vi].blue << '\n';
  }

  for (size_t fi = 0; fi < faceCount; fi++) {
    outfile << 3;

    for (int ii = 0; ii < 3; ii++) {
      int id = indices[fi * 3 + ii];
      outfile << ' ' << id;
    }
    outfile << '\n';
  }
}

int main() {
  GDALAllRegister();  //注册格式

  //设置Proj数据
  std::string projDataPath = getenv("GISBasic");
  projDataPath += "/share/proj";
  CPLSetConfigOption("PROJ_LIB", projDataPath.c_str());

  InitColorTable();

  ReadDem();

  WriteDemModel();

  return 0;
}
