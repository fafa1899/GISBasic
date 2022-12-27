// Example1VectorConvert.cpp : 此文件包含 "main"
// 函数。程序执行将在此处开始并结束。
//

#include <ogrsf_frmts.h>

#include <Eigen/Eigen>
#include <iostream>
#include <vector>

using namespace std;
using namespace Eigen;

using Point = Vector3d;
using Line = vector<Point>;
using Polygon = vector<Line>;

vector<Polygon> polygonData;

OGRSpatialReference srcFileSpatialReference;
OGRSpatialReference dstFileSpatialReference;

void OgrRing2Line(OGRLinearRing *ogrLinearRing, Line &line) {
  for (int i = 0; i < ogrLinearRing->getNumPoints(); i++) {
    line.emplace_back(ogrLinearRing->getX(i), ogrLinearRing->getY(i),
                      ogrLinearRing->getZ(i));
  }
}

void OgrPolygon2Polygon(OGRPolygon *ogrPolygon, Polygon &polygon) {
  //外环
  Line line;
  OgrRing2Line(ogrPolygon->getExteriorRing(), line);
  polygon.push_back(line);

  //内环
  for (int ri = 0; ri < ogrPolygon->getNumInteriorRings(); ri++) {
    Line line;
    OgrRing2Line(ogrPolygon->getInteriorRing(ri), line);
    polygon.push_back(line);
  }
}

bool ReadShp() {
  string srcFile = getenv("GISBasic");
  srcFile = srcFile + "/../Data/Vector/multipolygons.shp";

  GDALDataset *poDS = (GDALDataset *)GDALOpenEx(srcFile.c_str(), GDAL_OF_VECTOR,
                                                NULL, NULL, NULL);
  if (!poDS) {
    printf("无法读取该文件，请检查数据是否存在问题！");
    return false;
  }

  if (poDS->GetLayerCount() < 1) {
    printf("该文件的层数小于1，请检查数据是否存在问题！");
    return false;
  }

  //原始数据空间参考
  char *pszWKT = nullptr;
  poDS->GetLayer(0)->GetSpatialRef()->exportToWkt(&pszWKT);
  srcFileSpatialReference.importFromWkt(pszWKT);
  CPLFree(pszWKT);
  pszWKT = nullptr;

  for (int li = 0; li < poDS->GetLayerCount(); li++) {
    OGRLayer *poLayer = poDS->GetLayer(li);  //读取层
    poLayer->ResetReading();

    //输出字段名
    OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
    int n = poFDefn->GetFieldCount();
    for (int iField = 0; iField < n; iField++) {
      OGRFieldDefn *OGRFieldDefn = poFDefn->GetFieldDefn(iField);
      cout << OGRFieldDefn->GetNameRef() << endl;
    }

    //遍历特征
    OGRFeature *poFeature = nullptr;
    while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
      OGRGeometry *geometry = poFeature->GetGeometryRef();
      OGRwkbGeometryType geometryType = geometry->getGeometryType();

      switch (geometryType) {
        case wkbPolygon:
        case wkbPolygonM:
        case wkbPolygonZM: {
          OGRPolygon *ogrPolygon = dynamic_cast<OGRPolygon *>(geometry);
          if (!ogrPolygon) {
            continue;
          }

          Polygon polygon;
          OgrPolygon2Polygon(ogrPolygon, polygon);
          polygonData.push_back(polygon);

          break;
        }
        case wkbMultiPolygon:
        case wkbMultiPolygonM:
        case wkbMultiPolygonZM: {
          OGRMultiPolygon *ogrMultiPolygon =
              dynamic_cast<OGRMultiPolygon *>(geometry);
          if (!ogrMultiPolygon) {
            continue;
          }

          for (int gi = 0; gi < ogrMultiPolygon->getNumGeometries(); gi++) {
            OGRPolygon *ogrPolygon =
                dynamic_cast<OGRPolygon *>(ogrMultiPolygon->getGeometryRef(gi));
            if (!ogrPolygon) {
              continue;
            }

            Polygon polygon;
            OgrPolygon2Polygon(ogrPolygon, polygon);
            polygonData.push_back(polygon);
          }

          break;
        }
        default: {
          printf("未处理的特征类型\n");
          break;
        }
      }

      //输出每个字段的值
      for (int iField = 0; iField < n; iField++) {
        cout << poFeature->GetFieldAsString(iField) << "    ";
      }
      cout << endl;

      OGRFeature::DestroyFeature(poFeature);
    }
  }

  GDALClose(poDS);
  poDS = nullptr;

  return true;
}

