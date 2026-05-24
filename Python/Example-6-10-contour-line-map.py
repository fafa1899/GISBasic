from osgeo import gdal, ogr
import numpy as np
from pathlib import Path
import os
import sys

class TrigonVertexIndex:
    def __init__(self):
        self.index = [0, 0, 0]


# 参数
start_height = 550
end_height = 2815
height_interval = 500

# 全局数据
vertex_xyz = []
face_vertex_index = []


# ------------------------------------------------------------------------------
# 工具函数
# ------------------------------------------------------------------------------

def chop_string_with_space(line: str):
    return line.strip().split()


# ------------------------------------------------------------------------------
# 读取 TIN(Ply)
# ------------------------------------------------------------------------------

def read_tin(model_path):
    global vertex_xyz
    global face_vertex_index

    model_path = Path(model_path)

    if not model_path.exists():
        print(f"Can't Load {model_path}")
        return False

    with open(model_path, "rb") as infile:

        n_v = 0
        n_f = 0

        # 读取 header
        while True:
            line = infile.readline().decode("utf-8").strip()

            if line == "end_header":
                break

            substring = chop_string_with_space(line)

            if len(substring) == 3 and substring[0] == "element":
                if substring[1] == "vertex":
                    n_v = int(substring[2])

                elif substring[1] == "face":
                    n_f = int(substring[2])

        # 读取 vertex
        vertex_data = np.fromfile(
            infile,
            dtype=np.float64,
            count=n_v * 3
        ).reshape((n_v, 3))

        vertex_xyz = vertex_data

        # 读取 face
        face_vertex_index = []

        for _ in range(n_f):

            face_type = np.fromfile(infile, dtype=np.uint8, count=1)[0]

            if face_type != 3:
                print("Format Incompatible Or Non Trigon!")
                return False

            ids = np.fromfile(infile, dtype=np.int32, count=3)

            tri = TrigonVertexIndex()
            tri.index[0] = int(ids[0])
            tri.index[1] = int(ids[1])
            tri.index[2] = int(ids[2])

            face_vertex_index.append(tri)

    return True


# ------------------------------------------------------------------------------
# 三角形类型判断
# ------------------------------------------------------------------------------

def cal_triangle_type(trigon_vid, vertex_flag):

    tri_vertex_flag = [False, False, False]

    for vi in range(3):
        vid = trigon_vid.index[vi]
        tri_vertex_flag[vi] = vertex_flag[vid]

    if (not tri_vertex_flag[0]
            and not tri_vertex_flag[1]
            and not tri_vertex_flag[2]):
        return 0

    elif (not tri_vertex_flag[0]
          and not tri_vertex_flag[1]
          and tri_vertex_flag[2]):
        return 1

    elif (tri_vertex_flag[0]
          and not tri_vertex_flag[1]
          and not tri_vertex_flag[2]):
        return 2

    elif (not tri_vertex_flag[0]
          and tri_vertex_flag[1]
          and not tri_vertex_flag[2]):
        return 3

    elif (tri_vertex_flag[0]
          and tri_vertex_flag[1]
          and tri_vertex_flag[2]):
        return 4

    elif (tri_vertex_flag[0]
          and tri_vertex_flag[1]
          and not tri_vertex_flag[2]):
        return 5

    elif (not tri_vertex_flag[0]
          and tri_vertex_flag[1]
          and tri_vertex_flag[2]):
        return 6

    elif (tri_vertex_flag[0]
          and not tri_vertex_flag[1]
          and tri_vertex_flag[2]):
        return 7

    return 0


# ------------------------------------------------------------------------------
# 计算线段与 Z 平面的交点
# ------------------------------------------------------------------------------

def cal_point_of_segment_line_with_z(o, e, z):

    o = np.array(o)
    e = np.array(e)

    if e[2] < o[2]:
        o, e = e, o

    dz = e[2] - o[2]

    if abs(dz) < 1e-10:
        return None

    t = (z - o[2]) / dz

    # 修复了原 C++ 中的 bug:
    # if (t < 0 && t > 1)
    if t < 0 or t > 1:
        return None

    p = o + (e - o) * t

    return p


