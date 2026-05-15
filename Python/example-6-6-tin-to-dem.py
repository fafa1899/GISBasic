import os
import struct
import math
import numpy as np
from osgeo import gdal

# 模拟 C++ 中的全局变量
tin_triangles = []
tri_boxes = []
bound = [float('inf'), float('inf'), -float('inf'), -float('inf')]

dst_dx = 100.0
dst_dy = 100.0
start_x = start_y = 0.0
dst_dem_column = dst_dem_row = 0
dst_no_data_value = -32768.0
dst_dem_buf = None

# 判断点P是否在空间三角形内，如果在则计算并赋值Z值
def point_in_triangle_3d(triangle, p):
    v0p = p - triangle[0]
    v0v1 = triangle[1] - triangle[0]
    v0v2 = triangle[2] - triangle[0]

    d = v0v1[0] * v0v2[1] - v0v1[1] * v0v2[0]
    if abs(d) < 1e-9:  # 防止除以零，对应 C++ 的 D == 0.0
        return False

    d1 = v0p[0] * v0v2[1] - v0p[1] * v0v2[0]
    d2 = v0v1[0] * v0p[1] - v0v1[1] * v0p[0]

    u = d1 / d
    v = d2 / d
    
    if u >= 0 and v >= 0 and (u + v) <= 1:
        p[2] = v0v1[2] * u + v0v2[2] * v + triangle[0][2]
        return True
    return False

def read_tin():
    global tin_triangles, tri_boxes, bound

    work_dir = os.getenv("GISBasicRepo")
    tin_path = os.path.join(work_dir, "Data", "Terrain", "terrain.ply")

    with open(tin_path, 'rb') as infile:
        # 检查是否是 ply 格式
        line = infile.readline().decode('ascii').strip()
        if line != "ply":
            print("Not a valid PLY file!")
            return

        vertex_count = face_count = 0
        
        # 解析 header
        while True:
            line = infile.readline().decode('ascii').strip()
            if line == "end_header":
                break
            
            parts = line.split()
            if len(parts) == 3 and parts[0] == "element":
                if parts[1] == "vertex":
                    vertex_count = int(parts[2])
                elif parts[1] == "face":
                    face_count = int(parts[2])

        # 读取顶点数据 (使用 struct 解包二进制数据)
        # 假设 PLY 文件中顶点坐标是 double (8字节)，如果是 float 请将 '3d' 改为 '3f'
        vertex_tmp = []
        for _ in range(vertex_count):
            data = infile.read(24)  # 3个double = 24字节
            x, y, z = struct.unpack('3d', data)
            vertex_tmp.extend([x, y, z])

        # 计算边界范围 bound
        for vi in range(vertex_count):
            vx, vy = vertex_tmp[vi * 3], vertex_tmp[vi * 3 + 1]
            bound[0] = min(vx, bound[0])
            bound[1] = min(vy, bound[1])
            bound[2] = max(vx, bound[2])
            bound[3] = max(vy, bound[3])

        # 读取面（三角形）索引数据
        # C++ 中 stepSize=13 意味着：1个uint8 + 3个int32 (1 + 3*4 = 13字节)
        tin_triangles = []
        tri_boxes = []
        
        for _ in range(face_count):
            # 读取顶点数量标志位 (通常是 uint8)
            flag = struct.unpack('B', infile.read(1))[0]
            # 读取三个顶点的索引 (int32)
            ids = list(struct.unpack('3i', infile.read(12)))

            triangle = np.zeros((3, 3))
            box = [float('inf'), float('inf'), -float('inf'), -float('inf')]
            
            for i in range(3):
                idx = ids[i]
                triangle[i][0] = vertex_tmp[idx * 3]
                triangle[i][1] = vertex_tmp[idx * 3 + 1]
                triangle[i][2] = vertex_tmp[idx * 3 + 2]
                
                # 顺便计算当前三角形的包围盒
                box[0] = min(triangle[i][0], box[0])
                box[1] = min(triangle[i][1], box[1])
                box[2] = max(triangle[i][0], box[2])
                box[3] = max(triangle[i][1], box[3])

            tin_triangles.append(triangle)
            tri_boxes.append(box)

def convert():
    global start_x, start_y, dst_dem_column, dst_dem_row, dst_dem_buf

    start_x = math.ceil(bound[0] / dst_dx) * dst_dx
    start_y = math.ceil(bound[1] / dst_dy) * dst_dy
    end_x = math.floor(bound[2] / dst_dx) * dst_dx
    end_y = math.floor(bound[3] / dst_dy) * dst_dy

    dst_dem_column = int((end_x - start_x) / dst_dx) + 1
    dst_dem_row = int((end_y - start_y) / dst_dy) + 1

    # 初始化目标栅格数组，填充 NoData 值
    dst_dem_buf = np.full((dst_dem_row, dst_dem_column), dst_no_data_value, dtype=np.float32)
    flag_map = np.zeros((dst_dem_row, dst_dem_column), dtype=bool)

    total_triangles = len(tin_triangles)
    for ti in range(total_triangles):
        if (ti + 1) % 100 == 0 or ti == total_triangles - 1:
            print(f"处理进度: {float(ti + 1) / total_triangles:.2%}")

        box = tri_boxes[ti]
        triangle = tin_triangles[ti]

        # 计算当前三角形覆盖的栅格行列范围
        left = max(int(math.floor((box[0] - start_x) / dst_dx)), 0)
        bottom = max(int(math.floor((box[1] - start_y) / dst_dy)), 0)
        right = min(int(math.ceil((box[2] - start_x) / dst_dx)), dst_dem_column - 1)
        top = min(int(math.ceil((box[3] - start_y) / dst_dy)), dst_dem_row - 1)

        # 遍历该范围内的所有像元
        for yi in range(bottom, top + 1):
            for xi in range(left, right + 1):
                m = (yi, xi)
                if not flag_map[m]:
                    # 构造测试点 P
                    p = np.array([start_x + xi * dst_dx, start_y + yi * dst_dy, 0.0])
                    if point_in_triangle_3d(triangle, p):
                        dst_dem_buf[m] = p[2]
                        flag_map[m] = True

def write_dem():
    work_dir = os.getenv("GISBasicRepo")
    dem_path = os.path.join(work_dir, "Data", "Terrain", "dst.tif")

    driver = gdal.GetDriverByName("GTIFF")
    dem = driver.Create(dem_path, dst_dem_column, dst_dem_row, 1, gdal.GDT_Float32, options=['BIGTIFF=IF_NEEDED'])
    if not dem:
        print("Can't Write Image!")
        return

    # 设置地理变换参数 (注意左上角 Y 坐标的计算)
    padf_transform = [
        start_x - 0.5 * dst_dx,         # 左上角点坐标X
        dst_dx,                         # X方向的分辨率
        0,                              # 旋转系数
        start_y + (dst_dem_row - 0.5) * dst_dy, # 左上角点坐标Y
        0,                              # 旋转系数
        -dst_dy                         # Y方向的分辨率
    ]
    dem.SetGeoTransform(padf_transform)
    dem.GetRasterBand(1).SetNoDataValue(dst_no_data_value)

    # 写入数据 (NumPy 数组直接写入即可，GDAL会自动处理内存布局)
    dem.GetRasterBand(1).WriteArray(dst_dem_buf)
    dem = None
    print("TIN转DEM完成！")

if __name__ == "__main__":
    gdal.AllRegister()
    read_tin()
    convert()
    write_dem()