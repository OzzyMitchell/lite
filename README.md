# QLIC Lite

QLIC Lite is a lossless image codec built for a mix of fast decoding with good file sizes.

## CLI

To use the CLI, there are 3 commands and 2 flags.

```text
pack
unpack
batch
--parallel
--heap_limit
```

`pack` will encode an input into a .lite file.

`unpack` will decode a .lite file into an output.

`batch` will encode the images in a folder into .lite files. Output files will be beside the original files.

`--parallel` allows multiple workers. If you set a number after it (like `--parallel=4`), it will use up to 4 workers. If you use `--parallel` on its own, it will use all logical CPUs if there are enough images for each worker.

`--heap_limit` lets you set the heap limit for each image. By default, it's 1 GiB. There's no need to change this unless a particular image requires a lot more memory.

```text
qlic_lite pack image.ppm --heap_limit=512MiB
qlic_lite batch "C:\Images"
qlic_lite batch "C:\Images" --parallel
qlic_lite batch "C:\Images" --parallel=4
```

On windows, extract the zip and double click `qlic_lite.exe`, keep the included DLLs beside the exe, and when it asks for a folder, paste the folder path and press enter.

QLIC Lite supports importing png, bmp, tiff, netpbm, tcom, bin, jpeg, webp, and jxl, it supports exporting png (effort 1), uncompressed bmp and tiff, netpbm, bin, and tcom. (TCOM exports are 8 bit only and omit HDR metadata.) Raw bin input needs width and height.

See [BUILDING.md](https://github.com/OzzyMitchell/lite/blob/main/BUILDING.md) for the build commands and dependencies.

## Benchmarks

### 3200 image corpus

The benchmark was done on a 9950x3d, single threaded, on a dataset of 3200 images, 32 categories, 100 image per category.

| Codec | Total size (MB) | Encode (MP/s) | Decode (MP/s) |
|---|---:|---:|---:|
| QLIC Lite | 2,267.68 | 75.67 | 247.81 |
| JPEG XL effort 7 | 2,270.20 | 1.83 | 10.57 |
| HTJ2K | 2,994.26 | 72.09 | 136.56 |
| libpng level 1 | 6,074.60 | 42.67 | 123.78 |

### 2400 image RGB corpus

The RGB dataset has 24 categories with 100 images per category.

| Codec | Total size (MB) | Encode (MP/s) | Decode (MP/s) |
|---|---:|---:|---:|
| QLIC Lite | 1,770.32 | 71.89 | 218.26 |
| JPEG XL effort 7 | 1,770.35 | 1.83 | 10.84 |
| fNBLI | 1,881.63 | 35.95 | 54.71 |
| L1C AVX2 effort 0 | 1,919.99 | 207.60 | 213.23 |
| HTJ2K | 2,282.39 | 67.07 | 135.96 |
| libpng level 1 | 3,876.12 | 45.42 | 121.79 |

[Full benchmark results](BENCHMARKS.md)

Copyright 2026 Ozzy M. The decoder (`qlic_lite_decoder.c` and `qlic_lite_decoder.h`) is licensed under [Apache 2.0](LICENSE-APACHE-2.0). The rest of QLIC Lite 1.0 is licensed under [AGPLv3](LICENSE).
