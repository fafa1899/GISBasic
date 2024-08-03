#include <gdal_priv.h>
#include <ogrsf_frmts.h>

#include <Eigen/Eigen>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;
using namespace Eigen;

struct TrigonVertexIndex {
  size_t index[3];
};

double startHeight = 550;
double endHeight = 2815;
double heightInterval = 500;

size_t nV;                                       //点的个数
std::vector<Vector3d> vertexXyz;                 //点集
size_t nF;                                       //面的个数.
std::vector<TrigonVertexIndex> faceVertexIndex;  //面在点集中的序号

//根据空截断字符串
void ChopStringWithSpace(string line, vector<string>& substring) {
  std::stringstream linestream(line);
  string sub;

  while (linestream >> sub) {
    substring.push_back(sub);
  }
}

bool ReadTin(const char* szModelPath) {
  ifstream infile(szModelPath, ios::binary);
  if (!infile) {
    printf("Can't Load %s\n", szModelPath);
    return false;
  }

  string line;
  while (line != string("end_header")) {
    getline(infile, line);
    vector<string> substring;
    ChopStringWithSpace(line, substring);

    if (substring.size() == 3 && substring[0] == "element") {
      if (substring[1] == "vertex") {
        nV = stoul(substring[2]);
      } else if (substring[1] == "face") {
        nF = stoul(substring[2]);
      }
    }
  }

  vertexXyz.resize(nV);
  vertexXyz.shrink_to_fit();

  uint8_t propertyNum = 3;
  double* vertexTmp = new double[propertyNum * nV];
  infile.read((char*)(vertexTmp),
              static_cast<int64_t>(propertyNum * nV * sizeof(double)));
  for (size_t i = 0; i < nV; i++) {
    vertexXyz[i].x() = vertexTmp[i * propertyNum];
    vertexXyz[i].y() = vertexTmp[i * propertyNum + 1];
    vertexXyz[i].z() = vertexTmp[i * propertyNum + 2];
  }

  delete[] vertexTmp;
  vertexTmp = nullptr;

  faceVertexIndex.resize(nF);
  faceVertexIndex.shrink_to_fit();

  for (size_t i = 0; i < nF; i++) {
    uint8_t type;
    infile.read((char*)(&type), 1);

    if (type != 3) {
      printf("Format Incompatible Or Non Trigon!\n");
      return false;
    }

    for (unsigned int j = 0; j < type; j++) {
      int id;
      infile.read((char*)(&id), sizeof(int));
      faceVertexIndex[i].index[j] = static_cast<size_t>(id);
    }
  }

  infile.close();

  return true;
}

//判断几种可能的相交情况
int CalTriangleType(TrigonVertexIndex trigonVID,
                    std::vector<bool>& vertexFlag) {
  bool triVertexFlag[3] = {false, false, false};
  for (int vi = 0; vi < 3; vi++) {
    size_t vid = trigonVID.index[vi];
    triVertexFlag[vi] = vertexFlag[vid];
  }

  int type = 0;
  if (!triVertexFlag[0] && !triVertexFlag[1] && !triVertexFlag[2]) {
    type = 0;
  } else if (!triVertexFlag[0] && !triVertexFlag[1] && triVertexFlag[2]) {
    type = 1;
  } else if (triVertexFlag[0] && !triVertexFlag[1] && !triVertexFlag[2]) {
    type = 2;
  } else if (!triVertexFlag[0] && triVertexFlag[1] && !triVertexFlag[2]) {
    type = 3;
  } else if (triVertexFlag[0] && triVertexFlag[1] && triVertexFlag[2]) {
    type = 4;
  } else if (triVertexFlag[0] && triVertexFlag[1] && !triVertexFlag[2]) {
    type = 5;
  } else if (!triVertexFlag[0] && triVertexFlag[1] && triVertexFlag[2]) {
    type = 6;
  } else if (triVertexFlag[0] && !triVertexFlag[1] && triVertexFlag[2]) {
    type = 7;
  }

  return type;
}

//计算空间线段已知Z值的点的坐标
bool CalPointOfSegmentLineWithZ(Vector3d O, Vector3d E, double z, Vector3d& P) {
  if (E.z() < O.z()) {
    Vector3d tmp = O;
    O = E;
    E = tmp;
  }

  double t = (z - O.z()) / (E.z() - O.z());
  if (t < 0 && t > 1) {
    return false;
  }

  Vector3d D = E - O;
  P = O + D * t;

  return true;
}

