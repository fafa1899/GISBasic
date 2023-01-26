// Example9Points2Tin.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Projection_traits_xy_3.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Projection_traits_xy_3<K> Gt;
typedef CGAL::Delaunay_triangulation_2<Gt> Delaunay;
typedef K::Point_3 Point;

#include <ogrsf_frmts.h>

#include <iostream>
#include <string>

using namespace std;

bool ReadVector(vector<Point> &vertexList) {
  string srcFile = getenv("GISBasic");
  srcFile = srcFile + "/../Data/Vector/points.shp";

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

  for (int li = 0; li < poDS->GetLayerCount(); li++) {
    OGRLayer *poLayer = poDS->GetLayer(li);  //读取层
    poLayer->ResetReading();

    //遍历特征
    OGRFeature *poFeature = nullptr;
    while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
      OGRGeometry *geometry = poFeature->GetGeometryRef();
      OGRwkbGeometryType geometryType = geometry->getGeometryType();

      switch (geometryType) {
        case wkbPoint:
        case wkbPointM:
        case wkbPointZM: {
          OGRPoint *ogrPoint = dynamic_cast<OGRPoint *>(geometry);
          if (ogrPoint) {
            vertexList.emplace_back(ogrPoint->getX(), ogrPoint->getY(), 0);
          }
          break;
        }
        case wkbMultiPoint:
        case wkbMultiPointM:
        case wkbMultiPointZM: {
          OGRMultiPoint *ogrMultiPoint =
              dynamic_cast<OGRMultiPoint *>(geometry);
          if (!ogrMultiPoint) {
            continue;
          }

          for (int gi = 0; gi < ogrMultiPoint->getNumGeometries(); gi++) {
            OGRPoint *ogrPoint =
                dynamic_cast<OGRPoint *>(ogrMultiPoint->getGeometryRef(gi));
            if (ogrPoint) {
              vertexList.emplace_back(ogrPoint->getX(), ogrPoint->getY(), 0);
            }
          }

          break;
        }
        default: {
          printf("未处理的特征类型\n");
          break;
        }
      }

      OGRFeature::DestroyFeature(poFeature);
    }
  }

  GDALClose(poDS);
  poDS = nullptr;

  return true;
}

bool WriteVector(const Delaunay &dt) {
  string dstFile = getenv("GISBasic");
  dstFile = dstFile + "/../Data/Out.shp";

  GDALDriver *driver =
      GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
  if (!driver) {
    printf("Get Driver ESRI Shapefile Error！\n");
    return false;
  }

  GDALDataset *dataset =
      driver->Create(dstFile.c_str(), 0, 0, 0, GDT_Unknown, NULL);
  OGRLayer *poLayer = dataset->CreateLayer("tin", NULL, wkbPolygon, NULL);

  //创建面要素
  for (const auto &f : dt.finite_face_handles()) {
    OGRFeature *poFeature = new OGRFeature(poLayer->GetLayerDefn());

    OGRLinearRing ogrring;
    for (int i = 0; i < 3; i++) {
      ogrring.setPoint(i, f->vertex(i)->point().x(), f->vertex(i)->point().y());
    }
    ogrring.closeRings();

    OGRPolygon polygon;
    polygon.addRing(&ogrring);
    poFeature->SetGeometry(&polygon);

    if (poLayer->CreateFeature(poFeature) != OGRERR_NONE) {
      printf("Failed to create feature in shapefile.\n");
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

  vector<Point> vertexList;
  ReadVector(vertexList);

  Delaunay dt(vertexList.begin(), vertexList.end());

  WriteVector(dt);
  return 0;
}
