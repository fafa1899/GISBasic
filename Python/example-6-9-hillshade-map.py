import os
import sys
import math
import numpy as np
from osgeo import gdal

# =========================================================
# 常量
# =========================================================

PI = 3.14159265358979323846
D2R = PI / 180.0

# =========================================================
# 全局变量
# =========================================================

dem_width = 0
dem_height = 0

geo_transform = [0] * 6

start_x = 0.0
dx = 0.0

start_y = 0.0
dy = 0.0

no_value = 0.0

dem_buf = None
dst_buf = None

# 颜色查找表 LUT
table_rgb = np.zeros((256, 3), dtype=np.float32)

# =========================================================
# 颜色渐变
# =========================================================

def gradient(start, end, count):
    start = np.array(start, dtype=np.float32)
    end = np.array(end, dtype=np.float32)

    t = np.linspace(
        0.0,
        1.0,
        count,
        endpoint=False,
        dtype=np.float32
    ).reshape(-1, 1)

    return start + (end - start) * t


# =========================================================
# 初始化颜色表
# =========================================================

def init_color_table():
    global table_rgb

    blue = [17, 60, 235]
    green = [17, 235, 86]
    yellow = [235, 173, 17]
    red = [235, 60, 17]
    purple = [235, 17, 235]

    table_rgb[0:60] = gradient(blue, green, 60)
    table_rgb[60:120] = gradient(green, yellow, 60)
    table_rgb[120:180] = gradient(yellow, red, 60)
    table_rgb[180:256] = gradient(red, purple, 76)


# =========================================================
# 读取 DEM
# =========================================================

def read_dem():
    global dem_width
    global dem_height
    global geo_transform
    global start_x
    global dx
    global start_y
    global dy
    global no_value
    global dem_buf

    work_dir = os.getenv("GISBasicRepo")

    if not work_dir:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        sys.exit(1)

    dem_path = os.path.join(
        work_dir,
        "Data",
        "Terrain",
        "dem.tif"
    )

    dem = gdal.Open(dem_path, gdal.GA_ReadOnly)

    if not dem:
        print("Can't Open Image!")
        sys.exit(1)

    dem_width = dem.RasterXSize
    dem_height = dem.RasterYSize

    geo_transform = dem.GetGeoTransform()

    start_x = geo_transform[0]
    dx = geo_transform[1]

    start_y = geo_transform[3]
    dy = geo_transform[5]

    band = dem.GetRasterBand(1)

    no_value = band.GetNoDataValue()

    # float32
    dem_buf = band.ReadAsArray().astype(np.float32)

    dem = None

    print(f"DEM Size: {dem_width} x {dem_height}")


# =========================================================
# Hillshade + 彩色渲染
# =========================================================

