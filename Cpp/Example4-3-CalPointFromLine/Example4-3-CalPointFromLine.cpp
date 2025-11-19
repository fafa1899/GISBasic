// Example3CalPointFromLine.cpp : 此文件包含 "main"
// 函数。程序执行将在此处开始并结束。
//

#include <Eigen/Eigen>
#include <iostream>

using namespace Eigen;
using namespace std;

void CalPointFromLineWithDistance(const Vector2d& O, const Vector2d& E,
                                  double d, Vector2d& P) {
  Vector2d D = E - O;
  double t = d / D.norm();
  P = O + t * D;
}

int main() {
  Vector2d O(1.0, 2.4);
  Vector2d E(10.2, 11.5);
  double d = 5;
  Vector2d P;

  CalPointFromLineWithDistance(O, E, d, P);
  cout << "计算的点为：" << P.x() << '\t' << P.y() << '\n';
  cout << "验算距离是否为" << d << "：" << (P - O).norm() << '\n';
}
