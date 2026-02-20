import os
import sys
import cv2
import numpy as np

def compute_gradient_manual(img, kernel):
    """
    手动实现一维卷积（梯度图）
    :param img: 输入灰度图像
    :param kernel: 卷积核
    :return: 梯度图
    """
    rows, cols = img.shape
    gradient_img = np.zeros_like(img, dtype=np.uint8)
    
    for i in range(rows):
        for j in range(cols):
            # 获取相邻像素值，边界情况用0填充
            b = [0] * len(kernel)
            for k in range(len(kernel)):
                col_index = j + (k - len(kernel) // 2)
                if 0 <= col_index < cols:
                    b[k] = img[i, col_index]
            
            # 计算卷积结果
            value = sum([kernel[k] * b[k] for k in range(len(kernel))])
            value = max(0, min(int(value), 255))
            gradient_img[i, j] = value
            
    return gradient_img


def main():
    # 设置环境变量 GISBasic
    gis_basic = os.getenv("GISBasicRepo")
    if not gis_basic:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        return 1

    img_path = os.path.join(gis_basic, "Data", "Raster", "image2.png")

    # 从文件中读取成灰度图像
    img = cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)
    if img is None:
        print(f"无法加载图像 {img_path}", file=sys.stderr)
        return -1

    # OpenCV 函数进行一维卷积（梯度图）
    x_kernel = np.array([-1, 0, 1], dtype=np.float32).reshape(1, -1)
    ix = cv2.filter2D(img, -1, x_kernel)

    # 自建算法进行一维卷积（梯度图）
    ixx = compute_gradient_manual(img, x_kernel.flatten())

    # 比较两者的结果
    comparison = cv2.compare(ix, ixx, cv2.CMP_EQ)

    # 显示图像
    cv2.imshow("Original", img)
    cv2.imshow("Gradient_OpenCV", ix)
    cv2.imshow("Gradient_Manual", ixx)
    cv2.imshow("Comparison_EQ", comparison)

    cv2.waitKey(0)
    cv2.destroyAllWindows()

    return 0


if __name__ == "__main__":
    sys.exit(main())