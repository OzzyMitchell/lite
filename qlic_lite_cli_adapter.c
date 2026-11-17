/* Copyright (C) 2026 Ozzy M.
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <wchar.h>
#ifdef QLIC_LITE_IMAGE_IO
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#elif defined(QLIC_LITE_IMAGE_IO)
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef QLIC_LITE_IMAGE_IO
#include <ctype.h>
#include <limits.h>
#include <setjmp.h>
#include <jxl/decode.h>
#include <png.h>
#include <tiffio.h>
#include <turbojpeg.h>
#include <webp/decode.h>
#include <zlib.h>
#endif

#ifndef QLIC_LITE_H
#define QLIC_LITE_H



#ifndef QLIC_LITE_DECODER_H
#define QLIC_LITE_DECODER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    QLIC_LITE_OK, QLIC_LITE_INVALID, QLIC_LITE_LIMIT, QLIC_LITE_ALLOCATOR,
    QLIC_LITE_REENTRANT, QLIC_LITE_INTERNAL
} QlicLiteStatus;

enum {
    QLIC_LITE_ALPHA_NONE, QLIC_LITE_ALPHA_STRAIGHT, QLIC_LITE_ALPHA_PREMULTIPLIED
};

enum {
    QLIC_LITE_COLOR_UNSPECIFIED, QLIC_LITE_COLOR_ICC, QLIC_LITE_COLOR_CICP,
    QLIC_LITE_COLOR_ICC_PREFERRED, QLIC_LITE_COLOR_CICP_PREFERRED
};

typedef struct {
    uint16_t primaries, transfer, matrix;
    uint8_t full_range, reserved;
} QlicLiteCicp;

typedef struct {
    uint16_t x[3], y[3], white_x, white_y;
    uint32_t max_luminance, min_luminance;
} QlicLiteMastering;

typedef struct {
    uint32_t width, height, bits, channels, alpha, authority;
    uint32_t has_cicp, has_mastering, has_light;
    QlicLiteCicp cicp;
    QlicLiteMastering mastering;
    uint32_t max_cll, max_fall;
    const uint8_t *icc;
    size_t icc_size;
} QlicLiteDescription;

typedef void *(*QlicLiteAllocate)(size_t bytes, void *user);
typedef void (*QlicLiteRelease)(void *pointer, void *user);

typedef struct {
    QlicLiteAllocate allocate;
    QlicLiteRelease release;
    void *user;
} QlicLiteAllocator;

typedef struct {
    size_t input_bytes, pixels, pixel_data_bytes;
    size_t file_bytes;
    size_t metadata_bytes;
    size_t heap_bytes, allocation_requests;
} QlicLiteLimits;

typedef struct {
    size_t requests, allocator_calls, peak_bytes, retained_bytes, limit_denials;
    size_t allocator_failures, internal_errors;
} QlicLiteStats;

typedef struct {
    uint8_t *samples;
    size_t bytes;
    QlicLiteDescription description;
    QlicLiteRelease release;
    void *user;
} QlicLiteImage;

#if defined(QLIC_LITE_BUILD_SHARED) && defined(_WIN32)
#define QLIC_LITE_PUBLIC __declspec(dllexport)
#elif defined(QLIC_LITE_BUILD_SHARED) && (defined(__GNUC__) || defined(__clang__))
#define QLIC_LITE_PUBLIC __attribute__((visibility("default")))
#else
#define QLIC_LITE_PUBLIC
#endif

QLIC_LITE_PUBLIC void qlic_lite_limits_default(QlicLiteLimits *limits);
QLIC_LITE_PUBLIC QlicLiteStatus qlic_lite_decode(const uint8_t *data, size_t bytes,
                               const QlicLiteLimits *limits,
                               const QlicLiteAllocator *allocator,
                               QlicLiteImage *image, QlicLiteStats *stats);
QLIC_LITE_PUBLIC void qlic_lite_image_free(QlicLiteImage *image);

#undef QLIC_LITE_PUBLIC

#ifdef __cplusplus
}
#endif
#endif

typedef struct {
    uint8_t *data;
    size_t bytes;
    QlicLiteRelease release;
    void *user;
} QlicLiteFile;

#ifndef QLIC_LITE_DECODER_ONLY
#ifdef __cplusplus
extern "C" {
#endif

#if defined(QLIC_LITE_BUILD_SHARED) && defined(_WIN32)
#define QLIC_LITE_PUBLIC __declspec(dllexport)
#elif defined(QLIC_LITE_BUILD_SHARED) && (defined(__GNUC__) || defined(__clang__))
#define QLIC_LITE_PUBLIC __attribute__((visibility("default")))
#else
#define QLIC_LITE_PUBLIC
#endif

QLIC_LITE_PUBLIC QlicLiteStatus qlic_lite_encode(const QlicLiteDescription *description,
                               const uint8_t *samples, size_t bytes, size_t stride,
                               const QlicLiteLimits *limits, QlicLiteFile *file,
                               size_t *peak_bytes);
QLIC_LITE_PUBLIC void qlic_lite_file_free(QlicLiteFile *file);
#undef QLIC_LITE_PUBLIC

#ifdef __cplusplus
}
#endif
#endif
#endif

#ifdef _WIN32
typedef wchar_t PathChar;
#define PATH_TEXT(text) L##text
#define path_compare wcscmp
#define remove_path _wremove
#define cli_main wmain
static FILE *open_path(const PathChar *path, const PathChar *mode)
{
    FILE *file;
    return _wfopen_s(&file, path, mode) == 0 ? file : NULL;
}
#else
typedef char PathChar;
#define PATH_TEXT(text) text
#define path_compare strcmp
#define remove_path remove
#define cli_main main
#define open_path fopen
#endif

static void command_limits(QlicLiteLimits *limits)
{
    qlic_lite_limits_default(limits);
    limits->heap_bytes = 1u << 28;
    limits->input_bytes = 1u << 27;
    limits->pixel_data_bytes = 1u << 27;
    limits->file_bytes = 1u << 27;
}

static uint8_t *read_file(const PathChar *path, size_t minimum, size_t maximum, size_t *bytes)
{
    FILE *file = open_path(path, PATH_TEXT("rb"));
    if (!file) return NULL;
    uint8_t *data = NULL;
    if (fseek(file, 0, SEEK_END) == 0) {
        long size = ftell(file);
        if (size >= 0 && (uint64_t)size >= minimum && (uint64_t)size <= maximum &&
            fseek(file, 0, SEEK_SET) == 0) {
            *bytes = (size_t)size;
            data = malloc(*bytes);
            if (data && fread(data, 1, *bytes, file) != *bytes) {
                free(data);
                data = NULL;
            }
        }
    }
    if (fclose(file)) {
        free(data);
        return NULL;
    }
    return data;
}

static int finish_file(FILE *file, const PathChar *path, int ok)
{
    if (fclose(file)) ok = 0;
    if (!ok) remove_path(path);
    return ok;
}

typedef enum { FILE_FAILED, FILE_WRITTEN, FILE_EXISTS } WriteStatus;

static WriteStatus write_new_file(const PathChar *path, const uint8_t *data, size_t bytes)
{
    FILE *file = open_path(path, PATH_TEXT("wbx"));
    if (!file) return errno == EEXIST ? FILE_EXISTS : FILE_FAILED;
    int ok = fwrite(data, 1, bytes, file) == bytes;
    return finish_file(file,path,ok) ? FILE_WRITTEN : FILE_FAILED;
}

static unsigned dimension(const PathChar *text)
{
    if (!*text) return 0;
    unsigned value = 0;
    for (; *text; ++text) {
        if (*text < '0' || *text > '9') return 0;
        value = value * 10 + (unsigned)(*text - '0');
        if (value > 16384) return 0;
    }
    return value;
}

static const PathChar *extension(const PathChar *path)
{
    const PathChar *name = path, *dot = NULL;
    while (*path) {
        if (*path == '/'
#ifdef _WIN32
            || *path == '\\'
#endif
        ) {
            name = path + 1;
            dot = NULL;
        } else if (*path == '.' && path != name) {
            dot = path;
        }
        ++path;
    }
    return dot ? dot : path;
}

static int suffix_is(const PathChar *suffix, const PathChar *expected)
{
    while (*suffix && *expected) {
        PathChar c = *suffix++;
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (c != *expected++) return 0;
    }
    return *suffix == *expected;
}

static const PathChar lite_suffix[] = PATH_TEXT(".lite");

static int has_lite_extension(const PathChar *path)
{
    return suffix_is(extension(path),lite_suffix);
}

static PathChar *default_output(const PathChar *source)
{
    size_t length = (size_t)(extension(source) - source);
    if (length > (SIZE_MAX - sizeof(lite_suffix)) / sizeof(*source)) return NULL;
    PathChar *path = malloc(length * sizeof(*path) + sizeof(lite_suffix));
    if (path) {
        memcpy(path, source, length * sizeof(*path));
        memcpy(path + length, lite_suffix, sizeof(lite_suffix));
    }
    return path;
}

#ifdef QLIC_LITE_IMAGE_IO

typedef enum {
    FORMAT_OTHER, FORMAT_RAW, FORMAT_PNG, FORMAT_BMP, FORMAT_TIFF, FORMAT_TCOM,
    FORMAT_JPEG, FORMAT_JXL, FORMAT_WEBP, FORMAT_PNM, FORMAT_PAM, FORMAT_PBM,
    FORMAT_PGM, FORMAT_PPM
} CliFormat;

static CliFormat image_format(const PathChar *ext)
{
    static const struct { const PathChar *suffix; CliFormat format; } known[] = {
        {PATH_TEXT(".raw"), FORMAT_RAW}, {PATH_TEXT(".bin"), FORMAT_RAW}, {PATH_TEXT(".rgba"), FORMAT_RAW},
        {PATH_TEXT(".png"), FORMAT_PNG}, {PATH_TEXT(".bmp"), FORMAT_BMP},
        {PATH_TEXT(".tif"), FORMAT_TIFF}, {PATH_TEXT(".tiff"), FORMAT_TIFF}, {PATH_TEXT(".tcom"), FORMAT_TCOM},
        {PATH_TEXT(".jpg"), FORMAT_JPEG}, {PATH_TEXT(".jpeg"), FORMAT_JPEG},
        {PATH_TEXT(".jxl"), FORMAT_JXL}, {PATH_TEXT(".webp"), FORMAT_WEBP},
        {PATH_TEXT(".pnm"), FORMAT_PNM}, {PATH_TEXT(".pam"), FORMAT_PAM}, {PATH_TEXT(".pbm"), FORMAT_PBM},
        {PATH_TEXT(".pgm"), FORMAT_PGM}, {PATH_TEXT(".ppm"), FORMAT_PPM}
    };
    for (size_t i=0; i<sizeof(known)/sizeof(known[0]); ++i)
        if (suffix_is(ext,known[i].suffix)) return known[i].format;
    return FORMAT_OTHER;
}

typedef enum {
  QLIC_BMP_EXACT_INVALID = -1,
  QLIC_BMP_EXACT_UNSUPPORTED = 0,
  QLIC_BMP_EXACT_OK = 1
} QlicBmpExactStatus;

typedef struct {
  uint8_t *rgba;
  uint32_t width;
  uint32_t height;
  uint32_t channels;
  uint32_t alpha_mode;
  const uint8_t *icc;
  size_t icc_size;
  int srgb;
} QlicBmpExact;

static QlicBmpExactStatus qlic_bmp_exact_decode(const uint8_t *data, size_t size,
    uint64_t max_pixels, size_t max_decoded_bytes, QlicBmpExact *out,
    char *error, size_t error_capacity);
static void qlic_bmp_exact_free(QlicBmpExact *image);

typedef struct QlicPngExact {
  uint8_t *pixels;
  size_t pixels_size;
  uint8_t **rows;
  uint32_t width, height, channels, bits;
  uint8_t *icc;
  size_t icc_size;
  uint8_t cicp[4], sbit[4];
  uint8_t mdcv[24], clli[8];
  uint32_t sbit_size;
  int has_cicp, has_gamma, has_chromaticity, has_srgb;
  int has_mdcv, has_clli, source_icc, source_cicp;
} QlicPngExact;

static int qlic_png_exact_decode(const uint8_t *data, size_t size,
                          uint64_t max_pixels, size_t max_decoded_bytes,
                          QlicPngExact *out);
static void qlic_png_exact_free(QlicPngExact *image);

typedef struct QlicTiffHandle QlicTiffHandle;
typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t bits_per_sample;
  uint32_t channels;
  uint32_t alpha_mode;
} QlicTiffInfo;

static int qlic_tiff_v1_open(const uint8_t *data, size_t size,
                                       uint64_t max_pixels,
                                       QlicTiffHandle **handle,
                                       QlicTiffInfo *info);
static int qlic_tiff_v1_read(QlicTiffHandle *handle,
                                       void *pixels, size_t pixels_size,
                                       size_t stride);
static void qlic_tiff_v1_close(QlicTiffHandle *handle);
static int qli_tiff_icc(QlicTiffHandle *handle, const uint8_t **data, uint32_t *size);

/* Adapted from my other codec, TCOM. See TCOM-LICENSE (Apache-2.0). */

#define TCOM_INLINE inline __attribute__((always_inline))

static TCOM_INLINE unsigned cli_tcom_read_le(const unsigned char *bytes, unsigned count) {
    unsigned value = 0;
    memcpy(&value, bytes, count);
    if (!*(unsigned char *)&(unsigned){1})
        value = __builtin_bswap32(value);
    return value;
}

static TCOM_INLINE void cli_tcom_write_le(unsigned char *bytes, unsigned count, unsigned value) {
    if (!*(unsigned char *)&(unsigned){1})
        value = __builtin_bswap32(value);
    memcpy(bytes, &value, count);
}

