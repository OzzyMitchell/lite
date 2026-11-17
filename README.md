# QLIC Lite

QLIC Lite is a lossless image codec built for a mix of fast decoding with good file sizes.

## CLI

To use the CLI, you use `pack` and `unpack` like below.

```text
qlic_lite pack image.png
qlic_lite unpack image.lite restored.png
```

On windows, extract the zip and double click `qlic_lite.exe`, keep the included DLLs beside the exe, and when it asks for a folder, paste the folder path and press enter.

Running `qlic_lite` on its own opens the same prompt, and you can also do the folder directly with `qlic_lite batch "path/to/folder"` which will save `.lite` files next to the images in said folder.

QLIC Lite supports importing png, bmp, tiff, netpbm, tcom, bin, jpeg, webp, and jxl, it supports exporting png (effort 1), uncompressed bmp and tiff, netpbm, bin, and tcom. (TCOM does not support HDR, so it will not work with HDR images.)

See [BUILDING.md](BUILDING.md) for the build commands and dependencies.

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
