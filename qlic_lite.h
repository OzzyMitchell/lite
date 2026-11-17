/* Copyright (C) 2026 Ozzy M.
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#ifndef QLIC_LITE_H
#define QLIC_LITE_H

/*
QLIC Lite

Include this header and link with the qlic_lite library. For decoding specifically,
include qlic_lite_decoder.h and link with qlic_lite_decoder.

This example reads a .lite image that's in memory.

    QlicLiteLimits limits;
    qlic_lite_limits_default(&limits);

    QlicLiteImage image = {0};
    QlicLiteStatus status =
        qlic_lite_decode(data, size, &limits, NULL, &image, NULL);

If the call returns QLIC_LITE_OK, the pixel data is in image.samples
and the image size, channel count, and bit depth are in image.description.
Use qlic_lite_image_free when you're done with the image.

To encode, fill in QlicLiteDescription and pass it to qlic_lite_encode
with the pixel data and the distance in bytes between row starts.

Start with QlicLiteFile file = {0}. If the call returns QLIC_LITE_OK, file.data
and file.bytes will give you the complete .lite file. Use qlic_lite_file_free
when you're done with that.

Pixels and memory

The free functions clear the output so you can use it again. Freeing the
image also frees its ICC profile.

Pixels run left to right, top to bottom, with each pixel's channel values
together. Each of the decoded rows have width * channels values.

At 8 bits per channel, each value takes one byte.
At 9 or 10 bits, each takes two bytes, starting with the low byte.

Start with the default limits and lower them if you need to. A small .lite
file can turn into a large image in memory, so use pixel_data_bytes to cap
the pixel data. The encoder checks that limit on what you give it and uses
file_bytes to limit how big a .lite file it gives back.

Pass NULL for the decoder's allocator to use the default. If you use your
own allocator, keep whatever it needs around until you're done with the
image, since freeing the image calls your release function. If the
allocator calls encode or decode again on the same thread during a decode,
that call returns QLIC_LITE_REENTRANT.

How the file stores pixels

The first four bytes tell the decoder which layout to read. If an image
uses a few colors over and over, the encoder can store those colors once
in a palette and refer to them by number. When nearby pixels are similar,
it can predict a channel value and store the difference, and the decoder
adds that difference to the same prediction to get the value back.

QHI1 stores the image description with the pixel data, including any ICC
profile. For a 10 bit image, it can use the 8 bit codec for the top eight
bits and keep the last two bits beside the compressed data, and the
decoder puts them back together again. If that doesn't save space, QHI1
packs the full values directly.
*/

#include "qlic_lite_decoder.h"

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
