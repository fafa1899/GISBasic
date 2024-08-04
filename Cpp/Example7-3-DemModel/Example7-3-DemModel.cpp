#include <gdal_priv.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace std;
using namespace nlohmann;

size_t pointNum = 0;
size_t binBufNum = 0;
size_t indicesNum = 0;

void CreateBinFile() {
  string workDir = getenv("GISBasic");
  string demPath = workDir + "/../Data/Model/dem.tif";

  GDALDataset *img = (GDALDataset *)GDALOpen(demPath.c_str(), GA_ReadOnly);
  if (!img) {
    printf("Can't Open Image!");
    return;
  }
  int bufWidth = img->GetRasterXSize();   //图像宽度
  int bufHeight = img->GetRasterYSize();  //图像高度
  int bandNum = img->GetRasterCount();    //波段数
  if (bandNum != 1) {
    printf("DEM波段数不为1");
    return;
  }
  int depth = GDALGetDataTypeSize(img->GetRasterBand(1)->GetRasterDataType()) /
              8;  //图像深度

  //获取地理坐标信息
  double padfTransform[6];
  if (img->GetGeoTransform(padfTransform) == CE_Failure) {
    printf("获取仿射变换参数失败");
    return;
  }

  double startX = padfTransform[0];
  double dX = padfTransform[1];
  double startY = padfTransform[3];
  double dY = padfTransform[5];

  //申请buf
  size_t imgBufNum = (size_t)bufWidth * bufHeight * bandNum;
  float *imgBuf = new float[imgBufNum];

  //读取
  img->RasterIO(GF_Read, 0, 0, bufWidth, bufHeight, imgBuf, bufWidth, bufHeight,
                GDT_Float32, bandNum, nullptr, bandNum * depth,
                bufWidth * bandNum * depth, depth);

  pointNum = (size_t)bufWidth * bufHeight;
  size_t position_texture_num = pointNum * 5;
  float *position_texture = new float[position_texture_num];

  for (int yi = 0; yi < bufHeight; yi++) {
    for (int xi = 0; xi < bufWidth; xi++) {
      size_t n = (size_t)(bufWidth * 5) * yi + 5 * xi;
      position_texture[n] = dX * xi;
      position_texture[n + 1] = dY * yi;
      size_t m = (size_t)(bufWidth * bandNum) * yi + bandNum * xi;
      position_texture[n + 2] = imgBuf[m];
      position_texture[n + 3] = float(xi) / (bufWidth - 1);
      position_texture[n + 4] = float(yi) / (bufHeight - 1);
    }
  }

  //释放
  delete[] imgBuf;
  imgBuf = nullptr;

  string binPath = workDir + "/../Data/Model/new.bin";
  ofstream binFile(binPath, std::ios::binary);

  binFile.write((char *)position_texture, position_texture_num * sizeof(float));

  size_t vertexBufNum = position_texture_num * sizeof(float);
  binBufNum = binBufNum + vertexBufNum;

  int mod = vertexBufNum % sizeof(uint16_t);
  if (mod != 0) {
    int spaceNum = sizeof(float) - mod;
    char *space = new char[spaceNum];
    binBufNum = binBufNum + sizeof(char) * spaceNum;
    memset(space, 0, sizeof(char) * spaceNum);
    binFile.write(space, sizeof(char) * spaceNum);
    delete[] space;
    space = nullptr;
  }

  indicesNum = (size_t)(bufWidth - 1) * (bufHeight - 1) * 2 * 3;
  uint16_t *indices = new uint16_t[indicesNum];

  for (int yi = 0; yi < bufHeight - 1; yi++) {
    for (int xi = 0; xi < bufWidth - 1; xi++) {
      uint16_t m00 = (uint16_t)(bufWidth * yi + xi);
      uint16_t m01 = (uint16_t)(bufWidth * (yi + 1) + xi);
      uint16_t m11 = (uint16_t)(bufWidth * (yi + 1) + xi + 1);
      uint16_t m10 = (uint16_t)(bufWidth * yi + xi + 1);

      size_t n = (size_t)(bufWidth - 1) * yi + xi;
      indices[n * 6] = m00;
      indices[n * 6 + 1] = m01;
      indices[n * 6 + 2] = m11;
      indices[n * 6 + 3] = m11;
      indices[n * 6 + 4] = m10;
      indices[n * 6 + 5] = m00;
    }
  }

  binFile.write((char *)indices, sizeof(uint16_t) * indicesNum);
  binBufNum = binBufNum + sizeof(uint16_t) * indicesNum;

  delete[] position_texture;
  position_texture = nullptr;

  delete[] indices;
  indices = nullptr;
}

