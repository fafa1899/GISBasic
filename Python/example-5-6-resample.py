import os
import numpy as np
from osgeo import gdal
from osgeo import osr


# 0=Nearest
# 1=Bilinear
# 2=Bicubic
interpolation_method = 1


def bicubic_kernel(x):
    a = -0.5

    x = np.abs(x)

    result = np.zeros_like(x)

    mask1 = x <= 1
    result[mask1] = (
        1
        - (a + 3) * x[mask1] ** 2
        + (a + 2) * x[mask1] ** 3
    )

    mask2 = (x > 1) & (x <= 2)
    result[mask2] = (
        -4 * a
        + 8 * a * x[mask2]
        - 5 * a * x[mask2] ** 2
        + a * x[mask2] ** 3
    )

    return result


def read_image(path):

    ds = gdal.Open(path)

    if ds is None:
        raise RuntimeError("Open image failed")

    width = ds.RasterXSize
    height = ds.RasterYSize
    bands = ds.RasterCount

    geo = ds.GetGeoTransform()

    img = ds.ReadAsArray()

    if bands == 1:
        img = img[np.newaxis]

    img = np.transpose(img, (1, 2, 0))

    return ds, img, width, height, bands, geo


def get_dst_bound(ds, width, height):

    geo = ds.GetGeoTransform()

    start_x = geo[0]
    start_y = geo[3]

    dx = geo[1]
    dy = geo[5]

    end_x = start_x + dx * width
    end_y = start_y + dy * height

    xs = np.array(
        [start_x, end_x, end_x, start_x],
        dtype=np.float64
    )

    ys = np.array(
        [start_y, start_y, end_y, end_y],
        dtype=np.float64
    )

    src = osr.SpatialReference()
    src.ImportFromWkt(ds.GetProjection())

    dst = osr.SpatialReference()
    dst.ImportFromEPSG(3857)

    trans = osr.CoordinateTransformation(
        src,
        dst
    )

    xyz = trans.TransformPoints(
        np.column_stack([xs, ys])
    )

    xyz = np.array(xyz)

    xmin = xyz[:, 0].min()
    xmax = xyz[:, 0].max()

    ymin = xyz[:, 1].min()
    ymax = xyz[:, 1].max()

    return (
        xmin,
        ymin,
        xmax,
        ymax,
        src,
        dst,
        dx,
        dy,
        start_x,
        start_y
    )


def nearest(img, lx, ly):

    h, w, _ = img.shape

    x = np.rint(lx).astype(np.int32)
    y = np.rint(ly).astype(np.int32)

    x = np.clip(
        x,
        0,
        w - 1
    )

    y = np.clip(
        y,
        0,
        h - 1
    )

    return img[y, x]


def bilinear(img, lx, ly):

    h, w, _ = img.shape

    x0 = np.floor(lx).astype(np.int32)
    y0 = np.floor(ly).astype(np.int32)

    x1 = np.clip(x0 + 1, 0, w - 1)
    y1 = np.clip(y0 + 1, 0, h - 1)

    x0 = np.clip(x0, 0, w - 1)
    y0 = np.clip(y0, 0, h - 1)

    u = lx - x0
    v = ly - y0

    f00 = img[y0, x0]
    f10 = img[y0, x1]
    f01 = img[y1, x0]
    f11 = img[y1, x1]

    out = (
        f00 * (1-u)[..., None] * (1-v)[..., None]
        + f10 * u[..., None] * (1-v)[..., None]
        + f01 * (1-u)[..., None] * v[..., None]
        + f11 * u[..., None] * v[..., None]
    )

    return np.clip(out, 0, 255)


def bicubic(img, lx, ly):

    h, w, c = img.shape

    x0 = np.floor(lx).astype(np.int32)
    y0 = np.floor(ly).astype(np.int32)

    result = np.zeros(
        (*lx.shape, c),
        dtype=np.float32
    )

    for iy in range(-1, 3):

        wy = bicubic_kernel(
            ly - (y0 + iy)
        )

        yy = np.clip(
            y0 + iy,
            0,
            h - 1
        )

        for ix in range(-1, 3):

            wx = bicubic_kernel(
                lx - (x0 + ix)
            )

            xx = np.clip(
                x0 + ix,
                0,
                w - 1
            )

            weight = (
                wx * wy
            )[..., None]

            result += (
                img[yy, xx] * weight
            )

    return np.clip(
        result,
        0,
        255
    )


