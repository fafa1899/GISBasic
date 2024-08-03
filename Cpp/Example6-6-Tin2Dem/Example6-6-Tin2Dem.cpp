#include <gdal_priv.h>

#include <Eigen/Eigen>
#include <array>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;
using namespace Eigen;

using Triangle = std::array<Vector3d, 3>;
using Rect = std::array<double, 4>;

vector<Triangle> tinTriangles;
vector<Rect> triBoxs;

std::array<double, 4> bound;

double dstDx = 100;
double dstDy = 100;
double startX;
double startY;
int dstDemColumn;
int dstDemRow;
float dstNoDataValue = -32768;
vector<float> dstDemBuf;

// 判断点P是否在空间三角形内,在则取Z值
bool PointInTriangle3D(const Triangle& triangle, Vector3d& P) {
  auto v0p = P - triangle[0];
  auto v0v1 = triangle[1] - triangle[0];
  auto v0v2 = triangle[2] - triangle[0];

  double D = v0v1.x() * v0v2.y() - v0v1.y() * v0v2.x();
  if (D == 0.0) {
    return false;
  }

  double D1 = v0p.x() * v0v2.y() - v0p.y() * v0v2.x();
  double D2 = v0v1.x() * v0p.y() - v0v1.y() * v0p.x();

  double u = D1 / D;
  double v = D2 / D;
  
  if (u >= 0 && v >= 0 && u + v <= 1) {
    P.z() = v0v1.z() * u + v0v2.z() * v + triangle[0].z();
    return true;
  }

  return false;
}

//bool PointInRect(double x, double y, const Rect& rect) {
//  return (x >= rect[0]) && (x <= rect[2]) && (y >= rect[1]) && (y <= rect[3]);
//}

//根据空截断字符串
void ChopStringWithSpace(string line, vector<string>& substring) {
  std::stringstream linestream(line);
  string sub;

  while (linestream >> sub) {
    substring.push_back(sub);
  }
}

void ReadTin() {
  string workDir = getenv("GISBasic");
  string tinPath = workDir + "/../Data/Terrain/terrain.ply";

  ifstream infile(tinPath, ios::binary);
  if (!infile) {
    printf("Can't Read Terrain File!\n");
  }

  //检查是否是ply格式
  string line;
  getline(infile, line);
  if (line != "ply") {
    return;
  }

  size_t vertexCount = 0;
  size_t faceCount = 0;

  while (getline(infile, line)) {
    if (line == "end_header") {
      break;
    }

    vector<string> substring;
    ChopStringWithSpace(line, substring);

    if (substring.size() == 3 && substring[0] == "element") {
      if (substring[1] == "vertex") {
        vertexCount = stoul(substring[2]);
      }
      if (substring[1] == "face") {
        faceCount = stoul(substring[2]);
      }
    }
  }

  vector<double> vertexTmp(vertexCount * 3, 0);
  infile.read((char*)vertexTmp.data(), vertexCount * 3 * sizeof(double));

  bound = {DBL_MAX, DBL_MAX, -DBL_MAX, -DBL_MAX};
  for (int vi = 0; vi < vertexCount; vi++) {
    bound[0] = std::min(vertexTmp[vi * 3], bound[0]);
    bound[1] = std::min(vertexTmp[vi * 3 + 1], bound[1]);
    bound[2] = std::max(vertexTmp[vi * 3], bound[2]);
    bound[3] = std::max(vertexTmp[vi * 3 + 1], bound[3]);
  }

  int stepSize = 13;
  vector<char> indiceTmp(faceCount * stepSize, 0);
  infile.read(indiceTmp.data(), faceCount * stepSize);

  tinTriangles.resize(faceCount);
  triBoxs.resize(faceCount);
  for (size_t fi = 0; fi < faceCount; fi++) {
    size_t offset = fi * stepSize;
    offset = offset + sizeof(uint8_t);

    int ids[3];
    memcpy(ids, indiceTmp.data() + offset, sizeof(int) * 3);

    for (int i = 0; i < 3; i++) {
      tinTriangles[fi][i].x() = vertexTmp[ids[i] * 3];
      tinTriangles[fi][i].y() = vertexTmp[ids[i] * 3 + 1];
      tinTriangles[fi][i].z() = vertexTmp[ids[i] * 3 + 2];
    }

    triBoxs[fi][0] =
        std::min(std::min(tinTriangles[fi][0].x(), tinTriangles[fi][1].x()),
                 tinTriangles[fi][2].x());
    triBoxs[fi][1] =
        std::min(std::min(tinTriangles[fi][0].y(), tinTriangles[fi][1].y()),
                 tinTriangles[fi][2].y());
    triBoxs[fi][2] =
        std::max(std::max(tinTriangles[fi][0].x(), tinTriangles[fi][1].x()),
                 tinTriangles[fi][2].x());
    triBoxs[fi][3] =
        std::max(std::max(tinTriangles[fi][0].y(), tinTriangles[fi][1].y()),
                 tinTriangles[fi][2].y());
  }
}

