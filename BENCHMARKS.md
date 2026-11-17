# QLIC Lite benchmarks

The benchmarks all are on a single thread of a 9950x3d in memory.

## 3200 image corpus

This corpus contains 3200 images with 32 categories and 100 images per category.

| Codec | Size (MB) | Encode (MP/s) | Decode (MP/s) |
| --- | ---: | ---: | ---: |
| QLIC Lite (AVX2) | 2,267.68 | 75.67 | 247.81 |
| JPEG XL effort 7 | 2,270.20 | 1.83 | 10.57 |
| HTJ2K | 2,994.26 | 72.09 | 136.56 |
| libpng level 1 | 6,074.60 | 42.67 | 123.78 |

See [all 32 categories](benchmarks/categories.csv). There are weaknesses on small graphics, which I am aware of. The categories show in detail these weaknesses, however for now I believe these weaknesses are acceptable.

PNG uses libpng 1.6.59 with default filters, HTJ2K uses OpenJPH 0.33.0 with reversible coding and full resolution decoding, and JXL uses lossless libjxl 0.13.0.

## QLIC Lite CPU paths

This run uses the same 3200 image corpus and compares the other cpu paths. Portable C is just for compatibility. I wouldn't recommend using it but you can.

| CPU path | Encode MP/s | Decode MP/s |
| --- | ---: | ---: |
| AVX2 | 75.67 | 247.81 |
| SSE4.2 + POPCNT | 70.86 | 203.45 |
| Portable C | 24.89 | 63.61 |

These paths do give the same files.

## 2400 image RGB corpus

This corpus has 2400 RGB images with 24 categories and 100 images per category, for the codecs that I wanted to include but did not have RGBA support.

| Codec | Size (MB) | Encode (MP/s) | Decode (MP/s) |
| --- | ---: | ---: | ---: |
| QLIC Lite (AVX2) | 1,770.32 | 71.89 | 218.26 |
| JPEG XL effort 7 | 1,770.35 | 1.83 | 10.84 |
| fNBLI | 1,881.63 | 35.95 | 54.71 |
| L1C AVX2 effort 0 | 1,919.99 | 207.60 | 213.23 |
| HTJ2K | 2,282.39 | 67.07 | 135.96 |
| libpng level 1 | 3,876.12 | 45.42 | 121.79 |

FLIC, QLIC2, and HALIC were supposed to be benchmarked as well, however I couldn't do an in memory test as they are closed source, and to be honest I didn't feel like trying to reverse engineer 3 different image codecs for the sake of benchmarking them in memory, so they've been excluded. 

Also, while L1C has other effort settings, no effort setting changed the file sizes by a significant amount, and appeared to change the results across different categories, which fits what their documentation says. As a result, I simply chose the fastest result L1C was able to put out, as their file size improvements across effort levels were neglible on this corpus.
