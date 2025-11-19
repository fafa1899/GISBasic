// Example8Gradient.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <opencv2\opencv.hpp>

using namespace cv;
using namespace std;

int main() {
  //从文件中读取成灰度图像

  string envDir = getenv("GISBasic");
  string imgPath = envDir + "/../Data/Raster/image2.png";

  Mat img = imread(imgPath, IMREAD_GRAYSCALE);
  if (img.empty()) {
    fprintf(stderr, "Can not load image %s\n", imgPath.c_str());
    return -1;
  }

  // OpenCV函数进行一维卷积（梯度图）
  Mat xKernel = (Mat_<double>(1, 3) << -1, 0, 1);  //卷积算子
  Mat Ix;
  filter2D(img, Ix, -1, xKernel);

  //自建算法进行一维卷积（梯度图）
  Mat Ixx;
  Ixx.create(img.cols, img.rows, CV_8UC1);
  double xk[3] = {-1, 0, 1};  //卷积算子
  for (int i = 0; i < img.rows; ++i) {
    for (int j = 0; j < img.cols; ++j) {
      // img.at<uchar>(i, j) = 255;
      uchar b[3] = {0};
      b[0] = (j == 0 ? 0 : img.at<uchar>(i, j - 1));
      b[1] = img.at<uchar>(i, j);
      b[2] = (j == img.cols - 1 ? 0 : img.at<uchar>(i, j + 1));

      double value = xk[0] * b[0] + xk[1] * b[1] + xk[2] * b[2];
      value = (std::min)(std::max(value, 0.0), 255.0);
      Ixx.at<uchar>(i, j) = (uchar)value;
    }
  }

  //比较两者的结果
  Mat c;
  compare(Ix, Ixx, c, CMP_EQ);

  //显示图像
  imshow("原始", img);
  imshow("梯度图(CV)", Ix);
  imshow("梯度图(MY)", Ixx);
  imshow("比较结果", c);

  waitKey();
  return 0;
}