// Example6Resample.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <algorithm>
#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

Mat srcImage;
Mat myImage;

void ZoomIn() {
  int dstCols = (int)((srcImage.cols * 2.3) + 0.5);
  int dstRows = (int)((srcImage.rows * 2.3) + 0.5);

  double rX = (double)srcImage.cols / dstCols;
  double rY = (double)srcImage.rows / dstRows;

  //自定义方法
  myImage.create(dstRows, dstCols, CV_8UC3);
  for (int i = 0; i < dstRows; ++i) {
    for (int j = 0; j < dstCols; ++j) {
      double srcX = (j + 0.5) * rX - 0.5;
      double srcY = (i + 0.5) * rY - 0.5;

      // printf("%lf,%lf\t", srcX, srcY);

      int left = floor(srcX);
      int top = floor(srcY);
      left = std::max(std::min(left, srcImage.cols - 1), 0);
      top = std::max(std::min(top, srcImage.rows - 1), 0);
      int right = std::max(std::min(left + 1, srcImage.cols - 1), 0);
      int bottom = std::max(std::min(top + 1, srcImage.rows - 1), 0);

      double u = srcX - left;
      double v = srcY - top;


      for (int b = 0; b < 3; b++) {
        int n00 = top * srcImage.cols * 3 + left * 3 + b;
        int n10 = top * srcImage.cols * 3 + right * 3 + b;
        int n01 = bottom * srcImage.cols * 3 + left * 3 + b;
        int n11 = bottom * srcImage.cols * 3 + right * 3 + b;

        double vaule = (1 - u) * (1 - v) * srcImage.data[n00] +
                       u * (1 - v) * srcImage.data[n10] +
                       (1 - u) * v * srcImage.data[n01] +
                       u * v * srcImage.data[n11];

        int m = i * dstCols * 3 + j * 3 + b;
        myImage.data[m] = saturate_cast<uchar>(vaule + 0.5);
      }

      //
    }
  }

  imwrite("D:/2.jpg", myImage);
}

int main() {
  //打开数据集对象
  string baseDir = getenv("GISBasic");
  string srcFilePath = baseDir + "/../Data/Raster/image2.jpg";

  srcImage = imread(srcFilePath);
  // imshow("原图像", srcImage);

  ZoomIn();

  Mat cvImage;
  resize(srcImage, cvImage, Size(0, 0), 2.3, 2.3, INTER_LINEAR);
  // imwrite("D:/2.jpg", dstImage2);

  //比较两者的结果
  Mat c;
  cv::compare(myImage, cvImage, c, CMP_EQ);

  imshow("比较结果", c);

  cv::waitKey();

  return 0;
}
