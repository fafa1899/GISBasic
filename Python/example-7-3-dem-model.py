import os
import json
import struct
import sys
import numpy as np
from osgeo import gdal


point_num = 0
bin_buf_num = 0
indices_num = 0


def create_bin_file():
    global point_num
    global bin_buf_num
    global indices_num

    work_dir = os.environ["GISBasicRepo"]

    dem_path = os.path.join(work_dir, "Data", "Model", "dem.tif")

    img = gdal.Open(dem_path)

    if img is None:
        print("Can't Open Image!")
        return

    buf_width = img.RasterXSize
    buf_height = img.RasterYSize
    band_num = img.RasterCount

    if band_num != 1:
        print("DEM波段数不为1")
        return

    band = img.GetRasterBand(1)

    geo = img.GetGeoTransform()

    start_x = geo[0]
    dx = geo[1]
    start_y = geo[3]
    dy = geo[5]

    img_buf = band.ReadAsArray().astype(np.float32)

    point_num = buf_width * buf_height

    # xyz + uv
    position_texture = np.zeros(
        (point_num, 5),
        dtype=np.float32
    )

    for yi in range(buf_height):

        y = dy * yi
        v = yi / (buf_height - 1)

        for xi in range(buf_width):

            idx = yi * buf_width + xi

            position_texture[idx, 0] = dx * xi
            position_texture[idx, 1] = y
            position_texture[idx, 2] = img_buf[yi, xi]

            position_texture[idx, 3] = xi / (buf_width - 1)
            position_texture[idx, 4] = v

    bin_path = os.path.join(work_dir, "Data", "Model", "dem.bin")

    with open(bin_path, "wb") as f:

        vertex_bytes = position_texture.tobytes()

        f.write(vertex_bytes)

        vertex_buf_num = len(vertex_bytes)
        bin_buf_num += vertex_buf_num

        # 2字节对齐
        mod = vertex_buf_num % 2

        if mod != 0:

            space_num = 4 - mod

            f.write(bytes(space_num))

            bin_buf_num += space_num

        indices_num = (
            (buf_width - 1)
            * (buf_height - 1)
            * 2
            * 3
        )

        indices = np.zeros(
            indices_num,
            dtype=np.uint16
        )

        for yi in range(buf_height - 1):

            for xi in range(buf_width - 1):

                m00 = buf_width * yi + xi
                m01 = buf_width * (yi + 1) + xi
                m11 = buf_width * (yi + 1) + xi + 1
                m10 = buf_width * yi + xi + 1

                n = yi * (buf_width - 1) + xi

                base = n * 6

                indices[base + 0] = m00
                indices[base + 1] = m01
                indices[base + 2] = m11

                indices[base + 3] = m11
                indices[base + 4] = m10
                indices[base + 5] = m00

        idx_bytes = indices.tobytes()

        f.write(idx_bytes)

        bin_buf_num += len(idx_bytes)

    xyz = position_texture[:, :3]

    return {
        "max_pos": xyz.max(axis=0).tolist(),
        "min_pos": xyz.min(axis=0).tolist(),
        "max_index": int(indices.max())
    }

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

    global point_num
    global bin_buf_num
    global indices_num

    gdal.AllRegister()

    setup_proj_lib()

    # 设置 PROJ_LIB 环境变量
    work_dir = os.getenv("GISBasicRepo") 
    if not work_dir:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        sys.exit(1)

    stat = create_bin_file()

    gltf = {}

    gltf["asset"] = {
        "generator": "CL",
        "version": "2.0"
    }

    gltf["scene"] = 0

    gltf["scenes"] = [
        {
            "nodes": [0]
        }
    ]

    gltf["nodes"] = [
        {
            "mesh": 0
        }
    ]

    gltf["meshes"] = [
        {
            "primitives": [
                {
                    "attributes": {
                        "POSITION": 1,
                        "TEXCOORD_0": 2
                    },
                    "indices": 0,
                    "material": 0
                }
            ]
        }
    ]

    gltf["materials"] = [
        {
            "pbrMetallicRoughness": {
                "baseColorTexture": {
                    "index": 0
                }
            }
        }
    ]

    gltf["textures"] = [
        {
            "sampler": 0,
            "source": 0
        }
    ]

    gltf["images"] = [
        {
            "uri": "tex.jpg"
        }
    ]

    gltf["samplers"] = [
        {
            "magFilter": 9729,
            "minFilter": 9987,
            "wrapS": 33648,
            "wrapT": 33648
        }
    ]

    gltf["buffers"] = [
        {
            "uri": "dem.bin",
            "byteLength": bin_buf_num
        }
    ]

    vertex_bytes = point_num * 5 * 4

    gltf["bufferViews"] = [

        {
            "buffer": 0,
            "byteOffset": vertex_bytes,
            "byteLength": indices_num * 2,
            "target": 34963
        },

        {
            "buffer": 0,
            "byteStride": 20,
            "byteOffset": 0,
            "byteLength": vertex_bytes,
            "target": 34962
        }

    ]

    gltf["accessors"] = [

        {
            "bufferView": 0,
            "byteOffset": 0,
            "componentType": 5123,
            "count": indices_num,
            "type": "SCALAR",
            "max": [stat["max_index"]],
            "min": [0]
        },

        {
            "bufferView": 1,
            "byteOffset": 0,
            "componentType": 5126,
            "count": point_num,
            "type": "VEC3",
            "max": stat["max_pos"],
            "min": stat["min_pos"]
        },

        {
            "bufferView": 1,
            "byteOffset": 12,
            "componentType": 5126,
            "count": point_num,
            "type": "VEC2",
            "max": [1, 1],
            "min": [0, 0]
        }

    ]

    gltf_path = os.path.join(work_dir, "Data", "Model", "dem.gltf")

    with open(
        gltf_path,
        "w",
        encoding="utf-8"
    ) as f:

        json.dump(
            gltf,
            f,
            indent=4
        )

    print("完成")
    print("顶点:", point_num)
    print("索引:", indices_num)


if __name__ == "__main__":
    main()