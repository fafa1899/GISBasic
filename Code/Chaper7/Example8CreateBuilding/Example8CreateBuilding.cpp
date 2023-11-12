#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>
#include <postprocess.h>
#include <scene.h>

#include <Eigen/Eigen>
#include <Exporter.hpp>
#include <iostream>

using namespace std;
using namespace Eigen;

struct FaceInfo2 {
  FaceInfo2() {}
  int nesting_level;
  bool in_domain() { return nesting_level % 2 == 1; }
};

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Triangulation_vertex_base_2<K> Vb;
typedef CGAL::Triangulation_face_base_with_info_2<FaceInfo2, K> Fbb;
typedef CGAL::Constrained_triangulation_face_base_2<K, Fbb> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb, Fb> TDS;
typedef CGAL::Exact_predicates_tag Itag;
typedef CGAL::Constrained_Delaunay_triangulation_2<K, TDS, Itag> CDT;
typedef CDT::Point Point;
typedef CGAL::Polygon_2<K> Polygon_2;
typedef CDT::Face_handle Face_handle;

using MyPoint = Vector2d;
using MyPolygon = vector<MyPoint>;
MyPolygon simplePolygon;

vector<aiVector3D> vertexPositions;

const double pi = 3.14159265358979323846;
const double d2r = pi / 180;

const double a = 6378137.0;              //椭球长半轴
const double f_inverse = 298.257223563;  //扁率倒数
const double b = a - a / f_inverse;
const double e = sqrt(a * a - b * b) / a;

double L = -77.036252;
double B = 38.897557;
double H = 0;
Eigen::Matrix4d wolrd2localMatrix;

double wallHeight = 26;

bool ReadShp() {
  GDALAllRegister();

  string srcFile = getenv("GISBasic");
  srcFile = srcFile + "/../Data/Model/wh.shp";

  GDALDataset *poDS = (GDALDataset *)GDALOpenEx(srcFile.c_str(), GDAL_OF_VECTOR,
                                                NULL, NULL, NULL);
  if (!poDS) {
    printf("无法读取该文件，请检查数据是否存在问题！");
    return false;
  }

  if (poDS->GetLayerCount() < 1) {
    printf("该文件的层数小于1，请检查数据是否存在问题！");
    return false;
  }

  for (int li = 0; li < poDS->GetLayerCount(); li++) {
    OGRLayer *poLayer = poDS->GetLayer(li);  //读取层
    poLayer->ResetReading();

    //遍历特征
    OGRFeature *poFeature = nullptr;
    while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
      OGRPolygon *ogrPolygon =
          dynamic_cast<OGRPolygon *>(poFeature->GetGeometryRef());
      if (!ogrPolygon) {
        continue;
      }

      OGRLinearRing *ring = ogrPolygon->getExteriorRing();
      for (int i = 0; i < ring->getNumPoints(); i++) {
        simplePolygon.push_back(Vector2d(ring->getX(i), ring->getY(i)));
      }

      OGRFeature::DestroyFeature(poFeature);
    }
  }

  GDALClose(poDS);
  poDS = nullptr;

  return true;
}

void Blh2Xyz(double &x, double &y, double &z) {
  double L = x * d2r;
  double B = y * d2r;
  double H = z;

  double N = a / sqrt(1 - e * e * sin(B) * sin(B));
  x = (N + H) * cos(B) * cos(L);
  y = (N + H) * cos(B) * sin(L);
  z = (N * (1 - e * e) + H) * sin(B);
}

void CalEcef2EnuMatrix() {
  Eigen::Vector3d topocentricOrigin(L, B, H);

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

  wolrd2localMatrix = rotation * translation;
}

void mark_domains(CDT &ct, Face_handle start, int index,
                  std::list<CDT::Edge> &border) {
  if (start->info().nesting_level != -1) {
    return;
  }
  std::list<Face_handle> queue;
  queue.push_back(start);
  while (!queue.empty()) {
    Face_handle fh = queue.front();
    queue.pop_front();
    if (fh->info().nesting_level == -1) {
      fh->info().nesting_level = index;
      for (int i = 0; i < 3; i++) {
        CDT::Edge e(fh, i);
        Face_handle n = fh->neighbor(i);
        if (n->info().nesting_level == -1) {
          if (ct.is_constrained(e))
            border.push_back(e);
          else
            queue.push_back(n);
        }
      }
    }
  }
}

void mark_domains(CDT &cdt) {
  for (CDT::Face_handle f : cdt.all_face_handles()) {
    f->info().nesting_level = -1;
  }
  std::list<CDT::Edge> border;
  mark_domains(cdt, cdt.infinite_face(), 0, border);
  while (!border.empty()) {
    CDT::Edge e = border.front();
    border.pop_front();
    Face_handle n = e.first->neighbor(e.second);
    if (n->info().nesting_level == -1) {
      mark_domains(cdt, n, e.first->info().nesting_level + 1, border);
    }
  }
}

