// Example6LineOrthogonIntersection.cpp : 此文件包含 "main"
// 函数。程序执行将在此处开始并结束。
//

#include <Eigen/Eigen>
#include <iostream>

using namespace Eigen;
using namespace std;

using LineSegment = Vector2d[2];
using Orthogon = double[4];  //按照最小X，最小Y，最大X，最大Y进行排序

bool PointInOrthogon(const Orthogon& orthogon, const Vector2d& point) {
  return (point.x() >= orthogon[0]) && (point.x() <= orthogon[2]) &&
         (point.y() >= orthogon[1]) && (point.y() <= orthogon[3]);
}

//两条线段相交
bool LineIntersection2D(const Vector2d& startPoint1, const Vector2d& endPoint1,
                        const Vector2d& startPoint2,
                        const Vector2d& endPoint2) {
  Vector2d direction1 = endPoint1 - startPoint1;
  Vector2d direction2 = endPoint2 - startPoint2;

  double D = -direction1.x() * direction2.y() + direction1.y() * direction2.x();
  if (D == 0.0) {
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

//线段与矩形相交
bool IsIntersectsOrthogon2D(const LineSegment& lineSegment,
                            const Orthogon& orthogon) {
  if (PointInOrthogon(orthogon, lineSegment[0]) ||
      PointInOrthogon(orthogon, lineSegment[1])) {
    return true;
  }

  LineSegment diagonal1 = {Vector2d(orthogon[0], orthogon[1]),
                           Vector2d(orthogon[2], orthogon[3])};
  LineSegment diagonal2 = {Vector2d(orthogon[0], orthogon[3]),
                           Vector2d(orthogon[2], orthogon[1])};

  return LineIntersection2D(lineSegment[0], lineSegment[1], diagonal1[0],
                            diagonal1[1]) ||
         LineIntersection2D(lineSegment[0], lineSegment[1], diagonal2[0],
                            diagonal2[1]);
}

int main() {
  Orthogon orthogon = {-50, -20, 40, 30};
  LineSegment lineSegment = {Vector2d(20, 20), Vector2d(16, 14)};
  cout << "线段与矩形是否相交："
       << IsIntersectsOrthogon2D(lineSegment, orthogon) << endl;
}