static __attribute__((noinline, cold)) unsigned
cli_tcom_read_tail(const unsigned char *bytes, unsigned count) {
    unsigned value = 0;
    while (count)
        value = value << 8 | bytes[--count];
    return value;
}

static TCOM_INLINE unsigned cli_tcom_read_pixel(const unsigned char *bytes, unsigned count, size_t remaining,
                                       unsigned channel_mask) {
    if (remaining >= 4) {
        unsigned value;
        memcpy(&value, bytes, 4);
        if (!*(unsigned char *)&(unsigned){1})
            value = __builtin_bswap32(value);
        return value & channel_mask;
    }
    return cli_tcom_read_tail(bytes, count);
}

static __attribute__((noinline, cold)) void
cli_tcom_write_tail(unsigned char *bytes, unsigned value, unsigned count) {
    while (count--)
        *bytes++ = value, value >>= 8;
}

static TCOM_INLINE void cli_tcom_write_pixel(unsigned char *bytes, unsigned count, unsigned value,
                                    size_t remaining) {
    if (remaining >= 4) {
        if (!*(unsigned char *)&(unsigned){1})
            value = __builtin_bswap32(value);
        memcpy(bytes, &value, 4);
    } else
        cli_tcom_write_tail(bytes, value, count);
}

static TCOM_INLINE size_t cli_tcom_process_pixels(const unsigned char *input, size_t input_size_or_capacity,
                                         unsigned char *output, size_t row_bytes, size_t raw_size,
                                         unsigned channels, int decode, unsigned model,
                                         unsigned previous) {
    const unsigned char *input_end = input + (decode ? input_size_or_capacity : 0);
    unsigned char *output_end = output + (decode ? raw_size : input_size_or_capacity),
                  *output_begin = output;
    size_t remaining = raw_size, after_first_row = row_bytes ? raw_size - row_bytes : 0;
    unsigned channel_mask = ~0u >> ((4 - channels) * 8);
    if (decode)
        input += 16;
    else
        output += 16;
    const unsigned char *pixels = decode ? output : input;
    while (remaining) {
        unsigned count = 1, payload_size = 0, tag = 255, pixel, payload;
        unsigned above =
            remaining <= after_first_row
                ? cli_tcom_read_pixel(pixels - row_bytes, channels, remaining + row_bytes, channel_mask)
                : previous;
        unsigned prediction = (previous | above) - ((previous ^ above) & 0xfefefefe) / 2 -
                              ((previous ^ above) & model * 128 & 256);
        if (decode) {
            if (input == input_end)
                return 0;
            tag = *input++;
            payload_size = tag & 3;
            pixel = tag & 128 ? above : previous;
            if (payload_size == 3 && tag < 255) {
                count = tag / 4 % 32 + 1;
                if (count * channels > remaining)
                    return 0;
            } else {
                payload_size = tag == 255 ? channels : payload_size;
                if (payload_size > (size_t)(input_end - input))
                    return 0;
                if (tag == 255)
                    pixel = cli_tcom_read_pixel(input, channels, input_end - input, channel_mask);
                else {
                    unsigned bits =
                                 0x78461422u >> (8 * (payload_size ? payload_size + 1 : model & 1)),
                             green_bits = bits & 15, color_bits = bits >> 4 & 15;
                    unsigned packed = tag / 4 + (cli_tcom_read_pixel(input, payload_size, input_end - input,
                                                            (1u << (8 * payload_size)) - 1)
                                                 << 6);
                    unsigned green_range = 1u << green_bits, color_range = 1u << color_bits,
                             green_delta = (packed & (green_range - 1)) + 256 - green_range / 2;
                    packed >>= green_bits;
                    unsigned red_delta = (packed & (color_range - 1)) - color_range / 2,
                             blue_delta = (packed >> color_bits) - color_range / 2;
                    pixel = (prediction & 0xff000000u) |
                            ((prediction + green_delta * 256) & 0xff00) |
                            (((prediction & 0xff00ff) + (green_delta + red_delta) +
                              (green_delta + blue_delta) * 65536) &
                             0xff00ff);
                }
                input += payload_size;
            }
            unsigned repeat = count;
            do {
                cli_tcom_write_pixel(output, channels, pixel, output_end - output);
                output += channels;
            } while (--repeat);
        } else {
            pixel = payload = cli_tcom_read_pixel(input, channels, remaining, channel_mask);
            if (pixel == previous || pixel == above) {
                tag = pixel != previous;
                while (count < 32 - tag && count * channels < remaining &&
                       cli_tcom_read_pixel(input + count * channels, channels, remaining - count * channels,
                                  channel_mask) == pixel)
                    count++;
                tag = count * 4 - 1 + 128 * tag;
            } else {
                payload_size = channels;
                int green_delta = (signed char)((pixel >> (channels == 1 ? 0 : 8)) -
                                                (prediction >> (channels == 1 ? 0 : 8)));
                if (channels == 1 && green_delta < -8)
                    green_delta = -8;
                int red_delta = (signed char)(pixel - prediction - green_delta),
                    blue_delta = (signed char)(channels < 3 ? -green_delta * (model >> 2)
                                                            : pixel / 65536 - prediction / 65536 -
                                                                  green_delta);
                if (!((pixel ^ prediction) >> 24)) {
                    unsigned green_range = model & 1 ? 16 : 4, color_range = model & 1 ? 2 : 4,
                             green = green_delta + green_range / 2,
                             red = red_delta + color_range / 2, blue = blue_delta + color_range / 2;
                    if ((green | ((red | blue) << (model & 1 ? 3 : 0))) < green_range) {
                        tag = (green + (red + blue * color_range) * green_range) * 4;
                        payload_size = 0;
                    } else if (channels > 1) {
                        green = green_delta + 32;
                        red = red_delta + 8;
                        blue = blue_delta + 8;
                        if ((green | ((red | blue) << 2)) < 64) {
                            tag = green * 4 + 1;
                            payload = red | blue << 4;
                            payload_size = 1;
                        } else if (channels > 2) {
                            green = green_delta + 128;
                            red = red_delta + 64;
                            blue = blue_delta + 64;
                            if ((red | blue) < 128) {
                                tag = green * 4 + 2;
                                payload = green >> 6 | red << 2 | blue << 9;
                                payload_size = 2;
                            }
                        }
                    }
                }
            }
            if ((size_t)(output_end - output) <= payload_size)
                return 0;
            *output++ = tag;
            cli_tcom_write_pixel(output, payload_size, payload, output_end - output);
            output += payload_size;
            input += count * channels;
        }
        previous = pixel;
        remaining -= count * channels;
        pixels += count * channels;
    }
    return (!decode || input == input_end) ? (size_t)(output - output_begin) : 0;
}

static size_t cli_tcom_convert(const unsigned char *input, size_t input_size, unsigned char *output,
                        size_t capacity, unsigned *dimensions, int decode) {
    if (!input || !dimensions)
        return 0;
    unsigned selected_model = 0;
    if (decode) {
        if (input_size < 16 || cli_tcom_read_le(input, 4) != 0x4d4f4354 ||
            (selected_model = cli_tcom_read_le(input + 12, 4)) >> 5 != 16)
            return 0;
        *dimensions = cli_tcom_read_le(input + 4, 4);
        dimensions[1] = cli_tcom_read_le(input + 8, 4);
        dimensions[2] = selected_model & 7;
        selected_model = selected_model / 8 & 3;
    }
    unsigned width = *dimensions, height = dimensions[1], channels = dimensions[2];
    size_t row_bytes, raw_size;
    if (channels - 1 > 3 || (uint64_t)width * height > (1u << 24) ||
        __builtin_mul_overflow((size_t)width, (size_t)channels, &row_bytes) ||
        __builtin_mul_overflow(row_bytes, (size_t)height, &raw_size) || !raw_size)
        return 0;
    if (!output || (decode ? capacity < raw_size : input_size != raw_size || capacity < 16))
        return (decode && !output) * raw_size;
    if (!decode) {
        selected_model = channels == 1;
        cli_tcom_write_le(output, 4, 0x4d4f4354);
        cli_tcom_write_le(output + 4, 4, width);
        cli_tcom_write_le(output + 8, 4, height);
        cli_tcom_write_le(output + 12, 4, 512 + channels + selected_model * 8);
        size_t sample_step = selected_model ? 0 : (raw_size / channels - 1) / 127 * channels;
        unsigned best_score = (unsigned)-1;
        for (unsigned trial_model = 0; sample_step && trial_model < 4; trial_model++) {
            unsigned score = 3 * !!trial_model;
            size_t sample_offset = 0;
            for (unsigned sample = 0; sample < 128; sample++, sample_offset += sample_step)
                score += (unsigned)cli_tcom_process_pixels(
                    input + sample_offset, capacity, output,
                    sample_offset >= row_bytes ? row_bytes : 0, channels, channels, 0,
                    trial_model | 4,
                    sample_offset ? cli_tcom_read_tail(input + sample_offset - channels, channels) : 0);
            if (score < best_score)
                best_score = score, selected_model = trial_model;
        }
        output[12] = channels + selected_model * 8;
    }
    if (decode) {
        if (channels == 1)
            return cli_tcom_process_pixels(input, input_size, output, row_bytes, raw_size, 1, 1,
                                  selected_model, 0);
        if (channels == 3)
            return cli_tcom_process_pixels(input, input_size, output, row_bytes, raw_size, 3, 1,
                                  selected_model, 0);
        if (channels == 4)
            return cli_tcom_process_pixels(input, input_size, output, row_bytes, raw_size, 4, 1,
                                  selected_model, 0);
        return cli_tcom_process_pixels(input, input_size, output, row_bytes, raw_size, 2, 1, selected_model,
                              0);
    }
    if (channels == 1)
        return cli_tcom_process_pixels(input, capacity, output, row_bytes, raw_size, 1, 0, selected_model,
                              0);
    if (channels == 3)
        return cli_tcom_process_pixels(input, capacity, output, row_bytes, raw_size, 3, 0, selected_model,
                              0);
    if (channels == 4)
        return cli_tcom_process_pixels(input, capacity, output, row_bytes, raw_size, 4, 0, selected_model,
                              0);
    return cli_tcom_process_pixels(input, capacity, output, row_bytes, raw_size, 2, 0, selected_model, 0);
}

#undef TCOM_INLINE

enum { SOURCE_LOSSLESS, SOURCE_LOSSY, SOURCE_COMPRESSION_UNKNOWN };

typedef struct {
    QlicLiteDescription description;
    uint8_t *samples;
    size_t bytes, stride;
    uint8_t *icc;
    int source_compression;
} CliImage;

static int import_layout(CliImage *image, uint32_t w, uint32_t h, uint32_t bits,
                         uint32_t channels, uint32_t alpha, const QlicLiteLimits *limits)
{
    if (!w || !h || (uint64_t)w * h > limits->pixels ||
        bits < 8 || bits > 10 || !channels || channels > 4)
        return 0;
    size_t stride = (size_t)w * channels * (bits > 8 ? 2u : 1u);
    if (stride > limits->pixel_data_bytes / h) return 0;
    image->description = (QlicLiteDescription){
        .width=w, .height=h, .bits=bits, .channels=channels, .alpha=alpha
    };
    image->stride = stride;
    image->bytes = stride * h;
    return 1;
}

static unsigned cli_be(const uint8_t *data, unsigned size)
{
    unsigned value = 0;
    while (size--) value = (value << 8) | *data++;
    return value;
}

static unsigned cli_sample(const uint8_t *data, unsigned bytes)
{
    return bytes == 1 ? data[0] : (unsigned)data[0] | (unsigned)data[1] << 8;
}

static void cli_store(uint8_t *data, unsigned bytes, unsigned value)
{
    data[0] = (uint8_t)value;
    if (bytes == 2) data[1] = (uint8_t)(value >> 8);
}

static void set_profile(CliImage *image, uint8_t *icc, size_t size)
{
    image->icc = icc;
    image->description.icc = icc;
    image->description.icc_size = size;
    image->description.authority = QLIC_LITE_COLOR_ICC;
}

static int import_profile(CliImage *image, const uint8_t *icc, size_t size,
                          const QlicLiteLimits *limits)
{
    if (!size) return 1;
    if (!icc || size > limits->metadata_bytes) return 0;
    uint8_t *copy = malloc(size);
    if (!copy) return 0;
    memcpy(copy, icc, size);
    set_profile(image, copy, size);
    return 1;
}

