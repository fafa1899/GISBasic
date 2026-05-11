import os
import sys
import cv2
import numpy as np

def main():
    # 获取图像路径
    gis_basic = os.getenv("GISBasicRepo")
    if not gis_basic:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        return 1

    img_path = os.path.join(gis_basic, "Data", "Raster", "image2.png")

    # 读取图像
    img = cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)
    if img is None:
        print(f"Can not load image {img_path}")
        return

    # --- 方法一：直接使用高斯滤波函数 ---
    # 对应 C++: GaussianBlur(img, dst1, Size(3, 3), 1, 1);
    # 参数: (源图像, 核大小(ksize), X方向标准差(sigmaX), Y方向标准差(sigmaY))
    dst1 = cv2.GaussianBlur(img, (3, 3), 1, 1)

    # --- 方法二：自定义高斯滤波器 ---
    # 1. 生成高斯核
    # 对应 C++: getGaussianKernel(3, 1)
    # Python中生成的核是二维数组，形状为 (3, 1)
    kernel_x = cv2.getGaussianKernel(3, 1)
    kernel_y = cv2.getGaussianKernel(3, 1)
    
    # 2. 计算二维高斯核 G = kernelX * kernelY.t()
    # 对应 C++: Mat G = kernelX * kernelY.t();
    # 在Python中，使用NumPy的矩阵乘法或外积来实现
    # kernel_x.T 是转置，形状为 (1, 3)
    # np.dot(kernel_x, kernel_y.T) 或 kernel_x @ kernel_y.T 得到 (3, 3) 的二维核
    G = np.dot(kernel_x, kernel_y.T)

    # 3. 使用 filter2D 进行卷积
    # 对应 C++: filter2D(img, dst2, -1, G);
    # 参数: (源图像, 目标深度(-1表示与源图像相同), 卷积核)
    dst2 = cv2.filter2D(img, -1, G)

    # --- 比较两者的结果 ---
    # 对应 C++: Mat c = dst1 - dst2;
    c = cv2.absdiff(dst1, dst2)

    # --- 显示结果 ---
    # 对应 C++: imshow(...)
    cv2.imshow("Original", img)
    cv2.imshow("Gaussian Blur 1", dst1)
    cv2.imshow("Gaussian Blur 2", dst2)
    cv2.imshow("Comparison", c)

    # 等待按键，对应 C++: waitKey();
    cv2.waitKey(0)
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()