void Convert() {
  startX = ceil(bound[0] / dstDx) * dstDx;
  startY = ceil(bound[1] / dstDy) * dstDy;
  double endX = floor(bound[2] / dstDx) * dstDx;
  double endY = floor(bound[3] / dstDy) * dstDy;

  dstDemColumn = (int)((endX - startX) / dstDx) + 1;
  dstDemRow = (int)((endY - startY) / dstDy) + 1;

  size_t dstDemBufSize = (size_t)dstDemColumn * dstDemRow;
  dstDemBuf.resize(dstDemBufSize, dstNoDataValue);

  vector<bool> flagMap(dstDemBufSize, false);

  //  for (int yi = 0; yi < dstDemRow; yi++) {
  //    // cout << float(yi + 1) / dstDemRow << '\t';
  //    for (int xi = 0; xi < dstDemColumn; xi++) {
  //      Vector3d P(startX + xi * dstDx, startY + yi * dstDy, 0);
  //      size_t m = (size_t)dstDemColumn * yi + xi;
  //
  //      for (int ti = 0; ti < tinTriangles.size(); ti++) {
  //        const auto& triangle = tinTriangles[ti];
  //        if (PointInRect(P.x(), P.y(), triBoxs[ti]) &&
  //            PointInTriangle3D(triangle, P)) {
  //          dstDemBuf[m] = P.z();
  //          break;
  //        }
  //      }
  //    }
  //  }

  for (int ti = 0; ti < tinTriangles.size(); ti++) {
    cout << float(ti + 1) / tinTriangles.size() << '\t';

    int left = std::max((int)floor((triBoxs[ti][0] - startX) / dstDx), 0);
    int bottom = std::max((int)floor((triBoxs[ti][1] - startY) / dstDy), 0);
    int right = std::min((int)ceil((triBoxs[ti][2] - startX) / dstDx),
                         dstDemColumn - 1);
    int top =
        std::min((int)ceil((triBoxs[ti][3] - startY) / dstDy), dstDemRow - 1);

    for (int yi = bottom; yi <= top; yi++) {
      for (int xi = left; xi <= right; xi++) {
        Vector3d P(startX + xi * dstDx, startY + yi * dstDy, 0);
        size_t m = (size_t)dstDemColumn * yi + xi;
        if (!flagMap[m] && PointInTriangle3D(tinTriangles[ti], P)) {
          dstDemBuf[m] = P.z();
          flagMap[m] = true;
          // break;
        }
      }
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
      startY + (dstDemRow - 0.5) * dstDy,  //左上角点坐标Y
      0,       //旋转系数，如果为0，就是标准的正北向图像
      -dstDy,  // Y方向的分辨率
  };
  dem->SetGeoTransform(padfTransform);

  dem->GetRasterBand(1)->SetNoDataValue(dstNoDataValue);

  size_t depth = sizeof(float);
  size_t dstDemBufOffset = (size_t)dstDemColumn * (dstDemRow - 1);
  dem->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, dstDemColumn, dstDemRow,
                                  dstDemBuf.data() + dstDemBufOffset,
                                  dstDemColumn, dstDemRow, GDT_Float32, depth,
                                  -dstDemColumn * depth);

  GDALClose(dem);
}

int main() {
  GDALAllRegister();  //注册格式

  ReadTin();

  Convert();

  WriteDem();

  return 0;
}