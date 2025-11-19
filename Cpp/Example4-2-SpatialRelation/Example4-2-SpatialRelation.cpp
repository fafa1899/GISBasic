// Example2SpatialRelation.cpp : 此文件包含 "main"
// 函数。程序执行将在此处开始并结束。
//

#include <ogrsf_frmts.h>

#include <iostream>

using namespace std;

int main() {
  OGRLinearRing linearRing;
  linearRing.addPoint(268.28, 784.75);
  linearRing.addPoint(153.98, 600.60);
  linearRing.addPoint(274.63, 336.02);
  linearRing.addPoint(623.88, 401.64);
  linearRing.addPoint(676.80, 634.47);
  linearRing.addPoint(530.75, 822.85);
  linearRing.closeRings();

  OGRPolygon polygon;
  polygon.addRing(&linearRing);

  cout << "点A是否在多边形内：";
  OGRPoint pointA(407.98, 579.43);  
  cout << polygon.Contains(&pointA) << endl;

  cout << "点B是否在多边形内：";
  OGRPoint pointB(678.92, 482.07);  
  cout << polygon.Contains(&pointB) << endl;
}