void Convert() {
  dstFileSpatialReference.importFromEPSG(3857);
  dstFileSpatialReference.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
  srcFileSpatialReference.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

  OGRCoordinateTransformation *src2DstTransformation =
      OGRCreateCoordinateTransformation(&srcFileSpatialReference,
                                        &dstFileSpatialReference);

  if (!src2DstTransformation) {
    return;
  }

  for (auto &polygon : polygonData) {
    for (auto &line : polygon) {
      for (auto &point : line) {
        src2DstTransformation->Transform(1, point.data(), point.data() + 1,
                                         point.data() + 2);
      }
    }
  }
}

bool CreateField(OGRLayer *poLayer) {
  // 字符串
  OGRFieldDefn oField1("Type", OFTString);
  oField1.SetWidth(8);
  if (poLayer->CreateField(&oField1) != OGRERR_NONE) {
    printf("Creating Name field failed.\n");
    return false;
  }

  // 浮点数
  OGRFieldDefn oField2("Area", OFTReal);
  oField2.SetPrecision(3);
  if (poLayer->CreateField(&oField2) != OGRERR_NONE) {
    printf("Creating Name field failed.\n");
    return false;
  }

  // 整型
  OGRFieldDefn oField3("VertexCount", OFTInteger);
  if (poLayer->CreateField(&oField3) != OGRERR_NONE) {
    printf("Creating Name field failed.\n");
    return false;
  }

  return true;
}

bool WriteGeoJson() {
  string dstFile = getenv("GISBasic");
  dstFile = dstFile + "/../Data/Out.geoJson";

  //创建
  GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("GeoJSON");
  if (!driver) {
    printf("Get Driver GeoJSON Error！\n");
    return false;
  }

  GDALDataset *dataset =
      driver->Create(dstFile.c_str(), 0, 0, 0, GDT_Unknown, NULL);
  OGRLayer *poLayer = dataset->CreateLayer(
      "FirstLayer", &dstFileSpatialReference, wkbPolygon, NULL);

  if (!CreateField(poLayer)) {
    return false;
  }

  //创建特征
  for (const auto &polygon : polygonData) {
    OGRFeature ogrFeature(poLayer->GetLayerDefn());

    OGRPolygon ogrPolygon;

    int vertexNum = 0;
    for (const auto &line : polygon) {
      OGRLinearRing ogrRing;
      for (const auto &point : line) {
        ogrRing.addPoint(point.x(), point.y(), point.z());
        vertexNum++;
      }
      ogrPolygon.addRing(&ogrRing);
    }
    ogrFeature.SetGeometry(&ogrPolygon);

    ogrFeature.SetField("Type", "Polygon");
    ogrFeature.SetField("Area", ogrPolygon.get_Area());
    ogrFeature.SetField("VertexCount", vertexNum);

    if (poLayer->CreateFeature(&ogrFeature) != OGRERR_NONE) {
      printf("Failed to create feature.\n");
      return false;
    }
  }

  //释放
  GDALClose(dataset);
  dataset = nullptr;

  return true;
}

int main() {
  GDALAllRegister();
  CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");  //支持中文路径
  CPLSetConfigOption("SHAPE_ENCODING", "");           //解决中文乱码问题

  if (!ReadShp()) {
    return 1;
  }

  Convert();

  WriteGeoJson();
}