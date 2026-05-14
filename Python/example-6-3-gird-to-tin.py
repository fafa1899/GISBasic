import os
import sys
import numpy as np
from osgeo import gdal, osr
from scipy.spatial import Delaunay

def cal_distance_point_and_line(point, line_begin, line_end):
    """计算点到直线的距离 (对应 C++ 的 CalDistancePointAndLine)"""
    # 直线方向向量
    n = line_end - line_begin
    # 直线上某一点的向量到点的向量
    m = point - line_begin
    # 叉乘的范数除以方向向量的范数
    return np.linalg.norm(np.cross(n, m)) / np.linalg.norm(n)

def main():
    osr.UseExceptions() # 启用异常处理（推荐），避免静默错误

    gdal.AllRegister()

    #设置 PROJ_LIB
    setup_proj_lib()

    # 拼接输出 DEM 文件路径
    gis_basic = os.getenv("GISBasicRepo")
    if not gis_basic:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        return
    dem_path = os.path.join(gis_basic, "Data", "Terrain", "dem.tif")
    tin_path = os.path.join(gis_basic, "Data", "Terrain", "tin.ply")

    # 读取 DEM
    dem = gdal.Open(dem_path, gdal.GA_ReadOnly)
    if not dem:
        print("Can't Open Image!")
        return

    dem_width = dem.RasterXSize
    dem_height = dem.RasterYSize

    # 获取地理变换参数
    geo_transform = dem.GetGeoTransform()
    dx = geo_transform[1]
    dy = -geo_transform[5]  
    start_x = geo_transform[0] + 0.5 * dx
    start_y = geo_transform[3] - dem_height * dy + 0.5 * dy

    # 读取 DEM 高程数据
    band = dem.GetRasterBand(1)
    dem_buf = band.ReadAsArray().astype(np.float32)
    dem.Close()

    points = []
    z_threshold = 5

    # ==========================================
    #  ↓↓↓  双重循环已被替换为以下向量化操作  ↓↓↓
    # ==========================================
    print("正在进行向量化特征点筛选...")

    # 1. 提取内部点及其四周对角线邻居 (去掉了最外圈像素，避免越界)
    # z_center 对应原本循环里的 P 点高程
    z_center = dem_buf[1:-1, 1:-1] 
    # z_tl, z_tr, z_bl, z_br 对应原本循环里的 A, C, G, E 四个角点高程
    z_tl = dem_buf[0:-2, 0:-2] # Top-Left
    z_tr = dem_buf[0:-2, 2:]   # Top-Right
    z_bl = dem_buf[2:, 0:-2]   # Bottom-Left
    z_br = dem_buf[2:, 2:]     # Bottom-Right

    # 2. 向量化计算高程偏差 (完美替代原本复杂的点到直线距离计算)
    # 计算 P 点与 (左上A-右下E) 连线中点的高程差绝对值
    diff1 = np.abs(z_center - (z_tl + z_br) / 2.0)
    # 计算 P 点与 (右上C-左下G) 连线中点的高程差绝对值
    diff2 = np.abs(z_center - (z_tr + z_bl) / 2.0)
    
    # 取两个方向偏差的最大值作为该点的特征强度
    max_diff = np.maximum(diff1, diff2)

    # 3. 根据阈值筛选出特征点的内部索引，并还原到原始 DEM 的真实行列号
    valid_y_internal, valid_x_internal = np.where(max_diff > z_threshold)
    valid_y = valid_y_internal + 1  # 还原 y 坐标 (加上被切片去掉的那一行)
    valid_x = valid_x_internal + 1  # 还原 x 坐标 (加上被切片去掉的那一列)

    # 4. 批量计算所有特征点的真实地理坐标 (X, Y, Z)  
    gx = start_x + dx * valid_x
    gy = start_y + dy * valid_y
    gz = dem_buf[valid_y, valid_x]

    # 5. 提取四个角点（保证 TIN 范围与 DEM 一致）
    corner_indices = [(0, 0), (dem_width - 1, 0), (dem_width - 1, dem_height - 1), (0, dem_height - 1)]
    corner_points = []
    for xi, yi in corner_indices:
        # 换回 GDAL 原生公式计算角点坐标
        cx = start_x + dx * xi
        cy = start_y + dy * yi
        corner_points.append([cx, cy, dem_buf[yi, xi]])

    # 6. 合并角点和筛选出的特征点
    feature_points = np.column_stack((gx, gy, gz))
    all_points = np.vstack((corner_points, feature_points))
    points = all_points.tolist()
    # ==========================================
    #  ↑↑↑  向量化操作结束  ↑↑↑
    # ==========================================

    print(f"筛选出 {len(points)} 个特征点，正在进行 Delaunay 三角剖分...")

    # 使用 scipy 进行德劳内三角剖分
    points_np = np.array(points)
    tri = Delaunay(points_np[:, :2])

    # 将结果导出为 PLY 格式
    with open(tin_path, 'w') as f:
        f.write("ply\n")
        f.write("format ascii 1.0\n")
        f.write(f"element vertex {len(points_np)}\n")
        f.write("property float x\n")
        f.write("property float y\n")
        f.write("property float z\n")
        f.write(f"element face {len(tri.simplices)}\n")
        f.write("property list uchar int vertex_indices\n")
        f.write("end_header\n")

        # 写入顶点
        for p in points_np:
            f.write(f"{p[0]} {p[1]} {p[2]}\n")
        
        # 写入三角面
        for face in tri.simplices:
            f.write(f"3 {face[0]} {face[1]} {face[2]}\n")

    print(f"TIN 已成功生成并保存至: {tin_path}")

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
    main()