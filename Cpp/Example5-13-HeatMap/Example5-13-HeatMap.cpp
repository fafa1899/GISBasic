// Example13HeatMap.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <algorithm>
#include <array>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

struct HPoint {
  int x;
  int y;
  int value;
};

struct HRect {
  int left;
  int top;
  int right;
  int bottom;
};

int width = 512;   //热力图宽
int height = 512;  //热力图高
int reach = 25;    //影响范围
int valueRange = 100;

vector<HPoint> heatPoints;  //热力点
vector<HRect> heatRects;    //热力范围

array<array<uchar, 3>, 256> bGRTable;  //颜色映射表

void GetHeatPoint() {
  int num = 100;
  heatPoints.resize(num);
  heatRects.resize(num);

  for (int i = 0; i < num; i++) {
    heatPoints[i].x = rand() % width;
    heatPoints[i].y = rand() % height;
    heatPoints[i].value = rand() % valueRange;

    heatRects[i].left = (std::max)(heatPoints[i].x - reach, 0);
    heatRects[i].top = (std::max)(heatPoints[i].y - reach, 0);
    heatRects[i].right = (std::min)(heatPoints[i].x + reach, width - 1);
    heatRects[i].bottom = (std::min)(heatPoints[i].y + reach, height - 1);
  }
}

//生成渐变色
void Gradient(array<uchar, 3> &start, array<uchar, 3> &end,
              vector<array<uchar, 3>> &RGBList) {
  array<float, 3> dBgr;
  for (int i = 0; i < 3; i++) {
    dBgr[i] = (float)(end[i] - start[i]) / (RGBList.size() - 1);
  }

  for (size_t i = 0; i < RGBList.size(); i++) {
    for (int j = 0; j < 3; j++) {
      RGBList[i][j] = (uchar)(start[j] + dBgr[j] * i);
    }
  }
}

void InitAlpha2BGRTable() {
  array<double, 7> boundaryValue = {0.2, 0.3, 0.4, 0.6, 0.8, 0.9, 1.0};
  array<array<uchar, 3>, 7> boundaryBGR;
  boundaryBGR[0] = {255, 0, 0};
  boundaryBGR[1] = {231, 111, 43};
  boundaryBGR[2] = {241, 192, 2};
  boundaryBGR[3] = {148, 222, 44};
  boundaryBGR[4] = {83, 237, 254};
  boundaryBGR[5] = {50, 118, 253};
  boundaryBGR[6] = {28, 64, 255};

  double lastValue = 0;
  array<uchar, 3> lastRGB = {0, 0, 0};
  vector<array<uchar, 3>> RGBList;
  int sumNum = 0;
  for (size_t i = 0; i < boundaryValue.size(); i++) {
    int num = 0;
    if (i == boundaryValue.size() - 1) {
      num = 256 - sumNum;
    } else {
      num = (int)((boundaryValue[i] - lastValue) * 256 + 0.5);
    }

    RGBList.resize(num);
    Gradient(lastRGB, boundaryBGR[i], RGBList);

    for (int i = 0; i < num; i++) {
      bGRTable[i + sumNum] = RGBList[i];
    }
    sumNum = sumNum + num;

    lastValue = boundaryValue[i];
    lastRGB = boundaryBGR[i];
  }
}

int main() {
  GetHeatPoint();

  InitAlpha2BGRTable();

  Mat img(height, width, CV_8UC4);
  int nBand = 4;

  uchar *data = img.data;
  size_t dataLength = (size_t)width * height * nBand;

  for (size_t i = 0; i < heatPoints.size(); i++) {
    //权值因子
    float ratio = (float)heatPoints[i].value / valueRange;

    //遍历热力点范围
    for (int hi = heatRects[i].top; hi <= heatRects[i].bottom; hi++) {
      for (int wi = heatRects[i].left; wi <= heatRects[i].right; wi++) {
        //判断是否在热力圈范围
        float length =
            sqrt((float)(wi - heatPoints[i].x) * (wi - heatPoints[i].x) +
                 (hi - heatPoints[i].y) * (hi - heatPoints[i].y));
        if (length <= reach) {
          float alpha = ((reach - length) / reach) * ratio;

          //计算Alpha
          size_t m = (size_t)width * nBand * hi + wi * nBand;
          float newAlpha = data[m + 3] / 255.0f + alpha;
          newAlpha = std::min(std::max(newAlpha * 255, 0.0f), 255.0f);
          data[m + 3] = (uchar)(newAlpha);

          //颜色映射
          for (int bi = 0; bi < 3; bi++) {
            data[m + bi] = bGRTable[data[m + 3]][bi];
          }
        }
      }
    }
  }

  imshow("热力图", img);

  string envDir = getenv("GISBasic");
  string imgPath = envDir + "/../Data/Raster/dst.png";

  imwrite(imgPath, img);

  waitKey();
  return 0;
}