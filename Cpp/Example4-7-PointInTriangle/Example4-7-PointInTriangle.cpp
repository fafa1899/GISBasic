// Example7PointInTriangle.cpp : 此文件包含 "main"
// 函数。程序执行将在此处开始并结束。
//

#include <Eigen/Eigen>
#include <iostream>

using namespace Eigen;
using namespace std;

using Triangle = Vector3d[3];

// 判断点P是否在空间三角形内
bool PointInTriangle3D(Triangle triangle, Vector3d& P) {
  auto v0p = P - triangle[0];
  auto v0v1 = triangle[1] - triangle[0];
  auto v0v2 = triangle[2] - triangle[0];

  double D = v0v1.x() * v0v2.y() - v0v1.y() * v0v2.x();
  if (D == 0.0) {
    return false;
  }

  double D1 = v0p.x() * v0v2.y() - v0p.y() * v0v2.x();
  double D2 = v0v1.x() * v0p.y() - v0v1.y() * v0p.x();

  double u = D1 / D;
  double v = D2 / D;

  //如果在三维空间判断，还需要判断第三个向量是否成立
  // double eps = v0v1.z() * u + v0v2.z() * v - P.z();

  if (u >= 0 && v >= 0 && u + v <= 1) {
    return true;
  }

  return false;
}

int main() {
  Triangle triangle = {Vector3d(-20, -25.6, 0), Vector3d(34, -24, 0),
                       Vector3d(30, 27, 0)};

  Vector3d A(1.2, 3.4, 0);
  cout << "点A在三角形内：" << PointInTriangle3D(triangle, A) << endl;

  Vector3d B(14.4, 8.14, 0);
  cout << "点B在三角形内：" << PointInTriangle3D(triangle, B) << endl;
}