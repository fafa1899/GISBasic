// Example3Gird2Tin.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Point_set_3.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/Projection_traits_xy_3.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/boost/graph/copy_face_graph.h>
#include <CGAL/boost/graph/graph_traits_Delaunay_triangulation_2.h>
#include <gdal_priv.h>

#include <Eigen/Eigen>
#include <iostream>
#include <string>

using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using Projection_traits = CGAL::Projection_traits_xy_3<Kernel>;
using Point_2 = Kernel::Point_2;
using Point_3 = Kernel::Point_3;
using Segment_3 = Kernel::Segment_3;
// Triangulated Irregular Network
using TIN = CGAL::Delaunay_triangulation_2<Projection_traits>;

using namespace std;
using namespace Eigen;

double CalDistancePointAndLine(const Vector3d& point, const Vector3d& lineBegin,
                               const Vector3d& lineEnd) {
  //直线方向向量
  Vector3d n = lineEnd - lineBegin;

  //直线上某一点的向量到点的向量
  Vector3d m = point - lineBegin;

  return (n.cross(m)).norm() / n.norm();
}

int main(int argc, char* argv[]) {
  GDALAllRegister();

  string workDir = getenv("GISBasic");
  string demPath = workDir + "/../Data/Terrain/dem.tif";
  string tinPath = workDir + "/../Data/Terrain/tin.ply";

  GDALDataset* dem = (GDALDataset*)GDALOpen(demPath.c_str(), GA_ReadOnly);
  if (!dem) {
    cout << "Can't Open Image!" << endl;
    return 1;
  }

  int demWidth = dem->GetRasterXSize();
  int demHeight = dem->GetRasterYSize();
  int bandNum = dem->GetRasterCount();

  double geoTransform[6] = {0};
  dem->GetGeoTransform(geoTransform);
  double dx = geoTransform[1];
  double startx = geoTransform[0] + 0.5 * dx;
  double dy = -geoTransform[5];
  double starty = geoTransform[3] - demHeight * dy + 0.5 * dy;

  //申请buf
  size_t demBufNum = (size_t)demWidth * demHeight;
  size_t demBufOffset = (size_t)demWidth * (demHeight - 1);
  vector<float> demBuf(demBufNum, 0);

  //读取
  int depth = sizeof(float);
  dem->GetRasterBand(1)->RasterIO(
      GF_Read, 0, 0, demWidth, demHeight, demBuf.data() + demBufOffset,
      demWidth, demHeight, GDT_Float32, depth, -demWidth * depth);

  CGAL::Point_set_3<Point_3> points;
  double zThreshold = 5;

  //
  for (int yi = 0; yi < demHeight; yi++) {
    for (int xi = 0; xi < demWidth; xi++) {
      //将四个角点的约束加入，保证与DEM范围一致
      if ((xi == 0 && yi == 0) || (xi == demWidth - 1 && yi == 0) ||
          (xi == demWidth - 1 && yi == demHeight - 1) ||
          (xi == 0 && yi == demHeight - 1)) {
        double gx1 = startx + dx * xi;
        double gy1 = starty + dy * yi;
        size_t m11 = (size_t)(demWidth)*yi + xi;
        points.insert(Point_3(gx1, gy1, demBuf[m11]));
      } else {
        double gx0 = startx + dx * (xi - 1);
        double gy0 = starty + dy * (yi - 1);

        double gx1 = startx + dx * xi;
        double gy1 = starty + dy * yi;

        double gx2 = startx + dx * (xi + 1);
        double gy2 = starty + dy * (yi + 1);

        size_t m00 = (size_t)demWidth * (yi - 1) + xi - 1;
        size_t m01 = (size_t)demWidth * (yi - 1) + xi;
        size_t m02 = (size_t)demWidth * (yi - 1) + xi + 1;

        size_t m10 = (size_t)demWidth * yi + xi - 1;
        size_t m11 = (size_t)demWidth * yi + xi;
        size_t m12 = (size_t)demWidth * yi + xi + 1;

        size_t m20 = (size_t)demWidth * (yi + 1) + xi - 1;
        size_t m21 = (size_t)demWidth * (yi + 1) + xi;
        size_t m22 = (size_t)demWidth * (yi + 1) + xi + 1;

        Vector3d P(gx1, gy1, demBuf[m11]);

        double zMeanDistance = 0;
        int counter = 0;

        if (m00 < demBufNum && m22 < demBufNum) {
          Vector3d A(gx0, gy0, demBuf[m00]);
          Vector3d E(gx2, gy2, demBuf[m22]);
          zMeanDistance = zMeanDistance + CalDistancePointAndLine(P, A, E);
          counter++;
        }

        if (m02 < demBufNum && m20 < demBufNum) {
          Vector3d C(gx2, gy0, demBuf[m02]);
          Vector3d G(gx0, gy2, demBuf[m20]);
          zMeanDistance = zMeanDistance + CalDistancePointAndLine(P, C, G);
          counter++;
        }

        if (m01 < demBufNum && m21 < demBufNum) {
          Vector3d B(gx1, gy0, demBuf[m01]);
          Vector3d F(gx1, gy2, demBuf[m21]);
          zMeanDistance = zMeanDistance + CalDistancePointAndLine(P, B, F);
          counter++;
        }

        if (m12 < demBufNum && m10 < demBufNum) {
          Vector3d D(gx2, gy1, demBuf[m12]);
          Vector3d H(gx0, gy1, demBuf[m10]);
          zMeanDistance = zMeanDistance + CalDistancePointAndLine(P, D, H);
          counter++;
        }

        zMeanDistance = zMeanDistance / counter;

        if (zMeanDistance > zThreshold) {
          points.insert(Point_3(P.x(), P.y(), P.z()));
        }
      }
    }
  }

  GDALClose(dem);

  // Create DSM
  TIN dsm(points.points().begin(), points.points().end());

  using Mesh = CGAL::Surface_mesh<Point_3>;
  Mesh dsmMesh;
  CGAL::copy_face_graph(dsm, dsmMesh);
  std::ofstream dsm_ofile(tinPath, std::ios_base::binary);
  CGAL::set_binary_mode(dsm_ofile);
  CGAL::write_ply(dsm_ofile, dsmMesh);
  dsm_ofile.close();

  return 0;
}