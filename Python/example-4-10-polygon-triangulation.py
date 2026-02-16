import os
import numpy as np
import triangle
from osgeo import ogr, gdal

def polygon_to_triangle_input(polygons):
    """
    将多个多边形（外环+内环）转换为 triangle 所需的输入格式
    polygons: list of list of (x, y) — 第一个是外边界，其余是洞
    """
    vertices = []
    segments = []
    segment_markers = []  # 可选，用于标记边界
    hole_points = []

    vertex_offset = 0
    for i, poly in enumerate(polygons):
        n = len(poly)
        # 添加顶点
        vertices.extend(poly)
        # 添加边（闭合）
        for j in range(n):
            start = vertex_offset + j
            end = vertex_offset + (j + 1) % n
            segments.append([start, end])
            segment_markers.append(1)  # 边界标记
        # 计算洞的代表点（质心）
        if i > 0:  # 第一个为外环，其余为洞
            xs = [p[0] for p in poly]
            ys = [p[1] for p in poly]
            hole_x = sum(xs) / len(xs)
            hole_y = sum(ys) / len(ys)
            hole_points.append([hole_x, hole_y])
        vertex_offset += n

    tri_input = {
        'vertices': np.array(vertices),
        'segments': np.array(segments),
        'segment_markers': np.array(segment_markers)
    }
    if hole_points:
        tri_input['holes'] = np.array(hole_points)

    return tri_input


def write_triangles_to_shapefile(tri_output, dst_file):
    """将 triangle 输出写入 Shapefile"""
    vertices = tri_output['vertices']
    triangles = tri_output['triangles']

    driver = ogr.GetDriverByName("ESRI Shapefile")
    if os.path.exists(dst_file):
        driver.DeleteDataSource(dst_file)

    ds = driver.CreateDataSource(dst_file)
    layer = ds.CreateLayer("tin", geom_type=ogr.wkbPolygon)

    for tri in triangles:
        ring = ogr.Geometry(ogr.wkbLinearRing)
        for idx in tri:
            x, y = vertices[idx]
            ring.AddPoint(x, y)
        ring.CloseRings()

        poly = ogr.Geometry(ogr.wkbPolygon)
        poly.AddGeometry(ring)

        feat = ogr.Feature(layer.GetLayerDefn())
        feat.SetGeometry(poly)
        layer.CreateFeature(feat)

        feat = None
        poly = None
        ring = None

    ds = None


def main():
    # 定义三个多边形（与 C++ 一致）
    polygon1 = [
        (-0.558868038740926, -0.38960351089588),
        (2.77833686440678, 5.37465950363197),
        (6.97052814769976, 8.07751967312349),
        (13.9207400121065, 5.65046156174335),
        (15.5755523607748, -1.98925544794189),
        (6.36376361985472, -6.18144673123487)
    ]

    polygon2 = [
        (2.17935556413387, 1.4555590039808),
        (3.75630057749723, 4.02942327866582),
        (5.58700685737883, 4.71820385921534),
        (6.54767450919789, 1.76369768475295),
        (5.71388749063795, -0.900795613688593),
        (3.21252643495814, -0.320769861646896)
    ]

    polygon3 = [
        (7.74397762278389, 0.821155837685192),
        (9.13966458863422, 4.24693293568146),
        (10.1909612642098, 1.83620090375816),
        (12.1485481773505, 4.84508449247446),
        (11.4416417920497, -2.29648257953892),
        (10.1547096547072, 0.712401009177374)
    ]

    # triangle 要求：第一个是 outer boundary，其余是 holes（必须完全在内部）
    polygons = [polygon1, polygon2, polygon3]

    # 构造 triangle 输入
    tri_in = polygon_to_triangle_input(polygons)

    # 执行受约束 Delaunay 三角剖分（带洞）
    tri_out = triangle.triangulate(tri_in, 'p')  # 'p' = PSLG (Planar Straight Line Graph)

    # 输出路径
    gis_basic = os.getenv("GISBasicRepo")
    if not gis_basic:
        raise EnvironmentError("GISBasicRepo 环境变量未设置")
    dst_file = os.path.join(gis_basic, "Data", "Out.shp")

    # 启用异常处理
    gdal.UseExceptions()

    # 写入 Shapefile
    write_triangles_to_shapefile(tri_out, dst_file)
    print(f"三角剖分结果已保存至: {dst_file}")


if __name__ == "__main__":
    main()