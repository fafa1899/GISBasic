import os
import cv2
import numpy as np

# --- 1. 加载图像 ---
gis_basic = os.getenv("GISBasicRepo")
img_path = os.path.join(gis_basic, "Data", "Raster", "image2.png")

# 以灰度模式读取图像
img = cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)

if img is None:
    print(f"无法加载图像，请检查路径: {img_path}")
    exit()

# --- 2. OpenCV 内置方法 ---
# 使用默认的结构元素（3x3 的矩形核）进行膨胀
dilated_cv = cv2.dilate(img, None)

# --- 3. 自定义方法 ---
# 创建一个与原图尺寸、类型相同的空图像用于存储结果
dilated_my = np.zeros_like(img)
height, width = img.shape

# 遍历图像的每一个像素（排除边缘，因为自定义实现未处理边界填充）
for i in range(1, height - 1):
    for j in range(1, width - 1):
        # 提取当前像素的 3x3 邻域
        # 这比双重循环取最大值更高效
        neighborhood = img[i-1:i+2, j-1:j+2]
        # 找到邻域内的最大像素值
        max_val = np.max(neighborhood)
        # 将最大值赋给输出图像的对应位置
        dilated_my[i, j] = max_val

# --- 4. 比较两者的结果 ---
# 使用 cv2.compare 比较两个膨胀后的图像是否完全相同
# cv2.CMP_EQ 表示“等于”比较
c = cv2.compare(dilated_cv, dilated_my, cv2.CMP_EQ)

# --- 5. 显示结果 ---
cv2.imshow("Original", img)
cv2.imshow("OpenCV", dilated_cv)
cv2.imshow("Custom", dilated_my)
cv2.imshow("Comparison", c)

print("按任意键关闭所有窗口...")
cv2.waitKey(0)
cv2.destroyAllWindows()