// Example10Dilated.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <algorithm>
#include <iostream>
#include <opencv2\opencv.hpp>

using namespace cv;
using namespace std;

int main() {
  string envDir = getenv("GISBasic");
  string imgPath = envDir + "/../Data/Raster/image2.png";

  //从文件中读取成灰度图像
  Mat img = imread(imgPath, IMREAD_GRAYSCALE);
  if (img.empty()) {
    fprintf(stderr, "Can not load image %s\n", imgPath.c_str());
    return -1;
  }

  // OpenCV方法
  Mat dilated_cv;
  dilate(img, dilated_cv, Mat());

  //自定义方法
  Mat dilated_my;
  dilated_my.create(img.rows, img.cols, CV_8UC1);
  for (int i = 0; i < img.rows; ++i) {
    for (int j = 0; j < img.cols; ++j) {
      // uchar minV = 255;
      uchar maxV = 0;

      //遍历周围最大像素值
      for (int yi = i - 1; yi <= i + 1; yi++) {
        for (int xi = j - 1; xi <= j + 1; xi++) {
          if (xi < 0 || xi >= img.cols || yi < 0 || yi >= img.rows) {
            continue;
          }
          // minV = (std::min<uchar>)(minV, img.at<uchar>(yi, xi));
          maxV = (std::max<uchar>)(maxV, img.at<uchar>(yi, xi));
        }
      }
      dilated_my.at<uchar>(i, j) = maxV;
    }
  }

  //比较两者的结果
  Mat c;
  compare(dilated_cv, dilated_my, c, CMP_EQ);

  //显示
  imshow("原始", img);
  imshow("膨胀_cv", dilated_cv);
  imshow("膨胀_my", dilated_my);
  imshow("比较结果", c);

  waitKey();

  return 0;
}
