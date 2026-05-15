import os
import sys
import numpy as np
from osgeo import gdal

# 常量定义
PI = 3.14159265358979323846
D2R = PI / 180.0
R2D = 180.0 / PI

# 全局变量模拟
dem_width = dem_height = 0
geo_transform = [0] * 6
start_x = dx = start_y = dy = 0.0
no_value = 0.0
dem_buf = None
dst_buf = None

def read_dem():
    global dem_width, dem_height, geo_transform, start_x, dx, start_y, dy, no_value, dem_buf

    work_dir = os.getenv("GISBasicRepo")
    if not work_dir:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        return
    
    dem_path = os.path.join(work_dir, "Data", "Terrain", "dem.tif")

    dem = gdal.Open(dem_path, gdal.GA_ReadOnly)
    if not dem:
        print("Can't Open Image!")
        return

    dem_width = dem.RasterXSize
    dem_height = dem.RasterYSize

    geo_transform = dem.GetGeoTransform()
    start_x = geo_transform[0]
    dx = geo_transform[1]
    start_y = geo_transform[3]
    dy = geo_transform[5]

    band = dem.GetRasterBand(1)
    no_value = band.GetNoDataValue()
    
    # 读取数据到 numpy 数组
    dem_buf = band.ReadAsArray().astype(np.float32)
    dem = None

# 对应 C++ 中的 CalHillshade 核心计算公式
def cal_hillshade_vectorized(z_factor=1.0):
    global dst_buf

    # 设置平行光参数
    solar_altitude = 45.0
    solar_azimuth = 315.0

    # 转换为弧度 (完全还原C++逻辑)
    zenith_rad = (90 - solar_altitude) * D2R
    azimuth_math = 360.0 - solar_azimuth + 90
    if azimuth_math >= 360.0:
        azimuth_math -= 360.0
    azimuth_rad = azimuth_math * D2R

    # ================== 核心向量化计算 (模拟 3x3 窗口) ==================
    # a b c
    # d e f
    # g h i
    
    # 提取 3x3 邻域对应的所有切片 (避开边缘一圈)
    # 注意：Python切片是 [行, 列]，即 [y, x]
    a = dem_buf[0:-2, 0:-2]
    b = dem_buf[0:-2, 1:-1]
    c = dem_buf[0:-2, 2:]
    d = dem_buf[1:-1, 0:-2]
    # e = dem_buf[1:-1, 1:-1]  # 中心点其实不需要参与梯度计算
    f = dem_buf[1:-1, 2:]
    g = dem_buf[2:, 0:-2]
    h = dem_buf[2:, 1:-1]
    i = dem_buf[2:, 2:]

    # 批量计算 X 和 Y 方向的坡度变化率 (dzdx, dzdy)
    # 对应 C++: ((c + 2f + i) - (a + 2d + g)) / (8 * dx)
    dzdx = ((c + 2 * f + i) - (a + 2 * d + g)) / (8 * dx)
    # 对应 C++: ((g + 2h + i) - (a + 2b + c)) / (8 * dy)
    # 注意：这里 dy 传入的是正数，因为C++调用时传的是 -dy，但公式里分母通常取绝对值或按实际分辨率处理
    dzdy = ((g + 2 * h + i) - (a + 2 * b + c)) / (8 * abs(dy))

    # 计算坡度 (Slope) 和坡向 (Aspect)
    slope_rad = np.arctan(z_factor * np.sqrt(dzdx * dzdx + dzdy * dzdy))
    
    # 计算坡向，并处理特殊情况 (完全还原C++的 atan2 逻辑)
    aspect_rad = np.arctan2(dzdy, -dzdx)
    # 将负角度转换为 0 ~ 2PI
    aspect_rad = np.where(aspect_rad < 0, 2 * PI + aspect_rad, aspect_rad)
    # 处理 dzdx 接近 0 的特殊情况 (平坦或正南正北)
    aspect_rad = np.where(np.abs(dzdx) < 1e-9, 
                          np.where(dzdy > 0, PI / 2, 
                                   np.where(dzdy < 0, 2 * PI - PI / 2, aspect_rad)), 
                          aspect_rad)

    # 代入 Hillshade 最终公式
    hillshade = 255.0 * (
        (np.cos(zenith_rad) * np.cos(slope_rad)) + 
        (np.sin(zenith_rad) * np.sin(slope_rad) * np.cos(azimuth_rad - aspect_rad))
    )

    # 初始化输出缓冲区，填充为0（或NoData）
    dst_buf = np.zeros((dem_height, dem_width), dtype=np.uint8)
    # 将计算好的内部区域赋值回原位置 (相当于 C++ 中从 yi=1 遍历到 demHeight-1)
    dst_buf[1:-1, 1:-1] = np.clip(hillshade, 0.0, 255.0).astype(np.uint8)

def write_dst():
    work_dir = os.getenv("GISBasicRepo")
    dem_path = os.path.join(work_dir, "Data", "Terrain", "dst.tif")

    driver = gdal.GetDriverByName("GTIFF")
    dst = driver.Create(dem_path, dem_width, dem_height, 1, gdal.GDT_Byte, options=['BIGTIFF=IF_NEEDED'])
    if not dst:
        print("Can't Write Image!")
        return

    dst.SetGeoTransform(geo_transform)
    dst.GetRasterBand(1).WriteArray(dst_buf)
    dst = None
    print("Horn算法山体阴影计算完成并已保存！")

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
    gdal.AllRegister()
    
    # 设置 PROJ_LIB
    setup_proj_lib()

    read_dem()
    cal_hillshade_vectorized()
    write_dst()