static int import_png(CliImage *image, const uint8_t *data, size_t size,
                      const QlicLiteLimits *limits)
{
    QlicPngExact png = {0};
    if (!qlic_png_exact_decode(data, size, limits->pixels, limits->pixel_data_bytes, &png))
        return 0;
    unsigned bits = png.bits;
    if (bits == 16 && png.sbit_size) {
        bits = png.sbit[0];
        for (uint32_t i = 1; i < png.sbit_size; ++i)
            if (png.sbit[i] != bits) bits = 16;
    }
    int ok = import_layout(image, png.width, png.height, bits, png.channels,
                           png.channels == 4 ? QLIC_LITE_ALPHA_STRAIGHT : 0, limits);
    if (ok && png.bits == 16) {
        unsigned max = (1u << bits) - 1;
        unsigned bytes = bits == 8 ? 1 : 2;
        uint8_t *dst = png.pixels;
        for (size_t i = 0; i < png.pixels_size; i += 2, dst += bytes) {
            unsigned expanded = cli_sample(png.pixels + i, 2);
            unsigned native = (expanded * max + 32767u) / 65535u;
            if ((native * 65535u + max / 2) / max != expanded) { ok = 0; break; }
            cli_store(dst, bytes, native);
        }
    }
    if (ok && png.icc_size) {
        ok = png.icc && png.icc_size <= limits->metadata_bytes;
        if (ok) {
            set_profile(image, png.icc, png.icc_size);
            png.icc = NULL;
        }
    }
    if (ok) {
        QlicLiteDescription *d = &image->description;
        if (png.has_cicp || (png.has_srgb && !png.icc_size)) {
            d->has_cicp = 1;
            d->authority = png.icc_size ? QLIC_LITE_COLOR_ICC_PREFERRED : QLIC_LITE_COLOR_CICP;
            d->cicp = png.has_cicp
                ? (QlicLiteCicp){png.cicp[0], png.cicp[1], png.cicp[2], png.cicp[3], 0}
                : (QlicLiteCicp){1, 13, 0, 1, 0};
        }
        if (png.has_mdcv) {
            d->has_mastering = 1;
            for (unsigned i = 0; i < 3; ++i) {
                d->mastering.x[i] = (uint16_t)cli_be(png.mdcv + 4 * i, 2);
                d->mastering.y[i] = (uint16_t)cli_be(png.mdcv + 4 * i + 2, 2);
            }
            d->mastering.white_x = (uint16_t)cli_be(png.mdcv + 12, 2);
            d->mastering.white_y = (uint16_t)cli_be(png.mdcv + 14, 2);
            d->mastering.max_luminance = cli_be(png.mdcv + 16, 4);
            d->mastering.min_luminance = cli_be(png.mdcv + 20, 4);
        }
        if (png.has_clli) {
            d->has_light = 1;
            d->max_cll = cli_be(png.clli, 4);
            d->max_fall = cli_be(png.clli + 4, 4);
        }
        if ((png.has_gamma || png.has_chromaticity) && !d->authority)
            fputs("Warning: PNG gamma and chromaticity tags are omitted.\n", stderr);
        image->samples = png.pixels;
        png.pixels = NULL;
    }
    qlic_png_exact_free(&png);
    return ok;
}

static int import_bmp(CliImage *image, const uint8_t *data, size_t size,
                      const QlicLiteLimits *limits)
{
    QlicBmpExact bmp = {0};
    char error[160] = {0};
    if (qlic_bmp_exact_decode(data, size, limits->pixels, limits->pixel_data_bytes,
                              &bmp, error, sizeof(error)) != QLIC_BMP_EXACT_OK) {
        if (*error) fprintf(stderr, "%s\n", error);
        return 0;
    }
    int ok = import_layout(image, bmp.width, bmp.height, 8, bmp.channels, bmp.alpha_mode, limits) &&
             import_profile(image, bmp.icc, bmp.icc_size, limits);
    if (ok) {
        if (bmp.channels == 3)
            for (size_t i = 0; i < image->bytes / 3; ++i)
                memmove(bmp.rgba + 3*i, bmp.rgba + 4*i, 3);
        if (bmp.srgb && !bmp.icc_size) {
            image->description.authority = QLIC_LITE_COLOR_CICP;
            image->description.has_cicp = 1;
            image->description.cicp = (QlicLiteCicp){1, 13, 0, 1, 0};
        }
        image->samples = bmp.rgba;
        bmp.rgba = NULL;
    }
    qlic_bmp_exact_free(&bmp);
    return ok;
}

static int import_tiff(CliImage *image, const uint8_t *data, size_t size,
                       const QlicLiteLimits *limits)
{
    QlicTiffHandle *tiff = NULL;
    QlicTiffInfo info;
    if (!qlic_tiff_v1_open(data, size, limits->pixels, &tiff, &info)) return 0;
    int ok = import_layout(image, info.width, info.height, info.bits_per_sample,
                           info.channels, info.alpha_mode, limits);
    if (ok) {
        const uint8_t *icc = NULL;
        uint32_t icc_size = 0;
        if (qli_tiff_icc(tiff, &icc, &icc_size))
            ok = import_profile(image, icc, icc_size, limits);
    }
    if (ok) {
        image->samples = malloc(image->bytes);
        ok = image->samples && qlic_tiff_v1_read(tiff, image->samples, image->bytes, image->stride);
        if (ok && info.bits_per_sample > 8) {
            for (size_t i = 0; i < image->bytes; i += 2) {
                uint16_t value;
                memcpy(&value, image->samples + i, 2);
                cli_store(image->samples + i, 2, value);
            }
        }
    }
    qlic_tiff_v1_close(tiff);
    return ok;
}

static int import_jpeg(CliImage *image, const uint8_t *data, size_t size,
                       const QlicLiteLimits *limits)
{
    tjhandle jpeg = tj3Init(TJINIT_DECOMPRESS);
    if (!jpeg) return 0;
    int ok = tj3Set(jpeg, TJPARAM_STOPONWARNING, 1) == 0 &&
             tj3Set(jpeg, TJPARAM_MAXPIXELS, (int)limits->pixels) == 0 &&
             tj3Set(jpeg, TJPARAM_MAXMEMORY, 128) == 0 &&
             tj3Set(jpeg, TJPARAM_SAVEMARKERS, 2) == 0 &&
             tj3DecompressHeader(jpeg, data, size) == 0;
    if (ok) {
        int color = tj3Get(jpeg, TJPARAM_COLORSPACE);
        image->source_compression = tj3Get(jpeg, TJPARAM_LOSSLESS) == 1
            ? SOURCE_LOSSLESS : SOURCE_LOSSY;
        unsigned channels = color == TJCS_GRAY ? 1u : 3u;
        ok = tj3Get(jpeg, TJPARAM_PRECISION) == 8 &&
             (color == TJCS_GRAY || color == TJCS_RGB || color == TJCS_YCbCr) &&
             import_layout(image, (uint32_t)tj3Get(jpeg, TJPARAM_JPEGWIDTH),
                           (uint32_t)tj3Get(jpeg, TJPARAM_JPEGHEIGHT), 8, channels, 0, limits);
        if (ok) {
            image->samples = malloc(image->bytes);
            ok = image->samples && tj3Decompress8(jpeg, data, size, image->samples,
                   (int)image->stride, channels == 1 ? TJPF_GRAY : TJPF_RGB) == 0;
        }
        if (ok) {
            size_t icc_size = 0;
            unsigned char *icc = NULL;
            if (tj3GetICCProfile(jpeg, &icc, &icc_size) == 0)
                ok = import_profile(image, icc, icc_size, limits);
            tj3Free(icc);
        }
    }
    tj3Destroy(jpeg);
    return ok;
}

static int import_webp(CliImage *image, const uint8_t *data, size_t size,
                       const QlicLiteLimits *limits)
{
    WebPBitstreamFeatures features;
    if (WebPGetFeatures(data, size, &features) != VP8_STATUS_OK || features.has_animation ||
        !import_layout(image, (uint32_t)features.width, (uint32_t)features.height, 8,
                       features.has_alpha ? 4u : 3u, features.has_alpha ? 1u : 0u, limits))
        return 0;
    image->source_compression = features.format == 1 ? SOURCE_LOSSY :
        features.format == 2 ? SOURCE_LOSSLESS : SOURCE_COMPRESSION_UNKNOWN;
    if (size >= 12 && memcmp(data, "RIFF", 4) == 0) {
        size_t end = (size_t)cli_tcom_read_le(data + 4, 4);
        if (end < 4 || end > size - 8) return 0;
        end += 8;
        for (size_t pos = 12; pos < end;) {
            if (end - pos < 8) return 0;
            size_t chunk = cli_tcom_read_le(data + pos + 4, 4);
            size_t padded = chunk + (chunk & 1u);
            if (chunk > end - pos - 8 || padded > end - pos - 8) return 0;
            if (memcmp(data + pos, "ICCP", 4) == 0) {
                if (image->icc || !chunk || !import_profile(image, data + pos + 8, chunk, limits))
                    return 0;
            }
            pos += 8 + padded;
        }
    }
    image->samples = malloc(image->bytes);
    if (!image->samples) return 0;
    return (features.has_alpha
        ? WebPDecodeRGBAInto(data, size, image->samples, image->bytes, (int)image->stride)
        : WebPDecodeRGBInto(data, size, image->samples, image->bytes, (int)image->stride)) != NULL;
}

typedef struct { size_t used, cap; } CliJxlMemory;
typedef union { size_t bytes; max_align_t alignment; } CliJxlAllocation;

static void *cli_jxl_alloc(void *opaque, size_t size)
{
    CliJxlMemory *memory = opaque;
    if (size > memory->cap - memory->used ||
        sizeof(CliJxlAllocation) > memory->cap - memory->used - size) return NULL;
    size += sizeof(CliJxlAllocation);
    CliJxlAllocation *block = malloc(size);
    if (!block) return NULL;
    memory->used += size;
    block->bytes = size;
    return block + 1;
}

static void cli_jxl_free(void *opaque, void *pointer)
{
    if (!pointer) return;
    CliJxlMemory *memory = opaque;
    CliJxlAllocation *block = (CliJxlAllocation *)pointer - 1;
    memory->used -= block->bytes;
    free(block);
}

static int import_jxl(CliImage *image, const uint8_t *data, size_t size,
                      const QlicLiteLimits *limits)
{
    CliJxlMemory memory = {0, limits->heap_bytes};
    JxlMemoryManager manager = {&memory, cli_jxl_alloc, cli_jxl_free};
    JxlDecoder *decoder = JxlDecoderCreate(&manager);
    if (!decoder) return 0;
    JxlPixelFormat format = {0, JXL_TYPE_UINT8, JXL_LITTLE_ENDIAN, 0};
    JxlBasicInfo info;
    int ok = JxlDecoderSubscribeEvents(decoder, JXL_DEC_BASIC_INFO | JXL_DEC_COLOR_ENCODING |
                       JXL_DEC_FULL_IMAGE) == JXL_DEC_SUCCESS &&
             JxlDecoderSetUnpremultiplyAlpha(decoder, JXL_FALSE) == JXL_DEC_SUCCESS &&
             JxlDecoderSetInput(decoder, data, size) == JXL_DEC_SUCCESS;
    if (ok) JxlDecoderCloseInput(decoder);
    if (ok) {
        ok = JxlDecoderProcessInput(decoder) == JXL_DEC_BASIC_INFO &&
             JxlDecoderGetBasicInfo(decoder, &info) == JXL_DEC_SUCCESS &&
             !info.have_animation && !info.exponent_bits_per_sample &&
             (info.num_color_channels == 1 || info.num_color_channels == 3) &&
             info.num_extra_channels == (info.alpha_bits != 0) &&
             (!info.alpha_bits || (info.alpha_bits == info.bits_per_sample && !info.alpha_exponent_bits)) &&
             info.bits_per_sample <= 10;
    }
    if (ok) {
        image->source_compression = info.uses_original_profile
            ? SOURCE_COMPRESSION_UNKNOWN : SOURCE_LOSSY;
        format.num_channels = info.num_color_channels + (info.alpha_bits != 0);
        format.data_type = info.bits_per_sample > 8 ? JXL_TYPE_UINT16 : JXL_TYPE_UINT8;
        ok = import_layout(image, info.xsize, info.ysize,
                           info.bits_per_sample < 8 ? 8u : info.bits_per_sample,
                           format.num_channels, info.alpha_bits ? (info.alpha_premultiplied ? 2u : 1u) : 0u,
                           limits) && JxlDecoderProcessInput(decoder) == JXL_DEC_COLOR_ENCODING;
    }
    if (ok) {
        size_t icc_size = 0;
        ok = JxlDecoderGetICCProfileSize(decoder, JXL_COLOR_PROFILE_TARGET_DATA, &icc_size) ==
             JXL_DEC_SUCCESS && icc_size && icc_size <= limits->metadata_bytes;
        if (ok) {
            image->icc = malloc(icc_size);
            ok = image->icc && JxlDecoderGetColorAsICCProfile(decoder, JXL_COLOR_PROFILE_TARGET_DATA,
                        image->icc, icc_size) == JXL_DEC_SUCCESS;
            if (ok) {
                image->description.icc = image->icc;
                image->description.icc_size = icc_size;
                image->description.authority = QLIC_LITE_COLOR_ICC;
            }
        }
    }
    if (ok) {
        size_t required = 0;
        ok = JxlDecoderProcessInput(decoder) == JXL_DEC_NEED_IMAGE_OUT_BUFFER &&
             JxlDecoderImageOutBufferSize(decoder, &format, &required) == JXL_DEC_SUCCESS &&
             required == image->bytes;
        if (ok) {
            image->samples = malloc(required);
            ok = image->samples && JxlDecoderSetImageOutBuffer(decoder, &format, image->samples,
                                 required) == JXL_DEC_SUCCESS;
            if (ok && info.bits_per_sample > 8) {
                JxlBitDepth depth = {JXL_BIT_DEPTH_FROM_CODESTREAM, 0, 0};
                ok = JxlDecoderSetImageOutBitDepth(decoder, &depth) == JXL_DEC_SUCCESS;
            }
            ok = ok && JxlDecoderProcessInput(decoder) == JXL_DEC_FULL_IMAGE &&
                       JxlDecoderProcessInput(decoder) == JXL_DEC_SUCCESS;
        }
    }
    JxlDecoderDestroy(decoder);
    return ok;
}

static int import_tcom(CliImage *image, const uint8_t *data, size_t size,
                       const QlicLiteLimits *limits)
{
    unsigned dimensions[3];
    if (!cli_tcom_convert(data, size, NULL, 0, dimensions, 1)) return 0;
    unsigned channels = dimensions[2];
    if (!import_layout(image, dimensions[0], dimensions[1], 8, channels,
                       channels == 2 || channels == 4 ? 1u : 0u, limits)) return 0;
    image->samples = malloc(image->bytes);
    return image->samples &&
        cli_tcom_convert(data, size, image->samples, image->bytes, dimensions, 1) == image->bytes;
}

