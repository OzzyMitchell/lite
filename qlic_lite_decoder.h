/* Copyright (C) 2026 Ozzy M.
 * SPDX-License-Identifier: Apache-2.0
 */

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
