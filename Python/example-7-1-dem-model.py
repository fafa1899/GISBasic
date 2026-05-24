import os
import sys
import numpy as np
from osgeo import gdal

# --- 1. 颜色表与渐变逻辑 (复刻 C++ 的 InitColorTable) ---
def gradient(start, end, count):
    """生成两个 RGB 颜色之间的渐变色列表"""
    d = (end - start) / count
    rgb_list = np.array([start + d * i for i in range(count)])
    return rgb_list

def init_color_table():
    """初始化 256 色查找表"""
    table_rgb = np.zeros((256, 3), dtype=np.float64)
    
    # 定义关键色 [R, G, B]
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

# --- 2. 读取 DEM 并生成带颜色的顶点数据 ---
def read_dem_and_generate_vertices(dem_path, table_rgb):
    """读取 DEM，计算顶点坐标并根据高程赋予颜色"""
    dem = gdal.Open(dem_path, gdal.GA_ReadOnly)
    if not dem:
        print("无法打开影像文件！")
        sys.exit(1)

    src_dem_width = dem.RasterXSize
    src_dem_height = dem.RasterYSize

    geo_transform = dem.GetGeoTransform()
    src_dx = geo_transform[1]
    src_dy = geo_transform[5]
    
    # 计算左上角起始坐标（对应 C++ 中的 startX, startY）
    start_x = geo_transform[0] + 0.5 * src_dx
    start_y = geo_transform[3] + 0.5 * src_dy

    # 读取高程数据
    band = dem.GetRasterBand(1)
    src_dem_buf = band.ReadAsArray().astype(np.float32)
    dem = None

    min_z = np.min(src_dem_buf)
    max_z = np.max(src_dem_buf)

    # 生成网格坐标 (对应 C++ 的双重 for 循环)
    # 使用 meshgrid 快速生成所有顶点的 X 和 Y 坐标
    xi = np.arange(src_dem_width)
    yi = np.arange(src_dem_height)
    xv, yv = np.meshgrid(xi, yi)
    
    vertex_x = start_x + xv * src_dx
    vertex_y = start_y + yv * src_dy
    vertex_z = src_dem_buf

    # 根据高程计算颜色索引 (复刻 GetColorIndex 逻辑)
    # floor((z - min_z) * 255 / (max_z - min_z) + 0.6)
    color_indices = np.floor((vertex_z - min_z) * 255 / (max_z - min_z) + 0.6).astype(int)
    color_indices = np.clip(color_indices, 0, 255)  # 防止越界

    # 提取对应的 RGB 颜色并转为 uint8
    colors = table_rgb[color_indices].astype(np.uint8)

    # 将所有数据展平为一维数组，方便后续写入 PLY
    vertices = np.column_stack((
        vertex_x.ravel(), 
        vertex_y.ravel(), 
        vertex_z.ravel(), 
        colors[:, :, 0].ravel(), 
        colors[:, :, 1].ravel(), 
        colors[:, :, 2].ravel()
    ))

    return vertices, src_dem_width, src_dem_height

# --- 3. 生成三角面索引 ---
def generate_face_indices(width, height):
    """生成 TIN 模型的三角面索引 (复刻 C++ 的 indices.push_back 逻辑)"""
    indices = []
    for yi in range(height - 1):
        for xi in range(width - 1):
            m = width * yi + xi
            # 第一个三角形
            indices.append([m, m + width, m + width + 1])
            # 第二个三角形
            indices.append([m + width + 1, m + 1, m])
    return np.array(indices, dtype=int)

# --- 4. 写入 PLY 文件 ---
def write_ply(vertices, faces, output_path):
    """将顶点和面数据写入 ASCII 格式的 PLY 文件"""
    vertex_count = len(vertices)
    face_count = len(faces)

    with open(output_path, 'w') as f:
        # 写入 PLY 头部信息
        f.write("ply\n")
        f.write("format ascii 1.0\n")
        f.write("comment CL generated\n")
        f.write(f"element vertex {vertex_count}\n")
        f.write("property double x\n")
        f.write("property double y\n")
        f.write("property double z\n")
        f.write("property uchar red\n")
        f.write("property uchar green\n")
        f.write("property uchar blue\n")
        f.write(f"element face {face_count}\n")
        f.write("property list uchar int vertex_indices\n")
        f.write("end_header\n")

        # 写入顶点数据
        for v in vertices:
            # x, y, z 为 double，red, green, blue 强制转为 int 输出
            f.write(f"{v[0]} {v[1]} {v[2]} {int(v[3])} {int(v[4])} {int(v[5])}\n")

        # 写入面索引数据
        for face in faces:
            # 3 表示该面由 3 个顶点组成，后面紧跟三个顶点的索引
            f.write(f"3 {face[0]} {face[1]} {face[2]}\n")

if __name__ == "__main__":
    gdal.AllRegister()  # 注册 GDAL 驱动

    # 设置 PROJ_LIB 环境变量 (对应 C++ 中的 CPLSetConfigOption)
    work_dir = os.getenv("GISBasicRepo")
    if not work_dir:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        sys.exit(1)
        
    proj_data_path = os.path.join(work_dir, "share", "proj")
    os.environ['PROJ_LIB'] = proj_data_path

    # 1. 初始化颜色表
    table_rgb = init_color_table()

    # 2. 读取 DEM 并生成顶点
    dem_path = os.path.join(work_dir, "Data", "Model", "dem.tif")
    print("正在读取 DEM 并生成顶点数据...")
    vertices, width, height = read_dem_and_generate_vertices(dem_path, table_rgb)

    # 3. 生成三角面索引
    print("正在生成三角网面片索引...")
    faces = generate_face_indices(width, height)

    # 4. 写入 PLY 模型文件
    out_ply_path = os.path.join(work_dir, "Data", "Model", "dst.ply")
    print(f"正在写入 PLY 文件: {out_ply_path}")
    write_ply(vertices, faces, out_ply_path)
    
    print("三维地形模型生成完成！")