typedef struct { const uint8_t *at, *end; } PnmReader;

static void pnm_space(PnmReader *reader)
{
    while (reader->at < reader->end) {
        if (*reader->at == '#') {
            while (reader->at < reader->end && *reader->at != '\n') ++reader->at;
        } else if (isspace(*reader->at)) ++reader->at;
        else break;
    }
}

static int pnm_token(PnmReader *reader, char *value, size_t capacity)
{
    pnm_space(reader);
    size_t length = 0;
    while (reader->at < reader->end && !isspace(*reader->at) && *reader->at != '#') {
        if (length == capacity - 1 || *reader->at == 0) return 0;
        value[length++] = (char)*reader->at++;
    }
    value[length] = 0;
    return length != 0;
}

static int pnm_number(PnmReader *reader, unsigned *value)
{
    char word[16];
    if (!pnm_token(reader, word, sizeof(word))) return 0;
    unsigned n = 0;
    for (const char *p = word; *p; ++p) {
        if (*p < '0' || *p > '9' || n > (UINT32_MAX - 9u) / 10u) return 0;
        n = n * 10 + (unsigned)(*p - '0');
    }
    *value = n;
    return 1;
}

static int pnm_pam_header(PnmReader *reader, unsigned *width, unsigned *height,
                          unsigned *channels, unsigned *max)
{
    char key[32], tuple[32];
    unsigned seen = 0;
    static const char *const tuples[] = {
        "", "GRAYSCALE", "GRAYSCALE_ALPHA", "RGB", "RGB_ALPHA"
    };
    for (;;) {
        if (!pnm_token(reader, key, sizeof(key))) return 0;
        if (!strcmp(key, "ENDHDR")) break;
        unsigned *value = NULL, flag = 0;
        if (!strcmp(key, "WIDTH")) { value = width; flag = 1; }
        else if (!strcmp(key, "HEIGHT")) { value = height; flag = 2; }
        else if (!strcmp(key, "DEPTH")) { value = channels; flag = 4; }
        else if (!strcmp(key, "MAXVAL")) { value = max; flag = 8; }
        else if (!strcmp(key, "TUPLTYPE")) flag = 16;
        if (!flag || (seen & flag)) return 0;
        seen |= flag;
        if (value ? !pnm_number(reader, value) : !pnm_token(reader, tuple, sizeof(tuple)))
            return 0;
    }
    return (seen & 15u) == 15u && *channels >= 1 && *channels <= 4 &&
        (!(seen & 16u) || !strcmp(tuple, tuples[*channels]) ||
         (*max == 1 && ((*channels == 1 && !strcmp(tuple, "BLACKANDWHITE")) ||
                        (*channels == 2 && !strcmp(tuple, "BLACKANDWHITE_ALPHA")))));
}

static int import_pnm(CliImage *image, const uint8_t *data, size_t size,
                       const QlicLiteLimits *limits)
{
    if (size < 4 || data[0] != 'P' || data[1] < '1' || data[1] > '7' || !isspace(data[2]))
        return 0;
    unsigned kind = data[1] - '0', width = 0, height = 0, channels = 1, max = 1;
    PnmReader reader = {data + 2, data + size};
    int bitmap = kind == 1 || kind == 4;
    if (kind == 7) {
        if (!pnm_pam_header(&reader, &width, &height, &channels, &max)) return 0;
    } else {
        if (!pnm_number(&reader, &width) || !pnm_number(&reader, &height) ||
            (!bitmap && !pnm_number(&reader, &max))) return 0;
        channels = kind == 3 || kind == 6 ? 3u : 1u;
    }
    if (!max || max > 1023) return 0;
    unsigned bits = max <= 255 ? 8u : max <= 511 ? 9u : 10u;
    if (!import_layout(image, width, height, bits, channels,
                       channels == 2 || channels == 4 ? 1u : 0u, limits)) return 0;
    unsigned sample_bytes = bits > 8 ? 2u : 1u;
    size_t count = image->bytes / sample_bytes;
    size_t row_bytes = bitmap ? ((size_t)width + 7u) / 8u : image->stride;
    if (kind >= 4) {
        if (reader.at == reader.end || !isspace(*reader.at)) return 0;
        if (*reader.at++ == '\r' && reader.at < reader.end && *reader.at == '\n' &&
            (size_t)(reader.end - reader.at) > row_bytes * height) ++reader.at;
        if ((size_t)(reader.end - reader.at) != row_bytes * height) return 0;
    }
    image->samples = malloc(image->bytes);
    if (!image->samples) return 0;
    unsigned target_max = (1u << bits) - 1;
    for (size_t i = 0; i < count; ++i) {
        unsigned sample;
        if (kind <= 3) {
            if (!pnm_number(&reader, &sample) || sample > max) return 0;
        } else if (bitmap) {
            size_t x = i % width, y = i / width;
            sample = (reader.at[y * row_bytes + x / 8] >> (7 - x % 8)) & 1;
        } else {
            sample = cli_be(reader.at + i * sample_bytes, sample_bytes);
            if (sample > max) return 0;
        }
        if (bitmap) sample = 1 - sample;
        cli_store(image->samples + i * sample_bytes, sample_bytes,
                  (sample * target_max + max / 2) / max);
    }
    if (kind <= 3) {
        pnm_space(&reader);
        if (reader.at != reader.end) return 0;
    }
    if (!bitmap && max != target_max)
        fputs("Note: Netpbm sample values are scaled to the full Lite sample range.\n", stderr);
    return 1;
}

static int pnm_raster(FILE *file, const QlicLiteImage *image, int bitmap)
{
    const QlicLiteDescription *d = &image->description;
    if (bitmap) {
        for (uint32_t y = 0; y < d->height; ++y) {
            unsigned packed = 0;
            for (uint32_t x = 0; x < d->width; ++x) {
                packed |= (image->samples[(size_t)y * d->width + x] == 0) << (7 - (x & 7));
                if ((x & 7) == 7 || x == d->width - 1) {
                    if (fputc((int)packed, file) == EOF) return 0;
                    packed = 0;
                }
            }
        }
        return 1;
    }
    if (d->bits == 8) return fwrite(image->samples, 1, image->bytes, file) == image->bytes;
    uint8_t buffer[4096];
    for (size_t i = 0; i < image->bytes;) {
        size_t size = image->bytes - i;
        if (size > sizeof(buffer)) size = sizeof(buffer);
        for (size_t j = 0; j < size; j += 2) {
            buffer[j] = image->samples[i+j+1];
            buffer[j+1] = image->samples[i+j];
        }
        if (fwrite(buffer, 1, size, file) != size) return 0;
        i += size;
    }
    return 1;
}

static int export_pnm(FILE *file, const QlicLiteImage *image, CliFormat format)
{
    const QlicLiteDescription *d = &image->description;
    int bitmap = format == FORMAT_PBM;
    int pam = format == FORMAT_PAM ||
              (format == FORMAT_PNM && (d->channels == 2 || d->channels == 4));
    unsigned max = (1u << d->bits) - 1;
    if (d->alpha == QLIC_LITE_ALPHA_PREMULTIPLIED) return 0;
    if (bitmap) {
        if (d->channels != 1 || d->bits != 8) return 0;
        for (size_t i = 0; i < image->bytes; ++i)
            if (image->samples[i] != 0 && image->samples[i] != 255) return 0;
        if (fprintf(file, "P4\n%u %u\n", d->width, d->height) < 0) return 0;
    } else if (pam) {
        static const char *const tuple[] = {"GRAYSCALE", "GRAYSCALE_ALPHA", "RGB", "RGB_ALPHA"};
        if (fprintf(file, "P7\nWIDTH %u\nHEIGHT %u\nDEPTH %u\nMAXVAL %u\nTUPLTYPE %s\nENDHDR\n",
                    d->width, d->height, d->channels, max, tuple[d->channels - 1]) < 0) return 0;
    } else {
        unsigned channels = d->channels;
        if ((channels != 1 && channels != 3) ||
            (channels != 1 && format == FORMAT_PGM) ||
            (channels != 3 && format == FORMAT_PPM)) return 0;
        if (fprintf(file, "P%c\n%u %u\n%u\n", channels == 1 ? '5' : '6',
                    d->width, d->height, max) < 0) return 0;
    }
    return pnm_raster(file, image, bitmap);
}

#define QLIC_TIFF_INTERNAL_MEMORY_LIMIT ((tmsize_t)268435456)
#define QLIC_TIFF_OUTPUT_LIMIT ((uint64_t)536870912)

struct QlicTiffHandle {
  const uint8_t *data;
  size_t size;
  size_t cursor;
  TIFF *tiff;
  QlicTiffInfo info;
  uint32_t tile_width;
  uint32_t tile_height;
  uint32_t rows_per_strip;
  int tiled;
};

static tmsize_t memory_read(thandle_t opaque, void *output, tmsize_t wanted) {
  QlicTiffHandle *handle = (QlicTiffHandle *)opaque;
  if (!handle || !output || wanted < 0 || handle->cursor > handle->size)
    return -1;
  size_t remaining = handle->size - handle->cursor;
  size_t count = (size_t)wanted < remaining ? (size_t)wanted : remaining;
  memcpy(output, handle->data + handle->cursor, count);
  handle->cursor += count;
  return (tmsize_t)count;
}

static tmsize_t memory_write(thandle_t opaque, void *input, tmsize_t wanted) {
  (void)opaque;
  (void)input;
  (void)wanted;
  return -1;
}

static toff_t memory_seek(thandle_t opaque, toff_t offset, int origin) {
  QlicTiffHandle *handle = (QlicTiffHandle *)opaque;
  if (!handle)
    return (toff_t)-1;
  uint64_t base = origin == SEEK_SET ? 0u :
                  origin == SEEK_CUR ? handle->cursor :
                  origin == SEEK_END ? handle->size : UINT64_MAX;
  if (base == UINT64_MAX)
    return (toff_t)-1;
  int64_t change = (int64_t)offset;
  uint64_t position;
  if (change < 0) {
    uint64_t magnitude = (uint64_t)(-(change + 1)) + 1u;
    if (magnitude > base)
      return (toff_t)-1;
    position = base - magnitude;
  } else {
    if ((uint64_t)change > UINT64_MAX - base)
      return (toff_t)-1;
    position = base + (uint64_t)change;
  }
  if (position > handle->size)
    return (toff_t)-1;
  handle->cursor = (size_t)position;
  return position;
}

static int memory_close(thandle_t opaque) {
  (void)opaque;
  return 0;
}

static toff_t memory_size(thandle_t opaque) {
  QlicTiffHandle *handle = (QlicTiffHandle *)opaque;
  return handle ? handle->size : 0u;
}

static int memory_map(thandle_t opaque, void **base, toff_t *size) {
  (void)opaque;
  (void)base;
  (void)size;
  return 0;
}

static void memory_unmap(thandle_t opaque, void *base, toff_t size) {
  (void)opaque;
  (void)base;
  (void)size;
}

static int quiet_error(TIFF *tiff, void *user, const char *module,
                       const char *format, va_list args) {
  (void)tiff;
  (void)user;
  (void)module;
  (void)format;
  (void)args;
  return 1;
}

static void qlic_tiff_v1_close(QlicTiffHandle *handle) {
  if (!handle)
    return;
  if (handle->tiff)
    TIFFClose(handle->tiff);
  free(handle);
}

static int qli_tiff_icc(QlicTiffHandle *handle, const uint8_t **data, uint32_t *size) {
  return TIFFGetField(handle->tiff, TIFFTAG_ICCPROFILE, size, data);
}

