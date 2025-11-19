// Example6EnuConvertor.cpp : 此文件包含 "main"
// 函数。程序执行将在此处开始并结束。
//

#include <Eigen/Eigen>
#include <iostream>

using namespace std;

const double epsilon = 0.000000000000001;
const double pi = 3.14159265358979323846;
const double d2r = pi / 180;
const double r2d = 180 / pi;

const double a = 6378137.0;              //椭球长半轴
const double f_inverse = 298.257223563;  //扁率倒数
const double b = a - a / f_inverse;
// const double b = 6356752.314245;			//椭球短半轴

const double e = sqrt(a * a - b * b) / a;

void Blh2Xyz(double& x, double& y, double& z) {
  double L = x * d2r;
  double B = y * d2r;
  double H = z;

  double N = a / sqrt(1 - e * e * sin(B) * sin(B));
  x = (N + H) * cos(B) * cos(L);
  y = (N + H) * cos(B) * sin(L);
  z = (N * (1 - e * e) + H) * sin(B);
}

void Xyz2Blh(double& x, double& y, double& z) {
  double tmpX = x;
  double temY = y;
  double temZ = z;

  double curB = 0;
  double N = 0;
  double calB = atan2(temZ, sqrt(tmpX * tmpX + temY * temY));

  int counter = 0;
  while (abs(curB - calB) * r2d > epsilon && counter < 25) {
    curB = calB;
    N = a / sqrt(1 - e * e * sin(curB) * sin(curB));
    calB = atan2(temZ + N * e * e * sin(curB), sqrt(tmpX * tmpX + temY * temY));
    counter++;
  }

  x = atan2(temY, tmpX) * r2d;
  y = curB * r2d;
  z = temZ / sin(curB) - N * (1 - e * e);
}

void CalEcef2Enu(Eigen::Vector3d& topocentricOrigin,
                 Eigen::Matrix4d& resultMat) {
  double rzAngle = -(topocentricOrigin.x() * d2r + pi / 2);
  Eigen::AngleAxisd rzAngleAxis(rzAngle, Eigen::Vector3d(0, 0, 1));
  Eigen::Matrix3d rZ = rzAngleAxis.matrix();

  double rxAngle = -(pi / 2 - topocentricOrigin.y() * d2r);
  Eigen::AngleAxisd rxAngleAxis(rxAngle, Eigen::Vector3d(1, 0, 0));
  Eigen::Matrix3d rX = rxAngleAxis.matrix();

  Eigen::Matrix4d rotation;
  rotation.setIdentity();
  rotation.block<3, 3>(0, 0) = (rX * rZ);
  // cout << rotation << endl;

  double tx = topocentricOrigin.x();
  double ty = topocentricOrigin.y();
  double tz = topocentricOrigin.z();
  Blh2Xyz(tx, ty, tz);
  Eigen::Matrix4d translation;
  translation.setIdentity();
  translation(0, 3) = -tx;
  translation(1, 3) = -ty;
  translation(2, 3) = -tz;

  resultMat = rotation * translation;
}

void CalEnu2Ecef(Eigen::Vector3d& topocentricOrigin,
                 Eigen::Matrix4d& resultMat) {
  double rzAngle = (topocentricOrigin.x() * d2r + pi / 2);
  Eigen::AngleAxisd rzAngleAxis(rzAngle, Eigen::Vector3d(0, 0, 1));
  Eigen::Matrix3d rZ = rzAngleAxis.matrix();

  double rxAngle = (pi / 2 - topocentricOrigin.y() * d2r);
  Eigen::AngleAxisd rxAngleAxis(rxAngle, Eigen::Vector3d(1, 0, 0));
  Eigen::Matrix3d rX = rxAngleAxis.matrix();

  Eigen::Matrix4d rotation;
  rotation.setIdentity();
  rotation.block<3, 3>(0, 0) = (rZ * rX);
  // cout << rotation << endl;

  double tx = topocentricOrigin.x();
  double ty = topocentricOrigin.y();
  double tz = topocentricOrigin.z();
  Blh2Xyz(tx, ty, tz);
  Eigen::Matrix4d translation;
  translation.setIdentity();
  translation(0, 3) = tx;
  translation(1, 3) = ty;
  translation(2, 3) = tz;

  resultMat = translation * rotation;
}

void TestXYZ2ENU() {
  double L = 116.9395751953;
  double B = 36.7399177551;
  double H = 0;

  cout << fixed << endl;
  Eigen::Vector3d topocentricOrigin(L, B, H);
  Eigen::Matrix4d wolrd2localMatrix;
  CalEcef2Enu(topocentricOrigin, wolrd2localMatrix);
  cout << "地心转站心矩阵：" << endl;
  cout << wolrd2localMatrix << endl << endl;

  cout << "站心转地心矩阵：" << endl;
  Eigen::Matrix4d local2WolrdMatrix;
  CalEnu2Ecef(topocentricOrigin, local2WolrdMatrix);
  cout << local2WolrdMatrix << endl << endl;

  double x = 117;
  double y = 37;
  double z = 10.3;
  Blh2Xyz(x, y, z);

  cout << "ECEF坐标（世界坐标）：" << endl;
  Eigen::Vector4d xyz(x, y, z, 1);
  cout << xyz << endl << endl;

  cout << "ENU坐标（局部坐标）：" << endl;
  Eigen::Vector4d enu = wolrd2localMatrix * xyz;
  cout << enu << endl;
}

int main() {
  cout << "使用Eigen进行转换实现：" << endl;
  TestXYZ2ENU();
}