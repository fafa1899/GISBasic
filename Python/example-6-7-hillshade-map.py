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
    start_x = geo_transform[0]  # 左上角点坐标X
    dx = geo_transform[1]       # X方向的分辨率
    start_y = geo_transform[3]  # 左上角点坐标Y
    dy = geo_transform[5]       # Y方向的分辨率

    band = dem.GetRasterBand(1)
    no_value = band.GetNoDataValue()
    
    # 读取数据到 numpy 数组
    dem_buf = band.ReadAsArray().astype(np.float32)
    dem = None

# 计算三点成面的法向量 (支持批量矩阵运算)
def cal_normal_3d(v1, v2, v3):
    # 提取 x, y, z 坐标 (保持维度)
    x1, y1, z1 = v1[..., 0], v1[..., 1], v1[..., 2]
    x2, y2, z2 = v2[..., 0], v2[..., 1], v2[..., 2]
    x3, y3, z3 = v3[..., 0], v3[..., 1], v3[..., 2]
    
    # 叉乘计算法向量的三个分量
    vn_x = (y2 - y1) * (z3 - z1) - (z2 - z1) * (y3 - y1)
    vn_y = (z2 - z1) * (x3 - x1) - (x2 - x1) * (z3 - z1)
    vn_z = (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1)
    
    # 将三个分量沿最后一个轴堆叠，保证输出形状始终为 (..., 3)
    return np.stack((vn_x, vn_y, vn_z), axis=-1)

def hillshade():
    global dst_buf

    # 1. 构造所有的顶点三维坐标 (pointList) - 保持原逻辑不变
    xi_grid, yi_grid = np.meshgrid(np.arange(dem_width), np.arange(dem_height))
    x_coords = start_x + (xi_grid + 0.5) * dx
    y_coords = start_y + (yi_grid + 0.5) * dy
    z_coords = dem_buf
    point_list = np.stack((x_coords, y_coords, z_coords), axis=-1)

    # 提取有效高程范围
    valid_mask = ~(np.isclose(z_coords, no_value) | (z_coords < -11034) | (z_coords > 8844.43))
    min_z = np.min(z_coords[valid_mask]) if np.any(valid_mask) else 0
    max_z = np.max(z_coords[valid_mask]) if np.any(valid_mask) else 0
    print(f"DEM高程范围: {min_z:.2f} ~ {max_z:.2f}")

    # ================== 核心向量化优化部分 ==================
    
    # 2. 批量提取所有三角形的顶点坐标
    # 获取构成当前栅格单元的四个顶点的索引切片
    y0x0 = point_list[:-1, :-1]  # 左上
    y1x0 = point_list[1:, :-1]   # 左下
    y0x1 = point_list[:-1, 1:]   # 右上
    y1x1 = point_list[1:, 1:]    # 右下

    # 批量计算第一个三角形 (左上-左下-右上) 的法向量
    vn1 = cal_normal_3d(y0x0, y1x0, y0x1) 
    # 批量计算第二个三角形 (左下-右下-右上) 的法向量
    vn2 = cal_normal_3d(y1x0, y1x1, y0x1)

    # 3. 将法向量重新映射回原始栅格大小 (模拟 C++ 的 multimap 累加逻辑)
    # 初始化一个全零的累加器，形状为 [高, 宽, 3(x,y,z)]
    normal_accumulator = np.zeros((dem_height, dem_width, 3), dtype=np.float64)

    # 将两个三角形的法向量分别累加到对应的三个顶点位置上
    normal_accumulator[:-1, :-1] += vn1  # 左上角贡献
    normal_accumulator[1:, :-1] += vn1   # 左下角贡献
    normal_accumulator[:-1, 1:] += vn1   # 右上角贡献

    normal_accumulator[1:, :-1] += vn2   # 左下角贡献
    normal_accumulator[1:, 1:] += vn2    # 右下角贡献
    normal_accumulator[:-1, 1:] += vn2   # 右上角贡献

    # 4. 设置平行光方向
    solar_altitude = 45.0
    solar_azimuth = 315.0
    f_altitude = solar_altitude * D2R
    f_azimuth = solar_azimuth * D2R
    
    array_vector = np.array([
        np.cos(f_altitude) * np.cos(f_azimuth),
        np.cos(f_altitude) * np.sin(f_azimuth),
        np.sin(f_altitude)
    ])

    # 5. 批量计算最终的阴影值
    # 对累加后的法向量进行归一化
    norms = np.linalg.norm(normal_accumulator, axis=2, keepdims=True)
    # 防止除以0，给norms加一个极小值
    n_normalized = normal_accumulator / (norms + 1e-9)

    # 批量计算点积和夹角 (完全还原C++逻辑：角度越大越亮)
    dot_product = np.dot(n_normalized, array_vector)
    dot_product = np.clip(dot_product, -1.0, 1.0)
    angle = np.arccos(dot_product) * R2D
    
    # 映射到 0-255 灰度值
    dst_buf = np.clip(angle / 90.0 * 255.0, 0.0, 255.0).astype(np.uint8)
    
    print("山体阴影(Hillshade)向量化计算完成！")

def write_dst():
    work_dir = os.getenv("GISBasicRepo")
    dem_path = os.path.join(work_dir, "Data", "Terrain", "dst.tif")

    driver = gdal.GetDriverByName("GTIFF")
    # 创建输出文件，数据类型为 GDT_Byte (对应 C++ 的 uint8_t)
    dst = driver.Create(dem_path, dem_width, dem_height, 1, gdal.GDT_Byte, options=['BIGTIFF=IF_NEEDED'])
    if not dst:
        print("Can't Write Image!")
        return

    dst.SetGeoTransform(geo_transform)

    # 直接将 numpy 数组写入波段
    dst.GetRasterBand(1).WriteArray(dst_buf)
    dst = None
    print("结果已保存至:", dem_path)

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
    hillshade()
    write_dst()