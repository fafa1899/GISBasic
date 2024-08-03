#include <gdal_priv.h>

#include <Eigen/Eigen>
#include <iostream>

using namespace std;
using namespace Eigen;

const double pi = 3.14159265358979323846;
const double d2r = pi / 180;
const double r2d = 180 / pi;

int demWidth;
int demHeight;

double geoTransform[6] = {0};
double startX;  //���Ͻǵ�����X
double dx;      // X����ķֱ���
double startY;  //���Ͻǵ�����Y
double dy;      // Y����ķֱ���

double noValue;

vector<float> demBuf;

vector<uint8_t> dstBuf;

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
  startX = geoTransform[0];  //���Ͻǵ�����X
  dx = geoTransform[1];      // X����ķֱ���
  startY = geoTransform[3];  //���Ͻǵ�����Y
  dy = geoTransform[5];      // Y����ķֱ���

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

//�����������ķ�����
void Cal_Normal_3D(const Vector3d& v1, const Vector3d& v2, const Vector3d& v3,
                   Vector3d& vn) {
  // v1(n1,n2,n3);
  //ƽ�淽��: na * (x �C n1) + nb * (y �C n2) + nc * (z �C n3) = 0 ;
  vn.x() = (v2.y() - v1.y()) * (v3.z() - v1.z()) -
           (v2.z() - v1.z()) * (v3.y() - v1.y());
  vn.y() = (v2.z() - v1.z()) * (v3.x() - v1.x()) -
           (v2.x() - v1.x()) * (v3.z() - v1.z());
  vn.z() = (v2.x() - v1.x()) * (v3.y() - v1.y()) -
           (v2.y() - v1.y()) * (v3.x() - v1.x());
}

void Hillshade() {
  double minZ = DBL_MAX;
  double maxZ = -DBL_MAX;

  vector<Vector3d> pointList;  //���еĶ���
  for (int yi = 0; yi < demHeight; yi++) {
    for (int xi = 0; xi < demWidth; xi++) {
      size_t m = (size_t)demWidth * yi + xi;
      double x = startX + (xi + 0.5) * dx;
      double y = startY + (yi + 0.5) * dy;
      double z = demBuf[m];
      pointList.emplace_back(x, y, z);

      if (abs(z - noValue) < 0.01 || z < -11034 || z > 8844.43) {
        continue;
      }

      minZ = (std::min)(minZ, z);
      maxZ = (std::max)(maxZ, z);
    }
  }

  //����ÿ����ķ�����
  multimap<size_t, size_t> pointMapFaceNomalId;
  vector<Vector3d> faceNomalList;
  for (int yi = 0; yi < demHeight - 1; yi++) {
    for (int xi = 0; xi < demWidth - 1; xi++) {
      size_t y0x0 = (size_t)demWidth * yi + xi;
      size_t y1x0 = (size_t)demWidth * (yi + 1) + xi;
      size_t y0x1 = (size_t)demWidth * yi + xi + 1;
      size_t y1x1 = (size_t)demWidth * (yi + 1) + xi + 1;

      Vector3d vn;
      Cal_Normal_3D(pointList[y0x0], pointList[y1x0], pointList[y0x1], vn);
      pointMapFaceNomalId.insert(make_pair(y0x0, faceNomalList.size()));
      pointMapFaceNomalId.insert(make_pair(y1x0, faceNomalList.size()));
      pointMapFaceNomalId.insert(make_pair(y0x1, faceNomalList.size()));
      faceNomalList.push_back(vn);

      Cal_Normal_3D(pointList[y1x0], pointList[y1x1], pointList[y0x1], vn);
      pointMapFaceNomalId.insert(make_pair(y1x0, faceNomalList.size()));
      pointMapFaceNomalId.insert(make_pair(y1x1, faceNomalList.size()));
      pointMapFaceNomalId.insert(make_pair(y0x1, faceNomalList.size()));
      faceNomalList.push_back(vn);
    }
  }

  //���÷���ƽ�й�
  double solarAltitude = 45.0;
  double solarAzimuth = 315.0;
  Vector3d arrayvector(0.0, 0.0, 0.0);
  double fAltitude = solarAltitude * d2r;  //��Դ�߶Ƚ�
  double fAzimuth = solarAzimuth * d2r;    //��Դ��λ��
  arrayvector[0] = cos(fAltitude) * cos(fAzimuth);
  arrayvector[1] = cos(fAltitude) * sin(fAzimuth);
  arrayvector[2] = sin(fAltitude);

  size_t dstBufNum = (size_t)demWidth * demHeight;
  dstBuf.resize(dstBufNum, 0);

  for (int yi = 0; yi < demHeight; yi++) {
    for (int xi = 0; xi < demWidth; xi++) {
      size_t m = (size_t)demWidth * yi + xi;

      const auto& beg = pointMapFaceNomalId.lower_bound(m);
      const auto& end = pointMapFaceNomalId.upper_bound(m);

      Vector3d n(0, 0, 0);
      int ci = 0;
      for (auto it = beg; it != end; ++it) {
        n = n + faceNomalList[it->second];
        ci++;
      }

      n.normalize();

      double angle = acos(n.dot(arrayvector)) * r2d;
      double value = (std::min)((std::max)(angle / 90 * 255, 0.0), 255.0);
      dstBuf[m] = (GByte)value;
    }
  }
}

void WriteDst() {
  string workDir = getenv("GISBasic");
  string demPath = workDir + "/../Data/Terrain/dst.tif";

  GDALDriver* pDriver =
      GetGDALDriverManager()->GetDriverByName("GTIFF");  //ͼ������
  char** ppszOptions = NULL;
  ppszOptions =
      CSLSetNameValue(ppszOptions, "BIGTIFF", "IF_NEEDED");  //����ͼ����Ϣ
  GDALDataset* dst = pDriver->Create(demPath.c_str(), demWidth, demHeight, 1,
                                     GDT_Byte, ppszOptions);
  if (!dst) {
    printf("Can't Write Image!");
    return;
  }

  dst->SetGeoTransform(geoTransform);

  int depth = sizeof(uint8_t);
  dst->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, demWidth, demHeight,
                                  dstBuf.data(), demWidth, demHeight, GDT_Byte,
                                  depth, demWidth * depth);

  GDALClose(dst);
  dst = nullptr;
}

int main() {
  GDALAllRegister();  // GDAL���в�������Ҫ��ע���ʽ

  //����Proj����
  std::string projDataPath = getenv("GISBasic");
  projDataPath += "/share/proj";
  CPLSetConfigOption("PROJ_LIB", projDataPath.c_str());

  ReadDem();

  Hillshade();

  WriteDst();

  return 0;
}