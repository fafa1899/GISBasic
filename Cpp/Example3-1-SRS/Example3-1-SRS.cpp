// Example1SRS.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <ogr_spatialref.h>

#include <iostream>

using namespace std;

void OutputGcs() {
  OGRSpatialReference spatialReference;
  spatialReference.importFromEPSG(4326);  // WGS84
  // spatialReference.importFromEPSG(4214);//BeiJing54
  // spatialReference.importFromEPSG(4610);//XIAN80
  // spatialReference.importFromEPSG(4490);//CGCS2000

  char* pszWKT = nullptr;
  spatialReference.exportToPrettyWkt(&pszWKT);
  cout << pszWKT << endl;
  CPLFree(pszWKT);
  pszWKT = nullptr;
}

void OutputPcs() {
  OGRSpatialReference spatialReference;
  spatialReference.importFromEPSG(4490);  // CGCS2000
  spatialReference.SetTM(0, 114, 1.0, 38500000, 0);

  char* pszWKT = nullptr;
  spatialReference.exportToPrettyWkt(&pszWKT);
  cout << pszWKT << endl;
  CPLFree(pszWKT);
  pszWKT = nullptr;
}

int main() {
  //设置Proj数据
  string projDataPath = getenv("GISBasic");
  projDataPath += "/share/proj";
  CPLSetConfigOption("PROJ_LIB", projDataPath.c_str());

  cout << "地理坐标系，WGS84坐标系：" << endl;
  OutputGcs();
  cout << "投影坐标系，高斯克吕格投影坐标系：" << endl;
  OutputPcs();
}