static int qlic_tiff_v1_open(const uint8_t *data, size_t size, uint64_t max_pixels,
                      QlicTiffHandle **output, QlicTiffInfo *info) {
  if (!data || !size || !output || !info
#if SIZE_MAX > INT64_MAX
      || size > INT64_MAX
#endif
      )
    return 0;
  *output = NULL;
  memset(info, 0, sizeof(*info));
  QlicTiffHandle *handle = (QlicTiffHandle *)calloc(1u, sizeof(*handle));
  if (!handle)
    return 0;
  handle->data = data;
  handle->size = size;
  TIFFOpenOptions *options = TIFFOpenOptionsAlloc();
  if (!options) {
    free(handle);
    return 0;
  }
  TIFFOpenOptionsSetMaxSingleMemAlloc(options, QLIC_TIFF_INTERNAL_MEMORY_LIMIT);
  TIFFOpenOptionsSetMaxCumulatedMemAlloc(options, QLIC_TIFF_INTERNAL_MEMORY_LIMIT);
  TIFFOpenOptionsSetErrorHandlerExtR(options, quiet_error, NULL);
  TIFFOpenOptionsSetWarningHandlerExtR(options, quiet_error, NULL);
  handle->tiff = TIFFClientOpenExt("QLIC TIFF input", "r", (thandle_t)handle,
                                    memory_read, memory_write, memory_seek,
                                    memory_close, memory_size, memory_map,
                                    memory_unmap, options);
  TIFFOpenOptionsFree(options);
  if (!handle->tiff || !TIFFLastDirectory(handle->tiff))
    goto fail;

  uint32_t width = 0u, height = 0u;
  uint16_t bits = 0u, channels = 0u, photometric = 0u;
  uint16_t planar = 0u, orientation = 0u, sample_format = 0u;
  uint16_t fill_order = 0u, compression = 0u;
  uint16_t extra_count = 0u;
  uint16_t *extra = NULL;
  if (!TIFFGetField(handle->tiff, TIFFTAG_IMAGEWIDTH, &width) ||
      !TIFFGetField(handle->tiff, TIFFTAG_IMAGELENGTH, &height) ||
      !TIFFGetFieldDefaulted(handle->tiff, TIFFTAG_BITSPERSAMPLE, &bits) ||
      !TIFFGetFieldDefaulted(handle->tiff, TIFFTAG_SAMPLESPERPIXEL, &channels) ||
      !TIFFGetFieldDefaulted(handle->tiff, TIFFTAG_PHOTOMETRIC, &photometric) ||
      !TIFFGetFieldDefaulted(handle->tiff, TIFFTAG_PLANARCONFIG, &planar) ||
      !TIFFGetFieldDefaulted(handle->tiff, TIFFTAG_ORIENTATION, &orientation) ||
      !TIFFGetFieldDefaulted(handle->tiff, TIFFTAG_SAMPLEFORMAT, &sample_format) ||
      !TIFFGetFieldDefaulted(handle->tiff, TIFFTAG_FILLORDER, &fill_order) ||
      !TIFFGetFieldDefaulted(handle->tiff, TIFFTAG_COMPRESSION, &compression))
    goto fail;
  if (!width || !height || !max_pixels ||
      (uint64_t)width * height > max_pixels ||
      (uint64_t)width * height > QLIC_TIFF_OUTPUT_LIMIT / 8u ||
      (bits != 8u && (bits < 9u || bits > 16u)) ||
      (channels < 1u || channels > 4u) ||
      (channels <= 2u && photometric != PHOTOMETRIC_MINISBLACK) ||
      (channels >= 3u && photometric != PHOTOMETRIC_RGB) ||
      planar != PLANARCONFIG_CONTIG || orientation != ORIENTATION_TOPLEFT ||
      fill_order != FILLORDER_MSB2LSB || sample_format != SAMPLEFORMAT_UINT ||
      (compression != COMPRESSION_NONE &&
       compression != COMPRESSION_ADOBE_DEFLATE &&
       compression != COMPRESSION_DEFLATE &&
       compression != COMPRESSION_LZW &&
       compression != COMPRESSION_PACKBITS))
    goto fail;
  if (channels == 2u || channels == 4u) {
    if (!TIFFGetField(handle->tiff, TIFFTAG_EXTRASAMPLES,
                      &extra_count, &extra) ||
        extra_count != 1u || !extra ||
        (extra[0] != EXTRASAMPLE_ASSOCALPHA &&
         extra[0] != EXTRASAMPLE_UNASSALPHA))
      goto fail;
    handle->info.alpha_mode =
        extra[0] == EXTRASAMPLE_ASSOCALPHA ? 2u : 1u;
  } else if (TIFFGetField(handle->tiff, TIFFTAG_EXTRASAMPLES,
                          &extra_count, &extra)) {
    goto fail;
  }
  handle->tiled = TIFFIsTiled(handle->tiff) != 0;
  if (handle->tiled) {
    if (!TIFFGetField(handle->tiff, TIFFTAG_TILEWIDTH, &handle->tile_width) ||
        !TIFFGetField(handle->tiff, TIFFTAG_TILELENGTH, &handle->tile_height) ||
        !handle->tile_width || !handle->tile_height)
      goto fail;
    uint64_t across = ((uint64_t)width + handle->tile_width - 1u) /
                      handle->tile_width;
    uint64_t down = ((uint64_t)height + handle->tile_height - 1u) /
                    handle->tile_height;
    if (!across || !down || across > UINT32_MAX / down ||
        TIFFNumberOfTiles(handle->tiff) != across * down)
      goto fail;
  } else {
    if (!TIFFGetFieldDefaulted(handle->tiff, TIFFTAG_ROWSPERSTRIP,
                               &handle->rows_per_strip) ||
        !handle->rows_per_strip)
      goto fail;
    uint64_t strips = ((uint64_t)height + handle->rows_per_strip - 1u) /
                      handle->rows_per_strip;
    if (!strips || strips > UINT32_MAX ||
        TIFFNumberOfStrips(handle->tiff) != strips)
      goto fail;
  }
  handle->info.width = width;
  handle->info.height = height;
  handle->info.bits_per_sample = bits;
  handle->info.channels = channels;
  *info = handle->info;
  *output = handle;
  return 1;
fail:
  qlic_tiff_v1_close(handle);
  return 0;
}

static void unpack_row(const uint8_t *source, uint32_t samples_to_copy,
                       uint32_t bits, uint32_t channels, uint8_t *target) {
  if (bits == 16u) {
    memcpy(target, source, (size_t)samples_to_copy * channels * 2u);
    return;
  }
  uint32_t bytes = bits <= 8u ? 1u : 2u;
  for (uint32_t sample = 0; sample < samples_to_copy * channels; ++sample) {
    uint64_t bit = (uint64_t)sample * bits;
    uint32_t byte = (uint32_t)(bit / 8u);
    uint32_t shift = (uint32_t)(bit % 8u);
    uint32_t value;
    if (bits == 8u) {
      value = source[byte];
    } else {
      uint32_t window = (uint32_t)source[byte] << 16u |
                        (uint32_t)source[byte + 1u] << 8u;
      if (shift + bits > 16u)
        window |= source[byte + 2u];
      value = (window >> (24u - shift - bits)) & ((1u << bits) - 1u);
    }
    if (bytes == 1u)
      target[sample] = (uint8_t)value;
    else
      ((uint16_t *)target)[sample] = (uint16_t)value;
  }
}

static int qlic_tiff_v1_read(QlicTiffHandle *handle, void *pixels,
                      size_t pixels_size, size_t stride) {
  if (!handle || !handle->tiff || !pixels)
    return 0;
  uint32_t width = handle->info.width;
  uint32_t height = handle->info.height;
  uint32_t channels = handle->info.channels;
  uint32_t bits = handle->info.bits_per_sample;
  uint32_t sample_bytes = bits == 8u ? 1u : 2u;
  uint64_t output_row = (uint64_t)width * channels * sample_bytes;
  if (output_row > stride ||
      (uint64_t)(height - 1u) * stride + output_row > pixels_size)
    return 0;
  uint32_t block_width = handle->tiled ? handle->tile_width : width;
  uint32_t block_height = handle->tiled ? handle->tile_height :
                          handle->rows_per_strip;
  uint64_t row_bytes = ((uint64_t)block_width * channels * bits + 7u) / 8u;
  tmsize_t block_size = handle->tiled ? TIFFTileSize(handle->tiff) :
                          TIFFStripSize(handle->tiff);
  if (!row_bytes || block_size <= 0 ||
      (uint64_t)block_size > QLIC_TIFF_INTERNAL_MEMORY_LIMIT ||
      row_bytes > (uint64_t)block_size) 
    return 0;
  uint8_t *buffer = (uint8_t *)malloc((size_t)block_size);
  if (!buffer)
    return 0;
  int ok = 1;
  for (uint32_t y0 = 0u; y0 < height && ok; y0 += block_height) {
    for (uint32_t x0 = 0u; x0 < width && ok;
         x0 += handle->tiled ? block_width : width) {
      uint32_t index = handle->tiled ?
          TIFFComputeTile(handle->tiff, x0, y0, 0u, 0u) :
          TIFFComputeStrip(handle->tiff, y0, 0u);
      tmsize_t got = handle->tiled ?
          TIFFReadEncodedTile(handle->tiff, index, buffer, block_size) :
          TIFFReadEncodedStrip(handle->tiff, index, buffer, block_size);
      uint32_t copy_rows = height - y0 < block_height ? height - y0 :
                           block_height;
      uint32_t copy_cols = width - x0 < block_width ? width - x0 :
                           block_width;
      if (got < 0 || (uint64_t)got < row_bytes * copy_rows) {
        ok = 0;
        break;
      }
      for (uint32_t row = 0u; row < copy_rows; ++row) {
        const uint8_t *source = buffer + (size_t)row * (size_t)row_bytes;
        uint8_t *target = (uint8_t *)pixels + (size_t)(y0 + row) * stride +
                          (size_t)x0 * channels * sample_bytes;
        unpack_row(source, copy_cols, bits, channels, target);
      }
    }
  }
  free(buffer);
  return ok;
}

static int export_tcom(FILE *file, const QlicLiteImage *image)
{
    const QlicLiteDescription *d = &image->description;
    if (d->bits != 8 || d->alpha == QLIC_LITE_ALPHA_PREMULTIPLIED) return 0;
    size_t pixels = image->bytes / d->channels;
    if (pixels > SIZE_MAX - image->bytes - 32) return 0;
    size_t capacity = image->bytes + pixels + 32;
    uint8_t *encoded = malloc(capacity);
    if (!encoded) return 0;
    unsigned dimensions[] = {d->width, d->height, d->channels};
    size_t size = cli_tcom_convert(image->samples, image->bytes, encoded, capacity, dimensions, 0);
    int ok = size && fwrite(encoded, 1, size, file) == size;
    free(encoded);
    return ok;
}

static int export_png(FILE *file, const QlicLiteImage *image)
{
    const QlicLiteDescription *d = &image->description;
    if (d->alpha == QLIC_LITE_ALPHA_PREMULTIPLIED ||
        (d->has_cicp && (d->cicp.primaries > 255 || d->cicp.transfer > 255 ||
                        d->cicp.matrix > 255))) return 0;
    size_t row_size = image->bytes / d->height;
    uint8_t *row = d->bits > 8 ? malloc(row_size) : NULL;
    if (d->bits > 8 && !row) return 0;
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png ? png_create_info_struct(png) : NULL;
    int ok = 0;
    if (info && !setjmp(png_jmpbuf(png))) {
        static const int color[] = {PNG_COLOR_TYPE_GRAY, PNG_COLOR_TYPE_GRAY_ALPHA,
                                    PNG_COLOR_TYPE_RGB, PNG_COLOR_TYPE_RGBA};
        png_init_io(png, file);
        png_set_compression_level(png, 1);
        png_set_IHDR(png, info, d->width, d->height, row ? 16 : 8, color[d->channels - 1],
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        if (d->icc_size)
            png_set_iCCP(png, info, "ICC profile", PNG_COMPRESSION_TYPE_BASE,
                         d->icc, (png_uint_32)d->icc_size);
        if (row) {
            png_byte bits = (png_byte)d->bits;
            png_color_8 significant = {bits, bits, bits, bits, bits};
            png_set_sBIT(png, info, &significant);
        }
        png_write_info(png, info);
        if (d->has_cicp && d->authority != QLIC_LITE_COLOR_ICC_PREFERRED) {
            const uint8_t tag[] = {(uint8_t)d->cicp.primaries, (uint8_t)d->cicp.transfer,
                                   (uint8_t)d->cicp.matrix, d->cicp.full_range};
            png_write_chunk(png, (const png_byte *)"cICP", tag, sizeof(tag));
        }
        if (d->has_mastering && d->has_cicp && d->authority != QLIC_LITE_COLOR_ICC_PREFERRED) {
            uint8_t tag[24];
            for (unsigned i = 0; i < 3; ++i) {
                tag[4*i] = (uint8_t)(d->mastering.x[i] >> 8);
                tag[4*i+1] = (uint8_t)d->mastering.x[i];
                tag[4*i+2] = (uint8_t)(d->mastering.y[i] >> 8);
                tag[4*i+3] = (uint8_t)d->mastering.y[i];
            }
            tag[12] = (uint8_t)(d->mastering.white_x >> 8);
            tag[13] = (uint8_t)d->mastering.white_x;
            tag[14] = (uint8_t)(d->mastering.white_y >> 8);
            tag[15] = (uint8_t)d->mastering.white_y;
            for (unsigned i = 0; i < 4; ++i) {
                tag[16+i] = (uint8_t)(d->mastering.max_luminance >> (24 - i*8));
                tag[20+i] = (uint8_t)(d->mastering.min_luminance >> (24 - i*8));
            }
            png_write_chunk(png, (const png_byte *)"mDCV", tag, sizeof(tag));
        }
        if (d->has_light) {
            uint8_t tag[8];
            for (unsigned i = 0; i < 4; ++i) {
                tag[i] = (uint8_t)(d->max_cll >> (24 - i*8));
                tag[i+4] = (uint8_t)(d->max_fall >> (24 - i*8));
            }
            png_write_chunk(png, (const png_byte *)"cLLI", tag, sizeof(tag));
        }
        unsigned max = (1u << d->bits) - 1;
        for (uint32_t y = 0; y < d->height; ++y) {
            const uint8_t *samples = image->samples + y * row_size;
            if (row) {
                for (size_t x = 0; x < row_size; x += 2) {
                    unsigned expanded = (cli_sample(samples + x, 2) * 65535u + max / 2) / max;
                    row[x] = (uint8_t)(expanded >> 8);
                    row[x+1] = (uint8_t)expanded;
                }
                samples = row;
            }
            png_write_row(png, samples);
        }
        png_write_end(png, info);
        ok = 1;
    }
    if (png) png_destroy_write_struct(&png, &info);
    free(row);
    return ok;
}

static void cli_le32(uint8_t *to, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) to[i] = (uint8_t)(value >> (8 * i));
}