int main() {
  GDALAllRegister();
  CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");  //支持中文路径

  //设置Proj数据
  std::string projDataPath = getenv("GISBasic");
  projDataPath += "/share/proj";
  CPLSetConfigOption("PROJ_LIB", projDataPath.c_str());

  ordered_json gltf;

  gltf["asset"] = {{"generator", "CL"}, {"version", "2.0"}};

  gltf["scene"] = 0;
  gltf["scenes"] = {{{"nodes", {0}}}};

  gltf["nodes"] = {{{"mesh", 0}}};

  ordered_json positionJson;
  positionJson["POSITION"] = 1;
  positionJson["TEXCOORD_0"] = 2;

  ordered_json primitivesJson;
  primitivesJson = {
      {{"attributes", positionJson}, {"indices", 0}, {"material", 0}}};

  gltf["meshes"] = {{{"primitives", primitivesJson}}};

  ordered_json pbrJson;
  pbrJson["baseColorTexture"]["index"] = 0;

  gltf["materials"] = {{{"pbrMetallicRoughness", pbrJson}}};

  CreateBinFile();

  gltf["textures"] = {{{"sampler", 0}, {"source", 0}}};

  gltf["images"] = {{{"uri", "tex.jpg"}}};

  gltf["samplers"] = {{{"magFilter", 9729},
                       {"minFilter", 9987},
                       {"wrapS", 33648},
                       {"wrapT", 33648}}};

  gltf["buffers"] = {{{"uri", "new.bin"}, {"byteLength", binBufNum}}};

  ordered_json indicesBufferJson;
  indicesBufferJson["buffer"] = 0;
  indicesBufferJson["byteOffset"] = pointNum * 5 * 4;
  indicesBufferJson["byteLength"] = indicesNum * 2;
  indicesBufferJson["target"] = 34963;

  ordered_json positionBufferJson;
  positionBufferJson["buffer"] = 0;
  positionBufferJson["byteStride"] = sizeof(float) * 5;
  positionBufferJson["byteOffset"] = 0;
  positionBufferJson["byteLength"] = pointNum * 5 * 4;
  positionBufferJson["target"] = 34962;

  gltf["bufferViews"] = {indicesBufferJson, positionBufferJson};

  ordered_json indicesAccessors;
  indicesAccessors["bufferView"] = 0;
  indicesAccessors["byteOffset"] = 0;
  indicesAccessors["componentType"] = 5123;
  indicesAccessors["count"] = indicesNum;
  indicesAccessors["type"] = "SCALAR";
  indicesAccessors["max"] = {18719};
  indicesAccessors["min"] = {0};

  ordered_json positionAccessors;
  positionAccessors["bufferView"] = 1;
  positionAccessors["byteOffset"] = 0;
  positionAccessors["componentType"] = 5126;
  positionAccessors["count"] = pointNum;
  positionAccessors["type"] = "VEC3";
  positionAccessors["max"] = {770, 0.0, 1261.151611328125};
  positionAccessors["min"] = {0.0, -2390, 733.5555419921875};

  ordered_json textureAccessors;
  textureAccessors["bufferView"] = 1;
  textureAccessors["byteOffset"] = sizeof(float) * 3;
  textureAccessors["componentType"] = 5126;
  textureAccessors["count"] = pointNum;
  textureAccessors["type"] = "VEC2";
  textureAccessors["max"] = {1, 1};
  textureAccessors["min"] = {0, 0};

  gltf["accessors"] = {indicesAccessors, positionAccessors, textureAccessors};

  string workDir = getenv("GISBasic");
  string jsonFile = workDir + "/../Data/Model/new.gltf";
  ofstream binFile(jsonFile, std::ios::binary);

  std::ofstream outFile(jsonFile);
  outFile << std::setw(4) << gltf << std::endl;
}
