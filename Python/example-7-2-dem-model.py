import os
import sys
import numpy as np
from osgeo import gdal

# --- 1. 读取 DEM 并生成带纹理坐标的顶点数据 ---
def read_dem_and_generate_vertices(dem_path):
    """读取 DEM，计算顶点坐标并生成对应的 UV 纹理坐标"""
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
    print(f"DEM 高程范围: {min_z:.2f} ~ {max_z:.2f}")

    # 生成网格坐标 (对应 C++ 的双重 for 循环)
    xi = np.arange(src_dem_width)
    yi = np.arange(src_dem_height)
    xv, yv = np.meshgrid(xi, yi)
    
    vertex_x = start_x + xv * src_dx
    vertex_y = start_y + yv * src_dy
    vertex_z = src_dem_buf

    # 生成纹理坐标 (UV)，复刻 C++ 逻辑：xi / (width - 1), yi / (height - 1)
    tex_coord_x = xv / (src_dem_width - 1)
    tex_coord_y = yv / (src_dem_height - 1)

    # 将所有数据展平为一维数组，方便后续写入 OBJ
    vertices = np.column_stack((vertex_x.ravel(), vertex_y.ravel(), vertex_z.ravel()))
    tex_coords = np.column_stack((tex_coord_x.ravel(), tex_coord_y.ravel()))

    return vertices, tex_coords, src_dem_width, src_dem_height

# --- 2. 生成三角面索引 ---
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

# --- 3. 写入 OBJ 和 MTL 文件 ---
def write_obj_mtl(vertices, tex_coords, faces, work_dir):
    """将顶点、纹理坐标和面数据写入 OBJ 文件，并生成配套的 MTL 文件"""
    obj_path = os.path.join(work_dir, "Data", "Model", "dst.obj")
    mtl_path = os.path.join(work_dir, "Data", "Model", "dst.mtl")

    # 写入 MTL 材质文件
    with open(mtl_path, 'w') as f:
        f.write("newmtl dst\n")
        f.write("illum 2\n")
        f.write("map_Ka tex.jpg\n")
        f.write("map_Kd tex.jpg\n")
        f.write("map_Ks tex.jpg\n")
        f.write("Ns 10.000\n")

    # 写入 OBJ 模型文件
    with open(obj_path, 'w') as f:
        # 引用材质库
        f.write("mtllib dst.mtl\n")
        
        # 写入顶点坐标 (v)
        for v in vertices:
            f.write(f"v {v[0]} {v[1]} {v[2]}\n")
        
        # 写入纹理坐标 (vt)
        for vt in tex_coords:
            f.write(f"vt {vt[0]} {vt[1]}\n")
        
        # 应用材质
        f.write("usemtl dst\n")
        
        # 写入面索引 (f)。注意：OBJ 格式索引从 1 开始，所以这里要 +1
        # 格式为 f v/vt v/vt v/vt (顶点索引/纹理坐标索引)
        for face in faces:
            idx1, idx2, idx3 = face + 1  # OBJ 索引转为 1-based
            f.write(f"f {idx1}/{idx1} {idx2}/{idx2} {idx3}/{idx3}\n")

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

    gdal.AllRegister()  # 注册 GDAL 驱动

    gdal.UseExceptions() # 启用 GDAL 异常处理，方便调试

    setup_proj_lib()

    # 设置 PROJ_LIB 环境变量
    work_dir = os.getenv("GISBasicRepo")
    if not work_dir:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        sys.exit(1)
        
    # 1. 读取 DEM 并生成顶点和纹理坐标
    dem_path = os.path.join(work_dir, "Data", "Model", "dem.tif")
    print("正在读取 DEM 并生成顶点与纹理坐标...")
    vertices, tex_coords, width, height = read_dem_and_generate_vertices(dem_path)

    # 2. 生成三角面索引
    print("正在生成三角网面片索引...")
    faces = generate_face_indices(width, height)

    # 3. 写入 OBJ 和 MTL 文件
    print("正在写入 OBJ 和 MTL 文件...")
    write_obj_mtl(vertices, tex_coords, faces, work_dir)
    
    print("带纹理坐标的三维地形模型生成完成！")