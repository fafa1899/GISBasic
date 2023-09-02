#include <gdal_priv.h>

#include <iostream>

using namespace std;

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
  //���÷���ƽ�й�
  double solarAltitude = 45.0;
  double solarAzimuth = 315.0;

  //
  double Zenith_rad = (90 - solarAltitude) * d2r;
  double Azimuth_math = 360.0 - solarAzimuth + 90;
  if (Azimuth_math >= 360.0) {
    Azimuth_math = Azimuth_math - 360.0;
  }
  double Azimuth_rad = (Azimuth_math)*d2r;

  size_t dstBufNum = (size_t)demWidth * demHeight;
  dstBuf.resize(dstBufNum, 0);

  // a b c
  // d e f
  // g h i
  double z_factor = 1;
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

      dstBuf[e] = (GByte)(std::min(std::max(Hillshade, 0.0), 255.0));
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

  ReadDem();

  Hillshade();

  WriteDst();

  return 0;
}
