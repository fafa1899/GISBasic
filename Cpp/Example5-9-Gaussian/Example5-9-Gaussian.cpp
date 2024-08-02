// Example9Gaussian.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <opencv2\opencv.hpp>

using namespace cv;
using namespace std;

int main() {
  string envDir = getenv("GISBasic");
  string imgPath = envDir + "/../Data/Raster/image2.png";

  Mat img = imread(imgPath, IMREAD_GRAYSCALE);
  if (img.empty()) {
    fprintf(stderr, "Can not load image %s\n", imgPath.c_str());
    return -1;
  }

  //直接高斯滤波
  Mat dst1;
  GaussianBlur(img, dst1, Size(3, 3), 1, 1);

  //自定义高斯滤波器
  Mat kernelX = getGaussianKernel(3, 1);
  Mat kernelY = getGaussianKernel(3, 1);
  Mat G = kernelX * kernelY.t();
  Mat dst2;

  filter2D(img, dst2, -1, G);

  //比较两者的结果
  Mat c = dst1 - dst2;
  // Mat c;
  // compare(dst1, dst2, c, CMP_EQ);

  //
  imshow("原始", img);
  imshow("高斯滤波1", dst1);
  imshow("高斯滤波2", dst2);
  imshow("比较结果", c);

  waitKey();

  return 0;
}
