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
  double texCoordX;
  double texCoordY;
};

size_t vertexCount;
vector<VertexProperty> vertexData;
size_t faceCount;
vector<int> indices;

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
      vertexData[m].texCoordX = (double)xi / (srcDemWidth - 1);
      vertexData[m].texCoordY = (double)yi / (srcDemHeight - 1);
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
  string demPath = workDir + "/../Data/Model/dst.obj";

  ofstream outfile(demPath);
  if (!outfile) {
    printf("write file error %s\n", demPath.c_str());
    return;
  }

  outfile << "mtllib dst.mtl\n";
  outfile << fixed;
  for (int vi = 0; vi < vertexCount; vi++) {
    outfile << "v" << ' ';
    outfile << vertexData[vi].x << ' ';
    outfile << vertexData[vi].y << ' ';
    outfile << vertexData[vi].z << '\n';
  }

  for (int vi = 0; vi < vertexCount; vi++) {
    outfile << "vt" << ' ';
    outfile << vertexData[vi].texCoordX << ' ';
    outfile << vertexData[vi].texCoordY << '\n';
  }

  outfile << "usemtl dst\n";
  for (size_t fi = 0; fi < faceCount; fi++) {
    outfile << "f";

    for (int ii = 0; ii < 3; ii++) {
      int id = indices[fi * 3 + ii] + 1;
      outfile << ' ' << id << '/' << id;
    }
    outfile << '\n';
  }

  string mtlPath = workDir + "/../Data/Model/dst.mtl";
  ofstream mtlfile(mtlPath);
  if (!mtlfile) {
    printf("write file error %s\n", mtlPath.c_str());
    return;
  }

  mtlfile << "newmtl dst\n";
  mtlfile << "illum 2\n";
  mtlfile << "map_Ka tex.jpg\n";
  mtlfile << "map_Kd tex.jpg\n";
  mtlfile << "map_Ks tex.jpg\n";
  mtlfile << "Ns 10.000\n";
}

int main() {
  GDALAllRegister();  //注册格式

  //设置Proj数据
  std::string projDataPath = getenv("GISBasic");
  projDataPath += "/share/proj";
  CPLSetConfigOption("PROJ_LIB", projDataPath.c_str());

  ReadDem();

  WriteDemModel();

  return 0;
}