void CreateRoof() {
  Polygon_2 polygon1;

  for (const auto &p : simplePolygon) {
    polygon1.push_back(Point(p.x(), p.y()));
  }

  //将多边形插入受约束的三角剖分
  CDT cdt;
  cdt.insert_constraint(polygon1.vertices_begin(), polygon1.vertices_end());

  //标记由多边形界定的域内的面
  mark_domains(cdt);

  for (Face_handle f : cdt.finite_face_handles()) {
    if (f->info().in_domain()) {
      for (int i = 0; i < 3; i++) {
        Eigen::Vector4d xyz(f->vertex(i)->point().x(),
                            f->vertex(i)->point().y(), wallHeight, 1);
        Blh2Xyz(xyz.x(), xyz.y(), xyz.z());
        xyz = wolrd2localMatrix * xyz;
        vertexPositions.emplace_back(xyz.y(), xyz.z(), xyz.x());
      }
    }
  }
}

void CreateWalls() {
  for (size_t pi = 0; pi < simplePolygon.size() - 1; ++pi) {
    Eigen::Vector4d points[4] = {
        {simplePolygon[pi].x(), simplePolygon[pi].y(), wallHeight, 1},
        {simplePolygon[pi + 1].x(), simplePolygon[pi + 1].y(), wallHeight, 1},
        {simplePolygon[pi].x(), simplePolygon[pi].y(), 0, 1},
        {simplePolygon[pi + 1].x(), simplePolygon[pi + 1].y(), 0, 1},
    };

    for (int i = 0; i < 4; i++) {
      Blh2Xyz(points[i].x(), points[i].y(), points[i].z());
      points[i] = wolrd2localMatrix * points[i];
    }

    vertexPositions.emplace_back(points[1].y(), points[1].z(), points[1].x());
    vertexPositions.emplace_back(points[2].y(), points[2].z(), points[2].x());
    vertexPositions.emplace_back(points[0].y(), points[0].z(), points[0].x());

    vertexPositions.emplace_back(points[3].y(), points[3].z(), points[3].x());
    vertexPositions.emplace_back(points[2].y(), points[2].z(), points[2].x());
    vertexPositions.emplace_back(points[1].y(), points[1].z(), points[1].x());
  }
}

void WriteGltf() {
  Assimp::Exporter exporter;

  aiMesh *mesh = new aiMesh();

  mesh->mPrimitiveTypes = aiPrimitiveType::aiPrimitiveType_TRIANGLE;

  mesh->mNumVertices = vertexPositions.size();
  mesh->mVertices = new aiVector3D[mesh->mNumVertices];
  std::copy(vertexPositions.begin(), vertexPositions.end(), mesh->mVertices);
  // cout << mesh->mNumVertices << endl;
  for (int i = 0; i < mesh->mNumVertices; i++) {
    // printf("%lf\t%lf\t%lf\n", mesh->mVertices[i].x, mesh->mVertices[i].y,
    // mesh->mVertices[i].z);
  }

  mesh->mNumFaces = mesh->mNumVertices / 3;
  mesh->mFaces = new aiFace[mesh->mNumFaces];
  for (unsigned int fi = 0; fi < mesh->mNumFaces; ++fi) {
    mesh->mFaces[fi].mNumIndices = 3;
    mesh->mFaces[fi].mIndices = new unsigned int[mesh->mFaces[fi].mNumIndices];
    mesh->mFaces[fi].mIndices[0] = fi * 3;
    mesh->mFaces[fi].mIndices[1] = fi * 3 + 1;
    mesh->mFaces[fi].mIndices[2] = fi * 3 + 2;
  }

  mesh->mMaterialIndex = 0;

  aiScene scene;
  scene.mNumMeshes = 1;
  scene.mMeshes = new aiMesh *[1];
  scene.mMeshes[0] = mesh;

  scene.mRootNode = new aiNode();
  scene.mRootNode->mNumMeshes = 1;
  scene.mRootNode->mMeshes = new unsigned int[scene.mRootNode->mNumMeshes];
  scene.mRootNode->mMeshes[0] = 0;

  aiMaterial *material = new aiMaterial();
  int mode = aiShadingMode::aiShadingMode_PBR_BRDF;
  material->AddProperty(&mode, 1, AI_MATKEY_SHADING_MODEL);
  aiColor4D baseColor(1.000f, 0.766f, 0.336f, 1.0f);
  material->AddProperty(&baseColor, 1, AI_MATKEY_BASE_COLOR);
  float m = 0.5f;
  material->AddProperty(&m, 1, AI_MATKEY_METALLIC_FACTOR);
  float r = 0.1f;
  material->AddProperty(&r, 1, AI_MATKEY_ROUGHNESS_FACTOR);

  scene.mNumMaterials = 1;
  scene.mMaterials = new aiMaterial *[scene.mNumMaterials];
  scene.mMaterials[0] = material;

  string dstFile = getenv("GISBasic");
  dstFile = dstFile + "/../Code/JavaScriptWork/wh.gltf";

  exporter.Export(&scene, "gltf2", dstFile, aiProcess_GenNormals);
}

int main() {
  //获取建筑物矢量面
  ReadShp();

  //计算转换参数
  CalEcef2EnuMatrix();

  //矢量面三角剖分作为屋顶
  CreateRoof();

  //根据矢量面生成墙面
  CreateWalls();

  //写出
  WriteGltf();

  return 0;
}
