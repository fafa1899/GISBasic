from osgeo import gdal
import os

# 启用异常处理
gdal.UseExceptions()

# 注册所有格式（通常在导入时自动完成，但显式调用更清晰）
gdal.AllRegister()

# 获取环境变量 GISBasic 并构造输出文件路径
gis_basic = os.getenv("GISBasicRepo")
if not gis_basic:
    raise EnvironmentError("环境变量 GISBasicRepo 未设置")

dst_file = os.path.join(gis_basic, "Data", "Raster", "dst.tif")

# 获取 GeoTIFF 驱动
driver = gdal.GetDriverByName("GTiff")
if driver is None:
    raise RuntimeError("无法获取 GTiff 驱动")

# 设置创建选项
options = ["BIGTIFF=IF_NEEDED"]

# 创建新栅格文件：256x256，3波段，字节型（GDT_Byte）
img_width = 256
img_height = 256
band_num = 3
data_type = gdal.GDT_Byte

dataset = driver.Create(dst_file, img_width, img_height, band_num, data_type, options)

if dataset is None:
    print("无法创建图像！")
    exit(1)

# 关闭数据集（释放资源）
dataset = None

print(f"成功创建空 GeoTIFF 文件: {dst_file}")