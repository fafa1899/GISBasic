import os
import sys
import numpy as np
from osgeo import gdal

# --- 全局参数与配置 ---
start_height = 550.0
end_height = 2815.0
height_interval = 500.0

# 初始化 GDAL
gdal.AllRegister()

def gradient(start, end, count):
    """生成两个 RGB 颜色之间的渐变色列表"""
    d = (end - start) / count
    rgb_list = np.array([start + d * i for i in range(count)])
    return rgb_list

def init_color_table():
    """复刻 C++ 中的 InitColorTable 逻辑，生成 256 色查找表"""
    table_rgb = np.zeros((256, 3), dtype=np.float64)
    
    # 定义关键色 (注意：C++中 array<double, 3> 对应这里 [R, G, B])
    blue = np.array([17, 60, 235])
    green = np.array([17, 235, 86])
    yellow = np.array([235, 173, 17])
    red = np.array([235, 60, 17])
    white = np.array([235, 17, 235])
    
    # 分段生成渐变并填充到查找表中
    table_rgb[0:60] = gradient(blue, green, 60)
    table_rgb[60:120] = gradient(green, yellow, 60)
    table_rgb[120:180] = gradient(yellow, red, 60)
    table_rgb[180:256] = gradient(red, white, 76)
    
    return table_rgb

def read_dem():
    """读取 DEM 数据及地理信息"""
    work_dir = os.getenv("GISBasicRepo")
    if not work_dir:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        sys.exit(1)
        
    dem_path = os.path.join(work_dir, "Data", "Terrain", "dem.tif")

    dem = gdal.Open(dem_path, gdal.GA_ReadOnly)
    if not dem:
        print("无法打开影像文件！")
        sys.exit(1)

    dem_width = dem.RasterXSize
    dem_height = dem.RasterYSize

    geo_transform = dem.GetGeoTransform()
    
    # 读取第一个波段的 DEM 高程数据
    band = dem.GetRasterBand(1)
    dem_buf = band.ReadAsArray().astype(np.float32)
    
    dem = None  # 释放资源
    return dem_width, dem_height, geo_transform, dem_buf

def handle_dem(dem_buf, height_threshold_list, height_rgb_list):
    """核心处理函数：使用向量化操作替代 C++ 的多重循环"""
    dem_height, dem_width = dem_buf.shape
    
    # 初始化输出缓冲区，默认填充为 255 (白色)，对应 dstBuf.resize(dstBufNum, 255)
    # 形状为 (高, 宽, 4)，分别对应 R, G, B, A
    dst_buf = np.full((dem_height, dem_width, 4), 255, dtype=np.uint8)

    # 遍历每一个高度阈值和对应的颜色
    for threshold, color in zip(height_threshold_list, height_rgb_list):
        # 找出所有大于当前阈值的像素位置 (布尔索引)
        mask = dem_buf > threshold
        
        # 将这些位置的 RGB 通道替换为对应的阈值颜色
        # 直接通过掩码对三维数组进行赋值
        dst_buf[mask, 0] = int(color[0])  # R
        dst_buf[mask, 1] = int(color[1])  # G
        dst_buf[mask, 2] = int(color[2])  # B
        # Alpha 通道保持默认的 255

    return dst_buf

def write_dst(dst_buf, geo_transform, dem_width, dem_height):
    """将结果写入 GeoTIFF 文件"""
    work_dir = os.getenv("GISBasicRepo")
    dst_path = os.path.join(work_dir, "Data", "Terrain", "dst.tif")

    driver = gdal.GetDriverByName("GTIFF")
    # 创建 4 波段的 GeoTIFF (RGBA)
    dst = driver.Create(dst_path, dem_width, dem_height, 4, gdal.GDT_Byte, options=['BIGTIFF=IF_NEEDED'])
    if not dst:
        print("无法创建输出文件！")
        return

    dst.SetGeoTransform(geo_transform)

    # 逐波段写入数据 (GDAL 存储顺序通常是 R, G, B, A)
    for i in range(4):
        dst.GetRasterBand(i + 1).WriteArray(dst_buf[:, :, i])
    
    dst = None
    print("栅格形式等高线地形图已保存！")

# 自动设置 PROJ_LIB for Conda (Windows/Linux/macOS) 
def setup_proj_lib():
    if 'CONDA_PREFIX' in os.environ:
        conda_prefix = os.environ['CONDA_PREFIX']
        # Windows
        proj_path = os.path.join(conda_prefix, 'Library', 'share', 'proj')
        if not os.path.exists(proj_path):
            # Unix-like
            proj_path = os.path.join(conda_prefix, 'share', 'proj')
        if os.path.exists(proj_path):       
            os.environ['PROJ_LIB'] = proj_path
            return True
    return False

if __name__ == "__main__":
    setup_proj_lib()

    # 1. 读取 DEM
    dem_width, dem_height, geo_transform, dem_buf = read_dem()

    # 2. 初始化颜色表
    table_rgb = init_color_table()

    # 3. 生成高度阈值列表 (复刻 C++ 的 while 循环)
    height_threshold_list = []
    height_threshold = start_height
    while height_threshold < end_height:
        height_threshold_list.append(height_threshold)
        height_threshold += height_interval

    # 4. 生成高度阈值对应的颜色列表 (复刻 C++ 的等间距采样逻辑)
    height_rgb_list = []
    if len(height_threshold_list) == 1:
        height_rgb_list.append(table_rgb[0])
    else:
        step = len(table_rgb) // (len(height_threshold_list) - 1)
        index = 0
        for _ in range(len(height_threshold_list) - 1):
            height_rgb_list.append(table_rgb[index])
            index += step
        height_rgb_list.append(table_rgb[-1])

    # 5. 核心处理 (替代 HandleDem)
    dst_buf = handle_dem(dem_buf, height_threshold_list, height_rgb_list)

    # 6. 写入结果
    write_dst(dst_buf, geo_transform, dem_width, dem_height)