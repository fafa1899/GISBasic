import os
import math
import numpy as np
from osgeo import gdal,osr

# 全局变量模拟
startX = startY = endX = endY = 0.0
srcDemWidth = srcDemHeight = 0
srcDx = srcDy = 0.0
srcDemBuf = None

dstDx = 100
dstDy = -100
dstDemColumn = dstDemRow = 0
dstNoDataValue = -32768
dstDemBuf = None

def read_dem():
    global startX, startY, endX, endY, srcDemWidth, srcDemHeight, srcDx, srcDy, srcDemBuf
    
    work_dir = os.getenv("GISBasicRepo") # 对应你之前的环境变量
    dem_path = os.path.join(work_dir, "Data", "Terrain", "dem.tif")

    dem = gdal.Open(dem_path, gdal.GA_ReadOnly)
    if not dem:
        print("Can't Open Image!")
        return

    srcDemWidth = dem.RasterXSize
    srcDemHeight = dem.RasterYSize

    # 坐标信息
    geo_transform = dem.GetGeoTransform()
    srcDx = geo_transform[1]
    srcDy = geo_transform[5]
    startX = geo_transform[0] + 0.5 * srcDx
    startY = geo_transform[3] + 0.5 * srcDy
    endX = startX + (srcDemWidth - 1) * srcDx
    endY = startY + (srcDemHeight - 1) * srcDy

    # 读取数据到 numpy 数组 (对应 C++ 的 vector<float>)
    srcDemBuf = dem.GetRasterBand(1).ReadAsArray().astype(np.float32)
    dem = None

# 双线性插值 (完全对应 C++ 的 Bilinear 函数)
def bilinear(lx, ly, xi, yi):
    global dstDemBuf
    x0 = min(max(math.floor(lx), 0), srcDemWidth - 1)
    y0 = min(max(math.floor(ly), 0), srcDemHeight - 1)
    x1 = min(max(x0 + 1, 0), srcDemWidth - 1)
    y1 = min(max(y0 + 1, 0), srcDemHeight - 1)

    u = lx - x0
    v = ly - y0

    f00 = srcDemBuf[y0, x0]
    f10 = srcDemBuf[y0, x1]
    f01 = srcDemBuf[y1, x0]
    f11 = srcDemBuf[y1, x1]

    value = f00 * (1 - u) * (1 - v) + \
            f10 * u * (1 - v) + \
            f01 * (1 - u) * v + \
            f11 * u * v

    dstDemBuf[yi, xi] = float(value)

def resample():
    global dstDemColumn, dstDemRow, dstDemBuf
    dstDemColumn = int((endX - startX) / dstDx + 1)
    dstDemRow = int((endY - startY) / dstDy + 1)

    # 初始化目标数组，填充 NoData 值
    dstDemBuf = np.full((dstDemRow, dstDemColumn), dstNoDataValue, dtype=np.float32)

    for yi in range(dstDemRow):
        for xi in range(dstDemColumn):
            lx = startX + dstDx * xi
            ly = startY + dstDy * yi
            # 转换为源图像的像素坐标
            lx = (lx - startX) / srcDx
            ly = (ly - startY) / srcDy

            if lx < 0 or lx > srcDemWidth - 1 or ly < 0 or ly > srcDemHeight - 1:
                continue

            bilinear(lx, ly, xi, yi)

def write_dem():
    work_dir = os.getenv("GISBasicRepo")
    dem_path = os.path.join(work_dir, "Data", "Terrain", "dst.tif")

    driver = gdal.GetDriverByName("GTIFF")
    # 创建输出文件
    dem = driver.Create(dem_path, dstDemColumn, dstDemRow, 1, gdal.GDT_Float32, options=['BIGTIFF=IF_NEEDED'])
    if not dem:
        print("Can't Write Image!")
        return

    # 坐标信息 (注意左上角坐标的计算)
    padf_transform = [
        startX - 0.5 * dstDx,  # 左上角点坐标X
        dstDx,                 # X方向的分辨率
        0,                     # 旋转系数
        startY - 0.5 * dstDy,  # 左上角点坐标Y
        0,                     # 旋转系数
        dstDy                  # Y方向的分辨率
    ]
    dem.SetGeoTransform(padf_transform)
    dem.GetRasterBand(1).SetNoDataValue(dstNoDataValue)

    # 写入数据
    dem.GetRasterBand(1).WriteArray(dstDemBuf)
    dem = None

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
    osr.UseExceptions() # 启用异常处理（推荐），避免静默错误
    gdal.AllRegister()     
    setup_proj_lib()   #设置 PROJ_LIB
    read_dem()
    resample()
    write_dem()
    print("重采样完成！")

if __name__ == "__main__":
    main()