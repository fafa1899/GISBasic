import os
import sys
from osgeo import gdal
from osgeo import osr
import numpy as np

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

def main():
    # 注册所有 GDAL 支持的格式驱动
    gdal.AllRegister()

    # 拼接 DEM 文件路径
    gis_basic = os.getenv("GISBasicRepo")
    if not gis_basic:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        return 1
    dem_path = os.path.join(gis_basic, "Data", "Terrain", "dem.tif")

    # 以只读模式打开 DEM 数据集
    dem = gdal.Open(dem_path, gdal.GA_ReadOnly)
    if not dem:
        print("Can't Open Image!")
        return

    # 获取 DEM 的基本信息
    dem_width = dem.RasterXSize
    dem_height = dem.RasterYSize
    band_num = dem.RasterCount

    print(f"DEM列数：{dem_width}")
    print(f"DEM行数：{dem_height}")
    print(f"数据集波段数：{band_num}")

    # 获取第一个波段的数据类型名称
    band1 = dem.GetRasterBand(1)
    string_data_type = gdal.GetDataTypeName(band1.DataType)
    print(f"DEM数据类型：{string_data_type}")

    # 获取无效值 (NoData Value)
    no_data_value = band1.GetNoDataValue()
    if no_data_value is None:
        print("DEM无效值：None (未设置)")
    else:
        print(f"DEM无效值：{no_data_value}")

    # 获取空间参考坐标系 (WKT字符串)
    print("DEM空间参考坐标系：")
    wkt_string = dem.GetProjection()
    print(wkt_string)

    # 获取地理变换参数（坐标信息）
    geo_transform = dem.GetGeoTransform()
    # geo_transform 包含 [起点X, 像素宽, 旋转参数, 起点Y, 旋转参数, 像素高]
    dx = geo_transform[1]
    dy = geo_transform[5]
    # 计算左上角第一个像素的中心点坐标
    start_x = geo_transform[0] + 0.5 * dx
    start_y = geo_transform[3] + 0.5 * dy

    print(f"DEM间距：{dx}\t{dy}")
    print(f"DEM左上角起点位置：{start_x}\t{start_y}")

    # 读取 DEM 高程数据
    print("读取DEM高程...")
    # 直接将数据读取为 NumPy 数组 (对应 C++ 的 vector<float>)
    # ReadAsArray 默认会读取为 float32 (如果源数据是 float32)
    dem_buf = band1.ReadAsArray(0, 0, dem_width, dem_height).astype(np.float32)
    print("完成")
    
    # 打印前几个高程值作为验证（可选）
    # print("前5个高程值:", dem_buf.flatten()[:5])

    # 释放数据集对象
    dem = None

if __name__ == "__main__":
    osr.UseExceptions() # 启用异常处理（推荐），避免静默错误
    setup_proj_lib() #设置 PROJ_LIB 
    main()