def cal_hillshade_vectorized(z_factor=1.0):

    global dst_buf

    # =====================================================
    # 光照参数
    # =====================================================

    solar_altitude = 45.0
    solar_azimuth = 315.0

    zenith_rad = (90.0 - solar_altitude) * D2R

    azimuth_math = 360.0 - solar_azimuth + 90.0

    if azimuth_math >= 360.0:
        azimuth_math -= 360.0

    azimuth_rad = azimuth_math * D2R

    # =====================================================
    # Horn 3x3
    #
    # a b c
    # d e f
    # g h i
    # =====================================================

    a = dem_buf[0:-2, 0:-2]
    b = dem_buf[0:-2, 1:-1]
    c = dem_buf[0:-2, 2:]

    d = dem_buf[1:-1, 0:-2]
    e = dem_buf[1:-1, 1:-1]
    f = dem_buf[1:-1, 2:]

    g = dem_buf[2:, 0:-2]
    h = dem_buf[2:, 1:-1]
    i = dem_buf[2:, 2:]

    # =====================================================
    # dzdx dzdy
    # =====================================================

    dzdx = (
        (c + 2.0 * f + i) -
        (a + 2.0 * d + g)
    ) / (8.0 * dx)

    dzdy = (
        (g + 2.0 * h + i) -
        (a + 2.0 * b + c)
    ) / (8.0 * abs(dy))

    # =====================================================
    # slope
    # =====================================================

    slope_rad = np.arctan(
        z_factor *
        np.sqrt(dzdx * dzdx + dzdy * dzdy)
    )

    # =====================================================
    # aspect
    # =====================================================

    aspect_rad = np.arctan2(dzdy, -dzdx)

    aspect_rad = np.where(
        aspect_rad < 0.0,
        2.0 * PI + aspect_rad,
        aspect_rad
    )

    mask = np.abs(dzdx) < 1e-9

    aspect_rad = np.where(
        mask,
        np.where(
            dzdy > 0.0,
            PI / 2.0,
            np.where(
                dzdy < 0.0,
                2.0 * PI - PI / 2.0,
                0.0
            )
        ),
        aspect_rad
    )

    # =====================================================
    # hillshade
    # =====================================================

    hillshade = 255.0 * (
        np.cos(zenith_rad) * np.cos(slope_rad) +
        np.sin(zenith_rad) *
        np.sin(slope_rad) *
        np.cos(azimuth_rad - aspect_rad)
    )

    hillshade = np.clip(
        hillshade,
        0.0,
        255.0
    )

    # =====================================================
    # 高程颜色 LUT
    # =====================================================

    valid_dem = dem_buf

    if no_value is not None:
        valid_dem = dem_buf[dem_buf != no_value]

    min_z = np.min(valid_dem)
    max_z = np.max(valid_dem)

    color_index = (
        (e - min_z) * 255.0 / (max_z - min_z) + 0.6
    ).astype(np.int32)

    color_index = np.clip(
        color_index,
        0,
        255
    )

    # LUT 查色
    dem_color = table_rgb[color_index]

    # =====================================================
    # 阴影混合
    # =====================================================

    alpha = 0.3

    hillshade_3 = hillshade[:, :, None]

    rgb = (
        hillshade_3 * alpha +
        dem_color * (1.0 - alpha)
    )

    rgb = np.clip(
        rgb,
        0.0,
        255.0
    ).astype(np.uint8)

    # =====================================================
    # RGBA 输出
    # =====================================================

    dst_buf = np.zeros(
        (dem_height, dem_width, 4),
        dtype=np.uint8
    )

    dst_buf[1:-1, 1:-1, 0:3] = rgb
    dst_buf[1:-1, 1:-1, 3] = 255

    # =====================================================
    # NoData 透明
    # =====================================================

    if no_value is not None:

        nodata_mask = (dem_buf == no_value)

        dst_buf[nodata_mask, 0] = 0
        dst_buf[nodata_mask, 1] = 0
        dst_buf[nodata_mask, 2] = 0
        dst_buf[nodata_mask, 3] = 0

    print("Hillshade Finished")


# =========================================================
# 输出 TIFF
# =========================================================

def write_dst():

    work_dir = os.getenv("GISBasicRepo")

    output_path = os.path.join(
        work_dir,
        "Data",
        "Terrain",
        "dst.tif"
    )

    driver = gdal.GetDriverByName("GTiff")

    dst = driver.Create(
        output_path,
        dem_width,
        dem_height,
        4,
        gdal.GDT_Byte,
        options=[
            'BIGTIFF=IF_NEEDED',
            'COMPRESS=LZW'
        ]
    )

    if not dst:
        print("Can't Write Image!")
        return

    dst.SetGeoTransform(geo_transform)

    # 如果原DEM有投影，继承
    work_dir = os.getenv("GISBasicRepo")

    dem_path = os.path.join(
        work_dir,
        "Data",
        "Terrain",
        "dem.tif"
    )

    src = gdal.Open(dem_path)

    if src:
        projection = src.GetProjection()

        if projection:
            dst.SetProjection(projection)

        src = None

    # 写RGBA
    for i in range(4):
        band = dst.GetRasterBand(i + 1)

        band.WriteArray(dst_buf[:, :, i])

    dst.FlushCache()

    dst = None

    print("彩色 Hillshade 已保存:")
    print(output_path)


# =========================================================
# 自动设置 PROJ_LIB
# =========================================================

def setup_proj_lib():

    if 'CONDA_PREFIX' in os.environ:

        conda_prefix = os.environ['CONDA_PREFIX']

        # Windows
        proj_path = os.path.join(
            conda_prefix,
            'Library',
            'share',
            'proj'
        )

        if not os.path.exists(proj_path):

            # Linux/macOS
            proj_path = os.path.join(
                conda_prefix,
                'share',
                'proj'
            )

        if os.path.exists(proj_path):

            os.environ['PROJ_LIB'] = proj_path

            print("PROJ_LIB =", proj_path)

            return True

    return False


# =========================================================
# Main
# =========================================================

if __name__ == "__main__":

    gdal.AllRegister()

    setup_proj_lib()

    init_color_table()

    read_dem()

    cal_hillshade_vectorized()

    write_dst()