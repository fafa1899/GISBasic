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

    # 从文件中读取成灰度图像
    img = cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)
    if img is None:
        print(f"Can not load image {img_path}")
        return

    # --- OpenCV 方法 ---
    # 对应 C++: erode(img, eroded_cv, Mat());
    # Mat() 默认创建一个 3x3 的矩形结构元素
    kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (3, 3))
    eroded_cv = cv2.erode(img, kernel)

    # --- 自定义方法 ---
    # 对应 C++: eroded_my.create(img.cols, img.rows, CV_8UC1);
    # Python 中使用 np.zeros 或 np.empty 创建数组，注意形状是 (height, width)
    eroded_my = np.zeros_like(img)
    
    rows, cols = img.shape
    
    # 对应 C++ 的双重 for 循环
    for i in range(rows):
        for j in range(cols):
            min_v = 255
            # max_v = 0

            # 遍历周围像素 (3x3 邻域)
            # 对应 C++: for (int yi = i - 1; yi <= i + 1; yi++)
            for yi in range(i - 1, i + 2):
                # 对应 C++: for (int xi = j - 1; xi <= j + 1; xi++)
                for xi in range(j - 1, j + 2):
                    # 边界检查
                    if xi < 0 or xi >= cols or yi < 0 or yi >= rows:
                        continue
                    
                    # 对应 C++: minV = (std::min<uchar>)(minV, img.at<uchar>(yi, xi));
                    min_v = min(min_v, img[yi, xi])
                    # max_v = max(max_v, img[yi, xi])
            
            # 对应 C++: eroded_my.at<uchar>(i, j) = minV;
            eroded_my[i, j] = min_v

    # --- 比较两者的结果 ---
    # 对应 C++: compare(eroded_cv, eroded_my, c, CMP_EQ);
    # NumPy 数组可以直接比较，生成布尔数组，astype(np.uint8) * 255 转为二值图像
    c = (eroded_cv == eroded_my).astype(np.uint8) * 255

    # --- 显示结果 ---
    cv2.imshow("Original", img)
    cv2.imshow("Eroded (OpenCV)", eroded_cv)
    cv2.imshow("Eroded (Custom)", eroded_my)
    cv2.imshow("Comparison", c)

    cv2.waitKey(0)
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()