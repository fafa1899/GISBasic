import os
import sys
import cv2
import numpy as np
import random
import math

# 全局变量定义
width = 512   # 热力图宽
height = 512  # 热力图高
reach = 25    # 影响范围
value_range = 100

heat_points = []  # 热力点列表
heat_rects = []   # 热力范围列表

# 颜色映射表 (对应 C++ 的 array<array<uchar, 3>, 256>)
bgr_table = np.zeros((256, 3), dtype=np.uint8)

def get_heat_point():
    """生成随机的热力点及其影响范围"""
    global heat_points, heat_rects
    num = 100
    heat_points = []
    heat_rects = []

    for _ in range(num):
        x = random.randint(0, width - 1)
        y = random.randint(0, height - 1)
        value = random.randint(0, value_range - 1)
        
        heat_points.append({'x': x, 'y': y, 'value': value})

        left = max(x - reach, 0)
        top = max(y - reach, 0)
        right = min(x + reach, width - 1)
        bottom = min(y + reach, height - 1)
        heat_rects.append({'left': left, 'top': top, 'right': right, 'bottom': bottom})

def gradient(start, end, num):
    """生成 start 到 end 之间的渐变色列表"""
    rgb_list = []
    # 对应 C++: (float)(end[i] - start[i]) / (RGBList.size() - 1)
    # 注意：当 num 为 1 时，C++ 原代码会出现除以 0 的情况，这里做个保护
    if num <= 1:
        return [start]
        
    d_bgr = [(end[i] - start[i]) / (num - 1) for i in range(3)]
    
    for i in range(num):
        color = [int(start[j] + d_bgr[j] * i) for j in range(3)]
        rgb_list.append(color)
    return rgb_list

def init_alpha2_bgr_table():
    """初始化颜色映射表"""
    global bgr_table
    boundary_value = [0.2, 0.3, 0.4, 0.6, 0.8, 0.9, 1.0]
    # 注意：OpenCV 中颜色通道默认是 BGR 顺序
    boundary_bgr = [
        [255, 0, 0],    # 蓝
        [43, 111, 231], # 橙
        [2, 192, 241],  # 黄
        [44, 222, 148], # 绿
        [254, 237, 83], # 青
        [253, 118, 50], # 浅蓝
        [255, 64, 28]   # 深蓝
    ]

    last_value = 0.0
    last_rgb = [0, 0, 0]
    sum_num = 0

    for i in range(len(boundary_value)):
        if i == len(boundary_value) - 1:
            num = 256 - sum_num
        else:
            # 对应 C++: (int)((boundaryValue[i] - lastValue) * 256 + 0.5)
            num = int((boundary_value[i] - last_value) * 256 + 0.5)
        
        rgb_list = gradient(last_rgb, boundary_bgr[i], num)

        for j in range(num):
            bgr_table[sum_num + j] = rgb_list[j]
            
        sum_num += num
        last_value = boundary_value[i]
        last_rgb = boundary_bgr[i]

def main():
    get_heat_point()
    init_alpha2_bgr_table()

    # 创建 4 通道图像 (BGRA)，对应 C++ 的 CV_8UC4
    img = np.zeros((height, width, 4), dtype=np.uint8)

    for i in range(len(heat_points)):
        # 权值因子
        ratio = heat_points[i]['value'] / value_range

        # 遍历该热力点的影响范围矩形
        rect = heat_rects[i]
        point = heat_points[i]
        
        for hi in range(rect['top'], rect['bottom'] + 1):
            for wi in range(rect['left'], rect['right'] + 1):
                # 判断是否在圆形热力圈范围内
                length = math.sqrt((wi - point['x'])**2 + (hi - point['y'])**2)
                if length <= reach:
                    alpha = ((reach - length) / reach) * ratio

                    # 计算并更新 Alpha 通道 (叠加现有的 alpha)
                    # 对应 C++: float newAlpha = data[m + 3] / 255.0f + alpha;
                    current_alpha = img[hi, wi, 3] / 255.0
                    new_alpha = current_alpha + alpha
                    new_alpha = np.clip(new_alpha * 255, 0, 255) # 限制在 0-255 之间
                    img[hi, wi, 3] = int(new_alpha)

                    # 颜色映射：根据当前的 Alpha 值从映射表中取颜色，赋值给 BGR 通道
                    alpha_idx = int(img[hi, wi, 3])
                    img[hi, wi, 0:3] = bgr_table[alpha_idx]

    # 显示热力图
    cv2.imshow("Heatmap", img)

    # 保存图片
    gis_basic = os.getenv("GISBasicRepo")  
    if gis_basic:
        img_path = os.path.join(gis_basic, "Data", "Raster", "dst.png")
        cv2.imwrite(img_path, img)
        print(f"热力图已保存至: {img_path}")
    else:
        print("未找到 GISBasic 环境变量，跳过图片保存。")

    cv2.waitKey(0)
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()