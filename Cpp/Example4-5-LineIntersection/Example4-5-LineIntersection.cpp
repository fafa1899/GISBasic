// Example5LineIntersection.cpp : 此文件包含 "main"
// 函数。程序执行将在此处开始并结束。
//

#include <Eigen/Eigen>
#include <iostream>

using namespace std;
using namespace Eigen;

//两条线段相交
bool LineIntersection2D(const Vector3d& startPoint1, const Vector3d& endPoint1,
                        const Vector3d& startPoint2, const Vector3d& endPoint2,
                        Vector3d& insPoint) {
  Vector3d direction1 = endPoint1 - startPoint1;
  Vector3d direction2 = endPoint2 - startPoint2;

  double D = -direction1.x() * direction2.y() + direction1.y() * direction2.x();
  if (D == 0.0) {
    return false;
  }

  Vector3d O12 = startPoint2 - startPoint1;
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

  insPoint = startPoint1 + direction1 * t1;
  return true;
}

int main() {
  Vector3d O1(1.0, 2.4, 0);
  Vector3d E1(10.2, 11.5, 0);

  Vector3d O2(10.8, 3.2, 0);
  Vector3d E2(2.6, 10.4, 0);

  Vector3d insPoint;

  LineIntersection2D(O1, E1, O2, E2, insPoint);
  cout << "空间两线段相交的交点为：" << insPoint.transpose() << endl;
}
