import os
import sys
from osgeo import gdal, osr
import numpy as np
import random

def main():
    dem_column = 512
    dem_row = 512

    # 注册所有 GDAL 支持的格式驱动
    gdal.AllRegister()

    # 拼接输出 DEM 文件路径
    gis_basic = os.getenv("GISBasicRepo")
    if not gis_basic:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        return
    dem_path = os.path.join(gis_basic, "Data", "Terrain", "dst.tif")

    # 获取 GTiff 图像驱动
    driver = gdal.GetDriverByName("GTIFF")
    
    # 创建新的 DEM 数据集 (对应 C++ 的 pDriver->Create)
    # 参数: 路径, 列数, 行数, 波段数, 数据类型, 选项
    dem = driver.Create(dem_path, dem_column, dem_row, 1, gdal.GDT_Float32, options=['BIGTIFF=IF_NEEDED'])
    if not dem:
        print("Can't Write Image!")
        return

    # --- 设置空间参考 ---
    # 对应 C++: spatialReference.importFromEPSG(3857);
    spatial_reference = osr.SpatialReference()
    spatial_reference.ImportFromEPSG(3857)  # web墨卡托坐标系
    dem.SetProjection(spatial_reference.ExportToWkt())

    # --- 设置坐标信息 (地理变换参数) ---
    # 对应 C++: dem->SetGeoTransform(padfTransform);
    geo_transform = [
        5,    # 左上角点坐标X
        10,   # X方向的分辨率
        0,    # 旋转系数
        95,   # 左上角点坐标Y
        0,    # 旋转系数
        -10   # Y方向的分辨率
    ]
    dem.SetGeoTransform(geo_transform)

    # --- 设置无效值 (NoData Value) ---
    band = dem.GetRasterBand(1)
    no_data_value = -99
    band.SetNoDataValue(no_data_value)

    # --- 生成高程数据 ---
    # 对应 C++ 的 vector<float> demBuf 和 rand() 赋值
    dem_buf = np.zeros((dem_row, dem_column), dtype=np.float32)
    for yi in range(dem_row):
        for xi in range(dem_column):
            dem_buf[yi, xi] = random.randint(0, 99)

    # 将右下角特定区域设置为无效值
    # 对应 C++: for (int yi = 0; yi < 100; yi++) { for (int xi = 400; xi < demColumn; xi++) ... }
    dem_buf[0:100, 400:dem_column] = no_data_value

    # --- 写入数据 ---
    # 直接将 numpy 数组写入波段 (对应 C++ 的 RasterIO)
    band.WriteArray(dem_buf)
    
    # 刷新缓存，确保数据写入磁盘
    band.FlushCache()

    # 释放数据集对象
    dem = None
    print(f"DEM 文件已成功创建并保存至: {dem_path}")

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
    osr.UseExceptions() # 启用异常处理（推荐），避免静默错误
    setup_proj_lib() #设置 PROJ_LIB 
    main()