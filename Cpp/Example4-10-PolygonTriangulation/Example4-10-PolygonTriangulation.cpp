// Example10PolygonTriangulation.cpp : 此文件包含 "main"
// 函数。程序执行将在此处开始并结束。
//

#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>

#include <iostream>

struct FaceInfo2 {
  FaceInfo2() {}
  int nesting_level;
  bool in_domain() { return nesting_level % 2 == 1; }
};

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Triangulation_vertex_base_2<K> Vb;
typedef CGAL::Triangulation_face_base_with_info_2<FaceInfo2, K> Fbb;
typedef CGAL::Constrained_triangulation_face_base_2<K, Fbb> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb, Fb> TDS;
typedef CGAL::Exact_predicates_tag Itag;
typedef CGAL::Constrained_Delaunay_triangulation_2<K, TDS, Itag> CDT;
typedef CDT::Point Point;
typedef CGAL::Polygon_2<K> Polygon_2;
typedef CDT::Face_handle Face_handle;

void mark_domains(CDT& ct, Face_handle start, int index,
                  std::list<CDT::Edge>& border) {
  if (start->info().nesting_level != -1) {
    return;
  }
  std::list<Face_handle> queue;
  queue.push_back(start);
  while (!queue.empty()) {
    Face_handle fh = queue.front();
    queue.pop_front();
    if (fh->info().nesting_level == -1) {
      fh->info().nesting_level = index;
      for (int i = 0; i < 3; i++) {
        CDT::Edge e(fh, i);
        Face_handle n = fh->neighbor(i);
        if (n->info().nesting_level == -1) {
          if (ct.is_constrained(e))
            border.push_back(e);
          else
            queue.push_back(n);
        }
      }
    }
  }
}

// explore set of facets connected with non constrained edges, and attribute to
// each such set a nesting level. We start from facets incident to the infinite
// vertex, with a nesting level of 0. Then we recursively consider the
// non-explored facets incident to constrained edges bounding the former set and
// increase the nesting level by 1. Facets in the domain are those with an odd
// nesting level.
void mark_domains(CDT& cdt) {
  for (CDT::Face_handle f : cdt.all_face_handles()) {
    f->info().nesting_level = -1;
  }
  std::list<CDT::Edge> border;
  mark_domains(cdt, cdt.infinite_face(), 0, border);
  while (!border.empty()) {
    CDT::Edge e = border.front();
    border.pop_front();
    Face_handle n = e.first->neighbor(e.second);
    if (n->info().nesting_level == -1) {
      mark_domains(cdt, n, e.first->info().nesting_level + 1, border);
    }
  }
}

bool WriteShp(CDT& cdt) {
  GDALAllRegister();

  GDALDriver* driver =
      GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
  if (!driver) {
    printf("Get Driver ESRI Shapefile Error！\n");
    return false;
  }

  std::string dstFile = getenv("GISBasic");
  dstFile = dstFile + "/../Data/Out.shp";
  GDALDataset* dataset =
      driver->Create(dstFile.c_str(), 0, 0, 0, GDT_Unknown, NULL);
  OGRLayer* poLayer = dataset->CreateLayer("test", NULL, wkbPolygon, NULL);

  //创建面要素
  for (Face_handle f : cdt.finite_face_handles()) {
    if (f->info().in_domain()) {
      OGRFeature* poFeature = new OGRFeature(poLayer->GetLayerDefn());

      OGRLinearRing ogrring;
      for (int i = 0; i < 3; i++) {
        ogrring.setPoint(i, f->vertex(i)->point().x(),
                         f->vertex(i)->point().y());
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
  }

  //释放
  GDALClose(dataset);
  dataset = nullptr;

  return true;
}

int main() {
  //创建三个不相交的嵌套多边形
  Polygon_2 polygon1;
  polygon1.push_back(Point(-0.558868038740926, -0.38960351089588));
  polygon1.push_back(Point(2.77833686440678, 5.37465950363197));
  polygon1.push_back(Point(6.97052814769976, 8.07751967312349));
  polygon1.push_back(Point(13.9207400121065, 5.65046156174335));
  polygon1.push_back(Point(15.5755523607748, -1.98925544794189));
  polygon1.push_back(Point(6.36376361985472, -6.18144673123487));

  Polygon_2 polygon2;
  polygon2.push_back(Point(2.17935556413387, 1.4555590039808));
  polygon2.push_back(Point(3.75630057749723, 4.02942327866582));
  polygon2.push_back(Point(5.58700685737883, 4.71820385921534));
  polygon2.push_back(Point(6.54767450919789, 1.76369768475295));
  polygon2.push_back(Point(5.71388749063795, -0.900795613688593));
  polygon2.push_back(Point(3.21252643495814, -0.320769861646896));

  Polygon_2 polygon3;
  polygon3.push_back(Point(7.74397762278389, 0.821155837685192));
  polygon3.push_back(Point(9.13966458863422, 4.24693293568146));
  polygon3.push_back(Point(10.1909612642098, 1.83620090375816));
  polygon3.push_back(Point(12.1485481773505, 4.84508449247446));
  polygon3.push_back(Point(11.4416417920497, -2.29648257953892));
  polygon3.push_back(Point(10.1547096547072, 0.712401009177374));

  //将多边形插入受约束的三角剖分
  CDT cdt;
  cdt.insert_constraint(polygon1.vertices_begin(), polygon1.vertices_end(),
                        true);
  cdt.insert_constraint(polygon2.vertices_begin(), polygon2.vertices_end(),
                        true);
  cdt.insert_constraint(polygon3.vertices_begin(), polygon3.vertices_end(),
                        true);

  //标记由多边形界定的域内的面
  mark_domains(cdt);

  //将结果输出成shp文件，方便查看
  WriteShp(cdt);

  return 0;
}
