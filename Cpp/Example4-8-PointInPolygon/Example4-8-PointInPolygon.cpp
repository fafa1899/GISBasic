// Example8PointInPolygon.cpp : 此文件包含 "main"
// 函数。程序执行将在此处开始并结束。
//

#include <Eigen/Eigen>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#define EPSILON 0.000001

using namespace Eigen;
using namespace std;

using Polygon = vector<Vector2d>;

const double epsilon = 0.000000001;

//判断点在线段上
bool PointInLine(const Vector2d& point, const Vector2d& startPoint,
                 const Vector2d& endPoint) {
  Vector3d P1P2;
  P1P2 << endPoint - startPoint, 0;
  Vector3d P1P;
  P1P << point - startPoint, 0;

  if (fabs((P1P2.cross(P1P)).norm()) > epsilon) {
    return false;
  }

  double dotProduct = P1P2.dot(P1P);
  if (dotProduct > 0 && dotProduct < P1P2.squaredNorm()) {
    return true;
  }

  return false;
}

//两条线段相交
bool LineIntersection(const Vector2d& startPoint1, const Vector2d& endPoint1,
                      const Vector2d& startPoint2, const Vector2d& endPoint2) {
  Vector2d direction1 = endPoint1 - startPoint1;
  Vector2d direction2 = endPoint2 - startPoint2;

  double D = -direction1.x() * direction2.y() + direction1.y() * direction2.x();
  if (fabs(D) < epsilon) {
    return false;
  }

  Vector2d O12 = startPoint2 - startPoint1;
  double D1 = -O12.x() * direction2.y() + O12.y() * direction2.x();
  double D2 = direction1.x() * O12.y() - direction1.y() * O12.x();

  double t1 = D1 / D;
  if (t1 < 0 || t1 > 1) {
    return false;
  }

  double t2 = D2 / D;
  if (t2 < 0 || t2 > 1) {
    return false;
  }

  return true;
}

//判断点在多边形内
bool Point_In_Polygon_2D(const Vector2d& point, const Polygon& polygon) {
  bool isInside = false;
  int count = 0;

  double minX = DBL_MAX;
  for (int i = 0; i < polygon.size(); i++) {
    minX = std::min(minX, polygon[i].x());
  }

  const Vector2d& testRayLineStart = point;
  Vector2d testRayLineEnd(minX - 10,
                          point.y());  //取最小的X值还小的值作为射线的终点

  //遍历每一条边
  for (int i = 0; i < polygon.size() - 1; i++) {
    if (PointInLine(point, polygon[i], polygon[i + 1])) {
      return true;
    }

    if (PointInLine(polygon[i], testRayLineStart, testRayLineEnd)) {
      if (polygon[i].y() > polygon[i + 1].y())  //只保证上端点+1
      {
        count++;
      }
    } else if (PointInLine(polygon[i + 1], testRayLineStart, testRayLineEnd)) {
      if (polygon[i + 1].y() > polygon[i].y())  //只保证上端点+1
      {
        count++;
      }
    } else if (LineIntersection(polygon[i], polygon[i + 1], testRayLineStart,
                                testRayLineEnd)) {
      count++;
    }
  }

  if (count % 2 == 1) {
    isInside = true;
  }

  return isInside;
}

int main() {
  //定义一个多边形（六边形）
  Polygon polygon;
  polygon.emplace_back(268.28, 784.75);
  polygon.emplace_back(153.98, 600.60);
  polygon.emplace_back(274.63, 336.02);
  polygon.emplace_back(623.88, 401.64);
  polygon.emplace_back(676.80, 634.47);
  polygon.emplace_back(530.75, 822.85);
  polygon.emplace_back(268.28, 784.75);  //将起始点放入尾部，方便遍历每一条边

  Vector2d a(407.98, 579.43);
  cout << "点（407.98, 579.43）是否在多边形内："
       << Point_In_Polygon_2D(a, polygon) << endl;

  Vector2d b(678.92, 482.07);
  cout << "点（678.92, 482.07）是否在多边形内："
       << Point_In_Polygon_2D(b, polygon) << endl;

  return 0;
}
