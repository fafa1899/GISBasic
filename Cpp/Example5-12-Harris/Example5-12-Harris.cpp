// Example12Harris.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <algorithm>
#include <iostream>
#include <opencv2\opencv.hpp>

using namespace cv;
using namespace std;

void detectHarrisCorners(const Mat& imgSrc, Mat& imgDst, double alpha) {
  //
  Mat gray;
  imgSrc.convertTo(gray, CV_64F);

  //计算图像I(x,y)在X，Y方向的梯度
  Mat xKernel = (Mat_<double>(1, 3) << -1, 0, 1);
  Mat yKernel = xKernel.t();

  Mat Ix, Iy;
  filter2D(gray, Ix, CV_64F, xKernel);
  filter2D(gray, Iy, CV_64F, yKernel);

  //计算图像两个方向梯度的乘积。
  Mat Ix2, Iy2, Ixy;
  Ix2 = Ix.mul(Ix);
  Iy2 = Iy.mul(Iy);
  Ixy = Ix.mul(Iy);

  //对Ix2、Iy2和Ixy进行高斯滤波，生成矩阵M的元素A、B和C。
  Mat gaussKernel = getGaussianKernel(7, 1);
  filter2D(Ix2, Ix2, CV_64F, gaussKernel);
  filter2D(Iy2, Iy2, CV_64F, gaussKernel);
  filter2D(Ixy, Ixy, CV_64F, gaussKernel);

  //根据公式计算每个像素的Harris响应值R，得到图像对应的响应值矩阵。
  Mat cornerStrength(gray.size(), gray.type());
  for (int i = 0; i < gray.rows; i++) {
    for (int j = 0; j < gray.cols; j++) {
      double det_m = Ix2.at<double>(i, j) * Iy2.at<double>(i, j) -
                     Ixy.at<double>(i, j) * Ixy.at<double>(i, j);
      double trace_m = Ix2.at<double>(i, j) + Iy2.at<double>(i, j);
      cornerStrength.at<double>(i, j) = det_m - alpha * trace_m * trace_m;
    }
  }

  //在3×3的邻域内进行非最大值抑制，找到局部最大值点，即为图像中的角点
  double maxStrength;
  minMaxLoc(cornerStrength, NULL, &maxStrength, NULL, NULL);
  Mat dilated;
  Mat localMax;
  dilate(cornerStrength, dilated, Mat());              //膨胀
  compare(cornerStrength, dilated, localMax, CMP_EQ);  //比较保留最大值的点

  //得到角点的位置
  Mat cornerMap;
  double qualityLevel = 0.01;
  double thresh = qualityLevel * maxStrength;
  cornerMap = cornerStrength > thresh;  //小于阈值t的R置为零。
  bitwise_and(cornerMap, localMax, cornerMap);  //位与运算，有0则为0, 全为1则为1

  imgDst = cornerMap.clone();
}

//在角点位置绘制标记
void drawCornerOnImage(Mat& image, const Mat& binary) {
  Mat_<uchar>::const_iterator it = binary.begin<uchar>();
  Mat_<uchar>::const_iterator itd = binary.end<uchar>();
  for (int i = 0; it != itd; it++, i++) {
    if (*it)
      circle(image, Point(i % image.cols, i / image.cols), 3,
             Scalar(255, 255, 255), 1);
  }
}

int main() {
  string envDir = getenv("GISBasic");
  string imgPath = envDir + "/../Data/Raster/image3.png";

  //从文件中读取成灰度图像
  Mat img = imread(imgPath, IMREAD_GRAYSCALE);
  if (img.empty()) {
    fprintf(stderr, "Can not load image %s\n", imgPath.c_str());
    return -1;
  }

  //
  Mat imgDst;
  double alpha = 0.05;
  detectHarrisCorners(img, imgDst, alpha);

  //在角点位置绘制标记
  drawCornerOnImage(img, imgDst);

  //
  imshow("Harris角点检测", img);
  waitKey();

  return 0;
}