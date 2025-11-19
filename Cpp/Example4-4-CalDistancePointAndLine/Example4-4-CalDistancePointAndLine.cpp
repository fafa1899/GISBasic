// Example4CalDistancePointAndLine.cpp : 此文件包含 "main"
// 函数。程序执行将在此处开始并结束。
//

#include <Eigen/Eigen>
#include <iostream>

using namespace Eigen;
using namespace std;

double CalDistancePointAndLine(Vector3d& point, Vector3d& lineBegin,
                               Vector3d& lineEnd) {
  //直线方向向量
  Vector3d n = lineEnd - lineBegin;

  //直线上某一点的向量到点的向量
  Vector3d m = point - lineBegin;

  return (n.cross(m)).norm() / n.norm();
}

double CalDistancePointAndLine1(Vector3d& point, Vector3d& lineBegin,
                               Vector3d& lineEnd) {
  double A = 1 / (lineEnd.x() - lineBegin.x());
  double B = -1 / (lineEnd.y() - lineBegin.y());
  double C = lineBegin.y() / (lineEnd.y() - lineBegin.y()) -
             lineBegin.x() / (lineEnd.x() - lineBegin.x());

  return abs(A * point.x() + B * point.y() + C) / sqrt(A * A + B * B);
}

int main() {
  Vector3d point(0.5, 0.6, 0);
  Vector3d O(1.0, 2.4, 0);
  Vector3d E(10.2, 11.5, 0);

  cout << "点到直线的距离为:" << CalDistancePointAndLine(point, O, E) << '\n';
  cout << "进行验算:" << CalDistancePointAndLine1(point, O, E) << '\n';
}