static int export_bmp(FILE *file, const QlicLiteImage *image)
{
    const QlicLiteDescription *d = &image->description;
    if (d->bits != 8 || d->alpha == QLIC_LITE_ALPHA_PREMULTIPLIED) return 0;
    unsigned channels = d->alpha ? 4u : 3u;
    size_t row_size = ((size_t)d->width * channels + 3) & ~(size_t)3;
    uint8_t *row = calloc(1, row_size);
    if (!row) return 0;
    uint8_t header[122] = {'B', 'M'};
    unsigned header_size = channels == 4 ? 122u : 54u;
    cli_le32(header + 2, (uint32_t)(header_size + row_size * d->height));
    cli_le32(header + 10, header_size);
    cli_le32(header + 14, header_size - 14);
    cli_le32(header + 18, d->width);
    cli_le32(header + 22, d->height);
    header[26] = 1;
    header[28] = (uint8_t)(channels * 8);
    if (channels == 4) {
        header[30] = 3;
        cli_le32(header + 54, 0x00ff0000u);
        cli_le32(header + 58, 0x0000ff00u);
        cli_le32(header + 62, 0x000000ffu);
        cli_le32(header + 66, 0xff000000u);
        cli_le32(header + 70, 0x73524742u);
    }
    int ok = fwrite(header, 1, header_size, file) == header_size;
    for (uint32_t y = d->height; ok && y > 0; --y) {
        const uint8_t *samples = image->samples + (size_t)(y - 1) * d->width * d->channels;
        for (uint32_t x = 0; x < d->width; ++x, samples += d->channels) {
            uint8_t *pixel = row + (size_t)x * channels;
            pixel[0] = samples[d->channels >= 3 ? 2 : 0];
            pixel[1] = samples[d->channels >= 3 ? 1 : 0];
            pixel[2] = samples[0];
            if (channels == 4) pixel[3] = samples[d->channels - 1];
        }
        ok = fwrite(row, 1, row_size, file) == row_size;
    }
    free(row);
    return ok;
}

static tmsize_t cli_tiff_read(thandle_t handle, void *data, tmsize_t bytes)
{
    return bytes < 0 ? -1 : (tmsize_t)fread(data, 1, (size_t)bytes, handle);
}

static tmsize_t cli_tiff_write(thandle_t handle, void *data, tmsize_t bytes)
{
    return bytes < 0 ? -1 : (tmsize_t)fwrite(data, 1, (size_t)bytes, handle);
}

static toff_t cli_tiff_seek(thandle_t handle, toff_t offset, int origin)
{
    if (offset > LONG_MAX || fseek(handle, (long)offset, origin) != 0) return (toff_t)-1;
    long at = ftell(handle);
    return at < 0 ? (toff_t)-1 : (toff_t)at;
}

static toff_t cli_tiff_size(thandle_t handle)
{
    long current = ftell(handle);
    if (current < 0 || fseek(handle, 0, SEEK_END) != 0) return 0;
    long end = ftell(handle);
    return fseek(handle, current, SEEK_SET) != 0 || end < 0 ? 0 : (toff_t)end;
}

static int export_tiff(FILE *file, const QlicLiteImage *image)
{
    const QlicLiteDescription *d = &image->description;
    TIFF *tiff = TIFFClientOpen("QLIC Lite output", "wl", file, cli_tiff_read, cli_tiff_write,
                                 cli_tiff_seek, memory_close, cli_tiff_size, memory_map, memory_unmap);
    if (!tiff) return 0;
    uint16_t extra = d->alpha == QLIC_LITE_ALPHA_PREMULTIPLIED ? EXTRASAMPLE_ASSOCALPHA :
                                                               EXTRASAMPLE_UNASSALPHA;
    int ok = TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, d->width) &&
             TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, d->height) &&
             TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, d->channels) &&
             TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, d->bits) &&
             TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, d->channels <= 2 ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB) &&
             TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE) &&
             TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) &&
             TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff, 0)) &&
             (!d->alpha || TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, 1, &extra)) &&
             (!d->icc_size || TIFFSetField(tiff, TIFFTAG_ICCPROFILE, (uint32_t)d->icc_size, d->icc));
    size_t row_samples = (size_t)d->width * d->channels;
    size_t packed_size = (row_samples * d->bits + 7) / 8;
    uint8_t *packed = d->bits > 8 ? malloc(packed_size) : NULL;
    if (d->bits > 8 && !packed) ok = 0;
    size_t stride = image->bytes / d->height;
    for (uint32_t y = 0; ok && y < d->height; ++y) {
        uint8_t *row = image->samples + y * stride;
        if (packed) {
            unsigned word = 0, bits = 0;
            size_t at = 0;
            for (size_t x = 0; x < row_samples; ++x) {
                word = (word << d->bits) | cli_sample(row + 2*x, 2);
                bits += d->bits;
                while (bits >= 8) {
                    bits -= 8;
                    packed[at++] = (uint8_t)(word >> bits);
                }
            }
            if (bits) packed[at] = (uint8_t)(word << (8 - bits));
            row = packed;
        }
        ok = TIFFWriteScanline(tiff, row, y, 0) == 1;
    }
    if (ok) ok = TIFFWriteDirectory(tiff) == 1 && TIFFFlush(tiff) == 1;
    TIFFClose(tiff);
    free(packed);
    return ok && !ferror(file);
}

typedef struct {
  const uint8_t *data;
  size_t size;
  size_t offset;
} CliPngReader;

static void qlic_png_exact_free(QlicPngExact *image) {
  if (!image)
    return;
  free(image->pixels);
  free(image->rows);
  free(image->icc);
  memset(image, 0, sizeof(*image));
}

static void read_bytes(png_structp png, png_bytep output, png_size_t count) {
  CliPngReader *reader = (CliPngReader *)png_get_io_ptr(png);
  if (!reader || count > reader->size - reader->offset)
    png_error(png, "truncated PNG source");
  memcpy(output, reader->data + reader->offset, count);
  reader->offset += count;
}

static void png_error_quiet(png_structp png, png_const_charp message) {
  (void)message;
  png_longjmp(png, 1);
}

static void png_warning_quiet(png_structp png, png_const_charp message) {
  (void)png;
  (void)message;
}

static uint32_t read_be32(const uint8_t *p) {
  return (uint32_t)p[0] << 24u | (uint32_t)p[1] << 16u |
         (uint32_t)p[2] << 8u | (uint32_t)p[3];
}

static int single_image_png(const uint8_t *data, size_t size,
                            QlicPngExact *out) {
  size_t at = 8u;
  int seen_palette = 0;
  int seen_pixels = 0;
  int seen_header = 0;
  int seen_sbit = 0;
  uint8_t depth = 0;
  uint8_t color = 0;
  while (at <= size && size - at >= 12u) {
    uint32_t length = read_be32(data + at);
    if ((size_t)length > size - at - 12u || length > UINT32_MAX - 4u)
      return 0;
    const uint8_t *tag = data + at + 4u;
    if (read_be32(data + at + 8u + length) !=
        (uint32_t)crc32(crc32(0L, Z_NULL, 0), tag,
                        (uInt)(length + 4u)))
      return 0;
    if (!seen_header) {
      if (memcmp(tag, "IHDR", 4u) || length != 13u)
        return 0;
      seen_header = 1;
      depth = tag[12u];
      color = tag[13u];
    } else if (!memcmp(tag, "IHDR", 4u)) {
      return 0;
    }
    if (!memcmp(tag, "acTL", 4u) || !memcmp(tag, "fcTL", 4u) ||
        !memcmp(tag, "fdAT", 4u))
      return 0;
    if (!memcmp(tag, "mDCV", 4u)) {
      if (seen_palette || seen_pixels || out->has_mdcv || length != 24u ||
          read_be32(tag + 20u) < read_be32(tag + 24u))
        return 0;
      memcpy(out->mdcv, tag + 4u, sizeof(out->mdcv));
      out->has_mdcv = 1;
    }
    if (!memcmp(tag, "cLLI", 4u)) {
      if (seen_palette || seen_pixels || out->has_clli || length != 8u ||
          read_be32(tag + 4u) < read_be32(tag + 8u))
        return 0;
      memcpy(out->clli, tag + 4u, sizeof(out->clli));
      out->has_clli = 1;
    }
    if (!memcmp(tag, "PLTE", 4u))
      seen_palette = 1;
    if (!memcmp(tag, "IDAT", 4u))
      seen_pixels = 1;
    if (!memcmp(tag, "iCCP", 4u)) {
      if (seen_palette || seen_pixels || out->source_icc || length < 4u)
        return 0;
      out->source_icc = 1;
    }
    if (!memcmp(tag, "cICP", 4u)) {
      if (seen_palette || seen_pixels || out->source_cicp || length != 4u ||
          tag[7u] > 1u)
        return 0;
      out->source_cicp = 1;
      memcpy(out->cicp, tag + 4u, 4u);
      out->has_cicp = 1;
    }
    if (!memcmp(tag, "sBIT", 4u)) {
      uint32_t expected = color == 0u ? 1u : color == 2u || color == 3u
                              ? 3u : color == 4u ? 2u : color == 6u ? 4u : 0u;
      uint8_t maximum = color == 3u ? 8u : depth;
      if (seen_palette || seen_pixels || seen_sbit || !expected ||
          length != expected || !maximum)
        return 0;
      for (uint32_t index = 0; index < length; ++index)
        if (!tag[4u + index] || tag[4u + index] > maximum)
          return 0;
      memcpy(out->sbit, tag + 4u, length);
      out->sbit_size = length;
      seen_sbit = 1;
    }
    if (!memcmp(tag, "gAMA", 4u) || !memcmp(tag, "cHRM", 4u)) {
      int gamma = !memcmp(tag, "gAMA", 4u);
      if (seen_palette || seen_pixels ||
          (gamma ? out->has_gamma : out->has_chromaticity) ||
          length != (gamma ? 4u : 32u) ||
          (gamma && !read_be32(tag + 4u)))
        return 0;
      if (gamma) out->has_gamma = 1;
      else out->has_chromaticity = 1;
    } else if (!memcmp(tag, "sRGB", 4u)) {
      if (seen_palette || seen_pixels || out->has_srgb || length != 1u ||
          tag[4u] > 3u)
        return 0;
      out->has_srgb = 1;
    }
    at += (size_t)length + 12u;
    if (!memcmp(tag, "IEND", 4u))
      return length == 0u && at == size &&
             (!out->has_mdcv || out->source_cicp);
  }
  return 0;
}

static int qlic_png_exact_decode(const uint8_t *data, size_t size,
                          uint64_t max_pixels, size_t max_decoded_bytes,
                          QlicPngExact *out) {
  if (!out)
    return 0;
  qlic_png_exact_free(out);
  if (!data || size < 45 || !max_pixels || !max_decoded_bytes ||
      png_sig_cmp(data, 0, 8) || !single_image_png(data, size, out)) {
    qlic_png_exact_free(out);
    return 0;
  }
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                         NULL, png_error_quiet,
                                         png_warning_quiet);
  if (!png) {
    qlic_png_exact_free(out);
    return 0;
  }
  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_read_struct(&png, NULL, NULL);
    qlic_png_exact_free(out);
    return 0;
  }
  if (setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png, &info, NULL);
    qlic_png_exact_free(out);
    return 0;
  }
  CliPngReader reader = {data, size, 0};
  png_set_read_fn(png, &reader, read_bytes);
  png_set_user_limits(png, max_pixels > UINT32_MAX
                              ? UINT32_MAX : (png_uint_32)max_pixels,
                      max_pixels > UINT32_MAX
                              ? UINT32_MAX : (png_uint_32)max_pixels);
  png_set_chunk_malloc_max(png, 16777216u);
  png_read_info(png, info);
  png_uint_32 width = png_get_image_width(png, info);
  png_uint_32 height = png_get_image_height(png, info);
  int depth = png_get_bit_depth(png, info);
  int color = png_get_color_type(png, info);
  if (!width || !height || (uint64_t)width * height > max_pixels ||
      (depth != 1 && depth != 2 && depth != 4 && depth != 8 && depth != 16)) {
    png_destroy_read_struct(&png, &info, NULL);
    qlic_png_exact_free(out);
    return 0;
  }
  if (color == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);
  if (color == PNG_COLOR_TYPE_GRAY && depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);
  if (png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);
  if (color == PNG_COLOR_TYPE_GRAY_ALPHA ||
      (color == PNG_COLOR_TYPE_GRAY &&
       png_get_valid(png, info, PNG_INFO_tRNS)))
    png_set_gray_to_rgb(png);
  if (depth == 16)
    png_set_swap(png);
  (void)png_set_interlace_handling(png);
  png_read_update_info(png, info);
  out->bits = png_get_bit_depth(png, info);
  out->channels = png_get_channels(png, info);
  size_t row_bytes = png_get_rowbytes(png, info);
  if ((out->bits != 8 && out->bits != 16) ||
      (out->channels != 1 && out->channels != 3 && out->channels != 4) ||
      row_bytes != (size_t)width * out->channels * (out->bits / 8u) ||
      row_bytes > max_decoded_bytes / height) {
    png_destroy_read_struct(&png, &info, NULL);
    qlic_png_exact_free(out);
    return 0;
  }
  out->pixels_size = row_bytes * height;
  out->pixels = (uint8_t *)malloc(out->pixels_size);
  out->rows = (png_bytep *)malloc(sizeof(*out->rows) * height);
  if (!out->pixels || !out->rows)
    png_error(png, "out of memory");
  for (png_uint_32 y = 0; y < height; ++y)
    out->rows[y] = out->pixels + y * row_bytes;
  png_read_image(png, out->rows);
  png_read_end(png, info);
  free(out->rows);
  out->rows = NULL;
  if (reader.offset != size)
    png_error(png, "trailing PNG data");
  png_charp name = NULL;
  int compression = 0;
  png_bytep profile = NULL;
  png_uint_32 profile_size = 0;
  if (png_get_iCCP(png, info, &name, &compression,
                   &profile, &profile_size)) {
    if (!profile_size || profile_size > 16777216u)
      png_error(png, "invalid ICC profile");
    out->icc = (uint8_t *)malloc(profile_size);
    if (!out->icc)
      png_error(png, "out of memory");
    memcpy(out->icc, profile, profile_size);
    out->icc_size = profile_size;
  }
  if (out->source_icc && !out->icc_size)
    png_error(png, "source color signaling was discarded");
  out->width = width;
  out->height = height;
  png_destroy_read_struct(&png, &info, NULL);
  return 1;
}

