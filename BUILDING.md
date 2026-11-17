# Build QLIC Lite

C17 and CMake 3.25 or newer. The full CLI uses libpng, libtiff 4.7+, turbojpeg 3.2+, libwebp/libwebpdecoder, and libjxl 0.12+. The two codec libraries build independently of said dependencies.

## Windows x64

For the full CLI, you'll need to use an MSYS2 UCRT64 shell with GCC, CMake, Ninja, and the UCRT64 versions of the libraries.

~~~sh
cmake -S . -B build-windows -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-windows --parallel 1
./build-windows/qlic_lite.exe --help
~~~

The raw only CLI uses an x64 Visual Studio Developer Command Prompt with clang-cl, lld-link, and Ninja.

~~~bat
cmake --preset windows-x64 -DQLIC_LITE_IMAGE_IO=OFF
cmake --build --preset windows-x64
native-windows-x64\qlic_lite.exe --help
~~~

## Linux

For the full CLI, use a linux shell with clang, CMake, Make or Ninja, and the development packages for the libraries.

~~~sh
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang
cmake --build build-linux --parallel 1
./build-linux/qlic_lite --help
~~~

The raw CLI just uses clang.

~~~sh
clang -std=c17 -O2 qlic_lite_full.c -lm -pthread -o qlic_lite
~~~

## Build targets

| CMake target | Output |
| --- | --- |
| `qlic_lite_full` | CLI with the codec built in |
| `qlic_lite` | Full codec library |
| `qlic_lite_cli_adapter` | CLI that uses the library |
| `qlic_lite_decoder` | Decode only library |

## Image formats

| Format | Import | Export |
| --- | --- | --- |
| png | Yes | Level 1 |
| bmp | Yes | Uncompressed 8 bit |
| tiff | Single page | Uncompressed |
| netpbm | Yes | Binary |
| bin | Raw RGBA8, needs width and height | Raw samples |
| tcom | 8 bit | 8 bit |
| jpeg, webp, jxl | Yes | No |