def resample(img,
             src,
             dst,
             bound,
             dx,
             dy,
             src_x,
             src_y):

    xmin, ymin, xmax, ymax = bound

    dst_x = np.floor(xmin / dx) * dx
    dst_y = np.ceil(ymax / dy) * dy

    end_x = np.ceil(xmax / dx) * dx
    end_y = np.floor(ymin / dy) * dy

    dst_w = int(
        (end_x - dst_x) / dx
    )

    dst_h = int(
        (end_y - dst_y) / dy
    )

    xs = dst_x + dx * (
        np.arange(dst_w) + 0.5
    )

    ys = dst_y + dy * (
        np.arange(dst_h) + 0.5
    )

    xx, yy = np.meshgrid(
        xs,
        ys
    )

    points = np.column_stack(
        [
            xx.ravel(),
            yy.ravel()
        ]
    )

    trans = osr.CoordinateTransformation(
        dst,
        src
    )

    xyz = trans.TransformPoints(
        points
    )

    xyz = np.array(xyz)

    lx = (
        xyz[:, 0]
        - (src_x + dx * 0.5)
    ) / dx

    ly = (
        xyz[:, 1]
        - (src_y + dy * 0.5)
    ) / dy

    valid = (
        (lx >= 0)
        & (lx < img.shape[1])
        & (ly >= 0)
        & (ly < img.shape[0])
    )

    out = np.zeros(
        (
            dst_h * dst_w,
            img.shape[2]
        ),
        dtype=np.uint8
    )

    lxv = lx[valid]
    lyv = ly[valid]

    if interpolation_method == 0:
        value = nearest(
            img,
            lxv,
            lyv
        )

    elif interpolation_method == 2:
        value = bicubic(
            img,
            lxv,
            lyv
        )

    else:
        value = bilinear(
            img,
            lxv,
            lyv
        )

    out[valid] = value

    out = out.reshape(
        dst_h,
        dst_w,
        img.shape[2]
    )

    return (
        out,
        dst_w,
        dst_h,
        dst_x,
        dst_y
    )


def write_image(
    path,
    img,
    width,
    height,
    bands,
    geo,
    proj
):

    driver = gdal.GetDriverByName(
        "GTiff"
    )

    ds = driver.Create(
        path,
        width,
        height,
        bands,
        gdal.GDT_Byte
    )

    ds.SetGeoTransform(geo)
    ds.SetProjection(proj)

    for b in range(bands):

        ds.GetRasterBand(
            b + 1
        ).WriteArray(
            img[:, :, b]
        )

    ds.FlushCache()
    ds = None

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

    gdal.AllRegister()

    setup_proj_lib()
    
    root = os.getenv("GISBasicRepo")
  
    src_path = os.path.join(root, "Data", "Raster", "berry_ali_2011127_crop_geo.tif")

    dst_path = os.path.join(root, "Data", "Raster", "dst.tif")

    ds, img, w, h, bands, geo = \
        read_image(src_path)

    (
        xmin,
        ymin,
        xmax,
        ymax,
        src,
        dst,
        dx,
        dy,
        src_x,
        src_y
    ) = get_dst_bound(
        ds,
        w,
        h
    )

    out, dw, dh, dst_x, dst_y = \
        resample(
            img,
            src,
            dst,
            (
                xmin,
                ymin,
                xmax,
                ymax
            ),
            dx,
            dy,
            src_x,
            src_y
        )

    geo = (
        dst_x,
        dx,
        0,
        dst_y,
        0,
        dy
    )

    write_image(
        dst_path,
        out,
        dw,
        dh,
        bands,
        geo,
        dst.ExportToWkt()
    )


if __name__ == "__main__":
    main()