static uint16_t le16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] | (uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t *p) {
  return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
         (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static QlicBmpExactStatus bmp_fail(QlicBmpExactStatus status, const char *message,
                               char *error, size_t capacity) {
  if (error && capacity)
    (void)snprintf(error, capacity, "%s", message);
  return status;
}

static int set_pixel(uint8_t *rgba, uint32_t width, uint32_t height,
                     uint32_t x, uint32_t y, const uint8_t *palette,
                     uint32_t palette_count, uint32_t index) {
  if (x >= width || y >= height || index >= palette_count)
    return 0;
  uint8_t *pixel = rgba + ((size_t)y * width + x) * 4u;
  pixel[0] = palette[index * 4u + 2u];
  pixel[1] = palette[index * 4u + 1u];
  pixel[2] = palette[index * 4u];
  pixel[3] = 255u;
  return 1;
}

static void qlic_bmp_exact_free(QlicBmpExact *image) {
  if (!image)
    return;
  free(image->rgba);
  memset(image, 0, sizeof(*image));
}

static QlicBmpExactStatus qlic_bmp_exact_decode(const uint8_t *data, size_t size,
    uint64_t max_pixels, size_t max_decoded_bytes, QlicBmpExact *out,
    char *error, size_t error_capacity) {
  if (!out)
    return bmp_fail(QLIC_BMP_EXACT_INVALID, "invalid BMP output", error,
                error_capacity);
  memset(out, 0, sizeof(*out));
  if (!data || size < 26u || data[0] != 'B' || data[1] != 'M')
    return bmp_fail(QLIC_BMP_EXACT_INVALID, "the BMP file header is invalid",
                error, error_capacity);
  uint32_t declared_file_size = le32(data + 2u);
  uint32_t pixel_offset = le32(data + 10u);
  uint32_t header_size = le32(data + 14u);
  if ((declared_file_size && declared_file_size != size) ||
      le16(data + 6u) || le16(data + 8u) || pixel_offset > size ||
      header_size > size - 14u)
    return bmp_fail(QLIC_BMP_EXACT_INVALID, "the BMP file extent is invalid",
                error, error_capacity);
  if (header_size != 40u && header_size != 108u && header_size != 124u)
    return bmp_fail(QLIC_BMP_EXACT_UNSUPPORTED,
                "this BMP header isn't supported", error,
                error_capacity);
  if (pixel_offset < 14u + header_size || size < 14u + header_size)
    return bmp_fail(QLIC_BMP_EXACT_INVALID, "the BMP pixel offset is invalid",
                error, error_capacity);

  int32_t signed_width = (int32_t)le32(data + 18u);
  int32_t signed_height = (int32_t)le32(data + 22u);
  uint32_t width = signed_width > 0 ? (uint32_t)signed_width : 0u;
  uint32_t height = signed_height == INT32_MIN ? 0u
                    : signed_height < 0 ? (uint32_t)(-signed_height)
                                        : (uint32_t)signed_height;
  uint16_t depth = le16(data + 28u);
  uint32_t compression = le32(data + 30u);
  uint32_t image_size = le32(data + 34u);
  if (le16(data + 26u) != 1u || !width || !height ||
      (uint64_t)width * height > max_pixels ||
      (uint64_t)width * height > max_decoded_bytes / 4u ||
      (uint64_t)width * height > SIZE_MAX / 4u)
    return bmp_fail(QLIC_BMP_EXACT_INVALID,
                "the BMP dimensions or planes exceed limits", error,
                error_capacity);

  int indexed = depth == 1u || depth == 4u || depth == 8u;
  int bgrx32 = compression == 0u && depth == 32u && header_size == 40u;
  int raw = compression == 0u && (indexed || depth == 24u || bgrx32);
  int rle = (compression == 1u && depth == 8u) ||
            (compression == 2u && depth == 4u);
  int rgba32 = compression == 3u && depth == 32u &&
               (header_size == 108u || header_size == 124u);
  if (!raw && !rle && !rgba32)
    return bmp_fail(QLIC_BMP_EXACT_UNSUPPORTED,
                "this BMP pixel layout or compression isn't supported",
                error, error_capacity);
  if (rle && signed_height < 0)
    return bmp_fail(QLIC_BMP_EXACT_INVALID,
                "an RLE BMP has to store its rows from bottom to top", error, error_capacity);

  const uint8_t *palette = NULL;
  uint32_t palette_count = 0u;
  if (indexed) {
    palette_count = le32(data + 46u);
    if (!palette_count)
      palette_count = 1u << depth;
    uint64_t palette_end = 14u + (uint64_t)header_size +
                           (uint64_t)palette_count * 4u;
    if (palette_count > (1u << depth) || palette_end > pixel_offset)
      return bmp_fail(QLIC_BMP_EXACT_INVALID, "the BMP palette is invalid",
                  error, error_capacity);
    palette = data + 14u + header_size;
  }

  const uint8_t *icc = NULL;
  size_t icc_size = 0u;
  int srgb = 0;
  if (header_size >= 108u) {
    uint32_t red = le32(data + 54u);
    uint32_t green = le32(data + 58u);
    uint32_t blue = le32(data + 62u);
    uint32_t alpha = le32(data + 66u);
    uint32_t color_space = le32(data + 70u);
    int canonical_masks = red == 0x00ff0000u && green == 0x0000ff00u &&
                          blue == 0x000000ffu && alpha == 0xff000000u;
    int empty_masks = !red && !green && !blue && !alpha;
    if (rgba32 ? !canonical_masks
               : indexed ? !empty_masks && !canonical_masks
                         : !empty_masks)
      return bmp_fail(QLIC_BMP_EXACT_UNSUPPORTED,
                  "this BMP uses channel masks that aren't supported",
                  error, error_capacity);
    if (color_space == 0x73524742u) {
      srgb = 1;
    } else if (header_size == 124u && color_space == 0x4d424544u) {
      uint32_t relative_offset = le32(data + 126u);
      uint32_t length = le32(data + 130u);
      uint64_t absolute_offset = 14u + (uint64_t)relative_offset;
      if (relative_offset < 124u || length < 128u ||
          length > 16777216u || absolute_offset > size ||
          length > size - absolute_offset)
        return bmp_fail(QLIC_BMP_EXACT_INVALID,
                    "the embedded BMP ICC profile is invalid or exceeds limits",
                    error, error_capacity);
      icc = data + (size_t)absolute_offset;
      icc_size = length;
      if (indexed && absolute_offset <
          14u + (uint64_t)header_size + (uint64_t)palette_count * 4u)
        return bmp_fail(QLIC_BMP_EXACT_INVALID,
                    "the BMP palette overlaps the embedded ICC profile", error,
                    error_capacity);
      if (read_be32(icc) < 128u || read_be32(icc) > length ||
          memcmp(icc + 36u, "acsp", 4u))
        return bmp_fail(QLIC_BMP_EXACT_INVALID,
                    "the embedded BMP ICC profile is invalid", error,
                    error_capacity);
    } else if (header_size == 124u && color_space == 0x4c494e4bu) {
      return bmp_fail(QLIC_BMP_EXACT_INVALID,
                  "linked BMP color profiles are unsupported; embed the ICC profile",
                  error, error_capacity);
    } else {
      return bmp_fail(QLIC_BMP_EXACT_UNSUPPORTED,
                  "this BMP color space isn't supported", error,
                  error_capacity);
    }
  }

  uint64_t row_bytes = ((uint64_t)width * depth + 31u) / 32u * 4u;
  uint64_t pixel_bytes = row_bytes * height;
  if (!rle && (row_bytes > SIZE_MAX || pixel_bytes > size - pixel_offset ||
               (image_size && image_size != pixel_bytes)))
    return bmp_fail(QLIC_BMP_EXACT_INVALID, "the BMP pixel extent is invalid",
                error, error_capacity);
  if (icc && !rle) {
    uint64_t pixel_end = (uint64_t)pixel_offset + pixel_bytes;
    uint64_t profile_at = (uint64_t)(icc - data);
    if (pixel_end > profile_at && profile_at + icc_size > pixel_offset)
      return bmp_fail(QLIC_BMP_EXACT_INVALID,
                  "the BMP pixels overlap the embedded ICC profile", error,
                  error_capacity);
  }
  if (rle && image_size > size - pixel_offset)
    return bmp_fail(QLIC_BMP_EXACT_INVALID, "the BMP RLE extent is invalid",
                error, error_capacity);
  if (rle && icc)
    return bmp_fail(QLIC_BMP_EXACT_UNSUPPORTED,
                "RLE BMP files with an embedded ICC profile aren't supported",
                error, error_capacity);

  size_t decoded_size = (size_t)width * height * 4u;
  uint8_t *rgba = (uint8_t *)malloc(decoded_size);
  if (!rgba)
    return bmp_fail(QLIC_BMP_EXACT_INVALID, "out of memory", error,
                error_capacity);
  if (indexed) {
    for (size_t index = 0; index < (size_t)width * height; ++index) {
      rgba[index * 4u] = palette[2];
      rgba[index * 4u + 1u] = palette[1];
      rgba[index * 4u + 2u] = palette[0];
      rgba[index * 4u + 3u] = 255u;
    }
  }
  if (!rle) {
    for (uint32_t y = 0u; y < height; ++y) {
      uint32_t source_y = signed_height < 0 ? y : height - 1u - y;
      const uint8_t *source = data + pixel_offset +
                              (size_t)source_y * (size_t)row_bytes;
      uint8_t *target = rgba + (size_t)y * width * 4u;
      for (uint32_t x = 0u; x < width; ++x) {
        if (indexed) {
          uint32_t index = depth == 1u
              ? (source[x >> 3u] >> (7u - (x & 7u))) & 1u
              : depth == 4u
                  ? (source[x >> 1u] >> ((x & 1u) ? 0u : 4u)) & 15u
                  : source[x];
          if (!set_pixel(rgba, width, height, x, y, palette,
                         palette_count, index)) {
            free(rgba);
            return bmp_fail(QLIC_BMP_EXACT_INVALID,
                        "the BMP palette index is invalid", error,
                        error_capacity);
          }
        } else {
          const uint8_t *pixel = source + (size_t)x * (depth / 8u);
          target[x * 4u] = pixel[2];
          target[x * 4u + 1u] = pixel[1];
          target[x * 4u + 2u] = pixel[0];
          target[x * 4u + 3u] = rgba32 ? pixel[3] : 255u;
        }
      }
    }
  } else {
    size_t position = pixel_offset;
    size_t end = image_size ? pixel_offset + image_size : size;
    uint32_t x = 0u;
    uint32_t line = 0u;
    int finished = 0;
    while (position + 2u <= end && !finished) {
      uint32_t count = data[position++];
      uint32_t value = data[position++];
      if (count) {
        if (line >= height || x > width || count > width - x)
          break;
        for (uint32_t index = 0u; index < count; ++index) {
          uint32_t palette_index = depth == 8u ? value
              : (value >> ((index & 1u) ? 0u : 4u)) & 15u;
          if (!set_pixel(rgba, width, height, x + index,
                         height - 1u - line, palette, palette_count,
                         palette_index))
            goto invalid_rle;
        }
        x += count;
      } else if (value == 0u) {
        if (line >= height)
          break;
        x = 0u;
        ++line;
      } else if (value == 1u) {
        finished = 1;
      } else if (value == 2u) {
        if (position + 2u > end || line >= height)
          break;
        uint32_t dx = data[position++];
        uint32_t dy = data[position++];
        if (x > width || dx > width - x || dy >= height - line)
          break;
        x += dx;
        line += dy;
      } else {
        uint32_t length = value;
        size_t packed = depth == 8u ? length : (length + 1u) / 2u;
        if (line >= height || x > width || length > width - x ||
            packed + (packed & 1u) > end - position)
          break;
        for (uint32_t index = 0u; index < length; ++index) {
          uint32_t palette_index = depth == 8u ? data[position + index]
              : (data[position + (index >> 1u)] >>
                 ((index & 1u) ? 0u : 4u)) & 15u;
          if (!set_pixel(rgba, width, height, x + index,
                         height - 1u - line, palette, palette_count,
                         palette_index))
            goto invalid_rle;
        }
        position += packed + (packed & 1u);
        x += length;
      }
    }
    if (!finished)
      goto invalid_rle;
  }
  out->rgba = rgba;
  out->width = width;
  out->height = height;
  out->channels = rgba32 ? 4u : 3u;
  out->alpha_mode = rgba32 ? 1u : 0u;
  out->icc = icc;
  out->icc_size = icc_size;
  out->srgb = srgb;
  return QLIC_BMP_EXACT_OK;

invalid_rle:
  free(rgba);
  return bmp_fail(QLIC_BMP_EXACT_INVALID, "the BMP RLE data is invalid", error,
              error_capacity);
}

static void warn_source_compression(int compression)
{
    if (compression == SOURCE_LOSSLESS) return;
#ifdef _WIN32
    HANDLE stream = GetStdHandle(STD_ERROR_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO console;
    int colored = GetConsoleScreenBufferInfo(stream, &console) &&
        SetConsoleTextAttribute(stream, (console.wAttributes & ~0x0f) |
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
    int colored = isatty(STDERR_FILENO);
    if (colored) fputs("\033[33m", stderr);
#endif
    fputs(compression == SOURCE_LOSSY ? "Warning: the input uses lossy compression. " :
          "Note: JPEG XL can use lossy compression. ", stderr);
    fputs("Lossy inputs are often smaller because they discard image data. "
          "QLIC Lite preserves the decoded pixels.\n", stderr);
#ifdef _WIN32
    if (colored) SetConsoleTextAttribute(stream, console.wAttributes);
#else
    if (colored) fputs("\033[0m", stderr);
#endif
}

typedef int ImageImporter(CliImage *, const uint8_t *, size_t, const QlicLiteLimits *);

static ImageImporter *image_importer(CliFormat format)
{
    switch (format) {
    case FORMAT_PNG: return import_png;
    case FORMAT_BMP: return import_bmp;
    case FORMAT_TIFF: return import_tiff;
    case FORMAT_JPEG: return import_jpeg;
    case FORMAT_JXL: return import_jxl;
    case FORMAT_WEBP: return import_webp;
    case FORMAT_TCOM: return import_tcom;
    case FORMAT_PNM: case FORMAT_PAM: case FORMAT_PBM: case FORMAT_PGM: case FORMAT_PPM:
        return import_pnm;
    default: return NULL;
    }
}

static WriteStatus pack_image(const PathChar *source, const PathChar *target, ImageImporter *import)
{
    if (!import) return FILE_FAILED;
    QlicLiteLimits limits;
    command_limits(&limits);
    size_t bytes = 0;
    uint8_t *data = read_file(source, 1, limits.input_bytes, &bytes);
    if (!data) return FILE_FAILED;
    CliImage image = {0};
    int ok = import(&image, data, bytes, &limits);
    free(data);
    QlicLiteFile file = {0};
    ok = ok && qlic_lite_encode(&image.description, image.samples, image.bytes,
                              image.stride, &limits, &file, NULL) == QLIC_LITE_OK;
    free(image.samples);
    free(image.icc);
    WriteStatus status = ok ? write_new_file(target, file.data, file.bytes) : FILE_FAILED;
    qlic_lite_file_free(&file);
    if (status == FILE_WRITTEN) warn_source_compression(image.source_compression);
    return status;
}

static int write_decoded_image(const PathChar *target, const QlicLiteImage *image)
{
    CliFormat format = image_format(extension(target));
    const QlicLiteDescription *d = &image->description;
    if (format == FORMAT_RAW) {
        int ok = write_new_file(target, image->samples, image->bytes) == FILE_WRITTEN;
        if (ok && (d->icc_size || d->has_cicp || d->has_mastering || d->has_light))
            fputs("Warning: raw output contains samples only. Color and HDR tags are omitted.\n", stderr);
        return ok;
    }

    int png = format == FORMAT_PNG;
    int bmp = format == FORMAT_BMP;
    int tcom = format == FORMAT_TCOM;
    int tiff = format == FORMAT_TIFF;
    int pnm = format >= FORMAT_PNM && format <= FORMAT_PPM;
    if (!png && !bmp && !tiff && !pnm && !tcom) return 0;
    FILE *file = open_path(target, PATH_TEXT("w+bx"));
    if (!file) return 0;
    int ok = png ? export_png(file, image) : bmp ? export_bmp(file, image) :
             tiff ? export_tiff(file, image) : tcom ? export_tcom(file, image) :
             export_pnm(file, image, format);
    ok = finish_file(file,target,ok);
    if (ok && ((d->icc_size && !png && !tiff) ||
               ((d->has_cicp || d->has_mastering || d->has_light) && !png) ||
               (png && ((d->has_mastering && !d->has_cicp) ||
                        d->authority == QLIC_LITE_COLOR_ICC_PREFERRED))))
        fputs("Warning: some color or HDR information is omitted in this output format.\n", stderr);
    return ok;
}

typedef struct {
    size_t written, skipped, failed;
} BatchCounts;

static size_t batch_path_length(const PathChar *path)
{
#ifdef _WIN32
    return wcslen(path);
#else
    return strlen(path);
#endif
}

static int batch_prompt(PathChar *path, size_t capacity)
{
    fputs("Paste folder path here: ", stdout);
    fflush(stdout);
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode, count;
    if (GetConsoleMode(console, &mode)) {
        if (!ReadConsoleW(console, path, (DWORD)(capacity - 1), &count, NULL) ||
            !count || (path[count - 1] != '\n' && path[count - 1] != '\r')) return 0;
        path[count] = 0;
    } else {
        char input[32770];
        if (!fgets(input, sizeof(input), stdin) ||
            (!strchr(input, '\n') && !feof(stdin)) ||
            !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input, -1, path, (int)capacity))
            return 0;
    }
#else
    if (!fgets(path, (int)capacity, stdin) || (!strchr(path, '\n') && !feof(stdin))) return 0;
#endif
    size_t length = batch_path_length(path);
    while (length && (path[length - 1] == '\n' || path[length - 1] == '\r')) path[--length] = 0;
    if (length >= 2 && path[0] == '"' && path[length - 1] == '"') {
        length -= 2;
        memmove(path, path + 1, length * sizeof(*path));
        path[length] = 0;
    }
    return length != 0;
}

static PathChar *batch_join(const PathChar *folder, const PathChar *name)
{
    const size_t folder_len = batch_path_length(folder), name_len = batch_path_length(name);
    const PathChar last = folder[folder_len - 1];
    const int add_separator = last != '/'
#ifdef _WIN32
        && last != '\\'
#endif
        ;
    if (folder_len > SIZE_MAX / sizeof(PathChar) - name_len - 2) return NULL;
    PathChar *path = malloc((folder_len + name_len + add_separator + 1) * sizeof(*path));
    if (!path) return NULL;
    memcpy(path, folder, folder_len * sizeof(*path));
#ifdef _WIN32
    if (add_separator) path[folder_len] = '\\';
#else
    if (add_separator) path[folder_len] = '/';
#endif
    memcpy(path + folder_len + add_separator, name, (name_len + 1) * sizeof(*path));
    return path;
}

static void batch_notice(const char *message, const PathChar *path)
{
#ifdef _WIN32
    char name[4 * 260 + 1];
    if (!WideCharToMultiByte(CP_UTF8, 0, path, -1, name, (int)sizeof(name), NULL, NULL))
        strcpy(name, "(image)");
    fprintf(stderr, "%s %s\n", message, name);
#else
    fprintf(stderr, "%s %s\n", message, path);
#endif
}

static void batch_image(const PathChar *folder, const PathChar *name, BatchCounts *counts)
{
    ImageImporter *import = image_importer(image_format(extension(name)));
    if (!import) return;
    PathChar *source = batch_join(folder, name);
#ifndef _WIN32
    struct stat file_info;
    if (source && stat(source, &file_info) == 0 && !S_ISREG(file_info.st_mode)) {
        free(source);
        return;
    }
#endif
    PathChar *target = source ? default_output(source) : NULL;
    WriteStatus result = FILE_FAILED;
    if (target) {
#ifdef _WIN32
        const int exists = GetFileAttributesW(target) != INVALID_FILE_ATTRIBUTES;
#else
        struct stat target_info;
        const int exists = stat(target, &target_info) == 0;
#endif
        result = exists ? FILE_EXISTS : pack_image(source, target, import);
    }
    if (result == FILE_WRITTEN) ++counts->written;
    else if (result == FILE_EXISTS) {
        ++counts->skipped;
        batch_notice("Skipped existing output for", name);
    } else {
        ++counts->failed;
        batch_notice("Could not pack", name);
    }
    free(target);
    free(source);
}

static int batch_scan(const PathChar *folder, BatchCounts *counts)
{
#ifdef _WIN32
    DWORD attributes = GetFileAttributesW(folder);
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) return 0;
    PathChar *pattern = batch_join(folder, PATH_TEXT("*"));
    if (!pattern) return 0;
    WIN32_FIND_DATAW item;
    HANDLE search = FindFirstFileW(pattern, &item);
    const DWORD error = search == INVALID_HANDLE_VALUE ? GetLastError() : ERROR_SUCCESS;
    free(pattern);
    if (search == INVALID_HANDLE_VALUE) return error == ERROR_FILE_NOT_FOUND;
    do {
        if (!(item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            batch_image(folder, item.cFileName, counts);
    } while (FindNextFileW(search, &item));
    const int complete = GetLastError() == ERROR_NO_MORE_FILES;
    FindClose(search);
    return complete;
#else
    DIR *directory = opendir(folder);
    if (!directory) return 0;
    struct dirent *item;
    for (;;) {
        errno = 0;
        item = readdir(directory);
        if (!item) break;
        batch_image(folder, item->d_name, counts);
    }
    const int complete = errno == 0;
    closedir(directory);
    return complete;
#endif
}

static int batch_pack(const PathChar *folder)
{
    PathChar input[32770];
    if (!folder) {
        if (!batch_prompt(input, sizeof(input) / sizeof(*input))) {
            fputs("Could not read a folder path.\n", stderr);
            return 1;
        }
        folder = input;
    }
    BatchCounts counts = {0};
    const int complete = *folder && batch_scan(folder, &counts);
    if (!complete) fputs("Could not scan the folder.\n", stderr);
    if (complete && !counts.written && !counts.skipped && !counts.failed)
        puts("No supported images found.");
    else
        printf("Packed %zu, skipped %zu, failed %zu.\n", counts.written, counts.skipped, counts.failed);
    return !complete || counts.failed != 0;
}
#endif

static int pack(const PathChar *source, const PathChar *target, unsigned w, unsigned h)
{
    QlicLiteLimits limits;
    command_limits(&limits);
    size_t stride = 4 * (size_t)w, expected = stride * h, bytes = 0;
    uint8_t *raw = read_file(source, expected, expected, &bytes);
    if (!raw) return 0;
    QlicLiteDescription description = {
        .width = w, .height = h, .bits = 8, .channels = 4, .alpha = QLIC_LITE_ALPHA_STRAIGHT
    };
    QlicLiteFile file = {0};
    int ok = qlic_lite_encode(&description, raw, bytes, stride, &limits, &file, NULL) ==
             QLIC_LITE_OK && write_new_file(target, file.data, file.bytes) == FILE_WRITTEN;
    qlic_lite_file_free(&file);
    free(raw);
    return ok;
}

static int unpack(const PathChar *source, const PathChar *target)
{
    QlicLiteLimits limits;
    command_limits(&limits);
    size_t bytes = 0;
    uint8_t *file = read_file(source, 1, limits.input_bytes, &bytes);
    if (!file) return 0;
    QlicLiteImage image = {0};
    int ok = qlic_lite_decode(file, bytes, &limits, NULL, &image, NULL) == QLIC_LITE_OK;
    if (ok) {
#ifdef QLIC_LITE_IMAGE_IO
        ok = write_decoded_image(target, &image);
#else
        ok = write_new_file(target, image.samples, image.bytes) == FILE_WRITTEN;
#endif
    }
    qlic_lite_image_free(&image);
    free(file);
    return ok;
}

static void usage(FILE *stream)
{
#ifdef QLIC_LITE_IMAGE_IO
    fputs("qlic_lite pack INPUT [OUTPUT.lite]\n"
          "qlic_lite pack WIDTH HEIGHT INPUT.bin [OUTPUT.lite]\n"
          "qlic_lite unpack INPUT.lite OUTPUT\n"
          "qlic_lite                    Prompt for a folder to pack\n"
          "qlic_lite batch [FOLDER]\n"
          "Input: PNG, WebP, JPEG, JXL, BMP, TIFF, TCOM, PNM/PBM/PGM/PPM/PAM, or raw RGBA8.\n"
          "Output: PNG, BMP, TIFF, TCOM, PNM/PBM/PGM/PPM/PAM, or raw BIN.\n", stream);
#else
    fputs("qlic_lite pack WIDTH HEIGHT INPUT.rgba [OUTPUT.lite]\n"
          "qlic_lite unpack INPUT.lite OUTPUT.raw\n", stream);
#endif
}

int cli_main(int argc, PathChar **argv)
{
    if (argc == 2 && path_compare(argv[1], PATH_TEXT("--version")) == 0) {
        puts("QLIC Lite 1.0");
        return 0;
    }
    if (argc == 2 && path_compare(argv[1], PATH_TEXT("--help")) == 0) {
        usage(stdout);
        return 0;
    }
#ifdef QLIC_LITE_IMAGE_IO
    if (argc == 1) return batch_pack(NULL);
    if ((argc == 2 || argc == 3) && path_compare(argv[1], PATH_TEXT("batch")) == 0)
        return batch_pack(argc == 3 ? argv[2] : NULL);
    if ((argc == 3 || argc == 4) && path_compare(argv[1], PATH_TEXT("pack")) == 0) {
        PathChar *generated = argc == 3 ? default_output(argv[2]) : NULL;
        const PathChar *target = argc == 4 ? argv[3] : generated;
        CliFormat format = image_format(extension(argv[2]));
        int ok = target && has_lite_extension(target) &&
            pack_image(argv[2], target, image_importer(format)) == FILE_WRITTEN;
        free(generated);
        if (!ok) {
            fputs("Could not pack this image. Check the format, precision, and output path.\n", stderr);
            if (format == FORMAT_RAW)
                fputs("Raw input needs width and height.\n", stderr);
        }
        return ok ? 0 : 1;
    }
#endif
    if ((argc == 5 || argc == 6) && path_compare(argv[1], PATH_TEXT("pack")) == 0) {
        unsigned w = dimension(argv[2]), h = dimension(argv[3]);
        if (w && h && (size_t)w*h <= (1u << 24)) {
            PathChar *generated = argc == 5 ? default_output(argv[4]) : NULL;
            const PathChar *target = argc == 6 ? argv[5] : generated;
            if (target && !has_lite_extension(target)) {
                free(generated);
                fputs("The output filename must end in .lite.\n", stderr);
                return 1;
            }
            int ok = target && pack(argv[4], target, w, h);
            free(generated);
            if (!ok) fputs("Could not pack the image. Check the input and output paths.\n", stderr);
            return ok ? 0 : 1;
        }
    } else if (argc == 4 && path_compare(argv[1], PATH_TEXT("unpack")) == 0) {
        if (unpack(argv[2], argv[3])) return 0;
        fputs("Could not unpack the image. Check the input and output paths.\n", stderr);
        return 1;
    }
    usage(stderr);
    return 2;
}
