from osgeo import gdal
import os

# 注册所有支持的格式（在 Python 中通常自动完成，但显式调用也无妨）
gdal.AllRegister()

# 获取环境变量 GISBasic 并构造文件路径
gis_basic = os.getenv("GISBasicRepo")
if not gis_basic:
    raise EnvironmentError("环境变量 GISBasicRepo 未设置")

src_file = os.path.join(gis_basic, "Data", "Raster", "berry_ali_2011127_crop_geo.tif")

# 打开栅格数据集
dataset = gdal.Open(src_file, gdal.GA_ReadOnly)
if dataset is None:
    print("无法打开文件:", src_file)
    exit(1)

# 获取图像信息
img_width = dataset.RasterXSize      # 图像宽度
img_height = dataset.RasterYSize     # 图像高度
band_num = dataset.RasterCount       # 波段数

# 获取第一个波段的数据类型，并计算每个像素的字节数（深度）
band = dataset.GetRasterBand(1)
data_type = band.DataType
depth = gdal.GetDataTypeSize(data_type) // 8  # 转换为字节

# 输出信息
print(f"宽度：{img_width}")
print(f"高度：{img_height}")
print(f"波段数：{band_num}")
print(f"深度：{depth}")

# 关闭数据集（Python 中通常靠引用计数自动释放，但显式置 None 更安全）
dataset = None