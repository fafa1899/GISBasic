import os
import sys
import cv2
import numpy as np

def detect_harris_corners(img_src, alpha):
    """手动实现 Harris 角点检测"""
    # 1. 将图像转换为 float64 类型 (对应 C++ 的 CV_64F)
    gray = img_src.astype(np.float64)

    # 2. 计算图像在 X, Y 方向的梯度
    # 对应 C++: Mat_<double>(1, 3) << -1, 0, 1
    x_kernel = np.array([[-1, 0, 1]], dtype=np.float64)
    y_kernel = x_kernel.T

    Ix = cv2.filter2D(gray, -1, x_kernel)
    Iy = cv2.filter2D(gray, -1, y_kernel)

    # 3. 计算两个方向梯度的乘积
    Ix2 = Ix * Ix
    Iy2 = Iy * Iy
    Ixy = Ix * Iy

    # 4. 对乘积进行高斯滤波 (对应 C++ 的 getGaussianKernel(7, 1))
    # 在 Python 中可以直接传入 (ksize, ksize) 和 sigma
    gauss_kernel_size = 7
    gauss_sigma = 1
    Ix2 = cv2.GaussianBlur(Ix2, (gauss_kernel_size, gauss_kernel_size), gauss_sigma)
    Iy2 = cv2.GaussianBlur(Iy2, (gauss_kernel_size, gauss_kernel_size), gauss_sigma)
    Ixy = cv2.GaussianBlur(Ixy, (gauss_kernel_size, gauss_kernel_size), gauss_sigma)

    # 5. 计算每个像素的 Harris 响应值 R
    # 对应 C++ 中的双重 for 循环计算 det_m 和 trace_m
    # R = det(M) - alpha * (trace(M))^2
    det_m = Ix2 * Iy2 - Ixy * Ixy
    trace_m = Ix2 + Iy2
    corner_strength = det_m - alpha * trace_m * trace_m

    # 6. 非最大值抑制 (NMS)
    max_strength = corner_strength.max()
    
    # 膨胀操作 (对应 C++: dilate(cornerStrength, dilated, Mat()))
    dilated = cv2.dilate(corner_strength, None)
    
    # 比较保留局部最大值的点 (对应 C++: compare(..., CMP_EQ))
    local_max = (corner_strength == dilated)

    # 7. 阈值处理，得到最终的角点位置
    quality_level = 0.01
    thresh = quality_level * max_strength
    corner_map = (corner_strength > thresh)
    
    # 位与运算，同时满足阈值和局部最大值 (对应 C++: bitwise_and)
    corner_map = cv2.bitwise_and(corner_map.astype(np.uint8), local_max.astype(np.uint8))

    return corner_map

def draw_corner_on_image(image, binary):
    """在角点位置绘制标记"""
    # 遍历二值图像，在非零像素位置画圆
    # 对应 C++ 中通过迭代器遍历 Mat
    rows, cols = binary.shape
    for i in range(rows):
        for j in range(cols):
            if binary[i, j]:
                cv2.circle(image, (j, i), 3, (255, 255, 255), 1)

def main():
    # 获取图像路径
    gis_basic = os.getenv("GISBasicRepo")
    if not gis_basic:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        return 1
    img_path = os.path.join(gis_basic, "Data", "Raster", "image3.png")

    # 从文件中读取成灰度图像
    img = cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)
    if img is None:
        print(f"Can not load image {img_path}")
        return

    # 执行 Harris 角点检测
    alpha = 0.05
    img_dst = detect_harris_corners(img, alpha)

    # 在角点位置绘制标记
    # 注意：绘制时需要将灰度图转为彩色图，否则白色的圆 (255,255,255) 在单通道图上看不出颜色差异
    img_color = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
    draw_corner_on_image(img_color, img_dst)

    # 显示结果
    cv2.imshow("Harris Corner Detection", img_color)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()