//计算空间中三角形与直线相交
void CalTriangleIntersectingLine(TrigonVertexIndex trigonVID, int cornerId,
                                 Vector3d& start, Vector3d& end, double z) {
  vector<Vector3d> xyzList(3);
  for (size_t vi = 0; vi < 3; vi++) {
    size_t vid = trigonVID.index[vi];
    xyzList[vi] = vertexXyz[vid];
  }

  if (cornerId == 0) {
    CalPointOfSegmentLineWithZ(xyzList[0], xyzList[1], z, start);
    CalPointOfSegmentLineWithZ(xyzList[0], xyzList[2], z, end);
  } else if (cornerId == 1) {
    CalPointOfSegmentLineWithZ(xyzList[1], xyzList[0], z, start);
    CalPointOfSegmentLineWithZ(xyzList[1], xyzList[2], z, end);
  } else if (cornerId == 2) {
    CalPointOfSegmentLineWithZ(xyzList[2], xyzList[1], z, start);
    CalPointOfSegmentLineWithZ(xyzList[2], xyzList[0], z, end);
  }
}

bool CalIsoHeightLine(TrigonVertexIndex trigonVID, int type, Vector3d& start,
                      Vector3d& end, double height) {
  bool flag = false;
  switch (type) {
    case 1:
    case 5: {
      CalTriangleIntersectingLine(trigonVID, 2, start, end, height);
      flag = true;
      break;
    }
    case 2:
    case 6: {
      CalTriangleIntersectingLine(trigonVID, 0, start, end, height);
      flag = true;
      break;
    }
    case 3:
    case 7: {
      CalTriangleIntersectingLine(trigonVID, 1, start, end, height);
      flag = true;
      break;
    }
    case 0:
    case 4:
    default:
      break;
  }

  return flag;
}

int main() {
  GDALAllRegister();  // GDAL所有操作都需要先注册格式

  vector<double> heightThresholdList;
  {
    double heightThreshold = startHeight;
    while (heightThreshold < endHeight) {
      heightThresholdList.push_back(heightThreshold);
      heightThreshold = heightThreshold + heightInterval;
    }
  }

  string workDir = getenv("GISBasic");
  string outShpFile = workDir + "/../Data/Terrain/dst.shp";

  string tinPath = workDir + "/../Data/Terrain/terrain.ply";
  if (!ReadTin(tinPath.c_str())) {
    return 1;
  }

  //创建
  GDALDriver* driver =
      GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
  if (!driver) {
    printf("Get Driver ESRI Shapefile Error！\n");
    return 1;
  }

  GDALDataset* dataset =
      driver->Create(outShpFile.c_str(), 0, 0, 0, GDT_Unknown, nullptr);
  OGRLayer* poLayer = dataset->CreateLayer("IsoHeightline", nullptr,
                                           wkbMultiLineStringZM, nullptr);

  OGRFeature* poFeature = new OGRFeature(poLayer->GetLayerDefn());
  OGRMultiLineString multiLineString;

  for (size_t i = 0; i < heightThresholdList.size(); i++) {
    double heightThreshold = heightThresholdList[i];

    std::vector<bool> vertexFlag(vertexXyz.size(), false);
    for (size_t i = 0; i < vertexXyz.size(); i++) {
      if (vertexXyz[i].z() >= heightThreshold) {
        vertexFlag[i] = true;
      }
    }

    for (size_t fi = 0; fi < faceVertexIndex.size(); fi++) {
      int type = CalTriangleType(faceVertexIndex[fi], vertexFlag);

      Vector3d start;
      Vector3d end;
      if (CalIsoHeightLine(faceVertexIndex[fi], type, start, end,
                           heightThreshold)) {
        OGRLinearRing ogrring;
        ogrring.setPoint(0, start.x(), start.y(), start.z());
        ogrring.setPoint(1, end.x(), end.y(), end.z());
        multiLineString.addGeometry(&ogrring);
      }
    }
  }

  poFeature->SetGeometry(&multiLineString);
  if (poLayer->CreateFeature(poFeature) != OGRERR_NONE) {
    printf("Failed to create feature in shapefile.\n");
    return 1;
  }

  //释放
  GDALClose(dataset);
  dataset = nullptr;

  return 0;
}