# ------------------------------------------------------------------------------
# 计算三角形与等高面的交线
# ------------------------------------------------------------------------------

def cal_triangle_intersecting_line(trigon_vid, corner_id, z):

    xyz_list = []

    for vi in range(3):
        vid = trigon_vid.index[vi]
        xyz_list.append(vertex_xyz[vid])

    if corner_id == 0:
        start = cal_point_of_segment_line_with_z(
            xyz_list[0], xyz_list[1], z
        )
        end = cal_point_of_segment_line_with_z(
            xyz_list[0], xyz_list[2], z
        )

    elif corner_id == 1:
        start = cal_point_of_segment_line_with_z(
            xyz_list[1], xyz_list[0], z
        )
        end = cal_point_of_segment_line_with_z(
            xyz_list[1], xyz_list[2], z
        )

    else:
        start = cal_point_of_segment_line_with_z(
            xyz_list[2], xyz_list[1], z
        )
        end = cal_point_of_segment_line_with_z(
            xyz_list[2], xyz_list[0], z
        )

    return start, end


# ------------------------------------------------------------------------------
# 计算等高线
# ------------------------------------------------------------------------------

def cal_iso_height_line(trigon_vid, tri_type, height):

    if tri_type in [1, 5]:
        return cal_triangle_intersecting_line(
            trigon_vid, 2, height
        )

    elif tri_type in [2, 6]:
        return cal_triangle_intersecting_line(
            trigon_vid, 0, height
        )

    elif tri_type in [3, 7]:
        return cal_triangle_intersecting_line(
            trigon_vid, 1, height
        )

    return None, None


# ------------------------------------------------------------------------------
# 主函数
# ------------------------------------------------------------------------------

def main():

    gdal.AllRegister()

    # 等高线高程列表
    height_threshold_list = []

    h = start_height

    while h < end_height:
        height_threshold_list.append(h)
        h += height_interval

    # 工作目录
    work_dir = os.getenv("GISBasicRepo")

    if not work_dir:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        return

    tin_path = os.path.join(
        work_dir,
        "Data",
        "Terrain",
        "terrain.ply"
    )
  
    out_shp_file = os.path.join(
        work_dir,
        "Data",
        "Terrain",
        "dst.shp"
    )

    # 读取TIN
    if not read_tin(tin_path):
        return

    # 创建 shp
    driver = ogr.GetDriverByName("ESRI Shapefile")

    if driver is None:
        print("Get Driver ESRI Shapefile Error!")
        return

    dataset = driver.CreateDataSource(str(out_shp_file))

    layer = dataset.CreateLayer(
        "IsoHeightline",
        geom_type=ogr.wkbMultiLineStringZM
    )

    multi_line_string = ogr.Geometry(ogr.wkbMultiLineStringZM)

    # 逐层生成等高线
    for height_threshold in height_threshold_list:

        print(f"Processing Height: {height_threshold}")

        vertex_flag = vertex_xyz[:, 2] >= height_threshold

        for fi in range(len(face_vertex_index)):

            tri_type = cal_triangle_type(
                face_vertex_index[fi],
                vertex_flag
            )

            start, end = cal_iso_height_line(
                face_vertex_index[fi],
                tri_type,
                height_threshold
            )

            if start is None or end is None:
                continue

            line = ogr.Geometry(ogr.wkbLineStringZM)

            line.AddPoint(
                float(start[0]),
                float(start[1]),
                float(start[2])
            )

            line.AddPoint(
                float(end[0]),
                float(end[1]),
                float(end[2])
            )

            multi_line_string.AddGeometry(line)

    # 写入 feature
    feature_defn = layer.GetLayerDefn()
    feature = ogr.Feature(feature_defn)

    feature.SetGeometry(multi_line_string)

    if layer.CreateFeature(feature) != 0:
        print("Failed to create feature in shapefile.")

    # 释放
    feature = None
    dataset = None

    print("Done!")


if __name__ == "__main__":
    main()