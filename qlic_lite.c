/* Copyright (C) 2026 Ozzy M.
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif !defined(QLIC_WASM)
#include <pthread.h>
#endif

#ifdef _MSC_VER
#define QLI_INLINE __forceinline
#define QLI_NOINLINE __declspec(noinline)
#else
#define QLI_INLINE inline __attribute__((always_inline))
#define QLI_NOINLINE __attribute__((noinline))
#endif

#define QLI_TIER_SCALAR 0
#define QLI_TIER_SSE42 1
#define QLI_TIER_AVX2 2

#if !defined(QLI_MAX_CPU_TIER)
#if defined(_M_X64) || defined(__x86_64__)
#define QLI_MAX_CPU_TIER QLI_TIER_AVX2
#else
#define QLI_MAX_CPU_TIER QLI_TIER_SCALAR
#endif
#endif

#if QLI_MAX_CPU_TIER < QLI_TIER_SCALAR || QLI_MAX_CPU_TIER > QLI_TIER_AVX2
#error Invalid QLIC Lite CPU tier
#endif

enum {
    QLI_CPU_SSE42 = 1u << 0,
    QLI_CPU_AVX2 = 1u << 1,
    QLI_CPU_PCLMUL_CRC = 1u << 2
};

unsigned qli_cpu_capabilities(void);

#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42
#include <immintrin.h>
#include <stdatomic.h>
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
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

enum { QLH_OK, QLH_ARGUMENT, QLH_DESCRIPTION, QLH_LIMIT, QLH_LENGTH,
       QLH_CHECKSUM, QLH_CAPACITY, QLH_SAMPLE, QLH_OVERLAP };

typedef struct {
    uint64_t max_pixels, max_metadata, max_record, max_sample_bytes;
} QlhLimits;

int qlh_record_size(const QlicLiteDescription *, const QlhLimits *, size_t *);
int qlh_write(const QlicLiteDescription *, const QlhLimits *, uint8_t *, size_t,
              size_t *written);
int qlh_parse(const uint8_t *, size_t, const QlhLimits *, QlicLiteDescription *);
int qlh_validate_samples(const QlicLiteDescription *, const QlhLimits *,
                         const uint8_t *, size_t, size_t stride);
uint32_t qint_crc32(const uint8_t *, size_t);

int qli_allocation_active(void);

void *qli_malloc(size_t);
void *qli_calloc(size_t,size_t);
void qli_free(void *);

typedef enum {
    QLI_FORMAT_UNKNOWN, QLI_FORMAT_FAST, QLI_FORMAT_SIMPLE, QLI_FORMAT_TILE,
    QLI_FORMAT_STRIP, QLI_FORMAT_NUMERICAL, QLI_FORMAT_PALETTE, QLI_FORMAT_HISTORY,
    QLI_FORMAT_TRANSPOSED_HISTORY, QLI_FORMAT_AFFINE, QLI_FORMAT_ICON, QLI_FORMAT_SAMPLES
} QliFormat;
typedef struct {
    QlicLiteDescription desc;
    size_t desc_bytes, core_bytes, decoded_bytes;
    QliFormat format;
    unsigned tail_bits;
} QliView;
static inline size_t qli_packed_bytes(size_t count, unsigned bits)
{
    return (count * bits + 7) / 8;
}
static inline QlhLimits qli_description_limits(const QlicLiteLimits *limits)
{
    size_t metadata = limits->metadata_bytes < (1u << 24) ? limits->metadata_bytes : (1u << 24);
    return (QlhLimits){limits->pixels, metadata, metadata + 32, limits->pixel_data_bytes};
}
QlicLiteStatus qli_inspect(const uint8_t *,size_t,const QlicLiteLimits *,QliView *);
QlicLiteStatus qli_decode_body(const uint8_t *,size_t,const QliView *,uint8_t **,size_t *,QlicLiteDescription *);
typedef QlicLiteStatus QliEncodeBody(const QlicLiteDescription *, const uint8_t *, size_t, size_t,
                                const QlicLiteLimits *, uint8_t **, size_t *);
QlicLiteStatus qli_encode_owned(const QlicLiteDescription *, const uint8_t *, size_t, size_t,
                           const QlicLiteLimits *, const QlicLiteAllocator *, QlicLiteFile *, QlicLiteStats *,
                           QliEncodeBody *);

enum { ALLOCATION_SLOTS = 256 };

typedef struct {
    void *pointer;
    size_t bytes;
} AllocationSlot;

typedef struct {
    AllocationSlot inline_slots[ALLOCATION_SLOTS];
    AllocationSlot *slots;
    size_t slot_capacity, slot_limit;
    size_t live_bytes, live_blocks;
    QlicLiteLimits limits;
    QlicLiteAllocator allocator;
    QlicLiteStats stats;
    QlicLiteStatus failure;
} AllocationContext;

static _Thread_local AllocationContext *active_allocations;

static _Thread_local int heap_limit_hit;

#if defined(QLIC_LITE_BUILD_SHARED) && defined(_WIN32)
__declspec(dllexport)
#elif defined(QLIC_LITE_BUILD_SHARED) && (defined(__GNUC__) || defined(__clang__))
__attribute__((visibility("default")))
#endif
int qli_take_heap_limit(void)
{
    int hit = heap_limit_hit;
    heap_limit_hit = 0;
    return hit;
}

static void *ordinary_allocate(size_t bytes, void *user)
{
    (void)user;
    return malloc(bytes);
}

static void ordinary_release(void *block, void *user)
{
    (void)user;
    free(block);
}

static void record_failure(AllocationContext *context, QlicLiteStatus status)
{
    if (context->failure == QLIC_LITE_OK) context->failure = status;
}

static void *allocate_block(AllocationContext *context, size_t bytes)
{
    ++context->stats.allocator_calls;
    void *block = context->allocator.allocate(bytes, context->allocator.user);
    if (!block) {
        ++context->stats.allocator_failures;
        record_failure(context, QLIC_LITE_ALLOCATOR);
        return NULL;
    }

    context->live_bytes += bytes;
    if (context->live_bytes > context->stats.peak_bytes)
        context->stats.peak_bytes = context->live_bytes;
    return block;
}

static QLI_NOINLINE int grow_slots(AllocationContext *context, size_t pending)
{
    size_t max_slots = SIZE_MAX / sizeof(AllocationSlot);
    if (max_slots > context->slot_limit) max_slots = context->slot_limit;
    if (context->slot_capacity >= max_slots ||
        context->stats.requests >= context->limits.allocation_requests) return 0;

    size_t capacity = context->slot_capacity * 2;
    if (capacity > max_slots) capacity = max_slots;
    size_t bytes = capacity * sizeof(AllocationSlot);
    if (bytes > context->limits.heap_bytes - context->live_bytes - pending) {
        heap_limit_hit = 1;
        return 0;
    }

    ++context->stats.requests;
    AllocationSlot *slots = allocate_block(context, bytes);
    if (!slots) return 0;
    size_t old_bytes = context->slot_capacity * sizeof(AllocationSlot);
    memcpy(slots, context->slots, old_bytes);
    if (context->slots != context->inline_slots) {
        context->allocator.release(context->slots, context->allocator.user);
        context->live_bytes -= old_bytes;
    }
    context->slots = slots;
    context->slot_capacity = capacity;
    return 1;
}

QLI_NOINLINE void *qli_malloc(size_t bytes)
{
    AllocationContext *context = active_allocations;
    if (!context || context->failure != QLIC_LITE_OK) return NULL;
    if (context->stats.requests == SIZE_MAX) goto limited;

    ++context->stats.requests;
    size_t actual = bytes ? bytes : 1;
    if (context->stats.requests > context->limits.allocation_requests ||
        actual > context->limits.heap_bytes - context->live_bytes) goto limited;
    if (context->live_blocks == context->slot_capacity && !grow_slots(context, actual)) {
        if (context->failure != QLIC_LITE_OK) return NULL;
        goto limited;
    }
    void *block = allocate_block(context, actual);
    if (block) context->slots[context->live_blocks++] = (AllocationSlot){block, actual};
    return block;

limited:
    if (context->stats.requests < SIZE_MAX && context->stats.requests <= context->limits.allocation_requests &&
        (bytes ? bytes : 1) > context->limits.heap_bytes - context->live_bytes) heap_limit_hit = 1;
    ++context->stats.limit_denials;
    record_failure(context, QLIC_LITE_LIMIT);
    return NULL;
}

static size_t find_allocation(const AllocationContext *context, const void *block)
{
    for (size_t i = 0; i < context->live_blocks; ++i)
        if (context->slots[i].pointer == block) return i;
    return context->live_blocks;
}

void qli_free(void *block)
{
    if (!block) return;
    AllocationContext *context = active_allocations;
    if (!context) return;
    size_t i = find_allocation(context, block);
    if (i == context->live_blocks) {
        ++context->stats.internal_errors;
        record_failure(context, QLIC_LITE_INTERNAL);
        return;
    }

    context->live_bytes -= context->slots[i].bytes;
    context->slots[i] = context->slots[--context->live_blocks];
    context->allocator.release(block, context->allocator.user);
}

void *qli_calloc(size_t count, size_t bytes)
{
    if (bytes && count > SIZE_MAX / bytes) {
        if (active_allocations) {
            ++active_allocations->stats.limit_denials;
            record_failure(active_allocations, QLIC_LITE_LIMIT);
        }
        return NULL;
    }
    size_t size = count * bytes;
    void *block = qli_malloc(size);
    if (block) memset(block, 0, size);
    return block;
}

static QlicLiteStatus check_allocator(const QlicLiteLimits *limits, const QlicLiteAllocator *allocator)
{
    if (active_allocations) return QLIC_LITE_REENTRANT;
    if (!limits || (allocator && (!allocator->allocate != !allocator->release)))
        return QLIC_LITE_INVALID;
    return QLIC_LITE_OK;
}

static void start_allocations(AllocationContext *context, const QlicLiteLimits *limits,
                              const QlicLiteAllocator *allocator)
{
    context->slots = context->inline_slots;
    context->slot_capacity = ALLOCATION_SLOTS;
    context->slot_limit = ALLOCATION_SLOTS;
    context->live_bytes = 0;
    context->live_blocks = 0;
    context->stats = (QlicLiteStats){0};
    context->failure = QLIC_LITE_OK;
    context->limits = *limits;
    context->allocator = allocator ? *allocator : (QlicLiteAllocator){0};
    if (!context->allocator.allocate)
        context->allocator = (QlicLiteAllocator){ordinary_allocate, ordinary_release, NULL};
    active_allocations = context;
}

static QlicLiteStatus finish_allocations(AllocationContext *context, QlicLiteStatus status,
                                    uint8_t **output, size_t bytes, QlicLiteStats *stats)
{
    if (context->failure != QLIC_LITE_OK) status = context->failure;
    if (status == QLIC_LITE_OK) {
        if (!*output || !bytes || context->live_blocks != 1 ||
            context->slots[0].pointer != *output || context->slots[0].bytes < bytes) {
            ++context->stats.internal_errors;
            status = QLIC_LITE_INTERNAL;
        } else {
            context->stats.retained_bytes = context->slots[0].bytes;
            context->live_blocks = 0;
            context->live_bytes = 0;
        }
    }
    if (status != QLIC_LITE_OK) {
        qli_free(*output);
        *output = NULL;
        if (context->live_blocks) {
            ++context->stats.internal_errors;
            status = QLIC_LITE_INTERNAL;
        }
        while (context->live_blocks)
            qli_free(context->slots[context->live_blocks - 1].pointer);
    }
    if (context->slots != context->inline_slots)
        context->allocator.release(context->slots, context->allocator.user);
    if (context->stats.internal_errors) status = QLIC_LITE_INTERNAL;
    active_allocations = NULL;
    if (stats) *stats = context->stats;
    return status;
}

void qlic_lite_limits_default(QlicLiteLimits *limits)
{
    if (limits)
        *limits = (QlicLiteLimits){(size_t)1 << 28, (size_t)1 << 25, (size_t)1 << 28,
                             (size_t)1 << 28, (size_t)1 << 24, (size_t)1 << 30, 4096};
}

int qli_allocation_active(void)
{
    return active_allocations != NULL;
}

QlicLiteStatus qli_encode_owned(const QlicLiteDescription *desc, const uint8_t *samples, size_t bytes,
                           size_t stride, const QlicLiteLimits *limits, const QlicLiteAllocator *allocator,
                           QlicLiteFile *file, QlicLiteStats *stats, QliEncodeBody *encode)
{
    if (stats) memset(stats, 0, sizeof(*stats));
    if (!file || file->data || file->bytes || file->release || file->user)
        return QLIC_LITE_INVALID;
    QlicLiteStatus status = check_allocator(limits, allocator);
    if (status != QLIC_LITE_OK) return status;
    if (!desc || !samples || !desc->width || !desc->height || desc->width > 32768 ||
        desc->height > 32768 || (uint64_t)desc->width * desc->height > (1u << 25))
        return QLIC_LITE_INVALID;
    if (bytes > limits->input_bytes || (uint64_t)desc->width * desc->height > limits->pixels ||
        !limits->file_bytes)
        return QLIC_LITE_LIMIT;

    QlhLimits description_limits = qli_description_limits(limits);
    int checked = qlh_validate_samples(desc, &description_limits, samples, bytes, stride);
    if (checked != QLH_OK) return checked == QLH_LIMIT ? QLIC_LITE_LIMIT : QLIC_LITE_INVALID;

    AllocationContext context;
    start_allocations(&context, limits, allocator);
    context.slot_limit = limits->allocation_requests;
    uint8_t *encoded = NULL;
    size_t written = 0;
    status = encode(desc, samples, bytes, stride, limits, &encoded, &written);
    if (status == QLIC_LITE_OK && written > limits->file_bytes) {
        ++context.stats.limit_denials;
        status = QLIC_LITE_LIMIT;
    }
    status = finish_allocations(&context, status, &encoded, written, stats);
    if (status == QLIC_LITE_OK)
        *file = (QlicLiteFile){encoded, written, context.allocator.release, context.allocator.user};
    return status;
}
QlicLiteStatus qlic_lite_decode(const uint8_t *data, size_t size,
                               const QlicLiteLimits *limits, const QlicLiteAllocator *allocator,
                               QlicLiteImage *image, QlicLiteStats *stats)
{
    if (stats) memset(stats, 0, sizeof(*stats));
    if (!limits || !image || image->samples || image->bytes || image->release || image->user)
        return QLIC_LITE_INVALID;
    QlicLiteStatus status = check_allocator(limits, allocator);
    if (status != QLIC_LITE_OK) return status;
    if (!data || size < 16) return QLIC_LITE_INVALID;
    if (size > limits->input_bytes || size > ((size_t)1 << 28)) return QLIC_LITE_LIMIT;

    QliView view;
    status = qli_inspect(data, size, limits, &view);
    if (status != QLIC_LITE_OK) return status;

    AllocationContext context;
    start_allocations(&context, limits, allocator);
    uint8_t *samples = NULL;
    size_t bytes = 0;
    QlicLiteDescription desc = {0};
    status = qli_decode_body(data, size, &view, &samples, &bytes, &desc);
    status = finish_allocations(&context, status, &samples, bytes + desc.icc_size, stats);
    if (status == QLIC_LITE_OK)
        *image = (QlicLiteImage){samples, bytes, desc, context.allocator.release, context.allocator.user};
    return status;
}

void qlic_lite_image_free(QlicLiteImage *image)
{
    if (!image) return;
    QlicLiteImage owned = *image;
    memset(image, 0, sizeof(*image));
    if (owned.samples && owned.release) owned.release(owned.samples, owned.user);
}

static inline uint32_t refill_prefix(uint32_t value,const uint8_t **cursor){
    const uint8_t *p=*cursor;
    unsigned take=(unsigned)(value<(1u<<23))+(unsigned)(value<(1u<<15));
    unsigned bits=take*8u;
    uint32_t word=((uint32_t)p[0]<<8)|p[1];
    *cursor=p+take;
    return (value<<bits)|((word<<bits)>>16);
}

static inline size_t refill_prefix_rounds(size_t rounds,size_t a,size_t b,size_t c,size_t d){
    size_t low=a<b?a:b,high=c<d?c:d;
    size_t safe=(low<high?low:high)/2;
    return safe<rounds?safe:rounds;
}

enum {
    QLIC_RANS_SCALE_BITS = 12,
    QLIC_RANS_SCALE = 1 << QLIC_RANS_SCALE_BITS
};

typedef struct {
    size_t descriptor_bytes;
    unsigned used_symbols;
    double modeled_bits;
} QlicRansModelStats;

int qlic_rans_measure_histogram(const uint64_t histogram[256],
                                QlicRansModelStats *stats_out);

int qlic_rans_encode_alloc(const uint8_t *symbols, size_t count,
                           unsigned lanes, uint8_t **frame_out,
                           size_t *size_out);

int qlic_rans_decode(const uint8_t *frame, size_t size,
                     uint8_t *symbols_out, size_t capacity,
                     size_t *count_out);

enum {
    RANS_BYTE_L = 1 << 23,
    RANS_HEADER_BYTES = 16,
    RANS_SYMBOLS = 256,
    QVP_LOW = 1u << 16,
    QVP_LANES = 16,
    QVP_CAP = 1u << 24
};

typedef struct {
    uint16_t frequency[RANS_SYMBOLS];
    uint16_t cumulative[RANS_SYMBOLS];
    uint8_t decode[QLIC_RANS_SCALE];
    unsigned used;
    double modeled_bits;
} RansModel;

static inline uint16_t qli_read_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static inline uint32_t qli_read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline uint64_t qli_read_le64(const uint8_t *p)
{
    return qli_read_le32(p) | (uint64_t)qli_read_le32(p + 4) << 32;
}

static inline void qli_write_le16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static inline void qli_write_le32(uint8_t *p, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i)
        p[i] = (uint8_t)(value >> (8 * i));
}

static inline void qli_write_le64(uint8_t *p, uint64_t value)
{
    qli_write_le32(p, (uint32_t)value);
    qli_write_le32(p + 4, (uint32_t)(value >> 32));
}

int qlr_model_build(const uint8_t *symbols, size_t count,
                       RansModel *model);
int qlr_model_normalize(const uint64_t histogram[RANS_SYMBOLS],
                        uint64_t count, RansModel *model);
size_t qlr_model_descriptor_bytes(const RansModel *model);
int qlr_model_write(const RansModel *model, uint8_t *output,
                       size_t capacity, size_t *offset);
int qlr_model_read(const uint8_t *input, size_t size, size_t *offset,
                      RansModel *model);

enum { QBS_NO_TABLE = 0, QBS_BYTE_TABLE = 256 };
static inline uint8_t packed_tag(unsigned tag)
{
    static const uint8_t pair[9]={0,1,2,4,5,6,8,9,10};
    return (uint8_t)(pair[tag%9]|(pair[tag/9]<<4));
}

_Static_assert(QLIC_RANS_SCALE_BITS==12,"This decoder table needs 4096 entries");
static inline int refill(uint32_t *v,const uint8_t **cursor,const uint8_t *end) {
    while(*v<RANS_BYTE_L) {if(*cursor==end)return 0;*v=(*v<<8)|*(*cursor)++;}return 1;
}
QLI_NOINLINE static int decode_multiple(const RansModel *model,uint32_t state[4],
    const uint8_t *cursor[4],const uint8_t *end[4],uint8_t *out,size_t count,unsigned alphabet) {
    uint32_t table[QLIC_RANS_SCALE];
    for(unsigned symbol=0;symbol<alphabet;++symbol) {
        unsigned f=model->frequency[symbol],base=model->cumulative[symbol];
        if(!f)continue;
        if(f>=QLIC_RANS_SCALE)return 0;
        unsigned value=alphabet==256 ? symbol : packed_tag(symbol);
        for(unsigned j=0;j<f;++j)table[base+j]=value|(j<<8)|(f<<20);
    }
    uint32_t a=state[0],b=state[1],c=state[2],d=state[3];size_t i=0;
    const uint8_t *p0=cursor[0],*p1=cursor[1],*p2=cursor[2],*p3=cursor[3];
    while(alphabet==256 && count-i>=4){
        size_t rounds=refill_prefix_rounds((count-i)/4,
            (size_t)(end[0]-p0),(size_t)(end[1]-p1),(size_t)(end[2]-p2),(size_t)(end[3]-p3));
        if(!rounds)break;
        size_t stop=i+4*rounds;
        for(;i<stop;i+=4){
            uint32_t e0=table[a&4095],e1=table[b&4095],e2=table[c&4095],e3=table[d&4095];
            out[i]=(uint8_t)e0;out[i+1]=(uint8_t)e1;out[i+2]=(uint8_t)e2;out[i+3]=(uint8_t)e3;
            a=(e0>>20)*(a>>12)+((e0>>8)&4095);
            b=(e1>>20)*(b>>12)+((e1>>8)&4095);
            c=(e2>>20)*(c>>12)+((e2>>8)&4095);
            d=(e3>>20)*(d>>12)+((e3>>8)&4095);
            a=refill_prefix(a,&p0);b=refill_prefix(b,&p1);
            c=refill_prefix(c,&p2);d=refill_prefix(d,&p3);
        }
    }
    for(;count-i>=4;i+=4) {
        uint32_t e0=table[a&4095],e1=table[b&4095],e2=table[c&4095],e3=table[d&4095];
        out[i]=(uint8_t)e0;out[i+1]=(uint8_t)e1;out[i+2]=(uint8_t)e2;out[i+3]=(uint8_t)e3;
        a=(e0>>20)*(a>>12)+((e0>>8)&4095);
        b=(e1>>20)*(b>>12)+((e1>>8)&4095);
        c=(e2>>20)*(c>>12)+((e2>>8)&4095);
        d=(e3>>20)*(d>>12)+((e3>>8)&4095);
        if(!refill(&a,&p0,end[0]) || !refill(&b,&p1,end[1]) || !refill(&c,&p2,end[2]) || !refill(&d,&p3,end[3]))return 0;
    }
    state[0]=a;state[1]=b;state[2]=c;state[3]=d;
    cursor[0]=p0;cursor[1]=p1;cursor[2]=p2;cursor[3]=p3;
    for(;i<count;++i) {
        unsigned lane=(unsigned)i&3;uint32_t v=state[lane],entry=table[v&4095];out[i]=(uint8_t)entry;
        v=(entry>>20)*(v>>12)+((entry>>8)&4095);
        if(!refill(&v,&cursor[lane],end[lane]))return 0;state[lane]=v;
    }
    return 1;
}
static inline int read_qbs_symbol(const RansModel *model, uint32_t *state,
                                  const uint8_t **cursor, const uint8_t *end, uint8_t *out)
{
    uint32_t value = *state;
    unsigned slot = value & (QLIC_RANS_SCALE - 1), symbol = model->decode[slot];
    value = (uint32_t)model->frequency[symbol] * (value >> QLIC_RANS_SCALE_BITS)
          + slot - model->cumulative[symbol];
    if (!refill(&value, cursor, end)) return 0;
    *state = value;
    *out = (uint8_t)symbol;
    return 1;
}

static int decode_small(const RansModel *model, uint32_t state[4],
                        const uint8_t *cursor[4], const uint8_t *end[4], uint8_t *out, size_t count)
{
    size_t i = 0;
    for (; count - i >= 4; i += 4) {
        if (!read_qbs_symbol(model, state, cursor, end[0], out+i) ||
            !read_qbs_symbol(model, state+1, cursor+1, end[1], out+i+1) ||
            !read_qbs_symbol(model, state+2, cursor+2, end[2], out+i+2) ||
            !read_qbs_symbol(model, state+3, cursor+3, end[3], out+i+3)) return 0;
    }
    for (; i < count; ++i) {
        unsigned lane = (unsigned)i & 3;
        if (!read_qbs_symbol(model, state+lane, cursor+lane, end[lane], out+i)) return 0;
    }
    return 1;
}

QLI_NOINLINE int qbs_decode(const uint8_t *frame, size_t size, uint8_t *out,
                                    size_t capacity, size_t *count_out, unsigned table_symbols)
{
    if (!count_out) return 0;
    *count_out = 0;
    if ((table_symbols != QBS_NO_TABLE && table_symbols != QBS_BYTE_TABLE &&
         table_symbols != 3 && table_symbols != 27 && table_symbols != 81) ||
        !frame || size < RANS_HEADER_BYTES+32 || memcmp(frame, "QBS1", 4) ||
        frame[4] != 4 || frame[5] != QLIC_RANS_SCALE_BITS || frame[14] || frame[15]) return 0;

    uint64_t stored = qli_read_le64(frame+6);
    if (stored > capacity || stored > (1u<<27) || (stored && !out)) return 0;
    size_t count = (size_t)stored, offset = RANS_HEADER_BYTES;
    RansModel model;
    if (!qlr_model_read(frame, size, &offset, &model) || (!!count != !!model.used) ||
        size - offset < 32) return 0;
    const int packed = table_symbols && table_symbols != QBS_BYTE_TABLE;
    if (packed)
        for (unsigned symbol = table_symbols; symbol < QBS_BYTE_TABLE; ++symbol)
            if (model.frequency[symbol]) return 0;

    const uint8_t *lanes = frame + offset;
    const uint8_t *cursor[4], *end[4];
    uint32_t state[4];
    offset += 32;
    for (unsigned lane = 0; lane < 4; ++lane) {
        uint32_t value = qli_read_le32(lanes+4*lane);
        size_t bytes = qli_read_le32(lanes+16+4*lane);
        if (value < RANS_BYTE_L || value >= (UINT32_C(1)<<31) || bytes > size - offset) return 0;
        state[lane] = value;
        cursor[lane] = frame + offset;
        offset += bytes;
        end[lane] = frame + offset;
    }
    if (offset != size) return 0;

    if (model.used > 1) {
        int ok = table_symbols ? decode_multiple(&model, state, cursor, end, out, count, table_symbols)
                               : decode_small(&model, state, cursor, end, out, count);
        if (!ok) return 0;
    }
    for (unsigned lane = 0; lane < 4; ++lane)
        if (cursor[lane] != end[lane] || state[lane] != RANS_BYTE_L) return 0;
    if (model.used == 1) {
        unsigned symbol = model.decode[0];
        memset(out, packed ? packed_tag(symbol) : symbol, count);
    }
    *count_out = count;
    return 1;
}

static int single(uint32_t *state, const uint32_t *table, const uint8_t **cursor,
                  const uint8_t *end, uint8_t *out) {
    uint32_t x=*state,e=table[x&4095u];
    x=(x>>12)*(e>>20)+((e>>8)&4095u);
    if (x<QVP_LOW) {
        if ((size_t)(end-*cursor)<2) return 0;
        x=(x<<16)|qli_read_le16(*cursor);
        *cursor+=2;
    }
    *state=x;
    *out=(uint8_t)e;
    return 1;
}

int qvp_read(const uint8_t *file,size_t bytes,uint8_t *out,
                  size_t count,int portable) {
    if (!file || !out || !count || count>QVP_CAP || bytes<RANS_HEADER_BYTES+64 ||
        memcmp(file,"QVP1",4) || file[4]!=QVP_LANES ||
        file[5]!=12 || file[14] || file[15] || qli_read_le64(file+6)!=count)
        return 0;
    size_t at=RANS_HEADER_BYTES;
    RansModel model;
    if (!qlr_model_read(file,bytes,&at,&model) || model.used<2 ||
        bytes-at<64 || (bytes-at-64)%2)
        return 0;
    uint32_t table[4096],state[QVP_LANES];
    for (unsigned s=0;s<256;++s) {
        unsigned f=model.frequency[s],b=model.cumulative[s];
        for (unsigned i=0;i<f;++i) table[b+i]=s|(i<<8)|(f<<20);
    }
    for (unsigned lane=0;lane<QVP_LANES;++lane) {
        state[lane]=qli_read_le32(file+at+4*lane);
        if (state[lane]<QVP_LOW) return 0;
    }
    const uint8_t *cursor=file+at+64,*end=file+bytes;
    (void)portable;
    size_t i=0;
    for (;count-i>=4 && (size_t)(end-cursor)>=8;i+=4) {
        uint32_t *group=state+(i&15);
        uint32_t next[4];
        for (unsigned lane=0;lane<4;++lane) {
            uint32_t s=group[lane],entry=table[s&4095u];
            next[lane]=(s>>12)*(entry>>20)+((entry>>8)&4095u);
            out[i+lane]=(uint8_t)entry;
        }
        unsigned offset=0;
        for (unsigned lane=0;lane<4;++lane) {
            uint32_t s=next[lane],filled=(s<<16)|qli_read_le16(cursor+offset);
            unsigned take=s<QVP_LOW;
            group[lane]=take ? filled : s;
            offset+=2u*take;
        }
        cursor+=offset;
    }
    for (;i<count;++i)
        if (!single(&state[i&15],table,&cursor,end,out+i)) return 0;
    for (unsigned lane=0;lane<QVP_LANES;++lane)
        if (state[lane]!=QVP_LOW) return 0;
    return cursor==end;
}

static int shape(const QlicLiteDescription *d, const QlhLimits *l) {
    if (!d || !l) return QLH_ARGUMENT;
    if (!d->width || !d->height || d->width > (1u << 20) ||
        d->height > (1u << 20) || d->bits < 8 || d->bits > 10 ||
        d->channels < 1 || d->channels > 4 || d->alpha > 2)
        return QLH_DESCRIPTION;
    if ((d->channels == 2 || d->channels == 4) ? d->alpha == 0 : d->alpha != 0)
        return QLH_DESCRIPTION;
    if ((uint64_t)d->width * d->height > l->max_pixels) return QLH_LIMIT;
    return QLH_OK;
}
int qlh_record_size(const QlicLiteDescription *d, const QlhLimits *l, size_t *size) {
    if (!size) return QLH_ARGUMENT;
    *size = 0;
    int status = shape(d, l);
    if (status) return status;
    if (d->authority > 4 || d->has_cicp > 1 || d->has_mastering > 1 ||
        d->has_light > 1 || d->icc_size > UINT32_MAX ||
        (d->icc_size && !d->icc)) return QLH_DESCRIPTION;
    unsigned icc = d->icc_size != 0, cicp = d->has_cicp;
    if ((d->authority == 0 && (icc || cicp)) ||
        (d->authority == 1 && (!icc || cicp)) ||
        (d->authority == 2 && (icc || !cicp)) ||
        (d->authority >= 3 && (!icc || !cicp))) return QLH_DESCRIPTION;
    if ((cicp && (d->cicp.full_range > 1 || d->cicp.reserved)) ||
        (d->has_mastering && d->mastering.max_luminance < d->mastering.min_luminance) ||
        (d->has_light && d->max_cll < d->max_fall)) return QLH_DESCRIPTION;
    uint64_t metadata = (uint64_t)d->icc_size + 8u * cicp +
                        24u * d->has_mastering + 8u * d->has_light;
    uint64_t record = metadata + 32u;
    if (metadata > UINT32_MAX || metadata > l->max_metadata ||
        record > l->max_record || record > SIZE_MAX) return QLH_LIMIT;
    *size = (size_t)record;
    return QLH_OK;
}
static int overlaps(const uint8_t *a, size_t an, const uint8_t *b, size_t bn) {
    if (!an || !bn) return 0;
    uintptr_t ap = (uintptr_t)a, bp = (uintptr_t)b;
    return ap <= bp ? bp - ap < an : ap - bp < bn;
}
int qlh_write(const QlicLiteDescription *d, const QlhLimits *l, uint8_t *out,
              size_t capacity, size_t *written) {
    if (!written) return QLH_ARGUMENT;
    *written = 0;
    size_t n;
    int status = qlh_record_size(d, l, &n);
    if (status) return status;
    if (!out) return QLH_ARGUMENT;
    if (capacity < n) return QLH_CAPACITY;
    if (overlaps(out, n, d->icc, d->icc_size) ||
        overlaps(out, n, (const uint8_t *)d, sizeof(*d))) return QLH_OVERLAP;
    memcpy(out, "QLH1", 4);
    out[4] = out[5] = 1;
    out[6] = (uint8_t)d->bits; out[7] = (uint8_t)d->channels;
    out[8] = (uint8_t)d->alpha; out[9] = (uint8_t)d->authority;
    out[10] = (uint8_t)(d->has_cicp | (d->has_mastering << 1) | (d->has_light << 2));
    out[11] = 0;
    qli_write_le32(out + 12, d->width); qli_write_le32(out + 16, d->height);
    qli_write_le32(out + 20, (uint32_t)d->icc_size); qli_write_le32(out + 24, (uint32_t)(n - 32));
    uint8_t *p = out + 28;
    if (d->has_cicp) {
        qli_write_le16(p, d->cicp.primaries); qli_write_le16(p + 2, d->cicp.transfer);
        qli_write_le16(p + 4, d->cicp.matrix); p[6] = d->cicp.full_range; p[7] = 0;
        p += 8;
    }
    if (d->has_mastering) {
        for (unsigned i = 0; i < 3; ++i) {
            qli_write_le16(p + 4 * i, d->mastering.x[i]);
            qli_write_le16(p + 4 * i + 2, d->mastering.y[i]);
        }
        qli_write_le16(p + 12, d->mastering.white_x); qli_write_le16(p + 14, d->mastering.white_y);
        qli_write_le32(p + 16, d->mastering.max_luminance);
        qli_write_le32(p + 20, d->mastering.min_luminance); p += 24;
    }
    if (d->has_light) {
        qli_write_le32(p, d->max_cll); qli_write_le32(p + 4, d->max_fall); p += 8;
    }
    if (d->icc_size) memcpy(p, d->icc, d->icc_size);
    qli_write_le32(out + n - 4, qint_crc32(out, n - 4));
    *written = n;
    return QLH_OK;
}
int qlh_parse(const uint8_t *data, size_t n, const QlhLimits *l,
              QlicLiteDescription *out) {
    if (!out) return QLH_ARGUMENT;
    memset(out, 0, sizeof(*out));
    if (!data || !l) return QLH_ARGUMENT;
    if (n < 32) return QLH_LENGTH;
    if ((uint64_t)n > l->max_record) return QLH_LIMIT;
    if (memcmp(data, "QLH1", 4) || data[4] != 1 || data[5] != 1 ||
        (data[10] & ~7u) || data[11]) return QLH_DESCRIPTION;
    QlicLiteDescription d = {0};
    d.bits = data[6]; d.channels = data[7]; d.alpha = data[8]; d.authority = data[9];
    d.width = qli_read_le32(data + 12); d.height = qli_read_le32(data + 16);
    int status = shape(&d, l);
    if (status) return status;
    d.has_cicp = data[10] & 1u;
    d.has_mastering = (data[10] >> 1) & 1u;
    d.has_light = (data[10] >> 2) & 1u;
    d.icc_size = qli_read_le32(data + 20);
    uint64_t metadata = (uint64_t)d.icc_size + 8u * d.has_cicp +
                        24u * d.has_mastering + 8u * d.has_light;
    if (metadata > l->max_metadata || metadata > UINT32_MAX) return QLH_LIMIT;
    if (metadata != qli_read_le32(data + 24) || metadata + 32 != (uint64_t)n)
        return QLH_LENGTH;
    if (qint_crc32(data, n - 4) != qli_read_le32(data + n - 4)) return QLH_CHECKSUM;
    const uint8_t *p = data + 28;
    if (d.has_cicp) {
        d.cicp.primaries = qli_read_le16(p); d.cicp.transfer = qli_read_le16(p + 2);
        d.cicp.matrix = qli_read_le16(p + 4); d.cicp.full_range = p[6]; d.cicp.reserved = p[7];
        p += 8;
    }
    if (d.has_mastering) {
        for (unsigned i = 0; i < 3; ++i) {
            d.mastering.x[i] = qli_read_le16(p + 4 * i); d.mastering.y[i] = qli_read_le16(p + 4 * i + 2);
        }
        d.mastering.white_x = qli_read_le16(p + 12); d.mastering.white_y = qli_read_le16(p + 14);
        d.mastering.max_luminance = qli_read_le32(p + 16);
        d.mastering.min_luminance = qli_read_le32(p + 20); p += 24;
    }
    if (d.has_light) { d.max_cll = qli_read_le32(p); d.max_fall = qli_read_le32(p + 4); p += 8; }
    d.icc = d.icc_size ? p : NULL;
    size_t expected;
    status = qlh_record_size(&d, l, &expected);
    if (status) return status;
    if (expected != n) return QLH_LENGTH;
    *out = d;
    return QLH_OK;
}
int qlh_validate_samples(const QlicLiteDescription *d, const QlhLimits *l,
                         const uint8_t *samples, size_t size, size_t stride) {
    size_t record;
    int status = qlh_record_size(d, l, &record);
    if (status) return status;
    if (!samples) return QLH_ARGUMENT;
    uint64_t bytes_per_sample = d->bits == 8 ? 1u : 2u;
    uint64_t row = (uint64_t)d->width * d->channels * bytes_per_sample;
    uint64_t packed = row * d->height;
    if (packed > l->max_sample_bytes || packed > SIZE_MAX) return QLH_LIMIT;
    if (row > stride) return QLH_LENGTH;
    if ((uint64_t)(d->height - 1u) &&
        (uint64_t)stride > (UINT64_MAX - row) / (d->height - 1u)) return QLH_LENGTH;
    uint64_t extent = (uint64_t)stride * (d->height - 1u) + row;
    if (extent > l->max_sample_bytes || extent > SIZE_MAX) return QLH_LIMIT;
    if (extent > size) return QLH_LENGTH;
    if (bytes_per_sample == 2) {
        unsigned max_sample = (1u << d->bits) - 1u;
        for (uint32_t y = 0; y < d->height; ++y) {
            const uint8_t *p = samples + (size_t)y * stride;
            for (uint64_t x = 0; x < row; x += 2)
                if (qli_read_le16(p + (size_t)x) > max_sample) return QLH_SAMPLE;
        }
    }
    return QLH_OK;
}

enum { FAST_RAW = 0, FAST_LEFT = 1, FAST_ROW = 2, FAST_PALETTE = 3,
       FAST_SOLID = 4, FAST_AUTO = 255, FAST_HEADER = 32, FAST_FOOTER = 4 };
int fast_encode_alloc(const uint8_t *rgba, uint32_t w, uint32_t h,
                      unsigned mode, uint8_t **file, size_t *size);
int fast_decode_alloc(const uint8_t *file, size_t size,
                              size_t max_rgba_bytes, uint8_t **rgba,
                              uint32_t *width, uint32_t *height);
static inline int fast_shape(uint32_t w, uint32_t h, size_t *pixels) {
    if (!w || !h || w > (1u << 20) || h > (1u << 20) || (uint64_t)w * h > (1u << 25)) return 0;
    *pixels = (size_t)w * h; return 1;
}
static inline unsigned qli_rgba_channels(const uint8_t *rgba, size_t pixels) {
    int opaque = 1, gray = 1;
    for (size_t i = 0; i < pixels; ++i) {
        const uint8_t *p = rgba + 4 * i;
        opaque &= p[3] == 255;
        gray &= p[0] == p[1] && p[1] == p[2];
    }
    return opaque ? (gray ? 1u : 3u) : 4u;
}
static inline uint8_t fast_predict(unsigned filter, int left, int up, int corner) {
    if (filter == 0) return 0;
    if (filter == 1) return (uint8_t)left;
    if (filter == 2) return (uint8_t)up;
    int p = left + up - corner, a = p - left, b = p - up, c = p - corner;
    a = a < 0 ? -a : a; b = b < 0 ? -b : b; c = c < 0 ? -c : c;
    return (uint8_t)(a <= b && a <= c ? left : b <= c ? up : corner);
}

#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42

#define QI_K64 UINT64_C(0xccaa009e)
#define QI_K128 UINT64_C(0xae689191)
#define QI_K192 UINT64_C(0x81256527)
#define QI_K256 UINT64_C(0xf1da05aa)
#define QI_K320 UINT64_C(0xaf449247)
#define QI_K384 UINT64_C(0x3db1ecdc)
#define QI_K448 UINT64_C(0x1d9513d7)
#define QI_K512 UINT64_C(0x8f352d95)
#define QI_SEED UINT32_C(0x9db42487)
#define QI_POLY UINT64_C(0x1db710641)

static inline uint32_t qli_crc_reduce(uint64_t low, uint64_t high)
{
    for (unsigned bit = 32; bit-- > 0;) {
        unsigned shift = 32 + bit;
        uint64_t take = 0 - ((high >> bit) & 1);
        low ^= (QI_POLY << shift) & take;
        high ^= (QI_POLY >> (64 - shift)) & take;
    }
    for (unsigned bit = 32; bit-- > 0;) {
        uint64_t take = 0 - ((low >> (32 + bit)) & 1);
        low ^= (QI_POLY << bit) & take;
    }
    return (uint32_t)low;
}

#if defined(__GNUC__) || defined(__clang__)
#define QLI_CRC_TARGET __attribute__((target("pclmul,sse2")))
#else
#define QLI_CRC_TARGET QLI_NOINLINE
#endif

static inline __m128i crc_native_load(const uint8_t *source)
{
    __m128i value;
    memcpy(&value, source, sizeof(value));
    return value;
}

static inline QLI_CRC_TARGET __m128i crc_native_fold(__m128i value, __m128i factors)
{
    return _mm_xor_si128(_mm_clmulepi64_si128(value, factors, 0x00),
                         _mm_clmulepi64_si128(value, factors, 0x11));
}

static QLI_CRC_TARGET uint32_t crc_native_finish(__m128i a,__m128i b,__m128i c,__m128i d,
                                                   const uint8_t *data,size_t size)
{
    __m128i combined = _mm_xor_si128(
        _mm_xor_si128(crc_native_fold(a, _mm_set_epi64x(QI_K320, QI_K384)),
                       crc_native_fold(b, _mm_set_epi64x(QI_K192, QI_K256))),
        _mm_xor_si128(crc_native_fold(c, _mm_set_epi64x(QI_K64, QI_K128)), d));
    __m128i folded = crc_native_fold(combined, _mm_set_epi64x(QI_K64, QI_K128));
    uint32_t value = qli_crc_reduce((uint64_t)_mm_cvtsi128_si64(folded),
                                     (uint64_t)_mm_cvtsi128_si64(_mm_srli_si128(folded, 8)));
    while (size--) {
        value ^= *data++;
        for (unsigned bit = 0; bit < 8; ++bit)
            value = (value >> 1) ^ (UINT32_C(0xedb88320) & (0u - (value & 1)));
    }
    return value ^ UINT32_MAX;
}

static QLI_CRC_TARGET uint32_t crc_native(const uint8_t *data, size_t size)
{
    __m128i a = _mm_set_epi64x(0, QI_SEED), b = _mm_setzero_si128(), c = b, d = b;
    const __m128i step = _mm_set_epi64x(QI_K448, QI_K512);
    while (size >= 64) {
        a = _mm_xor_si128(crc_native_fold(a, step), crc_native_load(data));
        b = _mm_xor_si128(crc_native_fold(b, step), crc_native_load(data + 16));
        c = _mm_xor_si128(crc_native_fold(c, step), crc_native_load(data + 32));
        d = _mm_xor_si128(crc_native_fold(d, step), crc_native_load(data + 48));
        data += 64;
        size -= 64;
    }
    return crc_native_finish(a,b,c,d,data,size);
}

#undef QLI_CRC_TARGET
#endif

static uint32_t crc_tab[8][256];
static uint32_t crc_byte_powers[sizeof(size_t) * 8];

static uint32_t crc_polynomial_product(uint32_t a, uint32_t b)
{
    uint32_t product = 0;
    for (uint32_t bit = UINT32_C(1) << 31; bit; bit >>= 1) {
        if (a & bit) product ^= b;
        b = (b >> 1) ^ (UINT32_C(0xedb88320) & (0u - (b & 1)));
    }
    return product;
}

static void crc_init_table(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++) c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crc_tab[0][i] = c;
    }
    for (int t = 1; t < 8; t++) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = crc_tab[t - 1][i];
            crc_tab[t][i] = crc_tab[0][c & 0xffu] ^ (c >> 8);
        }
    }
    crc_byte_powers[0] = UINT32_C(1) << 23;
    for (unsigned i = 1; i < sizeof(crc_byte_powers) / sizeof(*crc_byte_powers); ++i)
        crc_byte_powers[i] = crc_polynomial_product(crc_byte_powers[i-1], crc_byte_powers[i-1]);
}

#ifdef _WIN32
static INIT_ONCE crc_once = INIT_ONCE_STATIC_INIT;
static BOOL CALLBACK crc_init_once(PINIT_ONCE once, PVOID param, PVOID *ctx) {
    (void)once;
    (void)param;
    (void)ctx;
    crc_init_table();
    return TRUE;
}
static void crc_ensure(void) {
    InitOnceExecuteOnce(&crc_once, crc_init_once, NULL, NULL);
}
#elif !defined(QLIC_WASM)
static pthread_once_t crc_once = PTHREAD_ONCE_INIT;
static void crc_ensure(void) {
    (void)pthread_once(&crc_once, crc_init_table);
}
#else
static int crc_ready;
static void crc_ensure(void) {
    if (!crc_ready) {
        crc_init_table();
        crc_ready = 1;
    }
}
#endif

static QLI_INLINE uint32_t crc_word(uint32_t c, uint64_t v) {
    c ^= (uint32_t)v;
    uint32_t hi = (uint32_t)(v >> 32);
    return crc_tab[7][c & 0xffu] ^
           crc_tab[6][(c >> 8) & 0xffu] ^
           crc_tab[5][(c >> 16) & 0xffu] ^
           crc_tab[4][c >> 24] ^
           crc_tab[3][hi & 0xffu] ^
           crc_tab[2][(hi >> 8) & 0xffu] ^
           crc_tab[1][(hi >> 16) & 0xffu] ^
           crc_tab[0][hi >> 24];
}
static inline uint32_t crc_byte_factor(size_t bytes)
{
    uint32_t factor = UINT32_C(1) << 31;
    for (unsigned bit = 0; bytes; bytes >>= 1, ++bit)
        if (bytes & 1) factor = crc_polynomial_product(factor, crc_byte_powers[bit]);
    return factor;
}

static uint32_t crc_advance(uint32_t crc, size_t bytes)
{
    crc_ensure();
    return crc_polynomial_product(crc_byte_factor(bytes), crc);
}

static uint32_t crc_step(uint32_t c, const uint8_t *p, size_t n) {
    if (n >= 8192) {
        size_t span = (n / 32) * 8;
        const uint8_t *a = p, *b = p + span, *d = p + 2*span, *e = p + 3*span;
        uint32_t x = c, y = 0, z = 0, w = 0;
        for (size_t i = 0; i < span; i += 8) {
            x = crc_word(x, qli_read_le64(a + i));
            y = crc_word(y, qli_read_le64(b + i));
            z = crc_word(z, qli_read_le64(d + i));
            w = crc_word(w, qli_read_le64(e + i));
        }
        uint32_t jump = crc_byte_factor(span);
        c = crc_polynomial_product(jump, x) ^ y;
        c = crc_polynomial_product(jump, c) ^ z;
        c = crc_polynomial_product(jump, c) ^ w;
        p += 4*span;
        n -= 4*span;
    }
    while (n >= 8) {
        uint64_t v;
        memcpy(&v, p, sizeof(v));
        c = crc_word(c, v);
        p += 8;
        n -= 8;
    }
    for (size_t i = 0; i < n; i++) {
        c = crc_tab[0][(c ^ p[i]) & 0xffu] ^ (c >> 8);
    }
    return c;
}
uint32_t stream_crc32(const uint8_t *p, size_t n) {
    if (!p || !n) return 0;
    crc_ensure();
    return crc_step(UINT32_MAX, p, n) ^ UINT32_MAX;
}

uint32_t qint_crc32(const uint8_t *data, size_t size)
{
#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42
    if (data && size >= 256 && (qli_cpu_capabilities() & QLI_CPU_PCLMUL_CRC))
        return crc_native(data, size);
#endif
    return stream_crc32(data, size);
}

static uint32_t qli_wrapped_crc32(const uint8_t *data, size_t prefix, size_t packed)
{
    if (packed < 65536) return qint_crc32(data, prefix + packed);
    return crc_advance(qint_crc32(data, prefix), packed) ^ UINT32_C(0x2144df1c);
}

enum {
    ICON_SIDE = 64,
    ICON_PIXELS = ICON_SIDE * ICON_SIDE,
    ICON_HEADER = 11,
    ICON_RECORD = 5,
    ICON_MIN_FILE = ICON_HEADER + 4 * ICON_RECORD + 4,
    ICON_FILE_CAP = 32768
};

typedef struct {
    uint8_t events[ICON_PIXELS], alpha[ICON_PIXELS], green[ICON_PIXELS], sides[2 * ICON_PIXELS];
} IconStreams;

static inline int icon_clamp(int value, int low, int high)
{
    return value < low ? low : value > high ? high : value;
}

static inline int icon_gradient(int west, int north, int corner)
{
    int low = west < north ? west : north;
    int high = west < north ? north : west;
    return icon_clamp(west + north - corner, low, high);
}

static inline int icon_alpha_class(const uint8_t *pixel)
{
    return pixel ? (pixel[3] != 0) + (pixel[3] == 255) : 0;
}

static inline int icon_alpha_value(const uint8_t *pixel)
{
    return pixel ? pixel[3] : 0;
}

static inline const uint8_t *icon_visible(const uint8_t *pixel, int common)
{
    return pixel && common && pixel[3] == 0 ? NULL : pixel;
}

static inline int icon_color(unsigned channel, const uint8_t *west, const uint8_t *north,
                              const uint8_t *corner)
{
    if (west && north)
        return corner ? icon_gradient(west[channel], north[channel], corner[channel])
                      : (west[channel] + north[channel]) / 2;
    if (west) return west[channel];
    if (north) return north[channel];
    return corner ? corner[channel] : 0;
}

static inline const uint8_t *icon_hidden_color(const uint8_t *rgba, size_t bytes)
{
    const uint8_t *hidden = NULL;
    for (size_t i = 0; i < bytes; i += 4) {
        if (rgba[i + 3]) continue;
        if (!hidden) hidden = rgba + i;
        else if (memcmp(hidden, rgba + i, 3)) return NULL;
    }
    return hidden;
}

typedef int QliNumericalDecode(const uint8_t *, size_t, size_t, uint8_t **, size_t *, size_t *);

QliNumericalDecode spatial_banks_decode;

enum { QAF_HEADER = 12, QAF_RECORD = 14, QAF_CRC = 4, QAF_PIXEL_CAP = 65536 };

static uint32_t qaf_word(const uint8_t *p)
{
    uint32_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

static uint32_t qaf_value(const uint8_t *p, unsigned channels)
{
    if (channels == 4) return qaf_word(p);
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

static uint32_t qaf_add(uint32_t a, uint32_t b)
{
    const uint32_t low = 0x7f7f7f7fu;
    return ((a & low) + (b & low)) ^ ((a ^ b) & ~low);
}

enum { QAF_MAX_FILE = QAF_HEADER + 8 * QAF_RECORD + QAF_CRC };

static uint32_t qaf_sub(uint32_t a, uint32_t b)
{
    return qaf_add(qaf_add(a, ~b), 0x01010101u);
}

static uint32_t qaf_slope(const uint8_t *row, unsigned width, unsigned channels)
{
    return width > 1 ? qaf_sub(qaf_value(row+channels,channels), qaf_value(row,channels)) : 0;
}

static size_t qaf_pack(const uint8_t *raw, unsigned w, unsigned h,
                       uint8_t out[QAF_MAX_FILE], size_t limit, unsigned channels)
{
    if (limit < QAF_HEADER + QAF_RECORD + QAF_CRC) return 0;
    const size_t stride = (size_t)w * channels;
    const unsigned record_size = 2 + 3*channels;
    size_t at = QAF_HEADER;
    for (unsigned y = 0; y < h;) {
        size_t price = QAF_HEADER + (at-QAF_HEADER)/record_size*QAF_RECORD;
        if (price + QAF_RECORD + QAF_CRC > limit) return 0;
        const uint8_t *row = raw + (size_t)y * stride;
        const uint32_t origin = qaf_value(row,channels), dx = qaf_slope(row,w,channels);
        uint32_t dy = 0;
        if (y + 1 < h && qaf_slope(row+stride,w,channels) == dx)
            dy = qaf_sub(qaf_value(row+stride,channels), origin);
        unsigned rows = 0;
        for (uint32_t left = origin; y + rows < h; left = qaf_add(left, dy), ++rows) {
            const uint8_t *p = row + (size_t)rows * stride;
            if (qaf_value(p,channels) != left || qaf_slope(p,w,channels) != dx) break;
            uint32_t value = qaf_add(left, dx);
            for (unsigned x = 2; x < w; ++x) {
                value = qaf_add(value, dx);
                if (qaf_value(p+(size_t)x*channels,channels) != value) return 0;
            }
        }
        qli_write_le16(out + at, rows);
        memcpy(out+at+2,&origin,channels);
        memcpy(out+at+2+channels,&dx,channels);
        memcpy(out+at+2+2*channels,&dy,channels);
        at += record_size;
        y += rows;
    }
    memcpy(out, channels==3 ? "QAF3" : "QAF1", 4);
    qli_write_le16(out + 4, w);
    qli_write_le16(out + 6, h);
    uint32_t crc = qint_crc32(raw, stride * h);
    memcpy(out + 8, &crc, 4);
    crc = qint_crc32(out, at);
    memcpy(out + at, &crc, 4);
    return at + QAF_CRC;
}

static int qaf_inspect(const uint8_t *f, size_t n, unsigned *w, unsigned *h, unsigned channels)
{
    const unsigned record_size=2+3*channels;
    if (n < QAF_HEADER + record_size + QAF_CRC ||
        (n - QAF_HEADER - QAF_CRC) % record_size) return 0;
    *w = qli_read_le16(f + 4);
    *h = qli_read_le16(f + 6);
    if (!*w || !*h || (size_t)*w * *h > QAF_PIXEL_CAP ||
        n > QAF_HEADER + (size_t)*h * record_size + QAF_CRC ||
        qint_crc32(f, n - QAF_CRC) != qaf_word(f + n - QAF_CRC)) return 0;
    unsigned left = *h;
    for (size_t at = QAF_HEADER; at < n - QAF_CRC; at += record_size) {
        const uint8_t *p = f + at;
        unsigned rows = qli_read_le16(p);
        if (!rows || rows > left || (*w == 1 && qaf_value(p+2+channels,channels)) ||
            (rows == 1 && qaf_value(p+2+2*channels,channels))) return 0;
        left -= rows;
    }
    return left == 0;
}

static void qaf_rows(uint8_t *row, unsigned w, unsigned rows, uint32_t origin, uint32_t dx,
                     uint32_t dy, int native)
{
    const size_t stride = (size_t)w * 4;
#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42 && (defined(_M_X64) || defined(__x86_64__))
    if (native) {
        uint32_t x2 = qaf_add(dx, dx), x3 = qaf_add(x2, dx);
        const uint32_t first[4] = {origin, qaf_add(origin, dx), qaf_add(origin, x2), qaf_add(origin, x3)};
        __m128i start;
        memcpy(&start, first, sizeof(start));
        const __m128i step = _mm_set1_epi32((int)qaf_add(x2, x2)), down = _mm_set1_epi32((int)dy);
        for (unsigned y = 0; y < rows; ++y) {
            __m128i value = start;
            size_t x = 0;
            for (; x + 16 <= stride; x += 16) {
                memcpy(row + x, &value, sizeof(value));
                value = _mm_add_epi8(value, step);
            }
            if (x < stride) memcpy(row + x, &value, stride - x);
            row += stride;
            start = _mm_add_epi8(start, down);
        }
        return;
    }
#else
    (void)native;
#endif
    uint32_t step = qaf_add(dx, dx);
    for (unsigned y = 0; y < rows; ++y) {
        uint32_t pixels[2] = {origin, qaf_add(origin, dx)};
        size_t x = 0;
        for (; x + sizeof(pixels) <= stride; x += sizeof(pixels)) {
            memcpy(row + x, pixels, sizeof(pixels));
            pixels[0] = qaf_add(pixels[0], step);
            pixels[1] = qaf_add(pixels[1], step);
        }
        if (x < stride) memcpy(row + x, pixels, sizeof(pixels[0]));
        row += stride;
        origin = qaf_add(origin, dy);
    }
}

static int qaf_pixels(const uint8_t *f, size_t n, unsigned w, uint8_t *out, size_t bytes,
                      unsigned channels)
{
    uint8_t *row = out;
    int native = 0;
#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42 && (defined(_M_X64) || defined(__x86_64__))
    native = (qli_cpu_capabilities() & QLI_CPU_SSE42) != 0;
#endif
    for (size_t at = QAF_HEADER; at < n - QAF_CRC; at += 2+3*channels) {
        const uint8_t *p = f + at;
        unsigned rows = qli_read_le16(p);
        uint32_t origin=qaf_value(p+2,channels),dx=qaf_value(p+2+channels,channels);
        uint32_t dy=qaf_value(p+2+2*channels,channels);
        if (channels==3) {
            for (unsigned y=0;y<rows;++y) {
                uint32_t value=origin;
                for (unsigned x=0;x<w;++x) {
                    memcpy(row,&value,3);
                    row+=3;
                    value=qaf_add(value,dx);
                }
                origin=qaf_add(origin,dy);
            }
            continue;
        }
        qaf_rows(row, w, rows, origin, dx, dy, native);
        row += (size_t)w * channels * rows;
    }
    return qint_crc32(out, bytes) == qaf_word(f + 8);
}

typedef struct { uint32_t row[4][4]; } Tile;

static int transpose_native(void)
{
#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42 && (defined(_M_X64) || defined(__x86_64__))
    return (qli_cpu_capabilities() & QLI_CPU_SSE42) != 0;
#else
    return 0;
#endif
}

static inline Tile transposed_tile(const uint8_t *p, size_t stride, int native)
{
    Tile tile;
#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42 && (defined(_M_X64) || defined(__x86_64__))
    if (native) {
        __m128i a, b, c, d;
        memcpy(&a, p, sizeof(a));
        memcpy(&b, p + stride, sizeof(b));
        memcpy(&c, p + 2 * stride, sizeof(c));
        memcpy(&d, p + 3 * stride, sizeof(d));
        const __m128i ab0 = _mm_unpacklo_epi32(a, b), ab1 = _mm_unpackhi_epi32(a, b);
        const __m128i cd0 = _mm_unpacklo_epi32(c, d), cd1 = _mm_unpackhi_epi32(c, d);
        const __m128i rows[4] = {_mm_unpacklo_epi64(ab0, cd0), _mm_unpackhi_epi64(ab0, cd0),
                                 _mm_unpacklo_epi64(ab1, cd1), _mm_unpackhi_epi64(ab1, cd1)};
        memcpy(&tile, rows, sizeof(tile));
        return tile;
    }
#else
    (void)native;
#endif
    for (unsigned y = 0; y < 4; ++y)
        for (unsigned x = 0; x < 4; ++x)
            memcpy(&tile.row[x][y], p + y * stride + x * 4, 4);
    return tile;
}

static inline void store_tile(uint8_t *p, size_t stride, Tile tile)
{
    for (unsigned i = 0; i < 4; ++i)
        memcpy(p + i * stride, tile.row[i], sizeof(tile.row[i]));
}

static void transpose_copy(uint8_t *dst, const uint8_t *src, unsigned w, unsigned h)
{
    const size_t input = 4 * (size_t)w, output = 4 * (size_t)h;
    const unsigned full_w = w & ~3u, full_h = h & ~3u;
    const int native = transpose_native();
    for (unsigned y = 0; y < full_h; y += 4) {
        for (unsigned x = 0; x < full_w; x += 4)
            store_tile(dst + x * output + 4 * y, output,
                       transposed_tile(src + y * input + 4 * x, input, native));
        for (unsigned x = full_w; x < w; ++x)
            for (unsigned j = 0; j < 4; ++j)
                memcpy(dst + x * output + 4 * (y+j), src + (y+j) * input + 4 * x, 4);
    }
    for (unsigned y = full_h; y < h; ++y)
        for (unsigned x = 0; x < w; ++x)
            memcpy(dst + x * output + 4 * y, src + y * input + 4 * x, 4);
}

static void transpose_rgb(uint8_t *dst,const uint8_t *src,unsigned w,unsigned h)
{
    for (unsigned y=0;y<h;++y)
        for (unsigned x=0;x<w;++x)
            memcpy(dst+3*((size_t)x*h+y),src+3*((size_t)y*w+x),3);
}

static void transpose_square(uint8_t *samples, unsigned n)
{
    const size_t stride = 4 * (size_t)n;
    const unsigned full = n & ~3u;
    const int native = transpose_native();
    for (unsigned y = 0; y < full; y += 4) {
        uint8_t *diagonal = samples + y * stride + 4 * y;
        store_tile(diagonal, stride, transposed_tile(diagonal, stride, native));
        for (unsigned x = y + 4; x < full; x += 4) {
            uint8_t *a = samples + y * stride + 4 * x;
            uint8_t *b = samples + x * stride + 4 * y;
            Tile first = transposed_tile(a, stride, native), second = transposed_tile(b, stride, native);
            store_tile(a, stride, second);
            store_tile(b, stride, first);
        }
    }
    for (unsigned y = 0; y < n; ++y) {
        unsigned x = y < full ? full : y + 1;
        for (; x < n; ++x) {
            uint8_t temp[4];
            uint8_t *a = samples + y * stride + 4 * x;
            uint8_t *b = samples + x * stride + 4 * y;
            memcpy(temp, a, 4);
            memcpy(a, b, 4);
            memcpy(b, temp, 4);
        }
    }
}

int strip_decode(const uint8_t *,size_t,size_t,uint8_t **,uint32_t *,uint32_t *);
int strip_decode_limited(const uint8_t *,size_t,size_t,size_t,uint8_t **,uint32_t *,uint32_t *,size_t *);

int qep_tile_decode(const uint8_t *,size_t,size_t,uint8_t **,uint32_t *,uint32_t *);

int qps_encode_alloc(const uint8_t *, uint32_t, uint32_t, uint8_t **, size_t *);
int qps_decode_alloc(const uint8_t *, size_t, size_t, uint8_t **, uint32_t *, uint32_t *);

#define QLI_SIGNATURE(a, b, c, d) \
    ((uint32_t)(a) | (uint32_t)(b) << 8 | (uint32_t)(c) << 16 | (uint32_t)(d) << 24)

static QliFormat container_format(const uint8_t *data)
{
    switch (qli_read_le32(data)) {
    case QLI_SIGNATURE('Q','P','F','1'): return QLI_FORMAT_FAST;
    case QLI_SIGNATURE('Q','P','S','1'): return QLI_FORMAT_SIMPLE;
    case QLI_SIGNATURE('Q','E','P','1'): return QLI_FORMAT_TILE;
    case QLI_SIGNATURE('Q','W','I','1'):
    case QLI_SIGNATURE('Q','W','T','1'):
    case QLI_SIGNATURE('Q','W','N','L'): return QLI_FORMAT_STRIP;
    case QLI_SIGNATURE('Q','A','X','1'):
    case QLI_SIGNATURE('Q','F','L','1'): return QLI_FORMAT_NUMERICAL;
    case QLI_SIGNATURE('Q','P','K','3'):
    case QLI_SIGNATURE('Q','P','K','7'): return QLI_FORMAT_PALETTE;
    case QLI_SIGNATURE('Q','R','V','3'):
    case QLI_SIGNATURE('Q','R','V','P'):
    case QLI_SIGNATURE('Q','R','H','3'):
    case QLI_SIGNATURE('Q','R','H','P'):
    case QLI_SIGNATURE('Q','C','R','H'):
    case QLI_SIGNATURE('Q','C','H','R'): return QLI_FORMAT_HISTORY;
    case QLI_SIGNATURE('Q','C','T','H'): return QLI_FORMAT_TRANSPOSED_HISTORY;
    case QLI_SIGNATURE('Q','A','F','3'):
    case QLI_SIGNATURE('Q','A','F','1'): return QLI_FORMAT_AFFINE;
    case QLI_SIGNATURE('Q','I','E','1'): return QLI_FORMAT_ICON;
    case QLI_SIGNATURE('Q','H','I','1'): return QLI_FORMAT_SAMPLES;
    default: return QLI_FORMAT_UNKNOWN;
    }
}

#undef QLI_SIGNATURE

static int container_shape(const QlicLiteDescription *desc)
{
    return desc->width && desc->height && desc->width <= 32768 &&
           desc->height <= 32768 && (uint64_t)desc->width * desc->height <= (1u << 25);
}

static QlicLiteStatus description_status(int status)
{
    return status == QLH_OK ? QLIC_LITE_OK : status == QLH_LIMIT ? QLIC_LITE_LIMIT : QLIC_LITE_INVALID;
}

QlicLiteStatus qli_inspect(const uint8_t *data, size_t size, const QlicLiteLimits *limits, QliView *view)
{
    memset(view, 0, sizeof(*view));
    view->format = container_format(data);
    if (view->format == QLI_FORMAT_UNKNOWN) return QLIC_LITE_INVALID;
    if (view->format == QLI_FORMAT_TRANSPOSED_HISTORY) {
        if (size < 60 || container_format(data + 4) != QLI_FORMAT_HISTORY)
            return QLIC_LITE_INVALID;
        unsigned channels = (!memcmp(data + 4, "QRV3", 4) || !memcmp(data + 4, "QRVP", 4)) ? 3 : 4;
        if (channels == 4 && size < 69) return QLIC_LITE_INVALID;
        view->desc = (QlicLiteDescription){
            .width = qli_read_le32(data + 12), .height = qli_read_le32(data + 8),
            .bits = 8, .channels = channels, .alpha = channels == 4
        };
        if (!container_shape(&view->desc) ||
            (uint64_t)view->desc.width * view->desc.height > (1u << 20))
            return QLIC_LITE_INVALID;
        size_t pixels = (size_t)view->desc.width * view->desc.height;
        if (pixels > limits->pixels || pixels * channels > limits->pixel_data_bytes) return QLIC_LITE_LIMIT;
        if (qint_crc32(data, size - 4) != qli_read_le32(data + size - 4)) return QLIC_LITE_INVALID;
        view->decoded_bytes = pixels * channels;
        return QLIC_LITE_OK;
    }
    if (view->format == QLI_FORMAT_AFFINE) {
        unsigned width, height;
        unsigned channels = data[3] == '3' ? 3 : 4;
        if (!qaf_inspect(data, size, &width, &height, channels)) return QLIC_LITE_INVALID;
        view->desc = (QlicLiteDescription){
            .width = width, .height = height, .bits = 8, .channels = channels, .alpha = channels == 4
        };
        size_t pixels = (size_t)width * height;
        view->decoded_bytes = pixels * channels;
        return pixels > limits->pixels || view->decoded_bytes > limits->pixel_data_bytes
            ? QLIC_LITE_LIMIT : QLIC_LITE_OK;
    }
    if (view->format == QLI_FORMAT_ICON) {
        if (size < ICON_MIN_FILE || data[4] >= ICON_SIDE || data[5] >= ICON_SIDE || data[6] > 1)
            return QLIC_LITE_INVALID;
        view->desc = (QlicLiteDescription){
            .width = data[4] + 1u, .height = data[5] + 1u, .bits = 8, .channels = 4, .alpha = 1
        };
        size_t pixels = (size_t)view->desc.width * view->desc.height;
        view->decoded_bytes = pixels * 4;
        return pixels > limits->pixels || view->decoded_bytes > limits->pixel_data_bytes
            ? QLIC_LITE_LIMIT : QLIC_LITE_OK;
    }
    if (view->format != QLI_FORMAT_SAMPLES) {
        view->desc.width = qli_read_le32(data + 4);
        view->desc.height = qli_read_le32(data + 8);
        view->desc.bits = 8;
        const int rgb = (view->format == QLI_FORMAT_NUMERICAL && data[13] == 3) ||
                        ((view->format == QLI_FORMAT_TILE || view->format == QLI_FORMAT_STRIP) && data[14] == 3) ||
                        (view->format == QLI_FORMAT_HISTORY && data[1] == 'R') ||
                        (view->format == QLI_FORMAT_PALETTE && data[3] == '3');
        view->desc.channels = rgb ? 3 : 4;
        view->desc.alpha = !rgb;
        if (!container_shape(&view->desc)) return QLIC_LITE_INVALID;
        size_t pixels = (size_t)view->desc.width * view->desc.height;
        view->decoded_bytes = pixels * view->desc.channels;
        if (pixels > limits->pixels || view->decoded_bytes > limits->pixel_data_bytes)
            return QLIC_LITE_LIMIT;
        return QLIC_LITE_OK;
    }

    if (size < 28 || data[4] != 1 || data[5] > 1 || data[6] || data[7] ||
        qli_read_le32(data + 20)) return QLIC_LITE_INVALID;
    size_t desc_bytes = qli_read_le32(data + 8);
    size_t core_bytes = qli_read_le32(data + 12);
    size_t tail_bytes = qli_read_le32(data + 16);
    if (desc_bytes < 32 || (uint64_t)28 + desc_bytes + core_bytes + tail_bytes != size ||
        (!data[5] && core_bytes) || (data[5] && !core_bytes)) return QLIC_LITE_INVALID;
    QlhLimits desc_limits = qli_description_limits(limits);
    QlicLiteStatus status = description_status(qlh_parse(data + 24, desc_bytes, &desc_limits, &view->desc));
    if (status != QLIC_LITE_OK) return status;
    if (!container_shape(&view->desc)) return QLIC_LITE_INVALID;

    size_t pixels = (size_t)view->desc.width * view->desc.height;
    size_t values = pixels * view->desc.channels;
    unsigned bits = data[5] ? view->desc.bits - 8 : view->desc.bits;
    size_t decoded_bytes = values * (view->desc.bits == 8 ? 1 : 2);
    if (tail_bytes != qli_packed_bytes(values, bits)) return QLIC_LITE_INVALID;
    unsigned last_bits = (unsigned)((values * bits) & 7);
    if (last_bits && (data[24 + desc_bytes + core_bytes + tail_bytes - 1] >> last_bits))
        return QLIC_LITE_INVALID;
    if (pixels > limits->pixels || decoded_bytes > limits->pixel_data_bytes) return QLIC_LITE_LIMIT;
    if (qint_crc32(data, size - 4) != qli_read_le32(data + size - 4)) return QLIC_LITE_INVALID;
    view->desc_bytes = desc_bytes;
    view->core_bytes = core_bytes;
    view->decoded_bytes = decoded_bytes;
    view->tail_bits = bits;
    return QLIC_LITE_OK;
}

int pk_decode_owned(const uint8_t *,size_t,uint8_t **,size_t *);

int history_decode_owned(const uint8_t *,size_t,uint8_t **,size_t *);

int qie_decode_owned(const uint8_t *, size_t, size_t, uint8_t **, size_t *);

static int decode_rgba8(QliFormat format, const uint8_t *data, size_t size, size_t cap,
                        uint8_t **out, uint32_t *width, uint32_t *height)
{
    switch (format) {
    case QLI_FORMAT_FAST: return fast_decode_alloc(data, size, cap, out, width, height);
    case QLI_FORMAT_SIMPLE: return qps_decode_alloc(data, size, cap, out, width, height);
    case QLI_FORMAT_TILE: return qep_tile_decode(data, size, cap, out, width, height);
    case QLI_FORMAT_STRIP: return strip_decode(data, size, cap, out, width, height);
    default: return 0;
    }
}

typedef struct {
    const uint8_t *next;
    uint32_t pending;
    unsigned bits;
} PixelReader;

static unsigned read_pixel_bits(PixelReader *reader, unsigned bits)
{
    while (reader->bits < bits) {
        reader->pending |= (uint32_t)*reader->next++ << reader->bits;
        reader->bits += 8;
    }
    unsigned value = reader->pending & ((1u << bits) - 1);
    reader->pending >>= bits;
    reader->bits -= bits;
    return value;
}

QlicLiteStatus qli_decode_body(const uint8_t *data, size_t size, const QliView *view,
                          uint8_t **out, size_t *bytes, QlicLiteDescription *desc)
{
    *out = NULL;
    *bytes = 0;
    *desc = view->desc;
    uint8_t *scratch = NULL, *result = NULL;
    uint32_t width = 0, height = 0;
    QlicLiteStatus status = QLIC_LITE_INVALID;
    size_t pixels = (size_t)desc->width * desc->height;
    if (view->format != QLI_FORMAT_SAMPLES) {
        if (view->format == QLI_FORMAT_NUMERICAL || view->format == QLI_FORMAT_PALETTE) {
            size_t actual = 0, peak = 0;
            int ok = view->format == QLI_FORMAT_NUMERICAL
                ? spatial_banks_decode(data, size, SIZE_MAX, &result, &actual, &peak)
                : pk_decode_owned(data, size, &result, &actual);
            if (!ok || !result || actual != view->decoded_bytes) goto done;
        } else if (view->format == QLI_FORMAT_HISTORY) {
            size_t actual = 0;
            if (!history_decode_owned(data, size, &result, &actual) ||
                !result || actual != view->decoded_bytes) goto done;
        } else if (view->format == QLI_FORMAT_TRANSPOSED_HISTORY) {
            size_t actual = 0;
            if (!history_decode_owned(data + 4, size - 8, &result, &actual) ||
                actual != view->decoded_bytes) goto done;
            if (desc->width == desc->height && desc->channels == 4) transpose_square(result, desc->width);
            else {
                scratch = result;
                result = qli_malloc(actual);
                if (!result) { status = QLIC_LITE_ALLOCATOR; goto done; }
                if (desc->channels == 3) transpose_rgb(result, scratch, desc->height, desc->width);
                else transpose_copy(result, scratch, desc->height, desc->width);
            }
        } else if (view->format == QLI_FORMAT_AFFINE) {
            result = qli_malloc(view->decoded_bytes);
            if (!result) { status = QLIC_LITE_ALLOCATOR; goto done; }
            if (!qaf_pixels(data, size, desc->width, result, view->decoded_bytes, desc->channels)) goto done;
        } else if (view->format == QLI_FORMAT_ICON) {
            size_t actual = 0;
            if (!qie_decode_owned(data, size, pixels, &result, &actual) ||
                actual != view->decoded_bytes) goto done;
        } else {
            if (!decode_rgba8(view->format, data, size, view->decoded_bytes, &result, &width, &height))
                goto done;
            if (!result || width != desc->width || height != desc->height) goto done;
        }
    } else {
        if (view->core_bytes) {
            const uint8_t *core = data + 24 + view->desc_bytes;
            if (view->core_bytes < 16 || !memcmp(core, "QWNL", 4)) goto done;
            if (qli_read_le32(core + 4) != desc->width || qli_read_le32(core + 8) != desc->height)
                goto done;
            const QliFormat core_format = container_format(core);
            if ((core_format == QLI_FORMAT_TILE || core_format == QLI_FORMAT_STRIP) && core[14] != 4)
                goto done;
            if (!decode_rgba8(core_format, core, view->core_bytes, pixels * 4,
                              &scratch, &width, &height) ||
                !scratch || width != desc->width || height != desc->height)
                goto done;
            if (desc->channels < 4) {
                for (size_t i = 0; i < pixels; ++i) {
                    const uint8_t *rgba = scratch + 4 * i;
                    if ((desc->channels <= 2 && (rgba[0] != rgba[1] || rgba[0] != rgba[2])) ||
                        ((desc->channels == 1 || desc->channels == 3) && rgba[3] != 255))
                        goto done;
                }
            }
        }
        const int rgba8 = desc->bits == 8 && desc->channels == 4;
        const int reuse = scratch && !desc->icc_size && view->decoded_bytes == 4 * pixels;
        if (rgba8 && reuse) {
            result = scratch;
            scratch = NULL;
        } else {
            result = reuse ? scratch : qli_malloc(view->decoded_bytes + desc->icc_size);
            if (!result) { status = QLIC_LITE_ALLOCATOR; goto done; }
            const uint8_t *tail = data + 24 + view->desc_bytes + view->core_bytes;
            if (desc->bits == 8) {
                if (!scratch || rgba8) {
                    memcpy(result, scratch ? scratch : tail, view->decoded_bytes);
                } else {
                    unsigned channels = desc->channels;
                    for (size_t i = 0; i < pixels; ++i) {
                        const uint8_t *rgba = scratch + 4 * i;
                        uint8_t *pixel = result + channels * i;
                        pixel[0] = rgba[0];
                        if (channels == 2) pixel[1] = rgba[3];
                        else if (channels == 3) {
                            pixel[1] = rgba[1];
                            pixel[2] = rgba[2];
                        }
                    }
                }
            } else {
                PixelReader reader = {tail, 0, 0};
                for (size_t pixel = 0; pixel < pixels; ++pixel) {
                    for (unsigned c = 0; c < desc->channels; ++c) {
                        unsigned value = read_pixel_bits(&reader, view->tail_bits);
                        unsigned lane = desc->channels == 2 && c == 1 ? 3 : c;
                        if (scratch) value |= (unsigned)scratch[4 * pixel + lane] << view->tail_bits;
                        size_t offset = 2 * (pixel * desc->channels + c);
                        result[offset] = (uint8_t)value;
                        result[offset + 1] = (uint8_t)(value >> 8);
                    }
                }
            }
            if (desc->icc_size) {
                memcpy(result + view->decoded_bytes, desc->icc, desc->icc_size);
                desc->icc = result + view->decoded_bytes;
            }
            if (reuse) scratch = NULL;
        }
    }
    *out = result;
    result = NULL;
    *bytes = view->decoded_bytes;
    status = QLIC_LITE_OK;
done:
    qli_free(scratch);
    qli_free(result);
    return status;
}

typedef struct { uint8_t *data; size_t capacity; } QliEncodeBuffer;

static inline int qli_encode_buffer_reserve(QliEncodeBuffer *buffer, size_t needed, size_t keep)
{
    const size_t limit = (size_t)1 << 29;
    if (needed > limit || keep > needed || keep > buffer->capacity) return 0;
    if (needed <= buffer->capacity) return 1;
    size_t capacity = buffer->capacity ? buffer->capacity : 4096;
    while (capacity < needed) capacity *= 2;
    uint8_t *next = qli_malloc(capacity);
    if (!next) return 0;
    if (keep) memcpy(next, buffer->data, keep);
    qli_free(buffer->data);
    *buffer = (QliEncodeBuffer){next, capacity};
    return 1;
}

static inline void qli_encode_buffer_clear(QliEncodeBuffer *buffer)
{
    qli_free(buffer->data);
    *buffer = (QliEncodeBuffer){0};
}

typedef struct { QliEncodeBuffer scratch, frame; } QliFourWorkspace;
int qli_four_encode(const uint8_t *symbols, size_t count, const RansModel *prepared,
                    QliFourWorkspace *workspace, uint8_t **frame, size_t *size);
static int four_write(const uint8_t *symbols, size_t count, uint8_t **frame, size_t *size, unsigned *used,
                      const RansModel *prepared, QliFourWorkspace *workspace, int contiguous) {
    if (frame) *frame = NULL; if (size) *size = 0;
    if (!frame || !size || (count && !symbols) || count > (1u<<27)) return 0;
    RansModel storage;
    if (!prepared && !qlr_model_build(symbols, count, &storage)) return 0;
    const RansModel *model = prepared ? prepared : &storage;
    uint8_t *buffers[4] = {0}, *cursor[4] = {0}, *out = NULL, *scratch = NULL;
    size_t capacities[4] = {0}, lengths[4] = {0}; uint32_t states[4]; int ok = 0;
    size_t total = RANS_HEADER_BYTES+qlr_model_descriptor_bytes(model)+32;
    if (workspace) {
        if (!qli_encode_buffer_reserve(&workspace->scratch, 2 * count + 4, 0)) goto done;
        scratch = workspace->scratch.data;
    } else if (contiguous) {
        scratch = qli_malloc(2 * count + 4);
        if (!scratch) goto done;
    }
    size_t scratch_at = 0;
    for (unsigned lane = 0; lane < 4; ++lane) {
        size_t n = count > lane ? (count-lane+3)/4 : 0;
        capacities[lane] = n*2+1;
        buffers[lane] = scratch ? scratch + scratch_at : qli_malloc(capacities[lane]);
        scratch_at += capacities[lane];
        if (!buffers[lane]) goto done;
        cursor[lane] = buffers[lane]+capacities[lane]; uint32_t v = RANS_BYTE_L;
        for (size_t j = model->used > 1 ? n : 0; j-- > 0;) {
            unsigned symbol = symbols[j*4+lane]; uint32_t f = model->frequency[symbol], base = model->cumulative[symbol];
            if (!f) goto done;
            uint32_t max = ((RANS_BYTE_L >> QLIC_RANS_SCALE_BITS)<<8)*f;
            while (v >= max) { if (cursor[lane] == buffers[lane]) goto done; *--cursor[lane] = (uint8_t)v; v >>= 8; }
            v = (v/f << QLIC_RANS_SCALE_BITS) + v%f+base;
        }
        states[lane] = v; lengths[lane] = (size_t)(buffers[lane]+capacities[lane]-cursor[lane]); total += lengths[lane];
    }
    if (workspace) {
        if (!qli_encode_buffer_reserve(&workspace->frame, total, 0)) goto done;
        out = workspace->frame.data;
        memset(out, 0, total);
    } else {
        out = qli_calloc(1, total);
        if (!out) goto done;
    }
    memcpy(out, "QBS1", 4); out[4] = 4; out[5] = QLIC_RANS_SCALE_BITS; qli_write_le64(out+6, count);
    size_t offset = RANS_HEADER_BYTES; if (!qlr_model_write(model, out, total, &offset)) goto done;
    for (unsigned lane = 0; lane < 4; ++lane) { qli_write_le32(out+offset+lane*4, states[lane]); qli_write_le32(out+offset+16+lane*4, (uint32_t)lengths[lane]); }
    offset += 32;
    for (unsigned lane = 0; lane < 4; ++lane) { memcpy(out+offset, cursor[lane], lengths[lane]); offset += lengths[lane]; }
    if (offset != total) goto done;
    *frame = out; *size = total; out = NULL; ok = 1;
    if (used) *used = model->used;
done:
    if (!workspace) {
        if (contiguous) qli_free(scratch);
        else for (unsigned lane = 0; lane < 4; ++lane) qli_free(buffers[lane]);
        qli_free(out);
    }
    return ok;
}

static int write_four_separate(const uint8_t *symbols, size_t count,
                               uint8_t **frame, size_t *size, unsigned *used)
{
    return four_write(symbols, count, frame, size, used, NULL, NULL, 0);
}

int qli_four_encode(const uint8_t *symbols, size_t count, const RansModel *prepared,
                    QliFourWorkspace *workspace, uint8_t **frame, size_t *size)
{
    return four_write(symbols, count, frame, size, NULL, prepared, workspace, 1);
}

enum { QLI_HISTORY_HEADER = 28, QLI_HISTORY_RECORDS = 8, QLI_HISTORY_SLOTS = 65536,
       QLI_HISTORY_FILE_CAP = 1u << 27, QLI_HISTORY_PIXEL_CAP = 1u << 24 };

static inline int history_dimensions(unsigned w, unsigned h)
{
    return w && h && w <= 16384 && h <= 16384 && (size_t)w * h <= QLI_HISTORY_PIXEL_CAP;
}

static void repeat_bytes(uint8_t *dst,size_t distance,size_t length);

enum { SPAN=32, CACHE=128, MISS=255 };

#define HISTORY_KERNEL static QLI_INLINE

typedef struct {
    uint32_t colors[CACHE];
    uint8_t present[CACHE];
} ColorCache;

static int history_shape(unsigned w,unsigned h,size_t bytes, unsigned channels)
{
    return history_dimensions(w,h) && bytes==channels*(size_t)w*h;
}

HISTORY_KERNEL uint32_t pixel(const uint8_t *image,size_t at, unsigned channels)
{
    uint32_t value=0;
    memcpy(&value,image+channels*at,channels);
    return value;
}

HISTORY_KERNEL void store_pixel(uint8_t *image,size_t at,uint32_t value,unsigned channels)
{
    memcpy(image+channels*at,&value,channels);
}

HISTORY_KERNEL uint32_t history_key(uint32_t color,int virtual_alpha)
{
    return virtual_alpha ? color|UINT32_C(0xff000000) : color;
}

static unsigned history_color_slot(uint32_t color)
{
    return (color*UINT32_C(2654435761))>>25;
}

static void remember(ColorCache *cache,uint32_t color,int virtual_alpha)
{
    unsigned i=history_color_slot(history_key(color,virtual_alpha));
    cache->colors[i]=color;
    cache->present[i]=1;
}

static unsigned gradient(unsigned west,unsigned north,unsigned diagonal)
{
    unsigned lo=west<north ? west : north;
    unsigned hi=west>north ? west : north;
    if (diagonal<lo) return hi;
    if (diagonal>hi) return lo;
    return west+north-diagonal;
}

static QLI_INLINE uint32_t history_gradient(uint32_t west,uint32_t north,uint32_t diagonal,
                                           unsigned channels)
{
    uint32_t result=0;
    for (unsigned shift=0;shift<8*channels;shift+=8)
        result|=(uint32_t)gradient((west>>shift)&255,(north>>shift)&255,
                                   (diagonal>>shift)&255)<<shift;
    return result;
}

static uint32_t predict(const uint8_t *image,unsigned w,unsigned y,unsigned x,unsigned channels)
{
    size_t at=(size_t)y*w+x;
    uint32_t north=y ? pixel(image,at-w,channels) : 0;
    uint32_t west=x ? pixel(image,at-1,channels) : north;
    if (!y) north=west;
    uint32_t diagonal=x && y ? pixel(image,at-w-1,channels) : north;
    return history_gradient(west,north,diagonal,channels);
}

static inline uint32_t add_history_error(uint32_t prediction,const uint8_t *error,unsigned channels)
{
    const uint32_t e=pixel(error,0,channels),low=UINT32_C(0x7f7f7f7f);
    return ((prediction&low)+(e&low))^((prediction^e)&~low);
}

#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42 && (defined(_M_X64) || defined(__x86_64__))
#define QLI_HISTORY_VECTOR 1
static inline uint32_t history_vector_literal(const uint8_t *image,size_t at,unsigned w,
                                               unsigned x,unsigned y,const uint8_t *error)
{
    const __m128i encoded=_mm_cvtsi32_si128((int)pixel(error,0,4));
    __m128i prediction;
    if (x && y) {
        const __m128i zero=_mm_setzero_si128();
        const __m128i west=_mm_unpacklo_epi8(_mm_cvtsi32_si128((int)pixel(image,at-1,4)),zero);
        const __m128i north=_mm_unpacklo_epi8(_mm_cvtsi32_si128((int)pixel(image,at-w,4)),zero);
        const __m128i corner=_mm_unpacklo_epi8(_mm_cvtsi32_si128((int)pixel(image,at-w-1,4)),zero);
        const __m128i gradient=_mm_sub_epi16(_mm_add_epi16(west,north),corner);
        const __m128i bounded=_mm_max_epi16(_mm_min_epi16(west,north),
                                           _mm_min_epi16(_mm_max_epi16(west,north),gradient));
        prediction=_mm_packus_epi16(bounded,zero);
    } else {
        prediction=_mm_cvtsi32_si128((int)(x ? pixel(image,at-1,4) : y ? pixel(image,at-w,4) : 0));
    }
    return (uint32_t)_mm_cvtsi128_si32(_mm_add_epi8(prediction,encoded));
}
#else
#define QLI_HISTORY_VECTOR 0
#endif

enum { REUSE=254, MIN_REUSE=8, MAX_REUSE=128 };

HISTORY_KERNEL int reconstruct(const uint8_t *tokens,size_t count,const uint8_t *errors,size_t literals,
                    unsigned w,unsigned h,uint8_t *image,size_t bytes,
                    const uint8_t *lengths,const uint16_t *distances,size_t copies,unsigned channels,
                    int virtual_alpha)
{
    if (!tokens || !errors || !image || !history_shape(w,h,bytes,channels) ||
        !count || count>bytes/channels || literals>count) return 0;
    ColorCache cache={0};
    if ((copies && (!lengths || !distances)) || copies>count || copies>bytes/(8*channels)) return 0;
#if QLI_HISTORY_VECTOR
    const int native=channels==4 && (qli_cpu_capabilities() & QLI_CPU_SSE42)!=0;
#endif
    size_t read=0,used=0,reused=0;
    for (unsigned y=0;y<h;++y) for (unsigned x=0;x<w;) {
        if (read==count) return 0;
        unsigned token=tokens[read++];
        size_t at=(size_t)y*w+x;
        if (token<128) {
            unsigned mode=token/SPAN,length=(token%SPAN)+1;
            if (length>w-x) return 0;
            if (!mode) {
                if (!x) return 0;
                uint32_t value=pixel(image,at-1,channels);
                for (unsigned j=0;j<length;++j) store_pixel(image,at+j,value,channels);
            } else {
                const int shifts[3]={0,-1,1};
                int source=(int)x+shifts[mode-1];
                if (!y || source<0 || (unsigned)source+length>w) return 0;
                size_t previous=(size_t)(y-1)*w+(unsigned)source;
                memcpy(image+channels*at,image+channels*previous,channels*length);
            }
            x+=length;
        } else if (token==REUSE) {
            if (reused==copies) return 0;
            unsigned length=MIN_REUSE+lengths[reused];
            size_t distance=1u+distances[reused++];
            if (distance>at || length>MAX_REUSE || length>w-x) return 0;
            repeat_bytes(image+channels*at,channels*distance,channels*length);
            x+=length;
        } else if (token<MISS) {
            unsigned id=token-128;
            if (!cache.present[id]) return 0;
            store_pixel(image,at,cache.colors[id],channels);
            ++x;
        } else {
            if (used==literals) return 0;
            const uint8_t *error=errors+channels*used++;
            uint32_t value;
#if QLI_HISTORY_VECTOR
            if (native) value=history_vector_literal(image,at,w,x,y,error);
            else
#endif
                value=add_history_error(predict(image,w,y,x,channels),error,channels);
            store_pixel(image,at,value,channels);
            remember(&cache,value,virtual_alpha);
            ++x;
        }
    }
    return read==count && used==literals && reused==copies;
}

#undef QLI_HISTORY_VECTOR
#undef HISTORY_KERNEL

int history_reconstruct(const uint8_t *tokens,size_t count,const uint8_t *errors,size_t literals,
                        unsigned w,unsigned h,uint8_t *image,size_t bytes,
                        const uint8_t *lengths,const uint16_t *distances,size_t copies,unsigned channels,
                        int virtual_alpha)
{
    if (channels==3) {
        if (virtual_alpha)
            return reconstruct(tokens,count,errors,literals,w,h,image,bytes,lengths,distances,copies,3,1);
        return reconstruct(tokens,count,errors,literals,w,h,image,bytes,lengths,distances,copies,3,0);
    }
    return reconstruct(tokens,count,errors,literals,w,h,image,bytes,lengths,distances,copies,channels,
                       virtual_alpha);
}

typedef struct {
    uint64_t header_bytes;
    uint64_t table_bytes;
    uint64_t payload_bits;
    uint64_t payload_bytes;
    unsigned form;
} QlicLiteByteHuffmanStats;

enum {
    QLIC_LITE_BYTE_RAW,
    QLIC_LITE_BYTE_CONSTANT,
    QLIC_LITE_BYTE_HUFFMAN
};

int qlic_lite_huffman_bytes_decode(
    const uint8_t *input, size_t size, uint8_t *output, size_t output_size,
    QlicLiteByteHuffmanStats *stats_out);

enum {
    QLI_HUFFMAN_SYMBOLS = 256,
    QLI_HUFFMAN_DESCRIPTOR_CAP = 1 + QLI_HUFFMAN_SYMBOLS * 2 + 2,
    QLI_BYTE_INPUT_CAP = 1u << 28
};

typedef struct {
    uint64_t count[QLI_HUFFMAN_SYMBOLS];
    uint8_t length[QLI_HUFFMAN_SYMBOLS];
    uint32_t code[QLI_HUFFMAN_SYMBOLS];
    unsigned alphabet, max_bits;
} QliCodebook;

typedef struct {
    QliCodebook book;
    uint8_t descriptor[QLI_HUFFMAN_DESCRIPTOR_CAP];
    size_t descriptor_bytes, bytes;
    uint64_t bits;
    unsigned form, sole;
} QliBytePlan;

int qli_byte_plan(const uint8_t *input, size_t size, QliBytePlan *plan);
int qli_byte_emit(const QliBytePlan *plan, const uint8_t *input, size_t size, uint8_t *out);

enum {
    SYMBOLS = QLI_HUFFMAN_SYMBOLS,
    MAX_BITS = 24,
    PRIMARY_BITS = 10,
    MAX_DESCRIPTOR_BYTES = QLI_HUFFMAN_DESCRIPTOR_CAP,
    BYTE_HEADER_BYTES = 11
};

typedef QliCodebook Codebook;

typedef struct {
    uint64_t weight;
    unsigned minimum_symbol;
    int parent;
} Node;

typedef struct {
    uint8_t *next;
    uint8_t *end;
    uint64_t bits;
    unsigned count;
    uint64_t written;
    int failed;
} BitWriter;

typedef struct {
    const uint8_t *next;
    const uint8_t *end;
    uint64_t bits;
    unsigned count;
    uint64_t consumed;
    uint64_t limit;
} BitReader;

typedef struct {
    uint16_t *entry;
    unsigned bits;
    unsigned max_bits;
    uint32_t first[MAX_BITS + 1];
    uint16_t first_symbol[MAX_BITS + 1];
    uint16_t count[MAX_BITS + 1];
    uint8_t symbol[SYMBOLS];
} DecodeTable;

static int node_less(const Node *node, unsigned a, unsigned b) {
    if (node[a].weight != node[b].weight)
        return node[a].weight < node[b].weight;
    return node[a].minimum_symbol < node[b].minimum_symbol;
}

static void heap_push(unsigned *heap, unsigned *size, const Node *node,
                      unsigned value) {
    unsigned child = (*size)++;
    while (child) {
        unsigned parent = (child - 1u) >> 1;
        if (!node_less(node, value, heap[parent])) break;
        heap[child] = heap[parent];
        child = parent;
    }
    heap[child] = value;
}

static unsigned heap_pop(unsigned *heap, unsigned *size, const Node *node) {
    unsigned result = heap[0];
    unsigned value = heap[--*size];
    unsigned parent = 0;
    while (parent * 2u + 1u < *size) {
        unsigned child = parent * 2u + 1u;
        if (child + 1u < *size && node_less(node, heap[child + 1u],
                                            heap[child]))
            child++;
        if (!node_less(node, heap[child], value)) break;
        heap[parent] = heap[child];
        parent = child;
    }
    if (*size) heap[parent] = value;
    return result;
}

static int canonical_codes(Codebook *book) {
    unsigned count[MAX_BITS + 1] = {0};
    unsigned next[MAX_BITS + 1] = {0};
    unsigned code = 0;
    book->max_bits = 0;
    for (unsigned symbol = 0; symbol < book->alphabet; ++symbol) {
        unsigned length = book->length[symbol];
        if (length > MAX_BITS) return 0;
        if (length) {
            count[length]++;
            if (length > book->max_bits) book->max_bits = length;
        }
    }
    for (unsigned bits = 1; bits <= MAX_BITS; ++bits) {
        code = (code + count[bits - 1u]) << 1;
        if (code + count[bits] > (UINT32_C(1) << bits)) return 0;
        next[bits] = code;
    }
    for (unsigned symbol = 0; symbol < book->alphabet; ++symbol) {
        unsigned length = book->length[symbol];
        if (length) book->code[symbol] = next[length]++;
    }
    return 1;
}

static int build_codebook(Codebook *book) {
    Node node[SYMBOLS * 2 - 1];
    unsigned heap[SYMBOLS * 2 - 1];
    unsigned heap_size = 0;
    unsigned nodes = book->alphabet;
    unsigned used = 0;
    unsigned sole_symbol = 0;
    memset(book->length, 0, sizeof(book->length));
    memset(book->code, 0, sizeof(book->code));
    for (unsigned symbol = 0; symbol < book->alphabet; ++symbol) {
        node[symbol].weight = book->count[symbol];
        node[symbol].minimum_symbol = symbol;
        node[symbol].parent = -1;
        if (book->count[symbol]) {
            heap_push(heap, &heap_size, node, symbol);
            sole_symbol = symbol;
            used++;
        }
    }
    if (!used) {
        book->max_bits = 0;
        return 1;
    }
    if (used == 1u) {
        book->length[sole_symbol] = 1;
        return canonical_codes(book);
    }
    while (heap_size > 1u) {
        unsigned left = heap_pop(heap, &heap_size, node);
        unsigned right = heap_pop(heap, &heap_size, node);
        if (nodes >= SYMBOLS * 2u - 1u ||
            node[right].weight > UINT64_MAX - node[left].weight)
            return 0;
        node[nodes].weight = node[left].weight + node[right].weight;
        node[nodes].minimum_symbol =
            node[left].minimum_symbol < node[right].minimum_symbol
                ? node[left].minimum_symbol
                : node[right].minimum_symbol;
        node[nodes].parent = -1;
        node[left].parent = (int)nodes;
        node[right].parent = (int)nodes;
        heap_push(heap, &heap_size, node, nodes++);
    }

    for (unsigned symbol = 0; symbol < book->alphabet; ++symbol) {
        if (!book->count[symbol]) continue;
        int current = (int)symbol;
        while (node[current].parent >= 0) {
            if (book->length[symbol] == MAX_BITS) return 0;
            book->length[symbol]++;
            current = node[current].parent;
        }
    }
    if (!canonical_codes(book)) {
        fprintf(stderr, "invalid canonical Huffman lengths\n");
        return 0;
    }
    return 1;
}

static int make_descriptor(const Codebook *book, uint8_t *output,
                           size_t *size_out) {
    unsigned used = 0;
    unsigned runs = 0;
    for (unsigned symbol = 0; symbol < book->alphabet; ++symbol)
        used += book->length[symbol] != 0;
    for (unsigned symbol = 0; symbol < book->alphabet;) {
        uint8_t length = book->length[symbol];
        unsigned end = symbol + 1u;
        while (end < book->alphabet && book->length[end] == length) end++;
        runs++;
        symbol = end;
    }
    size_t raw_size = 1u + book->alphabet;
    size_t sparse_size = 3u + (size_t)used * 2u;
    size_t run_size = 1u + (size_t)runs * 2u;
    if (raw_size <= sparse_size && raw_size <= run_size) {
        output[0] = 0;
        memcpy(output + 1, book->length, book->alphabet);
        *size_out = raw_size;
        return 1;
    }
    if (sparse_size <= run_size) {
        output[0] = 1;
        qli_write_le16(output + 1, used);
        size_t next = 3;
        for (unsigned symbol = 0; symbol < book->alphabet; ++symbol) {
            if (!book->length[symbol]) continue;
            output[next++] = (uint8_t)symbol;
            output[next++] = book->length[symbol];
        }
        *size_out = next;
        return 1;
    }
    output[0] = 2;
    size_t next = 1;
    for (unsigned symbol = 0; symbol < book->alphabet;) {
        uint8_t length = book->length[symbol];
        unsigned end = symbol + 1u;
        while (end < book->alphabet && book->length[end] == length) end++;
        output[next++] = length;
        output[next++] = (uint8_t)(end - symbol - 1u);
        symbol = end;
    }
    *size_out = next;
    return 1;
}

static int parse_descriptor(Codebook *book, const uint8_t *input,
                            size_t size) {
    if (!size) return 0;
    memset(book->length, 0, sizeof(book->length));
    if (input[0] == 0) {
        if (size != 1u + book->alphabet) return 0;
        memcpy(book->length, input + 1, book->alphabet);
    } else if (input[0] == 1) {
        if (size < 3u) return 0;
        unsigned used = qli_read_le16(input + 1);
        if (size != 3u + (size_t)used * 2u || used > book->alphabet)
            return 0;
        unsigned previous = UINT_MAX;
        for (unsigned index = 0; index < used; ++index) {
            unsigned symbol = input[3u + index * 2u];
            unsigned length = input[4u + index * 2u];
            if (symbol >= book->alphabet || !length || length > MAX_BITS ||
                (previous != UINT_MAX && symbol <= previous))
                return 0;
            book->length[symbol] = (uint8_t)length;
            previous = symbol;
        }
    } else if (input[0] == 2) {
        if ((size & 1u) != 1u) return 0;
        unsigned symbol = 0;
        for (size_t next = 1; next < size; next += 2u) {
            unsigned length = input[next];
            unsigned run = (unsigned)input[next + 1u] + 1u;
            if (length > MAX_BITS || run > book->alphabet - symbol) return 0;
            memset(book->length + symbol, (int)length, run);
            symbol += run;
        }
        if (symbol != book->alphabet) return 0;
    } else {
        return 0;
    }
    return canonical_codes(book);
}

static void writer_put(BitWriter *writer, unsigned value, unsigned bits) {
    if (writer->failed || !bits || bits > MAX_BITS) {
        writer->failed = 1;
        return;
    }
    writer->bits = (writer->bits << bits) |
                   (value & ((UINT64_C(1) << bits) - 1u));
    writer->count += bits;
    writer->written += bits;
    while (writer->count >= 8u) {
        if (writer->next == writer->end) {
            writer->failed = 1;
            return;
        }
        writer->count -= 8u;
        *writer->next++ = (uint8_t)(writer->bits >> writer->count);
        if (writer->count)
            writer->bits &= ((UINT64_C(1) << writer->count) - 1u);
        else
            writer->bits = 0;
    }
}

static void writer_symbol(BitWriter *writer, const Codebook *book,
                          unsigned symbol) {
    if (symbol >= book->alphabet || !book->length[symbol]) {
        writer->failed = 1;
        return;
    }
    writer_put(writer, book->code[symbol], book->length[symbol]);
}

static int writer_finish(BitWriter *writer) {
    if (writer->failed) return 0;
    if (writer->count) {
        if (writer->next == writer->end) return 0;
        *writer->next++ = (uint8_t)(writer->bits << (8u - writer->count));
        writer->bits = 0;
        writer->count = 0;
    }
    return writer->next == writer->end;
}

static void decode_order(const Codebook *book, DecodeTable *table) {
    unsigned ordered = 0;
    for (unsigned length = 1; length <= book->max_bits; ++length) {
        table->first_symbol[length] = (uint16_t)ordered;
        for (unsigned symbol = 0; symbol < book->alphabet; ++symbol) {
            if (book->length[symbol] != length) continue;
            if (!table->count[length]) table->first[length] = book->code[symbol];
            table->count[length]++;
            table->symbol[ordered++] = (uint8_t)symbol;
        }
    }
}

static int build_decode_table(const Codebook *book, DecodeTable *table) {
    memset(table, 0, sizeof(*table));
    table->max_bits = book->max_bits;
    table->bits = book->max_bits < PRIMARY_BITS ? book->max_bits : PRIMARY_BITS;
    if (!table->bits) return 1;
    size_t entries = (size_t)1u << table->bits;
    table->entry = qli_calloc(entries, sizeof(*table->entry));
    if (!table->entry) return 0;
    decode_order(book, table);
    for (unsigned symbol = 0; symbol < book->alphabet; ++symbol) {
        unsigned length = book->length[symbol];
        if (!length || length > table->bits) continue;
        unsigned base = book->code[symbol] << (table->bits - length);
        unsigned copies = 1u << (table->bits - length);
        uint16_t entry = (uint16_t)((length << 8) | symbol);
        for (unsigned index = 0; index < copies; ++index) {
            if (table->entry[base + index]) return 0;
            table->entry[base + index] = entry;
        }
    }
    return 1;
}

static int reader_peek(BitReader *reader, unsigned bits, unsigned *value) {
    while (reader->count < bits && reader->next != reader->end) {
        reader->bits = (reader->bits << 8) | *reader->next++;
        reader->count += 8u;
    }
    if (reader->count >= bits)
        *value = (unsigned)(reader->bits >> (reader->count - bits));
    else
        *value = (unsigned)(reader->bits << (bits - reader->count));
    *value &= (1u << bits) - 1u;
    return 1;
}

static int reader_drop(BitReader *reader, unsigned bits) {
    if (!bits || reader->consumed > reader->limit ||
        bits > reader->limit - reader->consumed)
        return 0;
    if (reader->count < bits) return 0;
    reader->count -= bits;
    reader->consumed += bits;
    if (reader->count)
        reader->bits &= (UINT64_C(1) << reader->count) - 1u;
    else
        reader->bits = 0;
    return 1;
}

static int reader_get(BitReader *reader, unsigned bits, unsigned *value) {
    if (bits > 16u || reader->consumed > reader->limit ||
        bits > reader->limit - reader->consumed)
        return 0;
    while (reader->count < bits) {
        if (reader->next == reader->end) return 0;
        reader->bits = (reader->bits << 8) | *reader->next++;
        reader->count += 8u;
    }
    *value = (unsigned)(reader->bits >> (reader->count - bits));
    return reader_drop(reader, bits);
}

static int reader_long_symbol(BitReader *reader, const DecodeTable *table, unsigned *symbol) {
    unsigned length;
    unsigned code = 0;
    if (!reader_get(reader, table->bits, &code)) return 0;
    for (length = table->bits + 1u; length <= table->max_bits; ++length) {
        unsigned bit = 0;
        if (!reader_get(reader, 1u, &bit)) return 0;
        code = (code << 1) | bit;
        if (table->count[length] && code >= table->first[length] &&
            code - table->first[length] < table->count[length]) {
            *symbol = table->symbol[table->first_symbol[length] +
                                    code - table->first[length]];
            return 1;
        }
    }
    return 0;
}

static int reader_symbol(BitReader *reader, const DecodeTable *table,
                         unsigned *symbol) {
    if (!table->bits) return 0;
    unsigned index = 0;
    reader_peek(reader, table->bits, &index);
    uint16_t entry = table->entry[index];
    unsigned length = entry >> 8;
    if (length) {
        if (!reader_drop(reader, length)) return 0;
        *symbol = entry & 255u;
        return 1;
    }
    return reader_long_symbol(reader, table, symbol);
}

int qli_byte_plan(const uint8_t *input, size_t size, QliBytePlan *plan)
{
    if ((!input && size) || size > QLI_BYTE_INPUT_CAP) return 0;
    memset(plan, 0, sizeof(*plan));
    plan->book.alphabet = SYMBOLS;
    for (size_t i = 0; i < size; ++i) ++plan->book.count[input[i]];
    unsigned used = 0;
    for (unsigned symbol = 0; symbol < SYMBOLS; ++symbol) {
        if (!plan->book.count[symbol]) continue;
        plan->sole = symbol;
        ++used;
    }
    if (size >= 2 && used == 1) {
        plan->form = QLIC_LITE_BYTE_CONSTANT;
        plan->bytes = 2;
        return 1;
    }
    if (used >= 2 && build_codebook(&plan->book) &&
        make_descriptor(&plan->book, plan->descriptor, &plan->descriptor_bytes)) {
        for (unsigned symbol = 0; symbol < SYMBOLS; ++symbol)
            plan->bits += plan->book.count[symbol] * plan->book.length[symbol];
        size_t bytes = BYTE_HEADER_BYTES + plan->descriptor_bytes + (size_t)((plan->bits + 7) / 8);
        if (bytes < size + 1) {
            plan->form = QLIC_LITE_BYTE_HUFFMAN;
            plan->bytes = bytes;
            return 1;
        }
    }
    plan->form = QLIC_LITE_BYTE_RAW;
    plan->bytes = size + 1;
    plan->bits = (uint64_t)size * 8;
    plan->descriptor_bytes = 0;
    return 1;
}

int qli_byte_emit(const QliBytePlan *plan, const uint8_t *input, size_t size, uint8_t *out)
{
    out[0] = (uint8_t)plan->form;
    if (plan->form == QLIC_LITE_BYTE_CONSTANT) {
        out[1] = (uint8_t)plan->sole;
        return 1;
    }
    if (plan->form == QLIC_LITE_BYTE_RAW) {
        if (size) memcpy(out + 1, input, size);
        return 1;
    }
    qli_write_le16(out + 1, (unsigned)plan->descriptor_bytes);
    qli_write_le64(out + 3, plan->bits);
    memcpy(out + BYTE_HEADER_BYTES, plan->descriptor, plan->descriptor_bytes);
    BitWriter writer = { .next = out + BYTE_HEADER_BYTES + plan->descriptor_bytes,
                         .end = out + plan->bytes };
    for (size_t i = 0; i < size; ++i) writer_symbol(&writer, &plan->book, input[i]);
    return writer_finish(&writer) && writer.written == plan->bits;
}

#define QB_ENTER() ((void)0)
#define QB_FULL() ((void)0)
#define QB_BREAK(n) ((void)0)
#define QB_SHORT_INPUT() ((void)0)
#define QB_BUFFER(n) ((void)0)
typedef struct { DecodeTable single; uint32_t *pair; unsigned bits; } PairTable;
static int pair_build(const Codebook *book,PairTable *p){
    memset(p,0,sizeof(*p));unsigned shortest=MAX_BITS+1;
    for(unsigned s=0;s<book->alphabet;++s)if(book->length[s] && book->length[s]<shortest)shortest=book->length[s];
    unsigned bits=2*book->max_bits;if(bits>PRIMARY_BITS)bits=PRIMARY_BITS;
    if(2*shortest>bits)return build_decode_table(book,&p->single);
    p->bits=bits;p->single.max_bits=book->max_bits;
    p->single.bits=book->max_bits<PRIMARY_BITS?book->max_bits:PRIMARY_BITS;
    decode_order(book,&p->single);
    size_t entries=(size_t)1u<<bits;p->pair=qli_calloc(entries,sizeof(*p->pair));if(!p->pair)return 0;
    for(unsigned s=0;s<book->alphabet;++s){
        unsigned n=book->length[s];if(!n || n>bits)continue;
        unsigned start=book->code[s]<<(bits-n),copies=1u<<(bits-n);
        uint32_t word=s|(n<<16)|(n<<24);
        for(unsigned i=0;i<copies;++i){if(p->pair[start+i])return 0;p->pair[start+i]=word;}
    }
    for(unsigned i=0;i<entries;++i){
        uint32_t first=p->pair[i];unsigned n=(first>>24)&31;
        if(!n || n==bits)continue;
        uint32_t second=p->pair[(i<<n)&(entries-1)];unsigned m=(second>>24)&31;
        if(m && m<=bits-n)p->pair[i]=(first&UINT32_C(0xff0000ff))|((second&255)<<8)|((n+m)<<16)|(1u<<21);
    }
    return 1;
}
static int pair_decode(BitReader *reader,const PairTable *p,uint8_t *output,size_t n){
    if(!p->pair){
        for(size_t i=0;i<n;++i){unsigned s=0;if(!reader_symbol(reader,&p->single,&s))return 0;output[i]=(uint8_t)s;}
        return 1;
    }
    size_t i=0;
    while(i<n){

        unsigned batch_bits=4u*p->bits;
        if(n-i>=8 && reader->consumed<=reader->limit &&
           batch_bits<=reader->limit-reader->consumed){
            while(reader->count<batch_bits && reader->next!=reader->end){
                reader->bits=(reader->bits<<8)|*reader->next++;
                reader->count+=8u;
            }
            QB_BUFFER(reader->count);
            if(reader->count>=batch_bits){
                QB_ENTER();
                unsigned count=reader->count,step=0;
                uint64_t bits=reader->bits;
                const unsigned mask=(1u<<p->bits)-1u;
                for(;step<4;++step){
                    unsigned key=(unsigned)(bits>>(count-p->bits))&mask;
                    uint32_t word=p->pair[key];
                    if(!((word>>24)&31u)){QB_BREAK(step);break;}
                    count-=(word>>16)&31u;
                    output[i++]=(uint8_t)word;
                    if(word&(1u<<21))output[i++]=(uint8_t)(word>>8);
                }
                reader->consumed+=reader->count-count;
                reader->count=count;
                reader->bits=bits&((UINT64_C(1)<<count)-1u);
                if(step==4){QB_FULL();continue;}
            }else{QB_SHORT_INPUT();}
        }
        unsigned key=0;reader_peek(reader,p->bits,&key);uint32_t word=p->pair[key];
        unsigned first=(word>>24)&31,used=(word>>16)&31;
        if((word&(1u<<21)) && n-i>=2 && reader->count>=used && reader->consumed<=reader->limit && used<=reader->limit-reader->consumed){
            if(!reader_drop(reader,used))return 0;output[i++]=(uint8_t)word;output[i++]=(uint8_t)(word>>8);
        }else if(first){
            if(!reader_drop(reader,first))return 0;output[i++]=(uint8_t)word;
        }else{
            unsigned s=0;if(!reader_long_symbol(reader,&p->single,&s))return 0;output[i++]=(uint8_t)s;
        }
    }
    return 1;
}
static void pair_destroy(PairTable *p){qli_free(p->pair?(void*)p->pair:(void*)p->single.entry);}

int qlic_lite_huffman_bytes_decode(
    const uint8_t *input, size_t size, uint8_t *output, size_t output_size,
    QlicLiteByteHuffmanStats *stats_out) {
    Codebook book;
    PairTable table;
    QlicLiteByteHuffmanStats stats = {0};
    uint8_t canonical[MAX_DESCRIPTOR_BYTES];
    size_t canonical_size = 0;
    int okay = 0;
    if (!input || !size || (!output && output_size)) return 0;
    if (input[0] == QLIC_LITE_BYTE_RAW) {
        if (output_size == SIZE_MAX || size != output_size + 1u) return 0;
        if (output_size) memcpy(output, input + 1, output_size);
        stats.header_bytes = 1u;
        stats.payload_bits = (uint64_t)output_size * 8u;
        stats.payload_bytes = output_size;
        stats.form = QLIC_LITE_BYTE_RAW;
        if (stats_out) *stats_out = stats;
        return 1;
    }
    if (input[0] == QLIC_LITE_BYTE_CONSTANT) {
        if (size != 2u || output_size < 2u) return 0;
        memset(output, input[1], output_size);
        stats.header_bytes = 2u;
        stats.form = QLIC_LITE_BYTE_CONSTANT;
        if (stats_out) *stats_out = stats;
        return 1;
    }
    if (input[0] != QLIC_LITE_BYTE_HUFFMAN || size < BYTE_HEADER_BYTES ||
        !output_size || output_size == SIZE_MAX ||
        size >= output_size + 1u)
        return 0;
    unsigned descriptor_size = qli_read_le16(input + 1);
    uint64_t payload_bits = qli_read_le64(input + 3);
    size_t descriptor_offset = BYTE_HEADER_BYTES;
    if (descriptor_size > size - descriptor_offset) return 0;
    size_t payload_offset = descriptor_offset + descriptor_size;
    if (payload_bits > UINT64_MAX - 7u ||
        (payload_bits + 7u) / 8u != size - payload_offset)
        return 0;
    unsigned padding = (unsigned)((8u - (payload_bits & 7u)) & 7u);
    if (padding &&
        (input[size - 1u] & (uint8_t)((1u << padding) - 1u)))
        return 0;
    memset(&book, 0, sizeof(book));
    memset(&table, 0, sizeof(table));
    book.alphabet = SYMBOLS;
    if (!parse_descriptor(&book, input + descriptor_offset,
                          descriptor_size) ||
        !make_descriptor(&book, canonical, &canonical_size) ||
        canonical_size != descriptor_size ||
        memcmp(canonical, input + descriptor_offset, descriptor_size))
        return 0;
    unsigned used = 0;
    for (unsigned symbol = 0; symbol < SYMBOLS; ++symbol)
        used += book.length[symbol] != 0;
    uint32_t kraft = 0;
    for (unsigned symbol = 0; symbol < SYMBOLS; ++symbol)
        if (book.length[symbol])
            kraft += UINT32_C(1) <<
                     (book.max_bits - book.length[symbol]);
    if (used < 2u || kraft != (UINT32_C(1) << book.max_bits) ||
        !pair_build(&book, &table))
        return 0;
    BitReader reader;
    memset(&reader, 0, sizeof(reader));
    reader.next = input + payload_offset;
    reader.end = input + size;
    reader.limit = payload_bits;
    if (!pair_decode(&reader, &table, output, output_size)) goto cleanup_bytes;
    if (reader.consumed != payload_bits) goto cleanup_bytes;
    stats.header_bytes = BYTE_HEADER_BYTES;
    stats.table_bytes = descriptor_size;
    stats.payload_bits = payload_bits;
    stats.payload_bytes = size - payload_offset;
    stats.form = QLIC_LITE_BYTE_HUFFMAN;
    if (stats_out) *stats_out = stats;
    okay = 1;

cleanup_bytes:
    pair_destroy(&table);
    return okay;
}

static int take_icon_record(const uint8_t **cursor, size_t *room, uint8_t *out,
                       size_t cap, size_t *count) {
    const uint8_t *p = *cursor;
    if (*room < ICON_RECORD || p[0] > 1) return 0;
    size_t bytes = qli_read_le32(p+1);
    if (bytes > *room-ICON_RECORD) return 0;
    if (p[0]) {
        if (!qbs_decode(p+ICON_RECORD, bytes, out, cap, count,QBS_NO_TABLE) || bytes >= *count) return 0;
    } else {
        if (bytes > cap) return 0;
        memcpy(out, p+ICON_RECORD, bytes);
        *count = bytes;
    }
    *cursor += ICON_RECORD+bytes;
    *room -= ICON_RECORD+bytes;
    return 1;
}

static int icon_clear_alpha4(const uint8_t *above)
{
    const uint64_t alpha = UINT64_C(0xff000000ff000000);
    return !above || !((qli_read_le64(above) | qli_read_le64(above+8)) & alpha);
}

int qie_decode_owned(const uint8_t *file, size_t bytes, size_t pixel_cap,
                   uint8_t **result, size_t *written) {
    if (result) *result = NULL;
    if (written) *written = 0;
    if (!result || !written || !file || bytes < ICON_MIN_FILE || bytes > ICON_FILE_CAP ||
        memcmp(file,"QIE1",4) || file[4] >= ICON_SIDE || file[5] >= ICON_SIDE || file[6] > 1) return 0;
    unsigned w = (unsigned)file[4]+1, h = (unsigned)file[5]+1;
    size_t pixels = (size_t)w*h;
    int common = file[6];
    if (pixels > pixel_cap || qli_read_le32(file+bytes-4) != stream_crc32(file,bytes-4)) return 0;
    const uint8_t *hidden = file+ICON_HEADER, *cursor = hidden+3*common;
    size_t room = bytes-(size_t)(cursor-file)-4;
    IconStreams stream;
    size_t events, alpha, green, sides;
    if (!take_icon_record(&cursor,&room,stream.events,pixels,&events) || events != pixels ||
        !take_icon_record(&cursor,&room,stream.alpha,pixels,&alpha) ||
        !take_icon_record(&cursor,&room,stream.green,pixels,&green) ||
        !take_icon_record(&cursor,&room,stream.sides,2*pixels,&sides) || room) return 0;
    uint8_t *out = qli_malloc(4*pixels);
    if (!out) return 0;
    size_t ia = 0, ig = 0, is = 0;
    int used_hidden = 0;
    const size_t stride = 4*(size_t)w;
    uint32_t fill = common ? (qli_read_le32(hidden) & UINT32_C(0x00ffffff)) : 0;
    const uint64_t repeated = (uint64_t)fill * UINT64_C(0x100000001);
    for (unsigned y = 0; y < h; ++y) {
        for (unsigned x = 0; x < w; ++x) {
            size_t i = (size_t)y*w+x;
            uint8_t *p = out+4*i;
            const uint8_t *left = x ? p-4 : NULL, *above = y ? p-stride : NULL;
            const uint8_t *diag = x && y ? p-stride-4 : NULL;
            if (common && x%4 == 0 && w-x >= 4 && qli_read_le32(stream.events+i) == 0 &&
                (!left || !left[3]) && icon_clear_alpha4(above)) {
                qli_write_le64(p,repeated);
                qli_write_le64(p+8,repeated);
                used_hidden = 1;
                x += 3;
                continue;
            }
            unsigned code = stream.events[i];
            if (code >= 24) goto failed;
            unsigned cls = (code>>3)+icon_gradient(icon_alpha_class(left),icon_alpha_class(above),icon_alpha_class(diag));
            if (cls >= 3) cls -= 3;
            if (cls == 1) {
                if (ia == alpha) goto failed;
                p[3] = (uint8_t)(icon_gradient(icon_alpha_value(left),icon_alpha_value(above),icon_alpha_value(diag))+stream.alpha[ia++]);
                if (!p[3] || p[3] == 255) goto failed;
            } else p[3] = cls ? 255 : 0;
            if (!cls && common) {
                if (code & 7) goto failed;
                memcpy(p,hidden,3);
                used_hidden = 1;
                continue;
            }
            left = icon_visible(left,common);
            above = icon_visible(above,common);
            diag = icon_visible(diag,common);
            uint8_t ge = 0, re = 0, be = 0;
            if ((code & 1) && (ig == green || !(ge = stream.green[ig++]))) goto failed;
            if ((code & 2) && (is == sides || !(re = stream.sides[is++]))) goto failed;
            if ((code & 4) && (is == sides || !(be = stream.sides[is++]))) goto failed;
            int correction = ge < 128 ? ge : ge-256;
            p[0] = (uint8_t)(icon_clamp(icon_color(0,left,above,diag)+correction,0,255)+re);
            p[1] = (uint8_t)(icon_color(1,left,above,diag)+ge);
            p[2] = (uint8_t)(icon_clamp(icon_color(2,left,above,diag)+correction,0,255)+be);
        }
    }
    if (ia != alpha || ig != green || is != sides || used_hidden != common ||
        qli_read_le32(file+7) != stream_crc32(out,4*pixels)) goto failed;
    *result = out;
    *written = 4*pixels;
    return 1;
failed:
    qli_free(out);
    return 0;
}

enum {
    HASH_BITS = 16,
    HASH_SIZE = 1 << HASH_BITS,
    LITERAL_LIMIT = 128,
    MATCH_MINIMUM = 4,
    MATCH_DIRECT_LIMIT = MATCH_MINIMUM + 122,
    MATCH_EXTENDED_MINIMUM = MATCH_DIRECT_LIMIT + 1,
    SHORT_OFFSET_LIMIT = 65535,
    TAG_STRIDE_BASE = 251,
    PREFERRED_OFFSETS = 4,
    PROBED_OFFSETS = 4,
    TAG_EXTENDED = 255
};

static unsigned match_hash(const uint8_t *input) {
    return (unsigned)((qli_read_le32(input) * UINT32_C(2654435761)) >>
                      (32u - HASH_BITS));
}

static size_t uleb_size(size_t value) {
    size_t bytes = 1;
    while (value >= 128u) {
        value >>= 7;
        ++bytes;
    }
    return bytes;
}

static uint8_t *write_uleb(uint8_t *output, size_t value) {
    do {
        uint8_t byte = (uint8_t)(value & 127u);
        value >>= 7;
        *output++ = (uint8_t)(byte | (value ? 128u : 0u));
    } while (value);
    return output;
}

static int read_uleb(const uint8_t **input, const uint8_t *end,
                     size_t *value_out) {
    const uint8_t *start = *input;
    size_t value = 0;
    unsigned shift = 0;
    while (*input < end && shift < sizeof(size_t) * CHAR_BIT) {
        uint8_t byte = *(*input)++;
        size_t part = byte & 127u;
        if (part > (SIZE_MAX >> shift)) return 0;
        value |= part << shift;
        if (!(byte & 128u)) {
            if ((size_t)(*input - start) != uleb_size(value)) return 0;
            *value_out = value;
            return 1;
        }
        shift += 7;
    }
    return 0;
}

static int qlic_lite_match_bound(size_t input_size, size_t *bound_out) {
    if (!bound_out || input_size > SIZE_MAX - 127u) return 0;
    size_t runs = (input_size + 127u) / LITERAL_LIMIT;
    if (runs > SIZE_MAX - input_size) return 0;
    *bound_out = input_size + runs;
    return 1;
}


static int write_literals(const uint8_t *input, size_t size,
                          uint8_t **output, const uint8_t *end,
                          uint8_t **literals, const uint8_t *literal_end) {
    while (size) {
        size_t chunk = size > LITERAL_LIMIT ? LITERAL_LIMIT : size;
        if (*output == end || (size_t)(literal_end - *literals) < chunk)
            return 0;
        *(*output)++ = (uint8_t)(chunk - 1u);
        memcpy(*literals, input, chunk);
        *literals += chunk;
        input += chunk;
        size -= chunk;
    }
    return 1;
}

static size_t match_token_size(size_t length, size_t offset,
                               unsigned preferred) {
    if (preferred) return 1u + uleb_size(length - MATCH_MINIMUM);
    size_t bytes = length > MATCH_DIRECT_LIMIT
                       ? 3u + uleb_size(length - MATCH_EXTENDED_MINIMUM)
                       : 3u;
    if (offset > SHORT_OFFSET_LIMIT)
        bytes += uleb_size(offset - (SHORT_OFFSET_LIMIT + 1u));
    return bytes;
}

static int write_match(size_t length, size_t offset, unsigned preferred,
                       uint8_t **output, const uint8_t *end) {
    if (offset < 1u ||
        (size_t)(end - *output) <
            match_token_size(length, offset, preferred))
        return 0;
    if (preferred) {
        *(*output)++ = (uint8_t)(TAG_STRIDE_BASE + preferred - 1u);
        *output = write_uleb(*output, length - MATCH_MINIMUM);
    } else if (length <= MATCH_DIRECT_LIMIT) {
        *(*output)++ = (uint8_t)(128u + length - MATCH_MINIMUM);
    } else {
        *(*output)++ = TAG_EXTENDED;
        *output = write_uleb(*output, length - MATCH_EXTENDED_MINIMUM);
    }
    if (!preferred) {
        unsigned code = offset > SHORT_OFFSET_LIMIT
                            ? UINT16_MAX
                            : (unsigned)(offset - 1u);
        *(*output)++ = (uint8_t)code;
        *(*output)++ = (uint8_t)(code >> 8);
        if (offset > SHORT_OFFSET_LIMIT)
            *output = write_uleb(
                *output, offset - (SHORT_OFFSET_LIMIT + 1u));
    }
    return 1;
}

static size_t find_match(const uint8_t *input, size_t size, size_t position,
                         size_t candidate) {
    if (candidate >= position ||
        memcmp(input + candidate, input + position, MATCH_MINIMUM))
        return 0;
    size_t length = MATCH_MINIMUM;
    while (length < size - position &&
           input[candidate + length] == input[position + length])
        ++length;
    return length;
}

static void consider_match(size_t length, size_t offset, unsigned preferred,
                           size_t *best_length, size_t *best_offset,
                           unsigned *best_preferred, size_t *best_gain) {
    if (!length) return;
    size_t bytes = match_token_size(length, offset, preferred);
    if (bytes >= length) return;
    size_t gain = length - bytes;
    if (gain > *best_gain ||
        (gain == *best_gain && length > *best_length)) {
        *best_length = length;
        *best_offset = offset;
        *best_preferred = preferred;
        *best_gain = gain;
    }
}

static size_t match_bucket(const uint8_t *input, size_t position,
                           const size_t *heads, unsigned slots)
{
    const unsigned hash = match_hash(input + position);
    if (slots == HASH_SIZE) return hash;
    const unsigned mask = slots - 1;
    unsigned slot = hash & mask;
    size_t head = heads[slot];
    while (head != SIZE_MAX && match_hash(input + head) != hash) {
        slot = (slot + 1) & mask;
        head = heads[slot];
    }
    return slot;
}

static int match_encode_split(
    const uint8_t *input, size_t input_size, size_t stride,
    uint8_t *output, size_t capacity, size_t *size_out, uint8_t *literals,
    size_t literal_capacity, size_t *literal_size_out) {
    size_t *table = NULL;
    uint8_t *next = output;
    const uint8_t *end = output ? output + capacity : NULL;
    uint8_t *literal_next = literals;
    const uint8_t *literal_end =
        literals ? literals + literal_capacity : NULL;
    if (!size_out || !literal_size_out || (input_size && (!input || !output || !literals)) ||
        stride > SHORT_OFFSET_LIMIT) return 0;
    *size_out = 0;
    *literal_size_out = 0;
    if (!input_size) return 1;
    unsigned slots = HASH_SIZE;
    if (input_size <= 2048) {
        slots = 8;
        while (slots < 2 * input_size) slots <<= 1;
    }
    table = qli_malloc(slots * sizeof(*table));
    if (!table) return 0;
    for (size_t slot = 0; slot < slots; ++slot) table[slot] = SIZE_MAX;

    size_t anchor = 0;
    size_t position = 0;
    while (position + MATCH_MINIMUM <= input_size) {
        size_t base = match_bucket(input, position, table, slots);
        size_t candidate = table[base];
        table[base] = position;
        size_t best_length = 0;
        size_t best_offset = 0;
        size_t best_gain = 0;
        unsigned best_preferred = 0;
        if (candidate != SIZE_MAX) {
            size_t length =
                find_match(input, input_size, position, candidate);
            size_t offset = position - candidate;
            unsigned preferred = 0;
            if (stride && offset <= SHORT_OFFSET_LIMIT && !(offset % stride) &&
                offset / stride <= PREFERRED_OFFSETS)
                preferred = (unsigned)(offset / stride);
            consider_match(length, offset, preferred, &best_length,
                           &best_offset, &best_preferred, &best_gain);
        }
        for (unsigned preferred = 1; preferred <= PROBED_OFFSETS;
             ++preferred) {
            if (!stride || stride > SHORT_OFFSET_LIMIT / preferred) break;
            size_t offset = stride * preferred;
            if (position < offset) continue;
            size_t length = find_match(input, input_size, position,
                                       position - offset);
            consider_match(length, offset, preferred, &best_length,
                           &best_offset, &best_preferred, &best_gain);
        }
        if (!best_length) {
            ++position;
            continue;
        }
        if (!write_literals(input + anchor, position - anchor, &next, end,
                            &literal_next, literal_end) ||
            !write_match(best_length, best_offset, best_preferred, &next, end)) {
            qli_free(table);
            return 0;
        }
        position += best_length;
        anchor = position;
        size_t update = position > 3u ? position - 3u : 0u;
        while (update < position && update + MATCH_MINIMUM <= input_size) {
            size_t update_base = match_bucket(input, update, table, slots);
            table[update_base] = update;
            ++update;
        }
    }
    int okay = write_literals(input + anchor, input_size - anchor, &next, end,
                              &literal_next, literal_end);
    qli_free(table);
    if (!okay) return 0;
    *size_out = (size_t)(next - output);
    *literal_size_out = (size_t)(literal_next - literals);
    return 1;
}


static int match_decode_split(
    const uint8_t *input, size_t input_size, size_t stride, uint8_t *output,
    size_t output_size, const uint8_t *literals, size_t literal_size) {
    const uint8_t *next = input;
    const uint8_t *end = input ? input + input_size : NULL;
    const uint8_t *literal_next = literals;
    const uint8_t *literal_end =
        literals ? literals + literal_size : NULL;
    size_t written = 0;
    if ((!input && input_size) || (!output && output_size) ||
        (!literals && literal_size) ||
        stride > SHORT_OFFSET_LIMIT)
        return 0;
    while (next < end && written < output_size) {
        unsigned tag = *next++;
        if (tag < 128u) {
            size_t length = tag + 1u;
            if (length > output_size - written ||
                !literal_next || length > (size_t)(literal_end - literal_next)) return 0;
            memcpy(output + written, literal_next, length);
            literal_next += length;
            written += length;
            continue;
        }
        size_t length;
        unsigned preferred = 0;
        if (tag < TAG_STRIDE_BASE) {
            length = tag - 128u + MATCH_MINIMUM;
        } else if (tag < TAG_EXTENDED) {
            size_t extra = 0;
            if (!stride || !read_uleb(&next, end, &extra) ||
                extra > SIZE_MAX - MATCH_MINIMUM)
                return 0;
            preferred = tag - TAG_STRIDE_BASE + 1u;
            if (stride > SHORT_OFFSET_LIMIT / preferred) return 0;
            length = extra + MATCH_MINIMUM;
        } else {
            size_t extra = 0;
            if (!read_uleb(&next, end, &extra) ||
                extra > SIZE_MAX - MATCH_EXTENDED_MINIMUM)
                return 0;
            length = extra + MATCH_EXTENDED_MINIMUM;
        }
        size_t offset;
        if (preferred) {
            offset = stride * preferred;
        } else {
            if ((size_t)(end - next) < 2u) return 0;
            offset = (size_t)next[0] | (size_t)next[1] << 8;
            next += 2;
            if (offset == UINT16_MAX) {
                size_t extra = 0;
                if (!read_uleb(&next, end, &extra) ||
                    extra > SIZE_MAX - (SHORT_OFFSET_LIMIT + 1u))
                    return 0;
                offset = extra + SHORT_OFFSET_LIMIT + 1u;
            } else {
                ++offset;
            }
        }
        if (length > output_size - written) return 0;
        if (offset > written) return 0;
        size_t source = written - offset;
        for (size_t byte = 0; byte < length; ++byte)
            output[written + byte] = output[source + byte];
        written += length;
    }
    return next == end && written == output_size && literal_next == literal_end;
}

static inline unsigned qlic_count_bits(uint32_t bits)
{
#if defined(__POPCNT__) && (defined(__clang__) || defined(__GNUC__))
    return (unsigned)__builtin_popcount(bits);
#else
    bits -= (bits >> 1) & UINT32_C(0x55555555);
    bits = (bits & UINT32_C(0x33333333)) + ((bits >> 2) & UINT32_C(0x33333333));
    bits = (bits + (bits >> 4)) & UINT32_C(0x0f0f0f0f);
    return (bits * UINT32_C(0x01010101)) >> 24;
#endif
}

static inline unsigned scalar_predict(int mode, int w, int n, int nw, int ne, int limit) {
    switch (mode) {
    case 0: {
        int low = w < n ? w : n;
        int high = w > n ? w : n;
        int p = w + n - nw;
        return (unsigned)(p < low ? low : p > high ? high : p);
    }
    case 1: return (unsigned)w;
    case 2: return (unsigned)n;
    case 3: return (unsigned)(w + n + nw + ne + 2) >> 2;
    case 4: return (unsigned)(3*w + n + 2) >> 2;
    case 5: return (unsigned)(w + n + ne + 1) / 3;
    case 6: return (unsigned)(2*w + n + ne + 2) >> 2;
    default: {
        int p = (3*w + 3*n - 2*nw + 2) >> 2;
        return (unsigned)(p < 0 ? 0 : p > limit ? limit : p);
    }
    }
}

#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42 && (defined(__GNUC__) || defined(__clang__))
#define QLI_NUMERIC_SSE42 1
#if QLI_MAX_CPU_TIER >= QLI_TIER_AVX2
#define QLI_NUMERIC_AVX2 1
#define QLI_NUMERIC_TARGET __attribute__((target("avx2,popcnt")))
#define QLI_BAND_CRC_TARGET __attribute__((target("avx2,popcnt,pclmul")))
#define QLI_NUMERIC_FN(tier, name) ((tier) == QLI_TIER_AVX2 ? (name) : (sse42_##name))
#else
#define QLI_NUMERIC_FN(tier, name) (sse42_##name)
#endif

static inline int qli_numeric_tier(unsigned caps)
{
#ifdef QLI_NUMERIC_AVX2
    if (caps & QLI_CPU_AVX2) return QLI_TIER_AVX2;
#endif
    return (caps & QLI_CPU_SSE42) ? QLI_TIER_SSE42 : QLI_TIER_SCALAR;
}

#define QLI_SSE42_CRC_TARGET __attribute__((target("sse4.2,popcnt,pclmul,no-avx,no-avx2")))

#define QLI_SSE42_TARGET __attribute__((target("sse4.2,popcnt,no-avx,no-avx2")))
#define SSE_INLINE static QLI_SSE42_TARGET QLI_INLINE

typedef struct { __m128i lo, hi; } SsePair;

#define SSE_UNARY(op) \
    SSE_INLINE SsePair sse_##op(SsePair v) { \
        return (SsePair){_mm_##op(v.lo), _mm_##op(v.hi)}; \
    }
#define SSE_BINARY(op) \
    SSE_INLINE SsePair sse_##op(SsePair a, SsePair b) { \
        return (SsePair){_mm_##op(a.lo,b.lo), _mm_##op(a.hi,b.hi)}; \
    }
#define SSE_SHIFT(op) \
    SSE_INLINE SsePair sse_##op(SsePair a, unsigned shift) { \
        return (SsePair){_mm_##op(a.lo,shift), _mm_##op(a.hi,shift)}; \
    }
#define SSE_BROADCAST(op, type) \
    SSE_INLINE SsePair sse_##op(type v) { \
        const __m128i both = _mm_##op(v); \
        return (SsePair){both,both}; \
    }

SSE_UNARY(abs_epi8)
SSE_BINARY(add_epi8)
SSE_BINARY(add_epi16)
SSE_BINARY(add_epi32)
SSE_BINARY(add_epi64)
SSE_BINARY(sub_epi8)
SSE_BINARY(sub_epi16)
SSE_BINARY(sub_epi32)
SSE_BINARY(and_si128)
SSE_BINARY(andnot_si128)
SSE_BINARY(or_si128)
SSE_BINARY(xor_si128)
SSE_BINARY(cmpeq_epi8)
SSE_BINARY(cmpeq_epi16)
SSE_BINARY(cmpeq_epi32)
SSE_BINARY(cmpgt_epi16)
SSE_BINARY(cmpgt_epi32)
SSE_BINARY(min_epu8)
SSE_BINARY(max_epu8)
SSE_BINARY(min_epi16)
SSE_BINARY(max_epi16)
SSE_BINARY(min_epu16)
SSE_BINARY(max_epu16)
SSE_BINARY(min_epi32)
SSE_BINARY(mul_epu32)
SSE_BINARY(mulhi_epu16)
SSE_BINARY(mullo_epi32)
SSE_BINARY(avg_epu16)
SSE_BINARY(maddubs_epi16)
SSE_BINARY(packs_epi32)
SSE_BINARY(packus_epi16)
SSE_BINARY(sad_epu8)
SSE_BINARY(shuffle_epi8)
SSE_SHIFT(slli_epi16)
SSE_SHIFT(srai_epi16)
SSE_SHIFT(srli_epi16)
SSE_SHIFT(slli_epi32)
SSE_SHIFT(srai_epi32)
SSE_SHIFT(srli_epi32)
SSE_SHIFT(srli_epi64)
SSE_BROADCAST(set1_epi16, short)
SSE_BROADCAST(set1_epi32, int)
SSE_BROADCAST(set1_epi64x, int64_t)
#undef SSE_UNARY
#undef SSE_BINARY
#undef SSE_SHIFT
#undef SSE_BROADCAST

SSE_INLINE SsePair sse_setzero_si256(void)
{
    __m128i zero = _mm_setzero_si128();
    return (SsePair){zero, zero};
}

SSE_INLINE SsePair sse_set_m128i(__m128i hi,__m128i lo) { return (SsePair){lo,hi}; }
SSE_INLINE __m128i sse_extracti128_si256(SsePair x,unsigned half) { return half ? x.hi : x.lo; }
SSE_INLINE __m128i sse_castsi256_si128(SsePair x) { return x.lo; }

SSE_INLINE SsePair sse_cvtepu8_epi32(__m128i value)
{
    return (SsePair){_mm_cvtepu8_epi32(value),_mm_cvtepu8_epi32(_mm_srli_si128(value,4))};
}

SSE_INLINE SsePair sse_cvtepu16_epi32(__m128i value)
{
    return (SsePair){_mm_cvtepu16_epi32(value),_mm_cvtepu16_epi32(_mm_srli_si128(value,8))};
}

SSE_INLINE SsePair sse_cvtepi16_epi32(__m128i value)
{
    return (SsePair){_mm_cvtepi16_epi32(value),_mm_cvtepi16_epi32(_mm_srli_si128(value,8))};
}

SSE_INLINE SsePair sse_cvtepi32_epi64(__m128i value)
{
    return (SsePair){_mm_cvtepi32_epi64(value),_mm_cvtepi32_epi64(_mm_srli_si128(value,8))};
}

SSE_INLINE SsePair sse_cvtepi8_epi16(__m128i value)
{
    return (SsePair){_mm_cvtepi8_epi16(value),_mm_cvtepi8_epi16(_mm_srli_si128(value,8))};
}

SSE_INLINE SsePair sse_cvtepu8_epi16(__m128i value)
{
    return (SsePair){_mm_cvtepu8_epi16(value),_mm_cvtepu8_epi16(_mm_srli_si128(value,8))};
}

SSE_INLINE SsePair sse_cvtepi32_ps(SsePair value)
{
    return (SsePair){_mm_castps_si128(_mm_cvtepi32_ps(value.lo)),
                     _mm_castps_si128(_mm_cvtepi32_ps(value.hi))};
}

#define sse_castps_si256(value) (value)
#define sse_castsi256_ps(value) (value)

SSE_INLINE SsePair sse_blendv_epi8(SsePair a,SsePair b,SsePair mask)
{
    return (SsePair){_mm_blendv_epi8(a.lo,b.lo,mask.lo),_mm_blendv_epi8(a.hi,b.hi,mask.hi)};
}

SSE_INLINE unsigned sse_movemask_ps(SsePair value)
{
    return (unsigned)_mm_movemask_ps(_mm_castsi128_ps(value.lo)) |
           ((unsigned)_mm_movemask_ps(_mm_castsi128_ps(value.hi)) << 4);
}

SSE_INLINE int sse_movemask_epi8(SsePair value)
{
    const uint32_t mask=(unsigned)_mm_movemask_epi8(value.lo) |
                        ((unsigned)_mm_movemask_epi8(value.hi) << 16);
    return (int)mask;
}

SSE_INLINE int sse_testz_si256(SsePair a,SsePair b)
{
    __m128i bits=_mm_or_si128(_mm_and_si128(a.lo,b.lo),_mm_and_si128(a.hi,b.hi));
    return _mm_testz_si128(bits,bits);
}

SSE_INLINE __m128i sse_direction_shift4(__m128i packed,__m128i shifts)
{
    __m128i shifted=_mm_srli_epi32(packed,6);
    __m128i out=_mm_blendv_epi8(packed,shifted,_mm_cmpgt_epi32(shifts,_mm_setzero_si128()));
    return _mm_blendv_epi8(out,_mm_srli_epi32(packed,12),_mm_cmpgt_epi32(shifts,_mm_set1_epi32(6)));
}

SSE_INLINE SsePair sse_srlv_epi32(SsePair packed,SsePair shifts)
{
    return (SsePair){sse_direction_shift4(packed.lo,shifts.lo),sse_direction_shift4(packed.hi,shifts.hi)};
}

SSE_INLINE SsePair sse_permute4x64_epi64(SsePair value,unsigned order)
{
    assert(order==0xd8);
    (void)order;
    return (SsePair){_mm_unpacklo_epi64(value.lo,value.hi),_mm_unpackhi_epi64(value.lo,value.hi)};
}

SSE_INLINE SsePair sse_permute2x128_si256(SsePair a,SsePair b,unsigned order)
{
    assert(order==8);
    (void)b;
    (void)order;
    return (SsePair){_mm_setzero_si128(),a.lo};
}

SSE_INLINE SsePair sse_alignr_epi8(SsePair a,SsePair b,unsigned count)
{
    assert(count==14);
    (void)count;
    return (SsePair){_mm_alignr_epi8(a.lo,b.lo,14),_mm_alignr_epi8(a.hi,b.hi,14)};
}

SSE_INLINE SsePair sse_srli_si256(SsePair a,unsigned count)
{
    assert(count==8);
    (void)count;
    return (SsePair){_mm_srli_si128(a.lo,8),_mm_srli_si128(a.hi,8)};
}

SSE_INLINE SsePair sse_insert_epi16(SsePair a,int value,unsigned lane)
{
    assert(lane==0);
    (void)lane;
    a.lo=_mm_insert_epi16(a.lo,value,0);
    return a;
}

#define sse_extract_epi16(value,lane) \
    _mm_extract_epi16((lane) < 8 ? (value).lo : (value).hi, (lane) & 7)

#define sse_setr_epi16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) \
    ((SsePair){_mm_setr_epi16(a,b,c,d,e,f,g,h),_mm_setr_epi16(i,j,k,l,m,n,o,p)})

SSE_INLINE __m128i sse_gather4(__m128i value,const uint32_t *table,__m128i address,__m128i live)
{
    const unsigned mask=(unsigned)_mm_movemask_ps(_mm_castsi128_ps(live));
    uint32_t indices[4], words[4];
    memcpy(indices,&address,sizeof(indices));
    if (mask==15) {
        for (unsigned i=0;i<4;++i) words[i]=table[indices[i]];
    } else {
        memcpy(words,&value,sizeof(words));
        for (unsigned i=0;i<4;++i) if (mask & (1u<<i)) words[i]=table[indices[i]];
    }
    memcpy(&value,words,sizeof(value));
    return value;
}

SSE_INLINE SsePair sse_mask_i32gather_epi32(SsePair value,const int32_t *table,
                                            SsePair address,SsePair live,unsigned stride)
{
    assert(stride==4);
    (void)stride;
    return (SsePair){sse_gather4(value.lo,(const uint32_t *)table,address.lo,live.lo),
                     sse_gather4(value.hi,(const uint32_t *)table,address.hi,live.hi)};
}

SSE_INLINE SsePair sse_mask_i32gather_epi64(SsePair value,const long long *table,
                                            __m128i address,SsePair live,unsigned stride)
{
    assert(stride==8);
    (void)stride;
    const unsigned mask=(unsigned)_mm_movemask_pd(_mm_castsi128_pd(live.lo)) |
                        ((unsigned)_mm_movemask_pd(_mm_castsi128_pd(live.hi))<<2);
    uint32_t indices[4];
    uint64_t words[4];
    memcpy(indices,&address,sizeof(indices));
    if (mask==15) {
        for (unsigned i=0;i<4;++i) memcpy(words+i,table+indices[i],8);
    } else {
        memcpy(words,&value,sizeof(words));
        for (unsigned i=0;i<4;++i) if (mask & (1u<<i)) memcpy(words+i,table+indices[i],8);
    }
    memcpy(&value,words,sizeof(value));
    return value;
}

SSE_INLINE SsePair sse_i32gather_epi64(const long long *table,__m128i address,unsigned stride)
{
    assert(stride==8);
    (void)stride;
    uint32_t indices[4];
    uint64_t words[4];
    memcpy(indices,&address,sizeof(indices));
    for (unsigned i=0;i<4;++i) memcpy(words+i,table+indices[i],8);
    SsePair out;
    memcpy(&out,words,sizeof(out));
    return out;
}

#undef SSE_INLINE
#endif

#ifdef QLI_NUMERIC_AVX2


static QLI_NUMERIC_TARGET inline __m256i vector_predict(int mode, __m256i w, __m256i n,
                                    __m256i nw, __m256i ne, __m256i limit) {
    const __m256i two = _mm256_set1_epi16(2);
    __m256i wn = _mm256_add_epi16(w, n);
    switch (mode) {
    case 0: {
        __m256i p = _mm256_sub_epi16(wn, nw);
        return _mm256_min_epi16(_mm256_max_epi16(p, _mm256_min_epi16(w, n)),
                               _mm256_max_epi16(w, n));
    }
    case 1: return w;
    case 2: return n;
    case 3:
        return _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(wn, two),
                                                  _mm256_add_epi16(nw, ne)), 2);
    case 4:
        return _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(wn, two),
                                                  _mm256_slli_epi16(w, 1)), 2);
    case 5:
        return _mm256_mulhi_epu16(_mm256_add_epi16(_mm256_add_epi16(wn, ne),
                                                   _mm256_set1_epi16(1)),
                                 _mm256_set1_epi16(21846));
    case 6:
        return _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(wn, w),
                                                  _mm256_add_epi16(ne, two)), 2);
    default: {
        __m256i p = _mm256_add_epi16(wn, _mm256_slli_epi16(wn, 1));
        p = _mm256_add_epi16(_mm256_sub_epi16(p, _mm256_slli_epi16(nw, 1)), two);
        return _mm256_min_epi16(_mm256_max_epi16(_mm256_srai_epi16(p, 2),
                                               _mm256_setzero_si256()), limit);
    }
    }
}

#endif
#ifdef QLI_NUMERIC_SSE42


static QLI_SSE42_TARGET inline SsePair sse42_vector_predict(int mode, SsePair w, SsePair n,
                                    SsePair nw, SsePair ne, SsePair limit) {
    const SsePair two = sse_set1_epi16(2);
    SsePair wn = sse_add_epi16(w, n);
    switch (mode) {
    case 0: {
        SsePair p = sse_sub_epi16(wn, nw);
        return sse_min_epi16(sse_max_epi16(p, sse_min_epi16(w, n)),
                               sse_max_epi16(w, n));
    }
    case 1: return w;
    case 2: return n;
    case 3:
        return sse_srli_epi16(sse_add_epi16(sse_add_epi16(wn, two),
                                                  sse_add_epi16(nw, ne)), 2);
    case 4:
        return sse_srli_epi16(sse_add_epi16(sse_add_epi16(wn, two),
                                                  sse_slli_epi16(w, 1)), 2);
    case 5:
        return sse_mulhi_epu16(sse_add_epi16(sse_add_epi16(wn, ne),
                                                   sse_set1_epi16(1)),
                                 sse_set1_epi16(21846));
    case 6:
        return sse_srli_epi16(sse_add_epi16(sse_add_epi16(wn, w),
                                                  sse_add_epi16(ne, two)), 2);
    default: {
        SsePair p = sse_add_epi16(wn, sse_slli_epi16(wn, 1));
        p = sse_add_epi16(sse_sub_epi16(p, sse_slli_epi16(nw, 1)), two);
        return sse_min_epi16(sse_max_epi16(sse_srai_epi16(p, 2),
                                               sse_setzero_si256()), limit);
    }
    }
}

#endif

#ifdef QLI_NUMERIC_AVX2
typedef __m256i NumericalLanes;
static QLI_NUMERIC_TARGET inline __m128i numerical_lo(NumericalLanes v) { return _mm256_castsi256_si128(v); }
static QLI_NUMERIC_TARGET inline __m128i numerical_hi(NumericalLanes v) { return _mm256_extracti128_si256(v,1); }
static QLI_NUMERIC_TARGET inline NumericalLanes numerical_pair(__m128i hi,__m128i lo) { return _mm256_set_m128i(hi,lo); }

static QLI_NUMERIC_TARGET inline NumericalLanes numerical_load32(const void *source)
{
    NumericalLanes value;
    memcpy(&value, source, sizeof(value));
    return value;
}

static QLI_NUMERIC_TARGET inline void numerical_store32(void *destination, NumericalLanes value)
{
    memcpy(destination, &value, sizeof(value));
}

static QLI_NUMERIC_TARGET inline __m128i numerical_load16(const void *source)
{
    __m128i value;
    memcpy(&value, source, sizeof(value));
    return value;
}

static QLI_NUMERIC_TARGET inline void numerical_store16(void *destination, __m128i value)
{
    memcpy(destination, &value, sizeof(value));
}

static QLI_NUMERIC_TARGET inline void numerical_store8(void *destination, __m128i value)
{
    const int64_t word = _mm_cvtsi128_si64(value);
    memcpy(destination, &word, sizeof(word));
}
#endif
#ifdef QLI_NUMERIC_SSE42
typedef SsePair sse42_NumericalLanes;
static QLI_SSE42_TARGET inline __m128i sse42_numerical_lo(sse42_NumericalLanes v) { return sse_castsi256_si128(v); }
static QLI_SSE42_TARGET inline __m128i sse42_numerical_hi(sse42_NumericalLanes v) { return sse_extracti128_si256(v,1); }
static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_numerical_pair(__m128i hi,__m128i lo) { return sse_set_m128i(hi,lo); }

static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_numerical_load32(const void *source)
{
    sse42_NumericalLanes value;
    memcpy(&value, source, sizeof(value));
    return value;
}

static QLI_SSE42_TARGET inline void sse42_numerical_store32(void *destination, sse42_NumericalLanes value)
{
    memcpy(destination, &value, sizeof(value));
}

static QLI_SSE42_TARGET inline __m128i sse42_numerical_load16(const void *source)
{
    __m128i value;
    memcpy(&value, source, sizeof(value));
    return value;
}

static QLI_SSE42_TARGET inline void sse42_numerical_store16(void *destination, __m128i value)
{
    memcpy(destination, &value, sizeof(value));
}

static QLI_SSE42_TARGET inline void sse42_numerical_store8(void *destination, __m128i value)
{
    const int64_t word = _mm_cvtsi128_si64(value);
    memcpy(destination, &word, sizeof(word));
}
#endif

#ifdef QLI_NUMERIC_SSE42

typedef struct { __m128i p0, p1, p2, p3; } NumericPixels;

static QLI_INLINE NumericPixels
numeric_pixels16(const uint8_t *base, const uint8_t *delta1, const uint8_t *delta2,
                 __m128i alpha, unsigned basis)
{
    const __m128i bias = _mm_set1_epi8(-128);
    __m128i first, d1, d2;
    memcpy(&first, base, sizeof(first));
    memcpy(&d1, delta1, sizeof(d1));
    memcpy(&d2, delta2, sizeof(d2));
    d1 = _mm_sub_epi8(d1, bias);
    d2 = _mm_sub_epi8(d2, bias);
    __m128i g = basis ? _mm_add_epi8(first, d1) : first;
    __m128i r = basis == 1 ? first : _mm_add_epi8(g, basis == 2 ? d2 : d1);
    __m128i b = basis == 2 ? first : _mm_add_epi8(g, d2);
    if (basis==3 || basis==4) {
        const __m128i zero=_mm_setzero_si128();
        g=_mm_add_epi8(_mm_avg_epu8(first,zero),d1);
        __m128i other=_mm_add_epi8(_mm_avg_epu8(g,zero),d2);
        r=basis==3 ? first : other;
        b=basis==4 ? first : other;
    } else if (basis==5) {
        const __m128i half=_mm_avg_epu8(_mm_xor_si128(d1,bias),_mm_setzero_si128());
        r=_mm_sub_epi8(_mm_add_epi8(first,_mm_set1_epi8(64)),half);
        g=_mm_add_epi8(r,d1);
        b=_mm_add_epi8(first,d2);
    } else if (basis==6) {
        g=first;
        r=_mm_add_epi8(g,d1);
        b=_mm_add_epi8(_mm_avg_epu8(g,r),d2);
    }
    __m128i rg0 = _mm_unpacklo_epi8(r, g), rg1 = _mm_unpackhi_epi8(r, g);
    __m128i ba0 = _mm_unpacklo_epi8(b, alpha), ba1 = _mm_unpackhi_epi8(b, alpha);
    return (NumericPixels){
        _mm_unpacklo_epi16(rg0, ba0), _mm_unpackhi_epi16(rg0, ba0),
        _mm_unpacklo_epi16(rg1, ba1), _mm_unpackhi_epi16(rg1, ba1),
    };
}

static QLI_INLINE void store_numeric_pixels16(uint8_t *out, NumericPixels pixels)
{
    memcpy(out, &pixels.p0, 16);
    memcpy(out + 16, &pixels.p1, 16);
    memcpy(out + 32, &pixels.p2, 16);
    memcpy(out + 48, &pixels.p3, 16);
}
#endif

static QLI_INLINE void
numeric_color_tail(const uint8_t *planes, const uint8_t *alpha, uint8_t fixed_alpha,
                   size_t n, size_t i, uint8_t *out, unsigned basis)
{
    const uint8_t *delta1 = planes + n, *delta2 = delta1 + n;
    for (; i < n; ++i) {
        uint8_t first = planes[i];
        uint8_t g = basis ? (uint8_t)(first + delta1[i] - 128u) : first;
        out[4*i] = basis == 1 ? first : (uint8_t)(g + (basis == 2 ? delta2[i] : delta1[i]) - 128u);
        out[4*i + 1] = g;
        out[4*i + 2] = basis == 2 ? first : (uint8_t)(g + delta2[i] - 128u);
        out[4*i + 3] = alpha ? alpha[i] : fixed_alpha;
    }
}

static QLI_INLINE void
interleave_numeric_color(const uint8_t *planes, size_t n, uint8_t *out, unsigned basis)
{
    const uint8_t *alpha = planes + 3*n;
    size_t i = 0;
#ifdef QLI_NUMERIC_SSE42
    for (; n - i >= 16; i += 16) {
        __m128i a;
        memcpy(&a, alpha + i, sizeof(a));
        NumericPixels pixels = numeric_pixels16(planes + i, planes + n + i, planes + 2*n + i,
                                                 a, basis);
        store_numeric_pixels16(out + 4*i, pixels);
    }
#endif
    numeric_color_tail(planes, alpha, 0, n, i, out, basis);
}

static void color_interleave(const uint8_t *planes,size_t n,uint8_t *out,unsigned basis)
{
    switch (basis) {
    case 1: interleave_numeric_color(planes, n, out, 1); break;
    case 2: interleave_numeric_color(planes, n, out, 2); break;
    default: interleave_numeric_color(planes, n, out, 0); break;
    }
}

#ifdef QLI_NUMERIC_AVX2
typedef struct { __m128i a, b, c; } NumericRgb;

static QLI_NUMERIC_TARGET QLI_INLINE NumericRgb numeric_rgb16(const uint8_t *planes, size_t n,
                                           size_t i, unsigned basis)
{
    const __m128i mask = _mm_setr_epi8(0,1,2,4,5,6,8,9,10,12,13,14,-1,-1,-1,-1);
    NumericPixels packed = numeric_pixels16(planes+i,planes+n+i,planes+2*n+i,
                                             _mm_setzero_si128(),basis);
    __m128i a = _mm_shuffle_epi8(packed.p0, mask);
    __m128i b = _mm_shuffle_epi8(packed.p1, mask);
    __m128i c = _mm_shuffle_epi8(packed.p2, mask);
    __m128i d = _mm_shuffle_epi8(packed.p3, mask);
    return (NumericRgb){
        _mm_or_si128(a, _mm_slli_si128(b,12)),
        _mm_or_si128(_mm_srli_si128(b,4), _mm_slli_si128(c,8)),
        _mm_or_si128(_mm_srli_si128(c,8), _mm_slli_si128(d,4))
    };
}
#endif
#ifdef QLI_NUMERIC_SSE42
typedef struct { __m128i a, b, c; } sse42_NumericRgb;

static QLI_SSE42_TARGET QLI_INLINE sse42_NumericRgb sse42_numeric_rgb16(const uint8_t *planes, size_t n,
                                           size_t i, unsigned basis)
{
    const __m128i mask = _mm_setr_epi8(0,1,2,4,5,6,8,9,10,12,13,14,-1,-1,-1,-1);
    NumericPixels packed = numeric_pixels16(planes+i,planes+n+i,planes+2*n+i,
                                             _mm_setzero_si128(),basis);
    __m128i a = _mm_shuffle_epi8(packed.p0, mask);
    __m128i b = _mm_shuffle_epi8(packed.p1, mask);
    __m128i c = _mm_shuffle_epi8(packed.p2, mask);
    __m128i d = _mm_shuffle_epi8(packed.p3, mask);
    return (sse42_NumericRgb){
        _mm_or_si128(a, _mm_slli_si128(b,12)),
        _mm_or_si128(_mm_srli_si128(b,4), _mm_slli_si128(c,8)),
        _mm_or_si128(_mm_srli_si128(c,8), _mm_slli_si128(d,4))
    };
}
#endif

static QLI_INLINE void numeric_rgb_tail(const uint8_t *planes, size_t n, size_t i,
                                        uint8_t *out, unsigned basis)
{
    for (; i < n; ++i) {
        uint8_t first=planes[i], d1=planes[n+i], d2=planes[2*n+i];
        uint8_t g=basis ? (uint8_t)(first+d1-128u) : first;
        uint8_t r=basis==1 ? first : (uint8_t)(g+(basis==2 ? d2 : d1)-128u);
        uint8_t b=basis==2 ? first : (uint8_t)(g+d2-128u);
        if (basis==3 || basis==4) {
            g=(uint8_t)(((first+1u)>>1)+d1-128u);
            uint8_t other=(uint8_t)(((g+1u)>>1)+d2-128u);
            r=basis==3 ? first : other;
            b=basis==4 ? first : other;
        } else if (basis==5) {
            r=(uint8_t)(first+64u-((d1+1u)>>1));
            g=(uint8_t)(r+d1-128u);
            b=(uint8_t)(first+d2-128u);
        } else if (basis==6) {
            g=first;
            r=(uint8_t)(g+d1-128u);
            b=(uint8_t)(((g+r+1u)>>1)+d2-128u);
        }
        out[3*i]=r;
        out[3*i+1]=g;
        out[3*i+2]=b;
    }
}

#ifdef QLI_NUMERIC_AVX2
static QLI_NUMERIC_TARGET size_t interleave_rgb_blocks(const uint8_t *planes, size_t n,
                                                       uint8_t *out, unsigned basis)
{
    size_t i = 0;
    for (; n - i >= 16; i += 16) {
        NumericRgb pixels = numeric_rgb16(planes, n, i, basis);
        numerical_store16(out + 3*i, pixels.a);
        numerical_store16(out + 3*i + 16, pixels.b);
        numerical_store16(out + 3*i + 32, pixels.c);
    }
    return i;
}
#endif
#ifdef QLI_NUMERIC_SSE42
static QLI_SSE42_TARGET size_t sse42_interleave_rgb_blocks(const uint8_t *planes, size_t n,
                                                       uint8_t *out, unsigned basis)
{
    size_t i = 0;
    for (; n - i >= 16; i += 16) {
        sse42_NumericRgb pixels = sse42_numeric_rgb16(planes, n, i, basis);
        sse42_numerical_store16(out + 3*i, pixels.a);
        sse42_numerical_store16(out + 3*i + 16, pixels.b);
        sse42_numerical_store16(out + 3*i + 32, pixels.c);
    }
    return i;
}
#endif

static void color_interleave_rgb(const uint8_t *planes, size_t n, uint8_t *out,
                                 unsigned basis, int vector)
{
    size_t i = 0;
#ifdef QLI_NUMERIC_SSE42
    if (vector) i = QLI_NUMERIC_FN(vector, interleave_rgb_blocks)(planes, n, out, basis);
#else
    (void)vector;
#endif
    numeric_rgb_tail(planes, n, i, out, basis);
}

#ifdef QLI_NUMERIC_AVX2

static QLI_NUMERIC_TARGET inline __m128i predict8(int mode, __m128i w, __m128i n,
                              __m128i nw, __m128i ne, __m128i maxv) {
    __m128i wn = _mm_add_epi16(w,n);
    __m128i two = _mm_set1_epi16(2);
    switch (mode) {
    case 0:
        return _mm_min_epi16(_mm_max_epi16(_mm_sub_epi16(wn,nw),_mm_min_epi16(w,n)),
                             _mm_max_epi16(w,n));
    case 1: return w;
    case 2: return n;
    case 3:
        return _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(wn,two),_mm_add_epi16(nw,ne)),2);
    case 4:
        return _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(wn,two),_mm_slli_epi16(w,1)),2);
    case 5:
        return _mm_mulhi_epu16(_mm_add_epi16(_mm_add_epi16(wn,ne),_mm_set1_epi16(1)),
                              _mm_set1_epi16(21846));
    case 6:
        return _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(wn,w),_mm_add_epi16(ne,two)),2);
    default: {
        __m128i p = _mm_add_epi16(wn,_mm_slli_epi16(wn,1));
        p = _mm_add_epi16(_mm_sub_epi16(p,_mm_slli_epi16(nw,1)),two);
        return _mm_min_epi16(_mm_max_epi16(_mm_srai_epi16(p,2),_mm_setzero_si128()),maxv);
    }
    }
}

static QLI_NUMERIC_TARGET void transpose(__m128i q[8]) {
    __m128i a[8], b[8];
    for (int i = 0; i < 8; i += 2) {
        a[i] = _mm_unpacklo_epi16(q[i],q[i+1]);
        a[i+1] = _mm_unpackhi_epi16(q[i],q[i+1]);
    }
    for (int i = 0; i < 8; i += 4) {
        b[i] = _mm_unpacklo_epi32(a[i],a[i+2]);
        b[i+1] = _mm_unpackhi_epi32(a[i],a[i+2]);
        b[i+2] = _mm_unpacklo_epi32(a[i+1],a[i+3]);
        b[i+3] = _mm_unpackhi_epi32(a[i+1],a[i+3]);
    }
    for (int i = 0; i < 4; ++i) {
        q[2*i] = _mm_unpacklo_epi64(b[i],b[i+4]);
        q[2*i+1] = _mm_unpackhi_epi64(b[i],b[i+4]);
    }
}
#endif
#ifdef QLI_NUMERIC_SSE42

static QLI_SSE42_TARGET inline __m128i sse42_predict8(int mode, __m128i w, __m128i n,
                              __m128i nw, __m128i ne, __m128i maxv) {
    __m128i wn = _mm_add_epi16(w,n);
    __m128i two = _mm_set1_epi16(2);
    switch (mode) {
    case 0:
        return _mm_min_epi16(_mm_max_epi16(_mm_sub_epi16(wn,nw),_mm_min_epi16(w,n)),
                             _mm_max_epi16(w,n));
    case 1: return w;
    case 2: return n;
    case 3:
        return _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(wn,two),_mm_add_epi16(nw,ne)),2);
    case 4:
        return _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(wn,two),_mm_slli_epi16(w,1)),2);
    case 5:
        return _mm_mulhi_epu16(_mm_add_epi16(_mm_add_epi16(wn,ne),_mm_set1_epi16(1)),
                              _mm_set1_epi16(21846));
    case 6:
        return _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(wn,w),_mm_add_epi16(ne,two)),2);
    default: {
        __m128i p = _mm_add_epi16(wn,_mm_slli_epi16(wn,1));
        p = _mm_add_epi16(_mm_sub_epi16(p,_mm_slli_epi16(nw,1)),two);
        return _mm_min_epi16(_mm_max_epi16(_mm_srai_epi16(p,2),_mm_setzero_si128()),maxv);
    }
    }
}

static QLI_SSE42_TARGET void sse42_transpose(__m128i q[8]) {
    __m128i a[8], b[8];
    for (int i = 0; i < 8; i += 2) {
        a[i] = _mm_unpacklo_epi16(q[i],q[i+1]);
        a[i+1] = _mm_unpackhi_epi16(q[i],q[i+1]);
    }
    for (int i = 0; i < 8; i += 4) {
        b[i] = _mm_unpacklo_epi32(a[i],a[i+2]);
        b[i+1] = _mm_unpackhi_epi32(a[i],a[i+2]);
        b[i+2] = _mm_unpacklo_epi32(a[i+1],a[i+3]);
        b[i+3] = _mm_unpackhi_epi32(a[i+1],a[i+3]);
    }
    for (int i = 0; i < 4; ++i) {
        q[2*i] = _mm_unpacklo_epi64(b[i],b[i+4]);
        q[2*i+1] = _mm_unpackhi_epi64(b[i],b[i+4]);
    }
}
#endif
enum { IMG_CAP = 1u << 24, NUMERICAL_FILE_CAP = 1u << 27, BANKS = 16 };

typedef struct {
    const uint8_t *at, *end;
} Input;

typedef struct {
    const uint8_t *data, *end;
} Queue;

typedef struct {
    uint8_t *raster, *literals, *map, *modes, *labels;
    uint8_t *error;
    uint32_t *tables;
    const uint8_t *earlier[2];
    unsigned guides, table_models;
    int vector, stencil, color_plane, directional, full_frame;
    void *defer;
    int8_t stencil_fit[20];
    uint8_t *causal_aux;
    size_t modes_count, labels_cap;
} Work;

static size_t min_size(size_t a, size_t b) { return a < b ? a : b; }

static int decode_record(const uint8_t *data, size_t bytes, uint8_t *output,
                         size_t expected) {
    if (bytes>=4 && !memcmp(data,"QVP1",4))
        return qvp_read(data,bytes,output,expected,0);
    size_t decoded=0;
    return qbs_decode(data,bytes,output,expected,&decoded,QBS_NO_TABLE) &&
           decoded==expected;
}

typedef struct {
    const uint8_t *data;
    size_t bytes;
    unsigned compressed;
} PlaneRecord;

static int read_plane_record(Input *in, size_t expected, PlaneRecord *packet)
{
    if ((size_t)(in->end-in->at)<5) return 0;
    const unsigned kind=in->at[0];
    const size_t bytes=qli_read_le32(in->at+1);
    const uint8_t *data=in->at+5;
    if (bytes>(size_t)(in->end-data)) return 0;
    if (kind==0) {
        if (bytes!=expected) return 0;
    } else if (kind!=1 || !bytes || (bytes>=expected &&
               (bytes<4 || memcmp(data,"QVP1",4)))) return 0;
    *packet=(PlaneRecord){data,bytes,kind};
    in->at=data+bytes;
    return 1;
}

static const uint8_t *plane_record_data(const PlaneRecord *packet, size_t expected,
                                        uint8_t *memory)
{
    if (!packet->compressed) return packet->data;
    return decode_record(packet->data,packet->bytes,memory,expected) ? memory : NULL;
}

static int record(Input *in, size_t expected, uint8_t *memory,
                  const uint8_t **view)
{
    PlaneRecord packet;
    if (!read_plane_record(in,expected,&packet)) return 0;
    *view=plane_record_data(&packet,expected,memory);
    return *view!=NULL;
}

static int queue_records(Input *in, uint8_t *scratch, size_t expected,
                         Queue bank[BANKS]) {
    size_t left=expected;
    for (unsigned i=0;i<BANKS;++i) {
        if ((size_t)(in->end-in->at)<5) return 0;
        size_t bytes=qli_read_le32(in->at+1);
        unsigned kind=in->at[0];
        if (bytes>(size_t)(in->end-in->at)-5) return 0;
        uint64_t n=bytes;
        if (kind) {
            if (kind!=1 || bytes<16 ||
                (memcmp(in->at+5,"QBS1",4) && memcmp(in->at+5,"QVP1",4))) return 0;
            n=qli_read_le64(in->at+11);
        }
        const uint8_t *view;
        if (n>left || !record(in,(size_t)n,scratch,&view)) return 0;
        bank[i].data=view;
        bank[i].end=view+(size_t)n;
        scratch+=(size_t)n;
        left-=(size_t)n;
    }
    return !left && in->at==in->end;
}

static unsigned flag(const uint8_t *flat, size_t block) {
    return (flat[block/8]>>(block%8))&1u;
}

static int take_numerical(Queue *q, uint8_t *out, size_t n) {
    if (n>(size_t)(q->end-q->data)) return 0;
    memcpy(out,q->data,n);
    q->data+=n;
    return 1;
}

static size_t live_tiles(uint8_t *labels, const uint8_t *flat,
                         unsigned w, unsigned h) {
    size_t count=0;
    for (unsigned y=0;y<h;y+=8) {
        for (unsigned x=0;x<w;x+=8) {
            size_t width=min_size(8,w-x);
            unsigned live=0;
            for (unsigned r=y; r<y+8 && r<h; ++r) {
                size_t start=(size_t)r*w+x;
                live|=!(flag(flat,start/256) && flag(flat,(start+width-1)/256));
            }
            *labels++=(uint8_t)live;
            count+=live;
        }
    }
    return count;
}

static int tile_row(Queue *bank, size_t at, size_t len, const uint8_t *flat,
                    const uint8_t *constants, uint64_t *row) {
    uint8_t *bytes=(uint8_t *)row;
    size_t block=at/256;
    size_t first=min_size(len,256-at%256);
    if (flag(flat,block))
        memset(bytes,constants[block],first);
    else if (!take_numerical(bank,bytes,first))
        return 0;
    if (first==len) return 1;
    if (flag(flat,block+1)) {
        memset(bytes+first,constants[block+1],len-first);
        return 1;
    }
    return take_numerical(bank,bytes+first,len-first);
}

#ifdef QLI_NUMERIC_AVX2
static QLI_NUMERIC_TARGET inline uint64_t overlap(uint64_t left, uint64_t right, unsigned shift) {
    return (left>>(64-shift)) | (right<<shift);
}

static QLI_NUMERIC_TARGET inline __m128i wave_bytes(uint64_t row) {
    return _mm_cvtepu8_epi16(_mm_cvtsi64_si128((int64_t)row));
}
static QLI_NUMERIC_TARGET inline NumericalLanes numerical_gather8(const uint32_t *table, NumericalLanes address,
                                            NumericalLanes live)
{
    return _mm256_mask_i32gather_epi32(_mm256_setzero_si256(),
                                           (const int32_t *)table, address, live, 4);
}
#endif
#ifdef QLI_NUMERIC_SSE42
static QLI_SSE42_TARGET inline uint64_t sse42_overlap(uint64_t left, uint64_t right, unsigned shift) {
    return (left>>(64-shift)) | (right<<shift);
}

static QLI_SSE42_TARGET inline __m128i sse42_wave_bytes(uint64_t row) {
    return _mm_cvtepu8_epi16(_mm_cvtsi64_si128((int64_t)row));
}
static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_numerical_gather8(const uint32_t *table, sse42_NumericalLanes address,
                                            sse42_NumericalLanes live)
{
    return sse_mask_i32gather_epi32(sse_setzero_si256(),
                                           (const int32_t *)table, address, live, 4);
}
#endif

enum { CAUSAL_MODELS=32, CAUSAL_SCALE=4096, CAUSAL_LOW=65536,
       CAUSAL_CONTEXTS=18, CAUSAL_SKIP=255, CAUSAL_MODEL_BYTES=CAUSAL_MODELS*542 };

typedef struct {
    PlaneRecord packet;
    unsigned count, length;
} CausalModels;

static int read_causal_models(Input *in,CausalModels *book)
{
    if ((size_t)(in->end-in->at)<3) return 0;
    const unsigned count=in->at[0], length=qli_read_le16(in->at+1);
    in->at+=3;
    if (!count || count>CAUSAL_MODELS || length<32*count || length>542*count) return 0;
    book->count=count;
    book->length=length;
    return read_plane_record(in,length,&book->packet);
}

static int build_causal_tables(const CausalModels *book,uint32_t *table)
{
    uint8_t scratch[CAUSAL_MODEL_BYTES];
    const uint8_t *data=plane_record_data(&book->packet,book->length,scratch);
    if (!data) return 0;
    const uint8_t *end=data+book->length;
    for (unsigned k=0;k<book->count;++k) {
        if ((size_t)(end-data)<32) return 0;
        const uint8_t *support=data;
        data+=32;
        unsigned remain=0,cumulative=0;
        for (unsigned j=0;j<32;++j) remain+=(unsigned)qlic_count_bits(support[j]);
        if (!remain || (size_t)(end-data)<2*(remain-1)) return 0;
        for (unsigned s=0;s<256;++s) if (support[s/8]&(1u<<(s%8))) {
            unsigned frequency=4096-cumulative;
            if (--remain) {
                frequency=qli_read_le16(data);
                data+=2;
            }
            if (!frequency || frequency>4096-cumulative) return 0;
            uint32_t *slots=table+(size_t)k*4096+cumulative;
            for (unsigned j=0;j<frequency;++j)
                slots[j]=s|(j<<8)|((frequency-1)<<20);
            cumulative+=frequency;
        }
    }
    return data==end;
}

#ifdef QLI_NUMERIC_AVX2
static QLI_NUMERIC_TARGET inline __m128i direction_indices(__m128i west, __m128i north,
                                            __m128i northwest, __m128i prediction,
                                            __m128i has_mid, __m128i has_gradient)
{
    const __m128i bias=_mm_set1_epi8((char)128),stride=_mm_set1_epi8(6);
    const __m128i gradient=_mm_subs_epu8(
        _mm_adds_epu8(north,_mm_subs_epu8(west,northwest)),
        _mm_subs_epu8(northwest,west));
    const __m128i use_gradient=_mm_andnot_si128(_mm_cmpeq_epi8(gradient,prediction),has_gradient);
    __m128i value=_mm_blendv_epi8(_mm_avg_epu8(west,north),gradient,use_gradient);
    value=_mm_blendv_epi8(prediction,value,has_mid);
    value=_mm_xor_si128(value,bias);
    const __m128i expected=_mm_xor_si128(prediction,bias);
    return _mm_andnot_si128(_mm_cmpgt_epi8(expected,value),
        _mm_add_epi8(stride,_mm_and_si128(stride,_mm_cmpgt_epi8(value,expected))));
}

static QLI_NUMERIC_TARGET inline NumericalLanes direction_model(NumericalLanes packed, __m128i shifts)
{
    return _mm256_srlv_epi32(packed,_mm256_cvtepu8_epi32(shifts));
}
#endif
#ifdef QLI_NUMERIC_SSE42
static QLI_SSE42_TARGET inline __m128i sse42_direction_indices(__m128i west, __m128i north,
                                            __m128i northwest, __m128i prediction,
                                            __m128i has_mid, __m128i has_gradient)
{
    const __m128i bias=_mm_set1_epi8((char)128),stride=_mm_set1_epi8(6);
    const __m128i gradient=_mm_subs_epu8(
        _mm_adds_epu8(north,_mm_subs_epu8(west,northwest)),
        _mm_subs_epu8(northwest,west));
    const __m128i use_gradient=_mm_andnot_si128(_mm_cmpeq_epi8(gradient,prediction),has_gradient);
    __m128i value=_mm_blendv_epi8(_mm_avg_epu8(west,north),gradient,use_gradient);
    value=_mm_blendv_epi8(prediction,value,has_mid);
    value=_mm_xor_si128(value,bias);
    const __m128i expected=_mm_xor_si128(prediction,bias);
    return _mm_andnot_si128(_mm_cmpgt_epi8(expected,value),
        _mm_add_epi8(stride,_mm_and_si128(stride,_mm_cmpgt_epi8(value,expected))));
}

static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_direction_model(sse42_NumericalLanes packed, __m128i shifts)
{
    return sse_srlv_epi32(packed,sse_cvtepu8_epi32(shifts));
}
#endif

#ifdef QLI_NUMERIC_SSE42
enum { ROUTE_OFFSET_MASK=31u<<12, ROUTE_SKIP=32u<<12,
       DIRECTION_ROUTE_SKIP=ROUTE_SKIP|(ROUTE_SKIP<<6)|(ROUTE_SKIP<<12) };

static inline uint32_t route_offset(unsigned id)
{
    return id==CAUSAL_SKIP ? ROUTE_SKIP : id<<12;
}

static inline uint32_t direction_word(const uint8_t *ids)
{
    return route_offset(ids[0])|(route_offset(ids[1])<<6)|(route_offset(ids[2])<<12);
}

#endif

enum { DIAGONAL_ROWS=16, DIAGONAL_SKEW=4, DIAGONAL_STATES=2041 };

typedef struct {
    Work *work;
    const uint8_t *flat;
    const uint8_t *mapping;
    const uint8_t *route;
    unsigned w,h,mode_cols,nonzero_constants;
} Diagonal;

static unsigned stencil_scalar(const int8_t *a,const uint8_t *p,ptrdiff_t w)
{
    int v=16+a[0]*p[-1]+a[1]*p[-w]+a[2]*p[-w-1]+a[3]*p[-w+1]
            +a[4]*p[-2]+a[5]*p[-w-2]+a[6]*p[-2*w-1]+a[7]*p[-2*w]
            +a[8]*p[-2*w+1]+a[9]*p[-2*w+2];
    return v<0 ? 0u : v>=8192 ? 255u : (unsigned)v>>5;
}

static unsigned diagonal_cue(unsigned e)
{
    unsigned m=e<128 ? e : 256-e;
    return e ? 1+2*((m>1)+(m>4))+(e>=128) : 0;
}

static int diagonal_scalar(Input *in,Diagonal *d,uint32_t ans[DIAGONAL_ROWS],uint8_t *output)
{
    Work *work=d->work;
    unsigned w=d->w,h=d->h;
    uint16_t *north=(uint16_t *)work->causal_aux;
    memset(north,0,2*(size_t)(w+3));
    uint8_t bin[DIAGONAL_STATES];
    unsigned boundary=2, activity_bin=0;
    for (unsigned i=0;i<DIAGONAL_STATES;++i) {
        while (i*i+1>=boundary && activity_bin<17) {
            ++activity_bin;
            boundary*=2;
        }
        bin[i]=(uint8_t)activity_bin;
    }
    for (unsigned base=0;base<h;base+=DIAGONAL_ROWS) {
        unsigned rows=(unsigned)min_size(DIAGONAL_ROWS,h-base);
        uint16_t prev[DIAGONAL_ROWS]={0}, lag[DIAGONAL_ROWS]={0};
        const uint8_t *modes=work->modes+(size_t)(base/16)*d->mode_cols;
        for (unsigned t=0;t<w+DIAGONAL_SKEW*(rows-1);++t) {
            uint16_t next[DIAGONAL_ROWS]={0};
            for (unsigned k=0;k<rows;++k) {
                unsigned x=t-DIAGONAL_SKEW*k;
                if (x>=w) continue;
                unsigned y=base+k;
                size_t p=(size_t)y*w+x, block=p/256;
                unsigned e=0, west=prev[k];
                const uint8_t *above=y ? output+(size_t)(y-1)*w : NULL;
                unsigned left=x ? output[p-1] : above ? above[0] : 128;
                unsigned n=above ? above[x] : left;
                unsigned nw=above && x ? above[x-1] : n;
                unsigned ne=above && x+1<w ? above[x+1] : n;
                unsigned mode=modes[x/16];
                unsigned prediction=scalar_predict(mode,left,n,nw,ne,255);
                if (flag(d->flat,block)) {
                    e=work->raster[block];
                } else {
                    unsigned id=0;
                    if (work->guides) {
                        unsigned cue=work->earlier[0][p];
                        if (work->guides==2) cue=7*cue+work->earlier[1][p];
                        id=d->mapping[cue];
                    }
                    unsigned activity=bin[west];
                    unsigned key=id*CAUSAL_CONTEXTS+activity;
                    unsigned model=d->route[key];
                    if (model==CAUSAL_SKIP) return 0;
                    uint32_t a=ans[k], entry=work->tables[(size_t)model*CAUSAL_SCALE+(a&(CAUSAL_SCALE-1))];
                    a=(a>>12)*((entry>>20)+1)+((entry>>8)&(CAUSAL_SCALE-1));
                    if (a<CAUSAL_LOW) {
                        if (!a || (size_t)(in->end-in->at)<2) return 0;
                        a=(a<<16)|qli_read_le16(in->at);
                        in->at+=2;
                    }
                    ans[k]=a;
                    e=(uint8_t)entry;
                }
                unsigned a=k ? lag[k-1] : north[x+2];
                unsigned b=k ? prev[k-1] : north[x+3];
                unsigned folded=e<128 ? 2*e : 2*(256-e)-1;
                next[k]=(uint16_t)((west+(8*folded+(a>b ? a : b)+1)/2+1)/2);
                if (k==DIAGONAL_ROWS-1) north[x]=next[k];
                if (work->error) work->error[p]=(uint8_t)diagonal_cue(e);
                output[p]=(uint8_t)(e+prediction);
            }
            memcpy(lag,prev,sizeof(lag));
            memcpy(prev,next,sizeof(prev));
        }
    }
    return 1;
}

#ifdef QLI_NUMERIC_SSE42

static const uint8_t qxj_refill_masks[256][16]={
    {128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128},
    {0,1,128,128,128,128,128,128,128,128,128,128,128,128,128,128},
    {128,128,0,1,128,128,128,128,128,128,128,128,128,128,128,128},
    {0,1,2,3,128,128,128,128,128,128,128,128,128,128,128,128},
    {128,128,128,128,0,1,128,128,128,128,128,128,128,128,128,128},
    {0,1,128,128,2,3,128,128,128,128,128,128,128,128,128,128},
    {128,128,0,1,2,3,128,128,128,128,128,128,128,128,128,128},
    {0,1,2,3,4,5,128,128,128,128,128,128,128,128,128,128},
    {128,128,128,128,128,128,0,1,128,128,128,128,128,128,128,128},
    {0,1,128,128,128,128,2,3,128,128,128,128,128,128,128,128},
    {128,128,0,1,128,128,2,3,128,128,128,128,128,128,128,128},
    {0,1,2,3,128,128,4,5,128,128,128,128,128,128,128,128},
    {128,128,128,128,0,1,2,3,128,128,128,128,128,128,128,128},
    {0,1,128,128,2,3,4,5,128,128,128,128,128,128,128,128},
    {128,128,0,1,2,3,4,5,128,128,128,128,128,128,128,128},
    {0,1,2,3,4,5,6,7,128,128,128,128,128,128,128,128},
    {128,128,128,128,128,128,128,128,0,1,128,128,128,128,128,128},
    {0,1,128,128,128,128,128,128,2,3,128,128,128,128,128,128},
    {128,128,0,1,128,128,128,128,2,3,128,128,128,128,128,128},
    {0,1,2,3,128,128,128,128,4,5,128,128,128,128,128,128},
    {128,128,128,128,0,1,128,128,2,3,128,128,128,128,128,128},
    {0,1,128,128,2,3,128,128,4,5,128,128,128,128,128,128},
    {128,128,0,1,2,3,128,128,4,5,128,128,128,128,128,128},
    {0,1,2,3,4,5,128,128,6,7,128,128,128,128,128,128},
    {128,128,128,128,128,128,0,1,2,3,128,128,128,128,128,128},
    {0,1,128,128,128,128,2,3,4,5,128,128,128,128,128,128},
    {128,128,0,1,128,128,2,3,4,5,128,128,128,128,128,128},
    {0,1,2,3,128,128,4,5,6,7,128,128,128,128,128,128},
    {128,128,128,128,0,1,2,3,4,5,128,128,128,128,128,128},
    {0,1,128,128,2,3,4,5,6,7,128,128,128,128,128,128},
    {128,128,0,1,2,3,4,5,6,7,128,128,128,128,128,128},
    {0,1,2,3,4,5,6,7,8,9,128,128,128,128,128,128},
    {128,128,128,128,128,128,128,128,128,128,0,1,128,128,128,128},
    {0,1,128,128,128,128,128,128,128,128,2,3,128,128,128,128},
    {128,128,0,1,128,128,128,128,128,128,2,3,128,128,128,128},
    {0,1,2,3,128,128,128,128,128,128,4,5,128,128,128,128},
    {128,128,128,128,0,1,128,128,128,128,2,3,128,128,128,128},
    {0,1,128,128,2,3,128,128,128,128,4,5,128,128,128,128},
    {128,128,0,1,2,3,128,128,128,128,4,5,128,128,128,128},
    {0,1,2,3,4,5,128,128,128,128,6,7,128,128,128,128},
    {128,128,128,128,128,128,0,1,128,128,2,3,128,128,128,128},
    {0,1,128,128,128,128,2,3,128,128,4,5,128,128,128,128},
    {128,128,0,1,128,128,2,3,128,128,4,5,128,128,128,128},
    {0,1,2,3,128,128,4,5,128,128,6,7,128,128,128,128},
    {128,128,128,128,0,1,2,3,128,128,4,5,128,128,128,128},
    {0,1,128,128,2,3,4,5,128,128,6,7,128,128,128,128},
    {128,128,0,1,2,3,4,5,128,128,6,7,128,128,128,128},
    {0,1,2,3,4,5,6,7,128,128,8,9,128,128,128,128},
    {128,128,128,128,128,128,128,128,0,1,2,3,128,128,128,128},
    {0,1,128,128,128,128,128,128,2,3,4,5,128,128,128,128},
    {128,128,0,1,128,128,128,128,2,3,4,5,128,128,128,128},
    {0,1,2,3,128,128,128,128,4,5,6,7,128,128,128,128},
    {128,128,128,128,0,1,128,128,2,3,4,5,128,128,128,128},
    {0,1,128,128,2,3,128,128,4,5,6,7,128,128,128,128},
    {128,128,0,1,2,3,128,128,4,5,6,7,128,128,128,128},
    {0,1,2,3,4,5,128,128,6,7,8,9,128,128,128,128},
    {128,128,128,128,128,128,0,1,2,3,4,5,128,128,128,128},
    {0,1,128,128,128,128,2,3,4,5,6,7,128,128,128,128},
    {128,128,0,1,128,128,2,3,4,5,6,7,128,128,128,128},
    {0,1,2,3,128,128,4,5,6,7,8,9,128,128,128,128},
    {128,128,128,128,0,1,2,3,4,5,6,7,128,128,128,128},
    {0,1,128,128,2,3,4,5,6,7,8,9,128,128,128,128},
    {128,128,0,1,2,3,4,5,6,7,8,9,128,128,128,128},
    {0,1,2,3,4,5,6,7,8,9,10,11,128,128,128,128},
    {128,128,128,128,128,128,128,128,128,128,128,128,0,1,128,128},
    {0,1,128,128,128,128,128,128,128,128,128,128,2,3,128,128},
    {128,128,0,1,128,128,128,128,128,128,128,128,2,3,128,128},
    {0,1,2,3,128,128,128,128,128,128,128,128,4,5,128,128},
    {128,128,128,128,0,1,128,128,128,128,128,128,2,3,128,128},
    {0,1,128,128,2,3,128,128,128,128,128,128,4,5,128,128},
    {128,128,0,1,2,3,128,128,128,128,128,128,4,5,128,128},
    {0,1,2,3,4,5,128,128,128,128,128,128,6,7,128,128},
    {128,128,128,128,128,128,0,1,128,128,128,128,2,3,128,128},
    {0,1,128,128,128,128,2,3,128,128,128,128,4,5,128,128},
    {128,128,0,1,128,128,2,3,128,128,128,128,4,5,128,128},
    {0,1,2,3,128,128,4,5,128,128,128,128,6,7,128,128},
    {128,128,128,128,0,1,2,3,128,128,128,128,4,5,128,128},
    {0,1,128,128,2,3,4,5,128,128,128,128,6,7,128,128},
    {128,128,0,1,2,3,4,5,128,128,128,128,6,7,128,128},
    {0,1,2,3,4,5,6,7,128,128,128,128,8,9,128,128},
    {128,128,128,128,128,128,128,128,0,1,128,128,2,3,128,128},
    {0,1,128,128,128,128,128,128,2,3,128,128,4,5,128,128},
    {128,128,0,1,128,128,128,128,2,3,128,128,4,5,128,128},
    {0,1,2,3,128,128,128,128,4,5,128,128,6,7,128,128},
    {128,128,128,128,0,1,128,128,2,3,128,128,4,5,128,128},
    {0,1,128,128,2,3,128,128,4,5,128,128,6,7,128,128},
    {128,128,0,1,2,3,128,128,4,5,128,128,6,7,128,128},
    {0,1,2,3,4,5,128,128,6,7,128,128,8,9,128,128},
    {128,128,128,128,128,128,0,1,2,3,128,128,4,5,128,128},
    {0,1,128,128,128,128,2,3,4,5,128,128,6,7,128,128},
    {128,128,0,1,128,128,2,3,4,5,128,128,6,7,128,128},
    {0,1,2,3,128,128,4,5,6,7,128,128,8,9,128,128},
    {128,128,128,128,0,1,2,3,4,5,128,128,6,7,128,128},
    {0,1,128,128,2,3,4,5,6,7,128,128,8,9,128,128},
    {128,128,0,1,2,3,4,5,6,7,128,128,8,9,128,128},
    {0,1,2,3,4,5,6,7,8,9,128,128,10,11,128,128},
    {128,128,128,128,128,128,128,128,128,128,0,1,2,3,128,128},
    {0,1,128,128,128,128,128,128,128,128,2,3,4,5,128,128},
    {128,128,0,1,128,128,128,128,128,128,2,3,4,5,128,128},
    {0,1,2,3,128,128,128,128,128,128,4,5,6,7,128,128},
    {128,128,128,128,0,1,128,128,128,128,2,3,4,5,128,128},
    {0,1,128,128,2,3,128,128,128,128,4,5,6,7,128,128},
    {128,128,0,1,2,3,128,128,128,128,4,5,6,7,128,128},
    {0,1,2,3,4,5,128,128,128,128,6,7,8,9,128,128},
    {128,128,128,128,128,128,0,1,128,128,2,3,4,5,128,128},
    {0,1,128,128,128,128,2,3,128,128,4,5,6,7,128,128},
    {128,128,0,1,128,128,2,3,128,128,4,5,6,7,128,128},
    {0,1,2,3,128,128,4,5,128,128,6,7,8,9,128,128},
    {128,128,128,128,0,1,2,3,128,128,4,5,6,7,128,128},
    {0,1,128,128,2,3,4,5,128,128,6,7,8,9,128,128},
    {128,128,0,1,2,3,4,5,128,128,6,7,8,9,128,128},
    {0,1,2,3,4,5,6,7,128,128,8,9,10,11,128,128},
    {128,128,128,128,128,128,128,128,0,1,2,3,4,5,128,128},
    {0,1,128,128,128,128,128,128,2,3,4,5,6,7,128,128},
    {128,128,0,1,128,128,128,128,2,3,4,5,6,7,128,128},
    {0,1,2,3,128,128,128,128,4,5,6,7,8,9,128,128},
    {128,128,128,128,0,1,128,128,2,3,4,5,6,7,128,128},
    {0,1,128,128,2,3,128,128,4,5,6,7,8,9,128,128},
    {128,128,0,1,2,3,128,128,4,5,6,7,8,9,128,128},
    {0,1,2,3,4,5,128,128,6,7,8,9,10,11,128,128},
    {128,128,128,128,128,128,0,1,2,3,4,5,6,7,128,128},
    {0,1,128,128,128,128,2,3,4,5,6,7,8,9,128,128},
    {128,128,0,1,128,128,2,3,4,5,6,7,8,9,128,128},
    {0,1,2,3,128,128,4,5,6,7,8,9,10,11,128,128},
    {128,128,128,128,0,1,2,3,4,5,6,7,8,9,128,128},
    {0,1,128,128,2,3,4,5,6,7,8,9,10,11,128,128},
    {128,128,0,1,2,3,4,5,6,7,8,9,10,11,128,128},
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,128,128},
    {128,128,128,128,128,128,128,128,128,128,128,128,128,128,0,1},
    {0,1,128,128,128,128,128,128,128,128,128,128,128,128,2,3},
    {128,128,0,1,128,128,128,128,128,128,128,128,128,128,2,3},
    {0,1,2,3,128,128,128,128,128,128,128,128,128,128,4,5},
    {128,128,128,128,0,1,128,128,128,128,128,128,128,128,2,3},
    {0,1,128,128,2,3,128,128,128,128,128,128,128,128,4,5},
    {128,128,0,1,2,3,128,128,128,128,128,128,128,128,4,5},
    {0,1,2,3,4,5,128,128,128,128,128,128,128,128,6,7},
    {128,128,128,128,128,128,0,1,128,128,128,128,128,128,2,3},
    {0,1,128,128,128,128,2,3,128,128,128,128,128,128,4,5},
    {128,128,0,1,128,128,2,3,128,128,128,128,128,128,4,5},
    {0,1,2,3,128,128,4,5,128,128,128,128,128,128,6,7},
    {128,128,128,128,0,1,2,3,128,128,128,128,128,128,4,5},
    {0,1,128,128,2,3,4,5,128,128,128,128,128,128,6,7},
    {128,128,0,1,2,3,4,5,128,128,128,128,128,128,6,7},
    {0,1,2,3,4,5,6,7,128,128,128,128,128,128,8,9},
    {128,128,128,128,128,128,128,128,0,1,128,128,128,128,2,3},
    {0,1,128,128,128,128,128,128,2,3,128,128,128,128,4,5},
    {128,128,0,1,128,128,128,128,2,3,128,128,128,128,4,5},
    {0,1,2,3,128,128,128,128,4,5,128,128,128,128,6,7},
    {128,128,128,128,0,1,128,128,2,3,128,128,128,128,4,5},
    {0,1,128,128,2,3,128,128,4,5,128,128,128,128,6,7},
    {128,128,0,1,2,3,128,128,4,5,128,128,128,128,6,7},
    {0,1,2,3,4,5,128,128,6,7,128,128,128,128,8,9},
    {128,128,128,128,128,128,0,1,2,3,128,128,128,128,4,5},
    {0,1,128,128,128,128,2,3,4,5,128,128,128,128,6,7},
    {128,128,0,1,128,128,2,3,4,5,128,128,128,128,6,7},
    {0,1,2,3,128,128,4,5,6,7,128,128,128,128,8,9},
    {128,128,128,128,0,1,2,3,4,5,128,128,128,128,6,7},
    {0,1,128,128,2,3,4,5,6,7,128,128,128,128,8,9},
    {128,128,0,1,2,3,4,5,6,7,128,128,128,128,8,9},
    {0,1,2,3,4,5,6,7,8,9,128,128,128,128,10,11},
    {128,128,128,128,128,128,128,128,128,128,0,1,128,128,2,3},
    {0,1,128,128,128,128,128,128,128,128,2,3,128,128,4,5},
    {128,128,0,1,128,128,128,128,128,128,2,3,128,128,4,5},
    {0,1,2,3,128,128,128,128,128,128,4,5,128,128,6,7},
    {128,128,128,128,0,1,128,128,128,128,2,3,128,128,4,5},
    {0,1,128,128,2,3,128,128,128,128,4,5,128,128,6,7},
    {128,128,0,1,2,3,128,128,128,128,4,5,128,128,6,7},
    {0,1,2,3,4,5,128,128,128,128,6,7,128,128,8,9},
    {128,128,128,128,128,128,0,1,128,128,2,3,128,128,4,5},
    {0,1,128,128,128,128,2,3,128,128,4,5,128,128,6,7},
    {128,128,0,1,128,128,2,3,128,128,4,5,128,128,6,7},
    {0,1,2,3,128,128,4,5,128,128,6,7,128,128,8,9},
    {128,128,128,128,0,1,2,3,128,128,4,5,128,128,6,7},
    {0,1,128,128,2,3,4,5,128,128,6,7,128,128,8,9},
    {128,128,0,1,2,3,4,5,128,128,6,7,128,128,8,9},
    {0,1,2,3,4,5,6,7,128,128,8,9,128,128,10,11},
    {128,128,128,128,128,128,128,128,0,1,2,3,128,128,4,5},
    {0,1,128,128,128,128,128,128,2,3,4,5,128,128,6,7},
    {128,128,0,1,128,128,128,128,2,3,4,5,128,128,6,7},
    {0,1,2,3,128,128,128,128,4,5,6,7,128,128,8,9},
    {128,128,128,128,0,1,128,128,2,3,4,5,128,128,6,7},
    {0,1,128,128,2,3,128,128,4,5,6,7,128,128,8,9},
    {128,128,0,1,2,3,128,128,4,5,6,7,128,128,8,9},
    {0,1,2,3,4,5,128,128,6,7,8,9,128,128,10,11},
    {128,128,128,128,128,128,0,1,2,3,4,5,128,128,6,7},
    {0,1,128,128,128,128,2,3,4,5,6,7,128,128,8,9},
    {128,128,0,1,128,128,2,3,4,5,6,7,128,128,8,9},
    {0,1,2,3,128,128,4,5,6,7,8,9,128,128,10,11},
    {128,128,128,128,0,1,2,3,4,5,6,7,128,128,8,9},
    {0,1,128,128,2,3,4,5,6,7,8,9,128,128,10,11},
    {128,128,0,1,2,3,4,5,6,7,8,9,128,128,10,11},
    {0,1,2,3,4,5,6,7,8,9,10,11,128,128,12,13},
    {128,128,128,128,128,128,128,128,128,128,128,128,0,1,2,3},
    {0,1,128,128,128,128,128,128,128,128,128,128,2,3,4,5},
    {128,128,0,1,128,128,128,128,128,128,128,128,2,3,4,5},
    {0,1,2,3,128,128,128,128,128,128,128,128,4,5,6,7},
    {128,128,128,128,0,1,128,128,128,128,128,128,2,3,4,5},
    {0,1,128,128,2,3,128,128,128,128,128,128,4,5,6,7},
    {128,128,0,1,2,3,128,128,128,128,128,128,4,5,6,7},
    {0,1,2,3,4,5,128,128,128,128,128,128,6,7,8,9},
    {128,128,128,128,128,128,0,1,128,128,128,128,2,3,4,5},
    {0,1,128,128,128,128,2,3,128,128,128,128,4,5,6,7},
    {128,128,0,1,128,128,2,3,128,128,128,128,4,5,6,7},
    {0,1,2,3,128,128,4,5,128,128,128,128,6,7,8,9},
    {128,128,128,128,0,1,2,3,128,128,128,128,4,5,6,7},
    {0,1,128,128,2,3,4,5,128,128,128,128,6,7,8,9},
    {128,128,0,1,2,3,4,5,128,128,128,128,6,7,8,9},
    {0,1,2,3,4,5,6,7,128,128,128,128,8,9,10,11},
    {128,128,128,128,128,128,128,128,0,1,128,128,2,3,4,5},
    {0,1,128,128,128,128,128,128,2,3,128,128,4,5,6,7},
    {128,128,0,1,128,128,128,128,2,3,128,128,4,5,6,7},
    {0,1,2,3,128,128,128,128,4,5,128,128,6,7,8,9},
    {128,128,128,128,0,1,128,128,2,3,128,128,4,5,6,7},
    {0,1,128,128,2,3,128,128,4,5,128,128,6,7,8,9},
    {128,128,0,1,2,3,128,128,4,5,128,128,6,7,8,9},
    {0,1,2,3,4,5,128,128,6,7,128,128,8,9,10,11},
    {128,128,128,128,128,128,0,1,2,3,128,128,4,5,6,7},
    {0,1,128,128,128,128,2,3,4,5,128,128,6,7,8,9},
    {128,128,0,1,128,128,2,3,4,5,128,128,6,7,8,9},
    {0,1,2,3,128,128,4,5,6,7,128,128,8,9,10,11},
    {128,128,128,128,0,1,2,3,4,5,128,128,6,7,8,9},
    {0,1,128,128,2,3,4,5,6,7,128,128,8,9,10,11},
    {128,128,0,1,2,3,4,5,6,7,128,128,8,9,10,11},
    {0,1,2,3,4,5,6,7,8,9,128,128,10,11,12,13},
    {128,128,128,128,128,128,128,128,128,128,0,1,2,3,4,5},
    {0,1,128,128,128,128,128,128,128,128,2,3,4,5,6,7},
    {128,128,0,1,128,128,128,128,128,128,2,3,4,5,6,7},
    {0,1,2,3,128,128,128,128,128,128,4,5,6,7,8,9},
    {128,128,128,128,0,1,128,128,128,128,2,3,4,5,6,7},
    {0,1,128,128,2,3,128,128,128,128,4,5,6,7,8,9},
    {128,128,0,1,2,3,128,128,128,128,4,5,6,7,8,9},
    {0,1,2,3,4,5,128,128,128,128,6,7,8,9,10,11},
    {128,128,128,128,128,128,0,1,128,128,2,3,4,5,6,7},
    {0,1,128,128,128,128,2,3,128,128,4,5,6,7,8,9},
    {128,128,0,1,128,128,2,3,128,128,4,5,6,7,8,9},
    {0,1,2,3,128,128,4,5,128,128,6,7,8,9,10,11},
    {128,128,128,128,0,1,2,3,128,128,4,5,6,7,8,9},
    {0,1,128,128,2,3,4,5,128,128,6,7,8,9,10,11},
    {128,128,0,1,2,3,4,5,128,128,6,7,8,9,10,11},
    {0,1,2,3,4,5,6,7,128,128,8,9,10,11,12,13},
    {128,128,128,128,128,128,128,128,0,1,2,3,4,5,6,7},
    {0,1,128,128,128,128,128,128,2,3,4,5,6,7,8,9},
    {128,128,0,1,128,128,128,128,2,3,4,5,6,7,8,9},
    {0,1,2,3,128,128,128,128,4,5,6,7,8,9,10,11},
    {128,128,128,128,0,1,128,128,2,3,4,5,6,7,8,9},
    {0,1,128,128,2,3,128,128,4,5,6,7,8,9,10,11},
    {128,128,0,1,2,3,128,128,4,5,6,7,8,9,10,11},
    {0,1,2,3,4,5,128,128,6,7,8,9,10,11,12,13},
    {128,128,128,128,128,128,0,1,2,3,4,5,6,7,8,9},
    {0,1,128,128,128,128,2,3,4,5,6,7,8,9,10,11},
    {128,128,0,1,128,128,2,3,4,5,6,7,8,9,10,11},
    {0,1,2,3,128,128,4,5,6,7,8,9,10,11,12,13},
    {128,128,128,128,0,1,2,3,4,5,6,7,8,9,10,11},
    {0,1,128,128,2,3,4,5,6,7,8,9,10,11,12,13},
    {128,128,0,1,2,3,4,5,6,7,8,9,10,11,12,13},
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
};

enum { MAX_WAVE_BLOCKS=(16384+60+15)/16 };

enum { BLOCK_CODED=0, BLOCK_MIXED=1, BLOCK_KNOWN=2 };

#endif

#ifdef QLI_NUMERIC_AVX2

static QLI_NUMERIC_TARGET inline int qxj_refill(Input *in,NumericalLanes *state,unsigned refill)
{
    if (!refill) return 1;
    const unsigned bytes=2u*(unsigned)qlic_count_bits(refill);
    const size_t remaining=(size_t)(in->end-in->at);
    if (remaining<bytes) return 0;
    __m128i words;
    if (remaining>=16) {
        words=numerical_load16(in->at);
    } else {
        uint16_t tail[8]={0};
        memcpy(tail,in->at,bytes);
        words=numerical_load16(tail);
    }
    in->at+=bytes;
    const __m128i shuffle=numerical_load16(qxj_refill_masks[refill]);
    const NumericalLanes added=_mm256_cvtepu16_epi32(_mm_shuffle_epi8(words,shuffle));
    const NumericalLanes normal=_mm256_cmpeq_epi32(_mm256_srli_epi32(*state,16),_mm256_setzero_si256());
    *state=_mm256_or_si256(_mm256_blendv_epi8(*state,_mm256_slli_epi32(*state,16),normal),added);
    return 1;
}

static QLI_NUMERIC_TARGET inline void diagonal_transpose(__m128i row[16])
{
    __m128i pair[16],four[16];
    for (unsigned i=0;i<16;i+=2) {
        pair[i]=_mm_unpacklo_epi8(row[i],row[i+1]);
        pair[i+1]=_mm_unpackhi_epi8(row[i],row[i+1]);
    }
    for (unsigned i=0;i<16;i+=4)
        for (unsigned k=0;k<2;++k) {
            four[i+2*k]=_mm_unpacklo_epi16(pair[i+k],pair[i+k+2]);
            four[i+2*k+1]=_mm_unpackhi_epi16(pair[i+k],pair[i+k+2]);
        }
    for (unsigned i=0;i<16;i+=8)
        for (unsigned k=0;k<4;++k) {
            pair[i+2*k]=_mm_unpacklo_epi32(four[i+k],four[i+k+4]);
            pair[i+2*k+1]=_mm_unpackhi_epi32(four[i+k],four[i+k+4]);
        }
    for (unsigned k=0;k<8;++k) {
        row[2*k]=_mm_unpacklo_epi64(pair[k],pair[k+8]);
        row[2*k+1]=_mm_unpackhi_epi64(pair[k],pair[k+8]);
    }
}

static QLI_NUMERIC_TARGET inline __m128i diagonal_codes(__m128i error)
{
    const __m128i codes=_mm_setr_epi8(6,4,4,4,2,0,1,3,3,3,5,0,0,0,0,0);
    const __m128i clamped=_mm_min_epi8(_mm_max_epi8(error,_mm_set1_epi8(-5)),_mm_set1_epi8(5));
    return _mm_shuffle_epi8(codes,_mm_add_epi8(clamped,_mm_set1_epi8(5)));
}

static QLI_NUMERIC_TARGET void diagonal_store(uint8_t *out,__m128i values[16],unsigned w,unsigned rows,int t)
{
    diagonal_transpose(values);
    for (unsigned k=0;k<rows;++k) {
        int x=t-4*(int)k;
        int lo=x<0 ? -x : 0, hi=x+16>(int)w ? (int)w-x : 16;
        if (lo>=hi) continue;
        uint8_t *row=out+(size_t)k*w;
        if (hi-lo==16) numerical_store16((row+x),values[k]);
        else {
            uint8_t part[16];
            numerical_store16(part,values[k]);
            memcpy(row+(x+lo),part+lo,(size_t)(hi-lo));
        }
    }
}

typedef struct { NumericalLanes lo,hi; } FusedPair;
static QLI_NUMERIC_TARGET inline __m128i fused_pack_u8(NumericalLanes lo,NumericalLanes hi)
{
    const __m128i a=_mm_packus_epi32(numerical_lo(lo),numerical_hi(lo));
    const __m128i b=_mm_packus_epi32(numerical_lo(hi),numerical_hi(hi));
    return _mm_packus_epi16(a,b);
}

static QLI_NUMERIC_TARGET inline __m128i fused_symbols(NumericalLanes lo,NumericalLanes hi)
{
    const NumericalLanes byte=_mm256_set1_epi32(255);
    return fused_pack_u8(_mm256_and_si256(lo,byte),_mm256_and_si256(hi,byte));
}

static QLI_NUMERIC_TARGET inline int fused_invalid_routes(NumericalLanes a,NumericalLanes b)
{
    return !_mm256_testz_si256(_mm256_or_si256(a,b),_mm256_set1_epi32(ROUTE_SKIP));
}

static QLI_NUMERIC_TARGET inline NumericalLanes fused_all_lanes(void)
{
    return _mm256_set1_epi32(-1);
}

static QLI_NUMERIC_TARGET inline int fused_all_states(NumericalLanes state,uint32_t value)
{
    return _mm256_movemask_epi8(_mm256_cmpeq_epi32(state,_mm256_set1_epi32((int)value))) == -1;
}

static QLI_NUMERIC_TARGET void fused_coefficients(const int8_t *fitted,__m128i lut[11])
{
    static const int8_t ordinary[8][4]={{12,12,-12,0},{12,0,0,0},{0,12,0,0},
        {3,3,3,3},{9,3,0,0},{4,4,0,4},{6,3,0,3},{9,9,-6,0}};
    for (unsigned k=0;k<10;++k) {
        int8_t column[16]={0};
        if (k<4) for (unsigned m=0;m<8;++m) column[m]=ordinary[m][k];
        if (fitted) { column[8]=fitted[k]; column[9]=fitted[10+k]; }
        lut[k]=numerical_load16(column);
    }
    uint8_t wide[16]={0};
    if (fitted) {
        for (unsigned m=0;m<2;++m) {
            unsigned positive=0;
            for (unsigned k=0;k<10;++k) {
                int weight=fitted[10*m+k];
                if (weight>0) positive+=(unsigned)weight;
            }
            wide[8+m]=positive>128 ? 255 : 0;
        }
    }
    lut[10]=numerical_load16(wide);
}

static QLI_NUMERIC_TARGET inline __m128i fused_predict_bias(__m128i mode)
{
    return _mm_shuffle_epi8(_mm_setr_epi8(0,0,0,6,6,4,6,6,16,16,0,0,0,0,0,0),mode);
}

typedef struct {
    NumericalLanes wn, diagonals, bias;
    __m128i gradient;
} WordPredict;

static QLI_NUMERIC_TARGET inline NumericalLanes word_pair(__m128i a,__m128i b)
{
    return numerical_pair(_mm_unpackhi_epi8(a,b),_mm_unpacklo_epi8(a,b));
}

static QLI_NUMERIC_TARGET inline WordPredict word_predict_setup(const __m128i coefficient[11],__m128i mode)
{
    const __m128i w=_mm_shuffle_epi8(coefficient[0],mode);
    const __m128i n=_mm_shuffle_epi8(coefficient[1],mode);
    const __m128i nw=_mm_shuffle_epi8(coefficient[2],mode);
    const __m128i ne=_mm_shuffle_epi8(coefficient[3],mode);
    const __m128i bias=fused_predict_bias(mode), zero=_mm_setzero_si128();
    const NumericalLanes words=_mm256_cvtepu8_epi16(bias);
    return (WordPredict){word_pair(w,n),word_pair(nw,ne),words,_mm_cmpeq_epi8(mode,zero)};
}

static QLI_NUMERIC_TARGET inline __m128i finish_prediction(
    NumericalLanes sum,NumericalLanes scale,__m128i w,__m128i n,__m128i gradient)
{
    const NumericalLanes q=_mm256_mulhi_epu16(_mm256_max_epi16(sum,_mm256_setzero_si256()),scale);
    const __m128i raw=_mm_packus_epi16(numerical_lo(q),numerical_hi(q));
    const __m128i clamped=_mm_min_epu8(_mm_max_epu8(raw,_mm_min_epu8(w,n)),_mm_max_epu8(w,n));
    return _mm_blendv_epi8(raw,clamped,gradient);
}

static QLI_NUMERIC_TARGET inline __m128i word_prediction(const WordPredict *p,__m128i w,__m128i n,__m128i nw,__m128i ne)
{
    const NumericalLanes near_terms=_mm256_maddubs_epi16(word_pair(w,n),p->wn);
    const NumericalLanes far_terms=_mm256_maddubs_epi16(word_pair(nw,ne),p->diagonals);
    return finish_prediction(_mm256_add_epi16(_mm256_add_epi16(near_terms,far_terms),p->bias),
                             _mm256_set1_epi16(5462),w,n,p->gradient);
}

static QLI_NUMERIC_TARGET inline __m128i fused_above2(__m128i lag,const uint8_t *above,
                                 ptrdiff_t w,ptrdiff_t step,int dx,unsigned wide)
{
    __m128i tap=_mm_slli_si128(lag,2);
    if (wide&1) tap=_mm_insert_epi8(tap,above[step-w+dx],0);
    if (wide&2) tap=_mm_insert_epi8(tap,above[step-4+dx],1);
    return tap;
}

typedef struct {
    WordPredict first;
    NumericalLanes extra[3];
    __m128i mode;
    int wide_sum;
} FusedPredict;

static QLI_NUMERIC_TARGET FusedPredict fused_predict_setup(const __m128i lut[11],__m128i mode)
{
    FusedPredict p;
    p.first=word_predict_setup(lut,mode);
    for (unsigned k=0;k<3;++k)
        p.extra[k]=word_pair(_mm_shuffle_epi8(lut[4+2*k],mode),_mm_shuffle_epi8(lut[5+2*k],mode));
    p.mode=mode;
    p.wide_sum=_mm_movemask_epi8(_mm_shuffle_epi8(lut[10],mode));
    return p;
}

static QLI_NUMERIC_TARGET inline NumericalLanes wide_prediction_sum(const NumericalLanes terms[6])
{
    NumericalLanes low=_mm256_setzero_si256(),high=_mm256_setzero_si256();
    for (unsigned k=0;k<6;++k) {
        low=_mm256_add_epi32(low,_mm256_cvtepi16_epi32(numerical_lo(terms[k])));
        high=_mm256_add_epi32(high,_mm256_cvtepi16_epi32(numerical_hi(terms[k])));
    }
    return _mm256_permute4x64_epi64(_mm256_packs_epi32(low,high),0xd8);
}

static QLI_NUMERIC_TARGET inline __m128i fused_prediction(const FusedPredict *p,const __m128i hist[9],
                    __m128i west,__m128i north,__m128i nw,__m128i ne,
                    const uint8_t *above,unsigned w,unsigned step,__m128i wide,int interior)
{
    const unsigned bits=interior ? 3u : (unsigned)_mm_movemask_epi8(wide);
    if (!bits && !p->wide_sum) return word_prediction(&p->first,west,north,nw,ne);

    const NumericalLanes zero=_mm256_setzero_si256();
    NumericalLanes terms[6]={
        _mm256_maddubs_epi16(word_pair(west,north),p->first.wn),
        _mm256_maddubs_epi16(word_pair(nw,ne),p->first.diagonals),
        zero,zero,zero,p->first.bias
    };
    if (bits) {
        __m128i nww=_mm_slli_si128(hist[5],1);
        if (bits&1) nww=_mm_insert_epi8(nww,above[step-2],0);
        const __m128i nnw=fused_above2(hist[8],above,w,step,-1,bits);
        const __m128i nn=fused_above2(hist[7],above,w,step,0,bits);
        const __m128i nne=fused_above2(hist[6],above,w,step,1,bits);
        const __m128i nnee=fused_above2(hist[5],above,w,step,2,bits);
        terms[2]=_mm256_maddubs_epi16(word_pair(hist[1],nww),p->extra[0]);
        terms[3]=_mm256_maddubs_epi16(word_pair(nnw,nn),p->extra[1]);
        terms[4]=_mm256_maddubs_epi16(word_pair(nne,nnee),p->extra[2]);
    }
    NumericalLanes sum;
    if (p->wide_sum) sum=wide_prediction_sum(terms);
    else {
        sum=_mm256_add_epi16(terms[0],terms[1]);
        for (unsigned k=2;k<6;++k) sum=_mm256_add_epi16(sum,terms[k]);
    }
    const NumericalLanes scale=_mm256_blendv_epi8(_mm256_set1_epi16(5462),_mm256_set1_epi16(2048),
                                                 _mm256_cvtepi8_epi16(wide));
    return finish_prediction(sum,scale,west,north,p->first.gradient);
}

static QLI_NUMERIC_TARGET __m128i fused_map(const uint8_t map[64],__m128i key,unsigned guides)
{
    if (guides==1) return _mm_shuffle_epi8(numerical_load16(map),key);
    const __m128i lo=_mm_and_si128(key,_mm_set1_epi8(15));
    const __m128i hi=_mm_and_si128(key,_mm_set1_epi8(48));
    __m128i value=_mm_shuffle_epi8(numerical_load16(map),lo);
    for (unsigned k=1;k<4;++k) {
        __m128i v=_mm_shuffle_epi8(numerical_load16((map+16*k)),lo);
        value=_mm_blendv_epi8(value,v,_mm_cmpeq_epi8(hi,_mm_set1_epi8((char)(16*k))));
    }
    return value;
}

static QLI_NUMERIC_TARGET void fused_stage(const Diagonal *d,unsigned base,unsigned rows,int t,
                        __m128i groups[16],__m128i known[16],int banded)
{
    const Work *work=d->work;
    const __m128i skip=_mm_set1_epi8((char)CAUSAL_SKIP),zero=_mm_setzero_si128();
    const __m128i index=_mm_setr_epi8(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
    for (unsigned k=0;k<16;++k) {
        groups[k]=skip;
        known[k]=zero;
        int x=t-4*(int)k;
        unsigned begin=x<0 ? 0 : (unsigned)x;
        int limit=x+16<(int)d->w ? x+16 : (int)d->w;
        if (k>=rows || limit<=(int)begin) continue;
        unsigned end=(unsigned)limit,y=base+k;
        size_t row=(size_t)y*d->w;
        __m128i key=zero;
        if (work->guides) {
            unsigned off=(unsigned)((int)begin-x),n=end-begin;
            uint8_t first[16]={0},second[16]={0};
            size_t guide_row=banded ? (size_t)k*d->w : row;
            const uint8_t *a=work->earlier[0]+guide_row+begin;
            if (n==16) key=numerical_load16(a);
            else {
                memcpy(first+off,a,n);
                key=numerical_load16(first);
            }
            if (work->guides==2) {
                const uint8_t *b=work->earlier[1]+guide_row+begin;
                __m128i other;
                if (n==16) other=numerical_load16(b);
                else {
                    memcpy(second+off,b,n);
                    other=numerical_load16(second);
                }
                key=_mm_add_epi8(_mm_sub_epi8(_mm_slli_epi16(key,3),key),other);
            }
        }
        for (unsigned at=begin;at<end;) {
            size_t p=row+at;
            unsigned len=(unsigned)min_size(end-at,256-p%256);
            int flat=flag(d->flat,p/256);
            int start=(int)at-x,stop=start+(int)len;
            __m128i mask=_mm_and_si128(_mm_cmpgt_epi8(index,_mm_set1_epi8((char)(start-1))),
                                      _mm_cmpgt_epi8(_mm_set1_epi8((char)stop),index));
            if (flat) known[k]=_mm_blendv_epi8(known[k],_mm_set1_epi8((char)work->raster[p/256]),mask);
            else {
                __m128i id=work->guides ? fused_map(d->mapping,key,work->guides) : zero;
                groups[k]=_mm_blendv_epi8(groups[k],id,mask);
            }
            at+=len;
        }
    }
    diagonal_transpose(groups);
    diagonal_transpose(known);
}

static QLI_NUMERIC_TARGET inline NumericalLanes fused_activity_lanes(NumericalLanes v,unsigned above)
{
    __m128i a=numerical_lo(v),b=numerical_hi(v);
    return numerical_pair(_mm_alignr_epi8(b,a,14),
                           _mm_insert_epi16(_mm_slli_si128(a,2),(int)above,0));
}

static QLI_NUMERIC_TARGET inline NumericalLanes fused_update_activity(NumericalLanes recent,NumericalLanes older,
                                                  __m128i error,unsigned top)
{
    const NumericalLanes diagonal=fused_activity_lanes(_mm256_max_epu16(recent,older),top);
    const NumericalLanes e=_mm256_cvtepi8_epi16(error);
    const NumericalLanes zig=_mm256_xor_si256(_mm256_slli_epi16(e,1),_mm256_srai_epi16(e,15));
    return _mm256_avg_epu16(recent,_mm256_avg_epu16(_mm256_slli_epi16(zig,3),diagonal));
}


static QLI_NUMERIC_TARGET inline NumericalLanes activity_bins(__m128i activity)
{
    NumericalLanes a=_mm256_cvtepu16_epi32(activity);
    const NumericalLanes score=_mm256_add_epi32(_mm256_mullo_epi32(a,a),_mm256_set1_epi32(1));
    const NumericalLanes exp=_mm256_srli_epi32(_mm256_castps_si256(_mm256_cvtepi32_ps(score)),23);
    return _mm256_min_epi32(_mm256_sub_epi32(exp,_mm256_set1_epi32(127)),_mm256_set1_epi32(17));
}

static QLI_NUMERIC_TARGET inline NumericalLanes fused_model(const uint32_t *route,__m128i group,
                       __m128i activity,NumericalLanes *live)
{
    NumericalLanes bin=activity_bins(activity);
    NumericalLanes id=_mm256_cvtepu8_epi32(group);
    *live=_mm256_cmpgt_epi32(_mm256_set1_epi32(32),id);
    NumericalLanes addr=_mm256_or_si256(_mm256_slli_epi32(id,5),bin);
    return numerical_gather8(route,addr,*live);
}

static QLI_NUMERIC_TARGET inline NumericalLanes fused_entries(const uint32_t *table,NumericalLanes state,NumericalLanes model,
                                           NumericalLanes live)
{
    const NumericalLanes mask=_mm256_set1_epi32(CAUSAL_SCALE-1);
    NumericalLanes off=_mm256_and_si256(model,_mm256_set1_epi32(ROUTE_OFFSET_MASK));
    NumericalLanes addr=_mm256_or_si256(off,_mm256_and_si256(state,mask));
    return numerical_gather8(table,addr,live);
}

static QLI_NUMERIC_TARGET inline NumericalLanes full_word_next(NumericalLanes state,NumericalLanes entry)
{
    const NumericalLanes freq=_mm256_add_epi32(_mm256_srli_epi32(entry,20),_mm256_set1_epi32(1));
    const NumericalLanes rel=_mm256_and_si256(_mm256_srli_epi32(entry,8),_mm256_set1_epi32(4095));
    return _mm256_add_epi32(_mm256_mullo_epi32(_mm256_srli_epi32(state,12),freq),rel);
}

static QLI_NUMERIC_TARGET inline int fused_advance(Input *in,NumericalLanes *state,NumericalLanes entry,NumericalLanes live)
{
    const NumericalLanes zero=_mm256_setzero_si256();
    NumericalLanes next=_mm256_blendv_epi8(*state,full_word_next(*state,entry),live);
    unsigned refill=(unsigned)_mm256_movemask_ps(_mm256_castsi256_ps(
        _mm256_cmpeq_epi32(_mm256_srli_epi32(next,16),zero)));
    if (refill) {
        if (_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(next,zero)))) return 0;
        if (!qxj_refill(in,&next,refill)) return 0;
    }
    *state=next;
    return 1;
}

static QLI_NUMERIC_TARGET inline unsigned fused_last_activity(NumericalLanes activity)
{
    return (unsigned)_mm_extract_epi16(numerical_hi(activity),7);
}

static QLI_NUMERIC_TARGET inline NumericalLanes fused_keep_activity(NumericalLanes activity,NumericalLanes valid)
{
    return _mm256_and_si256(activity,valid);
}

static QLI_NUMERIC_TARGET inline void fused_positions(unsigned step,unsigned w,unsigned rows,NumericalLanes *active,
                                   __m128i *first,__m128i *last,__m128i *gradient)
{
    const NumericalLanes offsets=_mm256_setr_epi16(0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60);
    const NumericalLanes right=_mm256_set1_epi16((short)(w-1));
    const NumericalLanes x=_mm256_sub_epi16(_mm256_set1_epi16((short)step),offsets);
    const NumericalLanes inside=_mm256_cmpgt_epi16(_mm256_set1_epi16((short)rows),_mm256_srli_epi16(offsets,2));
    *active=_mm256_and_si256(inside,_mm256_andnot_si256(_mm256_cmpgt_epi16(x,right),
                                                       _mm256_cmpgt_epi16(x,_mm256_set1_epi16(-1))));
    const NumericalLanes a=_mm256_cmpeq_epi16(x,_mm256_setzero_si256()), b=_mm256_cmpeq_epi16(x,right);
    const NumericalLanes g=_mm256_cmpgt_epi16(x,_mm256_set1_epi16(1));
    *first=_mm_packs_epi16(numerical_lo(a),numerical_hi(a));
    *last=_mm_packs_epi16(numerical_lo(b),numerical_hi(b));
    *gradient=_mm_packs_epi16(numerical_lo(g),numerical_hi(g));
}

typedef struct { FusedPair ans; NumericalLanes recent,older; __m128i pix[9]; } FusedHistory;

static QLI_NUMERIC_TARGET inline void retain_fitted_lags(FusedHistory *s)
{
    for (unsigned k=8;k>4;--k) s->pix[k]=s->pix[k-1];
}

static QLI_NUMERIC_TARGET inline __attribute__((always_inline)) int fused_block(Input *in,Diagonal *d,const uint32_t *route,FusedHistory *s,uint16_t *north,
                       uint8_t *out,const uint8_t *above,unsigned base,unsigned rows,
                       unsigned t,unsigned steps,const FusedPredict p[4],int banded,const WordPredict word[4])
{
    Work *work=d->work;
    const unsigned w=d->w;
    const __m128i six=_mm_set1_epi8(7);
    __m128i group[16],known[16],samples[16]={0},codes[16]={0};
    fused_stage(d,base,rows,(int)t,group,known,banded);
    for (unsigned j=0;j<steps;++j) {
        unsigned step=t+j;
        NumericalLanes valid16;
        __m128i first,last,gradient;
        fused_positions(step,w,rows,&valid16,&first,&last,&gradient);
        __m128i valid=_mm_packs_epi16(numerical_lo(valid16),numerical_hi(valid16));
        const int coded=!banded || _mm_movemask_epi8(group[j])!=65535;
        const NumericalLanes clear=_mm256_setzero_si256();
        NumericalLanes m_a,m_b,live_a=clear,live_b=clear,ea=clear,eb=clear;
        if (coded) {
            m_a=fused_model(route,group[j],numerical_lo(s->recent),&live_a);
            m_b=fused_model(route,_mm_srli_si128(group[j],8),numerical_hi(s->recent),&live_b);
            if (!work->directional) {
                if (fused_invalid_routes(m_a,m_b)) return 0;
                ea=fused_entries(work->tables,s->ans.lo,m_a,live_a);
                eb=fused_entries(work->tables,s->ans.hi,m_b,live_b);
            }
        }
        __m128i west=s->pix[0],n=_mm_slli_si128(s->pix[3],1),
                 nw=_mm_slli_si128(s->pix[4],1),ne=_mm_slli_si128(s->pix[2],1);
        if (above) {
            unsigned ax=(unsigned)min_size(step,w-1);
            n=_mm_insert_epi8(n,above[ax],0);
            nw=_mm_insert_epi8(nw,above[ax ? ax-1 : 0],0);
            ne=_mm_insert_epi8(ne,above[ax+1<w ? ax+1 : ax],0);
        } else {
            if (!step) west=_mm_insert_epi8(west,128,0);
            unsigned a=(unsigned)_mm_extract_epi8(west,0);
            n=_mm_insert_epi8(n,a,0);
            nw=_mm_insert_epi8(nw,a,0);
            ne=_mm_insert_epi8(ne,a,0);
        }
        west=_mm_blendv_epi8(west,n,first);
        nw=_mm_blendv_epi8(nw,n,first);
        ne=_mm_blendv_epi8(ne,n,last);
        unsigned top=0;
        if (step<w) top=north[step+2]>north[step+3] ? north[step+2] : north[step+3];
        __m128i prediction;
        if (word) prediction=word_prediction(word+j/4,west,n,nw,ne);
        else {
            const FusedPredict *pred=p+j/4;
            __m128i wide=_mm_and_si128(valid,_mm_cmpgt_epi8(pred->mode,six));
            prediction=fused_prediction(pred,s->pix,west,n,nw,ne,above,w,step,wide,0);
        }
        if (coded && work->directional) {
            const __m128i all=_mm_set1_epi8(-1);
            const __m128i mid_rows=base ? all : _mm_slli_si128(all,1);
            const __m128i gradient_rows=base ? all : _mm_slli_si128(all,2);
            const __m128i has_mid=_mm_andnot_si128(first,mid_rows);
            __m128i has_gradient=_mm_and_si128(gradient_rows,gradient);
            __m128i shift=direction_indices(west,n,nw,prediction,has_mid,has_gradient);
            m_a=direction_model(m_a,shift);
            m_b=direction_model(m_b,_mm_srli_si128(shift,8));
            if (fused_invalid_routes(m_a,m_b)) return 0;
            ea=fused_entries(work->tables,s->ans.lo,m_a,live_a);
            eb=fused_entries(work->tables,s->ans.hi,m_b,live_b);
        }
        __m128i live=fused_symbols(live_a,live_b);
        __m128i symbols=fused_symbols(ea,eb);
        __m128i error=_mm_and_si128(valid,_mm_blendv_epi8(known[j],symbols,live));
        __m128i sample=_mm_add_epi8(prediction,error);
        NumericalLanes recent=fused_update_activity(s->recent,s->older,error,top);
        s->older=s->recent;
        s->recent=fused_keep_activity(recent,valid16);
        if (rows==16 && step-60<w) {
            north[step-60]=(uint16_t)fused_last_activity(s->recent);
        }
        for (unsigned k=8;k;k--) s->pix[k]=s->pix[k-1];
        s->pix[0]=samples[j]=sample;
        if (work->error) codes[j]=diagonal_codes(error);
        if (coded && (!fused_advance(in,&s->ans.lo,ea,live_a) ||
                       !fused_advance(in,&s->ans.hi,eb,live_b))) return 0;
    }
    diagonal_store(out,samples,w,rows,(int)t);
    if (work->error) diagonal_store(work->error+(banded ? 0 : (size_t)base*w),codes,w,rows,(int)t);
    return 1;
}

static QLI_NUMERIC_TARGET void known_marks(const uint8_t *flat,unsigned w,unsigned base,uint8_t *kind)
{
    const unsigned blocks=(w+60+15)/16;
    int16_t delta[MAX_WAVE_BLOCKS+1]={0};
    uint16_t edge[MAX_WAVE_BLOCKS]={0};
    const size_t begin=(size_t)base*w,end=begin+16*(size_t)w;
    for (size_t k=begin/256;k<=(end-1)/256;++k) {
        if (!(flat[k/8]&(1u<<(k%8)))) continue;
        size_t lo=k*256,hi=lo+256;
        if (lo<begin) lo=begin;
        if (hi>end) hi=end;
        while (lo<hi) {
            unsigned lane=(unsigned)((lo-begin)/w),x=(unsigned)((lo-begin)%w);
            unsigned run=w-x;
            if (run>hi-lo) run=(unsigned)(hi-lo);
            unsigned a=x+4*lane,b=a+run-1;
            unsigned first=a/16,last=b/16;
            if (first==last) edge[first]+=(uint16_t)run;
            else {
                edge[first]+=(uint16_t)(16-a%16);
                edge[last]+=(uint16_t)(1+b%16);
                delta[first+1]+=16;
                delta[last]-=16;
            }
            lo+=run;
        }
    }
    int inner=0;
    for (unsigned k=0;k<blocks;++k) {
        inner+=delta[k];
        unsigned n=(unsigned)(inner+edge[k]);
        kind[k]=(uint8_t)(n==0 ? BLOCK_CODED : n==256 ? BLOCK_KNOWN : BLOCK_MIXED);
    }
}

static QLI_NUMERIC_TARGET void known_rows(const uint8_t *values,unsigned w,unsigned base,unsigned t,__m128i rows[16])
{
    const __m128i index=_mm_setr_epi8(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
    for (unsigned lane=0;lane<16;++lane) {
        size_t pos=(size_t)(base+lane)*w+t-4*lane;
        const size_t first=pos/256;
        __m128i a=_mm_set1_epi8((char)values[first]);
        unsigned n=256-(unsigned)(pos%256);
        if (n<16) {
            __m128i b=_mm_set1_epi8((char)values[first+1]);
            __m128i prefix=_mm_cmpgt_epi8(_mm_set1_epi8((char)n),index);
            a=_mm_blendv_epi8(b,a,prefix);
        }
        rows[lane]=a;
    }
}

static QLI_NUMERIC_TARGET inline NumericalLanes paired_insert(NumericalLanes state,NumericalLanes mask,
                                    __m128i words,unsigned index)
{
    __m128i order=numerical_load16(qxj_refill_masks[index]);
    const __m128i packed=_mm_shuffle_epi8(words,order);
    NumericalLanes added=_mm256_cvtepu16_epi32(packed);
    return _mm256_or_si256(_mm256_blendv_epi8(state,_mm256_slli_epi32(state,16),mask),added);
}

static QLI_NUMERIC_TARGET inline int paired_refill(Input *in,FusedPair *state)
{
    const NumericalLanes zero=_mm256_setzero_si256();
    NumericalLanes a=_mm256_cmpeq_epi32(_mm256_srli_epi32(state->lo,16),zero);
    NumericalLanes b=_mm256_cmpeq_epi32(_mm256_srli_epi32(state->hi,16),zero);
    unsigned lo=(unsigned)_mm256_movemask_ps(_mm256_castsi256_ps(a));
    unsigned hi=(unsigned)_mm256_movemask_ps(_mm256_castsi256_ps(b));
    if (!(lo|hi)) return 1;
    NumericalLanes invalid=_mm256_or_si256(_mm256_cmpeq_epi32(state->lo,zero),
                                  _mm256_cmpeq_epi32(state->hi,zero));
    if (_mm256_movemask_ps(_mm256_castsi256_ps(invalid))) return 0;
    unsigned first=2u*(unsigned)qlic_count_bits(lo),bytes=first+2u*(unsigned)qlic_count_bits(hi);
    size_t available=(size_t)(in->end-in->at);
    if (available<bytes) return 0;
    uint8_t tail[32]={0};
    const uint8_t *source=in->at;
    if (available<32) {
        memcpy(tail,source,bytes);
        source=tail;
    }
    state->lo=paired_insert(state->lo,a,numerical_load16(source),lo);
    state->hi=paired_insert(state->hi,b,numerical_load16((source+first)),hi);
    in->at+=bytes;
    return 1;
}

static QLI_NUMERIC_TARGET void byte_live_stage(const Diagonal *d,unsigned base,unsigned t,__m128i groups[16],int banded)
{
    const Work *work=d->work;
    unsigned guides=work->guides;
    if (!guides) {
        memset(groups,0,16*sizeof(*groups));
        return;
    }
    for (unsigned lane=0;lane<16;++lane) {
        unsigned x=t-4*lane,y=base+lane;
        size_t pos=(size_t)(banded ? lane : y)*d->w+x;
        __m128i key=numerical_load16(work->earlier[0]+pos);
        if (guides==2) {
            __m128i other=numerical_load16((work->earlier[1]+pos));
            key=_mm_add_epi8(_mm_sub_epi8(_mm_slli_epi16(key,3),key),other);
        }
        groups[lane]=fused_map(d->mapping,key,guides);
    }
    diagonal_transpose(groups);
}

static QLI_NUMERIC_TARGET inline NumericalLanes byte_live_model(const uint32_t *route,__m128i group,__m128i activity)
{
    NumericalLanes bin=activity_bins(activity);
    NumericalLanes id=_mm256_slli_epi32(_mm256_cvtepu8_epi32(group),5);
    return numerical_gather8(route,_mm256_or_si256(id,bin),_mm256_set1_epi32(-1));
}

static QLI_NUMERIC_TARGET void byte_live_store(uint8_t *out,__m128i values[16],unsigned w,unsigned t)
{
    diagonal_transpose(values);
    for (unsigned k=0;k<16;++k)
        numerical_store16((out+(size_t)k*w+t-4*k),values[k]);
}

static QLI_NUMERIC_TARGET __attribute__((noinline)) int full_live_block(Input *in,Diagonal *d,const uint32_t *route,FusedHistory *s,
                           uint16_t *north,uint8_t *out,const uint8_t *above,
                           unsigned base,unsigned t,const FusedPredict p[4],int known,int banded,const WordPredict word[4])
{
    Work *work=d->work;
    const unsigned w=d->w;
    const __m128i all=_mm_set1_epi8((char)255);
    const NumericalLanes live=fused_all_lanes();
    __m128i groups[16],samples[16],codes[16];
    if (known) {
        known_rows(work->raster,w,base,t,groups);
        diagonal_transpose(groups);
    } else byte_live_stage(d,base,t,groups,banded);
    for (unsigned j=0;j<16;++j) {
        const unsigned step=t+j;
        NumericalLanes m_a,m_b;
        if (!known) {
            m_a=byte_live_model(route,groups[j],numerical_lo(s->recent));
            m_b=byte_live_model(route,_mm_srli_si128(groups[j],8),numerical_hi(s->recent));
        }
        const __m128i west=s->pix[0];
        const __m128i n=_mm_insert_epi8(_mm_slli_si128(s->pix[3],1),above[step],0);
        const __m128i nw=_mm_insert_epi8(_mm_slli_si128(s->pix[4],1),above[step-1],0);
        const __m128i ne=_mm_insert_epi8(_mm_slli_si128(s->pix[2],1),above[step+1],0);
        __m128i pred;
        if (word) pred=word_prediction(word+j/4,west,n,nw,ne);
        else {
            const FusedPredict *fit=p+j/4;
            const __m128i wide=_mm_cmpgt_epi8(fit->mode,_mm_set1_epi8(7));
            pred=fused_prediction(fit,s->pix,west,n,nw,ne,above,w,step,wide,0);
        }
        __m128i error;
        if (known) error=groups[j];
        else {
            NumericalLanes ea,eb;
            if (work->directional) {
                __m128i shift=direction_indices(west,n,nw,pred,all,all);
                m_a=direction_model(m_a,shift);
                m_b=direction_model(m_b,_mm_srli_si128(shift,8));
            }
            if (fused_invalid_routes(m_a,m_b)) return 0;
            ea=fused_entries(work->tables,s->ans.lo,m_a,live);
            eb=fused_entries(work->tables,s->ans.hi,m_b,live);
            FusedPair next={full_word_next(s->ans.lo,ea),full_word_next(s->ans.hi,eb)};
            if (!paired_refill(in,&next)) return 0;
            s->ans=next;
            error=fused_symbols(ea,eb);
        }
        unsigned top=north[step+2]>north[step+3] ? north[step+2] : north[step+3];
        const NumericalLanes recent=fused_update_activity(s->recent,s->older,error,top);
        s->older=s->recent;
        s->recent=recent;
        north[step-60]=(uint16_t)fused_last_activity(recent);
        if (work->stencil) retain_fitted_lags(s);
        for (unsigned k=4;k;k--) s->pix[k]=s->pix[k-1];
        s->pix[0]=samples[j]=_mm_add_epi8(pred,error);
        if (work->error) codes[j]=diagonal_codes(error);
    }
    byte_live_store(out,samples,w,t);
    if (work->error) byte_live_store(work->error+(banded ? 0 : (size_t)base*w),codes,w,t);
    return 1;
}

static QLI_NUMERIC_TARGET void fused_route(uint32_t route[32*32],const uint8_t *source,unsigned count,int directional)
{
    for (unsigned i=0;i<32;++i)
        for (unsigned k=0;k<32;++k)
            if (i<count && k<CAUSAL_CONTEXTS) {
                unsigned key=i*CAUSAL_CONTEXTS+k;
                route[32*i+k]=directional ? direction_word(source+3*key) : route_offset(source[key]);
            } else {
                route[32*i+k]=directional ? DIRECTION_ROUTE_SKIP : ROUTE_SKIP;
            }
}

static QLI_NUMERIC_TARGET int diagonal_fused(Input *in,Diagonal *d,uint32_t states[16],
                               uint8_t *out,unsigned group_count)
{
    Work *work=d->work;
    unsigned w=d->w,h=d->h;
    uint16_t *north=(uint16_t *)work->causal_aux;
    memset(north,0,2*(size_t)(w+3));
    uint32_t route[32*32];
    fused_route(route,d->route,group_count,work->directional);
    const __m128i selectors[4]={
        _mm_setr_epi8(4,3,3,3,3,2,2,2,2,1,1,1,1,0,0,0),
        _mm_setr_epi8(4,4,3,3,3,3,2,2,2,2,1,1,1,1,0,0),
        _mm_setr_epi8(4,4,4,3,3,3,3,2,2,2,2,1,1,1,1,0),
        _mm_setr_epi8(4,4,4,4,3,3,3,3,2,2,2,2,1,1,1,1)
    };
    __m128i coefficient[11];
    fused_coefficients(work->stencil ? work->stencil_fit : NULL,coefficient);
    FusedPair ans={numerical_load32(states),
                   numerical_load32((states+8))};
    for (unsigned base=0;base<h;base+=16) {
        unsigned rows=(unsigned)min_size(16,h-base),end=w+4*(rows-1);
        uint8_t *output=out+(size_t)base*w;
        const uint8_t *above=base ? output-w : NULL;
        const uint8_t *modes=work->modes+(size_t)(base/16)*d->mode_cols;
        FusedHistory history={0};
        history.ans=ans;
        uint8_t dirty[MAX_WAVE_BLOCKS];
        const int full_band=above && rows==16 && w>80 && !work->stencil && !work->directional;
        if (full_band) known_marks(d->flat,w,base,dirty);
        for (unsigned t=0;t<end;t+=16) {
            uint8_t lut[16]={0};
            for (int k=0;k<5;++k) {
                int tile=(int)(t/16)+k-4;
                lut[k]=modes[tile<0 ? 0 : min_size((unsigned)tile,d->mode_cols-1)];
            }
            __m128i mode_lut=numerical_load16(lut);
            FusedPredict prep[4];
            for (unsigned k=0;k<4;++k)
                prep[k]=fused_predict_setup(coefficient,_mm_shuffle_epi8(mode_lut,selectors[k]));
            unsigned steps=(unsigned)min_size(16,end-t);
            int ok;
            if (full_band && t>=64 && t+16<w && dirty[t/16]!=BLOCK_MIXED) {
                ok=full_live_block(in,d,route,&history,north,output,above,base,t,prep,
                                   dirty[t/16]==BLOCK_KNOWN,0,NULL);
            } else ok=fused_block(in,d,route,&history,north,output,above,base,rows,t,steps,prep,0,NULL);
            if (!ok) return 0;
        }
        ans=history.ans;
    }
    numerical_store32(states,ans.lo);
    numerical_store32((states+8),ans.hi);
    return 1;
}

typedef struct BandPlane {
    Input input;
    CausalModels book;
    Diagonal d;
    FusedPair ans;
    uint8_t mapping[64];
    uint32_t route[32*32];
} BandPlane;

static QLI_NUMERIC_TARGET void prepare_band_plane(Work *work,const Input *in,const CausalModels *book,
                    const Diagonal *d,const uint8_t map[64],unsigned old_count,const uint32_t ans[DIAGONAL_ROWS])
{
    BandPlane *p=work->defer;
    p->input=*in;
    p->book=*book;
    memcpy(p->mapping,map,sizeof(p->mapping));
    p->d=*d;
    p->d.mapping=p->mapping;
    p->d.route=NULL;
    fused_route(p->route,d->route,old_count,work->directional);
    memcpy(&p->ans.lo,ans,32);
    memcpy(&p->ans.hi,ans+8,32);
}

typedef struct {
    FusedHistory history;
    BandPlane *plane;
    uint8_t *pixels;
    const uint8_t *above;
    const __m128i *stencil;
    unsigned base,rows,t,end;
    uint8_t blocks[MAX_WAVE_BLOCKS];
} ChromaBand;

typedef struct {
    union { WordPredict word; FusedPredict fit; } predict[4];
    unsigned fitted;
    __m128i group[16],sample[16],code[16];
} ChromaBlock;

static QLI_NUMERIC_TARGET void chroma_begin(ChromaBand *b,BandPlane *p,uint8_t *out,const uint8_t *above,unsigned base)
{
    memset(&b->history,0,sizeof(b->history));
    b->history.ans=p->ans;
    b->plane=p;
    b->pixels=out;
    b->above=above;
    b->base=base;
    b->rows=(unsigned)min_size(16,p->d.h-base);
    b->t=0;
    b->end=p->d.w+4*(b->rows-1);
    if (above && b->rows==16 && p->d.w>80)
        known_marks(p->d.flat,p->d.w,base,b->blocks);
}

static QLI_NUMERIC_TARGET unsigned chroma_kind_at(const ChromaBand *b,unsigned t)
{
    if (b->above && b->rows==16 && t>=64 && t+16<b->plane->d.w)
        return b->blocks[t/16];
    return BLOCK_MIXED;
}

static QLI_NUMERIC_TARGET unsigned chroma_kind(const ChromaBand *b)
{
    return chroma_kind_at(b,b->t);
}

static QLI_NUMERIC_TARGET void chroma_modes(const ChromaBand *b,__m128i mode[4])
{
    static const int8_t selectors[4][16]={
        {4,3,3,3,3,2,2,2,2,1,1,1,1,0,0,0},
        {4,4,3,3,3,3,2,2,2,2,1,1,1,1,0,0},
        {4,4,4,3,3,3,3,2,2,2,2,1,1,1,1,0},
        {4,4,4,4,3,3,3,3,2,2,2,2,1,1,1,1}
    };
    const Diagonal *d=&b->plane->d;
    const uint8_t *modes=d->work->modes+(size_t)(b->base/16)*d->mode_cols;
    uint8_t lut[16]={0};
    for (int k=0;k<5;++k) {
        int tile=(int)(b->t/16)+k-4;
        lut[k]=modes[tile<0 ? 0 : min_size((unsigned)tile,d->mode_cols-1)];
    }
    const __m128i table=numerical_load16(lut);
    for (unsigned k=0;k<4;++k)
        mode[k]=_mm_shuffle_epi8(table,numerical_load16(selectors[k]));
}

static QLI_NUMERIC_TARGET unsigned chroma_fitted_groups(const ChromaBand *b,const __m128i mode[4])
{
    unsigned fitted=0;
    if (b->stencil) {
        const __m128i seven=_mm_set1_epi8(7);
        for (unsigned k=0;k<4;++k)
            if (_mm_movemask_epi8(_mm_cmpgt_epi8(mode[k],seven))) fitted|=1u<<k;
    }
    return fitted;
}

static QLI_NUMERIC_TARGET void chroma_prepare(const ChromaBand *b,const __m128i coefficient[11],ChromaBlock *block)
{
    __m128i mode[4];
    chroma_modes(b,mode);
    block->fitted=chroma_fitted_groups(b,mode);
    for (unsigned k=0;k<4;++k) {
        if (block->fitted & (1u<<k))
            block->predict[k].fit=fused_predict_setup(b->stencil,mode[k]);
        else
            block->predict[k].word=word_predict_setup(coefficient,mode[k]);
    }
}

static QLI_NUMERIC_TARGET int zero_block(const uint8_t *values,unsigned w,unsigned base,unsigned t)
{
    size_t pos=(size_t)base*w+t;
    for (unsigned lane=0;lane<16;++lane,pos+=w-4)
        if (values[pos/256] || values[(pos+15)/256]) return 0;
    return 1;
}

static QLI_NUMERIC_TARGET int uniform_history(const FusedHistory *s,const uint8_t *above,
                                  unsigned w,unsigned t,int fitted)
{
    const __m128i value=_mm_shuffle_epi8(s->pix[0],_mm_setzero_si128());
    __m128i diff=_mm_xor_si128(numerical_load16(above+t+1),value);
    for (unsigned k=0;k<(fitted ? 9u : 5u);++k)
        diff=_mm_or_si128(diff,_mm_xor_si128(s->pix[k],value));
    if (fitted) {
        diff=_mm_or_si128(diff,_mm_xor_si128(numerical_load16(above+t-5),value));
        diff=_mm_or_si128(diff,_mm_xor_si128(numerical_load16(above+t-w-1),value));
        diff=_mm_or_si128(diff,_mm_xor_si128(numerical_load16(above+t-w+2),value));
    } else diff=_mm_or_si128(diff,_mm_xor_si128(numerical_load16(above+t-1),value));
    return _mm_testz_si128(diff,diff);
}

static QLI_NUMERIC_TARGET int stable_activity(const FusedHistory *s,const uint16_t *north,unsigned t)
{
    const NumericalLanes change=_mm256_xor_si256(s->older,s->recent);
    if (!_mm256_testz_si256(change,change)) return 0;
    const unsigned head=(unsigned)_mm_extract_epi16(numerical_lo(s->recent),0);
    const unsigned high=2*head,low=high>3 ? high-3 : 0;
    const NumericalLanes top=_mm256_max_epu16(numerical_load32(north+t+2),numerical_load32(north+t+3));
    const NumericalLanes offset=_mm256_sub_epi16(top,_mm256_set1_epi16((short)low));
    const NumericalLanes outside=_mm256_xor_si256(offset,_mm256_min_epu16(offset,_mm256_set1_epi16((short)(high-low))));
    if (!_mm256_testz_si256(outside,outside)) return 0;
    const NumericalLanes next=fused_update_activity(s->recent,s->older,_mm_setzero_si128(),high);
    const NumericalLanes diff=_mm256_xor_si256(next,s->recent);
    return _mm256_testz_si256(diff,diff);
}

static QLI_NUMERIC_TARGET int fill_known_block(ChromaBand *b,uint16_t *north)
{
    const Diagonal *d=&b->plane->d;
    const Work *work=d->work;
    const unsigned w=d->w,t=b->t;
    FusedHistory *s=&b->history;
    if (!uniform_history(s,b->above,w,t,work->stencil) ||
        (d->nonzero_constants && !zero_block(work->raster,w,b->base,t)) ||
        !stable_activity(s,north,t)) return 0;
    numerical_store32(north+t-60,_mm256_set1_epi16((short)fused_last_activity(s->recent)));
    for (unsigned lane=0;lane<16;++lane)
        numerical_store16(b->pixels+t+(size_t)lane*(w-4),s->pix[0]);
    if (work->error)
        for (unsigned lane=0;lane<16;++lane)
            numerical_store16(work->error+t+(size_t)lane*(w-4),_mm_setzero_si128());
    return 1;
}

static QLI_NUMERIC_TARGET __attribute__((noinline)) int chroma_single(ChromaBand *b,const __m128i coefficient[11])
{
    BandPlane *p=b->plane;
    Diagonal *d=&p->d;
    uint16_t *north=(uint16_t *)d->work->causal_aux;
    const unsigned kind=chroma_kind(b);
    if (kind==BLOCK_KNOWN && fill_known_block(b,north)) {
        b->t+=16;
        return 1;
    }
    __m128i mode[4];
    chroma_modes(b,mode);
    if (chroma_fitted_groups(b,mode)) {
        FusedPredict predict[4];
        for (unsigned k=0;k<4;++k) predict[k]=fused_predict_setup(b->stencil,mode[k]);
        unsigned steps=(unsigned)min_size(16,b->end-b->t);
        int ok;
        if (kind==BLOCK_MIXED)
            ok=fused_block(&p->input,d,p->route,&b->history,north,b->pixels,b->above,
                           b->base,b->rows,b->t,steps,predict,1,NULL);
        else
            ok=full_live_block(&p->input,d,p->route,&b->history,north,b->pixels,b->above,
                               b->base,b->t,predict,kind==BLOCK_KNOWN,1,NULL);
        b->t+=16;
        return ok;
    }
    WordPredict predict[4];
    for (unsigned k=0;k<4;++k) predict[k]=word_predict_setup(coefficient,mode[k]);
    int ok;
    if (kind==BLOCK_MIXED) {
        unsigned steps=(unsigned)min_size(16,b->end-b->t);
        ok=fused_block(&p->input,d,p->route,&b->history,north,b->pixels,b->above,
                       b->base,b->rows,b->t,steps,NULL,1,predict);
    } else {
        ok=full_live_block(&p->input,d,p->route,&b->history,north,b->pixels,b->above,
                           b->base,b->t,NULL,kind==BLOCK_KNOWN,1,predict);
    }
    b->t+=16;
    return ok;
}

static QLI_NUMERIC_TARGET inline FusedPair chroma_model(const ChromaBand *b,__m128i group)
{
    const uint32_t *route=b->plane->route;
    NumericalLanes recent=b->history.recent;
    return (FusedPair){
        byte_live_model(route,group,numerical_lo(recent)),
        byte_live_model(route,_mm_srli_si128(group,8),numerical_hi(recent))
    };
}

static QLI_NUMERIC_TARGET inline FusedPair chroma_entries(const ChromaBand *b,FusedPair model)
{
    const uint32_t *table=b->plane->d.work->tables;
    FusedPair s=b->history.ans;
    const NumericalLanes live=fused_all_lanes();
    return (FusedPair){
        fused_entries(table,s.lo,model.lo,live),
        fused_entries(table,s.hi,model.hi,live)
    };
}

static QLI_NUMERIC_TARGET inline FusedPair chroma_update(FusedPair state,FusedPair entry)
{
    return (FusedPair){full_word_next(state.lo,entry.lo),full_word_next(state.hi,entry.hi)};
}

static QLI_NUMERIC_TARGET inline __m128i chroma_prediction(ChromaBand *b,ChromaBlock *block,unsigned j,__m128i *shift)
{
    const FusedHistory *s=&b->history;
    const unsigned step=b->t+j;
    const __m128i west=s->pix[0],n=_mm_insert_epi8(_mm_slli_si128(s->pix[3],1),b->above[step],0);
    const __m128i nw=_mm_insert_epi8(_mm_slli_si128(s->pix[4],1),b->above[step-1],0);
    const __m128i ne=_mm_insert_epi8(_mm_slli_si128(s->pix[2],1),b->above[step+1],0);
    const unsigned group=j/4;
    __m128i pred;
    if (block->fitted & (1u<<group)) {
        const FusedPredict *fit=&block->predict[group].fit;
        const __m128i wide=_mm_cmpgt_epi8(fit->mode,_mm_set1_epi8(7));
        pred=fused_prediction(fit,s->pix,west,n,nw,ne,b->above,b->plane->d.w,step,wide,1);
    } else {
        pred=word_prediction(&block->predict[group].word,west,n,nw,ne);
    }
    if (shift) {
        const __m128i all=_mm_set1_epi8(-1);
        *shift=direction_indices(west,n,nw,pred,all,all);
    }
    return pred;
}

static QLI_NUMERIC_TARGET inline FusedPair chroma_direction(ChromaBand *b,ChromaBlock *block,unsigned j,
                                        FusedPair model,__m128i *pred)
{
    if (b->plane->d.work->directional) {
        __m128i shift;
        *pred=chroma_prediction(b,block,j,&shift);
        model.lo=direction_model(model.lo,shift);
        model.hi=direction_model(model.hi,_mm_srli_si128(shift,8));
    }
    return model;
}

static QLI_NUMERIC_TARGET inline void chroma_reconstruct(ChromaBand *b,ChromaBlock *block,unsigned j,FusedPair entry,__m128i *codes,__m128i pred)
{
    Work *work=b->plane->d.work;
    FusedHistory *s=&b->history;
    uint16_t *north=(uint16_t *)work->causal_aux;
    const unsigned step=b->t+j;
    const __m128i error=fused_symbols(entry.lo,entry.hi);
    unsigned top=north[step+2]>north[step+3] ? north[step+2] : north[step+3];
    const NumericalLanes recent=fused_update_activity(s->recent,s->older,error,top);
    s->older=s->recent;
    s->recent=recent;
    north[step-60]=(uint16_t)fused_last_activity(recent);
    if (work->stencil) retain_fitted_lags(s);
    for (unsigned k=4;k;k--) s->pix[k]=s->pix[k-1];
    s->pix[0]=block->sample[j]=_mm_add_epi8(pred,error);
    if (work->error) codes[j]=diagonal_codes(error);
}

static QLI_NUMERIC_TARGET __attribute__((noinline)) int chroma_pair(ChromaBand *a,ChromaBand *b,const __m128i coefficient[11])
{
    Diagonal *d=&a->plane->d,*e=&b->plane->d;
    ChromaBlock x,y;
    chroma_prepare(a,coefficient,&x);
    chroma_prepare(b,coefficient,&y);
    byte_live_stage(d,a->base,a->t,x.group,1);
    byte_live_stage(e,b->base,b->t,y.group,1);
    for (unsigned j=0;j<16;++j) {
        FusedPair ma=chroma_model(a,x.group[j]),mb=chroma_model(b,y.group[j]);
        __m128i pa,pb;
        ma=chroma_direction(a,&x,j,ma,&pa);
        mb=chroma_direction(b,&y,j,mb,&pb);
        if (fused_invalid_routes(_mm256_or_si256(ma.lo,mb.lo),
                                 _mm256_or_si256(ma.hi,mb.hi))) return 0;
        FusedPair ea=chroma_entries(a,ma),eb=chroma_entries(b,mb);
        FusedPair sa=chroma_update(a->history.ans,ea),sb=chroma_update(b->history.ans,eb);
        if (!paired_refill(&a->plane->input,&sa) || !paired_refill(&b->plane->input,&sb)) return 0;
        a->history.ans=sa;
        b->history.ans=sb;
        chroma_reconstruct(a,&x,j,ea,x.code,
            d->work->directional ? pa : chroma_prediction(a,&x,j,NULL));
        chroma_reconstruct(b,&y,j,eb,y.code,
            e->work->directional ? pb : chroma_prediction(b,&y,j,NULL));
    }
    byte_live_store(a->pixels,x.sample,d->w,a->t);
    byte_live_store(b->pixels,y.sample,e->w,b->t);
    if (d->work->error) byte_live_store(d->work->error,x.code,d->w,a->t);
    if (e->work->error) byte_live_store(e->work->error,y.code,e->w,b->t);
    a->t+=16;
    b->t+=16;
    return 1;
}

static QLI_NUMERIC_TARGET int chroma_whole(ChromaBand *b,const __m128i coefficient[11])
{
    while (b->t<b->end) if (!chroma_single(b,coefficient)) return 0;
    b->plane->ans=b->history.ans;
    return 1;
}


typedef struct { NumericalLanes low, high; } CarrierRoute;

static QLI_NUMERIC_TARGET inline CarrierRoute carrier_route(const uint32_t *route)
{
    const __m128i a=_mm_packus_epi32(_mm_srli_epi32(numerical_load16(route),12),
                                     _mm_srli_epi32(numerical_load16((route+4)),12));
    const __m128i b=_mm_packus_epi32(_mm_srli_epi32(numerical_load16((route+8)),12),
                                     _mm_srli_epi32(numerical_load16((route+12)),12));
    const __m128i low=_mm_packus_epi16(a,b);
    const __m128i high=_mm_setr_epi8((char)(route[16]>>12),(char)(route[17]>>12),0,0,0,0,0,0,0,0,0,0,0,0,0,0);
    return (CarrierRoute){numerical_pair(low,low),numerical_pair(high,high)};
}

static QLI_NUMERIC_TARGET inline NumericalLanes carrier_model(CarrierRoute route,__m128i activity)
{
    NumericalLanes bin=activity_bins(activity);
    const NumericalLanes models=_mm256_blendv_epi8(_mm256_shuffle_epi8(route.low,bin),
                                           _mm256_shuffle_epi8(route.high,bin),
                                           _mm256_cmpgt_epi32(bin,_mm256_set1_epi32(15)));
    return _mm256_slli_epi32(_mm256_and_si256(models,_mm256_set1_epi32(63)),12);
}

static QLI_NUMERIC_TARGET inline FusedPair carrier_block_model(
    const ChromaBand *a,CarrierRoute carrier)
{
    if (a->plane->d.work->directional) return chroma_model(a,_mm_setzero_si128());
    NumericalLanes recent=a->history.recent;
    return (FusedPair){carrier_model(carrier,numerical_lo(recent)),
                       carrier_model(carrier,numerical_hi(recent))};
}

static QLI_NUMERIC_TARGET __attribute__((noinline)) int chroma_triple(ChromaBand *a,ChromaBand *b,ChromaBand *c,
                                                  const __m128i coefficient[11],
                                                  __m128i *oldest,const __m128i *newer,__m128i *chroma)
{
    Diagonal *d=&a->plane->d,*e=&b->plane->d,*f=&c->plane->d;
    ChromaBlock x,y,z;
    chroma_prepare(a,coefficient,&x);
    chroma_prepare(b,coefficient,&y);
    chroma_prepare(c,coefficient,&z);
    CarrierRoute carrier={0};
    if (!d->work->directional) carrier=carrier_route(a->plane->route);
    if (oldest) {
        for (unsigned j=0;j<16;++j) {
            const __m128i key=_mm_add_epi8(_mm_sub_epi8(_mm_slli_epi16(oldest[j],3),oldest[j]),chroma[j]);
            y.group[j]=fused_map(e->mapping,newer[j],1);
            z.group[j]=fused_map(f->mapping,key,2);
        }
    } else {
        byte_live_stage(e,b->base,b->t,y.group,1);
        byte_live_stage(f,c->base,c->t,z.group,1);
    }
    __m128i *next_carrier=oldest ? oldest : x.code;
    __m128i *next_chroma=oldest ? chroma : y.code;
    for (unsigned j=0;j<16;++j) {
        FusedPair ma=carrier_block_model(a,carrier);
        FusedPair mb=chroma_model(b,y.group[j]),mc=chroma_model(c,z.group[j]);
        __m128i pa,pb,pc;
        ma=chroma_direction(a,&x,j,ma,&pa);
        mb=chroma_direction(b,&y,j,mb,&pb);
        mc=chroma_direction(c,&z,j,mc,&pc);
        if (fused_invalid_routes(_mm256_or_si256(_mm256_or_si256(ma.lo,mb.lo),mc.lo),
                                 _mm256_or_si256(_mm256_or_si256(ma.hi,mb.hi),mc.hi))) return 0;
        FusedPair ea=chroma_entries(a,ma),eb=chroma_entries(b,mb),ec=chroma_entries(c,mc);
        FusedPair sa=chroma_update(a->history.ans,ea),sb=chroma_update(b->history.ans,eb);
        FusedPair sc=chroma_update(c->history.ans,ec);
        chroma_reconstruct(a,&x,j,ea,next_carrier,
            d->work->directional ? pa : chroma_prediction(a,&x,j,NULL));
        chroma_reconstruct(b,&y,j,eb,next_chroma,
            e->work->directional ? pb : chroma_prediction(b,&y,j,NULL));
        chroma_reconstruct(c,&z,j,ec,z.code,
            f->work->directional ? pc : chroma_prediction(c,&z,j,NULL));
        if (!paired_refill(&a->plane->input,&sa) || !paired_refill(&b->plane->input,&sb) ||
            !paired_refill(&c->plane->input,&sc)) return 0;
        a->history.ans=sa;
        b->history.ans=sb;
        c->history.ans=sc;
    }
    byte_live_store(a->pixels,x.sample,d->w,a->t);
    byte_live_store(b->pixels,y.sample,e->w,b->t);
    byte_live_store(c->pixels,z.sample,f->w,c->t);
    if (!oldest) {
        byte_live_store(d->work->error,x.code,d->w,a->t);
        byte_live_store(e->work->error,y.code,e->w,b->t);
    }
    if (f->work->error) byte_live_store(f->work->error,z.code,f->w,c->t);
    a->t+=16;
    b->t+=16;
    c->t+=16;
    return 1;
}

static QLI_NUMERIC_TARGET void guide_load(__m128i values[16],const uint8_t *raster,unsigned w,unsigned t)
{
    for (unsigned lane=0;lane<16;++lane)
        values[lane]=numerical_load16((raster+(size_t)lane*w+t-4*lane));
    diagonal_transpose(values);
}

static QLI_NUMERIC_TARGET unsigned triple_run_steps(const ChromaBand *a,const ChromaBand *b,const ChromaBand *c)
{
    unsigned offset=16;
    while (chroma_kind_at(a,a->t+offset)==BLOCK_CODED &&
           chroma_kind_at(b,b->t+offset)==BLOCK_CODED &&
           chroma_kind_at(c,c->t+offset)==BLOCK_CODED)
        offset+=16;
    return offset;
}

static QLI_NUMERIC_TARGET __attribute__((noinline)) int chroma_triple_run(
    ChromaBand *a,ChromaBand *b,ChromaBand *c,const __m128i coefficient[11],unsigned steps)
{
    __m128i carrier[2][16],first_chroma[16];
    __m128i *oldest=carrier[0],*newer=carrier[1];
    uint8_t *carrier_raster=a->plane->d.work->error;
    uint8_t *chroma_raster=b->plane->d.work->error;
    unsigned w=a->plane->d.w, stop=a->t+steps;
    guide_load(oldest,carrier_raster,w,c->t);
    guide_load(newer,carrier_raster,w,b->t);
    guide_load(first_chroma,chroma_raster,w,c->t);
    do {
        if (!chroma_triple(a,b,c,coefficient,oldest,newer,first_chroma)) return 0;
        __m128i *next=oldest;
        oldest=newer;
        newer=next;
    } while (a->t<stop);
    byte_live_store(carrier_raster,oldest,w,c->t);
    byte_live_store(carrier_raster,newer,w,b->t);
    byte_live_store(chroma_raster,first_chroma,w,c->t);
    return 1;
}

static QLI_NUMERIC_TARGET int chroma_pipeline(ChromaBand *a,ChromaBand *b,ChromaBand *c,const __m128i coefficient[11])
{
    if (a->end<=32)
        return chroma_whole(a,coefficient) && chroma_whole(b,coefficient) && chroma_whole(c,coefficient);
    if (!chroma_single(a,coefficient) || !chroma_single(a,coefficient) ||
        !chroma_single(b,coefficient)) return 0;
    while (a->t<a->end) {
        int ready_a=chroma_kind(a)==BLOCK_CODED,ready_b=chroma_kind(b)==BLOCK_CODED;
        int ready_c=chroma_kind(c)==BLOCK_CODED;
        if (ready_a && ready_b && ready_c) {
            unsigned steps=triple_run_steps(a,b,c);
            if (steps>=48) {
                if (!chroma_triple_run(a,b,c,coefficient,steps)) return 0;
            } else {
                do {
                    if (!chroma_triple(a,b,c,coefficient,NULL,NULL,NULL)) return 0;
                    steps-=16;
                } while (steps);
            }
        } else if (ready_b && ready_c) {
            if (!chroma_single(a,coefficient) || !chroma_pair(b,c,coefficient)) return 0;
        } else if (ready_a && ready_b) {
            if (!chroma_pair(a,b,coefficient) || !chroma_single(c,coefficient)) return 0;
        } else if (!chroma_single(a,coefficient) || !chroma_single(b,coefficient) ||
                   !chroma_single(c,coefficient)) return 0;
    }
    return chroma_whole(a,coefficient) && chroma_whole(b,coefficient) && chroma_whole(c,coefficient);
}
#endif
#ifdef QLI_NUMERIC_SSE42

static QLI_SSE42_TARGET inline int sse42_qxj_refill(Input *in,sse42_NumericalLanes *state,unsigned refill)
{
    if (!refill) return 1;
    const unsigned bytes=2u*(unsigned)qlic_count_bits(refill);
    const size_t remaining=(size_t)(in->end-in->at);
    if (remaining<bytes) return 0;
    __m128i words;
    if (remaining>=16) {
        words=sse42_numerical_load16(in->at);
    } else {
        uint16_t tail[8]={0};
        memcpy(tail,in->at,bytes);
        words=sse42_numerical_load16(tail);
    }
    in->at+=bytes;
    const __m128i shuffle=sse42_numerical_load16(qxj_refill_masks[refill]);
    const sse42_NumericalLanes added=sse_cvtepu16_epi32(_mm_shuffle_epi8(words,shuffle));
    const sse42_NumericalLanes normal=sse_cmpeq_epi32(sse_srli_epi32(*state,16),sse_setzero_si256());
    *state=sse_or_si128(sse_blendv_epi8(*state,sse_slli_epi32(*state,16),normal),added);
    return 1;
}

static QLI_SSE42_TARGET inline void sse42_diagonal_transpose(__m128i row[16])
{
    __m128i pair[16],four[16];
    for (unsigned i=0;i<16;i+=2) {
        pair[i]=_mm_unpacklo_epi8(row[i],row[i+1]);
        pair[i+1]=_mm_unpackhi_epi8(row[i],row[i+1]);
    }
    for (unsigned i=0;i<16;i+=4)
        for (unsigned k=0;k<2;++k) {
            four[i+2*k]=_mm_unpacklo_epi16(pair[i+k],pair[i+k+2]);
            four[i+2*k+1]=_mm_unpackhi_epi16(pair[i+k],pair[i+k+2]);
        }
    for (unsigned i=0;i<16;i+=8)
        for (unsigned k=0;k<4;++k) {
            pair[i+2*k]=_mm_unpacklo_epi32(four[i+k],four[i+k+4]);
            pair[i+2*k+1]=_mm_unpackhi_epi32(four[i+k],four[i+k+4]);
        }
    for (unsigned k=0;k<8;++k) {
        row[2*k]=_mm_unpacklo_epi64(pair[k],pair[k+8]);
        row[2*k+1]=_mm_unpackhi_epi64(pair[k],pair[k+8]);
    }
}

static QLI_SSE42_TARGET inline __m128i sse42_diagonal_codes(__m128i error)
{
    const __m128i codes=_mm_setr_epi8(6,4,4,4,2,0,1,3,3,3,5,0,0,0,0,0);
    const __m128i clamped=_mm_min_epi8(_mm_max_epi8(error,_mm_set1_epi8(-5)),_mm_set1_epi8(5));
    return _mm_shuffle_epi8(codes,_mm_add_epi8(clamped,_mm_set1_epi8(5)));
}

static QLI_SSE42_TARGET void sse42_diagonal_store(uint8_t *out,__m128i values[16],unsigned w,unsigned rows,int t)
{
    sse42_diagonal_transpose(values);
    for (unsigned k=0;k<rows;++k) {
        int x=t-4*(int)k;
        int lo=x<0 ? -x : 0, hi=x+16>(int)w ? (int)w-x : 16;
        if (lo>=hi) continue;
        uint8_t *row=out+(size_t)k*w;
        if (hi-lo==16) sse42_numerical_store16((row+x),values[k]);
        else {
            uint8_t part[16];
            sse42_numerical_store16(part,values[k]);
            memcpy(row+(x+lo),part+lo,(size_t)(hi-lo));
        }
    }
}

typedef struct { sse42_NumericalLanes lo,hi; } sse42_FusedPair;
static QLI_SSE42_TARGET inline __m128i sse42_fused_pack_u8(sse42_NumericalLanes lo,sse42_NumericalLanes hi)
{
    const __m128i a=_mm_packus_epi32(sse42_numerical_lo(lo),sse42_numerical_hi(lo));
    const __m128i b=_mm_packus_epi32(sse42_numerical_lo(hi),sse42_numerical_hi(hi));
    return _mm_packus_epi16(a,b);
}

static QLI_SSE42_TARGET inline __m128i sse42_fused_symbols(sse42_NumericalLanes lo,sse42_NumericalLanes hi)
{
    const sse42_NumericalLanes byte=sse_set1_epi32(255);
    return sse42_fused_pack_u8(sse_and_si128(lo,byte),sse_and_si128(hi,byte));
}

static QLI_SSE42_TARGET inline int sse42_fused_invalid_routes(sse42_NumericalLanes a,sse42_NumericalLanes b)
{
    return !sse_testz_si256(sse_or_si128(a,b),sse_set1_epi32(ROUTE_SKIP));
}

static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_fused_all_lanes(void)
{
    return sse_set1_epi32(-1);
}

static QLI_SSE42_TARGET inline int sse42_fused_all_states(sse42_NumericalLanes state,uint32_t value)
{
    return sse_movemask_epi8(sse_cmpeq_epi32(state,sse_set1_epi32((int)value))) == -1;
}

static QLI_SSE42_TARGET void sse42_fused_coefficients(const int8_t *fitted,__m128i lut[11])
{
    static const int8_t ordinary[8][4]={{12,12,-12,0},{12,0,0,0},{0,12,0,0},
        {3,3,3,3},{9,3,0,0},{4,4,0,4},{6,3,0,3},{9,9,-6,0}};
    for (unsigned k=0;k<10;++k) {
        int8_t column[16]={0};
        if (k<4) for (unsigned m=0;m<8;++m) column[m]=ordinary[m][k];
        if (fitted) { column[8]=fitted[k]; column[9]=fitted[10+k]; }
        lut[k]=sse42_numerical_load16(column);
    }
    uint8_t wide[16]={0};
    if (fitted) {
        for (unsigned m=0;m<2;++m) {
            unsigned positive=0;
            for (unsigned k=0;k<10;++k) {
                int weight=fitted[10*m+k];
                if (weight>0) positive+=(unsigned)weight;
            }
            wide[8+m]=positive>128 ? 255 : 0;
        }
    }
    lut[10]=sse42_numerical_load16(wide);
}

static QLI_SSE42_TARGET inline __m128i sse42_fused_predict_bias(__m128i mode)
{
    return _mm_shuffle_epi8(_mm_setr_epi8(0,0,0,6,6,4,6,6,16,16,0,0,0,0,0,0),mode);
}

typedef struct {
    sse42_NumericalLanes wn, diagonals, bias;
    __m128i gradient;
} sse42_WordPredict;

static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_word_pair(__m128i a,__m128i b)
{
    return sse42_numerical_pair(_mm_unpackhi_epi8(a,b),_mm_unpacklo_epi8(a,b));
}

static QLI_SSE42_TARGET inline sse42_WordPredict sse42_word_predict_setup(const __m128i coefficient[11],__m128i mode)
{
    const __m128i w=_mm_shuffle_epi8(coefficient[0],mode);
    const __m128i n=_mm_shuffle_epi8(coefficient[1],mode);
    const __m128i nw=_mm_shuffle_epi8(coefficient[2],mode);
    const __m128i ne=_mm_shuffle_epi8(coefficient[3],mode);
    const __m128i bias=sse42_fused_predict_bias(mode), zero=_mm_setzero_si128();
    const sse42_NumericalLanes words=sse_cvtepu8_epi16(bias);
    return (sse42_WordPredict){sse42_word_pair(w,n),sse42_word_pair(nw,ne),words,_mm_cmpeq_epi8(mode,zero)};
}

static QLI_SSE42_TARGET inline __m128i sse42_finish_prediction(
    sse42_NumericalLanes sum,sse42_NumericalLanes scale,__m128i w,__m128i n,__m128i gradient)
{
    const sse42_NumericalLanes q=sse_mulhi_epu16(sse_max_epi16(sum,sse_setzero_si256()),scale);
    const __m128i raw=_mm_packus_epi16(sse42_numerical_lo(q),sse42_numerical_hi(q));
    const __m128i clamped=_mm_min_epu8(_mm_max_epu8(raw,_mm_min_epu8(w,n)),_mm_max_epu8(w,n));
    return _mm_blendv_epi8(raw,clamped,gradient);
}

static QLI_SSE42_TARGET inline __m128i sse42_word_prediction(const sse42_WordPredict *p,__m128i w,__m128i n,__m128i nw,__m128i ne)
{
    const sse42_NumericalLanes near_terms=sse_maddubs_epi16(sse42_word_pair(w,n),p->wn);
    const sse42_NumericalLanes far_terms=sse_maddubs_epi16(sse42_word_pair(nw,ne),p->diagonals);
    return sse42_finish_prediction(sse_add_epi16(sse_add_epi16(near_terms,far_terms),p->bias),
                             sse_set1_epi16(5462),w,n,p->gradient);
}

static QLI_SSE42_TARGET inline __m128i sse42_fused_above2(__m128i lag,const uint8_t *above,
                                 ptrdiff_t w,ptrdiff_t step,int dx,unsigned wide)
{
    __m128i tap=_mm_slli_si128(lag,2);
    if (wide&1) tap=_mm_insert_epi8(tap,above[step-w+dx],0);
    if (wide&2) tap=_mm_insert_epi8(tap,above[step-4+dx],1);
    return tap;
}

typedef struct {
    sse42_WordPredict first;
    sse42_NumericalLanes extra[3];
    __m128i mode;
    int wide_sum;
} sse42_FusedPredict;

static QLI_SSE42_TARGET sse42_FusedPredict sse42_fused_predict_setup(const __m128i lut[11],__m128i mode)
{
    sse42_FusedPredict p;
    p.first=sse42_word_predict_setup(lut,mode);
    for (unsigned k=0;k<3;++k)
        p.extra[k]=sse42_word_pair(_mm_shuffle_epi8(lut[4+2*k],mode),_mm_shuffle_epi8(lut[5+2*k],mode));
    p.mode=mode;
    p.wide_sum=_mm_movemask_epi8(_mm_shuffle_epi8(lut[10],mode));
    return p;
}

static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_wide_prediction_sum(const sse42_NumericalLanes terms[6])
{
    sse42_NumericalLanes low=sse_setzero_si256(),high=sse_setzero_si256();
    for (unsigned k=0;k<6;++k) {
        low=sse_add_epi32(low,sse_cvtepi16_epi32(sse42_numerical_lo(terms[k])));
        high=sse_add_epi32(high,sse_cvtepi16_epi32(sse42_numerical_hi(terms[k])));
    }
    return sse_permute4x64_epi64(sse_packs_epi32(low,high),0xd8);
}

static QLI_SSE42_TARGET inline __m128i sse42_fused_prediction(const sse42_FusedPredict *p,const __m128i hist[9],
                    __m128i west,__m128i north,__m128i nw,__m128i ne,
                    const uint8_t *above,unsigned w,unsigned step,__m128i wide,int interior)
{
    const unsigned bits=interior ? 3u : (unsigned)_mm_movemask_epi8(wide);
    if (!bits && !p->wide_sum) return sse42_word_prediction(&p->first,west,north,nw,ne);

    const sse42_NumericalLanes zero=sse_setzero_si256();
    sse42_NumericalLanes terms[6]={
        sse_maddubs_epi16(sse42_word_pair(west,north),p->first.wn),
        sse_maddubs_epi16(sse42_word_pair(nw,ne),p->first.diagonals),
        zero,zero,zero,p->first.bias
    };
    if (bits) {
        __m128i nww=_mm_slli_si128(hist[5],1);
        if (bits&1) nww=_mm_insert_epi8(nww,above[step-2],0);
        const __m128i nnw=sse42_fused_above2(hist[8],above,w,step,-1,bits);
        const __m128i nn=sse42_fused_above2(hist[7],above,w,step,0,bits);
        const __m128i nne=sse42_fused_above2(hist[6],above,w,step,1,bits);
        const __m128i nnee=sse42_fused_above2(hist[5],above,w,step,2,bits);
        terms[2]=sse_maddubs_epi16(sse42_word_pair(hist[1],nww),p->extra[0]);
        terms[3]=sse_maddubs_epi16(sse42_word_pair(nnw,nn),p->extra[1]);
        terms[4]=sse_maddubs_epi16(sse42_word_pair(nne,nnee),p->extra[2]);
    }
    sse42_NumericalLanes sum;
    if (p->wide_sum) sum=sse42_wide_prediction_sum(terms);
    else {
        sum=sse_add_epi16(terms[0],terms[1]);
        for (unsigned k=2;k<6;++k) sum=sse_add_epi16(sum,terms[k]);
    }
    const sse42_NumericalLanes scale=sse_blendv_epi8(sse_set1_epi16(5462),sse_set1_epi16(2048),
                                                 sse_cvtepi8_epi16(wide));
    return sse42_finish_prediction(sum,scale,west,north,p->first.gradient);
}

static QLI_SSE42_TARGET __m128i sse42_fused_map(const uint8_t map[64],__m128i key,unsigned guides)
{
    if (guides==1) return _mm_shuffle_epi8(sse42_numerical_load16(map),key);
    const __m128i lo=_mm_and_si128(key,_mm_set1_epi8(15));
    const __m128i hi=_mm_and_si128(key,_mm_set1_epi8(48));
    __m128i value=_mm_shuffle_epi8(sse42_numerical_load16(map),lo);
    for (unsigned k=1;k<4;++k) {
        __m128i v=_mm_shuffle_epi8(sse42_numerical_load16((map+16*k)),lo);
        value=_mm_blendv_epi8(value,v,_mm_cmpeq_epi8(hi,_mm_set1_epi8((char)(16*k))));
    }
    return value;
}

static QLI_SSE42_TARGET void sse42_fused_stage(const Diagonal *d,unsigned base,unsigned rows,int t,
                        __m128i groups[16],__m128i known[16],int banded)
{
    const Work *work=d->work;
    const __m128i skip=_mm_set1_epi8((char)CAUSAL_SKIP),zero=_mm_setzero_si128();
    const __m128i index=_mm_setr_epi8(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
    for (unsigned k=0;k<16;++k) {
        groups[k]=skip;
        known[k]=zero;
        int x=t-4*(int)k;
        unsigned begin=x<0 ? 0 : (unsigned)x;
        int limit=x+16<(int)d->w ? x+16 : (int)d->w;
        if (k>=rows || limit<=(int)begin) continue;
        unsigned end=(unsigned)limit,y=base+k;
        size_t row=(size_t)y*d->w;
        __m128i key=zero;
        if (work->guides) {
            unsigned off=(unsigned)((int)begin-x),n=end-begin;
            uint8_t first[16]={0},second[16]={0};
            size_t guide_row=banded ? (size_t)k*d->w : row;
            const uint8_t *a=work->earlier[0]+guide_row+begin;
            if (n==16) key=sse42_numerical_load16(a);
            else {
                memcpy(first+off,a,n);
                key=sse42_numerical_load16(first);
            }
            if (work->guides==2) {
                const uint8_t *b=work->earlier[1]+guide_row+begin;
                __m128i other;
                if (n==16) other=sse42_numerical_load16(b);
                else {
                    memcpy(second+off,b,n);
                    other=sse42_numerical_load16(second);
                }
                key=_mm_add_epi8(_mm_sub_epi8(_mm_slli_epi16(key,3),key),other);
            }
        }
        for (unsigned at=begin;at<end;) {
            size_t p=row+at;
            unsigned len=(unsigned)min_size(end-at,256-p%256);
            int flat=flag(d->flat,p/256);
            int start=(int)at-x,stop=start+(int)len;
            __m128i mask=_mm_and_si128(_mm_cmpgt_epi8(index,_mm_set1_epi8((char)(start-1))),
                                      _mm_cmpgt_epi8(_mm_set1_epi8((char)stop),index));
            if (flat) known[k]=_mm_blendv_epi8(known[k],_mm_set1_epi8((char)work->raster[p/256]),mask);
            else {
                __m128i id=work->guides ? sse42_fused_map(d->mapping,key,work->guides) : zero;
                groups[k]=_mm_blendv_epi8(groups[k],id,mask);
            }
            at+=len;
        }
    }
    sse42_diagonal_transpose(groups);
    sse42_diagonal_transpose(known);
}

static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_fused_activity_lanes(sse42_NumericalLanes v,unsigned above)
{
    __m128i a=sse42_numerical_lo(v),b=sse42_numerical_hi(v);
    return sse42_numerical_pair(_mm_alignr_epi8(b,a,14),
                           _mm_insert_epi16(_mm_slli_si128(a,2),(int)above,0));
}

static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_fused_update_activity(sse42_NumericalLanes recent,sse42_NumericalLanes older,
                                                  __m128i error,unsigned top)
{
    const sse42_NumericalLanes diagonal=sse42_fused_activity_lanes(sse_max_epu16(recent,older),top);
    const sse42_NumericalLanes e=sse_cvtepi8_epi16(error);
    const sse42_NumericalLanes zig=sse_xor_si128(sse_slli_epi16(e,1),sse_srai_epi16(e,15));
    return sse_avg_epu16(recent,sse_avg_epu16(sse_slli_epi16(zig,3),diagonal));
}


static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_activity_bins(__m128i activity)
{
    sse42_NumericalLanes a=sse_cvtepu16_epi32(activity);
    const sse42_NumericalLanes score=sse_add_epi32(sse_mullo_epi32(a,a),sse_set1_epi32(1));
    const sse42_NumericalLanes exp=sse_srli_epi32(sse_castps_si256(sse_cvtepi32_ps(score)),23);
    return sse_min_epi32(sse_sub_epi32(exp,sse_set1_epi32(127)),sse_set1_epi32(17));
}

static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_fused_model(const uint32_t *route,__m128i group,
                       __m128i activity,sse42_NumericalLanes *live)
{
    sse42_NumericalLanes bin=sse42_activity_bins(activity);
    sse42_NumericalLanes id=sse_cvtepu8_epi32(group);
    *live=sse_cmpgt_epi32(sse_set1_epi32(32),id);
    sse42_NumericalLanes addr=sse_or_si128(sse_slli_epi32(id,5),bin);
    return sse42_numerical_gather8(route,addr,*live);
}

static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_fused_entries(const uint32_t *table,sse42_NumericalLanes state,sse42_NumericalLanes model,
                                           sse42_NumericalLanes live)
{
    const sse42_NumericalLanes mask=sse_set1_epi32(CAUSAL_SCALE-1);
    sse42_NumericalLanes off=sse_and_si128(model,sse_set1_epi32(ROUTE_OFFSET_MASK));
    sse42_NumericalLanes addr=sse_or_si128(off,sse_and_si128(state,mask));
    return sse42_numerical_gather8(table,addr,live);
}

static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_full_word_next(sse42_NumericalLanes state,sse42_NumericalLanes entry)
{
    const sse42_NumericalLanes freq=sse_add_epi32(sse_srli_epi32(entry,20),sse_set1_epi32(1));
    const sse42_NumericalLanes rel=sse_and_si128(sse_srli_epi32(entry,8),sse_set1_epi32(4095));
    return sse_add_epi32(sse_mullo_epi32(sse_srli_epi32(state,12),freq),rel);
}

static QLI_SSE42_TARGET inline int sse42_fused_advance(Input *in,sse42_NumericalLanes *state,sse42_NumericalLanes entry,sse42_NumericalLanes live)
{
    const sse42_NumericalLanes zero=sse_setzero_si256();
    sse42_NumericalLanes next=sse_blendv_epi8(*state,sse42_full_word_next(*state,entry),live);
    unsigned refill=(unsigned)sse_movemask_ps(sse_castsi256_ps(
        sse_cmpeq_epi32(sse_srli_epi32(next,16),zero)));
    if (refill) {
        if (sse_movemask_ps(sse_castsi256_ps(sse_cmpeq_epi32(next,zero)))) return 0;
        if (!sse42_qxj_refill(in,&next,refill)) return 0;
    }
    *state=next;
    return 1;
}

static QLI_SSE42_TARGET inline unsigned sse42_fused_last_activity(sse42_NumericalLanes activity)
{
    return (unsigned)_mm_extract_epi16(sse42_numerical_hi(activity),7);
}

static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_fused_keep_activity(sse42_NumericalLanes activity,sse42_NumericalLanes valid)
{
    return sse_and_si128(activity,valid);
}

static QLI_SSE42_TARGET inline void sse42_fused_positions(unsigned step,unsigned w,unsigned rows,sse42_NumericalLanes *active,
                                   __m128i *first,__m128i *last,__m128i *gradient)
{
    const sse42_NumericalLanes offsets=sse_setr_epi16(0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60);
    const sse42_NumericalLanes right=sse_set1_epi16((short)(w-1));
    const sse42_NumericalLanes x=sse_sub_epi16(sse_set1_epi16((short)step),offsets);
    const sse42_NumericalLanes inside=sse_cmpgt_epi16(sse_set1_epi16((short)rows),sse_srli_epi16(offsets,2));
    *active=sse_and_si128(inside,sse_andnot_si128(sse_cmpgt_epi16(x,right),
                                                       sse_cmpgt_epi16(x,sse_set1_epi16(-1))));
    const sse42_NumericalLanes a=sse_cmpeq_epi16(x,sse_setzero_si256()), b=sse_cmpeq_epi16(x,right);
    const sse42_NumericalLanes g=sse_cmpgt_epi16(x,sse_set1_epi16(1));
    *first=_mm_packs_epi16(sse42_numerical_lo(a),sse42_numerical_hi(a));
    *last=_mm_packs_epi16(sse42_numerical_lo(b),sse42_numerical_hi(b));
    *gradient=_mm_packs_epi16(sse42_numerical_lo(g),sse42_numerical_hi(g));
}

typedef struct { sse42_FusedPair ans; sse42_NumericalLanes recent,older; __m128i pix[9]; } sse42_FusedHistory;

static QLI_SSE42_TARGET inline void sse42_retain_fitted_lags(sse42_FusedHistory *s)
{
    for (unsigned k=8;k>4;--k) s->pix[k]=s->pix[k-1];
}

static QLI_SSE42_TARGET inline __attribute__((always_inline)) int sse42_fused_block(Input *in,Diagonal *d,const uint32_t *route,sse42_FusedHistory *s,uint16_t *north,
                       uint8_t *out,const uint8_t *above,unsigned base,unsigned rows,
                       unsigned t,unsigned steps,const sse42_FusedPredict p[4],int banded,const sse42_WordPredict word[4])
{
    Work *work=d->work;
    const unsigned w=d->w;
    const __m128i six=_mm_set1_epi8(7);
    __m128i group[16],known[16],samples[16]={0},codes[16]={0};
    sse42_fused_stage(d,base,rows,(int)t,group,known,banded);
    for (unsigned j=0;j<steps;++j) {
        unsigned step=t+j;
        sse42_NumericalLanes valid16;
        __m128i first,last,gradient;
        sse42_fused_positions(step,w,rows,&valid16,&first,&last,&gradient);
        __m128i valid=_mm_packs_epi16(sse42_numerical_lo(valid16),sse42_numerical_hi(valid16));
        const int coded=!banded || _mm_movemask_epi8(group[j])!=65535;
        const sse42_NumericalLanes clear=sse_setzero_si256();
        sse42_NumericalLanes m_a,m_b,live_a=clear,live_b=clear,ea=clear,eb=clear;
        if (coded) {
            m_a=sse42_fused_model(route,group[j],sse42_numerical_lo(s->recent),&live_a);
            m_b=sse42_fused_model(route,_mm_srli_si128(group[j],8),sse42_numerical_hi(s->recent),&live_b);
            if (!work->directional) {
                if (sse42_fused_invalid_routes(m_a,m_b)) return 0;
                ea=sse42_fused_entries(work->tables,s->ans.lo,m_a,live_a);
                eb=sse42_fused_entries(work->tables,s->ans.hi,m_b,live_b);
            }
        }
        __m128i west=s->pix[0],n=_mm_slli_si128(s->pix[3],1),
                 nw=_mm_slli_si128(s->pix[4],1),ne=_mm_slli_si128(s->pix[2],1);
        if (above) {
            unsigned ax=(unsigned)min_size(step,w-1);
            n=_mm_insert_epi8(n,above[ax],0);
            nw=_mm_insert_epi8(nw,above[ax ? ax-1 : 0],0);
            ne=_mm_insert_epi8(ne,above[ax+1<w ? ax+1 : ax],0);
        } else {
            if (!step) west=_mm_insert_epi8(west,128,0);
            unsigned a=(unsigned)_mm_extract_epi8(west,0);
            n=_mm_insert_epi8(n,a,0);
            nw=_mm_insert_epi8(nw,a,0);
            ne=_mm_insert_epi8(ne,a,0);
        }
        west=_mm_blendv_epi8(west,n,first);
        nw=_mm_blendv_epi8(nw,n,first);
        ne=_mm_blendv_epi8(ne,n,last);
        unsigned top=0;
        if (step<w) top=north[step+2]>north[step+3] ? north[step+2] : north[step+3];
        __m128i prediction;
        if (word) prediction=sse42_word_prediction(word+j/4,west,n,nw,ne);
        else {
            const sse42_FusedPredict *pred=p+j/4;
            __m128i wide=_mm_and_si128(valid,_mm_cmpgt_epi8(pred->mode,six));
            prediction=sse42_fused_prediction(pred,s->pix,west,n,nw,ne,above,w,step,wide,0);
        }
        if (coded && work->directional) {
            const __m128i all=_mm_set1_epi8(-1);
            const __m128i mid_rows=base ? all : _mm_slli_si128(all,1);
            const __m128i gradient_rows=base ? all : _mm_slli_si128(all,2);
            const __m128i has_mid=_mm_andnot_si128(first,mid_rows);
            __m128i has_gradient=_mm_and_si128(gradient_rows,gradient);
            __m128i shift=sse42_direction_indices(west,n,nw,prediction,has_mid,has_gradient);
            m_a=sse42_direction_model(m_a,shift);
            m_b=sse42_direction_model(m_b,_mm_srli_si128(shift,8));
            if (sse42_fused_invalid_routes(m_a,m_b)) return 0;
            ea=sse42_fused_entries(work->tables,s->ans.lo,m_a,live_a);
            eb=sse42_fused_entries(work->tables,s->ans.hi,m_b,live_b);
        }
        __m128i live=sse42_fused_symbols(live_a,live_b);
        __m128i symbols=sse42_fused_symbols(ea,eb);
        __m128i error=_mm_and_si128(valid,_mm_blendv_epi8(known[j],symbols,live));
        __m128i sample=_mm_add_epi8(prediction,error);
        sse42_NumericalLanes recent=sse42_fused_update_activity(s->recent,s->older,error,top);
        s->older=s->recent;
        s->recent=sse42_fused_keep_activity(recent,valid16);
        if (rows==16 && step-60<w) {
            north[step-60]=(uint16_t)sse42_fused_last_activity(s->recent);
        }
        for (unsigned k=8;k;k--) s->pix[k]=s->pix[k-1];
        s->pix[0]=samples[j]=sample;
        if (work->error) codes[j]=sse42_diagonal_codes(error);
        if (coded && (!sse42_fused_advance(in,&s->ans.lo,ea,live_a) ||
                       !sse42_fused_advance(in,&s->ans.hi,eb,live_b))) return 0;
    }
    sse42_diagonal_store(out,samples,w,rows,(int)t);
    if (work->error) sse42_diagonal_store(work->error+(banded ? 0 : (size_t)base*w),codes,w,rows,(int)t);
    return 1;
}

static QLI_SSE42_TARGET void sse42_known_marks(const uint8_t *flat,unsigned w,unsigned base,uint8_t *kind)
{
    const unsigned blocks=(w+60+15)/16;
    int16_t delta[MAX_WAVE_BLOCKS+1]={0};
    uint16_t edge[MAX_WAVE_BLOCKS]={0};
    const size_t begin=(size_t)base*w,end=begin+16*(size_t)w;
    for (size_t k=begin/256;k<=(end-1)/256;++k) {
        if (!(flat[k/8]&(1u<<(k%8)))) continue;
        size_t lo=k*256,hi=lo+256;
        if (lo<begin) lo=begin;
        if (hi>end) hi=end;
        while (lo<hi) {
            unsigned lane=(unsigned)((lo-begin)/w),x=(unsigned)((lo-begin)%w);
            unsigned run=w-x;
            if (run>hi-lo) run=(unsigned)(hi-lo);
            unsigned a=x+4*lane,b=a+run-1;
            unsigned first=a/16,last=b/16;
            if (first==last) edge[first]+=(uint16_t)run;
            else {
                edge[first]+=(uint16_t)(16-a%16);
                edge[last]+=(uint16_t)(1+b%16);
                delta[first+1]+=16;
                delta[last]-=16;
            }
            lo+=run;
        }
    }
    int inner=0;
    for (unsigned k=0;k<blocks;++k) {
        inner+=delta[k];
        unsigned n=(unsigned)(inner+edge[k]);
        kind[k]=(uint8_t)(n==0 ? BLOCK_CODED : n==256 ? BLOCK_KNOWN : BLOCK_MIXED);
    }
}

static QLI_SSE42_TARGET void sse42_known_rows(const uint8_t *values,unsigned w,unsigned base,unsigned t,__m128i rows[16])
{
    const __m128i index=_mm_setr_epi8(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
    for (unsigned lane=0;lane<16;++lane) {
        size_t pos=(size_t)(base+lane)*w+t-4*lane;
        const size_t first=pos/256;
        __m128i a=_mm_set1_epi8((char)values[first]);
        unsigned n=256-(unsigned)(pos%256);
        if (n<16) {
            __m128i b=_mm_set1_epi8((char)values[first+1]);
            __m128i prefix=_mm_cmpgt_epi8(_mm_set1_epi8((char)n),index);
            a=_mm_blendv_epi8(b,a,prefix);
        }
        rows[lane]=a;
    }
}

static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_paired_insert(sse42_NumericalLanes state,sse42_NumericalLanes mask,
                                    __m128i words,unsigned index)
{
    __m128i order=sse42_numerical_load16(qxj_refill_masks[index]);
    const __m128i packed=_mm_shuffle_epi8(words,order);
    sse42_NumericalLanes added=sse_cvtepu16_epi32(packed);
    return sse_or_si128(sse_blendv_epi8(state,sse_slli_epi32(state,16),mask),added);
}

static QLI_SSE42_TARGET inline int sse42_paired_refill(Input *in,sse42_FusedPair *state)
{
    const sse42_NumericalLanes zero=sse_setzero_si256();
    sse42_NumericalLanes a=sse_cmpeq_epi32(sse_srli_epi32(state->lo,16),zero);
    sse42_NumericalLanes b=sse_cmpeq_epi32(sse_srli_epi32(state->hi,16),zero);
    unsigned lo=(unsigned)sse_movemask_ps(sse_castsi256_ps(a));
    unsigned hi=(unsigned)sse_movemask_ps(sse_castsi256_ps(b));
    if (!(lo|hi)) return 1;
    sse42_NumericalLanes invalid=sse_or_si128(sse_cmpeq_epi32(state->lo,zero),
                                  sse_cmpeq_epi32(state->hi,zero));
    if (sse_movemask_ps(sse_castsi256_ps(invalid))) return 0;
    unsigned first=2u*(unsigned)qlic_count_bits(lo),bytes=first+2u*(unsigned)qlic_count_bits(hi);
    size_t available=(size_t)(in->end-in->at);
    if (available<bytes) return 0;
    uint8_t tail[32]={0};
    const uint8_t *source=in->at;
    if (available<32) {
        memcpy(tail,source,bytes);
        source=tail;
    }
    state->lo=sse42_paired_insert(state->lo,a,sse42_numerical_load16(source),lo);
    state->hi=sse42_paired_insert(state->hi,b,sse42_numerical_load16((source+first)),hi);
    in->at+=bytes;
    return 1;
}

static QLI_SSE42_TARGET void sse42_byte_live_stage(const Diagonal *d,unsigned base,unsigned t,__m128i groups[16],int banded)
{
    const Work *work=d->work;
    unsigned guides=work->guides;
    if (!guides) {
        memset(groups,0,16*sizeof(*groups));
        return;
    }
    for (unsigned lane=0;lane<16;++lane) {
        unsigned x=t-4*lane,y=base+lane;
        size_t pos=(size_t)(banded ? lane : y)*d->w+x;
        __m128i key=sse42_numerical_load16(work->earlier[0]+pos);
        if (guides==2) {
            __m128i other=sse42_numerical_load16((work->earlier[1]+pos));
            key=_mm_add_epi8(_mm_sub_epi8(_mm_slli_epi16(key,3),key),other);
        }
        groups[lane]=sse42_fused_map(d->mapping,key,guides);
    }
    sse42_diagonal_transpose(groups);
}

static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_byte_live_model(const uint32_t *route,__m128i group,__m128i activity)
{
    sse42_NumericalLanes bin=sse42_activity_bins(activity);
    sse42_NumericalLanes id=sse_slli_epi32(sse_cvtepu8_epi32(group),5);
    return sse42_numerical_gather8(route,sse_or_si128(id,bin),sse_set1_epi32(-1));
}

static QLI_SSE42_TARGET void sse42_byte_live_store(uint8_t *out,__m128i values[16],unsigned w,unsigned t)
{
    sse42_diagonal_transpose(values);
    for (unsigned k=0;k<16;++k)
        sse42_numerical_store16((out+(size_t)k*w+t-4*k),values[k]);
}

static QLI_SSE42_TARGET __attribute__((noinline)) int sse42_full_live_block(Input *in,Diagonal *d,const uint32_t *route,sse42_FusedHistory *s,
                           uint16_t *north,uint8_t *out,const uint8_t *above,
                           unsigned base,unsigned t,const sse42_FusedPredict p[4],int known,int banded,const sse42_WordPredict word[4])
{
    Work *work=d->work;
    const unsigned w=d->w;
    const __m128i all=_mm_set1_epi8((char)255);
    const sse42_NumericalLanes live=sse42_fused_all_lanes();
    __m128i groups[16],samples[16],codes[16];
    if (known) {
        sse42_known_rows(work->raster,w,base,t,groups);
        sse42_diagonal_transpose(groups);
    } else sse42_byte_live_stage(d,base,t,groups,banded);
    for (unsigned j=0;j<16;++j) {
        const unsigned step=t+j;
        sse42_NumericalLanes m_a,m_b;
        if (!known) {
            m_a=sse42_byte_live_model(route,groups[j],sse42_numerical_lo(s->recent));
            m_b=sse42_byte_live_model(route,_mm_srli_si128(groups[j],8),sse42_numerical_hi(s->recent));
        }
        const __m128i west=s->pix[0];
        const __m128i n=_mm_insert_epi8(_mm_slli_si128(s->pix[3],1),above[step],0);
        const __m128i nw=_mm_insert_epi8(_mm_slli_si128(s->pix[4],1),above[step-1],0);
        const __m128i ne=_mm_insert_epi8(_mm_slli_si128(s->pix[2],1),above[step+1],0);
        __m128i pred;
        if (word) pred=sse42_word_prediction(word+j/4,west,n,nw,ne);
        else {
            const sse42_FusedPredict *fit=p+j/4;
            const __m128i wide=_mm_cmpgt_epi8(fit->mode,_mm_set1_epi8(7));
            pred=sse42_fused_prediction(fit,s->pix,west,n,nw,ne,above,w,step,wide,0);
        }
        __m128i error;
        if (known) error=groups[j];
        else {
            sse42_NumericalLanes ea,eb;
            if (work->directional) {
                __m128i shift=sse42_direction_indices(west,n,nw,pred,all,all);
                m_a=sse42_direction_model(m_a,shift);
                m_b=sse42_direction_model(m_b,_mm_srli_si128(shift,8));
            }
            if (sse42_fused_invalid_routes(m_a,m_b)) return 0;
            ea=sse42_fused_entries(work->tables,s->ans.lo,m_a,live);
            eb=sse42_fused_entries(work->tables,s->ans.hi,m_b,live);
            sse42_FusedPair next={sse42_full_word_next(s->ans.lo,ea),sse42_full_word_next(s->ans.hi,eb)};
            if (!sse42_paired_refill(in,&next)) return 0;
            s->ans=next;
            error=sse42_fused_symbols(ea,eb);
        }
        unsigned top=north[step+2]>north[step+3] ? north[step+2] : north[step+3];
        const sse42_NumericalLanes recent=sse42_fused_update_activity(s->recent,s->older,error,top);
        s->older=s->recent;
        s->recent=recent;
        north[step-60]=(uint16_t)sse42_fused_last_activity(recent);
        if (work->stencil) sse42_retain_fitted_lags(s);
        for (unsigned k=4;k;k--) s->pix[k]=s->pix[k-1];
        s->pix[0]=samples[j]=_mm_add_epi8(pred,error);
        if (work->error) codes[j]=sse42_diagonal_codes(error);
    }
    sse42_byte_live_store(out,samples,w,t);
    if (work->error) sse42_byte_live_store(work->error+(banded ? 0 : (size_t)base*w),codes,w,t);
    return 1;
}

static QLI_SSE42_TARGET void sse42_fused_route(uint32_t route[32*32],const uint8_t *source,unsigned count,int directional)
{
    for (unsigned i=0;i<32;++i)
        for (unsigned k=0;k<32;++k)
            if (i<count && k<CAUSAL_CONTEXTS) {
                unsigned key=i*CAUSAL_CONTEXTS+k;
                route[32*i+k]=directional ? direction_word(source+3*key) : route_offset(source[key]);
            } else {
                route[32*i+k]=directional ? DIRECTION_ROUTE_SKIP : ROUTE_SKIP;
            }
}

static QLI_SSE42_TARGET int sse42_diagonal_fused(Input *in,Diagonal *d,uint32_t states[16],
                               uint8_t *out,unsigned group_count)
{
    Work *work=d->work;
    unsigned w=d->w,h=d->h;
    uint16_t *north=(uint16_t *)work->causal_aux;
    memset(north,0,2*(size_t)(w+3));
    uint32_t route[32*32];
    sse42_fused_route(route,d->route,group_count,work->directional);
    const __m128i selectors[4]={
        _mm_setr_epi8(4,3,3,3,3,2,2,2,2,1,1,1,1,0,0,0),
        _mm_setr_epi8(4,4,3,3,3,3,2,2,2,2,1,1,1,1,0,0),
        _mm_setr_epi8(4,4,4,3,3,3,3,2,2,2,2,1,1,1,1,0),
        _mm_setr_epi8(4,4,4,4,3,3,3,3,2,2,2,2,1,1,1,1)
    };
    __m128i coefficient[11];
    sse42_fused_coefficients(work->stencil ? work->stencil_fit : NULL,coefficient);
    sse42_FusedPair ans={sse42_numerical_load32(states),
                   sse42_numerical_load32((states+8))};
    for (unsigned base=0;base<h;base+=16) {
        unsigned rows=(unsigned)min_size(16,h-base),end=w+4*(rows-1);
        uint8_t *output=out+(size_t)base*w;
        const uint8_t *above=base ? output-w : NULL;
        const uint8_t *modes=work->modes+(size_t)(base/16)*d->mode_cols;
        sse42_FusedHistory history={0};
        history.ans=ans;
        uint8_t dirty[MAX_WAVE_BLOCKS];
        const int full_band=above && rows==16 && w>80 && !work->stencil && !work->directional;
        if (full_band) sse42_known_marks(d->flat,w,base,dirty);
        for (unsigned t=0;t<end;t+=16) {
            uint8_t lut[16]={0};
            for (int k=0;k<5;++k) {
                int tile=(int)(t/16)+k-4;
                lut[k]=modes[tile<0 ? 0 : min_size((unsigned)tile,d->mode_cols-1)];
            }
            __m128i mode_lut=sse42_numerical_load16(lut);
            sse42_FusedPredict prep[4];
            for (unsigned k=0;k<4;++k)
                prep[k]=sse42_fused_predict_setup(coefficient,_mm_shuffle_epi8(mode_lut,selectors[k]));
            unsigned steps=(unsigned)min_size(16,end-t);
            int ok;
            if (full_band && t>=64 && t+16<w && dirty[t/16]!=BLOCK_MIXED) {
                ok=sse42_full_live_block(in,d,route,&history,north,output,above,base,t,prep,
                                   dirty[t/16]==BLOCK_KNOWN,0,NULL);
            } else ok=sse42_fused_block(in,d,route,&history,north,output,above,base,rows,t,steps,prep,0,NULL);
            if (!ok) return 0;
        }
        ans=history.ans;
    }
    sse42_numerical_store32(states,ans.lo);
    sse42_numerical_store32((states+8),ans.hi);
    return 1;
}

typedef struct sse42_BandPlane {
    Input input;
    CausalModels book;
    Diagonal d;
    sse42_FusedPair ans;
    uint8_t mapping[64];
    uint32_t route[32*32];
} sse42_BandPlane;

static QLI_SSE42_TARGET void sse42_prepare_band_plane(Work *work,const Input *in,const CausalModels *book,
                    const Diagonal *d,const uint8_t map[64],unsigned old_count,const uint32_t ans[DIAGONAL_ROWS])
{
    sse42_BandPlane *p=work->defer;
    p->input=*in;
    p->book=*book;
    memcpy(p->mapping,map,sizeof(p->mapping));
    p->d=*d;
    p->d.mapping=p->mapping;
    p->d.route=NULL;
    sse42_fused_route(p->route,d->route,old_count,work->directional);
    memcpy(&p->ans.lo,ans,32);
    memcpy(&p->ans.hi,ans+8,32);
}

typedef struct {
    sse42_FusedHistory history;
    sse42_BandPlane *plane;
    uint8_t *pixels;
    const uint8_t *above;
    const __m128i *stencil;
    unsigned base,rows,t,end;
    uint8_t blocks[MAX_WAVE_BLOCKS];
} sse42_ChromaBand;

typedef struct {
    union { sse42_WordPredict word; sse42_FusedPredict fit; } predict[4];
    unsigned fitted;
    __m128i group[16],sample[16],code[16];
} sse42_ChromaBlock;

static QLI_SSE42_TARGET void sse42_chroma_begin(sse42_ChromaBand *b,sse42_BandPlane *p,uint8_t *out,const uint8_t *above,unsigned base)
{
    memset(&b->history,0,sizeof(b->history));
    b->history.ans=p->ans;
    b->plane=p;
    b->pixels=out;
    b->above=above;
    b->base=base;
    b->rows=(unsigned)min_size(16,p->d.h-base);
    b->t=0;
    b->end=p->d.w+4*(b->rows-1);
    if (above && b->rows==16 && p->d.w>80)
        sse42_known_marks(p->d.flat,p->d.w,base,b->blocks);
}

static QLI_SSE42_TARGET unsigned sse42_chroma_kind_at(const sse42_ChromaBand *b,unsigned t)
{
    if (b->above && b->rows==16 && t>=64 && t+16<b->plane->d.w)
        return b->blocks[t/16];
    return BLOCK_MIXED;
}

static QLI_SSE42_TARGET unsigned sse42_chroma_kind(const sse42_ChromaBand *b)
{
    return sse42_chroma_kind_at(b,b->t);
}

static QLI_SSE42_TARGET void sse42_chroma_modes(const sse42_ChromaBand *b,__m128i mode[4])
{
    static const int8_t selectors[4][16]={
        {4,3,3,3,3,2,2,2,2,1,1,1,1,0,0,0},
        {4,4,3,3,3,3,2,2,2,2,1,1,1,1,0,0},
        {4,4,4,3,3,3,3,2,2,2,2,1,1,1,1,0},
        {4,4,4,4,3,3,3,3,2,2,2,2,1,1,1,1}
    };
    const Diagonal *d=&b->plane->d;
    const uint8_t *modes=d->work->modes+(size_t)(b->base/16)*d->mode_cols;
    uint8_t lut[16]={0};
    for (int k=0;k<5;++k) {
        int tile=(int)(b->t/16)+k-4;
        lut[k]=modes[tile<0 ? 0 : min_size((unsigned)tile,d->mode_cols-1)];
    }
    const __m128i table=sse42_numerical_load16(lut);
    for (unsigned k=0;k<4;++k)
        mode[k]=_mm_shuffle_epi8(table,sse42_numerical_load16(selectors[k]));
}

static QLI_SSE42_TARGET unsigned sse42_chroma_fitted_groups(const sse42_ChromaBand *b,const __m128i mode[4])
{
    unsigned fitted=0;
    if (b->stencil) {
        const __m128i seven=_mm_set1_epi8(7);
        for (unsigned k=0;k<4;++k)
            if (_mm_movemask_epi8(_mm_cmpgt_epi8(mode[k],seven))) fitted|=1u<<k;
    }
    return fitted;
}

static QLI_SSE42_TARGET void sse42_chroma_prepare(const sse42_ChromaBand *b,const __m128i coefficient[11],sse42_ChromaBlock *block)
{
    __m128i mode[4];
    sse42_chroma_modes(b,mode);
    block->fitted=sse42_chroma_fitted_groups(b,mode);
    for (unsigned k=0;k<4;++k) {
        if (block->fitted & (1u<<k))
            block->predict[k].fit=sse42_fused_predict_setup(b->stencil,mode[k]);
        else
            block->predict[k].word=sse42_word_predict_setup(coefficient,mode[k]);
    }
}

static QLI_SSE42_TARGET int sse42_zero_block(const uint8_t *values,unsigned w,unsigned base,unsigned t)
{
    size_t pos=(size_t)base*w+t;
    for (unsigned lane=0;lane<16;++lane,pos+=w-4)
        if (values[pos/256] || values[(pos+15)/256]) return 0;
    return 1;
}

static QLI_SSE42_TARGET int sse42_uniform_history(const sse42_FusedHistory *s,const uint8_t *above,
                                  unsigned w,unsigned t,int fitted)
{
    const __m128i value=_mm_shuffle_epi8(s->pix[0],_mm_setzero_si128());
    __m128i diff=_mm_xor_si128(sse42_numerical_load16(above+t+1),value);
    for (unsigned k=0;k<(fitted ? 9u : 5u);++k)
        diff=_mm_or_si128(diff,_mm_xor_si128(s->pix[k],value));
    if (fitted) {
        diff=_mm_or_si128(diff,_mm_xor_si128(sse42_numerical_load16(above+t-5),value));
        diff=_mm_or_si128(diff,_mm_xor_si128(sse42_numerical_load16(above+t-w-1),value));
        diff=_mm_or_si128(diff,_mm_xor_si128(sse42_numerical_load16(above+t-w+2),value));
    } else diff=_mm_or_si128(diff,_mm_xor_si128(sse42_numerical_load16(above+t-1),value));
    return _mm_testz_si128(diff,diff);
}

static QLI_SSE42_TARGET int sse42_stable_activity(const sse42_FusedHistory *s,const uint16_t *north,unsigned t)
{
    const sse42_NumericalLanes change=sse_xor_si128(s->older,s->recent);
    if (!sse_testz_si256(change,change)) return 0;
    const unsigned head=(unsigned)_mm_extract_epi16(sse42_numerical_lo(s->recent),0);
    const unsigned high=2*head,low=high>3 ? high-3 : 0;
    const sse42_NumericalLanes top=sse_max_epu16(sse42_numerical_load32(north+t+2),sse42_numerical_load32(north+t+3));
    const sse42_NumericalLanes offset=sse_sub_epi16(top,sse_set1_epi16((short)low));
    const sse42_NumericalLanes outside=sse_xor_si128(offset,sse_min_epu16(offset,sse_set1_epi16((short)(high-low))));
    if (!sse_testz_si256(outside,outside)) return 0;
    const sse42_NumericalLanes next=sse42_fused_update_activity(s->recent,s->older,_mm_setzero_si128(),high);
    const sse42_NumericalLanes diff=sse_xor_si128(next,s->recent);
    return sse_testz_si256(diff,diff);
}

static QLI_SSE42_TARGET int sse42_fill_known_block(sse42_ChromaBand *b,uint16_t *north)
{
    const Diagonal *d=&b->plane->d;
    const Work *work=d->work;
    const unsigned w=d->w,t=b->t;
    sse42_FusedHistory *s=&b->history;
    if (!sse42_uniform_history(s,b->above,w,t,work->stencil) ||
        (d->nonzero_constants && !sse42_zero_block(work->raster,w,b->base,t)) ||
        !sse42_stable_activity(s,north,t)) return 0;
    sse42_numerical_store32(north+t-60,sse_set1_epi16((short)sse42_fused_last_activity(s->recent)));
    for (unsigned lane=0;lane<16;++lane)
        sse42_numerical_store16(b->pixels+t+(size_t)lane*(w-4),s->pix[0]);
    if (work->error)
        for (unsigned lane=0;lane<16;++lane)
            sse42_numerical_store16(work->error+t+(size_t)lane*(w-4),_mm_setzero_si128());
    return 1;
}

static QLI_SSE42_TARGET __attribute__((noinline)) int sse42_chroma_single(sse42_ChromaBand *b,const __m128i coefficient[11])
{
    sse42_BandPlane *p=b->plane;
    Diagonal *d=&p->d;
    uint16_t *north=(uint16_t *)d->work->causal_aux;
    const unsigned kind=sse42_chroma_kind(b);
    if (kind==BLOCK_KNOWN && sse42_fill_known_block(b,north)) {
        b->t+=16;
        return 1;
    }
    __m128i mode[4];
    sse42_chroma_modes(b,mode);
    if (sse42_chroma_fitted_groups(b,mode)) {
        sse42_FusedPredict predict[4];
        for (unsigned k=0;k<4;++k) predict[k]=sse42_fused_predict_setup(b->stencil,mode[k]);
        unsigned steps=(unsigned)min_size(16,b->end-b->t);
        int ok;
        if (kind==BLOCK_MIXED)
            ok=sse42_fused_block(&p->input,d,p->route,&b->history,north,b->pixels,b->above,
                           b->base,b->rows,b->t,steps,predict,1,NULL);
        else
            ok=sse42_full_live_block(&p->input,d,p->route,&b->history,north,b->pixels,b->above,
                               b->base,b->t,predict,kind==BLOCK_KNOWN,1,NULL);
        b->t+=16;
        return ok;
    }
    sse42_WordPredict predict[4];
    for (unsigned k=0;k<4;++k) predict[k]=sse42_word_predict_setup(coefficient,mode[k]);
    int ok;
    if (kind==BLOCK_MIXED) {
        unsigned steps=(unsigned)min_size(16,b->end-b->t);
        ok=sse42_fused_block(&p->input,d,p->route,&b->history,north,b->pixels,b->above,
                       b->base,b->rows,b->t,steps,NULL,1,predict);
    } else {
        ok=sse42_full_live_block(&p->input,d,p->route,&b->history,north,b->pixels,b->above,
                           b->base,b->t,NULL,kind==BLOCK_KNOWN,1,predict);
    }
    b->t+=16;
    return ok;
}

static QLI_SSE42_TARGET inline sse42_FusedPair sse42_chroma_model(const sse42_ChromaBand *b,__m128i group)
{
    const uint32_t *route=b->plane->route;
    sse42_NumericalLanes recent=b->history.recent;
    return (sse42_FusedPair){
        sse42_byte_live_model(route,group,sse42_numerical_lo(recent)),
        sse42_byte_live_model(route,_mm_srli_si128(group,8),sse42_numerical_hi(recent))
    };
}

static QLI_SSE42_TARGET inline sse42_FusedPair sse42_chroma_entries(const sse42_ChromaBand *b,sse42_FusedPair model)
{
    const uint32_t *table=b->plane->d.work->tables;
    sse42_FusedPair s=b->history.ans;
    const sse42_NumericalLanes live=sse42_fused_all_lanes();
    return (sse42_FusedPair){
        sse42_fused_entries(table,s.lo,model.lo,live),
        sse42_fused_entries(table,s.hi,model.hi,live)
    };
}

static QLI_SSE42_TARGET inline sse42_FusedPair sse42_chroma_update(sse42_FusedPair state,sse42_FusedPair entry)
{
    return (sse42_FusedPair){sse42_full_word_next(state.lo,entry.lo),sse42_full_word_next(state.hi,entry.hi)};
}

static QLI_SSE42_TARGET inline __m128i sse42_chroma_prediction(sse42_ChromaBand *b,sse42_ChromaBlock *block,unsigned j,__m128i *shift)
{
    const sse42_FusedHistory *s=&b->history;
    const unsigned step=b->t+j;
    const __m128i west=s->pix[0],n=_mm_insert_epi8(_mm_slli_si128(s->pix[3],1),b->above[step],0);
    const __m128i nw=_mm_insert_epi8(_mm_slli_si128(s->pix[4],1),b->above[step-1],0);
    const __m128i ne=_mm_insert_epi8(_mm_slli_si128(s->pix[2],1),b->above[step+1],0);
    const unsigned group=j/4;
    __m128i pred;
    if (block->fitted & (1u<<group)) {
        const sse42_FusedPredict *fit=&block->predict[group].fit;
        const __m128i wide=_mm_cmpgt_epi8(fit->mode,_mm_set1_epi8(7));
        pred=sse42_fused_prediction(fit,s->pix,west,n,nw,ne,b->above,b->plane->d.w,step,wide,1);
    } else {
        pred=sse42_word_prediction(&block->predict[group].word,west,n,nw,ne);
    }
    if (shift) {
        const __m128i all=_mm_set1_epi8(-1);
        *shift=sse42_direction_indices(west,n,nw,pred,all,all);
    }
    return pred;
}

static QLI_SSE42_TARGET inline sse42_FusedPair sse42_chroma_direction(sse42_ChromaBand *b,sse42_ChromaBlock *block,unsigned j,
                                        sse42_FusedPair model,__m128i *pred)
{
    if (b->plane->d.work->directional) {
        __m128i shift;
        *pred=sse42_chroma_prediction(b,block,j,&shift);
        model.lo=sse42_direction_model(model.lo,shift);
        model.hi=sse42_direction_model(model.hi,_mm_srli_si128(shift,8));
    }
    return model;
}

static QLI_SSE42_TARGET inline void sse42_chroma_reconstruct(sse42_ChromaBand *b,sse42_ChromaBlock *block,unsigned j,sse42_FusedPair entry,__m128i *codes,__m128i pred)
{
    Work *work=b->plane->d.work;
    sse42_FusedHistory *s=&b->history;
    uint16_t *north=(uint16_t *)work->causal_aux;
    const unsigned step=b->t+j;
    const __m128i error=sse42_fused_symbols(entry.lo,entry.hi);
    unsigned top=north[step+2]>north[step+3] ? north[step+2] : north[step+3];
    const sse42_NumericalLanes recent=sse42_fused_update_activity(s->recent,s->older,error,top);
    s->older=s->recent;
    s->recent=recent;
    north[step-60]=(uint16_t)sse42_fused_last_activity(recent);
    if (work->stencil) sse42_retain_fitted_lags(s);
    for (unsigned k=4;k;k--) s->pix[k]=s->pix[k-1];
    s->pix[0]=block->sample[j]=_mm_add_epi8(pred,error);
    if (work->error) codes[j]=sse42_diagonal_codes(error);
}

static QLI_SSE42_TARGET __attribute__((noinline)) int sse42_chroma_pair(sse42_ChromaBand *a,sse42_ChromaBand *b,const __m128i coefficient[11])
{
    Diagonal *d=&a->plane->d,*e=&b->plane->d;
    sse42_ChromaBlock x,y;
    sse42_chroma_prepare(a,coefficient,&x);
    sse42_chroma_prepare(b,coefficient,&y);
    sse42_byte_live_stage(d,a->base,a->t,x.group,1);
    sse42_byte_live_stage(e,b->base,b->t,y.group,1);
    for (unsigned j=0;j<16;++j) {
        sse42_FusedPair ma=sse42_chroma_model(a,x.group[j]),mb=sse42_chroma_model(b,y.group[j]);
        __m128i pa,pb;
        ma=sse42_chroma_direction(a,&x,j,ma,&pa);
        mb=sse42_chroma_direction(b,&y,j,mb,&pb);
        if (sse42_fused_invalid_routes(sse_or_si128(ma.lo,mb.lo),
                                 sse_or_si128(ma.hi,mb.hi))) return 0;
        sse42_FusedPair ea=sse42_chroma_entries(a,ma),eb=sse42_chroma_entries(b,mb);
        sse42_FusedPair sa=sse42_chroma_update(a->history.ans,ea),sb=sse42_chroma_update(b->history.ans,eb);
        if (!sse42_paired_refill(&a->plane->input,&sa) || !sse42_paired_refill(&b->plane->input,&sb)) return 0;
        a->history.ans=sa;
        b->history.ans=sb;
        sse42_chroma_reconstruct(a,&x,j,ea,x.code,
            d->work->directional ? pa : sse42_chroma_prediction(a,&x,j,NULL));
        sse42_chroma_reconstruct(b,&y,j,eb,y.code,
            e->work->directional ? pb : sse42_chroma_prediction(b,&y,j,NULL));
    }
    sse42_byte_live_store(a->pixels,x.sample,d->w,a->t);
    sse42_byte_live_store(b->pixels,y.sample,e->w,b->t);
    if (d->work->error) sse42_byte_live_store(d->work->error,x.code,d->w,a->t);
    if (e->work->error) sse42_byte_live_store(e->work->error,y.code,e->w,b->t);
    a->t+=16;
    b->t+=16;
    return 1;
}

static QLI_SSE42_TARGET int sse42_chroma_whole(sse42_ChromaBand *b,const __m128i coefficient[11])
{
    while (b->t<b->end) if (!sse42_chroma_single(b,coefficient)) return 0;
    b->plane->ans=b->history.ans;
    return 1;
}


typedef struct { sse42_NumericalLanes low, high; } sse42_CarrierRoute;

static QLI_SSE42_TARGET inline sse42_CarrierRoute sse42_carrier_route(const uint32_t *route)
{
    const __m128i a=_mm_packus_epi32(_mm_srli_epi32(sse42_numerical_load16(route),12),
                                     _mm_srli_epi32(sse42_numerical_load16((route+4)),12));
    const __m128i b=_mm_packus_epi32(_mm_srli_epi32(sse42_numerical_load16((route+8)),12),
                                     _mm_srli_epi32(sse42_numerical_load16((route+12)),12));
    const __m128i low=_mm_packus_epi16(a,b);
    const __m128i high=_mm_setr_epi8((char)(route[16]>>12),(char)(route[17]>>12),0,0,0,0,0,0,0,0,0,0,0,0,0,0);
    return (sse42_CarrierRoute){sse42_numerical_pair(low,low),sse42_numerical_pair(high,high)};
}

static QLI_SSE42_TARGET inline sse42_NumericalLanes sse42_carrier_model(sse42_CarrierRoute route,__m128i activity)
{
    sse42_NumericalLanes bin=sse42_activity_bins(activity);
    const sse42_NumericalLanes models=sse_blendv_epi8(sse_shuffle_epi8(route.low,bin),
                                           sse_shuffle_epi8(route.high,bin),
                                           sse_cmpgt_epi32(bin,sse_set1_epi32(15)));
    return sse_slli_epi32(sse_and_si128(models,sse_set1_epi32(63)),12);
}

static QLI_SSE42_TARGET inline sse42_FusedPair sse42_carrier_block_model(
    const sse42_ChromaBand *a,sse42_CarrierRoute carrier)
{
    if (a->plane->d.work->directional) return sse42_chroma_model(a,_mm_setzero_si128());
    sse42_NumericalLanes recent=a->history.recent;
    return (sse42_FusedPair){sse42_carrier_model(carrier,sse42_numerical_lo(recent)),
                       sse42_carrier_model(carrier,sse42_numerical_hi(recent))};
}

static QLI_SSE42_TARGET __attribute__((noinline)) int sse42_chroma_triple(sse42_ChromaBand *a,sse42_ChromaBand *b,sse42_ChromaBand *c,
                                                  const __m128i coefficient[11],
                                                  __m128i *oldest,const __m128i *newer,__m128i *chroma)
{
    Diagonal *d=&a->plane->d,*e=&b->plane->d,*f=&c->plane->d;
    sse42_ChromaBlock x,y,z;
    sse42_chroma_prepare(a,coefficient,&x);
    sse42_chroma_prepare(b,coefficient,&y);
    sse42_chroma_prepare(c,coefficient,&z);
    sse42_CarrierRoute carrier={0};
    if (!d->work->directional) carrier=sse42_carrier_route(a->plane->route);
    if (oldest) {
        for (unsigned j=0;j<16;++j) {
            const __m128i key=_mm_add_epi8(_mm_sub_epi8(_mm_slli_epi16(oldest[j],3),oldest[j]),chroma[j]);
            y.group[j]=sse42_fused_map(e->mapping,newer[j],1);
            z.group[j]=sse42_fused_map(f->mapping,key,2);
        }
    } else {
        sse42_byte_live_stage(e,b->base,b->t,y.group,1);
        sse42_byte_live_stage(f,c->base,c->t,z.group,1);
    }
    __m128i *next_carrier=oldest ? oldest : x.code;
    __m128i *next_chroma=oldest ? chroma : y.code;
    for (unsigned j=0;j<16;++j) {
        sse42_FusedPair ma=sse42_carrier_block_model(a,carrier);
        sse42_FusedPair mb=sse42_chroma_model(b,y.group[j]),mc=sse42_chroma_model(c,z.group[j]);
        __m128i pa,pb,pc;
        ma=sse42_chroma_direction(a,&x,j,ma,&pa);
        mb=sse42_chroma_direction(b,&y,j,mb,&pb);
        mc=sse42_chroma_direction(c,&z,j,mc,&pc);
        if (sse42_fused_invalid_routes(sse_or_si128(sse_or_si128(ma.lo,mb.lo),mc.lo),
                                 sse_or_si128(sse_or_si128(ma.hi,mb.hi),mc.hi))) return 0;
        sse42_FusedPair ea=sse42_chroma_entries(a,ma),eb=sse42_chroma_entries(b,mb),ec=sse42_chroma_entries(c,mc);
        sse42_FusedPair sa=sse42_chroma_update(a->history.ans,ea),sb=sse42_chroma_update(b->history.ans,eb);
        sse42_FusedPair sc=sse42_chroma_update(c->history.ans,ec);
        sse42_chroma_reconstruct(a,&x,j,ea,next_carrier,
            d->work->directional ? pa : sse42_chroma_prediction(a,&x,j,NULL));
        sse42_chroma_reconstruct(b,&y,j,eb,next_chroma,
            e->work->directional ? pb : sse42_chroma_prediction(b,&y,j,NULL));
        sse42_chroma_reconstruct(c,&z,j,ec,z.code,
            f->work->directional ? pc : sse42_chroma_prediction(c,&z,j,NULL));
        if (!sse42_paired_refill(&a->plane->input,&sa) || !sse42_paired_refill(&b->plane->input,&sb) ||
            !sse42_paired_refill(&c->plane->input,&sc)) return 0;
        a->history.ans=sa;
        b->history.ans=sb;
        c->history.ans=sc;
    }
    sse42_byte_live_store(a->pixels,x.sample,d->w,a->t);
    sse42_byte_live_store(b->pixels,y.sample,e->w,b->t);
    sse42_byte_live_store(c->pixels,z.sample,f->w,c->t);
    if (!oldest) {
        sse42_byte_live_store(d->work->error,x.code,d->w,a->t);
        sse42_byte_live_store(e->work->error,y.code,e->w,b->t);
    }
    if (f->work->error) sse42_byte_live_store(f->work->error,z.code,f->w,c->t);
    a->t+=16;
    b->t+=16;
    c->t+=16;
    return 1;
}

static QLI_SSE42_TARGET void sse42_guide_load(__m128i values[16],const uint8_t *raster,unsigned w,unsigned t)
{
    for (unsigned lane=0;lane<16;++lane)
        values[lane]=sse42_numerical_load16((raster+(size_t)lane*w+t-4*lane));
    sse42_diagonal_transpose(values);
}

static QLI_SSE42_TARGET unsigned sse42_triple_run_steps(const sse42_ChromaBand *a,const sse42_ChromaBand *b,const sse42_ChromaBand *c)
{
    unsigned offset=16;
    while (sse42_chroma_kind_at(a,a->t+offset)==BLOCK_CODED &&
           sse42_chroma_kind_at(b,b->t+offset)==BLOCK_CODED &&
           sse42_chroma_kind_at(c,c->t+offset)==BLOCK_CODED)
        offset+=16;
    return offset;
}

static QLI_SSE42_TARGET __attribute__((noinline)) int sse42_chroma_triple_run(
    sse42_ChromaBand *a,sse42_ChromaBand *b,sse42_ChromaBand *c,const __m128i coefficient[11],unsigned steps)
{
    __m128i carrier[2][16],first_chroma[16];
    __m128i *oldest=carrier[0],*newer=carrier[1];
    uint8_t *carrier_raster=a->plane->d.work->error;
    uint8_t *chroma_raster=b->plane->d.work->error;
    unsigned w=a->plane->d.w, stop=a->t+steps;
    sse42_guide_load(oldest,carrier_raster,w,c->t);
    sse42_guide_load(newer,carrier_raster,w,b->t);
    sse42_guide_load(first_chroma,chroma_raster,w,c->t);
    do {
        if (!sse42_chroma_triple(a,b,c,coefficient,oldest,newer,first_chroma)) return 0;
        __m128i *next=oldest;
        oldest=newer;
        newer=next;
    } while (a->t<stop);
    sse42_byte_live_store(carrier_raster,oldest,w,c->t);
    sse42_byte_live_store(carrier_raster,newer,w,b->t);
    sse42_byte_live_store(chroma_raster,first_chroma,w,c->t);
    return 1;
}

static QLI_SSE42_TARGET int sse42_chroma_pipeline(sse42_ChromaBand *a,sse42_ChromaBand *b,sse42_ChromaBand *c,const __m128i coefficient[11])
{
    if (a->end<=32)
        return sse42_chroma_whole(a,coefficient) && sse42_chroma_whole(b,coefficient) && sse42_chroma_whole(c,coefficient);
    if (!sse42_chroma_single(a,coefficient) || !sse42_chroma_single(a,coefficient) ||
        !sse42_chroma_single(b,coefficient)) return 0;
    while (a->t<a->end) {
        int ready_a=sse42_chroma_kind(a)==BLOCK_CODED,ready_b=sse42_chroma_kind(b)==BLOCK_CODED;
        int ready_c=sse42_chroma_kind(c)==BLOCK_CODED;
        if (ready_a && ready_b && ready_c) {
            unsigned steps=sse42_triple_run_steps(a,b,c);
            if (steps>=48) {
                if (!sse42_chroma_triple_run(a,b,c,coefficient,steps)) return 0;
            } else {
                do {
                    if (!sse42_chroma_triple(a,b,c,coefficient,NULL,NULL,NULL)) return 0;
                    steps-=16;
                } while (steps);
            }
        } else if (ready_b && ready_c) {
            if (!sse42_chroma_single(a,coefficient) || !sse42_chroma_pair(b,c,coefficient)) return 0;
        } else if (ready_a && ready_b) {
            if (!sse42_chroma_pair(a,b,coefficient) || !sse42_chroma_single(c,coefficient)) return 0;
        } else if (!sse42_chroma_single(a,coefficient) || !sse42_chroma_single(b,coefficient) ||
                   !sse42_chroma_single(c,coefficient)) return 0;
    }
    return sse42_chroma_whole(a,coefficient) && sse42_chroma_whole(b,coefficient) && sse42_chroma_whole(c,coefficient);
}
#endif

typedef struct {
    uint8_t *pixels;
    const uint8_t *above;
    const uint8_t *guide[2];
    uint8_t *errors;
    size_t offset;
} ActiveWaveRow;

static inline unsigned active_wave_direction(unsigned x, unsigned y, unsigned left,
                                             unsigned n, unsigned nw, unsigned prediction)
{
    if (!x || !y) return 1;
    int sum = (int)(left + n);
    int p = (int)prediction;
    int delta = (sum + 1) / 2 - p;
    if (x >= 2 && y >= 2) {
        int g = sum - (int)nw;
        int gradient = g < 0 ? 0 : g > 255 ? 255 : g;
        delta += 256 * (gradient - p);
    }
    return (unsigned)(1 + (delta > 0) - (delta < 0));
}

enum { ACTIVE_WAVE_ROUTE_STRIDE = 3 * CAUSAL_CONTEXTS };

static QLI_NOINLINE void active_wave_tables(const Diagonal *d, unsigned guides,
                                                     uint8_t *route, uint8_t *activity,
                                                     uint16_t *update)
{
    unsigned count = guides == 2 ? 49 : guides == 1 ? 7 : 1;
    for (unsigned cue = 0; cue < count; ++cue) {
        unsigned group = guides ? d->mapping[cue] : 0;
        memcpy(route + cue * ACTIVE_WAVE_ROUTE_STRIDE, d->route + group * ACTIVE_WAVE_ROUTE_STRIDE, ACTIVE_WAVE_ROUTE_STRIDE);
    }
    unsigned boundary = 2, bin = 0;
    for (unsigned i = 0; i < DIAGONAL_STATES; ++i) {
        while (i * i + 1 >= boundary && bin < 17) {
            ++bin;
            boundary *= 2;
        }
        activity[i] = (uint8_t)bin;
    }
    for (unsigned e = 0; e < 256; ++e) {
        unsigned folded = e < 128 ? 2 * e : 2 * (256 - e) - 1;
        update[e] = (uint16_t)(8 * folded + 3 + (diagonal_cue(e) << 11));
    }
}

static int diagonal_wave(Input *in, const Diagonal *d, uint32_t ans[DIAGONAL_ROWS],
                         uint8_t *output, int dense)
{
    Work *work = d->work;
    unsigned w = d->w;
    uint16_t *north = (uint16_t *)work->causal_aux;
    memset(north, 0, sizeof(*north) * (w + 3));

    uint8_t route[49 * ACTIVE_WAVE_ROUTE_STRIDE], activity[DIAGONAL_STATES];
    uint16_t update[256];
    unsigned guides = work->guides;
    active_wave_tables(d, guides, route, activity, update);

    const uint8_t *at = in->at;
    for (unsigned base = 0; base < d->h; base += DIAGONAL_ROWS) {
        const unsigned rows = (unsigned)min_size(DIAGONAL_ROWS, d->h - base);
        const uint8_t *modes = work->modes + (size_t)(base / 16) * d->mode_cols;
        ActiveWaveRow row[DIAGONAL_ROWS];
        uint16_t previous[DIAGONAL_ROWS] = {0}, lag[DIAGONAL_ROWS] = {0};
        for (unsigned k = 0; k < rows; ++k) {
            unsigned y = base + k;
            size_t offset = (size_t)y * w;
            row[k] = (ActiveWaveRow){
                .pixels = output + offset,
                .above = y ? output + offset - w : NULL,
                .guide = {guides ? work->earlier[0] + offset : NULL,
                          guides == 2 ? work->earlier[1] + offset : NULL},
                .errors = work->error ? work->error + offset : NULL,
                .offset = offset,
            };
        }
        for (unsigned t = 0; t < w + DIAGONAL_SKEW * (rows - 1); ++t) {
            unsigned first = t >= w ? (t - w) / DIAGONAL_SKEW + 1 : 0;
            unsigned end = (unsigned)min_size(rows, t / DIAGONAL_SKEW + 1);
            if (first >= end) continue;
            uint8_t tail[2 * DIAGONAL_ROWS];
            size_t remaining = (size_t)(in->end - at);
            int short_tail = remaining < 2 * (end - first);
            const uint8_t *cursor = at;
            if (short_tail) {
                memset(tail, 0, sizeof(tail));
                memcpy(tail, at, remaining);
                cursor = tail;
            }
            if (dense) {
                uint32_t states[DIAGONAL_ROWS];
                uint16_t updates[DIAGONAL_ROWS];
                uint8_t pixels[DIAGONAL_ROWS];
                for (unsigned k = first; k < end; ++k) {
                    const ActiveWaveRow *r = row + k;
                    unsigned x = t - DIAGONAL_SKEW * k;
                    uint8_t *pixel = r->pixels + x;
                    unsigned left = x ? pixel[-1] : r->above ? r->above[0] : 128;
                    unsigned n = r->above ? r->above[x] : left;
                    unsigned nw = r->above && x ? r->above[x - 1] : n;
                    unsigned ne = r->above && x + 1 < w ? r->above[x + 1] : n;
                    unsigned mode = modes[x / 16];
                    unsigned prediction = mode >= 8 ? stencil_scalar(work->stencil_fit + 10 * (mode - 8), pixel, w) :
                                                      scalar_predict(mode, left, n, nw, ne, 255);
                    unsigned cue = r->guide[0] ? r->guide[0][x] : 0;
                    if (r->guide[1]) cue = 7 * cue + r->guide[1][x];
                    unsigned dir = active_wave_direction(x, base + k, left, n, nw, prediction);
                    unsigned model = route[cue * ACTIVE_WAVE_ROUTE_STRIDE + 3 * activity[previous[k]] + dir];
                    if (model == CAUSAL_SKIP) return 0;
                    uint32_t a = ans[k];
                    uint32_t entry = work->tables[((size_t)model << 12) + (a & 4095)];
                    unsigned e = (uint8_t)entry;
                    states[k] = (a >> 12) * ((entry >> 20) + 1) + ((entry >> 8) & 4095);
                    updates[k] = update[e];
                    pixels[k] = (uint8_t)(e + prediction);
                }
                unsigned prior = 0, older = 0;
                if (first) {
                    unsigned past = t - DIAGONAL_SKEW * (first - 1) - w;
                    prior = past == 0 ? previous[first - 1] : 0;
                    older = past == 0 ? lag[first - 1] : past == 1 ? previous[first - 1] : 0;
                }
                for (unsigned k = first; k < end; ++k) {
                    const ActiveWaveRow *r = row + k;
                    unsigned x = t - DIAGONAL_SKEW * k;
                    uint32_t state = states[k];
                    unsigned refill = state < CAUSAL_LOW;
                    ans[k] = (state << (16 * refill)) | (qli_read_le16(cursor) & (0u - refill));
                    cursor += 2 * refill;
                    unsigned west = previous[k];
                    unsigned a = k ? older : north[x + 2];
                    unsigned b = k ? prior : north[x + 3];
                    unsigned packed = updates[k];
                    unsigned next = (2 * west + (packed & 2047) + (a > b ? a : b)) / 4;
                    if (k == DIAGONAL_ROWS - 1) north[x] = (uint16_t)next;
                    if (r->errors) r->errors[x] = (uint8_t)(packed >> 11);
                    r->pixels[x] = pixels[k];
                    older = lag[k];
                    prior = west;
                    lag[k] = (uint16_t)west;
                    previous[k] = (uint16_t)next;
                }
            } else {
                unsigned prior = 0, older = 0;
                if (first) {
                    unsigned past = t - DIAGONAL_SKEW * (first - 1) - w;
                    prior = past == 0 ? previous[first - 1] : 0;
                    older = past == 0 ? lag[first - 1] : past == 1 ? previous[first - 1] : 0;
                }
                for (unsigned k = first; k < end; ++k) {
                    const ActiveWaveRow *r = row + k;
                    unsigned x = t - DIAGONAL_SKEW * k;
                    uint8_t *pixel = r->pixels + x;
                    unsigned left = x ? pixel[-1] : r->above ? r->above[0] : 128;
                    unsigned n = r->above ? r->above[x] : left;
                    unsigned nw = r->above && x ? r->above[x - 1] : n;
                    unsigned ne = r->above && x + 1 < w ? r->above[x + 1] : n;
                    unsigned mode = modes[x / 16];
                    unsigned prediction = mode >= 8 ? stencil_scalar(work->stencil_fit + 10 * (mode - 8), pixel, w) :
                                                      scalar_predict(mode, left, n, nw, ne, 255);
                    unsigned west = previous[k];
                    size_t block = (r->offset + x) / 256;
                    unsigned e;
                    if (flag(d->flat, block)) {
                        e = work->raster[block];
                    } else {
                        unsigned cue = r->guide[0] ? r->guide[0][x] : 0;
                        if (r->guide[1]) cue = 7 * cue + r->guide[1][x];
                        unsigned dir = active_wave_direction(x, base + k, left, n, nw, prediction);
                        unsigned model = route[cue * ACTIVE_WAVE_ROUTE_STRIDE + 3 * activity[west] + dir];
                        if (model == CAUSAL_SKIP) return 0;
                        uint32_t a = ans[k];
                        uint32_t entry = work->tables[((size_t)model << 12) + (a & 4095)];
                        a = (a >> 12) * ((entry >> 20) + 1) + ((entry >> 8) & 4095);
                        unsigned refill = a < CAUSAL_LOW;
                        a = (a << (16 * refill)) | (qli_read_le16(cursor) & (0u - refill));
                        cursor += 2 * refill;
                        ans[k] = a;
                        e = (uint8_t)entry;
                    }
                    unsigned a = k ? older : north[x + 2];
                    unsigned b = k ? prior : north[x + 3];
                    unsigned packed = update[e];
                    unsigned next = (2 * west + (packed & 2047) + (a > b ? a : b)) / 4;
                    if (k == DIAGONAL_ROWS - 1) north[x] = (uint16_t)next;
                    if (r->errors) r->errors[x] = (uint8_t)(packed >> 11);
                    *pixel = (uint8_t)(e + prediction);
                    older = lag[k];
                    prior = west;
                    lag[k] = (uint16_t)west;
                    previous[k] = (uint16_t)next;
                }
            }
            if (short_tail) {
                size_t consumed = (size_t)(cursor - tail);
                if (consumed > remaining) return 0;
                at += consumed;
            } else {
                at = cursor;
            }
        }
    }
    in->at = at;
    return 1;
}

static int diagonal_payload(Input *in,Work *work,const uint8_t *flat,const uint8_t *constant,
                            unsigned w,unsigned h,size_t literals,uint8_t *output)
{
    unsigned old_count=work->guides ? CAUSAL_MODELS : 1;
    uint8_t map[64]={0};
    if (work->guides) {
        unsigned width=work->guides==2 ? 49 : 7;
        const uint8_t *mapping;
        if (!record(in,width,map,&mapping)) return 0;
        for (unsigned j=0;j<width;++j)
            if (mapping[j]>=CAUSAL_MODELS) return 0;
        if (mapping!=map) memcpy(map,mapping,width);
    }
    CausalModels book;
    unsigned route_size=old_count*CAUSAL_CONTEXTS*(work->directional ? 3u : 1u);
    uint8_t scratch[3*32*CAUSAL_CONTEXTS];
    const uint8_t *route;
    if (!record(in,route_size,scratch,&route) ||
        !read_causal_models(in,&book)) return 0;
    const unsigned models=book.count;
    if (!work->defer && (models>work->table_models ||
        !build_causal_tables(&book,work->tables))) return 0;
    for (unsigned i=0;i<route_size;++i)
        if (route[i]!=CAUSAL_SKIP && route[i]>=models) return 0;
    size_t n=(size_t)w*h;
    size_t blocks=(n+255)/256;
    unsigned nonzero_constants=0;
    for (size_t i=0;i<blocks;++i) {
        if (flag(flat,i)) {
            uint8_t value=*constant++;
            work->raster[i]=value;
            nonzero_constants|=value;
        }
    }
    size_t bytes=(size_t)(in->end-in->at);
    const unsigned header=4*DIAGONAL_ROWS;
    if (bytes<header || (bytes&1u) || bytes>header+2*literals) return 0;
    uint32_t ans[DIAGONAL_ROWS];
    for (unsigned k=0;k<DIAGONAL_ROWS;++k) {
        ans[k]=qli_read_le32(in->at+4*k);
        if (ans[k]<CAUSAL_LOW) return 0;
    }
    in->at+=header;
    Diagonal d={work,flat,map,route,w,h,(w+15)/16,nonzero_constants};
#ifdef QLI_NUMERIC_SSE42
    if (work->defer) {
        QLI_NUMERIC_FN(work->vector, prepare_band_plane)(work,in,&book,&d,map,old_count,ans);
        return 1;
    }
#endif
    int ok;
#ifdef QLI_NUMERIC_SSE42
    if (work->vector)
        ok=QLI_NUMERIC_FN(work->vector, diagonal_fused)(in,&d,ans,output,old_count);
    else
#endif
    if (work->directional)
        ok=diagonal_wave(in,&d,ans,output,literals==n);
    else
        ok=diagonal_scalar(in,&d,ans,output);
    if (!ok || in->at!=in->end) return 0;
    for (unsigned k=0;k<DIAGONAL_ROWS;++k) if (ans[k]!=CAUSAL_LOW) return 0;
    return 1;
}

#ifdef QLI_NUMERIC_AVX2

static QLI_NUMERIC_TARGET int fill_proven_wave(uint8_t *out,const uint8_t *above,unsigned x,unsigned w,
                            __m128i prev1,__m128i prev2,__m128i prev3) {
    unsigned value=(unsigned)_mm_extract_epi16(prev1,0);
    __m128i target=_mm_set1_epi16((short)value);
    __m128i different=_mm_xor_si128(prev1,target);
    different=_mm_or_si128(different,_mm_xor_si128(prev2,target));
    different=_mm_or_si128(different,_mm_xor_si128(prev3,target));
    if (!_mm_testz_si128(different,different)) return 0;
    uint64_t repeated=UINT64_C(0x0101010101010101)*value;
    if (above) {
        uint64_t north;
        memcpy(&north,above+x-1,8);
        if (north!=repeated || above[x+7]!=value || above[x+8]!=value) return 0;
    }
    for (unsigned r=0;r<8;++r) memcpy(out+(size_t)r*w+x-2*r,&repeated,8);
    return 1;
}

static QLI_NUMERIC_TARGET void put_byte_rows(__m128i group[8], uint8_t *out, int width, int rows, int t) {
    transpose(group);
    for (int lane=0; lane<rows; ++lane) {
        int x=t-2*lane;
        uint8_t *row=out+(size_t)lane*width;
        __m128i bytes=_mm_packus_epi16(group[lane],group[lane]);
        if (x>=0 && x+8<=width) {
            numerical_store8((row+x),bytes);
        } else {
            int low=x<0 ? -x : 0;
            int high=x+8>width ? width-x : 8;
            if (low<high) {
                uint8_t clipped[8];
                numerical_store8(clipped,bytes);
                memcpy(row+(x+low),clipped+low,(size_t)(high-low));
            }
        }
    }
}

static QLI_NUMERIC_TARGET QLI_INLINE void interior_wave(__m128i group[8], uint8_t *out,
                                        const uint8_t *above, size_t width,
                                        unsigned x, unsigned left, unsigned right,
                                        __m128i *one, __m128i *two, __m128i *three) {
    __m128i prev1=*one, prev2=*two, prev3=*three;
    const __m128i offsets=_mm_setr_epi16(0,2,4,6,8,10,12,14);
    const __m128i maxv=_mm_set1_epi16(255);
    const uint8_t *north=above+x;
    #pragma clang loop unroll(full)
    for (unsigned i=0; i<8; ++i) {
        __m128i n=_mm_insert_epi16(_mm_slli_si128(prev2,2),north[i],0);
        __m128i nw=_mm_insert_epi16(_mm_slli_si128(prev3,2),north[(int)i-1],0);
        __m128i ne=_mm_insert_epi16(_mm_slli_si128(prev1,2),north[i+1],0);
        __m128i p=predict8(right,prev1,n,nw,ne,maxv);
        unsigned phase=(x&15u)+i;
        if (left!=right && phase<14) {
            __m128i mask=_mm_cmpgt_epi16(offsets,_mm_set1_epi16((short)phase));
            p=_mm_blendv_epi8(p,predict8(left,prev1,n,nw,ne,maxv),mask);
        }
        group[i]=_mm_and_si128(_mm_add_epi16(p,group[i]),maxv);
        prev3=prev2;
        prev2=prev1;
        prev1=group[i];
    }
    *one=prev1;
    *two=prev2;
    *three=prev3;
    transpose(group);
    for (size_t lane=0; lane<8; ++lane) {
        __m128i bytes=_mm_packus_epi16(group[lane],group[lane]);
        numerical_store8((out+lane*width+x-2*lane),bytes);
    }
}

static QLI_NUMERIC_TARGET int inverse_tiles_native(Work *work, unsigned w, unsigned h, const uint8_t *flat,
                       const uint8_t *constant, Queue bank[BANKS], uint8_t *output) {
    size_t blocks=((size_t)w*h+255)/256;
    uint8_t *constants=work->raster;
    for (size_t i=0;i<blocks;++i)
        if (flag(flat,i)) constants[i]=*constant++;
    unsigned steps=(w+21)&~7u, cols=(w+7)/8;
    int width=(int)w, half=128;
    size_t tiles_x=(w+15)/16;
    const __m128i offsets=_mm_setr_epi16(0,2,4,6,8,10,12,14);
    const __m128i zero=_mm_setzero_si128();
    const __m128i lane0=_mm_setr_epi16(-1,0,0,0,0,0,0,0);
    const __m128i edge=_mm_set1_epi16((short)(w-1));
    const __m128i maxv=_mm_set1_epi16(255);
    for (unsigned y=0;y<h;y+=8) {
        const uint8_t *labels=work->labels+(y/8)*cols;
        unsigned rows=(unsigned)min_size(8,h-y);
        const uint8_t *map=work->modes+(y/16)*tiles_x;
        uint8_t *out=output+(size_t)y*w;
        const uint8_t *above=y ? out-w : NULL;
        __m128i prev1=zero, prev2=zero, prev3=zero;
        uint64_t prior[8]={0}, before[8]={0};
        for (unsigned x=0;x<steps;x+=8) {
            uint64_t now[8]={0};
            if (x<w) {
                size_t len=min_size(8,w-x);
                Queue *q=&bank[labels[x/8]];
                for (unsigned r=0;r<rows;++r) {
                    size_t at=(size_t)(y+r)*w+x;
                    if (!tile_row(q,at,len,flat,constants,&now[r])) return 0;
                }
            }
            uint64_t wave[8]={
                now[0],
                overlap(prior[1],now[1],16),
                overlap(prior[2],now[2],32),
                overlap(prior[3],now[3],48),
                prior[4],
                overlap(before[5],prior[5],16),
                overlap(before[6],prior[6],32),
                overlap(before[7],prior[7],48),
            };
            memcpy(before,prior,sizeof(before));
            memcpy(prior,now,sizeof(prior));
            if (work->full_frame && rows==8 && x>=24 && x+8<w && !wave[0] &&
                !(wave[1]|wave[2]|wave[3]|wave[4]|wave[5]|wave[6]|wave[7]) &&
                fill_proven_wave(out,above,x,w,prev1,prev2,prev3)) continue;
            __m128i group[8];
            for (unsigned k=0;k<8;++k) group[k]=wave_bytes(wave[k]);
            transpose(group);
            if (above && rows==8 && x>=16 && x+8<w) {
                unsigned tile=x/16;
                unsigned left=map[tile-1], right=map[tile];
                if (!left && !right)
                    interior_wave(group,out,above,w,x,0,0,&prev1,&prev2,&prev3);
                else
                    interior_wave(group,out,above,w,x,left,right,&prev1,&prev2,&prev3);
                continue;
            }
            int start=(int)x;
            for (int i = 0; i < 8; ++i) {
                int t = start+i;
                int ax = t < width ? t : width-1;
                int left_tile = (t > 14 ? t-14 : 0)/16;
                int right_tile = ax/16;
                if (left_tile > right_tile) left_tile = right_tile;
                __m128i x = _mm_sub_epi16(_mm_set1_epi16((short)t),offsets);
                __m128i first = _mm_cmpeq_epi16(x,zero);
                __m128i last = _mm_cmpeq_epi16(x,edge);
                __m128i n = _mm_slli_si128(prev2,2);
                __m128i nw = _mm_slli_si128(prev3,2);
                __m128i ne = _mm_slli_si128(prev1,2);
                __m128i west = prev1;
                if (above) {
                    n = _mm_insert_epi16(n,above[ax],0);
                    nw = _mm_insert_epi16(nw,above[ax ? ax-1 : 0],0);
                    ne = _mm_insert_epi16(ne,above[ax+1 < width ? ax+1 : ax],0);
                } else {
                    if (!t) west = _mm_insert_epi16(west,half,0);
                    n = _mm_blendv_epi8(n,west,lane0);
                    nw = _mm_blendv_epi8(nw,west,lane0);
                    ne = _mm_blendv_epi8(ne,west,lane0);
                }
                west = _mm_blendv_epi8(west,n,first);
                nw = _mm_blendv_epi8(nw,n,first);
                ne = _mm_blendv_epi8(ne,n,last);
                __m128i p = predict8(map[right_tile],west,n,nw,ne,maxv);
                if (map[left_tile] != map[right_tile]) {
                    __m128i left = _mm_cmpgt_epi16(_mm_set1_epi16((short)(right_tile*16)),x);
                    p = _mm_blendv_epi8(p,predict8(map[left_tile],west,n,nw,ne,maxv),left);
                }
                __m128i e=group[i];
                group[i] = _mm_and_si128(_mm_add_epi16(p,e),maxv);
                prev3 = prev2;
                prev2 = prev1;
                prev1 = group[i];
            }
            put_byte_rows(group,out,width,(int)rows,start);
        }
    }
    return 1;
}
#endif
#ifdef QLI_NUMERIC_SSE42

static QLI_SSE42_TARGET int sse42_fill_proven_wave(uint8_t *out,const uint8_t *above,unsigned x,unsigned w,
                            __m128i prev1,__m128i prev2,__m128i prev3) {
    unsigned value=(unsigned)_mm_extract_epi16(prev1,0);
    __m128i target=_mm_set1_epi16((short)value);
    __m128i different=_mm_xor_si128(prev1,target);
    different=_mm_or_si128(different,_mm_xor_si128(prev2,target));
    different=_mm_or_si128(different,_mm_xor_si128(prev3,target));
    if (!_mm_testz_si128(different,different)) return 0;
    uint64_t repeated=UINT64_C(0x0101010101010101)*value;
    if (above) {
        uint64_t north;
        memcpy(&north,above+x-1,8);
        if (north!=repeated || above[x+7]!=value || above[x+8]!=value) return 0;
    }
    for (unsigned r=0;r<8;++r) memcpy(out+(size_t)r*w+x-2*r,&repeated,8);
    return 1;
}

static QLI_SSE42_TARGET void sse42_put_byte_rows(__m128i group[8], uint8_t *out, int width, int rows, int t) {
    sse42_transpose(group);
    for (int lane=0; lane<rows; ++lane) {
        int x=t-2*lane;
        uint8_t *row=out+(size_t)lane*width;
        __m128i bytes=_mm_packus_epi16(group[lane],group[lane]);
        if (x>=0 && x+8<=width) {
            sse42_numerical_store8((row+x),bytes);
        } else {
            int low=x<0 ? -x : 0;
            int high=x+8>width ? width-x : 8;
            if (low<high) {
                uint8_t clipped[8];
                sse42_numerical_store8(clipped,bytes);
                memcpy(row+(x+low),clipped+low,(size_t)(high-low));
            }
        }
    }
}

static QLI_SSE42_TARGET QLI_INLINE void sse42_interior_wave(__m128i group[8], uint8_t *out,
                                        const uint8_t *above, size_t width,
                                        unsigned x, unsigned left, unsigned right,
                                        __m128i *one, __m128i *two, __m128i *three) {
    __m128i prev1=*one, prev2=*two, prev3=*three;
    const __m128i offsets=_mm_setr_epi16(0,2,4,6,8,10,12,14);
    const __m128i maxv=_mm_set1_epi16(255);
    const uint8_t *north=above+x;
    #pragma clang loop unroll(full)
    for (unsigned i=0; i<8; ++i) {
        __m128i n=_mm_insert_epi16(_mm_slli_si128(prev2,2),north[i],0);
        __m128i nw=_mm_insert_epi16(_mm_slli_si128(prev3,2),north[(int)i-1],0);
        __m128i ne=_mm_insert_epi16(_mm_slli_si128(prev1,2),north[i+1],0);
        __m128i p=sse42_predict8(right,prev1,n,nw,ne,maxv);
        unsigned phase=(x&15u)+i;
        if (left!=right && phase<14) {
            __m128i mask=_mm_cmpgt_epi16(offsets,_mm_set1_epi16((short)phase));
            p=_mm_blendv_epi8(p,sse42_predict8(left,prev1,n,nw,ne,maxv),mask);
        }
        group[i]=_mm_and_si128(_mm_add_epi16(p,group[i]),maxv);
        prev3=prev2;
        prev2=prev1;
        prev1=group[i];
    }
    *one=prev1;
    *two=prev2;
    *three=prev3;
    sse42_transpose(group);
    for (size_t lane=0; lane<8; ++lane) {
        __m128i bytes=_mm_packus_epi16(group[lane],group[lane]);
        sse42_numerical_store8((out+lane*width+x-2*lane),bytes);
    }
}

static QLI_SSE42_TARGET int sse42_inverse_tiles_native(Work *work, unsigned w, unsigned h, const uint8_t *flat,
                       const uint8_t *constant, Queue bank[BANKS], uint8_t *output) {
    size_t blocks=((size_t)w*h+255)/256;
    uint8_t *constants=work->raster;
    for (size_t i=0;i<blocks;++i)
        if (flag(flat,i)) constants[i]=*constant++;
    unsigned steps=(w+21)&~7u, cols=(w+7)/8;
    int width=(int)w, half=128;
    size_t tiles_x=(w+15)/16;
    const __m128i offsets=_mm_setr_epi16(0,2,4,6,8,10,12,14);
    const __m128i zero=_mm_setzero_si128();
    const __m128i lane0=_mm_setr_epi16(-1,0,0,0,0,0,0,0);
    const __m128i edge=_mm_set1_epi16((short)(w-1));
    const __m128i maxv=_mm_set1_epi16(255);
    for (unsigned y=0;y<h;y+=8) {
        const uint8_t *labels=work->labels+(y/8)*cols;
        unsigned rows=(unsigned)min_size(8,h-y);
        const uint8_t *map=work->modes+(y/16)*tiles_x;
        uint8_t *out=output+(size_t)y*w;
        const uint8_t *above=y ? out-w : NULL;
        __m128i prev1=zero, prev2=zero, prev3=zero;
        uint64_t prior[8]={0}, before[8]={0};
        for (unsigned x=0;x<steps;x+=8) {
            uint64_t now[8]={0};
            if (x<w) {
                size_t len=min_size(8,w-x);
                Queue *q=&bank[labels[x/8]];
                for (unsigned r=0;r<rows;++r) {
                    size_t at=(size_t)(y+r)*w+x;
                    if (!tile_row(q,at,len,flat,constants,&now[r])) return 0;
                }
            }
            uint64_t wave[8]={
                now[0],
                sse42_overlap(prior[1],now[1],16),
                sse42_overlap(prior[2],now[2],32),
                sse42_overlap(prior[3],now[3],48),
                prior[4],
                sse42_overlap(before[5],prior[5],16),
                sse42_overlap(before[6],prior[6],32),
                sse42_overlap(before[7],prior[7],48),
            };
            memcpy(before,prior,sizeof(before));
            memcpy(prior,now,sizeof(prior));
            if (work->full_frame && rows==8 && x>=24 && x+8<w && !wave[0] &&
                !(wave[1]|wave[2]|wave[3]|wave[4]|wave[5]|wave[6]|wave[7]) &&
                sse42_fill_proven_wave(out,above,x,w,prev1,prev2,prev3)) continue;
            __m128i group[8];
            for (unsigned k=0;k<8;++k) group[k]=sse42_wave_bytes(wave[k]);
            sse42_transpose(group);
            if (above && rows==8 && x>=16 && x+8<w) {
                unsigned tile=x/16;
                unsigned left=map[tile-1], right=map[tile];
                if (!left && !right)
                    sse42_interior_wave(group,out,above,w,x,0,0,&prev1,&prev2,&prev3);
                else
                    sse42_interior_wave(group,out,above,w,x,left,right,&prev1,&prev2,&prev3);
                continue;
            }
            int start=(int)x;
            for (int i = 0; i < 8; ++i) {
                int t = start+i;
                int ax = t < width ? t : width-1;
                int left_tile = (t > 14 ? t-14 : 0)/16;
                int right_tile = ax/16;
                if (left_tile > right_tile) left_tile = right_tile;
                __m128i x = _mm_sub_epi16(_mm_set1_epi16((short)t),offsets);
                __m128i first = _mm_cmpeq_epi16(x,zero);
                __m128i last = _mm_cmpeq_epi16(x,edge);
                __m128i n = _mm_slli_si128(prev2,2);
                __m128i nw = _mm_slli_si128(prev3,2);
                __m128i ne = _mm_slli_si128(prev1,2);
                __m128i west = prev1;
                if (above) {
                    n = _mm_insert_epi16(n,above[ax],0);
                    nw = _mm_insert_epi16(nw,above[ax ? ax-1 : 0],0);
                    ne = _mm_insert_epi16(ne,above[ax+1 < width ? ax+1 : ax],0);
                } else {
                    if (!t) west = _mm_insert_epi16(west,half,0);
                    n = _mm_blendv_epi8(n,west,lane0);
                    nw = _mm_blendv_epi8(nw,west,lane0);
                    ne = _mm_blendv_epi8(ne,west,lane0);
                }
                west = _mm_blendv_epi8(west,n,first);
                nw = _mm_blendv_epi8(nw,n,first);
                ne = _mm_blendv_epi8(ne,n,last);
                __m128i p = sse42_predict8(map[right_tile],west,n,nw,ne,maxv);
                if (map[left_tile] != map[right_tile]) {
                    __m128i left = _mm_cmpgt_epi16(_mm_set1_epi16((short)(right_tile*16)),x);
                    p = _mm_blendv_epi8(p,sse42_predict8(map[left_tile],west,n,nw,ne,maxv),left);
                }
                __m128i e=group[i];
                group[i] = _mm_and_si128(_mm_add_epi16(p,e),maxv);
                prev3 = prev2;
                prev2 = prev1;
                prev1 = group[i];
            }
            sse42_put_byte_rows(group,out,width,(int)rows,start);
        }
    }
    return 1;
}
#endif

static int inverse_tiles(Work *work, unsigned w, unsigned h, const uint8_t *flat,
                         const uint8_t *constant, Queue bank[BANKS], uint8_t *output)
{
    const size_t blocks=((size_t)w*h+255)/256, cols=(w+7)/8, mode_cols=(w+15)/16;
    for (size_t i=0;i<blocks;++i)
        if (flag(flat,i)) work->raster[i]=*constant++;

    for (unsigned y=0;y<h;y+=8) {
        unsigned rows=(unsigned)min_size(8,h-y);
        const uint8_t *labels=work->labels+(y/8)*cols;
        for (unsigned x=0;x<w;x+=8) {
            size_t len=min_size(8,w-x);
            Queue *q=bank+(labels[x/8]);
            for (unsigned r=0;r<rows;++r) {
                size_t at=(size_t)(y+r)*w+x;
                uint64_t data=0;
                if (!tile_row(q,at,len,flat,work->raster,&data)) return 0;
                memcpy(output+at,&data,len);
            }
        }
        const uint8_t *modes=work->modes+(y/16)*mode_cols;
        for (unsigned r=0;r<rows;++r) {
            size_t at=(size_t)(y+r)*w;
            uint8_t *row=output+at;
            const uint8_t *above=at ? row-w : NULL;
            unsigned left=above ? above[0] : 128, nw=left;
            for (unsigned x=0;x<w;x+=16) {
                unsigned end=(unsigned)min_size(w,x+16);
                if (end==x+16) {
                    uint64_t e0,e1;
                    memcpy(&e0,row+x,8);
                    memcpy(&e1,row+x+8,8);
                    if (!(e0|e1) && left==nw) {
                        uint64_t a0=0,a1=0,repeated=0;
                        if (above) {
                            memcpy(&a0,above+x,8);
                            memcpy(&a1,above+x+8,8);
                            repeated=UINT64_C(0x0101010101010101)*left;
                        }
                        if (a0==repeated && a1==repeated &&
                            (!above || end==w || above[end]==left)) {
                            memset(row+x,(int)left,16);
                            continue;
                        }
                    }
                }
                unsigned mode=modes[x/16];
                for (unsigned j=x;j<end;++j) {
                    unsigned e=row[j];
                    unsigned n=above ? above[j] : left;
                    unsigned ne=above && j+1<w ? above[j+1] : n;
                    unsigned prediction=scalar_predict(mode,left,n,above ? nw : left,ne,255);
                    left=row[j]=(uint8_t)(prediction+e);
                    nw=n;
                }
            }
        }
    }
    return 1;
}

typedef struct {
    const uint8_t *flat, *constant;
    size_t blocks, constants;
} PlaneGrid;

static int read_plane_grid(Input *in, size_t n, PlaneGrid *grid)
{
    size_t blocks=(n+255)/256, flag_bytes=(blocks+7)/8;
    if ((size_t)(in->end-in->at)<20+flag_bytes || qli_read_le32(in->at)!=n ||
        qli_read_le32(in->at+4)!=blocks || qli_read_le32(in->at+12)!=11)
        return 0;
    size_t nconst=qli_read_le32(in->at+8), bytes=qli_read_le32(in->at+16);
    const uint8_t *flat=in->at+20;
    const uint8_t *constant=flat+flag_bytes;
    if (nconst>blocks || nconst>(size_t)(in->end-constant) ||
        bytes!=(size_t)(in->end-constant)-nconst ||
        (blocks%8 && flat[flag_bytes-1]>>(blocks%8)))
        return 0;
    in->at=constant+nconst;
    *grid=(PlaneGrid){flat,constant,blocks,nconst};
    return 1;
}

static int plane(const uint8_t *data, size_t size, unsigned w, unsigned h,
                 Work *work, uint8_t *output) {
    size_t n=(size_t)w*h;
    Input in={data,data+size};
    const uint8_t *modes;
    unsigned mode_bits=work->stencil ? 4u : 3u;
    size_t bits=mode_bits*work->modes_count, mode_bytes=(bits+7)/8;
    if (!record(&in,mode_bytes,work->map,&modes) ||
        (bits%8 && modes[mode_bytes-1]>>(bits%8)))
        return 0;
    unsigned fitted=0, cols=(w+15)/16;
    for (size_t k=0;k<work->modes_count;++k) {
        size_t i=mode_bits*k;
        unsigned code=modes[i/8]>>(i%8);
        if (i%8+mode_bits>8) code|=(unsigned)modes[i/8+1]<<(8-i%8);
        code&=(1u<<mode_bits)-1;
        if (code>=8) {
            unsigned tx=(unsigned)(k%cols),ty=(unsigned)(k/cols);
            if (code>9 || !tx || !ty || 16*(tx+1)+2>w) return 0;
            fitted=1;
        }
        work->modes[k]=(uint8_t)code;
    }
    if (work->stencil) {
        if (!fitted || (size_t)(in.end-in.at)<20) return 0;
        memcpy(work->stencil_fit,in.at,20);
        in.at+=20;
        for (unsigned k=0;k<2;++k) {
            int sum=0;
            for (unsigned j=0;j<10;++j) {
                int weight=work->stencil_fit[10*k+j];
                if (weight < -64 || weight > 64) return 0;
                sum+=weight;
            }
            if (sum!=32) return 0;
        }
    }
    PlaneGrid grid;
    if (!read_plane_grid(&in,n,&grid)) return 0;
    const uint8_t *flat=grid.flat, *constant=grid.constant;
    const size_t blocks=grid.blocks;
    size_t actual=0,literals=0;
    for (size_t k=0;k<blocks;++k) {
        if (flag(flat,k)) ++actual;
        else literals+=min_size(256,n-256*k);
    }
    if (actual!=grid.constants) return 0;
    if (work->color_plane)
        return diagonal_payload(&in,work,flat,constant,w,h,literals,output);
    size_t groups=live_tiles(work->labels,flat,w,h);
    const uint8_t *map;
    if (!record(&in,(groups+1)/2,work->map,&map) ||
        (groups%2 && map[groups/2]>>4))
        return 0;
    Queue bank[BANKS]={0};
    if (!queue_records(&in,work->literals,literals,bank)) return 0;
    for (size_t i=0,k=0;i<work->labels_cap;++i) {
        if (work->labels[i]) {
            work->labels[i]=(map[k/2]>>(4*(k%2)))&15u;
            ++k;
        }
    }
#ifdef QLI_NUMERIC_SSE42
    if (work->vector) {
        if (!QLI_NUMERIC_FN(work->vector, inverse_tiles_native)(work,w,h,flat,constant,bank,output)) return 0;
    } else
#endif
    if (!inverse_tiles(work,w,h,flat,constant,bank,output)) return 0;
    for (unsigned i=0;i<BANKS;++i)
        if (bank[i].data!=bank[i].end) return 0;
    return 1;
}

#ifdef QLI_NUMERIC_AVX2

typedef struct { __m128i lanes[4]; } BandCRC;

static QLI_BAND_CRC_TARGET QLI_INLINE void
band_color(const uint8_t *planes,const uint8_t *alpha,uint8_t fixed_alpha,size_t n,
           uint8_t *out,unsigned basis,BandCRC *crc)
{
    const __m128i a_fixed=_mm_set1_epi8((char)fixed_alpha);
    const __m128i step=_mm_set_epi64x(QI_K448,QI_K512);
    __m128i c0=crc->lanes[0],c1=crc->lanes[1],c2=crc->lanes[2],c3=crc->lanes[3];
    size_t i=0;
    for (;n-i>=16;i+=16) {
        __m128i a=a_fixed;
        if (alpha) memcpy(&a,alpha+i,sizeof(a));
        NumericPixels pixels=numeric_pixels16(planes+i,planes+n+i,planes+2*n+i,a,basis);
        store_numeric_pixels16(out+4*i,pixels);
        c0=_mm_xor_si128(crc_native_fold(c0,step),pixels.p0);
        c1=_mm_xor_si128(crc_native_fold(c1,step),pixels.p1);
        c2=_mm_xor_si128(crc_native_fold(c2,step),pixels.p2);
        c3=_mm_xor_si128(crc_native_fold(c3,step),pixels.p3);
    }
    crc->lanes[0]=c0; crc->lanes[1]=c1; crc->lanes[2]=c2; crc->lanes[3]=c3;
    numeric_color_tail(planes,alpha,fixed_alpha,n,i,out,basis);
}

static QLI_BAND_CRC_TARGET void
band_interleave(const uint8_t *planes,const uint8_t *alpha,uint8_t fixed_alpha,size_t n,
                uint8_t *out,unsigned basis,BandCRC *crc)
{
    switch (basis) {
    case 1: band_color(planes,alpha,fixed_alpha,n,out,1,crc); break;
    case 2: band_color(planes,alpha,fixed_alpha,n,out,2,crc); break;
    default: band_color(planes,alpha,fixed_alpha,n,out,0,crc); break;
    }
}
static QLI_BAND_CRC_TARGET void
band_interleave_rgb(const uint8_t *planes,size_t n,uint8_t *out,unsigned basis,BandCRC *crc)
{
    const __m128i step=_mm_set_epi64x(QI_K448,QI_K512);
    __m128i a=crc->lanes[0],b=crc->lanes[1],c=crc->lanes[2],d=crc->lanes[3];
    size_t i=0;
    for (;n-i>=64;i+=64) {
        for (unsigned j=0;j<4;++j) {
            NumericRgb packed=numeric_rgb16(planes,n,i+16*j,basis);
            uint8_t *target=out+3*i+48*j;
            numerical_store16(target,packed.a);
            numerical_store16(target+16,packed.b);
            numerical_store16(target+32,packed.c);
            switch (j) {
            case 0:
                a=_mm_xor_si128(crc_native_fold(a,step),packed.a);
                b=_mm_xor_si128(crc_native_fold(b,step),packed.b);
                c=_mm_xor_si128(crc_native_fold(c,step),packed.c);
                break;
            case 1:
                d=_mm_xor_si128(crc_native_fold(d,step),packed.a);
                a=_mm_xor_si128(crc_native_fold(a,step),packed.b);
                b=_mm_xor_si128(crc_native_fold(b,step),packed.c);
                break;
            case 2:
                c=_mm_xor_si128(crc_native_fold(c,step),packed.a);
                d=_mm_xor_si128(crc_native_fold(d,step),packed.b);
                a=_mm_xor_si128(crc_native_fold(a,step),packed.c);
                break;
            default:
                b=_mm_xor_si128(crc_native_fold(b,step),packed.a);
                c=_mm_xor_si128(crc_native_fold(c,step),packed.b);
                d=_mm_xor_si128(crc_native_fold(d,step),packed.c);
                break;
            }
        }
    }
    crc->lanes[0]=a; crc->lanes[1]=b; crc->lanes[2]=c; crc->lanes[3]=d;
    numeric_rgb_tail(planes,n,i,out,basis);
}

static QLI_NUMERIC_TARGET int banded_order(const uint8_t *file,size_t bytes,size_t cap,
                        unsigned w,unsigned h,unsigned basis,unsigned caps,int one_plane,int directional,
                        uint8_t **output,size_t *output_size,size_t *peak)
{
    const size_t n=(size_t)w*h, end=bytes-4;
    const unsigned channels=file[13];
    const uint8_t *data[4];
    size_t lengths[4],at=64;
    for (unsigned c=0;c<channels;++c) {
        size_t len=qli_read_le32(file+36+8*c);
        if (len>end-at) return 0;
        data[c]=file+at;
        lengths[c]=len;
        at+=len;
    }
    if (at!=end) return 0;
    const unsigned coded_planes=one_plane ? 1u : 3u;
    const unsigned fitted=file[15]&7u;
    if (one_plane && (lengths[1]!=1 || lengths[2]!=1 || (fitted&6u))) return 0;
    unsigned alpha_kind=qli_read_le32(file+56);
    if (channels==4 && ((alpha_kind==0 && lengths[3]!=n) ||
        (alpha_kind==2 && lengths[3]!=1) ||
        (alpha_kind!=0 && alpha_kind!=2 && alpha_kind!=4))) return 0;

    const size_t modes=(size_t)((w+15)/16)*((h+15)/16);
    const size_t labels=alpha_kind==4 ? (size_t)((w+7)/8)*((h+7)/8) : 0;
    const size_t map_size=labels ? (labels+1)/2 : ((fitted ? 4u : 3u)*modes+7)/8;
    const size_t raster=(n+255)/256;
    const size_t band=16*(size_t)w;
    const size_t plane_info=raster+modes;
    const size_t info_bytes=coded_planes*plane_info;
    const size_t north_bytes=2*(size_t)(w+3);
    const size_t activity_bytes=coded_planes*north_bytes;
    const int fused_crc=channels==4 && (caps & QLI_CPU_PCLMUL_CRC);
    const int fused_rgb_crc=channels==3 && (caps & QLI_CPU_PCLMUL_CRC) && w%4==0;
    const size_t carry=(fitted ? 2u : 1u)*(size_t)w;
    const size_t color_bytes=(channels==3 || fused_crc ? 3 : 4)*band;
    const size_t scratch=2*band+map_size+info_bytes+color_bytes+coded_planes*carry;
    size_t tracked=scratch+channels*n+activity_bytes;
    if (tracked>cap) return -1;

    uint8_t *memory=qli_malloc(scratch),*rgba=NULL;
    uint32_t *tables=NULL;
    int ok=0;
    if (!memory) return 0;

    uint8_t *literals=memory,*map=literals+2*band;
    uint8_t *info=map+map_size,*colors=info+info_bytes,*last=colors+color_bytes;
    const uint8_t *alpha=channels==4 && alpha_kind==0 ? data[3] : NULL;
    const uint8_t fixed_alpha=alpha_kind==2 ? data[3][0] : 0;
    Work work[3]={{0}};
    BandPlane state[3];
    for (unsigned c=0;c<coded_planes;++c) {
        Work *v=work+c;
        v->raster=info+c*plane_info;
        v->literals=literals;
        v->map=map;
        v->modes=v->raster+raster;
        v->modes_count=modes;
    }
    size_t models=0;
    for (unsigned c=0;c<coded_planes;++c) {
        Work *v=work+c;
        v->color_plane=1;
        v->directional=directional;
        v->guides=c;
        v->earlier[0]=literals;
        v->earlier[1]=literals+band;
        v->error=c+1<coded_planes ? literals+c*band : NULL;
        v->stencil=(fitted>>c)&1u;
        v->vector=QLI_TIER_AVX2;
        v->defer=state+c;
        if (!plane(data[c],lengths[c],w,h,v,NULL)) goto done;
        models+=state[c].book.count;
    }

    const size_t tables_bytes=models*CAUSAL_SCALE*sizeof(*tables);
    if (tables_bytes>cap-tracked) {
        ok=-1;
        goto done;
    }
    tracked+=tables_bytes;
    tables=qli_malloc(tables_bytes+activity_bytes);
    if (!tables) goto done;
    uint8_t *north=(uint8_t *)tables+tables_bytes;
    memset(north,0,activity_bytes);
    uint32_t *slots=tables;
    for (unsigned c=0;c<coded_planes;++c) {
        work[c].tables=slots;
        work[c].causal_aux=north+c*north_bytes;
        if (!build_causal_tables(&state[c].book,slots)) goto done;
        slots+=(size_t)state[c].book.count*CAUSAL_SCALE;
    }
    rgba=qli_malloc(channels*n);
    if (!rgba) goto done;
    if (labels) {
        uint8_t *alpha_plane=rgba+3*n;
        Work alpha_work={
            .raster=rgba+n, .literals=rgba, .map=map,
            .modes=rgba+n+raster, .labels=rgba+n+raster+modes,
            .modes_count=modes, .labels_cap=labels, .vector=QLI_TIER_AVX2
        };
        if (!plane(data[3],lengths[3],w,h,&alpha_work,alpha_plane)) goto done;
        alpha=alpha_plane;
    }

    __m128i coefficient[11];
    fused_coefficients(NULL,coefficient);
    __m128i stencil[3][11];
    for (unsigned c=0;c<coded_planes;++c)
        if (work[c].stencil) fused_coefficients(work[c].stencil_fit,stencil[c]);
    BandCRC crc={{_mm_set_epi64x(0,QI_SEED),_mm_setzero_si128(),
                  _mm_setzero_si128(),_mm_setzero_si128()}};
    for (unsigned base=0;base<h;base+=16) {
        size_t count=min_size(16,h-base)*w;
        ChromaBand wave[3];
        for (unsigned c=0;c<coded_planes;++c) {
            const uint8_t *above=base ? last+(c+1)*carry-w : NULL;
            chroma_begin(wave+c,state+c,colors+c*count,above,base);
            wave[c].stencil=work[c].stencil ? stencil[c] : NULL;
        }
        if (one_plane) {
            if (!chroma_whole(wave,coefficient)) goto done;
            if (base==0 || count!=band) {
                memset(colors+count,data[1][0],count);
                memset(colors+2*count,data[2][0],count);
            }
        } else if (!chroma_pipeline(wave,wave+1,wave+2,coefficient)) goto done;
        if (base+16<h)
            for (unsigned c=0;c<coded_planes;++c) memcpy(last+c*carry,colors+(c+1)*count-carry,carry);
        size_t pos=(size_t)base*w;
        if (channels==3) {
            if (fused_rgb_crc) band_interleave_rgb(colors,count,rgba+3*pos,basis,&crc);
            else color_interleave_rgb(colors,count,rgba+3*pos,basis,QLI_TIER_AVX2);
            continue;
        }
        if (fused_crc) {
            band_interleave(colors,alpha ? alpha+pos : NULL,fixed_alpha,count,rgba+4*pos,basis,&crc);
            continue;
        }
        if (alpha) memcpy(colors+3*count,alpha+pos,count);
        else memset(colors+3*count,fixed_alpha,count);
        color_interleave(colors,count,rgba+4*pos,basis);
    }
    for (unsigned c=0;c<coded_planes;++c) {
        BandPlane *p=state+c;
        if (p->input.at!=p->input.end) goto done;
        if (!fused_all_states(p->ans.lo,CAUSAL_LOW) || !fused_all_states(p->ans.hi,CAUSAL_LOW)) goto done;
    }
    uint32_t actual;
    if (fused_crc || fused_rgb_crc) {
        const size_t tail=fused_rgb_crc ? 3*(n&63) : (4*n)&63;
        actual=crc_native_finish(crc.lanes[0],crc.lanes[1],crc.lanes[2],crc.lanes[3],
                                 rgba+channels*n-tail,tail);
    } else actual=qint_crc32(rgba,channels*n);
    if (actual!=qli_read_le32(file+24)) goto done;
    *peak=tracked;
    *output=rgba;
    *output_size=channels*n;
    rgba=NULL;
    ok=1;
done:
    qli_free(tables);
    qli_free(rgba);
    qli_free(memory);
    return ok;
}

#endif
#ifdef QLI_NUMERIC_SSE42

typedef struct { __m128i lanes[4]; } sse42_BandCRC;

static QLI_SSE42_CRC_TARGET QLI_INLINE void
sse42_band_color(const uint8_t *planes,const uint8_t *alpha,uint8_t fixed_alpha,size_t n,
           uint8_t *out,unsigned basis,sse42_BandCRC *crc)
{
    const __m128i a_fixed=_mm_set1_epi8((char)fixed_alpha);
    const __m128i step=_mm_set_epi64x(QI_K448,QI_K512);
    __m128i c0=crc->lanes[0],c1=crc->lanes[1],c2=crc->lanes[2],c3=crc->lanes[3];
    size_t i=0;
    for (;n-i>=16;i+=16) {
        __m128i a=a_fixed;
        if (alpha) memcpy(&a,alpha+i,sizeof(a));
        NumericPixels pixels=numeric_pixels16(planes+i,planes+n+i,planes+2*n+i,a,basis);
        store_numeric_pixels16(out+4*i,pixels);
        c0=_mm_xor_si128(crc_native_fold(c0,step),pixels.p0);
        c1=_mm_xor_si128(crc_native_fold(c1,step),pixels.p1);
        c2=_mm_xor_si128(crc_native_fold(c2,step),pixels.p2);
        c3=_mm_xor_si128(crc_native_fold(c3,step),pixels.p3);
    }
    crc->lanes[0]=c0; crc->lanes[1]=c1; crc->lanes[2]=c2; crc->lanes[3]=c3;
    numeric_color_tail(planes,alpha,fixed_alpha,n,i,out,basis);
}

static QLI_SSE42_CRC_TARGET void
sse42_band_interleave(const uint8_t *planes,const uint8_t *alpha,uint8_t fixed_alpha,size_t n,
                uint8_t *out,unsigned basis,sse42_BandCRC *crc)
{
    switch (basis) {
    case 1: sse42_band_color(planes,alpha,fixed_alpha,n,out,1,crc); break;
    case 2: sse42_band_color(planes,alpha,fixed_alpha,n,out,2,crc); break;
    default: sse42_band_color(planes,alpha,fixed_alpha,n,out,0,crc); break;
    }
}
static QLI_SSE42_CRC_TARGET void
sse42_band_interleave_rgb(const uint8_t *planes,size_t n,uint8_t *out,unsigned basis,sse42_BandCRC *crc)
{
    const __m128i step=_mm_set_epi64x(QI_K448,QI_K512);
    __m128i a=crc->lanes[0],b=crc->lanes[1],c=crc->lanes[2],d=crc->lanes[3];
    size_t i=0;
    for (;n-i>=64;i+=64) {
        for (unsigned j=0;j<4;++j) {
            sse42_NumericRgb packed=sse42_numeric_rgb16(planes,n,i+16*j,basis);
            uint8_t *target=out+3*i+48*j;
            sse42_numerical_store16(target,packed.a);
            sse42_numerical_store16(target+16,packed.b);
            sse42_numerical_store16(target+32,packed.c);
            switch (j) {
            case 0:
                a=_mm_xor_si128(crc_native_fold(a,step),packed.a);
                b=_mm_xor_si128(crc_native_fold(b,step),packed.b);
                c=_mm_xor_si128(crc_native_fold(c,step),packed.c);
                break;
            case 1:
                d=_mm_xor_si128(crc_native_fold(d,step),packed.a);
                a=_mm_xor_si128(crc_native_fold(a,step),packed.b);
                b=_mm_xor_si128(crc_native_fold(b,step),packed.c);
                break;
            case 2:
                c=_mm_xor_si128(crc_native_fold(c,step),packed.a);
                d=_mm_xor_si128(crc_native_fold(d,step),packed.b);
                a=_mm_xor_si128(crc_native_fold(a,step),packed.c);
                break;
            default:
                b=_mm_xor_si128(crc_native_fold(b,step),packed.a);
                c=_mm_xor_si128(crc_native_fold(c,step),packed.b);
                d=_mm_xor_si128(crc_native_fold(d,step),packed.c);
                break;
            }
        }
    }
    crc->lanes[0]=a; crc->lanes[1]=b; crc->lanes[2]=c; crc->lanes[3]=d;
    numeric_rgb_tail(planes,n,i,out,basis);
}

static QLI_SSE42_TARGET int sse42_banded_order(const uint8_t *file,size_t bytes,size_t cap,
                        unsigned w,unsigned h,unsigned basis,unsigned caps,int one_plane,int directional,
                        uint8_t **output,size_t *output_size,size_t *peak)
{
    const size_t n=(size_t)w*h, end=bytes-4;
    const unsigned channels=file[13];
    const uint8_t *data[4];
    size_t lengths[4],at=64;
    for (unsigned c=0;c<channels;++c) {
        size_t len=qli_read_le32(file+36+8*c);
        if (len>end-at) return 0;
        data[c]=file+at;
        lengths[c]=len;
        at+=len;
    }
    if (at!=end) return 0;
    const unsigned coded_planes=one_plane ? 1u : 3u;
    const unsigned fitted=file[15]&7u;
    if (one_plane && (lengths[1]!=1 || lengths[2]!=1 || (fitted&6u))) return 0;
    unsigned alpha_kind=qli_read_le32(file+56);
    if (channels==4 && ((alpha_kind==0 && lengths[3]!=n) ||
        (alpha_kind==2 && lengths[3]!=1) ||
        (alpha_kind!=0 && alpha_kind!=2 && alpha_kind!=4))) return 0;

    const size_t modes=(size_t)((w+15)/16)*((h+15)/16);
    const size_t labels=alpha_kind==4 ? (size_t)((w+7)/8)*((h+7)/8) : 0;
    const size_t map_size=labels ? (labels+1)/2 : ((fitted ? 4u : 3u)*modes+7)/8;
    const size_t raster=(n+255)/256;
    const size_t band=16*(size_t)w;
    const size_t plane_info=raster+modes;
    const size_t info_bytes=coded_planes*plane_info;
    const size_t north_bytes=2*(size_t)(w+3);
    const size_t activity_bytes=coded_planes*north_bytes;
    const int fused_crc=channels==4 && (caps & QLI_CPU_PCLMUL_CRC);
    const int fused_rgb_crc=channels==3 && (caps & QLI_CPU_PCLMUL_CRC) && w%4==0;
    const size_t carry=(fitted ? 2u : 1u)*(size_t)w;
    const size_t color_bytes=(channels==3 || fused_crc ? 3 : 4)*band;
    const size_t scratch=2*band+map_size+info_bytes+color_bytes+coded_planes*carry;
    size_t tracked=scratch+channels*n+activity_bytes;
    if (tracked>cap) return -1;

    uint8_t *memory=qli_malloc(scratch),*rgba=NULL;
    uint32_t *tables=NULL;
    int ok=0;
    if (!memory) return 0;

    uint8_t *literals=memory,*map=literals+2*band;
    uint8_t *info=map+map_size,*colors=info+info_bytes,*last=colors+color_bytes;
    const uint8_t *alpha=channels==4 && alpha_kind==0 ? data[3] : NULL;
    const uint8_t fixed_alpha=alpha_kind==2 ? data[3][0] : 0;
    Work work[3]={{0}};
    sse42_BandPlane state[3];
    for (unsigned c=0;c<coded_planes;++c) {
        Work *v=work+c;
        v->raster=info+c*plane_info;
        v->literals=literals;
        v->map=map;
        v->modes=v->raster+raster;
        v->modes_count=modes;
    }
    size_t models=0;
    for (unsigned c=0;c<coded_planes;++c) {
        Work *v=work+c;
        v->color_plane=1;
        v->directional=directional;
        v->guides=c;
        v->earlier[0]=literals;
        v->earlier[1]=literals+band;
        v->error=c+1<coded_planes ? literals+c*band : NULL;
        v->stencil=(fitted>>c)&1u;
        v->vector=QLI_TIER_SSE42;
        v->defer=state+c;
        if (!plane(data[c],lengths[c],w,h,v,NULL)) goto done;
        models+=state[c].book.count;
    }

    const size_t tables_bytes=models*CAUSAL_SCALE*sizeof(*tables);
    if (tables_bytes>cap-tracked) {
        ok=-1;
        goto done;
    }
    tracked+=tables_bytes;
    tables=qli_malloc(tables_bytes+activity_bytes);
    if (!tables) goto done;
    uint8_t *north=(uint8_t *)tables+tables_bytes;
    memset(north,0,activity_bytes);
    uint32_t *slots=tables;
    for (unsigned c=0;c<coded_planes;++c) {
        work[c].tables=slots;
        work[c].causal_aux=north+c*north_bytes;
        if (!build_causal_tables(&state[c].book,slots)) goto done;
        slots+=(size_t)state[c].book.count*CAUSAL_SCALE;
    }
    rgba=qli_malloc(channels*n);
    if (!rgba) goto done;
    if (labels) {
        uint8_t *alpha_plane=rgba+3*n;
        Work alpha_work={
            .raster=rgba+n, .literals=rgba, .map=map,
            .modes=rgba+n+raster, .labels=rgba+n+raster+modes,
            .modes_count=modes, .labels_cap=labels, .vector=QLI_TIER_SSE42
        };
        if (!plane(data[3],lengths[3],w,h,&alpha_work,alpha_plane)) goto done;
        alpha=alpha_plane;
    }

    __m128i coefficient[11];
    sse42_fused_coefficients(NULL,coefficient);
    __m128i stencil[3][11];
    for (unsigned c=0;c<coded_planes;++c)
        if (work[c].stencil) sse42_fused_coefficients(work[c].stencil_fit,stencil[c]);
    sse42_BandCRC crc={{_mm_set_epi64x(0,QI_SEED),_mm_setzero_si128(),
                  _mm_setzero_si128(),_mm_setzero_si128()}};
    for (unsigned base=0;base<h;base+=16) {
        size_t count=min_size(16,h-base)*w;
        sse42_ChromaBand wave[3];
        for (unsigned c=0;c<coded_planes;++c) {
            const uint8_t *above=base ? last+(c+1)*carry-w : NULL;
            sse42_chroma_begin(wave+c,state+c,colors+c*count,above,base);
            wave[c].stencil=work[c].stencil ? stencil[c] : NULL;
        }
        if (one_plane) {
            if (!sse42_chroma_whole(wave,coefficient)) goto done;
            if (base==0 || count!=band) {
                memset(colors+count,data[1][0],count);
                memset(colors+2*count,data[2][0],count);
            }
        } else if (!sse42_chroma_pipeline(wave,wave+1,wave+2,coefficient)) goto done;
        if (base+16<h)
            for (unsigned c=0;c<coded_planes;++c) memcpy(last+c*carry,colors+(c+1)*count-carry,carry);
        size_t pos=(size_t)base*w;
        if (channels==3) {
            if (fused_rgb_crc) sse42_band_interleave_rgb(colors,count,rgba+3*pos,basis,&crc);
            else color_interleave_rgb(colors,count,rgba+3*pos,basis,QLI_TIER_SSE42);
            continue;
        }
        if (fused_crc) {
            sse42_band_interleave(colors,alpha ? alpha+pos : NULL,fixed_alpha,count,rgba+4*pos,basis,&crc);
            continue;
        }
        if (alpha) memcpy(colors+3*count,alpha+pos,count);
        else memset(colors+3*count,fixed_alpha,count);
        color_interleave(colors,count,rgba+4*pos,basis);
    }
    for (unsigned c=0;c<coded_planes;++c) {
        sse42_BandPlane *p=state+c;
        if (p->input.at!=p->input.end) goto done;
        if (!sse42_fused_all_states(p->ans.lo,CAUSAL_LOW) || !sse42_fused_all_states(p->ans.hi,CAUSAL_LOW)) goto done;
    }
    uint32_t actual;
    if (fused_crc || fused_rgb_crc) {
        const size_t tail=fused_rgb_crc ? 3*(n&63) : (4*n)&63;
        actual=crc_native_finish(crc.lanes[0],crc.lanes[1],crc.lanes[2],crc.lanes[3],
                                 rgba+channels*n-tail,tail);
    } else actual=qint_crc32(rgba,channels*n);
    if (actual!=qli_read_le32(file+24)) goto done;
    *peak=tracked;
    *output=rgba;
    *output_size=channels*n;
    rgba=NULL;
    ok=1;
done:
    qli_free(tables);
    qli_free(rgba);
    qli_free(memory);
    return ok;
}

#endif

static unsigned full_table_models(const uint8_t *file, size_t bytes, size_t n,
                                  size_t modes_count, unsigned fitted, unsigned color_kind)
{
    const size_t end=bytes-4;
    size_t at=64;
    unsigned guides=0, models=0;
    for (unsigned c=0;c<3;++c) {
        const unsigned kind=qli_read_le32(file+32+8*c);
        const size_t len=qli_read_le32(file+36+8*c);
        if (len>end-at) return CAUSAL_MODELS;
        Input in={file+at,file+at+len};
        at+=len;
        if (kind==0 || kind==2) continue;
        if (kind!=color_kind) return CAUSAL_MODELS;
        const int stencil=(fitted>>c)&1u;
        const size_t mode_bytes=((stencil ? 4u : 3u)*modes_count+7)/8;
        PlaneRecord packet;
        PlaneGrid grid;
        CausalModels book;
        if (!read_plane_record(&in,mode_bytes,&packet)) return CAUSAL_MODELS;
        if (stencil) {
            if ((size_t)(in.end-in.at)<20) return CAUSAL_MODELS;
            in.at+=20;
        }
        const unsigned route_size=(guides ? 32 : 1)*CAUSAL_CONTEXTS*(color_kind==38 ? 3 : 1);
        if (!read_plane_grid(&in,n,&grid) ||
            (guides && !read_plane_record(&in,guides==2 ? 49 : 7,&packet)) ||
            !read_plane_record(&in,route_size,&packet) ||
            !read_causal_models(&in,&book)) return CAUSAL_MODELS;
        if (models<book.count) models=book.count;
        ++guides;
    }
    return models;
}

int spatial_banks_decode(const uint8_t *file, size_t bytes, size_t cap,
                        uint8_t **output, size_t *output_size, size_t *peak) {
#ifdef QLI_NUMERIC_SSE42
    const unsigned caps=qli_cpu_capabilities();
    const int vector=qli_numeric_tier(caps);
#else
    const int vector=QLI_TIER_SCALAR;
#endif
    *output=NULL;
    *output_size=0;
    *peak=0;
    if (!file || bytes<68 || bytes>NUMERICAL_FILE_CAP || qli_read_le32(file+28)!=64) return 0;
    const int directional=!memcmp(file,"QFL1",4);
    const unsigned color_kind=directional ? 38u : 20u;
    const unsigned fitted=file[15];
    if (!directional && memcmp(file,"QAX1",4)) return 0;
    if (fitted>7 || (!directional && fitted)) return 0;
    const unsigned channels=file[13]==3 && !qli_read_le64(file+56) ? 3 : 4;
    if (file[12]!=8 || file[13]!=channels || file[14]>(channels==3 ? 6 : 2)) return 0;
    unsigned basis=file[14];
    unsigned w=qli_read_le32(file+4),h=qli_read_le32(file+8);
    if (!w || !h || w>16384 || h>16384 || (size_t)w*h>IMG_CAP) return 0;
    size_t n=(size_t)w*h;
    if (qli_read_le64(file+16)!=n || qint_crc32(file,bytes-4)!=qli_read_le32(file+bytes-4))
        return 0;
    Work work={0};
    work.modes_count=(size_t)((w+15)/16)*((h+15)/16);
    work.labels_cap=(size_t)((w+7)/8)*((h+7)/8);
    const size_t maps=(work.labels_cap+1)/2;
    size_t raster=(n+255)/256;
    size_t scratch=4*n+raster+maps+work.modes_count+work.labels_cap;
    const size_t extra=2*(size_t)(w+3);
    const size_t base=scratch+4*n+extra;
#ifdef QLI_NUMERIC_SSE42
    const unsigned p0=qli_read_le32(file+32),p1=qli_read_le32(file+40),p2=qli_read_le32(file+48);
    const int one_plane=p1==2 && p2==2;
    if (vector && h>=64 && n>=262144 &&
        p0==color_kind && (one_plane || (p1==color_kind && p2==color_kind))) {
        size_t band_cap=min_size(base+CAUSAL_MODELS*CAUSAL_SCALE*sizeof(uint32_t)+n,cap);
        int result=QLI_NUMERIC_FN(vector, banded_order)(file,bytes,band_cap,w,h,basis,caps,one_plane,directional,
                                output,output_size,peak);
        if (result >= 0) return result;
    }
#endif
    work.table_models=full_table_models(file,bytes,n,work.modes_count,fitted,color_kind);
    const size_t table_bytes=(size_t)work.table_models*CAUSAL_SCALE*sizeof(*work.tables);
    const size_t tracked=base+table_bytes;
    if (tracked>cap) return 0;
    uint8_t *memory=qli_malloc(scratch);
    uint8_t *rgba=qli_malloc(4*n);
    int ok=0;
    work.tables=qli_malloc(table_bytes+extra);
    if (!memory || !rgba || !work.tables) goto done;
    work.causal_aux=(uint8_t *)work.tables+table_bytes;
    uint8_t *planes=memory;
    work.raster=planes+4*n;
    work.literals=rgba+2*n;
    work.map=work.raster+raster;
    work.modes=work.map+maps;
    work.labels=work.modes+work.modes_count;
    work.vector=vector;
    work.directional=directional;
    work.full_frame=1;
    size_t at=64,end=bytes-4;
    for (unsigned c=0;c<channels;++c) {
        unsigned kind=qli_read_le32(file+32+8*c);
        size_t len=qli_read_le32(file+36+8*c);
        if (len>end-at) goto done;
        work.color_plane=c<3 && kind==color_kind;
        work.stencil=(fitted>>c)&1u;
        if (work.stencil && !work.color_plane) goto done;
        uint8_t *out=planes+c*n;
        if (kind==0 && len==n) memcpy(out,file+at,n);
        else if (kind==2 && len==1) memset(out,file[at],n);
        else {
            if (!work.color_plane && (c!=3 || kind!=4)) goto done;
            work.error=c<2 ? rgba+c*n : NULL;
            if (!plane(file+at,len,w,h,&work,out)) goto done;
            if (work.error && c<2) work.earlier[work.guides++]=work.error;
        }
        at+=len;
    }
    if (at!=end) goto done;
    if (channels==3) color_interleave_rgb(planes,n,rgba,basis,vector);
    else color_interleave(planes,n,rgba,basis);
    if (qint_crc32(rgba,channels*n)!=qli_read_le32(file+24)) goto done;
    *peak=tracked;
    *output=rgba;
    *output_size=channels*n;
    rgba=NULL;
    ok=1;
done:
    qli_free(work.tables);
    qli_free(memory);
    qli_free(rgba);
    return ok;
}

#undef QLI_BAND_CRC_TARGET
#undef QLI_SSE42_CRC_TARGET

int qli_pack_below(const uint8_t *, size_t, unsigned stride, size_t limit,
                   uint8_t **, size_t *, size_t *measured, unsigned *kind);
int pack_decode(const uint8_t *, size_t, size_t max_output, uint8_t **, size_t *);

enum { PACK_HEADER = 32, PACK_FOOTER = 4, MAX_BYTES = QLI_BYTE_INPUT_CAP,
       RAW = 0, HUFFMAN = 1, MATCH = 2 };

static void pack_header(uint8_t *frame, const uint8_t *input, size_t size, unsigned kind,
                        unsigned stride, size_t control, size_t cw, size_t literals, size_t lw)
{
    memcpy(frame, "QSB1", 4);
    frame[4] = (uint8_t)kind;
    qli_write_le16(frame + 6, (uint16_t)stride);
    qli_write_le32(frame + 8, (uint32_t)size);
    qli_write_le32(frame + 12, (uint32_t)control);
    qli_write_le32(frame + 16, (uint32_t)cw);
    qli_write_le32(frame + 20, (uint32_t)literals);
    qli_write_le32(frame + 24, (uint32_t)lw);
    qli_write_le32(frame + 28, qint_crc32(input, size));
}

int qli_pack_below(const uint8_t *input, size_t size, unsigned stride, size_t limit,
                   uint8_t **out, size_t *bytes, size_t *measured, unsigned *kind)
{
    QliBytePlan direct, controls, values;
    if (stride > UINT16_MAX || !qli_byte_plan(input, size, &direct)) return -1;
    unsigned mode = direct.bytes < size ? HUFFMAN : RAW;
    size_t chosen = mode == HUFFMAN ? direct.bytes : size;
    uint8_t *control = NULL, *literal = NULL, *frame = NULL;
    size_t nc = 0, nl = 0;
    int result = -1;
    if (size) {
        size_t bound;
        if (!qlic_lite_match_bound(size, &bound)) goto done;
        control = qli_malloc(bound);
        literal = qli_malloc(size);
        if (!control || !literal ||
            !match_encode_split(input, size, stride, control, bound, &nc, literal, size, &nl) ||
            !qli_byte_plan(control, nc, &controls) || !qli_byte_plan(literal, nl, &values))
            goto done;
        if (controls.bytes + values.bytes < chosen) {
            mode = MATCH;
            chosen = controls.bytes + values.bytes;
        }
    }
    size_t total = PACK_HEADER + chosen + PACK_FOOTER;
    if (measured) *measured = total;
    if (kind) *kind = mode;
    if (total >= limit) {
        result = 0;
        goto done;
    }
    frame = qli_malloc(total);
    if (!frame) goto done;
    frame[5] = 0;
    uint8_t *data = frame + PACK_HEADER;
    if (mode == MATCH) {
        pack_header(frame, input, size, mode, stride, nc, controls.bytes, nl, values.bytes);
        if (!qli_byte_emit(&controls, control, nc, data) ||
            !qli_byte_emit(&values, literal, nl, data + controls.bytes)) goto done;
    } else {
        pack_header(frame, input, size, mode, 0, mode == HUFFMAN ? size : 0, chosen, 0, 0);
        if (mode == HUFFMAN) {
            if (!qli_byte_emit(&direct, input, size, data)) goto done;
        } else if (size) {
            memcpy(data, input, size);
        }
    }
    qli_write_le32(frame + total - PACK_FOOTER, qint_crc32(frame, total - PACK_FOOTER));
    *out = frame;
    *bytes = total;
    frame = NULL;
    result = 1;
done:
    qli_free(frame);
    qli_free(control);
    qli_free(literal);
    return result;
}

int pack_decode(const uint8_t *input, size_t size, size_t max_output,
                uint8_t **output, size_t *output_size) {
    if (!output || !output_size || *output || *output_size || !input || size < PACK_HEADER + PACK_FOOTER ||
        size > MAX_BYTES + PACK_HEADER + PACK_FOOTER || memcmp(input, "QSB1", 4) || input[4] > MATCH || input[5]) return 0;
    unsigned kind = input[4], stride = input[6] | (unsigned)input[7] << 8;
    size_t count = qli_read_le32(input + 8), cn = qli_read_le32(input + 12), cw = qli_read_le32(input + 16);
    size_t ln = qli_read_le32(input + 20), lw = qli_read_le32(input + 24), bound = 0;
    if (count > MAX_BYTES || count > max_output ||
        cw > size - PACK_HEADER - PACK_FOOTER || lw != size - PACK_HEADER - PACK_FOOTER - cw ||
        qli_read_le32(input + size - PACK_FOOTER) != qint_crc32(input, size - PACK_FOOTER)) return 0;
    if (kind == RAW) {
        if (stride || cn || cw != count || ln || lw) return 0;
    } else if (kind == HUFFMAN) {
        if (!count || stride || cn != count || !cw || cw >= count || ln || lw) return 0;
    } else if (!count || !qlic_lite_match_bound(count, &bound) ||
               !cn || cn > bound || ln > count || !cw || !lw ||
               cw > cn + 1 || lw > ln + 1 || cw + lw >= count) return 0;
    uint8_t *decoded = qli_malloc(count ? count : 1), *control = NULL, *literals = NULL;
    int ok = 0;
    if (!decoded) return 0;
    if (kind == RAW) {
        if (count) memcpy(decoded, input + PACK_HEADER, count);
    } else if (kind == HUFFMAN) {
        if (!qlic_lite_huffman_bytes_decode(input + PACK_HEADER, cw, decoded, count, NULL)) goto done;
    } else {
        control = qli_malloc(cn); literals = qli_malloc(ln ? ln : 1);
        if (!control || !literals ||
            !qlic_lite_huffman_bytes_decode(input + PACK_HEADER, cw, control, cn, NULL) ||
            !qlic_lite_huffman_bytes_decode(input + PACK_HEADER + cw, lw, literals, ln, NULL) ||
            !match_decode_split(control, cn, stride, decoded, count, literals, ln)) goto done;
    }
    if (qli_read_le32(input + 28) != qint_crc32(decoded, count)) goto done;
    *output = decoded; *output_size = count; decoded = NULL; ok = 1;
done:
    qli_free(decoded); qli_free(control); qli_free(literals);
    return ok;
}


#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42 && (defined(_M_X64) || defined(__x86_64__))
#define QLI_PALETTE_X86 1
#endif

enum { PALETTE_PIXEL_CAP=1u<<24, PALETTE_FILE_CAP=1u<<27, PALETTE_HEADER=19 };

static inline int palette_dimensions(unsigned w,unsigned h)
{
    return w && h && w<=16384 && h<=16384 && (size_t)w*h<=PALETTE_PIXEL_CAP;
}

static inline unsigned palette_exchange(unsigned value,unsigned a,unsigned b)
{
    return value==a ? b : value==b ? a : value;
}

#ifdef QLI_PALETTE_X86
static inline __m128i palette_load16(const uint8_t *p)
{
    __m128i value;
    memcpy(&value,p,sizeof(value));
    return value;
}

static inline __m128i palette_select(__m128i a,__m128i b,__m128i mask)
{
    return _mm_or_si128(_mm_andnot_si128(mask,a),_mm_and_si128(mask,b));
}

static inline __m128i palette_exchange_vector(__m128i value,__m128i a,__m128i b)
{
    __m128i mask=_mm_or_si128(_mm_cmpeq_epi8(value,a),_mm_cmpeq_epi8(value,b));
    return _mm_xor_si128(value,_mm_and_si128(mask,_mm_xor_si128(a,b)));
}

static inline __m128i palette_second_vector(__m128i north,__m128i other)
{
    const __m128i zero=_mm_setzero_si128(),one=_mm_set1_epi8(1);
    const __m128i source=palette_exchange_vector(other,north,zero);
    return palette_select(source,one,_mm_cmpeq_epi8(source,zero));
}
#endif

#if defined(_MSC_VER) && defined(__clang__) && defined(_M_X64)
#define QLI_PALETTE_C_BLOCKS 1

static inline uint8_t palette_restore_label(uint8_t token, uint8_t north, uint8_t other)
{
    const uint8_t source_mask = (uint8_t)-(int)((other == north) | (other == 0));
    const uint8_t source = other ^ (source_mask & north);
    const uint8_t second = source | (source == 0);
    const uint8_t first_mask = (uint8_t)-(int)((token == second) | (token == 1));
    const uint8_t middle = token ^ (first_mask & (second ^ 1));
    const uint8_t last_mask = (uint8_t)-(int)((middle == north) | (middle == 0));
    return middle ^ (last_mask & north);
}

static inline void palette_restore_block(uint8_t out[restrict static 16],
                                         const uint8_t tokens[restrict static 16],
                                         const uint8_t north[restrict static 16],
                                         const uint8_t other[restrict static 16])
{
#pragma clang loop vectorize_width(16) interleave_count(1)
    for (unsigned i = 0; i < 16; ++i)
        out[i] = palette_restore_label(tokens[i], north[i], other[i]);
}

static unsigned palette_invert_blocks(uint8_t *restrict row, const uint8_t *restrict north,
                                      unsigned width)
{
    static const uint8_t zero[16] = {0};
    const uint64_t odd_mask = UINT64_C(0xff00ff00ff00ff00);
    unsigned x = 0;
    for (; width - x >= 16; x += 16) {
        uint64_t first, last;
        memcpy(&first, row + x, sizeof(first));
        memcpy(&last, row + x + 8, sizeof(last));
        const uint8_t *above = north ? north + x : zero;
        if (!(first | last)) {
            memcpy(row + x, above, 16);
            continue;
        }
        uint8_t upper_left[16], even[16], shifted[16], odd[16];
        upper_left[0] = north ? north[x ? x-1 : 0] : 0;
        memcpy(upper_left + 1, above, 15);
        palette_restore_block(even, row + x, above, upper_left);
        shifted[0] = 0;
        memcpy(shifted + 1, even, 15);
        palette_restore_block(odd, row + x, above, shifted);
        for (unsigned i = 0; i < 16; i += 8) {
            uint64_t a, b;
            memcpy(&a, even + i, sizeof(a));
            memcpy(&b, odd + i, sizeof(b));
            const uint64_t value = (a & ~odd_mask) | (b & odd_mask);
            memcpy(row + x + i, &value, sizeof(value));
        }
    }
    return x;
}
#endif

static void invert(uint8_t *labels,unsigned w,unsigned h)
{
#ifdef QLI_PALETTE_X86
    const __m128i zero=_mm_setzero_si128(),one=_mm_set1_epi8(1);
    const __m128i odd=_mm_set1_epi16(-256);
    unsigned end=w>=16 && (qli_cpu_capabilities()&QLI_CPU_SSE42) ? w&~15u : 0;
#endif
    for (unsigned y=0;y<h;++y) {
        uint8_t *row=labels+(size_t)y*w;
        const uint8_t *north=y ? row-w : NULL;
        unsigned x=0;
#ifdef QLI_PALETTE_X86
        for (;x<end;x+=16) {
            __m128i token=palette_load16(row+x);
            __m128i n=north ? palette_load16(north+x) : zero;
            __m128i nw=_mm_slli_si128(n,1);
            if (north) nw=_mm_or_si128(nw,_mm_cvtsi32_si128(north[x ? x-1 : 0]));
            __m128i even=palette_exchange_vector(palette_exchange_vector(token,palette_second_vector(n,nw),one),n,zero);
            __m128i other=palette_select(nw,_mm_slli_si128(even,1),odd);
            __m128i value=palette_exchange_vector(palette_exchange_vector(token,palette_second_vector(n,other),one),n,zero);
            value=palette_select(even,value,odd);
            memcpy(row+x,&value,sizeof(value));
        }
#endif
#ifdef QLI_PALETTE_C_BLOCKS
        if (!x) x = palette_invert_blocks(row, north, w);
#endif
        for (;x<w;++x) {
            unsigned n=north ? north[x] : 0;
            unsigned other=(x&1) ? row[x-1] : (north && x ? north[x-1] : n);
            unsigned source=palette_exchange(other,n,0);
            row[x]=(uint8_t)palette_exchange(palette_exchange(row[x],source ? source : 1,1),n,0);
        }
    }
}
#undef QLI_PALETTE_C_BLOCKS

int pk_decode_owned(const uint8_t *file,size_t bytes,uint8_t **output,size_t *out_bytes)
{
    *output=NULL; *out_bytes=0;
    if (!file || bytes<PALETTE_HEADER+9 || bytes>PALETTE_FILE_CAP) return 0;
    unsigned channels = 4;
    if (!memcmp(file,"QPK3",4)) channels = 3;
    if (channels == 4 && memcmp(file,"QPK7",4)) return 0;
    unsigned w=qli_read_le32(file+4),h=qli_read_le32(file+8),count=qli_read_le16(file+12),mode=file[14];
    if (!palette_dimensions(w,h) || !count || count>256 || mode>1 ||
        qint_crc32(file,bytes-4)!=qli_read_le32(file+bytes-4)) return 0;
    size_t n=(size_t)w*h,start=PALETTE_HEADER+channels*count;
    if (start>bytes || bytes-start<9) return 0;
    uint32_t palette[256];
    for (unsigned i=0;i<count;++i) {
        const uint8_t *entry=file+PALETTE_HEADER+channels*i;
        palette[i]=channels==4 ? qli_read_le32(entry) : qli_read_le16(entry) | (uint32_t)entry[2]<<16;
        if (i && palette[i]<=palette[i-1]) return 0;
    }
    unsigned kind=file[start];
    size_t size=qli_read_le32(file+start+1);
    if (size!=bytes-start-9 || kind>1 || (!kind && size!=n) || (kind && (!size || size>=n))) return 0;
    uint8_t *labels=qli_malloc(n),*raw=qli_malloc(channels*n);
    int ok=0;
    if (!labels || !raw) goto done;
    if (kind) {
        size_t decoded=0;
        if (!qbs_decode(file+start+5,size,labels,n,&decoded,QBS_BYTE_TABLE) || decoded!=n) goto done;
    } else memcpy(labels,file+start+5,n);
    if (mode) invert(labels,w,h);
    for (size_t i=0;i<n;++i) {
        if (labels[i]>=count) goto done;
        uint8_t *pixel=raw+channels*i;
        uint32_t color=palette[labels[i]];
        if (channels==4) qli_write_le32(pixel,color);
        else {
            qli_write_le16(pixel,(uint16_t)color);
            pixel[2]=(uint8_t)(color>>16);
        }
    }
    if (qint_crc32(raw,channels*n)!=qli_read_le32(file+15)) goto done;
    *output=raw; *out_bytes=channels*n; raw=NULL; ok=1;
done:
    qli_free(labels); qli_free(raw);
    return ok;
}

typedef struct { const uint8_t *data; size_t bytes; uint8_t *owned; } Record;

static int unpack_record(const uint8_t **cursor,const uint8_t *end,size_t count,Record *record,int action)
{
    const uint8_t *p=*cursor;
    if ((size_t)(end-p)<5) return 0;
    unsigned kind=p[0];
    size_t bytes=qli_read_le32(p+1);
    p+=5;
    if (kind>1 || bytes>(size_t)(end-p) || (!kind && bytes!=count) ||
        (kind && (!bytes || bytes>=count))) return 0;
    if (kind) {
        uint8_t *decoded=qli_malloc(count);
        if (!decoded) return 0;
        *record=(Record){decoded,count,decoded};
        size_t size=0;
        if (action && bytes>=4 && !memcmp(p,"QVP1",4)) {
            if (!qvp_read(p,bytes,decoded,count,0)) return 0;
        } else if (!qbs_decode(p,bytes,decoded,count,&size,QBS_BYTE_TABLE) || size!=count) return 0;
    } else *record=(Record){p,count,NULL};
    *cursor=p+bytes;
    return 1;
}

static int take_gap(const uint8_t *data,size_t bytes,size_t *at,size_t limit,size_t *gap)
{
    size_t value=0;
    for (unsigned shift=0;shift<35;shift+=7) {
        if (*at==bytes) return 0;
        const unsigned b=data[(*at)++];
        value|=(size_t)(b&127)<<shift;
        if (b<128) {
            if ((shift && !b) || value>limit) return 0;
            *gap=value;
            return 1;
        }
    }
    return 0;
}

static void repeat_bytes(uint8_t *dst,size_t distance,size_t length)
{
    if (distance>=length) {
        memcpy(dst,dst-distance,length);
        return;
    }
    if (distance==1) {
        memset(dst,dst[-1],length);
        return;
    }
    memcpy(dst,dst-distance,distance);
    size_t copied=distance;
    while (copied<length) {
        size_t step=length-copied;
        if (step>copied) step=copied;
        memcpy(dst+copied,dst,step);
        copied+=step;
    }
}

static int unpack_repeat_actions(const uint8_t **cursor,const uint8_t *end,
                                 size_t count,Record *record)
{
    const uint8_t *p=*cursor;
    if ((size_t)(end-p)<12) return 0;
    const size_t literals=qli_read_le32(p),matches=qli_read_le32(p+4),gap_bytes=qli_read_le32(p+8);
    if (!literals || literals>=count || !matches || matches>(count-literals)/4 ||
        gap_bytes<matches || gap_bytes>5*matches || gap_bytes>literals+matches) return 0;
    p+=12;
    Record parts[5]={{0}};
    const size_t sizes[5]={literals,gap_bytes,matches,matches,matches};
    uint8_t *actions=NULL;
    int ok=0;
    for (unsigned i=0;i<5;++i)
        if (!unpack_record(&p,end,sizes[i],&parts[i],i==0)) goto done;
    actions=qli_malloc(count);
    if (!actions) goto done;
    size_t at=0,from=0,gap_at=0;
    for (size_t i=0;i<matches;++i) {
        size_t gap=0;
        const size_t length=4u+parts[2].data[i];
        const size_t distance=1u+parts[3].data[i]+((unsigned)parts[4].data[i]<<8);
        if (!take_gap(parts[1].data,gap_bytes,&gap_at,literals-from,&gap) ||
            gap>count-at || length>255 || length>count-at-gap || distance>at+gap) goto done;
        memcpy(actions+at,parts[0].data+from,gap);
        from+=gap;
        at+=gap;
        repeat_bytes(actions+at,distance,length);
        at+=length;
    }
    if (gap_at!=gap_bytes || literals-from!=count-at) goto done;
    memcpy(actions+at,parts[0].data+from,literals-from);
    *record=(Record){actions,count,actions};
    *cursor=p;
    actions=NULL;
    ok=1;
done:
    for (unsigned i=0;i<5;++i) qli_free(parts[i].owned);
    qli_free(actions);
    return ok;
}

int history_decode_owned(const uint8_t *file,size_t bytes,uint8_t **output,size_t *out_bytes)
{
    *output=NULL; *out_bytes=0;
    if (!file || bytes<QLI_HISTORY_HEADER+24 || bytes>QLI_HISTORY_FILE_CAP) return 0;
    unsigned channels = 4;
    if (!memcmp(file,"QRH3",4) || !memcmp(file,"QRHP",4)) channels=3;
    int virtual_alpha=0;
    if (!memcmp(file,"QRV3",4) || !memcmp(file,"QRVP",4)) {
        virtual_alpha=1;
        channels=3;
    }
    const int phrases = !memcmp(file, virtual_alpha ? "QRVP" : channels==3 ? "QRHP" : "QCHR", 4);
    if (!phrases && memcmp(file, virtual_alpha ? "QRV3" : channels==3 ? "QRH3" : "QCRH", 4)) return 0;
    if (channels==4 && bytes<QLI_HISTORY_HEADER+29) return 0;
    const unsigned w=qli_read_le32(file+4),h=qli_read_le32(file+8);
    const size_t n=(size_t)w*h,actions=qli_read_le32(file+12),literals=qli_read_le32(file+16),copies=qli_read_le32(file+24);
    if (!history_dimensions(w,h) || !literals || literals>actions || actions>n || copies>actions || copies>n/8 ||
        qint_crc32(file,bytes-4)!=qli_read_le32(file+bytes-4)) return 0;

    const uint8_t *cursor=file+QLI_HISTORY_HEADER,*end=file+bytes-4;
    Record records[QLI_HISTORY_RECORDS]={{0}};
    uint16_t *distance=NULL;
    uint8_t *errors=NULL,*raw=NULL;
    int ok=0;
    if (phrases) {
        if (!unpack_repeat_actions(&cursor, end, actions, &records[0])) goto done;
    } else if (!unpack_record(&cursor,end,actions,&records[0],1)) goto done;
    const unsigned first=copies ? 4 : 1;
    if (copies) {
        for (unsigned i=1;i<4;++i)
            if (!unpack_record(&cursor,end,copies,&records[i],0)) goto done;
        distance=qli_malloc(copies*sizeof(*distance));
        if (!distance) goto done;
        for (size_t j=0;j<copies;++j)
            distance[j]=(uint16_t)(records[2].data[j]|((unsigned)records[3].data[j]<<8));
        for (unsigned i=2;i<4;++i) {
            qli_free(records[i].owned);
            records[i]=(Record){0};
        }
    }
    for (unsigned c=0;c<channels;++c)
        if (!unpack_record(&cursor,end,literals,&records[first+c],0)) goto done;
    if (cursor!=end) goto done;
    errors=qli_malloc(channels*literals);
    raw=qli_malloc(channels*n);
    if (!errors || !raw) goto done;
    const uint8_t *g=records[first].data,*r=records[first+1].data;
    const uint8_t *b=records[first+2].data,*a=channels==4 ? records[first+3].data : NULL;
    for (size_t i=0;i<literals;++i) {
        uint8_t *error=errors+channels*i;
        error[0]=(uint8_t)(r[i]+g[i]);
        error[1]=g[i];
        error[2]=(uint8_t)(b[i]+g[i]);
        if (a) error[3]=a[i];
    }
    if (!history_reconstruct(records[0].data,actions,errors,literals,w,h,raw,channels*n,
                             records[1].data,distance,copies,channels,virtual_alpha) ||
        qint_crc32(raw,channels*n)!=qli_read_le32(file+20)) goto done;
    *output=raw; *out_bytes=channels*n; raw=NULL; ok=1;
done:
    for (unsigned i=0;i<QLI_HISTORY_RECORDS;++i) qli_free(records[i].owned);
    qli_free(distance); qli_free(errors); qli_free(raw);
    return ok;
}

static int valid_lanes(unsigned lanes) {
    static const uint8_t allowed[17] = {[1] = 1, [4] = 1, [8] = 1, [16] = 1};
    return lanes < sizeof(allowed) && allowed[lanes];
}

static int encode_payload(const uint8_t *symbols, size_t count,
                          unsigned lanes, const RansModel *model,
                          uint32_t state[16], uint8_t **payload_out,
                          size_t *payload_size_out) {
    if (count > (SIZE_MAX - 1u) / 2u) return 0;
    size_t capacity = count * 2u + 1u;
    uint8_t *buffer = (uint8_t *)qli_malloc(capacity);
    if (!buffer) return 0;
    uint8_t *cursor = buffer + capacity;
    for (unsigned lane = 0; lane < lanes; ++lane)
        state[lane] = RANS_BYTE_L;

    for (size_t position = count; position-- > 0;) {
        unsigned lane = (unsigned)(position % lanes);
        unsigned symbol = symbols[position];
        uint32_t frequency = model->frequency[symbol];
        uint32_t cumulative = model->cumulative[symbol];
        if (!frequency) {
            qli_free(buffer);
            return 0;
        }
        uint32_t value = state[lane];
        uint32_t maximum =
            ((RANS_BYTE_L >> QLIC_RANS_SCALE_BITS) << 8) * frequency;
        while (value >= maximum) {
            if (cursor == buffer) {
                qli_free(buffer);
                return 0;
            }
            *--cursor = (uint8_t)value;
            value >>= 8;
        }
        state[lane] = (value / frequency << QLIC_RANS_SCALE_BITS) +
                      value % frequency + cumulative;
    }

    size_t payload_size = (size_t)(buffer + capacity - cursor);
    uint8_t *payload = (uint8_t *)qli_malloc(payload_size ? payload_size : 1u);
    if (!payload) {
        qli_free(buffer);
        return 0;
    }
    if (payload_size) memcpy(payload, cursor, payload_size);
    qli_free(buffer);
    *payload_out = payload;
    *payload_size_out = payload_size;
    return 1;
}

int qlic_rans_encode_alloc(const uint8_t *symbols, size_t count,
                           unsigned lanes, uint8_t **frame_out,
                           size_t *size_out) {
    RansModel model;
    uint32_t state[16] = {0};
    uint8_t *payload = NULL;
    size_t payload_size = 0;
    uint8_t *frame = NULL;
    size_t capacity = 0;
    size_t offset = RANS_HEADER_BYTES;
    int okay = 0;
    if (frame_out) *frame_out = NULL;
    if (size_out) *size_out = 0;
    if (!frame_out || !size_out || !valid_lanes(lanes) ||
        (count && !symbols))
        return 0;
    if (!qlr_model_build(symbols, count, &model)) return 0;
    if (!encode_payload(symbols, count, lanes, &model, state,
                        &payload, &payload_size))
        return 0;
    if (payload_size > SIZE_MAX - RANS_HEADER_BYTES - 1026u - lanes * 4u)
        goto cleanup;
    capacity = RANS_HEADER_BYTES + 1026u + lanes * 4u + payload_size;
    frame = (uint8_t *)qli_malloc(capacity);
    if (!frame) goto cleanup;
    memcpy(frame, "QRX1", 4u);
    frame[4] = (uint8_t)lanes;
    frame[5] = QLIC_RANS_SCALE_BITS;
    qli_write_le64(frame + 6u, (uint64_t)count);
    frame[14] = 0;
    frame[15] = 0;
    if (!qlr_model_write(&model, frame, capacity, &offset)) goto cleanup;
    for (unsigned lane = 0; lane < lanes; ++lane) {
        if (capacity - offset < 4u) goto cleanup;
        qli_write_le32(frame + offset, state[lane]);
        offset += 4u;
    }
    if (capacity - offset < payload_size) goto cleanup;
    if (payload_size) memcpy(frame + offset, payload, payload_size);
    offset += payload_size;
    *frame_out = frame;
    *size_out = offset;
    frame = NULL;
    okay = 1;

cleanup:
    qli_free(frame);
    qli_free(payload);
    return okay;
}

int qlic_rans_decode(const uint8_t *frame, size_t size,
                     uint8_t *symbols_out, size_t capacity,
                     size_t *count_out) {
    RansModel model;
    uint32_t state[16] = {0};
    if (count_out) *count_out = 0;
    if (!frame || size < RANS_HEADER_BYTES || !count_out ||
        memcmp(frame, "QRX1", 4u) ||
        frame[5] != QLIC_RANS_SCALE_BITS || frame[14] || frame[15])
        return 0;
    unsigned lanes = frame[4];
    if (!valid_lanes(lanes)) return 0;
    uint64_t stored_count = qli_read_le64(frame + 6u);
    if (stored_count > SIZE_MAX || stored_count > capacity ||
        (stored_count && !symbols_out))
        return 0;
    size_t count = (size_t)stored_count;
    size_t offset = RANS_HEADER_BYTES;
    if (!qlr_model_read(frame, size, &offset, &model) ||
        (!!count != !!model.used) || size - offset < lanes * 4u)
        return 0;
    for (unsigned lane = 0; lane < lanes; ++lane) {
        state[lane] = qli_read_le32(frame + offset);
        offset += 4u;
        if (state[lane] < RANS_BYTE_L ||
            state[lane] >= (UINT32_C(1) << 31))
            return 0;
    }
    const uint8_t *cursor = frame + offset;
    const uint8_t *end = frame + size;
    if (model.used != 1) for (size_t position = 0; position < count; ++position) {
        unsigned lane = (unsigned)(position % lanes);
        uint32_t value = state[lane];
        unsigned slot = value & (QLIC_RANS_SCALE - 1u);
        unsigned symbol = model.decode[slot];
        symbols_out[position] = (uint8_t)symbol;
        value = (uint32_t)model.frequency[symbol] *
                    (value >> QLIC_RANS_SCALE_BITS) +
                slot - model.cumulative[symbol];
        while (value < RANS_BYTE_L) {
            if (cursor == end) return 0;
            value = (value << 8) | *cursor++;
        }
        state[lane] = value;
    }
    if (cursor != end) return 0;
    for (unsigned lane = 0; lane < lanes; ++lane)
        if (state[lane] != RANS_BYTE_L) return 0;
    if (model.used == 1) memset(symbols_out, model.decode[0], count);
    *count_out = count;
    return 1;
}

static int put_varint(uint8_t *output, size_t capacity, size_t *offset,
                      unsigned value) {
    do {
        if (*offset >= capacity) return 0;
        uint8_t byte = (uint8_t)(value & 127u);
        value >>= 7;
        output[(*offset)++] = (uint8_t)(byte | (value ? 128u : 0u));
    } while (value);
    return 1;
}

static int get_varint(const uint8_t *input, size_t size, size_t *offset,
                      unsigned *value_out) {
    unsigned value = 0;
    unsigned shift = 0;
    for (unsigned byte = 0; byte < 3u; ++byte, shift += 7u) {
        if (*offset >= size) return 0;
        unsigned current = input[(*offset)++];
        value |= (current & 127u) << shift;
        if (!(current & 128u)) {
            if (!value || value > QLIC_RANS_SCALE) return 0;
            *value_out = value;
            return 1;
        }
    }
    return 0;
}

static int rans_finish_model(RansModel *model) {
    unsigned cumulative = 0;
    model->used = 0;
    for (unsigned symbol = 0; symbol < RANS_SYMBOLS; ++symbol) {
        unsigned frequency = model->frequency[symbol];
        model->cumulative[symbol] = (uint16_t)cumulative;
        if (!frequency) continue;
        if (frequency > QLIC_RANS_SCALE - cumulative) return 0;
        for (unsigned slot = 0; slot < frequency; ++slot)
            model->decode[cumulative + slot] = (uint8_t)symbol;
        cumulative += frequency;
        model->used++;
    }
    return cumulative == QLIC_RANS_SCALE;
}

typedef struct {
    unsigned symbol[RANS_SYMBOLS];
    int position[RANS_SYMBOLS];
    long double key[RANS_SYMBOLS];
    unsigned size;
    int maximize;
} NormalizeHeap;

static int normalize_heap_better(const NormalizeHeap *heap, unsigned left,
                                 unsigned right) {
    long double left_key = heap->key[left];
    long double right_key = heap->key[right];
    if (left_key > right_key) return heap->maximize;
    if (left_key < right_key) return !heap->maximize;
    return left < right;
}

static void normalize_heap_swap(NormalizeHeap *heap, unsigned left,
                                unsigned right) {
    unsigned symbol = heap->symbol[left];
    heap->symbol[left] = heap->symbol[right];
    heap->symbol[right] = symbol;
    heap->position[heap->symbol[left]] = (int)left;
    heap->position[heap->symbol[right]] = (int)right;
}

static void normalize_heap_down(NormalizeHeap *heap, unsigned index) {
    for (;;) {
        unsigned best = index;
        unsigned left = index * 2u + 1u;
        unsigned right = left + 1u;
        if (left < heap->size && normalize_heap_better(
                heap, heap->symbol[left], heap->symbol[best])) best = left;
        if (right < heap->size && normalize_heap_better(
                heap, heap->symbol[right], heap->symbol[best])) best = right;
        if (best == index) return;
        normalize_heap_swap(heap, index, best);
        index = best;
    }
}

static void normalize_heap_up(NormalizeHeap *heap, unsigned index) {
    while (index) {
        unsigned parent = (index - 1u) / 2u;
        if (!normalize_heap_better(heap, heap->symbol[index],
                                   heap->symbol[parent])) return;
        normalize_heap_swap(heap, index, parent);
        index = parent;
    }
}

static void normalize_heap_init(NormalizeHeap *heap,
                                const uint64_t histogram[RANS_SYMBOLS],
                                const RansModel *model, int maximize) {
    memset(heap, 0, sizeof(*heap));
    heap->maximize = maximize;
    for (unsigned symbol = 0; symbol < RANS_SYMBOLS; ++symbol)
        heap->position[symbol] = -1;
    for (unsigned symbol = 0; symbol < RANS_SYMBOLS; ++symbol) {
        unsigned frequency = model->frequency[symbol];
        if (!frequency || (!maximize && frequency <= 1u)) continue;
        heap->key[symbol] = maximize
            ? (long double)histogram[symbol] *
                  logl((long double)(frequency + 1u) / frequency)
            : (long double)histogram[symbol] *
                  logl((long double)frequency / (frequency - 1u));
        heap->position[symbol] = (int)heap->size;
        heap->symbol[heap->size++] = symbol;
    }
    if (heap->size)
        for (unsigned index = heap->size / 2u; index; --index)
            normalize_heap_down(heap, index - 1u);
}

static void normalize_heap_refresh(NormalizeHeap *heap,
                                   const uint64_t histogram[RANS_SYMBOLS],
                                   const RansModel *model, unsigned symbol) {
    unsigned frequency = model->frequency[symbol];
    int position = heap->position[symbol];
    if (!frequency || (!heap->maximize && frequency <= 1u)) {
        if (position >= 0) {
            unsigned last = heap->symbol[heap->size - 1u];
            --heap->size;
            heap->position[symbol] = -1;
            if ((unsigned)position < heap->size) {
                heap->symbol[position] = last;
                heap->position[last] = position;
                normalize_heap_down(heap, (unsigned)position);
                normalize_heap_up(heap, (unsigned)heap->position[last]);
            }
        }
        return;
    }
    heap->key[symbol] = heap->maximize
        ? (long double)histogram[symbol] *
              logl((long double)(frequency + 1u) / frequency)
        : (long double)histogram[symbol] *
              logl((long double)frequency / (frequency - 1u));
    if (position >= 0) {
        normalize_heap_down(heap, (unsigned)position);
        normalize_heap_up(heap, (unsigned)heap->position[symbol]);
    }
}

int qlr_model_normalize(const uint64_t histogram[RANS_SYMBOLS],
                           uint64_t count, RansModel *model) {
    memset(model, 0, sizeof(*model));
    if (!count) return 1;
    unsigned sum = 0;
    for (unsigned symbol = 0; symbol < RANS_SYMBOLS; ++symbol) {
        if (!histogram[symbol]) continue;
        long double scaled =
            (long double)histogram[symbol] * QLIC_RANS_SCALE / count;
        unsigned frequency = (unsigned)scaled;
        if (!frequency) frequency = 1u;
        model->frequency[symbol] = (uint16_t)frequency;
        sum += frequency;
    }

    if (sum < QLIC_RANS_SCALE) {
        NormalizeHeap heap;
        normalize_heap_init(&heap, histogram, model, 1);
        while (sum < QLIC_RANS_SCALE) {
            unsigned best;
            if (!heap.size) return 0;
            best = heap.symbol[0];
            model->frequency[best]++;
            normalize_heap_refresh(&heap, histogram, model, best);
            sum++;
        }
    } else if (sum > QLIC_RANS_SCALE) {
        NormalizeHeap heap;
        normalize_heap_init(&heap, histogram, model, 0);
        while (sum > QLIC_RANS_SCALE) {
            unsigned best;
            if (!heap.size) return 0;
            best = heap.symbol[0];
            model->frequency[best]--;
            normalize_heap_refresh(&heap, histogram, model, best);
            sum--;
        }
    }

    if (!rans_finish_model(model)) return 0;
    for (unsigned symbol = 0; symbol < RANS_SYMBOLS; ++symbol) {
        if (!histogram[symbol]) continue;
        model->modeled_bits -= (double)histogram[symbol] *
            log2((double)model->frequency[symbol] / QLIC_RANS_SCALE);
    }
    return 1;
}

int qlr_model_build(const uint8_t *symbols, size_t count,
                       RansModel *model) {
    uint64_t histogram[RANS_SYMBOLS] = {0};
    if (count && !symbols) return 0;
    for (size_t index = 0; index < count; ++index)
        histogram[symbols[index]]++;
    return qlr_model_normalize(histogram, count, model);
}

static size_t varint_size(unsigned value) {
    size_t bytes = 1u;
    while (value >>= 7) bytes++;
    return bytes;
}

size_t qlr_model_descriptor_bytes(const RansModel *model) {
    size_t bytes = 2u;
    unsigned written = 0;
    for (unsigned symbol = 0; symbol < RANS_SYMBOLS; ++symbol) {
        unsigned frequency = model->frequency[symbol];
        if (!frequency) continue;
        bytes++;
        if (++written != model->used) bytes += varint_size(frequency);
    }
    return bytes;
}

int qlic_rans_measure_histogram(const uint64_t histogram[256],
                                QlicRansModelStats *stats_out) {
    RansModel model;
    uint64_t count = 0;
    if (stats_out) memset(stats_out, 0, sizeof(*stats_out));
    if (!histogram || !stats_out) return 0;
    for (unsigned symbol = 0; symbol < RANS_SYMBOLS; ++symbol) {
        if (UINT64_MAX - count < histogram[symbol]) return 0;
        count += histogram[symbol];
    }
    if (!qlr_model_normalize(histogram, count, &model)) return 0;
    stats_out->descriptor_bytes = qlr_model_descriptor_bytes(&model);
    stats_out->used_symbols = model.used;
    stats_out->modeled_bits = model.modeled_bits;
    return 1;
}

int qlr_model_write(const RansModel *model, uint8_t *output,
                       size_t capacity, size_t *offset) {
    if (*offset > capacity || capacity - *offset < 2u) return 0;
    qli_write_le16(output + *offset, (uint16_t)model->used);
    *offset += 2u;
    if (!model->used) return 1;
    unsigned remaining = QLIC_RANS_SCALE;
    unsigned written = 0;
    for (unsigned symbol = 0; symbol < RANS_SYMBOLS; ++symbol) {
        unsigned frequency = model->frequency[symbol];
        if (!frequency) continue;
        if (*offset >= capacity) return 0;
        output[(*offset)++] = (uint8_t)symbol;
        written++;
        if (written != model->used &&
            !put_varint(output, capacity, offset, frequency))
            return 0;
        if (frequency > remaining) return 0;
        remaining -= frequency;
    }
    return written == model->used && !remaining;
}

int qlr_model_read(const uint8_t *input, size_t size, size_t *offset,
                      RansModel *model) {
    memset(model, 0, sizeof(*model));
    if (*offset > size || size - *offset < 2u) return 0;
    unsigned used = qli_read_le16(input + *offset);
    *offset += 2u;
    if (used > RANS_SYMBOLS) return 0;
    if (!used) return 1;
    unsigned remaining = QLIC_RANS_SCALE;
    int previous = -1;
    for (unsigned index = 0; index < used; ++index) {
        if (*offset >= size) return 0;
        unsigned symbol = input[(*offset)++];
        if ((int)symbol <= previous) return 0;
        previous = (int)symbol;
        unsigned frequency = remaining;
        if (index + 1u != used &&
            !get_varint(input, size, offset, &frequency))
            return 0;
        if (!frequency || frequency > remaining) return 0;
        model->frequency[symbol] = (uint16_t)frequency;
        remaining -= frequency;
    }
    if (remaining || !rans_finish_model(model) || model->used != used) return 0;
    return 1;
}

#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42 && (defined(_M_X64) || defined(__x86_64__))

static inline __m128i qpf_prefix1_x86(__m128i v)
{
    v = _mm_add_epi8(v, _mm_slli_si128(v, 1));
    v = _mm_add_epi8(v, _mm_slli_si128(v, 2));
    v = _mm_add_epi8(v, _mm_slli_si128(v, 4));
    return _mm_add_epi8(v, _mm_slli_si128(v, 8));
}

static inline __m128i qpf_prefix3_x86(__m128i v)
{
    v = _mm_add_epi8(v, _mm_slli_si128(v, 3));
    v = _mm_add_epi8(v, _mm_slli_si128(v, 6));
    return _mm_add_epi8(v, _mm_slli_si128(v, 12));
}

static inline __m128i qpf_prefix4_x86(__m128i v)
{
    v = _mm_add_epi8(v, _mm_slli_si128(v, 4));
    return _mm_add_epi8(v, _mm_slli_si128(v, 8));
}

static inline size_t qpf_left_x86(uint8_t *row, size_t bytes, unsigned channels)
{
    size_t x = 0;
    __m128i previous = _mm_setzero_si128();
#define QPF_X86_BLOCKS(C) do { \
    for (; bytes - x >= 16; x += 16) { \
        __m128i v; \
        memcpy(&v, row + x, sizeof(v)); \
        previous = qpf_prefix##C##_x86(_mm_add_epi8(v, _mm_srli_si128(previous, 16 - (C)))); \
        memcpy(row + x, &previous, sizeof(previous)); \
    } \
} while (0)
    switch (channels) {
    case 1: QPF_X86_BLOCKS(1); break;
    case 3: QPF_X86_BLOCKS(3); break;
    case 4: QPF_X86_BLOCKS(4); break;
    }
#undef QPF_X86_BLOCKS
    return x;
}
#endif

static inline uint64_t qpf_add_bytes(uint64_t a, uint64_t b)
{
    const uint64_t low = UINT64_C(0x7f7f7f7f7f7f7f7f);
    return ((a & low) + (b & low)) ^ ((a ^ b) & ~low);
}

static inline uint64_t qpf_prefix1(uint64_t v)
{
    v = qpf_add_bytes(v, v << 8);
    v = qpf_add_bytes(v, v << 16);
    return qpf_add_bytes(v, v << 32);
}

static inline uint64_t qpf_prefix3(uint64_t v)
{
    v = qpf_add_bytes(v, v << 24);
    return qpf_add_bytes(v, v << 48);
}

static inline uint64_t qpf_prefix4(uint64_t v)
{
    return qpf_add_bytes(v, v << 32);
}

static inline size_t qpf_left_words(uint8_t *row, size_t bytes, unsigned channels)
{
    const uint16_t one = 1;
    size_t x = 0;
    if (*(const unsigned char *)&one == 1) {
        uint64_t previous = 0;
#define QPF_BLOCKS(C, REPEAT) do { \
    for (; bytes - x >= 8; x += 8) { \
        uint64_t v; \
        memcpy(&v, row + x, sizeof(v)); \
        uint64_t carry = (previous >> (64 - 8*(C))) * (REPEAT); \
        previous = qpf_add_bytes(qpf_prefix##C(v), carry); \
        memcpy(row + x, &previous, sizeof(previous)); \
    } \
} while (0)
        switch (channels) {
        case 1: QPF_BLOCKS(1, UINT64_C(0x0101010101010101)); break;
        case 3: QPF_BLOCKS(3, UINT64_C(0x0001000001000001)); break;
        case 4: QPF_BLOCKS(4, UINT64_C(0x0000000100000001)); break;
        }
#undef QPF_BLOCKS
    }
    return x;
}

static inline void qpf_left_blocks(uint8_t *row, size_t bytes, unsigned channels, int native)
{
    size_t x;
#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42 && (defined(_M_X64) || defined(__x86_64__))
    if (native)
        x = qpf_left_x86(row, bytes, channels);
    else
#else
    (void)native;
#endif
        x = qpf_left_words(row, bytes, channels);
    for (; x < bytes; ++x)
        row[x] = (uint8_t)(row[x] + (x >= channels ? row[x - channels] : 0));
}

int fast_decode_alloc(const uint8_t *file, size_t size, size_t max_rgba_bytes,
                       uint8_t **rgba, uint32_t *width, uint32_t *height) {
    if (!rgba || !width || !height) return 0;
    *rgba = NULL; *width = *height = 0;
    if (!file || size < FAST_HEADER + FAST_FOOTER || size > (1u << 28) || memcmp(file, "QPF1", 4) || file[15] || qli_read_le32(file + 24) || qli_read_le32(file + 28)) return 0;
    uint32_t w = qli_read_le32(file + 4), h = qli_read_le32(file + 8);
    size_t pixels = 0;
    if (!fast_shape(w, h, &pixels) || pixels > max_rgba_bytes / 4) return 0;
    unsigned channels = file[12], mode = file[13], green = file[14];
    unsigned colors = qli_read_le32(file + 16); size_t packed_size = qli_read_le32(file + 20);
    if ((channels != 1 && channels != 3 && channels != 4) || mode > FAST_SOLID || green > 1 || colors > 256) return 0;
    if ((mode <= FAST_ROW && (colors || green != (unsigned)(mode != FAST_RAW && channels >= 3))) ||
        (mode == FAST_PALETTE && (channels != 4 || green || colors < 2)) ||
        (mode == FAST_SOLID && (channels != 4 || green || colors != 1 || packed_size))) return 0;
    size_t prefix = FAST_HEADER + (size_t)colors * 4;
    if (prefix > size - FAST_FOOTER || packed_size != size - FAST_FOOTER - prefix) return 0;
    if (qli_read_le32(file + size - FAST_FOOTER) != qli_wrapped_crc32(file, prefix, packed_size))
        return 0;
    uint8_t *compact = NULL; size_t compact_size = 0;
    size_t map_size = mode == FAST_ROW ? h : 0;
    size_t expected = mode == FAST_PALETTE ? pixels : pixels * channels + map_size;
    if (mode != FAST_SOLID && (!pack_decode(file + prefix, packed_size, expected, &compact, &compact_size) || compact_size != expected)) { qli_free(compact); return 0; }
    if (mode <= FAST_LEFT && channels == 4) {
        if (mode == FAST_LEFT) {
            size_t row_bytes = (size_t)w * 4;
            int native = 0;
#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42
            if (row_bytes >= 16) native = (qli_cpu_capabilities() & QLI_CPU_SSE42) != 0;
#endif
            for (uint32_t y = 0; y < h; ++y) {
                uint8_t *row = compact + (size_t)y * row_bytes;
                qpf_left_blocks(row, row_bytes, 4, native);
                for (uint8_t *p = row; p < row + row_bytes; p += 4) {
                    p[0] = (uint8_t)(p[0] + p[1]);
                    p[2] = (uint8_t)(p[2] + p[1]);
                }
            }
        }
        *rgba = compact;
        *width = w;
        *height = h;
        return 1;
    }
    uint8_t *out = qli_malloc(pixels * 4); int ok = 0;
    if (!out) goto done;
    if (mode == FAST_SOLID) {
        for (size_t i = 0; i < pixels; ++i) memcpy(out + i * 4, file + FAST_HEADER, 4);
    } else if (mode == FAST_PALETTE) {
        for (size_t i = 0; i < pixels; ++i) {
            if (compact[i] >= colors) goto done;
            memcpy(out + i * 4, file + FAST_HEADER + (size_t)compact[i] * 4, 4);
        }
    } else {
        size_t row_size = (size_t)w * channels;
        int native_left = 0;
#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42
        if (mode != FAST_RAW && row_size >= 16)
            native_left = (qli_cpu_capabilities() & QLI_CPU_SSE42) != 0;
#endif
        for (uint32_t y = 0; y < h; ++y) {
            uint8_t *row = compact + map_size + (size_t)y * row_size;
            uint8_t *up = y ? row - row_size : NULL;
            unsigned filter = mode == FAST_ROW ? compact[y] : mode == FAST_LEFT ? 1u : 0u;
            if (filter > 3) goto done;
            if(filter==1)qpf_left_blocks(row,row_size,channels,native_left);
            else if (filter) for (size_t x = 0; x < row_size; ++x)
                row[x] = (uint8_t)(row[x] + fast_predict(filter, x >= channels ? row[x - channels] : 0, up ? up[x] : 0, up && x >= channels ? up[x - channels] : 0));
            for (uint32_t x = 0; x < w; ++x) {
                uint8_t *p = row + (size_t)x * channels;
                uint8_t *q = out + ((size_t)y * w + x) * 4;
                if (channels == 1) { q[0] = q[1] = q[2] = p[0]; q[3] = 255; }
                else { q[0] = (uint8_t)(p[0] + (green ? p[1] : 0)); q[1] = p[1]; q[2] = (uint8_t)(p[2] + (green ? p[1] : 0)); q[3] = channels == 4 ? p[3] : 255; }
            }
        }
    }
    *rgba = out; out = NULL; *width = w; *height = h; ok = 1;
done:
    qli_free(out); qli_free(compact); return ok;
}

enum { TILE = 16, GROUPS = 4, MAX_STREAMS = 16, SIMPLE_HEADER = 32, SIMPLE_FOOTER = 4 };
static uint8_t simple_predict(unsigned mode, int l, int u, int c) {
    if (!mode) return 0;
    if (mode == 1) return (uint8_t)l;
    if (mode == 2) return (uint8_t)u;
    int v = l + u - c, low = l < u ? l : u, high = l > u ? l : u;
    return (uint8_t)(v < low ? low : v > high ? high : v);
}
static uint8_t residual_symbol(uint8_t value, uint8_t prediction) {
    unsigned v = (uint8_t)(value - prediction);
    return (uint8_t)(v < 128 ? v * 2 : (256 - v) * 2 - 1);
}
static unsigned magnitude(uint8_t value) { return ((unsigned)value + 1) / 2; }
static unsigned bit_cost(uint8_t value) {
    unsigned n = magnitude(value) + 1, cost = 1;
    while (n > 1) { n >>= 1; cost += 2; }
    return cost;
}
static uint8_t plane_predict(const uint8_t *plane, size_t pos, uint32_t x, uint32_t y, uint32_t w, unsigned mode) {
    return simple_predict(mode, x ? plane[pos - 1] : 0, y ? plane[pos - w] : 0, x && y ? plane[pos - w - 1] : 0);
}
static size_t tile_pixels(uint32_t tx, uint32_t ty, uint32_t w, uint32_t h) {
    uint32_t a = w - tx < TILE ? w - tx : TILE, b = h - ty < TILE ? h - ty : TILE;
    return (size_t)a * b;
}
int qps_encode_alloc(const uint8_t *rgba, uint32_t w, uint32_t h, uint8_t **file, size_t *size) {
    if (!file || !size) return 0;
    *file = NULL; *size = 0; size_t pixels = 0;
    if (!rgba || !fast_shape(w, h, &pixels)) return 0;
    unsigned channels = qli_rgba_channels(rgba, pixels), streams = channels * GROUPS;
    size_t tiles = (size_t)((w + TILE - 1) / TILE) * ((h + TILE - 1) / TILE), map_size = tiles * channels;
    uint8_t *planes = qli_malloc(pixels * channels), *map = qli_malloc(map_size), *pool = qli_malloc(pixels * channels), *out = NULL;
    uint8_t *encoded[MAX_STREAMS] = {0}; size_t wire[MAX_STREAMS] = {0}, counts[MAX_STREAMS] = {0}, offsets[MAX_STREAMS] = {0}, cursor[MAX_STREAMS] = {0};
    int ok = 0;
    if (!planes || !map || !pool) goto done;
    for (size_t i = 0; i < pixels; ++i) {
        if (channels == 1) planes[i] = rgba[i*4];
        else {
            planes[i] = (uint8_t)(rgba[i*4] - rgba[i*4+1]); planes[pixels+i] = rgba[i*4+1];
            planes[pixels*2+i] = (uint8_t)(rgba[i*4+2] - rgba[i*4+1]);
            if (channels == 4) planes[pixels*3+i] = rgba[i*4+3];
        }
    }
    for (unsigned c = 0; c < channels; ++c) {
        const uint8_t *plane = planes + c * pixels; size_t tile = 0;
        for (uint32_t ty = 0; ty < h; ty += TILE) for (uint32_t tx = 0; tx < w; tx += TILE, ++tile) {
            uint64_t scores[4] = {0}, mags[4] = {0};
            for (uint32_t y = ty; y < h && y < ty + TILE; ++y) for (uint32_t x = tx; x < w && x < tx + TILE; ++x) {
                size_t pos = (size_t)y * w + x;
                for (unsigned mode = 0; mode < 4; ++mode) {
                    uint8_t v = residual_symbol(plane[pos], plane_predict(plane, pos, x, y, w, mode));
                    scores[mode] += bit_cost(v); mags[mode] += magnitude(v);
                }
            }
            unsigned mode = 0; for (unsigned k = 1; k < 4; ++k) if (scores[k] < scores[mode]) mode = k;
            size_t n = tile_pixels(tx, ty, w, h);
            unsigned group = mags[mode] <= n ? 0 : mags[mode] <= n*4 ? 1 : mags[mode] <= n*16 ? 2 : 3;
            map[c*tiles+tile] = (uint8_t)(mode | (group << 2)); counts[c*GROUPS+group] += n;
        }
    }
    size_t offset = 0;
    for (unsigned s = 0; s < streams; ++s) { offsets[s] = cursor[s] = offset; offset += counts[s]; }
    for (unsigned c = 0; c < channels; ++c) {
        const uint8_t *plane = planes + c * pixels; size_t tile = 0;
        for (uint32_t ty = 0; ty < h; ty += TILE) for (uint32_t tx = 0; tx < w; tx += TILE, ++tile) {
            unsigned code = map[c*tiles+tile], mode = code & 3, s = c*GROUPS + (code >> 2);
            for (uint32_t y = ty; y < h && y < ty + TILE; ++y) for (uint32_t x = tx; x < w && x < tx + TILE; ++x) {
                size_t pos = (size_t)y * w + x;
                pool[cursor[s]++] = residual_symbol(plane[pos], plane_predict(plane, pos, x, y, w, mode));
            }
        }
    }
    size_t total = SIMPLE_HEADER + map_size + streams*4 + SIMPLE_FOOTER;
    for (unsigned s = 0; s < streams; ++s) {
        if (!qlic_rans_encode_alloc(pool + offsets[s], counts[s], 4, &encoded[s], &wire[s]) || wire[s] > (1u << 28) || total > (1u << 28) - wire[s]) goto done;
        total += wire[s];
    }
    out = qli_calloc(1, total); if (!out) goto done;
    memcpy(out, "QPS1", 4); qli_write_le32(out+4, w); qli_write_le32(out+8, h);
    out[12] = (uint8_t)channels; out[13] = (uint8_t)(channels >= 3); out[14] = 4;
    qli_write_le32(out+16, (uint32_t)map_size); qli_write_le32(out+20, streams); qli_write_le32(out+24, qint_crc32(rgba, pixels*4));
    memcpy(out+SIMPLE_HEADER, map, map_size); offset = SIMPLE_HEADER + map_size + streams*4;
    for (unsigned s = 0; s < streams; ++s) {
        qli_write_le32(out+SIMPLE_HEADER+map_size+s*4, (uint32_t)wire[s]); memcpy(out+offset, encoded[s], wire[s]); offset += wire[s];
    }
    qli_write_le32(out+total-SIMPLE_FOOTER, qint_crc32(out, total-SIMPLE_FOOTER));
    *file = out; *size = total; out = NULL; ok = 1;
done:
    qli_free(planes); qli_free(map); qli_free(pool); qli_free(out); for (unsigned s = 0; s < MAX_STREAMS; ++s) qli_free(encoded[s]);
    return ok;
}
static uint8_t undo_symbol(unsigned v) { return (uint8_t)((v & 1) ? -(int)((v + 1) / 2) : (int)(v / 2)); }
int qps_decode_alloc(const uint8_t *file, size_t size, size_t limit, uint8_t **rgba, uint32_t *width, uint32_t *height) {
    if (!rgba || !width || !height) return 0;
    *rgba = NULL; *width = *height = 0;
    if (!file || size < SIMPLE_HEADER+SIMPLE_FOOTER || size > (1u << 28) || memcmp(file, "QPS1", 4) || file[15] || qli_read_le32(file+28) || file[14] != 4) return 0;
    uint32_t w = qli_read_le32(file+4), h = qli_read_le32(file+8); size_t pixels = 0;
    unsigned channels = file[12], streams = channels * GROUPS;
    if (!fast_shape(w, h, &pixels) || pixels > limit/4 || (channels != 1 && channels != 3 && channels != 4) || file[13] != (channels >= 3) || qli_read_le32(file+20) != streams) return 0;
    size_t tiles = (size_t)((w+TILE-1)/TILE) * ((h+TILE-1)/TILE), map_size = tiles*channels;
    size_t prefix = SIMPLE_HEADER + map_size + streams*4;
    if (qli_read_le32(file+16) != map_size || prefix > size-SIMPLE_FOOTER || qli_read_le32(file+size-SIMPLE_FOOTER) != qint_crc32(file, size-SIMPLE_FOOTER)) return 0;
    const uint8_t *map = file+SIMPLE_HEADER;
    size_t counts[MAX_STREAMS] = {0}, cursor[MAX_STREAMS] = {0};
    for (unsigned c = 0; c < channels; ++c) {
        size_t tile = 0;
        for (uint32_t ty = 0; ty < h; ty += TILE) for (uint32_t tx = 0; tx < w; tx += TILE, ++tile) {
            unsigned code = map[c*tiles+tile]; if (code >= 16) return 0;
            counts[c*GROUPS+(code>>2)] += tile_pixels(tx, ty, w, h);
        }
    }
    uint8_t *pool = qli_malloc(pixels*channels), *planes = qli_malloc(pixels*channels), *out = qli_malloc(pixels*4); int ok = 0;
    if (!pool || !planes || !out) goto done;
    size_t offset = prefix, count = 0;
    for (unsigned s = 0; s < streams; ++s) {
        cursor[s] = count;
        size_t n = qli_read_le32(file+SIMPLE_HEADER+map_size+s*4), got = 0;
        if (n > size-SIMPLE_FOOTER-offset || !qlic_rans_decode(file+offset, n, pool+count, counts[s], &got) || got != counts[s]) goto done;
        count += counts[s]; offset += n;
    }
    if (offset != size-SIMPLE_FOOTER || count != pixels*channels) goto done;
    for (unsigned c = 0; c < channels; ++c) {
        uint8_t *plane = planes + c*pixels; size_t tile = 0;
        for (uint32_t ty = 0; ty < h; ty += TILE) for (uint32_t tx = 0; tx < w; tx += TILE, ++tile) {
            unsigned code = map[c*tiles+tile], mode = code&3, s = c*GROUPS+(code>>2);
            for (uint32_t y = ty; y < h && y < ty+TILE; ++y) for (uint32_t x = tx; x < w && x < tx+TILE; ++x) {
                size_t pos = (size_t)y*w+x;
                plane[pos] = (uint8_t)(undo_symbol(pool[cursor[s]++]) + plane_predict(plane, pos, x, y, w, mode));
            }
        }
    }
    for (size_t i = 0; i < pixels; ++i) {
        if (channels == 1) { out[i*4] = out[i*4+1] = out[i*4+2] = planes[i]; out[i*4+3] = 255; }
        else {
            out[i*4] = (uint8_t)(planes[i] + planes[pixels+i]); out[i*4+1] = planes[pixels+i];
            out[i*4+2] = (uint8_t)(planes[pixels*2+i] + planes[pixels+i]); out[i*4+3] = channels == 4 ? planes[pixels*3+i] : 255;
        }
    }
    if (qli_read_le32(file+24) != qint_crc32(out, pixels*4)) goto done;
    *rgba = out; out = NULL; *width = w; *height = h; ok = 1;
done:
    qli_free(pool); qli_free(planes); qli_free(out); return ok;
}

int qwi_value_info(const uint8_t *,size_t,size_t *,size_t *);
int qwi_value_decode(const uint8_t *,size_t,uint8_t *,size_t,size_t,size_t *,size_t *);

static int uniform_expand(unsigned state,size_t count,const uint8_t *values,
                          size_t *cursor,size_t end,uint8_t *steps){
    if(state>2 || *cursor>end)return 0;
    if(!state){memset(steps,0,count);return 1;}
    if(count>end-*cursor)return 0;
    const uint8_t *v=values+*cursor;
    if(state==1){for(size_t i=0;i<count;++i){if(v[i]>=127)return 0;steps[i]=(uint8_t)(v[i]+1);}}
    else {for(size_t i=0;i<count;++i){if(v[i]>127)return 0;steps[i]=(uint8_t)(-(int)(v[i]+1));}}
    *cursor+=count;return 1;
}

static void zero_reduce_correction(unsigned channel,unsigned mode,unsigned green,
                                   unsigned red,uint8_t coef[3]) {
    if((green&128) && (green&15)==mode) {
        if(channel==0)coef[0]=32;
        else if(channel==2)coef[1]=32;
    }
    if(channel==2 && (red&128) && (red&15)==mode)coef[2]=32;
}

static int zero_reconstruct_tile(uint8_t *plane,uint32_t width,uint32_t tx,uint32_t ty,
                                  unsigned tw,unsigned th,unsigned mode) {
    if(mode>2)return 0;
    if(mode==0 || (mode==2 && !ty)) {
        for(unsigned y=0;y<th;++y)memset(plane+(size_t)(ty+y)*width+tx,0,tw);
    } else if(mode==1) {
        for(unsigned y=0;y<th;++y) {
            uint8_t *row=plane+(size_t)(ty+y)*width+tx;
            memset(row,tx?row[-1]:0,tw);
        }
    } else {
        const uint8_t *top=plane+(size_t)(ty-1)*width+tx;
        for(unsigned y=0;y<th;++y)memcpy(plane+(size_t)(ty+y)*width+tx,top,tw);
    }
    return 1;
}

static int grouped_scalar_tile(const uint8_t *tags,unsigned tw,unsigned th,size_t stride,
    unsigned channel,const uint8_t *values,size_t *cursor,size_t end,uint8_t *plane) {
    for(unsigned y=0;y<th;++y)for(unsigned x=0;x<tw;++x) {
        unsigned state=(*tags++>>(2*channel))&3;int value=0;
        if(state) {
            if(state==3 || *cursor>=end)return 0;unsigned m=values[(*cursor)++];
            if(m>127 || (state==1 && m==127))return 0;value=state==1?(int)m+1:-(int)m-1;
        }
        plane[y*stride+x]=(uint8_t)value;
    }
    return 1;
}

typedef struct { int rg,bg,br; uint8_t rbias,bbias; } QccRule;
enum { STRIP_HEADER=44,STRIP_FILE_CAP=1u<<28 };

static inline int tile_floor16(int v) { return v>=0?v/16:-((-v+15)/16); }
static inline int clamp8(int v) { return v<0?0:v>255?255:v; }
static inline uint8_t tile_predict(unsigned mode,int l,int u,int c) {
    int v=l+u-c,lo=l<u?l:u,hi=l>u?l:u;
    switch(mode) {
    case 0:return 0;case 1:return (uint8_t)l;case 2:return (uint8_t)u;
    case 3:return (uint8_t)(v<lo?lo:v>hi?hi:v);
    case 4:return (uint8_t)((l+u)/2);case 5:return (uint8_t)clamp8(v);case 6:return (uint8_t)v;
    case 7:{int a=abs(v-l),b=abs(v-u),d=abs(v-c);return (uint8_t)(a<=b&&a<=d?l:b<=d?u:c);}
    case 8:return (uint8_t)((3*l+u)/4);case 9:return (uint8_t)((l+3*u)/4);
    case 10:return (uint8_t)((2*l+u+c)/4);case 11:return (uint8_t)((l+2*u+c)/4);
    case 12:return (uint8_t)((l+c)/2);case 13:return (uint8_t)((u+c)/2);case 14:return (uint8_t)c;
    default:{int n=3*(l+u)-2*c;return (uint8_t)clamp8(n>=0?n/4:-((-n+3)/4));}
    }
}
static inline uint8_t plane_prediction(const uint8_t *p,size_t i,uint32_t x,uint32_t y,uint32_t w,unsigned m) {
    return tile_predict(m,x?p[i-1]:0,y?p[i-w]:0,x&&y?p[i-w-1]:0);
}
static inline int signed_error(uint8_t value,uint8_t prediction) {
    unsigned v=(uint8_t)(value-prediction);return v<128?(int)v:(int)v-256;
}
static inline size_t region_count(uint32_t w,uint32_t h,unsigned log) {
    if(!log)return 1;size_t side=(size_t)1<<log;
    return ((w+side-1)/side)*((h+side-1)/side);
}
static inline size_t region_at(uint32_t w,uint32_t x,uint32_t y,unsigned log) {
    return !log?0:(size_t)(y>>log)*((w+((1u<<log)-1))>>log)+(x>>log);
}
static inline size_t tile_at(uint32_t w,uint32_t x,uint32_t y) { return (size_t)(y>>4)*((w+15)>>4)+(x>>4); }
static inline int correction(const uint8_t *planes,size_t pixels,uint32_t w,uint32_t x,uint32_t y,
                      unsigned c,unsigned mode,const uint8_t *coef) {
    if(c!=0 && c!=2)return 0;size_t i=(size_t)y*w+x;
    int a=(int)coef[c==0?0:1]-32,b=c==2?(int)coef[2]-32:0,total=0;
    if(a) {
        const uint8_t *g=planes+pixels;int ge=signed_error(g[i],plane_prediction(g,i,x,y,w,mode));
        total=tile_floor16(a*ge);
    }
    if(b) {
        const uint8_t *r=planes;int re=signed_error(r[i],plane_prediction(r,i,x,y,w,mode));
        total+=tile_floor16(b*re);
    }
    return total;
}

#define RECONSTRUCT(M) case M: { \
    size_t k=0; \
    for(uint32_t row=0;row<th;++row) { \
        uint32_t y=ty+row;size_t base=(size_t)y*w+tx; \
        const uint8_t *upper=y?p+base-w:NULL; \
        uint8_t left=tx?p[base-1]:0,corner=tx&&y?upper[-1]:0; \
        for(uint32_t col=0;col<tw;++col) { \
            uint8_t up=upper?upper[col]:0; \
            uint8_t v=(uint8_t)(steps[k++]+tile_predict(M,left,up,corner)); \
            p[base+col]=v;left=v;corner=up; \
        } \
    } \
    break; \
}
static void reconstruct_tile(uint8_t *p,uint32_t w,uint32_t tx,uint32_t ty,
    uint32_t tw,uint32_t th,unsigned mode,const uint8_t *steps) {
    switch(mode) {
        RECONSTRUCT(0) RECONSTRUCT(1) RECONSTRUCT(2) RECONSTRUCT(3)
        RECONSTRUCT(4) RECONSTRUCT(5) RECONSTRUCT(6) RECONSTRUCT(7)
        RECONSTRUCT(8) RECONSTRUCT(9) RECONSTRUCT(10) RECONSTRUCT(11)
        RECONSTRUCT(12) RECONSTRUCT(13) RECONSTRUCT(14) RECONSTRUCT(15)
    }
}
#undef RECONSTRUCT

#ifdef QRE_VERIFY_CACHE
static int verify_cache(const uint8_t *p,uint32_t w,uint32_t tx,uint32_t tw,uint32_t th,unsigned mode,const uint8_t *steps) {
    size_t k=0;
    for(uint32_t y=1;y<=th;++y)for(uint32_t x=tx;x<tx+tw;++x,++k) {
        size_t i=(size_t)y*w+x;
        if(steps[k]!=(uint8_t)signed_error(p[i],plane_prediction(p,i,x,y,w,mode)))return 0;
    }
    return 1;
}
#endif
static void strip_add_correction(const uint8_t *plane, uint32_t stride, uint32_t tx,
                                 uint32_t width, uint32_t height, unsigned mode,
                                 int coefficient, const uint8_t *cached, uint8_t *steps)
{
    const size_t count = (size_t)width * height;
    if (cached) {
        for (size_t i = 0; i < count; ++i)
            steps[i] = (uint8_t)(steps[i] + tile_floor16(coefficient * signed_error(cached[i], 0)));
        return;
    }
    size_t at = 0;
#define CORRECT_TILE(M) case M: \
    for (uint32_t y = 0; y < height; ++y) { \
        const uint8_t *row = plane + (size_t)(y + 1) * stride + tx; \
        const uint8_t *north = row - stride; \
        unsigned west = tx ? row[-1] : 0, corner = tx ? north[-1] : 0; \
        for (uint32_t x = 0; x < width; ++x, ++at) { \
            const unsigned above = north[x], value = row[x]; \
            const int error = signed_error((uint8_t)value, tile_predict(M, west, above, corner)); \
            steps[at] = (uint8_t)(steps[at] + tile_floor16(coefficient * error)); \
            west = value; \
            corner = above; \
        } \
    } \
    break;
    switch (mode) {
        CORRECT_TILE(0) CORRECT_TILE(1) CORRECT_TILE(2) CORRECT_TILE(3)
        CORRECT_TILE(4) CORRECT_TILE(5) CORRECT_TILE(6) CORRECT_TILE(7)
        CORRECT_TILE(8) CORRECT_TILE(9) CORRECT_TILE(10) CORRECT_TILE(11)
        CORRECT_TILE(12) CORRECT_TILE(13) CORRECT_TILE(14) CORRECT_TILE(15)
    }
#undef CORRECT_TILE
}

static void reuse_correction(const uint8_t *planes,size_t history,uint32_t w,
    uint32_t tx,uint32_t tw,uint32_t th,unsigned c,unsigned mode,const uint8_t *coef,
    unsigned green_mode,unsigned red_mode,const uint8_t *green,const uint8_t *red,uint8_t *steps)
{
    int a = (int)coef[c == 0 ? 0 : 1] - 32, b = c == 2 ? (int)coef[2] - 32 : 0;
    if (a)
        strip_add_correction(planes + history, w, tx, tw, th, mode, a,
                             green_mode == mode ? green : NULL, steps);
    if (b)
        strip_add_correction(planes, w, tx, tw, th, mode, b, red_mode == mode ? red : NULL, steps);
}
#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42 && (defined(_M_X64) || defined(__x86_64__))
#define QLI_STRIP_X86 1

static inline __m128i strip_load16(const void *source)
{
    __m128i v;
    memcpy(&v, source, sizeof(v));
    return v;
}

static inline void strip_store16(void *destination, __m128i value)
{
    memcpy(destination, &value, sizeof(value));
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("ssse3,sse4.1")))
#endif
static int strip_expand_rows(const uint8_t *tags,unsigned rows,unsigned channel,
                             const uint8_t *values,size_t *cursor,size_t end,uint8_t *steps)
{
    const __m128i zero=_mm_setzero_si128(), one=_mm_set1_epi8(1);
    const __m128i shift=_mm_cvtsi32_si128((int)(2*channel));
    const __m128i three=_mm_set1_epi8(3);
    const __m128i sign=_mm_setr_epi8(0,1,-1,0,0,0,0,0,0,0,0,0,0,0,0,0);
    size_t at=*cursor;
    if(at>end)return 0;
    for(unsigned y=0;y<rows;++y,tags+=16,steps+=16) {
        const __m128i state=_mm_and_si128(_mm_srl_epi16(strip_load16(tags),shift),three);
        if(_mm_testz_si128(state,state)) {
            strip_store16(steps,zero);
            continue;
        }
        if(_mm_movemask_epi8(_mm_cmpeq_epi8(state,three)))return 0;

        __m128i rank=_mm_min_epu8(state,one);
        rank=_mm_add_epi8(rank,_mm_slli_si128(rank,1));
        rank=_mm_add_epi8(rank,_mm_slli_si128(rank,2));
        rank=_mm_add_epi8(rank,_mm_slli_si128(rank,4));
        rank=_mm_add_epi8(rank,_mm_slli_si128(rank,8));
        const unsigned count=(unsigned)_mm_extract_epi8(rank,15);
        if(count>end-at)return 0;
        __m128i packed;
        if(end-at>=16) packed=strip_load16(values+at);
        else {
            uint8_t tail[16]={0};
            memcpy(tail,values+at,count);
            packed=strip_load16(tail);
        }
        const __m128i indexes=_mm_or_si128(_mm_sub_epi8(rank,one),_mm_cmpeq_epi8(state,zero));
        const __m128i magnitude=_mm_shuffle_epi8(packed,indexes);
        const __m128i positive=_mm_and_si128(_mm_cmpeq_epi8(state,one),one);
        if(_mm_movemask_epi8(_mm_adds_epu8(magnitude,positive)))return 0;
        strip_store16(steps,_mm_sign_epi8(_mm_add_epi8(magnitude,one),_mm_shuffle_epi8(sign,state)));
        at+=count;
    }
    *cursor=at;
    return 1;
}

static QLI_INLINE __m128i strip_select(__m128i a, __m128i b, __m128i mask)
{
    return _mm_or_si128(_mm_andnot_si128(mask,a), _mm_and_si128(mask,b));
}

static inline __m128i strip_prepend(__m128i row, uint8_t first)
{
    return _mm_or_si128(_mm_slli_si128(row,1), _mm_cvtsi32_si128(first));
}

static void wave_transpose(__m128i v[16]){
    __m128i a[16],b[16];
    for(unsigned i=0;i<16;i+=2){
        a[i]=_mm_unpacklo_epi8(v[i],v[i+1]);
        a[i+1]=_mm_unpackhi_epi8(v[i],v[i+1]);
    }
    for(unsigned j=0;j<16;j+=4)for(unsigned i=0;i<2;++i){
        b[j+2*i]=_mm_unpacklo_epi16(a[j+i],a[j+2+i]);
        b[j+2*i+1]=_mm_unpackhi_epi16(a[j+i],a[j+2+i]);
    }
    for(unsigned j=0;j<16;j+=8)for(unsigned i=0;i<4;++i){
        a[j+2*i]=_mm_unpacklo_epi32(b[j+i],b[j+4+i]);
        a[j+2*i+1]=_mm_unpackhi_epi32(b[j+i],b[j+4+i]);
    }
    for(unsigned i=0;i<8;++i){
        v[2*i]=_mm_unpacklo_epi64(a[i],a[8+i]);
        v[2*i+1]=_mm_unpackhi_epi64(a[i],a[8+i]);
    }
}

static QLI_INLINE __m128i strip_abs_words(__m128i value)
{
    return _mm_max_epi16(value, _mm_sub_epi16(_mm_setzero_si128(), value));
}

static QLI_INLINE __m128i strip_predict_words(unsigned mode, __m128i w, __m128i n, __m128i c)
{
    const __m128i wn = _mm_add_epi16(w,n);
    switch (mode) {
    case 5: return _mm_sub_epi16(wn,c);
    case 7: {
        const __m128i dw = strip_abs_words(_mm_sub_epi16(n,c));
        const __m128i dn = strip_abs_words(_mm_sub_epi16(w,c));
        const __m128i dc = strip_abs_words(_mm_sub_epi16(wn,_mm_slli_epi16(c,1)));
        const __m128i best = strip_select(w,n,_mm_cmpgt_epi16(dw,dn));
        return strip_select(best,c,_mm_cmpgt_epi16(_mm_min_epi16(dw,dn),dc));
    }
    case 8: return _mm_srli_epi16(_mm_add_epi16(wn,_mm_slli_epi16(w,1)),2);
    case 9: return _mm_srli_epi16(_mm_add_epi16(wn,_mm_slli_epi16(n,1)),2);
    case 10: return _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(wn,w),c),2);
    case 11: return _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(wn,n),c),2);
    default: {
        const __m128i three = _mm_add_epi16(wn,_mm_slli_epi16(wn,1));
        return _mm_srai_epi16(_mm_sub_epi16(three,_mm_slli_epi16(c,1)),2);
    }
    }
}

static QLI_INLINE __m128i wave_prediction(unsigned mode,__m128i west,__m128i north,__m128i corner){
    if(mode==3){
        __m128i lo=_mm_min_epu8(west,north),hi=_mm_max_epu8(west,north);
        return _mm_add_epi8(lo,_mm_min_epu8(_mm_subs_epu8(hi,corner),_mm_sub_epi8(hi,lo)));
    }
    if(mode==4 || mode==12){
        __m128i other=mode==4?north:corner;
        return _mm_sub_epi8(_mm_avg_epu8(west,other),_mm_and_si128(_mm_xor_si128(west,other),_mm_set1_epi8(1)));
    }
    if(mode==6)return _mm_sub_epi8(_mm_add_epi8(west,north),corner);

    const __m128i zero = _mm_setzero_si128();
    const __m128i lo = strip_predict_words(mode, _mm_unpacklo_epi8(west,zero),
        _mm_unpacklo_epi8(north,zero), _mm_unpacklo_epi8(corner,zero));
    const __m128i hi = strip_predict_words(mode, _mm_unpackhi_epi8(west,zero),
        _mm_unpackhi_epi8(north,zero), _mm_unpackhi_epi8(corner,zero));
    return _mm_packus_epi16(lo,hi);
}

static void reconstruct_tile_wave(uint8_t *plane,uint32_t w,uint32_t tx,uint32_t ty,
                                  unsigned mode,const uint8_t steps[256]){
    uint8_t *out=plane+(size_t)ty*w+tx;
    const uint8_t *top=ty?out-w:NULL;
    uint8_t buffer[16][32]={{0}},edge[16]={0};
    for(unsigned y=0;y<16;++y){
        strip_store16((buffer[y]+y),strip_load16((steps+16*y)));
        if(tx)edge[y]=(out+(size_t)y*w)[-1];
    }
    __m128i waves[32];
    for(unsigned block=0;block<32;block+=16){
        for(unsigned y=0;y<16;++y)waves[block+y]=strip_load16((buffer[y]+block));
        wave_transpose(waves+block);
    }
    const __m128i left=strip_load16(edge);
    const __m128i northwest=strip_prepend(left, top&&tx ? top[-1] : 0);
    const __m128i lanes=_mm_setr_epi8(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
    __m128i last=_mm_setzero_si128(),older=last;
#define WAVE_CASE(M) case M: \
    _Pragma("clang loop unroll(disable)") \
    for(unsigned t=0;t<16;++t){ \
        __m128i boundary=_mm_cmpeq_epi8(lanes,_mm_set1_epi8((char)t)); \
        __m128i wv=strip_select(last,left,boundary); \
        __m128i nv=strip_prepend(last, top ? top[t] : 0); \
        __m128i cv=strip_prepend(older, top&&t ? top[t-1] : 0); \
        cv=strip_select(cv,northwest,boundary); \
        older=last;last=_mm_add_epi8(wave_prediction(M,wv,nv,cv),waves[t]);waves[t]=last; \
    } \
    _Pragma("clang loop unroll(disable)") \
    for(unsigned t=16;t<31;++t){ \
        __m128i nv=_mm_slli_si128(last,1),cv=_mm_slli_si128(older,1); \
        __m128i v=_mm_add_epi8(wave_prediction(M,last,nv,cv),waves[t]); \
        older=last;last=v;waves[t]=v; \
    } \
    break;
    switch(mode){
        WAVE_CASE(3) WAVE_CASE(4) WAVE_CASE(5) WAVE_CASE(6) WAVE_CASE(7)
        WAVE_CASE(8) WAVE_CASE(9) WAVE_CASE(10) WAVE_CASE(11) WAVE_CASE(12) WAVE_CASE(15)
    }
#undef WAVE_CASE
    for(unsigned block=0;block<32;block+=16){
        wave_transpose(waves+block);
        for(unsigned y=0;y<16;++y)strip_store16((buffer[y]+block),waves[block+y]);
    }
    for(unsigned y=0;y<16;++y)strip_store16((out+(size_t)y*w),
                                               strip_load16((buffer[y]+y)));
}

static inline __m128i reference_error_row(const uint8_t *p, uint32_t stride,
                                          unsigned left_edge, unsigned mode){
    __m128i sample=strip_load16(p);
    if(mode==0)return sample;
    __m128i north=strip_load16((p-stride));
    __m128i west=left_edge?_mm_slli_si128(sample,1):strip_load16((p-1));
    __m128i corner=left_edge?_mm_slli_si128(north,1):strip_load16((p-stride-1));
    __m128i prediction;
    switch(mode){
    case 1:prediction=west;break;
    case 2:prediction=north;break;
    case 13:
        prediction=_mm_sub_epi8(_mm_avg_epu8(north,corner),
            _mm_and_si128(_mm_xor_si128(north,corner),_mm_set1_epi8(1)));
        break;
    case 14:prediction=corner;break;
    default:prediction=wave_prediction(mode,west,north,corner);break;
    }
    return _mm_sub_epi8(sample,prediction);
}

static void row_vector_correction(const uint8_t *planes, size_t history,
    uint32_t w, uint32_t tx, uint32_t th, unsigned c, unsigned mode, const uint8_t *coef,
    unsigned green_mode, unsigned red_mode, const uint8_t *green, const uint8_t *red, uint8_t *steps)
{
    const int a = (int)coef[c==0?0:1]-32, b = c==2 ? (int)coef[2]-32 : 0;
    const __m128i av = _mm_set1_epi16((short)a), bv = _mm_set1_epi16((short)b);
    const __m128i mask = _mm_set1_epi16(255), zero = _mm_setzero_si128();
    const uint8_t *gp = planes+history+w+tx, *rp = planes+w+tx;
    for (uint32_t y=0; y<th; ++y) {
        const size_t offset = (size_t)y*16;
        __m128i lo = zero, hi = zero;
        if (a) {
            const __m128i g = green_mode==mode ? strip_load16(green+offset)
                                               : reference_error_row(gp+(size_t)y*w,w,!tx,mode);
            const __m128i sign = _mm_cmpgt_epi8(zero,g);
            lo = _mm_srai_epi16(_mm_mullo_epi16(_mm_unpacklo_epi8(g,sign),av),4);
            hi = _mm_srai_epi16(_mm_mullo_epi16(_mm_unpackhi_epi8(g,sign),av),4);
        }
        if (b) {
            const __m128i r = red_mode==mode ? strip_load16(red+offset)
                                             : reference_error_row(rp+(size_t)y*w,w,!tx,mode);
            const __m128i sign = _mm_cmpgt_epi8(zero,r);
            lo = _mm_add_epi16(lo,_mm_srai_epi16(_mm_mullo_epi16(_mm_unpacklo_epi8(r,sign),bv),4));
            hi = _mm_add_epi16(hi,_mm_srai_epi16(_mm_mullo_epi16(_mm_unpackhi_epi8(r,sign),bv),4));
        }
        const __m128i add = _mm_packus_epi16(_mm_and_si128(lo,mask),_mm_and_si128(hi,mask));
        strip_store16(steps+offset,_mm_add_epi8(strip_load16(steps+offset),add));
    }
}
#else
#define QLI_STRIP_X86 0
#endif

static void strip_output(const uint8_t *r,const uint8_t *g,const uint8_t *b,const uint8_t *a,
                         uint8_t *out,size_t n,unsigned channels,int direct_gray,int native,
                         unsigned output_channels){
    size_t i=0;
#if QLI_STRIP_X86
    if (native && output_channels==4) {
        const __m128i bias = _mm_set1_epi8((char)128), opaque = _mm_set1_epi8(-1);
        for (; n-i >= 16; i += 16) {
            const __m128i green = strip_load16(g+i), centered = _mm_add_epi8(green, bias);
            const __m128i red = channels==1 ? (direct_gray ? green : centered)
                                             : _mm_add_epi8(strip_load16(r+i), centered);
            const __m128i blue = channels==1 ? red : _mm_add_epi8(strip_load16(b+i), centered);
            const __m128i alpha = channels==4 ? strip_load16(a+i) : opaque;
            const __m128i rg0 = _mm_unpacklo_epi8(red,green), rg1 = _mm_unpackhi_epi8(red,green);
            const __m128i ba0 = _mm_unpacklo_epi8(blue,alpha), ba1 = _mm_unpackhi_epi8(blue,alpha);
            strip_store16(out+4*i,    _mm_unpacklo_epi16(rg0,ba0));
            strip_store16(out+4*i+16, _mm_unpackhi_epi16(rg0,ba0));
            strip_store16(out+4*i+32, _mm_unpacklo_epi16(rg1,ba1));
            strip_store16(out+4*i+48, _mm_unpackhi_epi16(rg1,ba1));
        }
    }
#else
    (void)native;
#endif
    for(;i<n;++i){unsigned green=g[i];uint8_t *pixel=out+output_channels*i;
        pixel[1]=(uint8_t)green;
        pixel[0]=(uint8_t)(channels==1?(int)green-(direct_gray?0:128):(int)r[i]+(int)green-128);
        pixel[2]=(uint8_t)(channels==1?(int)green-(direct_gray?0:128):(int)b[i]+(int)green-128);
        if(output_channels==4)pixel[3]=channels==4?a[i]:255;}
}

typedef struct { size_t used,peak,limit; } Heap;
static uint8_t *allocate(Heap *heap,size_t bytes) {
    if(bytes>heap->limit-heap->used)return NULL;
    uint8_t *p=qli_malloc(bytes);if(p){heap->used+=bytes;if(heap->used>heap->peak)heap->peak=heap->used;}return p;
}

enum { QNL_SHIFT=10,QNL_SLOTS=1<<QNL_SHIFT,QNL_LOW=1<<23,QNL_LANES=4,QNL_TAG_CODES=171 };

static int qnl_record(const uint8_t **cursor,const uint8_t *end,uint8_t *out,size_t count)
{
    const uint8_t *p=*cursor;
    if((size_t)(end-p)<5 || p[0]>1) return 0;
    size_t n=qli_read_le32(p+1),got=0;
    if(n>(size_t)(end-p)-5) return 0;
    if(p[0]) {
        if(!n || n>=count || !qbs_decode(p+5,n,out,count,&got,QBS_BYTE_TABLE) || got!=count)
            return 0;
    } else {
        if(n!=count) return 0;
        memcpy(out,p+5,n);
    }
    *cursor=p+5+n;
    return 1;
}

static int qnl_full_tiles(const uint8_t **cursor,const uint8_t *end,uint32_t state[4],
                          const uint32_t *table,const uint32_t *route,
                          uint8_t *out)
{
    const uint8_t *p=*cursor;
    uint8_t north[16][4]={{0}};
    for(unsigned y=0;y<16;++y) {
        uint8_t west[4]={0};
        for(unsigned x=0;x<16;++x) {
            uint32_t next[4];
            #pragma clang loop unroll(full)
            for(unsigned lane=0;lane<4;++lane) {
                const unsigned n=north[x][lane],w=west[lane];
                const unsigned key=4*n+2u*(w!=n)+(w!=0);
                const uint32_t s=state[lane];
                const uint32_t entry=table[route[key]|(s&(QNL_SLOTS-1))];
                const uint8_t tag=(uint8_t)entry;
                north[x][lane]=west[lane]=tag;
                out[256*lane+16*y+x]=tag;
                next[lane]=(s>>QNL_SHIFT)*(entry>>18)+((entry>>8)&(QNL_SLOTS-1));
            }
            if((size_t)(end-p)>=8) {
                unsigned offset=0;
                #pragma clang loop unroll(full)
                for(unsigned lane=0;lane<4;++lane) {
                    const uint32_t s=next[lane];
                    const unsigned bytes=(s<QNL_LOW)+(s<(QNL_LOW>>8));
                    const uint32_t word=((uint32_t)p[offset]<<8)|p[offset+1];
                    state[lane]=(s<<(8*bytes))|(word>>(16-8*bytes));
                    offset+=bytes;
                }
                p+=offset;
            } else {
                for(unsigned lane=0;lane<4;++lane) {
                    uint32_t s=next[lane];
                    while(s<QNL_LOW) {
                        if(p==end || !s) return 0;
                        s=(s<<8)|*p++;
                    }
                    state[lane]=s;
                }
            }
        }
    }
    *cursor=p;
    return 1;
}

static int qnl_tags(Heap *heap,const uint8_t *p,size_t n,uint16_t *dims,size_t slots,
                    unsigned alphabet,uint8_t *out,size_t count)
{
    if(n<6 || p[4]!=1 || (alphabet!=3 && alphabet!=27 && alphabet!=81)) return 0;
    size_t tiles=0,pixels=0;
    for(size_t k=0;k<slots;++k) if(dims[k]) {
        unsigned w=dims[k]&255,h=dims[k]>>8;
        if(!w || w>16 || !h || h>16) return 0;
        dims[tiles++]=dims[k];
        pixels+=w*h;
    }
    if(!tiles || pixels!=count || n>2*count+65536) return 0;
    unsigned keys=3*alphabet,chosen=p[5],models=chosen+1;
    if(chosen>keys || n<6+chosen+16) return 0;
    uint32_t route[4*QNL_TAG_CODES]={0};
    for(unsigned k=0;k<chosen;++k) {
        unsigned id=p[6+k];
        if(id>=keys || (k && id<=p[5+k])) return 0;
        unsigned tag=packed_tag(id/3), relation=id%3;
        unsigned offset=(k+1)*QNL_SLOTS;
        route[4*tag+1+relation]=offset;
        if(!relation) route[4*tag]=offset;
    }
    size_t fields=(size_t)models*alphabet;
    size_t table_bytes=(size_t)models*QNL_SLOTS*sizeof(uint32_t);
    size_t scratch=table_bytes+2*fields;
    uint8_t *memory=allocate(heap,scratch);
    if(!memory) return 0;
    uint32_t *table=(uint32_t *)memory;
    uint8_t *lo=memory+table_bytes,*hi=lo+fields;
    const uint8_t *end=p+n;
    p+=6+chosen;
    int ok=0;
    if(!qnl_record(&p,end,lo,fields) || !qnl_record(&p,end,hi,fields) ||
       (size_t)(end-p)<16 || (size_t)(end-p)>16+2*count) goto done;
    for(unsigned k=0;k<models;++k) {
        unsigned total=0;
        for(unsigned v=0;v<alphabet;++v) {
            size_t pos=(size_t)k*alphabet+v;
            unsigned f=lo[pos]|((unsigned)hi[pos]<<8);
            if(f>QNL_SLOTS-total) goto done;
            unsigned tag=packed_tag(v);
            for(unsigned d=0;d<f;++d)
                table[(size_t)k*QNL_SLOTS+total+d]=tag|(d<<8)|(f<<18);
            total+=f;
        }
        if(total!=QNL_SLOTS) goto done;
    }
    uint32_t state[QNL_LANES];
    for(unsigned lane=0;lane<QNL_LANES;++lane) {
        state[lane]=qli_read_le32(p+4*lane);
        if(state[lane]<QNL_LOW || state[lane]>=(UINT32_C(1)<<31)) goto done;
    }
    p+=16;
    size_t offset=0;
    for(size_t b=0;b<tiles;b+=QNL_LANES) {
        if(tiles-b>=4 && dims[b]==0x1010 && dims[b+1]==0x1010 &&
           dims[b+2]==0x1010 && dims[b+3]==0x1010) {
            if(!qnl_full_tiles(&p,end,state,table,route,out+offset)) goto done;
            offset+=1024;
            continue;
        }
        unsigned width[QNL_LANES]={0},height[QNL_LANES]={0},maxw=0,maxh=0;
        uint8_t north[QNL_LANES][16]={{0}},*dst[QNL_LANES]={0};
        unsigned lanes=(unsigned)(tiles-b<QNL_LANES?tiles-b:QNL_LANES);
        for(unsigned lane=0;lane<lanes;++lane) {
            width[lane]=dims[b+lane]&255;height[lane]=dims[b+lane]>>8;
            if(width[lane]>maxw) maxw=width[lane];
            if(height[lane]>maxh) maxh=height[lane];
            dst[lane]=out+offset;offset+=width[lane]*height[lane];
        }
        for(unsigned y=0;y<maxh;++y) {
            uint8_t west[QNL_LANES]={0};
            for(unsigned x=0;x<maxw;++x) for(unsigned lane=0;lane<lanes;++lane) {
                if(x>=width[lane] || y>=height[lane]) continue;
                unsigned a=north[lane][x],w=west[lane];
                unsigned key=4*a+2u*(w!=a)+(w!=0);
                uint32_t s=state[lane],entry=table[route[key]|(s&(QNL_SLOTS-1))];
                uint8_t tag=(uint8_t)entry;
                north[lane][x]=west[lane]=tag;
                *dst[lane]++=tag;
                s=(s>>QNL_SHIFT)*(entry>>18)+((entry>>8)&(QNL_SLOTS-1));
                while(s<QNL_LOW) {
                    if(p==end || !s) goto done;
                    s=(s<<8)|*p++;
                }
                state[lane]=s;
            }
        }
    }
    if(p!=end || offset!=count) goto done;
    for(unsigned lane=0;lane<QNL_LANES;++lane) if(state[lane]!=QNL_LOW) goto done;
    ok=1;
done:
    qli_free(memory);heap->used-=scratch;
    return ok;
}
static unsigned key_for(const uint8_t *map,size_t tiles,size_t tile,unsigned channels,unsigned policy) {
    if(!policy)return 0;if(policy==1)return (map[(channels==1?0:1)*tiles+tile]>>4)&7;
    unsigned key=0;for(unsigned c=0;c<channels;++c)key|=((map[c*tiles+tile]>>4)&7)<<(3*c);return key;
}
int strip_decode_limited(const uint8_t *file,size_t size,size_t output_limit,size_t heap_limit,
    uint8_t **rgba,uint32_t *width,uint32_t *height,size_t *peak) {
    if(peak)*peak=0;if(!rgba || !width || !height)return 0;*rgba=NULL;*width=*height=0;
    if(!file || size<STRIP_HEADER+4 || size>STRIP_FILE_CAP || (memcmp(file,"QWI1",4) && memcmp(file,"QWT1",4) && memcmp(file,"QWNL",4)) || (file[13]!=1 && (file[13]!=2 || file[12]!=1)) || file[15]!=16)return 0;
    unsigned output_channels=4;
    if(file[14]==3 && file[12]!=4)output_channels=3;
    if(file[14]!=output_channels)return 0;
    int neighbor=!memcmp(file,"QWNL",4);
    int compact=neighbor || !memcmp(file,"QWT1",4);
    uint32_t w=qli_read_le32(file+4),h=qli_read_le32(file+8),flags=qli_read_le32(file+36);size_t pixels=0;
    unsigned channels=file[12],policy=flags&3,log=qli_read_le32(file+28),streams=qli_read_le32(file+20);
    if(!fast_shape(w,h,&pixels) || pixels>output_limit/output_channels || (channels!=1 && channels!=3 && channels!=4) ||
       (log!=0 && log!=4 && log!=7) || policy>2 || (flags&~UINT32_C(259)))return 0;
    size_t tiles=(size_t)((w+15)/16)*((h+15)/16),maps=tiles*channels,coeffsize=region_count(w,h,log)*3;
    size_t mapstored=qli_read_le32(file+40),offset=STRIP_HEADER;
    if(qli_read_le32(file+16)!=maps || qli_read_le32(file+32)!=coeffsize || mapstored>size-4-offset ||
       (!(flags&256) && mapstored!=maps) || qli_read_le32(file+size-4)!=qint_crc32(file,size-4))return 0;
    size_t mapoffset=offset;offset+=mapstored;if(coeffsize>size-4-offset)return 0;
    const uint8_t *coeffs=file+offset;offset+=coeffsize;
    for(size_t i=0;i<coeffsize;++i)if(coeffs[i]>64 || (channels==1 && coeffs[i]!=32))return 0;
    unsigned capacity=1u<<(channels*3),groups=policy==0?1:8;int16_t lookup[4096];
    for(unsigned i=0;i<4096;++i)lookup[i]=-1;
    if(policy==2) {
        if(size-4-offset<4)return 0;groups=qli_read_le32(file+offset);offset+=4;
        if(!groups || groups>capacity || groups>(size-4-offset)/2)return 0;
        unsigned previous=0;
        for(unsigned g=0;g<groups;++g) {
            unsigned key=file[offset]|((unsigned)file[offset+1]<<8);offset+=2;
            if(key>=capacity || (g && key<=previous))return 0;lookup[key]=(int16_t)g;previous=key;
        }
    } else for(unsigned g=0;g<groups;++g)lookup[g]=(int16_t)g;
    if(streams!=groups+channels*8 || streams>(size-4-offset)/4)return 0;
    const uint8_t *lengths=file+offset;offset+=streams*4;
    Heap heap={0,0,heap_limit};uint8_t *map=NULL,*masks=NULL,*values=NULL,*planes=NULL,*out=NULL,*descriptors=NULL;int ok=0;
    uint16_t *shape_by_tile=NULL;
    uint32_t tile_end[4096]={0},tile_cursor[4096]={0};
    unsigned mask_limit=1;for(unsigned c=0;c<channels;++c)mask_limit*=3;
    size_t mask_total=pixels,mask_allocation=pixels;
    size_t history=(size_t)w*(h<16?h+1:17);
    size_t mask_end[4096]={0},mask_cursor[4096]={0},value_counts[32]={0},value_cursor[32]={0},value_end[32]={0};
    size_t bank_max[32]={0};
    map=allocate(&heap,maps);if(!map)goto done;
    if(flags&256) {size_t got=0;if(!qbs_decode(file+mapoffset,mapstored,map,maps,&got,QBS_BYTE_TABLE) || got!=maps)goto done;}
    else memcpy(map,file+mapoffset,maps);
    for(size_t i=0;i<maps;++i)if(map[i]>=128)goto done;
    for(uint32_t ty=0;ty<h;ty+=16)for(uint32_t tx=0;tx<w;tx+=16) {
        size_t tile=tile_at(w,tx,ty);int group=lookup[key_for(map,tiles,tile,channels,policy)];if(group<0)goto done;
        if(compact)++tile_end[group];
        size_t count=(size_t)(w-tx<16?w-tx:16)*(h-ty<16?h-ty:16);
        mask_end[group]+=count;
        for(unsigned c=0;c<channels;++c)bank_max[c*8+(map[c*tiles+tile]>>4)]+=count;
    }
    if(policy==2)for(unsigned g=0;g<groups;++g)if(!mask_end[g])goto done;
    if(compact) {
        descriptors=allocate(&heap,tiles);if(!descriptors)goto done;
        if(neighbor) {
            shape_by_tile=(uint16_t *)allocate(&heap,tiles*sizeof(*shape_by_tile));
            if(!shape_by_tile) goto done;
        }
        size_t input=offset,base=0;
        for(unsigned g=0;g<groups;++g) {
            size_t bytes=qli_read_le32(lengths+4*g),got=0,count=tile_end[g];
            tile_cursor[g]=(uint32_t)base;
            if(bytes>size-4-input)goto done;
            if(bytes>=4 && !memcmp(file+input,"QTC1",4)) {
                if(bytes<8)goto done;size_t n=qli_read_le32(file+input+4);
                if(n>bytes-8 || !qbs_decode(file+input+8,n,descriptors+base,count,&got,QBS_BYTE_TABLE) || got!=count)goto done;
            } else memset(descriptors+base,(int)mask_limit,count);
            input+=bytes;base+=count;tile_end[g]=(uint32_t)base;mask_end[g]=0;
        }
        if(base!=tiles)goto done;
        mask_total=0;
        for(uint32_t ty=0;ty<h;ty+=16)for(uint32_t tx=0;tx<w;tx+=16) {
            size_t tile=tile_at(w,tx,ty);unsigned g=(unsigned)lookup[key_for(map,tiles,tile,channels,policy)];
            if(tile_cursor[g]>=tile_end[g])goto done;
            unsigned slot=tile_cursor[g]++;
            unsigned d=descriptors[slot];if(d>mask_limit)goto done;
            if(shape_by_tile) shape_by_tile[slot]=0;
            if(d==mask_limit) {
                unsigned tw=w-tx<16?w-tx:16,th=h-ty<16?h-ty:16;
                if(shape_by_tile) shape_by_tile[slot]=(uint16_t)(tw|(th<<8));
                size_t count=(size_t)tw*th;
                mask_end[g]+=count;mask_total+=count;
            }
        }
        for(unsigned g=0;g<groups;++g) {
            if(tile_cursor[g]!=tile_end[g])goto done;
            tile_cursor[g]=g?tile_end[g-1]:0;
        }
    }
    mask_allocation=mask_total?mask_total:1;
    masks=allocate(&heap,mask_allocation);if(!masks)goto done;
    size_t cumulative=0;
    for(unsigned g=0;g<groups;++g) {
        size_t bytes=qli_read_le32(lengths+4*g),got=0;mask_cursor[g]=cumulative;
        if(bytes>size-4-offset)goto done;
        size_t skip=0;
        if(compact && bytes>=4 && !memcmp(file+offset,"QTC1",4)) {
            if(bytes<8)goto done;size_t n=qli_read_le32(file+offset+4);if(n>bytes-8)goto done;
            skip=8+n;
        }
        const uint8_t *payload=file+offset+skip;
        if(neighbor && bytes-skip>=4 && !memcmp(payload,"QNL1",4)) {
            size_t first=g?tile_end[g-1]:0;
            if(!qnl_tags(&heap,payload,bytes-skip,shape_by_tile+first,tile_end[g]-first,
                         mask_limit,masks+cumulative,mask_end[g])) goto done;
            got=mask_end[g];
        } else if(!qbs_decode(payload,bytes-skip,masks+cumulative,mask_end[g],&got,mask_limit)
                  || got!=mask_end[g]) goto done;
        offset+=bytes;cumulative+=got;
        
        mask_end[g]=cumulative;
    }
    if(shape_by_tile) {
        qli_free(shape_by_tile);shape_by_tile=NULL;heap.used-=tiles*sizeof(*shape_by_tile);
    }
    if(cumulative!=mask_total)goto done;
    
    size_t value_input=offset;
    for(unsigned s=0;s<channels*8;++s) {
        size_t bytes=qli_read_le32(lengths+4*(groups+s));
        if(bytes>size-4-value_input)goto done;
        size_t count=0,scratch=0;
        if(!qwi_value_info(file+value_input,bytes,&count,&scratch) || count>bank_max[s])goto done;
        value_counts[s]=count;value_input+=bytes;
    }
    if(value_input!=size-4)goto done;
    size_t nonzero=0;for(unsigned s=0;s<channels*8;++s){value_cursor[s]=nonzero;nonzero+=value_counts[s];value_end[s]=nonzero;}
    values=allocate(&heap,nonzero+16);planes=allocate(&heap,history*channels);
    if(!values || !planes)goto done;
    memset(values+nonzero,0,16);
    for(unsigned s=0;s<channels*8;++s) {
        size_t bytes=qli_read_le32(lengths+4*(groups+s)),got=0;
        if(bytes>size-4-offset)goto done;
        size_t transient=0;
        int decoded=qwi_value_decode(file+offset,bytes,values+value_cursor[s],value_counts[s],heap.limit-heap.used,&got,&transient);
        if(transient>heap.limit-heap.used)goto done;
        if(heap.used+transient>heap.peak)heap.peak=heap.used+transient;
        if(!decoded || got!=value_counts[s])goto done;
        offset+=bytes;
    }
    if(offset!=size-4)goto done;
    out=allocate(&heap,pixels*output_channels);if(!out)goto done;
    for(unsigned c=0;c<channels;++c)memset(planes+c*history,0,w);
    int native=0;
#if QLI_STRIP_X86
    native=(qli_cpu_capabilities() & QLI_CPU_SSE42)!=0;
#endif
    
    const unsigned order[4]={1,0,2,3};
    for(unsigned g=0;g<groups;++g) {
        mask_cursor[g]=g?mask_end[g-1]:0;
        if(compact)tile_cursor[g]=g?tile_end[g-1]:0;
    }
    for(uint32_t ty=0;ty<h;ty+=16) {
      
      if(ty)for(unsigned c=0;c<channels;++c)
          memcpy(planes+c*history,planes+c*history+(size_t)16*w,w);
      for(uint32_t tx=0;tx<w;tx+=16) {
        size_t tile=tile_at(w,tx,ty);
        unsigned group=(unsigned)lookup[key_for(map,tiles,tile,channels,policy)];
        uint32_t tw=w-tx<16?w-tx:16,th=h-ty<16?h-ty:16;size_t count=(size_t)tw*th;
        unsigned descriptor=255;
        if(compact) {
            if(tile_cursor[group]>=tile_end[group])goto done;
            unsigned original=descriptors[tile_cursor[group]++];
            if(original<mask_limit) {
                descriptor=0;for(unsigned c=0;c<channels;++c){descriptor|=(original%3)<<(2*c);original/=3;}
            }
        }
        const uint8_t *tags=NULL;
        if(descriptor==255) {
            if(mask_cursor[group]>mask_end[group] || count>mask_end[group]-mask_cursor[group])goto done;
            tags=masks+mask_cursor[group];mask_cursor[group]+=count;
        }
        
        unsigned seen=descriptor==255?0:descriptor;
        if(tags)for(size_t j=0;j<count;++j)seen|=tags[j];
        for(unsigned c=0;c<channels;++c)if(!((seen>>(2*c))&3))map[c*tiles+tile]|=128;
        
        uint8_t tile_steps[3][256];
        for(unsigned stage=0;stage<channels;++stage) {
            unsigned c=channels==1?0:order[stage];uint8_t *p=planes+c*history;
            unsigned code=map[c*tiles+tile],bank=c*8+((code>>4)&7),mode=code&15;
            const uint8_t *source_coef=coeffs+3*region_at(w,tx,ty,log);
            uint8_t local_coef[3]={source_coef[0],source_coef[1],source_coef[2]};
            if(channels!=1)zero_reduce_correction(c,mode,map[tiles+tile],map[tile],local_coef);
            const uint8_t *coef=local_coef;
            int has_correction=channels!=1 && (c==0?coef[0]!=32:c==2?(coef[1]!=32 || coef[2]!=32):0);
            
            if(has_correction)map[c*tiles+tile]&=127;
            uint8_t *steps=tile_steps[channels==1?2:c==1?0:c==0?1:2];
            if((code&128) && !has_correction && mode<=2) {
                
                if(channels!=1 && (c==1 || c==0))memset(steps,0,count);
                if(!zero_reconstruct_tile(p,w,tx,1,tw,th,mode))goto done;
#ifdef QRE_VERIFY_CACHE
                if(channels!=1 && (c==1 || c==0) && !verify_cache(p,w,tx,tw,th,mode,steps))goto done;
#endif
                continue;
            }
            int expanded=0;
            if(code&128) {
                memset(steps,0,count);
                expanded=1;
            } else if(descriptor!=255)expanded=uniform_expand((descriptor>>(2*c))&3,count,values,&value_cursor[bank],value_end[bank],steps);
#if QLI_STRIP_X86
            else if(native && tw==16)expanded=strip_expand_rows(tags,th,c,values,&value_cursor[bank],value_end[bank],steps);
#endif
            else expanded=grouped_scalar_tile(tags,tw,th,tw,c,values,&value_cursor[bank],value_end[bank],steps);
            if(!expanded)goto done;
            if(has_correction) {
#if QLI_STRIP_X86
                if(native && tw==16)row_vector_correction(planes,history,w,tx,th,c,mode,coef,
                    map[tiles+tile]&15,map[tile]&15,tile_steps[0],tile_steps[1],steps);
                else
#endif
                reuse_correction(planes,history,w,tx,tw,th,c,mode,coef,
                    map[tiles+tile]&15,map[tile]&15,tile_steps[0],tile_steps[1],steps);
            }

#if QLI_STRIP_X86
            if(native && tw==16 && th==16 && mode>=3 && (mode<=12 || mode==15))
                reconstruct_tile_wave(p,w,tx,1,mode,steps);
            else
#endif
            reconstruct_tile(p,w,tx,1,tw,th,mode,steps);
#ifdef QRE_VERIFY_CACHE
            if(channels!=1 && (c==1 || c==0) && !verify_cache(p,w,tx,tw,th,mode,steps))goto done;
#endif
        }
      }
      size_t strip_pixels=(size_t)(h-ty<16?h-ty:16)*w;
      strip_output(channels==1?NULL:planes+w,
          planes+(channels==1?0:history)+w,
          channels==1?NULL:planes+2*history+w,
          channels==4?planes+3*history+w:NULL,
          out+output_channels*(size_t)ty*w,strip_pixels,channels,file[13]==2,native,output_channels);
    }
    for(unsigned g=0;g<groups;++g)if(mask_cursor[g]!=mask_end[g] || (compact && tile_cursor[g]!=tile_end[g]))goto done;
    for(unsigned s=0;s<channels*8;++s)if(value_cursor[s]!=value_end[s])goto done;
    qli_free(values);values=NULL;heap.used-=nonzero+16;
    qli_free(masks);masks=NULL;heap.used-=mask_allocation;
    if(descriptors){qli_free(descriptors);descriptors=NULL;heap.used-=tiles;}
    qli_free(planes);planes=NULL;heap.used-=history*channels;
    if(qint_crc32(out,pixels*output_channels)!=qli_read_le32(file+24))goto done;
    *rgba=out;*width=w;*height=h;out=NULL;ok=1;
done:
    if(peak)*peak=heap.peak;qli_free(shape_by_tile);qli_free(descriptors);qli_free(map);qli_free(masks);qli_free(values);qli_free(planes);qli_free(out);return ok;
}
int strip_decode(const uint8_t *file,size_t size,size_t limit,uint8_t **rgba,uint32_t *w,uint32_t *h) {
    return strip_decode_limited(file,size,limit,(size_t)1<<29,rgba,w,h,NULL);
}
#undef QLI_STRIP_X86

enum { TILE_HEADER=40,TILE_FOOTER=4,TILE_FILE_CAP=1u<<28 };
static uint8_t undo(unsigned v) { return (uint8_t)(v&1?-(int)((v+1)/2):(int)(v/2)); }
int qep_tile_decode(const uint8_t *file,size_t size,size_t limit,uint8_t **rgba,uint32_t *width,uint32_t *height) {
    if(!rgba || !width || !height)return 0;*rgba=NULL;*width=*height=0;
    if(!file || size<TILE_HEADER+TILE_FOOTER || size>TILE_FILE_CAP || memcmp(file,"QEP1",4) ||
       (file[13]!=1 && (file[13]!=2 || file[12]!=1)) || file[15]!=16 || qli_read_le32(file+36))return 0;
    unsigned output_channels=4;
    if(file[14]==3 && file[12]!=4)output_channels=3;
    if(file[14]!=output_channels)return 0;
    uint32_t w=qli_read_le32(file+4),h=qli_read_le32(file+8);size_t pixels=0;unsigned channels=file[12],log=qli_read_le32(file+28),streams=channels*8;
    if(!fast_shape(w,h,&pixels) || pixels>limit/output_channels || (channels!=1 && channels!=3 && channels!=4) ||
       (log!=0 && log!=4 && log!=7) || qli_read_le32(file+20)!=streams)return 0;
    size_t tiles=(size_t)((w+15)/16)*((h+15)/16),maps=tiles*channels,coefsize=region_count(w,h,log)*3;
    size_t lengths=TILE_HEADER+maps+coefsize,prefix=lengths+streams*4;
    if(qli_read_le32(file+16)!=maps || qli_read_le32(file+32)!=coefsize || prefix>size-4 ||
       qli_read_le32(file+size-4)!=qint_crc32(file,size-4))return 0;
    const uint8_t *map=file+TILE_HEADER,*coeffs=map+maps;
    for(size_t i=0;i<coefsize;++i)if(coeffs[i]>64 || (channels==1 && coeffs[i]!=32))return 0;
    size_t counts[32]={0},cursor[32]={0};
    for(unsigned c=0;c<channels;++c)for(uint32_t ty=0;ty<h;ty+=16)for(uint32_t tx=0;tx<w;tx+=16) {
        unsigned code=map[c*tiles+tile_at(w,tx,ty)];if(code>=128)return 0;
        size_t tw=w-tx<16?w-tx:16,th=h-ty<16?h-ty:16;counts[c*8+(code>>4)]+=tw*th;
    }
    uint8_t *pool=qli_malloc(pixels*channels),*planes=qli_malloc(pixels*channels),*out=qli_malloc(pixels*output_channels);int ok=0;
    if(!pool || !planes || !out)goto done;
    size_t offset=prefix,cumulative=0;
    for(unsigned s=0;s<streams;++s) {
        size_t length=qli_read_le32(file+lengths+s*4),got=0;cursor[s]=cumulative;
        if(length>size-4-offset || !qbs_decode(file+offset,length,pool+cumulative,counts[s],&got,QBS_NO_TABLE) || got!=counts[s])goto done;
        offset+=length;cumulative+=got;
    }
    if(offset!=size-4 || cumulative!=pixels*channels)goto done;
    const unsigned order[4]={1,0,2,3};
    for(unsigned stage=0;stage<channels;++stage) {
        unsigned c=channels==1?0:order[stage];uint8_t *p=planes+c*pixels;
        for(uint32_t ty=0;ty<h;ty+=16)for(uint32_t tx=0;tx<w;tx+=16) {
            unsigned code=map[c*tiles+tile_at(w,tx,ty)],mode=code&15,s=c*8+(code>>4);
            uint32_t tw=w-tx<16?w-tx:16,th=h-ty<16?h-ty:16;uint8_t steps[256];
            const uint8_t *coef=coeffs+3*region_at(w,tx,ty,log);
            int has_correction=channels!=1 && (c==0?coef[0]!=32:c==2?(coef[1]!=32 || coef[2]!=32):0);
            size_t k=0;
            if(!has_correction) {
                for(size_t j=0;j<(size_t)tw*th;++j)steps[j]=undo(pool[cursor[s]++]);
            } else {
                for(uint32_t y=ty;y<ty+th;++y)for(uint32_t x=tx;x<tx+tw;++x)
                    steps[k++]=(uint8_t)(undo(pool[cursor[s]++])+correction(planes,pixels,w,x,y,c,mode,coef));
            }
            reconstruct_tile(p,w,tx,ty,tw,th,mode,steps);
        }
    }
    for(size_t i=0;i<pixels;++i) {
        uint8_t *p=out+output_channels*i;
        unsigned g=channels==1?planes[i]:planes[pixels+i];p[1]=(uint8_t)g;
        p[0]=(uint8_t)(channels==1?(int)g-(file[13]==2?0:128):(int)planes[i]+(int)g-128);
        p[2]=(uint8_t)(channels==1?(int)g-(file[13]==2?0:128):(int)planes[2*pixels+i]+(int)g-128);
        if(output_channels==4)p[3]=channels==4?planes[3*pixels+i]:255;
    }
    if(qint_crc32(out,pixels*output_channels)!=qli_read_le32(file+24))goto done;
    *rgba=out;*width=w;*height=h;out=NULL;ok=1;
done:qli_free(pool);qli_free(planes);qli_free(out);return ok;
}

typedef struct { uint16_t freq[4096], base[4096]; unsigned used, single; } QliWideModel;
int qli_wide_model_read(const uint8_t *data, size_t size, size_t *used, QliWideModel *model);
int qli_wide_write(const uint16_t *tokens, size_t count, const uint8_t *literals, size_t literal_count,
                   const uint8_t *descriptor, size_t descriptor_bytes, const QliWideModel *model,
                   const RansModel *literal_model, uint8_t **frame, size_t *bytes, size_t *events);
int wide_encode(const uint8_t *, size_t, const uint8_t *, size_t, uint8_t **, size_t *, size_t *);
int wide_decode(const uint8_t *, size_t, uint8_t *, size_t, size_t, size_t *, size_t *);
int qwi_value_info(const uint8_t *file,size_t size,size_t *count,size_t *scratch){
    if(!file || !count || !scratch)return 0;*count=*scratch=0;
    if(size>=48 && !memcmp(file,"QBS1",4)){
        if(file[4]!=4 || file[5]!=12 || file[14] || file[15])return 0;
        uint64_t n=0;for(unsigned k=0;k<8;++k)n|=(uint64_t)file[6+k]<<(8*k);
        if(n>(1u<<25))return 0;*count=(size_t)n;return 1;
    }
    if(size<28 || size>(1u<<27) || memcmp(file,"QWS1",4) || file[24]!=1 || file[25] || file[26] || file[27])return 0;
    size_t n=qli_read_le32(file+4),nt=qli_read_le32(file+8),nl=qli_read_le32(file+12),tb=qli_read_le32(file+16),lb=qli_read_le32(file+20);
    if(n>(1u<<25) || nl>nt || nt>n || 2*(uint64_t)(nt-nl)+nl!=n || tb>size-28 || lb!=size-28-tb)return 0;
    *count=n;*scratch=nl;return 1;
}
int qwi_value_decode(const uint8_t *file,size_t size,uint8_t *out,size_t cap,size_t heap,size_t *got,size_t *peak){
    if(peak)*peak=0;if(!got)return 0;*got=0;size_t n=0,scratch=0;
    if(!qwi_value_info(file,size,&n,&scratch) || n>cap || (n && !out) || scratch>heap)return 0;
    if(!memcmp(file,"QBS1",4))return qbs_decode(file,size,out,cap,got,QBS_BYTE_TABLE);
    return wide_decode(file,size,out,cap,heap,got,peak);
}

#define SCALE 16384u
#define LOWER (1u << 23)
#define MAX_COUNT (1u << 25)
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0] | ((unsigned)p[1] << 8)); }
static uint32_t u32(const uint8_t *p) { return p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24); }
static uint64_t u64(const uint8_t *p) { return u32(p) | ((uint64_t)u32(p+4)<<32); }
static void p32(uint8_t *p,uint32_t v) { for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(8*i)); }

int qli_wide_model_read(const uint8_t *data,size_t size,size_t *used,QliWideModel *m) {
    if(!data || !size || !used || !m)return 0;
    memset(m,0,sizeof(*m));size_t at=1;unsigned kind=data[0],ids[4096],n=0;
    if(kind==0){*used=1;return 1;}
    if(kind==1){
        if(size<3 || u16(data+1)>=4096)return 0;
        m->single=u16(data+1);m->used=1;m->freq[m->single]=SCALE;*used=3;return 1;
    }
    if(kind==2){
        if(size<3)return 0;n=u16(data+1);at=3;
        if(n<2 || n>4096 || n>(size-at)/2)return 0;
        for(unsigned i=0;i<n;++i){ids[i]=u16(data+at);at+=2;if(ids[i]>=4096 || (i && ids[i]<=ids[i-1]))return 0;}
    }else if(kind==3){
        if(size<513)return 0;
        for(unsigned i=0;i<4096;++i)if(data[1+i/8]&(1u<<(i%8)))ids[n++]=i;
        at=513;if(n<2)return 0;
    }else return 0;
    unsigned total=0;
    for(unsigned i=0;i+1<n;++i){
        if(at==size)return 0;unsigned v=data[at++];
        if(v&128){
            if(at==size)return 0;unsigned next=data[at++];
            if(!next || (next&128))return 0;v=(v&127)|(next<<7);
        }
        if(!v || v>=SCALE || v>=SCALE-total)return 0;
        m->freq[ids[i]]=(uint16_t)v;total+=v;
    }
    m->freq[ids[n-1]]=(uint16_t)(SCALE-total);m->used=n;total=0;
    for(unsigned s=0;s<4096;++s){m->base[s]=(uint16_t)total;total+=m->freq[s];}
    if(total!=SCALE)return 0;*used=at;return 1;
}

static int token_encode(const uint16_t *tokens,size_t count,const uint8_t *desc,size_t nd,
                        const QliWideModel *m,uint8_t **frame,size_t *bytes) {
    uint8_t *buf[4]={0},*cur[4]={0},*out=NULL;size_t cap[4],len[4],total=16+nd+32;
    uint32_t state[4];int ok=0;
    for(unsigned lane=0;lane<4;++lane){
        size_t n=count>lane?(count-lane+3)/4:0;cap[lane]=2*n+1;
        buf[lane]=qli_malloc(cap[lane]);if(!buf[lane])goto done;cur[lane]=buf[lane]+cap[lane];uint32_t v=LOWER;
        for(size_t j=n;j-- >0;){
            unsigned s=tokens[4*j+lane];uint32_t f=m->freq[s],base=m->base[s];if(!f)goto done;
            uint32_t limit=((LOWER>>14)<<8)*f;
            while(v>=limit){if(cur[lane]==buf[lane])goto done;*--cur[lane]=(uint8_t)v;v>>=8;}
            v=((v/f)<<14)+v%f+base;
            if(v<LOWER || v>=(UINT32_C(1)<<31))goto done;
        }
        state[lane]=v;len[lane]=(size_t)(buf[lane]+cap[lane]-cur[lane]);total+=len[lane];
    }
    out=qli_calloc(1,total);if(!out)goto done;
    memcpy(out,"QWA1",4);out[4]=4;out[5]=14;p32(out+6,(uint32_t)count);memcpy(out+16,desc,nd);
    size_t at=16+nd;
    for(unsigned l=0;l<4;++l){p32(out+at+4*l,state[l]);p32(out+at+16+4*l,(uint32_t)len[l]);}
    at+=32;for(unsigned l=0;l<4;++l){memcpy(out+at,cur[l],len[l]);at+=len[l];}
    if(at!=total)goto done;*frame=out;*bytes=total;out=NULL;ok=1;
done:for(unsigned l=0;l<4;++l)qli_free(buf[l]);qli_free(out);return ok;
}

int qli_wide_write(const uint16_t *tokens, size_t nt, const uint8_t *lits, size_t nl,
                   const uint8_t *desc, size_t nd, const QliWideModel *m,
                   const RansModel *literal_model, uint8_t **frame, size_t *bytes, size_t *events)
{
    *frame = NULL;
    *bytes = 0;
    uint8_t *tf = NULL, *lf = NULL;
    size_t nbt = 0, nbl = 0;
    int ok = 0;
    unsigned used = literal_model ? literal_model->used : 0;
    if (!token_encode(tokens, nt, desc, nd, m, &tf, &nbt)) goto done;
    if (literal_model ? !qli_four_encode(lits, nl, literal_model, NULL, &lf, &nbl)
                      : !write_four_separate(lits, nl, &lf, &nbl, &used)) goto done;
    size_t total = 28 + nbt + nbl;
    if (total > (1u << 27)) goto done;
    uint8_t *out = qli_calloc(1, total);
    if (!out) goto done;
    memcpy(out, "QWS1", 4);
    p32(out + 4, (uint32_t)(2 * nt - nl));
    p32(out + 8, (uint32_t)nt);
    p32(out + 12, (uint32_t)nl);
    p32(out + 16, (uint32_t)nbt);
    p32(out + 20, (uint32_t)nbl);
    out[24] = 1;
    memcpy(out + 28, tf, nbt);
    memcpy(out + 28 + nbt, lf, nbl);
    if (events) *events = (m->used > 1 ? nt : 0) + (used > 1 ? nl : 0);
    *frame = out;
    *bytes = total;
    ok = 1;
done:
    qli_free(tf);
    qli_free(lf);
    return ok;
}

int wide_encode(const uint8_t *src,size_t n,const uint8_t *desc,size_t nd,
                uint8_t **frame,size_t *bytes,size_t *events) {
    if(frame)*frame=NULL;if(bytes)*bytes=0;if(events)*events=0;
    if(!frame || !bytes || n>MAX_COUNT || (n && !src))return 0;
    QliWideModel m;size_t used=0;if(!qli_wide_model_read(desc,nd,&used,&m) || used!=nd)return 0;
    uint16_t *tokens=n?qli_malloc(2*n):NULL;uint8_t *lits=n?qli_malloc(n):NULL;
    size_t nt=0,nl=0;uint8_t seen[4096]={0};int ok=0;
    if(n && (!tokens || !lits))goto done;
    for(size_t i=0;i<n;){
        unsigned s;
        if(n-i>=2 && src[i]<64 && src[i+1]<64 && (src[i]!=63 || src[i+1]!=63)){
            s=src[i]|((unsigned)src[i+1]<<6);i+=2;
        }else{s=4095;lits[nl++]=src[i++];}
        tokens[nt++]=(uint16_t)s;seen[s]=1;
    }
    for(unsigned s=0;s<4096;++s)if(!!seen[s]!=!!m.freq[s])goto done;
    ok=qli_wide_write(tokens,nt,lits,nl,desc,nd,&m,NULL,frame,bytes,events);
done:qli_free(tokens);qli_free(lits);return ok;
}

static inline int wide_refill(uint32_t *v,const uint8_t **p,const uint8_t *end){
    while(*v<LOWER){if(*p==end)return 0;*v=(*v<<8)|*(*p)++;}return 1;
}
static inline int expand(uint64_t e,const uint8_t *lit,size_t nl,size_t *li,uint8_t *out,size_t n,size_t *at){
    if(e&65536){if(*li>=nl || *at>=n)return 0;out[(*at)++]=lit[(*li)++];}
    else{if(n-*at<2)return 0;out[(*at)++]=(uint8_t)e;out[(*at)++]=(uint8_t)(e>>8);}
    return 1;
}
QLI_NOINLINE static int decode_multi(const QliWideModel *m,uint32_t state[4],const uint8_t *cur[4],
    const uint8_t *end[4],size_t nt,const uint8_t *lit,size_t nl,uint8_t *out,size_t n){
    uint64_t table[SCALE];
    for(unsigned s=0;s<4096;++s){
        unsigned f=m->freq[s],base=m->base[s];if(f>=SCALE)return 0;
        uint64_t value=s==4095?65536:((s&63)|((s>>6)<<8));
        for(unsigned j=0;j<f;++j)table[base+j]=value|((uint64_t)j<<17)|((uint64_t)f<<31);
    }
    size_t at=0,li=0,i=0;uint32_t a=state[0],b=state[1],c=state[2],d=state[3];
    const uint8_t *p0=cur[0],*p1=cur[1],*p2=cur[2],*p3=cur[3];
    for(;nt-i>=4;i+=4){
        uint64_t e0=table[a&16383],e1=table[b&16383],e2=table[c&16383],e3=table[d&16383];
        if(!expand(e0,lit,nl,&li,out,n,&at) || !expand(e1,lit,nl,&li,out,n,&at) ||
           !expand(e2,lit,nl,&li,out,n,&at) || !expand(e3,lit,nl,&li,out,n,&at))return 0;
        a=(uint32_t)(e0>>31)*(a>>14)+(uint32_t)((e0>>17)&16383);
        b=(uint32_t)(e1>>31)*(b>>14)+(uint32_t)((e1>>17)&16383);
        c=(uint32_t)(e2>>31)*(c>>14)+(uint32_t)((e2>>17)&16383);
        d=(uint32_t)(e3>>31)*(d>>14)+(uint32_t)((e3>>17)&16383);
        if(!wide_refill(&a,&p0,end[0]) || !wide_refill(&b,&p1,end[1]) || !wide_refill(&c,&p2,end[2]) || !wide_refill(&d,&p3,end[3]))return 0;
    }
    state[0]=a;state[1]=b;state[2]=c;state[3]=d;cur[0]=p0;cur[1]=p1;cur[2]=p2;cur[3]=p3;
    for(;i<nt;++i){
        unsigned lane=(unsigned)i&3;uint32_t v=state[lane];uint64_t e=table[v&16383];
        if(!expand(e,lit,nl,&li,out,n,&at))return 0;
        v=(uint32_t)(e>>31)*(v>>14)+(uint32_t)((e>>17)&16383);
        if(!wide_refill(&v,&cur[lane],end[lane]))return 0;state[lane]=v;
    }
    if(at!=n || li!=nl)return 0;
    for(unsigned l=0;l<4;++l)if(cur[l]!=end[l] || state[l]!=LOWER)return 0;
    return 1;
}

int wide_decode(const uint8_t *file,size_t size,uint8_t *out,size_t capacity,size_t heap_cap,
                size_t *count_out,size_t *peak){
    if(peak)*peak=0;if(!count_out)return 0;*count_out=0;
    if(!file || size<28 || size>(1u<<27) || memcmp(file,"QWS1",4) || file[24]!=1 || file[25] || file[26] || file[27])return 0;
    size_t n=u32(file+4),nt=u32(file+8),nl=u32(file+12),nbt=u32(file+16),nbl=u32(file+20);
    if(n>MAX_COUNT || n>capacity || (n && !out) || nl>nt || nt>n || 2*(uint64_t)(nt-nl)+nl!=n ||
       nbt>size-28 || nbl!=size-28-nbt || nl>heap_cap || nbt<49)return 0;
    const uint8_t *tf=file+28;
    if(memcmp(tf,"QWA1",4) || tf[4]!=4 || tf[5]!=14 || tf[14] || tf[15] || u64(tf+6)!=nt)return 0;
    QliWideModel m;size_t md=0;if(!qli_wide_model_read(tf+16,nbt-16,&md,&m) || (!!nt!=!!m.used))return 0;
    size_t at=16+md;if(nbt-at<32)return 0;
    uint32_t state[4],len[4];const uint8_t *cur[4],*end[4];
    for(unsigned l=0;l<4;++l){state[l]=u32(tf+at+4*l);len[l]=u32(tf+at+16+4*l);
        if(state[l]<LOWER || state[l]>=(UINT32_C(1)<<31))return 0;}
    at+=32;
    for(unsigned l=0;l<4;++l){if(len[l]>nbt-at)return 0;cur[l]=tf+at;at+=len[l];end[l]=tf+at;}
    if(at!=nbt)return 0;
    if(m.used<=1)for(unsigned l=0;l<4;++l)if(len[l] || state[l]!=LOWER)return 0;
    uint8_t *lit=nl?qli_malloc(nl):NULL;if(nl && !lit)return 0;if(peak)*peak=nl;
    size_t got=0;int ok=0;
    if(!qbs_decode(file+28+nbt,nbl,lit,nl,&got,QBS_BYTE_TABLE) || got!=nl)goto done;
    if(m.used<=1){
        if(!nt){if(n || nl)goto done;}
        else if(m.single==4095){if(nl!=nt || n!=nl)goto done;if(n)memcpy(out,lit,n);}
        else{if(nl || n!=2*nt)goto done;for(size_t i=0;i<n;i+=2){out[i]=(uint8_t)(m.single&63);out[i+1]=(uint8_t)(m.single>>6);}}
    }else if(!decode_multi(&m,state,cur,end,nt,lit,nl,out,n))goto done;
    *count_out=n;ok=1;
done:qli_free(lit);return ok;
}

#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42

enum {
    X86_SSE42 = 1u << 20,
    X86_POPCNT = 1u << 23,
    X86_AVX = 1u << 28,
    X86_OSXSAVE = 1u << 27,
    X86_AVX2 = 1u << 5,
    X86_PCLMUL = 1u << 1,
    X86_XMM_YMM = 6
};

static unsigned qli_x86_capabilities(unsigned leaf1, unsigned leaf7, unsigned xstate)
{
    const unsigned sse42 = X86_SSE42 | X86_POPCNT;
    if ((leaf1 & sse42) != sse42) return 0;
    unsigned caps = QLI_CPU_SSE42;
    if (leaf1 & X86_PCLMUL) caps |= QLI_CPU_PCLMUL_CRC;
    const unsigned avx = X86_AVX | X86_OSXSAVE;
    if ((leaf1 & avx) != avx || !(leaf7 & X86_AVX2) ||
        (xstate & X86_XMM_YMM) != X86_XMM_YMM)
        return caps;

    caps |= QLI_CPU_AVX2;
    return caps;
}

typedef struct { unsigned eax, ebx, ecx, edx; } QliCpuid;

static QliCpuid qli_cpuid(unsigned leaf)
{
#if defined(_MSC_VER)
    int regs[4];
    __cpuidex(regs, (int)leaf, 0);
    return (QliCpuid){(unsigned)regs[0], (unsigned)regs[1],
                      (unsigned)regs[2], (unsigned)regs[3]};
#else
    QliCpuid regs;
    __cpuid_count(leaf, 0, regs.eax, regs.ebx, regs.ecx, regs.edx);
    return regs;
#endif
}

#if QLI_MAX_CPU_TIER >= QLI_TIER_AVX2
static unsigned qli_x86_xstate(void)
{
#if defined(__clang__) || defined(__GNUC__)
    unsigned low, high;
    __asm__ volatile("xgetbv" : "=a"(low), "=d"(high) : "c"(0));
    return low;
#else
    return (unsigned)_xgetbv(0);
#endif
}
#endif

static unsigned qli_detect_cpu(void)
{
    const unsigned max_leaf = qli_cpuid(0).eax;
    if (max_leaf < 1) return 0;
    const unsigned leaf1 = qli_cpuid(1).ecx;
#if QLI_MAX_CPU_TIER >= QLI_TIER_AVX2
    if (max_leaf >= 7 && (leaf1 & (X86_AVX | X86_OSXSAVE)) == (X86_AVX | X86_OSXSAVE)) {
        const unsigned leaf7 = qli_cpuid(7).ebx;
        if (leaf7 & X86_AVX2) {
            return qli_x86_capabilities(leaf1, leaf7, qli_x86_xstate());
        }
    }
#endif
    return qli_x86_capabilities(leaf1, 0, 0);
}
#endif

unsigned qli_cpu_capabilities(void)
{
#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42
    enum { CAPS_KNOWN = 1u << 3 };
    static atomic_uint cached;
    unsigned caps = atomic_load_explicit(&cached, memory_order_relaxed);
    if (!caps) {
        caps = qli_detect_cpu() | CAPS_KNOWN;
        atomic_store_explicit(&cached, caps, memory_order_relaxed);
    }
    return caps & ~CAPS_KNOWN;
#else
    return 0;
#endif
}

typedef QlicLiteStatus (*QnlEncode)(const QlicLiteDescription *,const uint8_t *,size_t,size_t,
                    const QlicLiteLimits *,const QlicLiteAllocator *,QlicLiteFile *,QlicLiteStats *);

typedef struct {
    QlicLiteFile file;
    size_t peak;
    unsigned dense;
} QrouteFile;

typedef struct { size_t peak,calls,live; } PaletteStats;

typedef struct {
    QrouteFile samples;
    uint8_t *palette;
    size_t palette_size,peak;
} Encoded;

typedef struct {
    Encoded selection;
    uint8_t *copy;
    size_t bytes,peak;
} CopyResult;

typedef struct {
    CopyResult base;
    uint8_t *history;
    size_t bytes, peak;
    int try_transpose;
} HistoryFile;

static int qli_encode_rgba(const uint8_t *, size_t,
                    unsigned, unsigned, size_t, size_t, HistoryFile *);
static void qli_free_rgba(HistoryFile *);

static int qli_select_base(const uint8_t *, size_t, unsigned, unsigned,
                    size_t, size_t, CopyResult *);
static void qli_free_base(CopyResult *);


size_t qli_opaque_file_price(const uint8_t *, size_t);
size_t qli_opaque_history_record_size(size_t);

QlicLiteStatus qli_encode_samples(const QlicLiteDescription *, const uint8_t *, size_t, size_t,
                             const QlicLiteLimits *, const QlicLiteAllocator *, QlicLiteFile *, QlicLiteStats *);
QlicLiteStatus qli_encode_icon(const QlicLiteDescription *, const uint8_t *, size_t, size_t,
                          const QlicLiteLimits *, const QlicLiteAllocator *, QlicLiteFile *, QlicLiteStats *);
QlicLiteStatus qli_encode_structure(const QlicLiteDescription *, const uint8_t *, size_t, size_t,
                               const QlicLiteLimits *, const QlicLiteAllocator *, QlicLiteFile *, QlicLiteStats *);
QlicLiteStatus qli_encode_capped_structure(const QlicLiteDescription *, const uint8_t *, size_t, size_t,
                                      const QlicLiteLimits *, const QlicLiteAllocator *, QlicLiteFile *, QlicLiteStats *);
typedef int QliSampleCore(const uint8_t *, size_t, unsigned, unsigned, uint8_t **, size_t *);
QlicLiteStatus qli_write_sample_image(const QlicLiteDescription *, const uint8_t *, size_t, const QlicLiteLimits *,
                                 uint8_t **, size_t *, QliSampleCore *);
int qli_encode_sample_core(const uint8_t *, size_t, unsigned, unsigned, uint8_t **, size_t *);
size_t qli_numeric_bound(unsigned, unsigned, size_t);
int qli_encode_numeric(const uint8_t *, size_t, unsigned, unsigned, size_t,
                       size_t, uint8_t **, size_t *, size_t *);
int qli_encode_numeric_capped(const uint8_t *, size_t, unsigned, unsigned, size_t,
                              size_t, uint8_t **, size_t *, size_t *);
int qli_encode_repeat(const uint8_t *, size_t, unsigned, unsigned, size_t, size_t,
                      size_t, uint8_t **, size_t *, PaletteStats *);
int qli_encode_history(const uint8_t *, size_t, unsigned, unsigned, size_t, size_t,
                       size_t, uint8_t **, size_t *, PaletteStats *);
int qli_encode_palette(const uint8_t *, size_t, unsigned, unsigned, size_t, size_t,
                       size_t, uint8_t **, size_t *, PaletteStats *);

void qlic_lite_file_free(QlicLiteFile *file)
{
    if (!file) return;
    QlicLiteFile owned = *file;
    memset(file, 0, sizeof(*file));
    if (owned.data && owned.release) owned.release(owned.data, owned.user);
}

static QlicLiteFile take_encoded_file(HistoryFile *output)
{
    CopyResult *selection = &output->base;
    if (output->history) {
        uint8_t *data = output->history;
        output->history = NULL;
        return (QlicLiteFile){data, output->bytes, ordinary_release, NULL};
    }
    if (selection->copy) {
        uint8_t *data = selection->copy;
        selection->copy = NULL;
        return (QlicLiteFile){data, selection->bytes, ordinary_release, NULL};
    }
    if (selection->selection.palette) {
        uint8_t *data = selection->selection.palette;
        selection->selection.palette = NULL;
        return (QlicLiteFile){data, selection->selection.palette_size, ordinary_release, NULL};
    }
    QlicLiteFile owned = selection->selection.samples.file;
    memset(&selection->selection.samples.file, 0, sizeof(owned));
    QlicLiteRelease release = selection->selection.samples.dense
        ? owned.release : ordinary_release;
    return (QlicLiteFile){owned.data, owned.bytes, release, owned.user};
}

QlicLiteStatus qlic_lite_encode(const QlicLiteDescription *description,
                               const uint8_t *samples, size_t bytes, size_t stride,
                               const QlicLiteLimits *limits, QlicLiteFile *file,
                               size_t *peak_bytes)
{
    if (peak_bytes) *peak_bytes = 0;
    if (!description || !samples || !limits || !file || file->data || file->bytes ||
        file->release || file->user || !description->width || !description->height ||
        description->bits < 8 ||
        description->bits > 10) return QLIC_LITE_INVALID;
    if (qli_allocation_active()) return QLIC_LITE_REENTRANT;
    if (bytes > limits->pixel_data_bytes || bytes > limits->input_bytes ||
        (uint64_t)description->width * description->height > limits->pixels)
        return QLIC_LITE_LIMIT;

    const QlicLiteDescription *image = description;
    int byte_layout=image->channels==4 && image->alpha==QLIC_LITE_ALPHA_STRAIGHT;
    byte_layout|=image->channels==3 && !image->alpha;
    if (image->bits == 8 && byte_layout &&
        !image->authority && !image->has_cicp && !image->has_mastering && !image->has_light &&
        !image->icc_size && image->width <= 16384 && image->height <= 16384 &&
        (size_t)image->width * image->height <= (1u << 24) &&
        stride == image->channels*(size_t)image->width && bytes == stride*image->height &&
        limits->allocation_requests >= 4096 && limits->file_bytes) {
        const size_t max_file = 1u << 27;
        size_t limit = limits->file_bytes < max_file ? limits->file_bytes : max_file;
        if (limit < 16 || !limits->heap_bytes) return QLIC_LITE_LIMIT;
        HistoryFile output = {0};
        int success = qli_encode_rgba(samples, bytes, image->width,
                                      image->height, limits->heap_bytes, limit, &output);
        int retried = !success && limit < max_file;
        if (retried) {
            qli_free_rgba(&output);
            success = qli_encode_rgba(samples, bytes, image->width,
                                      image->height, limits->heap_bytes, max_file, &output);
        }
        if (success) {
            *file = take_encoded_file(&output);
        }
        size_t peak = retried ? limits->heap_bytes : output.peak;
        qli_free_rgba(&output);
        if (file->data && file->bytes && file->bytes <= limit) {
            if (peak_bytes) *peak_bytes = peak;
            return QLIC_LITE_OK;
        }
        if (file->data) heap_limit_hit = 0;
        qlic_lite_file_free(file);
        return QLIC_LITE_LIMIT;
    }

    QlicLiteStats stats = {0};
    QlicLiteStatus status = qli_encode_samples(image, samples, bytes, stride, limits, NULL, file, &stats);
    if (status == QLIC_LITE_OK && peak_bytes) *peak_bytes = stats.peak_bytes;
    return status;
}

typedef struct {
    size_t cap, fail_at;
    PaletteStats stats;
    const QlicLiteAllocator *allocator;
} QliWriterMemory;

static inline void *qli_writer_allocate(QliWriterMemory *memory, size_t size)
{
    if (memory->allocator)
        return memory->allocator->allocate(size ? size : 1, memory->allocator->user);
    PaletteStats *stats = &memory->stats;
    if (++stats->calls == memory->fail_at) return NULL;
    if (size > memory->cap - stats->live) {
        heap_limit_hit = 1;
        return NULL;
    }
    void *data = malloc(size ? size : 1);
    if (data) {
        stats->live += size;
        if (stats->live > stats->peak) stats->peak = stats->live;
    }
    return data;
}

static inline void qli_writer_free(QliWriterMemory *memory, void *data, size_t size)
{
    if (data) {
        if (memory->allocator) {
            memory->allocator->release(data, memory->allocator->user);
            return;
        }
        assert(size <= memory->stats.live);
        memory->stats.live -= size;
        free(data);
    }
}

typedef struct {
    uint8_t *data;
    size_t bytes, capacity;
} QliWriterPacket;

int qli_encode_byte_packet(QliWriterMemory *, const uint8_t *, size_t, QliWriterPacket *);
int qli_encode_action_packet(QliWriterMemory *, const uint8_t *, size_t, QliWriterPacket *);

typedef struct {
    uint64_t reciprocal;
    uint32_t frequency, base, limit;
} BytePacketEntry;

static inline int byte_packet_push(const BytePacketEntry *entry, uint32_t *state,
                                   uint8_t **cursor, const uint8_t *begin)
{
    uint32_t value = *state;
    if (!entry->frequency) return 0;
    while (value >= entry->limit) {
        if (*cursor == begin) return 0;
        *--*cursor = (uint8_t)value;
        value >>= 8;
    }
    uint32_t quotient = (uint32_t)(((uint64_t)value * entry->reciprocal) >> 32);
    uint32_t remainder = value - quotient * entry->frequency;
    if (remainder >= entry->frequency) {
        ++quotient;
        remainder -= entry->frequency;
    }
    *state = (quotient << QLIC_RANS_SCALE_BITS) + remainder + entry->base;
    return 1;
}

static int encode_byte_model(QliWriterMemory *memory, const uint8_t *symbols, size_t count,
                             const RansModel *model, QliWriterPacket *packet)
{
    BytePacketEntry table[256] = {0};
    for (unsigned symbol = 0; symbol < 256; ++symbol) {
        const uint32_t frequency = model->frequency[symbol];
        if (!frequency) continue;
        table[symbol] = (BytePacketEntry){
            (UINT64_C(1) << 32) / frequency, frequency, model->cumulative[symbol],
            ((RANS_BYTE_L >> QLIC_RANS_SCALE_BITS) << 8) * frequency,
        };
    }

    uint8_t *buffers[4] = {0}, *cursor[4] = {0}, *out = NULL;
    size_t capacities[4] = {0}, lengths[4] = {0};
    uint32_t states[4] = {RANS_BYTE_L, RANS_BYTE_L, RANS_BYTE_L, RANS_BYTE_L};
    size_t total = RANS_HEADER_BYTES + qlr_model_descriptor_bytes(model) + 32;
    int ok = 0;
    for (unsigned lane = 0; lane < 4; ++lane) {
        const size_t samples = count > lane ? (count - lane + 3) / 4 : 0;
        capacities[lane] = samples * 2 + 1;
        buffers[lane] = qli_writer_allocate(memory, capacities[lane]);
        if (!buffers[lane]) goto done;
        cursor[lane] = buffers[lane] + capacities[lane];
    }
    if (model->used > 1) {
        size_t position = count & ~(size_t)3;
        for (size_t i = count; i > position;) {
            --i;
            const unsigned lane = (unsigned)i & 3;
            if (!byte_packet_push(table + symbols[i], states + lane, cursor + lane,
                                  buffers[lane])) goto done;
        }
        while (position) {
            position -= 4;
            if (!byte_packet_push(table + symbols[position], states, cursor, buffers[0]) ||
                !byte_packet_push(table + symbols[position + 1], states + 1, cursor + 1, buffers[1]) ||
                !byte_packet_push(table + symbols[position + 2], states + 2, cursor + 2, buffers[2]) ||
                !byte_packet_push(table + symbols[position + 3], states + 3, cursor + 3, buffers[3])) goto done;
        }
    }
    for (unsigned lane = 0; lane < 4; ++lane) {
        lengths[lane] = (size_t)(buffers[lane] + capacities[lane] - cursor[lane]);
        total += lengths[lane];
    }
    out = qli_writer_allocate(memory, total);
    if (!out) goto done;
    memcpy(out, "QBS1", 4);
    out[4] = 4;
    out[5] = QLIC_RANS_SCALE_BITS;
    qli_write_le64(out + 6, count);
    out[14] = out[15] = 0;
    size_t offset = RANS_HEADER_BYTES;
    if (!qlr_model_write(model, out, total, &offset)) goto done;
    for (unsigned lane = 0; lane < 4; ++lane) {
        qli_write_le32(out + offset + 4 * lane, states[lane]);
        qli_write_le32(out + offset + 16 + 4 * lane, (uint32_t)lengths[lane]);
    }
    offset += 32;
    for (unsigned lane = 0; lane < 4; ++lane) {
        memcpy(out + offset, cursor[lane], lengths[lane]);
        offset += lengths[lane];
    }
    if (offset != total) goto done;
    *packet = (QliWriterPacket){out, total, total};
    out = NULL;
    ok = 1;
done:
    for (unsigned lane = 0; lane < 4; ++lane)
        qli_writer_free(memory, buffers[lane], capacities[lane]);
    qli_writer_free(memory, out, total);
    return ok;
}

int qli_encode_byte_packet(QliWriterMemory *memory, const uint8_t *symbols, size_t count,
                           QliWriterPacket *packet)
{
    *packet = (QliWriterPacket){0};
    if ((count && !symbols) || count > (1u << 27)) return 0;
    RansModel model;
    if (!qlr_model_build(symbols, count, &model)) return 0;
    return encode_byte_model(memory, symbols, count, &model, packet);
}

size_t qli_opaque_history_record_size(size_t count)
{
    
    if (count <= 53) return 5+count;
    const uint64_t histogram[256] = {[0]=count-1,[255]=1};
    RansModel model;
    if (!qlr_model_normalize(histogram,count,&model)) return 5+count;
    const unsigned frequency=model.frequency[0];
    const uint64_t reciprocal=(UINT64_C(1)<<32)/frequency;
    const uint32_t renorm=((RANS_BYTE_L>>QLIC_RANS_SCALE_BITS)<<8)*frequency;
    uint32_t state=RANS_BYTE_L;
    size_t emitted=0,sizes[2];
    
    const size_t zeros0=(count-1)/4;
    for (size_t i=0;i<zeros0;++i) {
        if (state>=renorm) { state>>=8; ++emitted; }
        uint32_t q=(uint32_t)(((uint64_t)state*reciprocal)>>32);
        uint32_t r=state-q*frequency;
        if (r>=frequency) { ++q; r-=frequency; }
        state=(q<<QLIC_RANS_SCALE_BITS)+r;
    }
    sizes[0]=emitted;
    sizes[1]=emitted+(state>=renorm);
    const uint32_t single=((RANS_BYTE_L>>QLIC_RANS_SCALE_BITS)<<8)*model.frequency[255];
    while (state>=single) { state>>=8; ++emitted; }
    size_t packet=RANS_HEADER_BYTES+qlr_model_descriptor_bytes(&model)+32;
    packet+=emitted;
    for (unsigned lane=1;lane<4;++lane) {
        size_t zeros=(count+3-lane)/4;
        packet+=sizes[zeros-zeros0];
    }
    return 5+(packet<count ? packet : count);
}

int qli_encode_action_packet(QliWriterMemory *memory, const uint8_t *symbols, size_t count,
                             QliWriterPacket *packet)
{
    *packet = (QliWriterPacket){0};
    if (!symbols || !count || count > QVP_CAP) return 0;
    RansModel model;
    if (!qlr_model_build(symbols, count, &model)) return 0;
    if (model.used == 1) return encode_byte_model(memory, symbols, count, &model, packet);

    const size_t end = RANS_HEADER_BYTES + qlr_model_descriptor_bytes(&model);
    const size_t payload_start = end + 4 * QVP_LANES;
    const size_t capacity = payload_start + 2 * count;
    uint8_t *file = qli_writer_allocate(memory, capacity);
    if (!file) return 0;
    memcpy(file, "QVP1", 4);
    file[4] = QVP_LANES;
    file[5] = QLIC_RANS_SCALE_BITS;
    qli_write_le64(file + 6, count);
    file[14] = file[15] = 0;
    size_t offset = RANS_HEADER_BYTES;
    if (!qlr_model_write(&model, file, end, &offset) || offset != end) {
        qli_writer_free(memory, file, capacity);
        return 0;
    }

    uint8_t *tail = file + capacity;
    uint32_t state[QVP_LANES];
    for (unsigned lane = 0; lane < QVP_LANES; ++lane) state[lane] = QVP_LOW;
    for (size_t i = count; i--;) {
        const unsigned lane = (unsigned)i & (QVP_LANES - 1u);
        const unsigned symbol = symbols[i], frequency = model.frequency[symbol];
        uint32_t value = state[lane];
        if (value >= ((uint32_t)frequency << 20)) {
            tail -= 2;
            tail[0] = (uint8_t)value;
            tail[1] = (uint8_t)(value >> 8);
            value >>= 16;
        }
        const uint32_t quotient = value / frequency;
        state[lane] = (quotient << QLIC_RANS_SCALE_BITS) + (value - quotient * frequency) +
                      model.cumulative[symbol];
    }
    const size_t payload = (size_t)(file + capacity - tail);
    memmove(file + payload_start, tail, payload);
    for (unsigned lane = 0; lane < QVP_LANES; ++lane)
        qli_write_le32(file + end + 4 * lane, state[lane]);
    *packet = (QliWriterPacket){file, payload_start + payload, capacity};
    return 1;
}

typedef void QliDenseSeedScores(const int16_t samples[4][256], unsigned count,
                                uint32_t scores[16], uint32_t magnitudes[16]);
typedef unsigned QliDenseSeedBest(const int16_t samples[4][256], unsigned count, uint32_t *magnitude);
typedef void QliDenseMoments(const int16_t samples[3][4][256], unsigned count,
                             const unsigned mode[2], unsigned mask, int64_t out[2][9]);
typedef void QliDenseCosts(const int16_t samples[3][4][256], unsigned count,
                           const unsigned mode[2], const int32_t coefficients[2][3],
                           unsigned mask, uint64_t out[2][3]);
typedef void QliDenseModeCosts(const int16_t samples[3][4][256], unsigned count,
                               const int coefficients[3], unsigned mask, uint32_t out[2][16]);
typedef void QliDenseSharedCosts(const int16_t samples[3][4][256], unsigned count,
                                 const unsigned mode[2], const int32_t coefficients[3][2][3],
                                 uint64_t out[3][2][3], unsigned distinct[2]);
typedef void QliDenseScoreRow(const uint8_t *current, const uint8_t *upper,
                              unsigned width, unsigned x, unsigned count, unsigned mode, int8_t *out);
typedef void QliDenseBankScores(const uint16_t counts[256], const float prices[256][8], double scores[8]);

QliDenseSeedScores qli_dense_seed_scores;
QliDenseSeedBest qli_dense_seed_best;
QliDenseMoments qli_dense_moments;
QliDenseCosts qli_dense_costs;
QliDenseModeCosts qli_dense_mode_costs;
QliDenseSharedCosts qli_dense_shared_costs;
QliDenseScoreRow qli_dense_score_row;
QliDenseBankScores qli_dense_seed_bank_scores, qli_dense_final_bank_scores;

typedef struct {
    QliDenseSeedScores *seed_scores;
    QliDenseSeedBest *seed_best;
    QliDenseMoments *moments;
    QliDenseCosts *costs;
    QliDenseModeCosts *mode_costs;
    QliDenseSharedCosts *shared_costs;
    QliDenseScoreRow *score_row;
    QliDenseBankScores *seed_bank_scores, *final_bank_scores;
} QliDenseKernels;

extern const QliDenseKernels qli_dense_portable;
#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42 && (defined(__GNUC__) || defined(__clang__))
#define QLI_DENSE_DISPATCH 1
#define QLI_DENSE_TARGET __attribute__((target("sse4.2,popcnt")))
static const QliDenseKernels qli_dense_sse42;
#endif

static inline const QliDenseKernels *qli_dense_kernels(void)
{
#ifdef QLI_DENSE_DISPATCH
    if (qli_cpu_capabilities() & QLI_CPU_SSE42) return &qli_dense_sse42;
#endif
    return &qli_dense_portable;
}

typedef struct {
    uint32_t w, h, raw_crc;
    unsigned channels, streams;
    size_t pixels, maps, tiles;
    uint8_t *planes;
    const uint8_t *initial_map;
} QliDenseImage;

typedef struct {
    unsigned log;
    size_t regions;
    uint8_t *map, *coeff;
} QliDenseFitState;

typedef struct {
    const QliDenseImage *source;
    QliDenseFitState state[3];
    unsigned count;
} QliDenseFit;

typedef struct {
    uint32_t w, h, raw_crc;
    unsigned channels, streams, log;
    size_t pixels, tiles, maps, coefficient_bytes;
    uint8_t *symbols, *map, *coefficients;
} QliDenseField;

typedef struct {
    QliDenseField field;
    uint32_t joint[32][256];
} QliStructuralField;

int qli_dense_prepare(const uint8_t *raw, size_t bytes, uint32_t w, uint32_t h,
                       QliDenseImage **out);
int qli_dense_prepare_structure(const uint8_t *raw, size_t bytes, uint32_t w, uint32_t h,
                                int *direct_gray, QliStructuralField **out);
void qli_dense_image_free(QliDenseImage *image);
int qli_dense_fit_sample(const QliDenseImage *image, QliDenseFit **out);
void qli_dense_fit_free(QliDenseFit *fit);
int qli_dense_field_create(const QliDenseFit *fit, QliDenseField **out);
void qli_dense_field_free(QliDenseField *field);
void qli_structural_field_free(QliStructuralField *field);
int qli_dense_write_tiles(const QliDenseField *field, uint8_t **out, size_t *bytes);
int qli_structural_write_tiles(const QliDenseField *field, int direct_gray, uint8_t **out, size_t *bytes);
int qli_dense_wide_model_hist(const uint32_t *counts, uint8_t *out, size_t cap, size_t *bytes);
int qli_dense_write_magnitude(const uint8_t *symbols, size_t count, uint8_t **out, size_t *bytes);
QlicLiteStatus qli_structural_finish(const QliStructuralField *field, int capped, int direct_gray,
                                size_t cap, uint8_t **out, size_t *bytes);
int qli_dense_finish_sample(const QliDenseField *field, size_t incumbent,
                            uint8_t **out, size_t *bytes);

static inline size_t qli_dense_tile_pixels(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    return (size_t)(w - x < 16 ? w - x : 16) * (h - y < 16 ? h - y : 16);
}

#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42 && (defined(_M_X64) || defined(__x86_64__))
#define QLI_DENSE_STRUCTURE_X86 1
#else
#define QLI_DENSE_STRUCTURE_X86 0
#endif

enum { DENSE_TILE_HEADER = 40, DENSE_TILE_FOOTER = 4, DENSE_FILE_CAP = 1u << 28 };
static const unsigned dense_fit_logs[3] = {0, 7, 4};

typedef struct {
    double n, g, r, e, gg, rr, gr, eg, er;
} DenseMoments;
typedef struct {
    int kg, ag, ar;
    uint64_t cost[3];
} DenseProposal;
typedef union {
    DenseMoments moment;
    DenseProposal proposal;
} DenseFitCell;
typedef struct {
    uint8_t *data;
    size_t stride, plane;
} DensePlanes;
typedef struct {
    const QliDenseKernels *kernels;
    int8_t errors[4][16][256];
    uint16_t filled[4];
    uint32_t tx, ty, tw, th;
    unsigned count;
} DenseTileCache;

static uint8_t dense_symbol(int error)
{
    unsigned v = (uint8_t)error;
    return (uint8_t)(v < 128 ? v * 2 : (256 - v) * 2 - 1);
}

static int dense_rounded_coefficient(double v)
{
    if (v <= -32) {
        return -32;
    }
    if (v >= 32) {
        return 32;
    }
    return (int)(v < 0 ? ceil(v - 0.5) : floor(v + 0.5));
}

static void dense_tile_start(DenseTileCache *t, const QliDenseImage *p, uint32_t x, uint32_t y)
{
    memset(t->filled, 0, sizeof(t->filled));
    t->tx = x;
    t->ty = y;
    t->tw = p->w - x < 16 ? p->w - x : 16;
    t->th = p->h - y < 16 ? p->h - y : 16;
    t->count = t->tw * t->th;
}

static DenseProposal dense_propose(DenseMoments a, unsigned ch)
{
    double gg = a.gg - a.g * a.g / a.n, rr = a.rr - a.r * a.r / a.n, gr = a.gr - a.g * a.r / a.n;
    double eg = a.eg - a.e * a.g / a.n, er = a.er - a.e * a.r / a.n;
    int kg = gg > 0 ? dense_rounded_coefficient(16 * eg / gg) : 0, ag = kg, ar = 0;
    double det = gg * rr - gr * gr;
    if (ch == 2 && det > gg * rr / 1024 && det > 0) {
        ag = dense_rounded_coefficient(16 * (eg * rr - er * gr) / det);
        ar = dense_rounded_coefficient(16 * (er * gg - eg * gr) / det);
    } else if (ch == 2 && gg <= 0 && rr > 0) {
        ag = 0;
        ar = dense_rounded_coefficient(16 * er / rr);
    }
    DenseProposal result = {kg, ag, ar, {0, 0, 0}};
    return result;
}

void qli_dense_fit_free(QliDenseFit *b)
{
    if (b) {
        for (unsigned k = 0; k < b->count; ++k) {
            qli_free(b->state[k].map);
            qli_free(b->state[k].coeff);
        }
        qli_free(b);
    }
}

static int dense_planes_create(const QliDenseImage *p, DensePlanes *v)
{
    memset(v, 0, sizeof(*v));
    if (p->channels == 1) {
        return 1;
    }
    v->stride = (size_t)p->w + 1;
    v->plane = v->stride * ((size_t)p->h + 1);
    v->data = qli_calloc(3, v->plane);
    if (!v->data) {
        return 0;
    }
    const unsigned order[3] = {1, 0, 2};
    for (unsigned c = 0; c < 3; ++c) {
        for (unsigned y = 0; y < p->h; ++y) {
            memcpy(v->data + c * v->plane + (y + 1) * v->stride + 1,
                   p->planes + order[c] * p->pixels + (size_t)y * p->w, p->w);
        }
    }
    return 1;
}

static int dense_structure_native(void);
static unsigned dense_seed_tile(const QliDenseImage *image, unsigned channel, unsigned tx,
                                unsigned ty, int16_t out[4][256], int native);

static unsigned dense_samples_load(const QliDenseImage *p, const DensePlanes *v, unsigned tx,
                                   unsigned ty, int16_t s[3][4][256])
{
    if (!v->data) {
        const int native = dense_structure_native();
        dense_seed_tile(p, 1, tx, ty, s[0], native);
        dense_seed_tile(p, 0, tx, ty, s[1], native);
        return dense_seed_tile(p, 2, tx, ty, s[2], native);
    }
    unsigned tw = p->w - tx < 16 ? p->w - tx : 16, th = p->h - ty < 16 ? p->h - ty : 16, n = tw * th;
    for (unsigned c = 0; c < 3; ++c) {
        const uint8_t *plane = v->data + c * v->plane;
        unsigned at = 0;
        for (unsigned y = ty; y < ty + th; ++y) {
            for (unsigned x = tx; x < tx + tw; ++x, ++at) {
                size_t j = (size_t)(y + 1) * v->stride + x + 1;
                s[c][0][at] = plane[j];
                s[c][1][at] = plane[j - 1];
                s[c][2][at] = plane[j - v->stride];
                s[c][3][at] = plane[j - v->stride - 1];
            }
        }
        for (unsigned i = n; i < ((n + 15) & ~15u); ++i) {
            for (unsigned j = 0; j < 4; ++j) {
                s[c][j][i] = 0;
            }
        }
    }
    return n;
}

static void dense_moments_add(DenseMoments *m, const int64_t a[9])
{
    m->n += (double)a[0];
    m->g += (double)a[1];
    m->r += (double)a[2];
    m->e += (double)a[3];
    m->gg += (double)a[4];
    m->rr += (double)a[5];
    m->gr += (double)a[6];
    m->eg += (double)a[7];
    m->er += (double)a[8];
}

static int dense_vector_fit(QliDenseFit *b, const DensePlanes *v, const uint8_t *dirty)
{
    const QliDenseKernels *kernels = qli_dense_kernels();
    const QliDenseImage *p = b->source;
    if (p->channels == 1) {
        return 1;
    }
    if (dirty) {
        size_t r = 0;
        while (r < b->state[0].regions && !dirty[r]) {
            ++r;
        }
        if (r == b->state[0].regions) {
            return 1;
        }
    }
    DenseFitCell *cells[3] = {0};
    int ok = 0;
    int16_t samples[3][4][256];
    for (unsigned k = 0; k < b->count; ++k) {
        cells[k] = qli_calloc(2 * b->state[k].regions, sizeof(DenseFitCell));
        if (!cells[k]) {
            goto done;
        }
    }
    for (unsigned stage = 0; stage < 2; ++stage) {
        for (unsigned ty = 0; ty < p->h; ty += 16) {
            for (unsigned tx = 0; tx < p->w; tx += 16) {
                size_t tile = tile_at(p->w, tx, ty);
                if (dirty && !dirty[region_at(p->w, tx, ty, b->state[0].log)]) {
                    continue;
                }
                unsigned n = dense_samples_load(p, v, tx, ty, samples);
                for (unsigned k = 0; k < b->count; ++k) {
                    QliDenseFitState *s = b->state + k;
                    size_t r = region_at(p->w, tx, ty, s->log);
                    unsigned mask = dirty ? dirty[r] : 3,
                             modes[2] = {s->map[tile] & 15, s->map[2 * p->tiles + tile] & 15};
                    if (!stage) {
                        int64_t local[2][9];
                        kernels->moments(samples, n, modes, mask, local);
                        for (unsigned c = 0; c < 2; ++c) {
                            if (mask & (1u << c)) {
                                dense_moments_add(&cells[k][2 * r + c].moment, local[c]);
                            }
                        }
                    } else {
                        int32_t a[2][3] = {{0}};
                        uint64_t local[2][3];
                        for (unsigned c = 0; c < 2; ++c) {
                            if (mask & (1u << c)) {
                                DenseProposal *q = &cells[k][2 * r + c].proposal;
                                a[c][0] = q->kg;
                                a[c][1] = q->ag;
                                a[c][2] = q->ar;
                            }
                        }
                        kernels->costs(samples, n, modes, a, mask, local);
                        for (unsigned c = 0; c < 2; ++c) {
                            if (mask & (1u << c)) {
                                for (unsigned j = 0; j < 3; ++j) {
                                    cells[k][2 * r + c].proposal.cost[j] += local[c][j];
                                }
                            }
                        }
                    }
                }
            }
        }
        if (!stage) {
            for (unsigned k = 0; k < b->count; ++k) {
                for (size_t r = 0; r < b->state[k].regions; ++r) {
                    for (unsigned c = 0; c < 2; ++c) {
                        if (!dirty || (dirty[r] & (1u << c))) {
                            DenseFitCell *f = cells[k] + 2 * r + c;
                            f->proposal = dense_propose(f->moment, c * 2);
                        }
                    }
                }
            }
        }
    }
    for (unsigned k = 0; k < b->count; ++k) {
        for (size_t r = 0; r < b->state[k].regions; ++r) {
            uint8_t *co = b->state[k].coeff + 3 * r;
            for (unsigned c = 0; c < 2; ++c) {
                if (!dirty || (dirty[r] & (1u << c))) {
                    DenseProposal *a = &cells[k][2 * r + c].proposal;
                    unsigned best = 0;
                    for (unsigned j = 1; j < (c ? 3u : 2u); ++j) {
                        if (a->cost[j] < a->cost[best]) {
                            best = j;
                        }
                    }
                    if (!c) {
                        co[0] = (uint8_t)(32 + (best ? a->kg : 0));
                    } else {
                        co[1] = (uint8_t)(32 + (best == 1 ? a->kg : best == 2 ? a->ag : 0));
                        co[2] = (uint8_t)(32 + (best == 2 ? a->ar : 0));
                    }
                }
            }
        }
    }
    ok = 1;
done:
    for (unsigned k = 0; k < 3; ++k) {
        qli_free(cells[k]);
    }
    return ok;
}

static void dense_vector_modes(QliDenseFit *b, const DensePlanes *v, const uint8_t *search,
                               uint8_t *dirty)
{
    const QliDenseKernels *kernels = qli_dense_kernels();
    const QliDenseImage *p = b->source;
    QliDenseFitState *s = b->state;
    if (p->channels == 1) {
        return;
    }
    int16_t samples[3][4][256];
    for (unsigned ty = 0; ty < p->h; ty += 16) {
        for (unsigned tx = 0; tx < p->w; tx += 16) {
            size_t r = region_at(p->w, tx, ty, s->log), tile = tile_at(p->w, tx, ty);
            unsigned mask = search[r];
            if (!mask) {
                continue;
            }
            unsigned n = dense_samples_load(p, v, tx, ty, samples);
            const uint8_t *co = s->coeff + 3 * r;
            int a[3] = {(int)co[0] - 32, (int)co[1] - 32, (int)co[2] - 32};
            uint32_t costs[2][16];
            kernels->mode_costs(samples, n, a, mask, costs);
            for (unsigned c = 0; c < 2; ++c) {
                if (mask & (1u << c)) {
                    size_t at = (c ? 2 : 0) * p->tiles + tile;
                    unsigned previous = s->map[at] & 15, best = previous;
                    for (unsigned m = 0; m < 16; ++m) {
                        if (costs[c][m] < costs[c][best]) {
                            best = m;
                        }
                    }
                    s->map[at] = (uint8_t)((s->map[at] & 112) | best);
                    if (best != previous) {
                        dirty[r] |= (uint8_t)(1u << c);
                    }
                }
            }
        }
    }
}

static const int8_t *dense_vector_tile_errors(DenseTileCache *t, const QliDenseImage *p, unsigned ch,
                                              unsigned mode)
{
    if (!(t->filled[ch] & (1u << mode))) {
        const uint8_t *plane = p->planes + ch * p->pixels;
        unsigned at = 0;
        for (unsigned y = t->ty; y < t->ty + t->th; ++y) {
            const uint8_t *row = plane + (size_t)y * p->w;
            t->kernels->score_row(row, y ? row - p->w : NULL, p->w, t->tx, t->tw, mode,
                                  t->errors[ch][mode] + at);
            at += t->tw;
        }
        t->filled[ch] |= (uint16_t)(1u << mode);
    }
    return t->errors[ch][mode];
}

static int dense_initial_fit(QliDenseFit *b, const DensePlanes *v, uint64_t prices[3],
                             DenseFitCell *seed[3])
{
    const QliDenseKernels *kernels = qli_dense_kernels();
    if (!b || !v || !b->source || b->count != 3) {
        return 0;
    }
    const QliDenseImage *p = b->source;
    for (unsigned k = 0; k < 3; ++k) {
        if (!b->state[k].map || !b->state[k].coeff || memcmp(b->state[k].map, p->initial_map, p->maps)) {
            return 0;
        }
        if (prices) {
            prices[k] = UINT64_C(24) * b->state[k].regions;
        }
    }
    if (p->channels == 1) {
        return 1;
    }
    DenseFitCell *cells[3] = {0};
    int16_t samples[3][4][256];
    int ok = 0;
    for (unsigned k = 0; k < 3; ++k) {
        cells[k] = seed ? seed[k] : qli_calloc(2 * b->state[k].regions, sizeof(DenseFitCell));
        if (!cells[k]) {
            goto done;
        }
    }
    if (seed) {
        for (unsigned k = 0; k < 3; ++k) {
            for (size_t r = 0; r < b->state[k].regions; ++r) {
                for (unsigned c = 0; c < 2; ++c) {
                    DenseFitCell *cell = cells[k] + 2 * r + c;
                    cell->proposal = dense_propose(cell->moment, c * 2);
                }
            }
        }
    }
    for (unsigned stage = seed ? 1 : 0; stage < 2; ++stage) {
        for (unsigned ty = 0; ty < p->h; ty += 16) {
            for (unsigned tx = 0; tx < p->w; tx += 16) {
                size_t tile = tile_at(p->w, tx, ty), region[3];
                unsigned n = dense_samples_load(p, v, tx, ty, samples);
                unsigned modes[2] = {p->initial_map[tile] & 15,
                                     p->initial_map[2 * p->tiles + tile] & 15};
                for (unsigned k = 0; k < 3; ++k) {
                    region[k] = region_at(p->w, tx, ty, b->state[k].log);
                }
                if (!stage) {
                    int64_t local[2][9];
                    kernels->moments(samples, n, modes, 3, local);
                    for (unsigned k = 0; k < 3; ++k) {
                        for (unsigned c = 0; c < 2; ++c) {
                            dense_moments_add(&cells[k][2 * region[k] + c].moment, local[c]);
                        }
                    }

                } else {
                    int32_t a[3][2][3];
                    uint64_t local[3][2][3];
                    for (unsigned k = 0; k < 3; ++k) {
                        for (unsigned c = 0; c < 2; ++c) {
                            DenseProposal *q = &cells[k][2 * region[k] + c].proposal;
                            a[k][c][0] = q->kg;
                            a[k][c][1] = q->ag;
                            a[k][c][2] = q->ar;
                        }
                    }
                    kernels->shared_costs(samples, n, modes, a, local, NULL);
                    for (unsigned k = 0; k < 3; ++k) {
                        for (unsigned c = 0; c < 2; ++c) {
                            for (unsigned j = 0; j < 3; ++j) {
                                cells[k][2 * region[k] + c].proposal.cost[j] += local[k][c][j];
                            }
                        }
                    }
                }
            }
        }
        if (!stage) {
            for (unsigned k = 0; k < 3; ++k) {
                for (size_t r = 0; r < b->state[k].regions; ++r) {
                    for (unsigned c = 0; c < 2; ++c) {
                        DenseFitCell *f = cells[k] + 2 * r + c;
                        f->proposal = dense_propose(f->moment, c * 2);
                    }
                }
            }
        }
    }
    for (unsigned k = 0; k < 3; ++k) {
        for (size_t r = 0; r < b->state[k].regions; ++r) {
            uint8_t *co = b->state[k].coeff + 3 * r;
            for (unsigned c = 0; c < 2; ++c) {
                DenseProposal *a = &cells[k][2 * r + c].proposal;
                unsigned best = 0;
                for (unsigned j = 1; j < (c ? 3u : 2u); ++j) {
                    if (a->cost[j] < a->cost[best]) {
                        best = j;
                    }
                }
                if (prices) {
                    prices[k] += a->cost[best];
                }
                if (!c) {
                    co[0] = (uint8_t)(32 + (best ? a->kg : 0));
                } else {
                    co[1] = (uint8_t)(32 + (best == 1 ? a->kg : best == 2 ? a->ag : 0));
                    co[2] = (uint8_t)(32 + (best == 2 ? a->ar : 0));
                }
            }
        }
    }
    ok = 1;
done:
    for (unsigned k = 0; k < 3; ++k) {
        if (!seed) {
            qli_free(cells[k]);
        }
    }
    return ok;
}

static uint8_t dense_seed_symbol(uint8_t x, uint8_t p) { return dense_symbol((int)x - p); }

void qli_dense_image_free(QliDenseImage *p)
{
    if (p) {
        qli_free(p->planes);
        qli_free(p);
    }
}

static void dense_cached_seed_banks(const uint8_t *planes, uint16_t *storage, uint32_t w, uint32_t h,
                                    unsigned channels, size_t tiles, uint8_t *map)
{
    const QliDenseKernels *kernels = qli_dense_kernels();
    size_t pixels = (size_t)w * h;
    uint16_t (*hist)[256] = (uint16_t (*)[256])storage;
    for (unsigned c = 0; c < channels; ++c) {
        const uint8_t *plane = planes + c * pixels;
        uint8_t *labels = map + c * tiles;
        size_t tile = 0;
        memset(storage, 0, tiles * 256 * sizeof(uint16_t));
        for (uint32_t ty = 0; ty < h; ty += 16) {
            for (uint32_t tx = 0; tx < w; tx += 16, ++tile) {
                unsigned mode = labels[tile] & 15;
                for (uint32_t y = ty; y < h && y < ty + 16; ++y) {
                    for (uint32_t x = tx; x < w && x < tx + 16; ++x) {
                        size_t pos = (size_t)y * w + x;
                        ++hist[tile][dense_seed_symbol(plane[pos],
                                                       plane_prediction(plane, pos, x, y, w, mode))];
                    }
                }
            }
        }
        for (unsigned iteration = 0; iteration < 3; ++iteration) {
            uint64_t banks[8][256] = {{0}}, totals[8] = {0};
            float price[256][8];
            for (tile = 0; tile < tiles; ++tile) {
                unsigned g = labels[tile] >> 4;
                for (unsigned s = 0; s < 256; ++s) {
                    if (hist[tile][s]) {
                        banks[g][s] += hist[tile][s];
                        totals[g] += hist[tile][s];
                    }
                }
            }
            for (unsigned g = 0; g < 8; ++g) {
                for (unsigned s = 0; s < 256; ++s) {
                    price[s][g] =
                        (float)-log2(((double)banks[g][s] + 0.5) / ((double)totals[g] + 128.0));
                }
            }
            unsigned changed = 0;
            for (tile = 0; tile < tiles; ++tile) {
                double scores[8];
                kernels->seed_bank_scores(hist[tile], price, scores);
                unsigned previous = labels[tile] >> 4, best = previous;
                for (unsigned g = 0; g < 8; ++g) {
                    if (scores[g] < scores[best]) {
                        best = g;
                    }
                }
                changed += best != previous;
                labels[tile] = (uint8_t)((labels[tile] & 15) | (best << 4));
            }
            if (!changed) {
                break;
            }
        }
    }
}

static QliDenseImage *dense_prepare_image(const uint8_t *raw, size_t bytes, uint32_t w,
                                          uint32_t h, int *direct_gray, uint16_t **seed_symbols)
{
    unsigned input_channels = 4;
    if (direct_gray && (uint64_t)w * h * 3 == bytes) input_channels = 3;
    if (!raw || !w || !h || w > 32768 || h > 32768 || (uint64_t)w * h > (1u << 25) ||
        (uint64_t)w * h * input_channels != bytes) {
        return NULL;
    }
    size_t pixels = (size_t)w * h;
    unsigned opaque = 1, gray = 1, equal_rgb = direct_gray != NULL;
    for (size_t i = 0; i < pixels; ++i) {
        const uint8_t *s = raw + input_channels * i;
        if (input_channels == 4) opaque &= s[3] == 255;
        gray &= (uint8_t)(s[0] + 128) == s[1] && (uint8_t)(s[2] + 128) == s[1];
        equal_rgb &= s[0] == s[1] && s[1] == s[2];
    }
    if (direct_gray) {
        *direct_gray = opaque && equal_rgb;
    }
    unsigned channels = opaque ? (gray || equal_rgb ? 1 : 3) : 4;
    size_t tiles = (size_t)((w + 15) / 16) * ((h + 15) / 16), maps = tiles * channels;
    QliDenseImage *p = qli_calloc(1, sizeof(*p));
    if (!p) {
        return NULL;
    }
    p->planes = qli_malloc(pixels * channels + maps);
    if (seed_symbols) {
        *seed_symbols = qli_malloc(tiles * 256 * sizeof(**seed_symbols));
    }
    if (!p->planes || (seed_symbols && !*seed_symbols)) {
        qli_dense_image_free(p);
        return NULL;
    }
    p->w = w;
    p->h = h;
    p->pixels = pixels;
    p->tiles = tiles;
    p->maps = maps;
    p->channels = channels;
    p->streams = channels * 8;
    p->raw_crc = qint_crc32(raw, bytes);
    p->initial_map = p->planes + pixels * channels;
    for (size_t i = 0; i < pixels; ++i) {
        const uint8_t *s = raw + input_channels * i;
        if (channels == 1) {
            p->planes[i] = s[1];
        } else {
            p->planes[i] = (uint8_t)((int)s[0] - s[1] + 128);
            p->planes[pixels + i] = s[1];
            p->planes[2 * pixels + i] = (uint8_t)((int)s[2] - s[1] + 128);
            if (channels == 4) {
                p->planes[3 * pixels + i] = s[3];
            }
        }
    }
    return p;
}

static unsigned dense_seed_bank(uint32_t magnitude, unsigned count)
{
    if (!magnitude) return 0;
    if (magnitude <= count / 2) return 1;
    if (magnitude <= count) return 2;
    if (magnitude <= count * 2) return 3;
    if (magnitude <= count * 4) return 4;
    if (magnitude <= count * 8) return 5;
    return magnitude <= count * 16 ? 6 : 7;
}

int qli_dense_prepare(const uint8_t *raw, size_t bytes, uint32_t w, uint32_t h, QliDenseImage **out)
{
    const QliDenseKernels *kernels = qli_dense_kernels();
    if (!out) {
        return 0;
    }
    *out = NULL;
    uint16_t *symbols = NULL;
    QliDenseImage *p = dense_prepare_image(raw, bytes, w, h, NULL, &symbols);
    if (!p) {
        qli_free(symbols);
        return 0;
    }
    const size_t pixels = p->pixels, tiles = p->tiles;
    const unsigned channels = p->channels;
    uint8_t *map = p->planes + pixels * channels;
    int16_t samples[4][256];
    for (unsigned c = 0; c < channels; ++c) {
        const uint8_t *plane = p->planes + c * pixels;
        size_t tile = 0;
        for (unsigned ty = 0; ty < h; ty += 16) {
            for (unsigned tx = 0; tx < w; tx += 16, ++tile) {
                unsigned tw = w - tx < 16 ? w - tx : 16, th = h - ty < 16 ? h - ty : 16, n = tw * th,
                         at = 0;
                for (unsigned y = ty; y < ty + th; ++y) {
                    for (unsigned x = tx; x < tx + tw; ++x, ++at) {
                        size_t i = (size_t)y * w + x;
                        samples[0][at] = plane[i];
                        samples[1][at] = x ? plane[i - 1] : 0;
                        samples[2][at] = y ? plane[i - w] : 0;
                        samples[3][at] = x && y ? plane[i - w - 1] : 0;
                    }
                }
                for (unsigned j = 0; j < 4; ++j) {
                    for (unsigned i = n; i < ((n + 15) & ~15u); ++i) {
                        samples[j][i] = 0;
                    }
                }
                uint32_t scores[16], mags[16];
                kernels->seed_scores(samples, n, scores, mags);
                unsigned best = 0;
                for (unsigned m = 1; m < 16; ++m) {
                    if (scores[m] < scores[best]) {
                        best = m;
                    }
                }
                map[c * tiles + tile] = (uint8_t)(best | (dense_seed_bank(mags[best], n) << 4));
            }
        }
    }
    dense_cached_seed_banks(p->planes, symbols, w, h, channels, tiles, map);
    qli_free(symbols);
    *out = p;
    return 1;
}

void qli_dense_field_free(QliDenseField *f)
{
    if (f) {
        qli_free(f->symbols);
        qli_free(f);
    }
}

int qli_dense_field_create(const QliDenseFit *fit, QliDenseField **result)
{
    if (result) {
        *result = NULL;
    }
    if (!result || !fit || fit->count != 1 || !fit->source) {
        return 0;
    }
    const QliDenseImage *p = fit->source;
    const QliDenseFitState *s = fit->state;
    QliDenseField *f = qli_calloc(1, sizeof(*f));
    uint16_t (*hist)[256] = NULL;
    int ok = 0;
    if (!f) {
        return 0;
    }
    f->w = p->w;
    f->h = p->h;
    f->raw_crc = p->raw_crc;
    f->channels = p->channels;
    f->streams = p->streams;
    f->log = s->log;
    f->pixels = p->pixels;
    f->tiles = p->tiles;
    f->maps = p->maps;
    f->coefficient_bytes = 3 * s->regions;
    f->symbols = qli_malloc(p->pixels * p->channels + f->maps + f->coefficient_bytes);
    hist = qli_malloc(f->tiles * sizeof(*hist));
    if (!f->symbols || !hist) {
        goto done;
    }
    f->map = f->symbols + f->pixels * f->channels;
    f->coefficients = f->map + f->maps;
    memcpy(f->map, s->map, f->maps);
    memcpy(f->coefficients, s->coeff, f->coefficient_bytes);
    DenseTileCache cache;
    cache.kernels = qli_dense_kernels();
    size_t event = 0;
    for (unsigned c = 0; c < f->channels; ++c) {
        memset(hist, 0, f->tiles * sizeof(*hist));
        size_t tile = 0;
        for (unsigned ty = 0; ty < f->h; ty += 16) {
            for (unsigned tx = 0; tx < f->w; tx += 16, ++tile) {
                dense_tile_start(&cache, p, tx, ty);
                unsigned mode = f->map[c * f->tiles + tile] & 15;
                const uint8_t *co = f->coefficients + 3 * region_at(f->w, tx, ty, f->log);
                int a = f->channels > 1 && c == 0   ? (int)co[0] - 32
                        : f->channels > 1 && c == 2 ? (int)co[1] - 32
                                                    : 0;
                int rc = f->channels > 1 && c == 2 ? (int)co[2] - 32 : 0;
                const int8_t *own = dense_vector_tile_errors(&cache, p, c, mode);
                const int8_t *green = a ? dense_vector_tile_errors(&cache, p, 1, mode) : NULL;
                const int8_t *red = rc ? dense_vector_tile_errors(&cache, p, 0, mode) : NULL;
                for (unsigned i = 0; i < cache.count; ++i) {
                    int correction =
                        (green ? tile_floor16(a * green[i]) : 0) + (red ? tile_floor16(rc * red[i]) : 0);
                    uint8_t value = dense_symbol((int)own[i] - correction);
                    f->symbols[event++] = value;
                    ++hist[tile][value];
                }
            }
        }
        for (unsigned iteration = 0; iteration < 3; ++iteration) {
            uint64_t banks[8][256] = {{0}}, totals[8] = {0};
            float price[256][8];
            for (tile = 0; tile < f->tiles; ++tile) {
                unsigned bank = f->map[c * f->tiles + tile] >> 4;
                for (unsigned value = 0; value < 256; ++value) {
                    if (hist[tile][value]) {
                        banks[bank][value] += hist[tile][value];
                        totals[bank] += hist[tile][value];
                    }
                }
            }
            for (unsigned bank = 0; bank < 8; ++bank) {
                for (unsigned value = 0; value < 256; ++value) {
                    price[value][bank] =
                        (float)-log2(((double)banks[bank][value] + 0.5) / ((double)totals[bank] + 128));
                }
            }
            unsigned changed = 0;
            for (tile = 0; tile < f->tiles; ++tile) {
                double scores[8];
                cache.kernels->final_bank_scores(hist[tile], price, scores);
                uint8_t *code = f->map + c * f->tiles + tile;
                unsigned previous = *code >> 4, best = previous;
                for (unsigned bank = 0; bank < 8; ++bank) {
                    if (scores[bank] < scores[best]) {
                        best = bank;
                    }
                }
                changed += previous != best;
                *code = (uint8_t)((*code & 15) | (best << 4));
            }
            if (!changed) {
                break;
            }
        }
    }
    if (event != f->pixels * f->channels) {
        goto done;
    }
    *result = f;
    f = NULL;
    ok = 1;
done:
    qli_free(hist);
    qli_dense_field_free(f);
    return ok;
}

static int dense_write_tiles(const QliDenseField *f, uint8_t **result, size_t *size, int structural,
                             int direct_gray)
{
    if (result) {
        *result = NULL;
    }
    if (size) {
        *size = 0;
    }
    if (!f || !result || !size) {
        return 0;
    }
    uint8_t *pool = qli_malloc(f->pixels * f->channels), *out = NULL, *encoded[32] = {0};
    size_t counts[32] = {0}, cursor[32] = {0}, sizes[32] = {0};
    int ok = 0;
    if (!pool) {
        goto done;
    }
    for (unsigned c = 0; c < f->channels; ++c) {
        size_t tile = 0;
        for (unsigned ty = 0; ty < f->h; ty += 16) {
            for (unsigned tx = 0; tx < f->w; tx += 16, ++tile) {
                counts[c * 8 + (f->map[c * f->tiles + tile] >> 4)] +=
                    qli_dense_tile_pixels(tx, ty, f->w, f->h);
            }
        }
    }
    size_t at = 0;
    for (unsigned stream = 0; stream < f->streams; ++stream) {
        cursor[stream] = at;
        at += counts[stream];
    }
    if (at != f->pixels * f->channels) {
        goto done;
    }
    size_t event = 0;
    for (unsigned c = 0; c < f->channels; ++c) {
        size_t tile = 0;
        for (unsigned ty = 0; ty < f->h; ty += 16) {
            for (unsigned tx = 0; tx < f->w; tx += 16, ++tile) {
                unsigned stream = c * 8 + (f->map[c * f->tiles + tile] >> 4);
                size_t n = qli_dense_tile_pixels(tx, ty, f->w, f->h);
                memcpy(pool + cursor[stream], f->symbols + event, n);
                cursor[stream] += n;
                event += n;
            }
        }
    }
    size_t total =
               DENSE_TILE_HEADER + f->maps + f->coefficient_bytes + 4 * f->streams + DENSE_TILE_FOOTER,
           offset = 0;
    for (unsigned stream = 0; stream < f->streams; ++stream) {
        int written = structural
            ? qli_four_encode(pool + offset, counts[stream], NULL, NULL, &encoded[stream], &sizes[stream])
            : write_four_separate(pool + offset, counts[stream], &encoded[stream], &sizes[stream], NULL);
        if (!written ||
            sizes[stream] > DENSE_FILE_CAP || total > DENSE_FILE_CAP - sizes[stream]) {
            goto done;
        }
        total += sizes[stream];
        offset += counts[stream];
    }
    out = qli_calloc(1, total);
    if (!out) {
        goto done;
    }
    memcpy(out, "QEP1", 4);
    qli_write_le32(out + 4, f->w);
    qli_write_le32(out + 8, f->h);
    out[12] = (uint8_t)f->channels;
    out[13] = direct_gray ? 2 : 1;
    out[14] = 4;
    out[15] = 16;
    qli_write_le32(out + 16, (uint32_t)f->maps);
    qli_write_le32(out + 20, f->streams);
    qli_write_le32(out + 24, f->raw_crc);
    qli_write_le32(out + 28, f->log);
    qli_write_le32(out + 32, (uint32_t)f->coefficient_bytes);
    memcpy(out + DENSE_TILE_HEADER, f->map, f->maps);
    memcpy(out + DENSE_TILE_HEADER + f->maps, f->coefficients, f->coefficient_bytes);
    size_t lengths = DENSE_TILE_HEADER + f->maps + f->coefficient_bytes;
    offset = lengths + 4 * f->streams;
    for (unsigned stream = 0; stream < f->streams; ++stream) {
        qli_write_le32(out + lengths + 4 * stream, (uint32_t)sizes[stream]);
        memcpy(out + offset, encoded[stream], sizes[stream]);
        offset += sizes[stream];
    }
    qli_write_le32(out + total - 4, qint_crc32(out, total - 4));
    *result = out;
    *size = total;
    out = NULL;
    ok = 1;
done:
    qli_free(pool);
    qli_free(out);
    for (unsigned s = 0; s < 32; ++s) {
        qli_free(encoded[s]);
    }
    return ok;
}

int qli_dense_write_tiles(const QliDenseField *f, uint8_t **out, size_t *bytes)
{
    return dense_write_tiles(f, out, bytes, 0, 0);
}

int qli_structural_write_tiles(const QliDenseField *f, int direct_gray, uint8_t **out, size_t *bytes)
{
    return dense_write_tiles(f, out, bytes, 1, direct_gray);
}

static int dense_refine_fit(QliDenseFit *fit, const DensePlanes *planes, unsigned passes)
{
    const QliDenseFitState *s = fit->state;
    uint8_t *previous = qli_malloc(3 * s->regions);
    uint8_t *search = qli_malloc(s->regions);
    uint8_t *dirty = qli_malloc(s->regions);
    int ok = 0;
    if (!previous || !search || !dirty) goto done;
    memset(previous, 32, 3 * s->regions);
    for (unsigned pass = 0; pass < passes; ++pass) {
        for (size_t r = 0; r < s->regions; ++r) {
            const uint8_t *a = s->coeff + 3 * r, *old = previous + 3 * r;
            search[r] = (uint8_t)((a[0] != old[0] ? 1 : 0) |
                                  ((a[1] != old[1] || a[2] != old[2]) ? 2 : 0));
        }
        memcpy(previous, s->coeff, 3 * s->regions);
        memset(dirty, 0, s->regions);
        dense_vector_modes(fit, planes, search, dirty);
        if (!dense_vector_fit(fit, planes, dirty)) goto done;
    }
    ok = 1;
done:
    qli_free(previous);
    qli_free(search);
    qli_free(dirty);
    return ok;
}

static void dense_free_seed(DenseFitCell *seed[3])
{
    for (unsigned k = 0; k < 3; ++k) {
        qli_free(seed[k]);
        seed[k] = NULL;
    }
}

static int dense_fit(const QliDenseImage *p, DenseFitCell *seed[3], unsigned passes,
                     int structural, QliDenseFit **result)
{
    *result = NULL;
    QliDenseFit *b = qli_calloc(1, sizeof(*b));
    DensePlanes v = {0};
    int ok = 0;
    if (!b) {
        return 0;
    }
    b->source = p;
    b->count = 3;
    for (unsigned k = 0; k < 3; ++k) {
        QliDenseFitState *s = b->state + k;
        s->log = dense_fit_logs[k];
        s->regions = region_count(p->w, p->h, s->log);
        s->map = qli_malloc(p->maps);
        s->coeff = qli_malloc(3 * s->regions);
        if (!s->map || !s->coeff) {
            goto done;
        }
        memcpy(s->map, p->initial_map, p->maps);
        memset(s->coeff, 32, 3 * s->regions);
    }
    uint64_t prices[3];
    if ((!structural && !dense_planes_create(p, &v)) ||
        !dense_initial_fit(b, &v, structural ? prices : NULL, seed)) {
        goto done;
    }
    if (seed) dense_free_seed(seed);
    if (structural) {
        unsigned best = 0;
        if (p->channels > 1) {
            for (unsigned k = 1; k < 3; ++k)
                if (prices[k] < prices[best]) best = k;
        }
        QliDenseFitState selected = b->state[best];
        for (unsigned k = 0; k < 3; ++k) {
            if (k == best) continue;
            qli_free(b->state[k].map);
            qli_free(b->state[k].coeff);
        }
        b->state[0] = selected;
        b->count = 1;
    }
    if (p->channels > 1) {
        for (unsigned k = 0; k < b->count; ++k) {
            QliDenseFit view = { .source = p, .state = {b->state[k]}, .count = 1 };
            if (!dense_refine_fit(&view, &v, passes)) goto done;
        }
    }
    *result = b;
    b = NULL;
    ok = 1;
done:
    qli_free(v.data);
    qli_dense_fit_free(b);
    return ok;
}

int qli_dense_fit_sample(const QliDenseImage *p, QliDenseFit **result)
{
    return dense_fit(p, NULL, 2, 0, result);
}

enum { DENSE_HISTOGRAM_BYTES = 1 + 32 + 256 };

static int dense_structure_native(void)
{
#if QLI_DENSE_STRUCTURE_X86
    return (qli_cpu_capabilities() & QLI_CPU_SSE42) != 0;
#else
    return 0;
#endif
}

#if QLI_DENSE_STRUCTURE_X86
static inline __m128i dense_load16(const void *p)
{
    __m128i row;
    memcpy(&row,p,sizeof(row));
    return row;
}

static inline void dense_store16(void *p, __m128i row)
{
    memcpy(p,&row,sizeof(row));
}

static void dense_seed_words(int16_t *out, __m128i bytes)
{
    const __m128i zero = _mm_setzero_si128();
    dense_store16(out, _mm_unpacklo_epi8(bytes, zero));
    dense_store16(out + 8, _mm_unpackhi_epi8(bytes, zero));
}
#endif

static unsigned dense_seed_tile(const QliDenseImage *image, unsigned channel, unsigned tx,
                                unsigned ty, int16_t out[4][256], int native)
{
    const unsigned width = image->w - tx < 16 ? image->w - tx : 16;
    const unsigned height = image->h - ty < 16 ? image->h - ty : 16;
    const uint8_t *plane = image->planes + channel * image->pixels;
    unsigned count = 0;
    for (unsigned y = ty; y < ty + height; ++y, count += width) {
        const uint8_t *row = plane + (size_t)y * image->w + tx;
        const uint8_t *top = y ? row - image->w : NULL;
#if QLI_DENSE_STRUCTURE_X86
        if (native && width == 16) {
            const __m128i current = dense_load16(row);
            const __m128i above = top ? dense_load16(top) : _mm_setzero_si128();
            const __m128i left = tx ? dense_load16(row - 1) : _mm_slli_si128(current, 1);
            const __m128i corner = top && tx ? dense_load16(top - 1) : _mm_slli_si128(above, 1);
            dense_seed_words(out[0] + count, current);
            dense_seed_words(out[1] + count, left);
            dense_seed_words(out[2] + count, above);
            dense_seed_words(out[3] + count, corner);
            continue;
        }
#else
        (void)native;
#endif
        unsigned left = tx ? row[-1] : 0, corner = tx && top ? top[-1] : 0;
        for (unsigned x=0; x<width; ++x) {
            const unsigned sample = row[x], above = top ? top[x] : 0;
            out[0][count+x] = (int16_t)sample;
            out[1][count+x] = (int16_t)left;
            out[2][count+x] = (int16_t)above;
            out[3][count+x] = (int16_t)corner;
            left=sample;
            corner=above;
        }
    }
    unsigned padding = ((count + 7u) & ~7u) - count;
    for (unsigned c = 0; c < 4; ++c) memset(out[c] + count, 0, padding * sizeof(int16_t));
    return count;
}

static int dense_structure_seeds(QliDenseImage *image, DenseFitCell *seed[3])
{
    const QliDenseKernels *kernels = qli_dense_kernels();
    const int native = dense_structure_native();
    uint8_t *map = image->planes + image->pixels * image->channels;
    const unsigned order[3] = {1, 0, 2};
    int16_t samples[3][4][256], extra[4][256];
    if (seed) {
        for (unsigned k = 0; k < 3; ++k) {
            seed[k] = qli_calloc(2 * region_count(image->w, image->h, dense_fit_logs[k]),
                                  sizeof(*seed[k]));
            if (!seed[k]) return 0;
        }
    }
    size_t tile = 0;
    for (unsigned y = 0; y < image->h; y += 16) {
        for (unsigned x = 0; x < image->w; x += 16, ++tile) {
            unsigned count = 0;
            for (unsigned c = 0; c < image->channels; ++c) {
                int16_t (*view)[256] = seed && c < 3 ? samples[order[c]] : extra;
                count = dense_seed_tile(image, c, x, y, view, native);
                uint32_t magnitude;
                unsigned mode = kernels->seed_best(view, count, &magnitude);
                map[c * image->tiles + tile] = (uint8_t)(mode | (dense_seed_bank(magnitude, count) << 4));
            }
            if (seed) {
                unsigned modes[2] = {map[tile] & 15, map[2 * image->tiles + tile] & 15};
                int64_t moments[2][9];
                kernels->moments(samples, count, modes, 3, moments);
                for (unsigned k = 0; k < 3; ++k) {
                    size_t region = region_at(image->w, x, y, dense_fit_logs[k]);
                    for (unsigned c = 0; c < 2; ++c)
                        dense_moments_add(&seed[k][2 * region + c].moment, moments[c]);
                }
            }
        }
    }
    return 1;
}

static unsigned dense_histogram_first(uint64_t bits)
{
#if defined(__x86_64__) && (defined(__clang__) || defined(__GNUC__))
    uint64_t index;
    __asm__("bsfq %1, %0" : "=r"(index) : "r"(bits) : "cc");
    return (unsigned)index;
#elif defined(__clang__) || defined(__GNUC__)
    return (unsigned)__builtin_ctzll(bits);
#else
    unsigned index = 0;
    while (!(bits & 1)) { bits >>= 1; ++index; }
    return index;
#endif
}

static unsigned dense_histogram_count(unsigned count) { return count ? count : 256; }

static size_t dense_histogram_pack(const uint8_t *symbols, unsigned count, uint8_t *out,
                                   uint64_t bank[256], int native)
{
    uint8_t histogram[256] = {0};
    uint64_t support[4] = {0};
    unsigned i = 0;
    for (; count - i >= 16; i += 16) {
        unsigned equal;
#if QLI_DENSE_STRUCTURE_X86
        if (native) {
            const __m128i values = dense_load16(symbols+i);
            equal = _mm_movemask_epi8(_mm_cmpeq_epi8(values,_mm_set1_epi8((char)symbols[i]))) == 65535;
        } else
#else
        (void)native;
#endif
        {
            uint64_t a, b;
            memcpy(&a,symbols+i,8);
            memcpy(&b,symbols+i+8,8);
            equal = a == b && a == (uint64_t)symbols[i]*UINT64_C(0x0101010101010101);
        }
        if (equal) histogram[symbols[i]] = (uint8_t)(histogram[symbols[i]] + 16);
        else for (unsigned k = 0; k < 16; ++k) ++histogram[symbols[i + k]];
    }
    for (; i < count; ++i) ++histogram[symbols[i]];
    unsigned entries = 0;
    if (!histogram[symbols[0]]) {
        support[symbols[0] >> 6] = UINT64_C(1) << (symbols[0] & 63);
        entries = 1;
    } else {
#if QLI_DENSE_STRUCTURE_X86
        if (native) {
            for (unsigned s = 0; s < 256; s += 16) {
                const __m128i values = dense_load16(histogram+s);
                const unsigned bits = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(values,_mm_setzero_si128())) ^ 65535;
                support[s >> 6] |= (uint64_t)bits << (s & 63);
                entries += qlic_count_bits(bits);
            }
        } else
#endif
        for (unsigned s=0; s<256; ++s) {
            if (histogram[s]) {
                support[s >> 6] |= UINT64_C(1) << (s & 63);
                ++entries;
            }
        }
    }
    const int sparse = 1 + 2 * entries <= DENSE_HISTOGRAM_BYTES;
    out[0] = sparse ? (uint8_t)entries : 0;
    if (!sparse) {
        memcpy(out + 1, support, sizeof(support));
        memcpy(out + 1 + sizeof(support), histogram, sizeof(histogram));
    }
    size_t at = 1;
    for (unsigned w = 0; w < 4; ++w) {
        for (uint64_t bits = support[w]; bits; bits &= bits - 1) {
            unsigned value = 64 * w + dense_histogram_first(bits);
            bank[value] += dense_histogram_count(histogram[value]);
            if (sparse) {
                out[at++] = (uint8_t)value;
                out[at++] = histogram[value];
            }
        }
    }
    return sparse ? at : DENSE_HISTOGRAM_BYTES;
}

typedef struct {
    const uint8_t *histogram;
    unsigned index;
    unsigned word;
    uint64_t bits;
} DenseHistogramCursor;

static int dense_histogram_next(DenseHistogramCursor *cursor, unsigned *value, unsigned *count)
{
    const uint8_t *p = cursor->histogram;
    if (p[0]) {
        if (cursor->index == p[0]) return 0;
        unsigned i = cursor->index++;
        *value = p[1 + 2 * i];
        *count = dense_histogram_count(p[2 + 2 * i]);
        return 1;
    }
    while (!cursor->bits) {
        if (cursor->word == 4) return 0;
        memcpy(&cursor->bits, p + 1 + 8 * cursor->word, 8);
        ++cursor->word;
    }
    *value = 64 * (cursor->word - 1) + dense_histogram_first(cursor->bits);
    cursor->bits &= cursor->bits - 1;
    *count = dense_histogram_count(p[33 + *value]);
    return 1;
}

static unsigned dense_histogram_move(const uint8_t *histogram, uint64_t from[256], uint64_t to[256])
{
    DenseHistogramCursor cursor = { .histogram = histogram };
    unsigned value, count, moved = 0;
    while (dense_histogram_next(&cursor, &value, &count)) {
        from[value] -= count;
        to[value] += count;
        moved += count;
    }
    return moved;
}

static void dense_histogram_scores(const uint8_t *histogram, const float price[256][8],
                                   double scores[8], int native)
{
    DenseHistogramCursor cursor = { .histogram = histogram };
    unsigned value, count;
#if QLI_DENSE_STRUCTURE_X86
    if (native) {
        const __m128d zero = _mm_setzero_pd();
        __m128d sums[4] = {zero,zero,zero,zero};
        while (dense_histogram_next(&cursor, &value, &count)) {
            const __m128 factor = _mm_set1_ps((float)count);
            for (unsigned k = 0; k < 2; ++k) {
                const __m128 product = _mm_mul_ps(factor, _mm_loadu_ps(price[value] + 4*k));
                sums[2*k] = _mm_add_pd(sums[2*k], _mm_cvtps_pd(product));
                sums[2*k+1] = _mm_add_pd(sums[2*k+1], _mm_cvtps_pd(_mm_movehl_ps(product,product)));
            }
        }
        for (unsigned k=0; k<4; ++k) _mm_storeu_pd(scores+2*k,sums[k]);
        return;
    }
#else
    (void)native;
#endif
    for (unsigned k=0; k<8; ++k) scores[k]=0;
    while (dense_histogram_next(&cursor, &value, &count)) {
        for (unsigned k=0; k<8; ++k) {
            const float product = (float)count*price[value][k];
            scores[k]+=(double)product;
        }
    }
}

static void dense_structure_banks(QliStructuralField *structure, uint8_t *storage, int native)
{
    QliDenseField *f = &structure->field;
    uint32_t *offsets = (uint32_t *)storage;
    uint8_t *data = storage + sizeof(*offsets) * f->tiles;
    for (unsigned c = 0; c < f->channels; ++c) {
        uint64_t banks[8][256] = {{0}}, totals[8] = {0};
        uint8_t *map = f->map + c * f->tiles;
        const uint8_t *symbols = f->symbols + c * f->pixels;
        size_t tile = 0, at = 0;
        for (unsigned y = 0; y < f->h; y += 16) {
            for (unsigned x = 0; x < f->w; x += 16, ++tile) {
                unsigned count = (unsigned)qli_dense_tile_pixels(x, y, f->w, f->h);
                unsigned bank = map[tile] >> 4;
                offsets[tile] = (uint32_t)at;
                at += dense_histogram_pack(symbols, count, data + at, banks[bank], native);
                symbols += count;
                totals[bank] += count;
            }
        }
        for (unsigned pass = 0; pass < 3; ++pass) {
            float price[256][8];
            for (unsigned bank = 0; bank < 8; ++bank)
                for (unsigned value = 0; value < 256; ++value)
                    price[value][bank] = (float)-log2(((double)banks[bank][value] + 0.5) /
                                                      ((double)totals[bank] + 128));
            unsigned changed = 0;
            for (tile = 0; tile < f->tiles; ++tile) {
                const uint8_t *histogram = data + offsets[tile];
                double scores[8];
                dense_histogram_scores(histogram, price, scores, native);
                unsigned previous = map[tile] >> 4, best = previous;
                for (unsigned bank = 0; bank < 8; ++bank)
                    if (scores[bank] < scores[best]) best = bank;
                if (previous != best) {
                    unsigned moved = dense_histogram_move(histogram, banks[previous], banks[best]);
                    totals[previous] -= moved;
                    totals[best] += moved;
                    map[tile] = (uint8_t)((map[tile] & 15) | (best << 4));
                    ++changed;
                }
            }
            if (!changed) break;
        }
        for (unsigned bank = 0; bank < 8; ++bank)
            for (unsigned value = 0; value < 256; ++value)
                structure->joint[c * 8 + bank][value] = (uint32_t)banks[bank][value];
    }
}

static void dense_fold_tile(const int8_t *own, const int8_t *green, const int8_t *red,
                            unsigned count, int a, int rc, uint8_t *out, int native)
{
    unsigned i = 0;
#if QLI_DENSE_STRUCTURE_X86
    if (native) {
        const __m128i zero = _mm_setzero_si128();
        const __m128i byte = _mm_set1_epi16(255);
        const __m128i ga = _mm_set1_epi16((short)a), ra = _mm_set1_epi16((short)rc);
        for (; count - i >= 16; i += 16) {
            __m128i x = dense_load16(own + i);
            __m128i sign = _mm_cmpgt_epi8(zero, x);
            __m128i low = _mm_unpacklo_epi8(x, sign), high = _mm_unpackhi_epi8(x, sign);
            if (a) {
                x = dense_load16(green + i);
                sign = _mm_cmpgt_epi8(zero, x);
                low = _mm_sub_epi16(low, _mm_srai_epi16(_mm_mullo_epi16(
                    ga, _mm_unpacklo_epi8(x, sign)), 4));
                high = _mm_sub_epi16(high, _mm_srai_epi16(_mm_mullo_epi16(
                    ga, _mm_unpackhi_epi8(x, sign)), 4));
            }
            if (rc) {
                x = dense_load16(red + i);
                sign = _mm_cmpgt_epi8(zero, x);
                low = _mm_sub_epi16(low, _mm_srai_epi16(_mm_mullo_epi16(
                    ra, _mm_unpacklo_epi8(x, sign)), 4));
                high = _mm_sub_epi16(high, _mm_srai_epi16(_mm_mullo_epi16(
                    ra, _mm_unpackhi_epi8(x, sign)), 4));
            }
            x = _mm_packus_epi16(_mm_and_si128(low, byte), _mm_and_si128(high, byte));
            dense_store16(out + i, _mm_xor_si128(_mm_add_epi8(x, x),
                                                            _mm_cmpgt_epi8(zero, x)));
        }
    }
#else
    (void)native;
#endif
    for (; i < count; ++i) {
        int correction = (a ? tile_floor16(a * green[i]) : 0) + (rc ? tile_floor16(rc * red[i]) : 0);
        out[i] = dense_symbol((int)own[i] - correction);
    }
}

void qli_structural_field_free(QliStructuralField *structure)
{
    if (structure) {
        qli_free(structure->field.symbols);
        qli_free(structure);
    }
}

static int dense_structure_field(const QliDenseFit *fit, QliStructuralField **result)
{
    const QliDenseImage *image = fit->source;
    const QliDenseFitState *state = fit->state;
    QliStructuralField *structure = qli_calloc(1, sizeof(*structure));
    if (!structure) return 0;
    QliDenseField *f = &structure->field;
    *f = (QliDenseField){ .w = image->w, .h = image->h, .raw_crc = image->raw_crc,
                         .channels = image->channels, .streams = image->streams, .log = state->log,
                         .pixels = image->pixels, .tiles = image->tiles, .maps = image->maps,
                         .coefficient_bytes = 3 * state->regions };
    size_t symbols = f->pixels * f->channels;
    f->symbols = qli_malloc(symbols + f->maps + f->coefficient_bytes);
    uint8_t *storage = qli_malloc((DENSE_HISTOGRAM_BYTES + sizeof(uint32_t)) * f->tiles + sizeof(uint32_t));
    if (!f->symbols || !storage) {
        qli_free(storage);
        qli_structural_field_free(structure);
        return 0;
    }
    f->map = f->symbols + symbols;
    f->coefficients = f->map + f->maps;
    memcpy(f->map, state->map, f->maps);
    memcpy(f->coefficients, state->coeff, f->coefficient_bytes);
    DenseTileCache cache;
    cache.kernels = qli_dense_kernels();
    const int native = dense_structure_native();
    size_t tile = 0, offset = 0;
    for (unsigned y = 0; y < f->h; y += 16) {
        for (unsigned x = 0; x < f->w; x += 16, ++tile) {
            dense_tile_start(&cache, image, x, y);
            const uint8_t *co = f->coefficients + 3 * region_at(f->w, x, y, f->log);
            for (unsigned c = 0; c < f->channels; ++c) {
                unsigned mode = f->map[c * f->tiles + tile] & 15;
                int a = f->channels == 1 ? 0 : c == 0 ? (int)co[0] - 32 : c == 2 ? (int)co[1] - 32 : 0;
                int rc = f->channels > 1 && c == 2 ? (int)co[2] - 32 : 0;
                const int8_t *own = dense_vector_tile_errors(&cache, image, c, mode);
                const int8_t *green = a ? dense_vector_tile_errors(&cache, image, 1, mode) : NULL;
                const int8_t *red = rc ? dense_vector_tile_errors(&cache, image, 0, mode) : NULL;
                dense_fold_tile(own, green, red, cache.count, a, rc,
                                f->symbols + c * f->pixels + offset, native);
            }
            offset += cache.count;
        }
    }
    dense_structure_banks(structure, storage, native);
    qli_free(storage);
    *result = structure;
    return 1;
}

int qli_dense_prepare_structure(const uint8_t *raw, size_t bytes, uint32_t w, uint32_t h,
                                int *direct_gray, QliStructuralField **out)
{
    *out = NULL;
    *direct_gray = 0;
    QliDenseImage *image = dense_prepare_image(raw, bytes, w, h, direct_gray, NULL);
    if (!image) return 0;
    DenseFitCell *seed[3] = {0};
    DenseFitCell **seed_cells = image->channels > 1 ? seed : NULL;
    QliDenseFit *fit = NULL;
    int ok = dense_structure_seeds(image, seed_cells) &&
             dense_fit(image, seed_cells, 2, 1, &fit) &&
             dense_structure_field(fit, out);
    dense_free_seed(seed);
    qli_dense_fit_free(fit);
    qli_dense_image_free(image);
    return ok;
}

#undef QLI_DENSE_STRUCTURE_X86

enum { DENSE_RED = 1, DENSE_BLUE = 2, DENSE_BOTH = DENSE_RED | DENSE_BLUE };

typedef struct DenseCorrectionPair {
    int32_t green;
    int32_t red;
} DenseCorrectionPair;

static unsigned dense_pair_index(DenseCorrectionPair pairs[7], unsigned *pair_count, int32_t green,
                                 int32_t red)
{
    for (unsigned i = 0; i < *pair_count; ++i) {
        if (pairs[i].green == green && pairs[i].red == red) {
            return i;
        }
    }

    unsigned index = *pair_count;
    pairs[index].green = green;
    pairs[index].red = red;
    ++*pair_count;
    return index;
}

static unsigned dense_build_pairs(const int32_t a[3][2][3], unsigned target,
                                  DenseCorrectionPair pairs[7], unsigned mapping[3][3])
{
    unsigned pair_count = 0;
    for (unsigned layout = 0; layout < 3; ++layout) {
        for (unsigned slot = 0; slot < 3; ++slot) {
            int32_t green = 0;
            int32_t red = 0;
            if (slot == 1) {
                green = a[layout][target][0];
            } else if (slot == 2) {
                green = a[layout][target][1];
                if (target != 0) {
                    red = a[layout][target][2];
                }
            }
            mapping[layout][slot] = dense_pair_index(pairs, &pair_count, green, red);
        }
    }
    return pair_count;
}

#ifdef QLI_DENSE_DISPATCH
static QLI_DENSE_TARGET inline __m128i fit_load128(const void *p)
{
    __m128i value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static QLI_DENSE_TARGET inline void fit_store128(void *p, __m128i value)
{
    memcpy(p, &value, sizeof(value));
}

typedef __m128i DenseVector;
#define KM_ADD _mm_add_epi16
#define KM_SUB _mm_sub_epi16
#define KM_MIN _mm_min_epi16
#define KM_MAX _mm_max_epi16
#define KM_SHL(x, n) _mm_slli_epi16((x), (n))
#define KM_SHR(x, n) _mm_srai_epi16((x), (n))

static QLI_DENSE_TARGET inline DenseVector km_num(int v) { return _mm_set1_epi16((short)v); }
static QLI_DENSE_TARGET inline DenseVector km_clip(DenseVector v, DenseVector l, DenseVector h)
{
    return KM_MIN(KM_MAX(v, l), h);
}
static QLI_DENSE_TARGET inline DenseVector km_wrap(DenseVector v) { return KM_SHR(KM_SHL(v, 8), 8); }
static QLI_DENSE_TARGET inline DenseVector km_predict(unsigned mode, DenseVector l, DenseVector u, DenseVector c)
{
    DenseVector sum = KM_ADD(l, u), v = KM_SUB(sum, c);
    switch (mode) {
    case 0:
        return km_num(0);
    case 1:
        return l;
    case 2:
        return u;
    case 3:
        return km_clip(v, KM_MIN(l, u), KM_MAX(l, u));
    case 4:
        return KM_SHR(sum, 1);
    case 5:
        return km_clip(v, km_num(0), km_num(255));
    case 6:
        return _mm_and_si128(v, km_num(255));
    case 7: {
        DenseVector a = _mm_abs_epi16(KM_SUB(v, l));
        DenseVector b = _mm_abs_epi16(KM_SUB(v, u));
        DenseVector d = _mm_abs_epi16(KM_SUB(v, c));
        DenseVector first = _mm_andnot_si128(
            _mm_or_si128(_mm_cmpgt_epi16(a, b), _mm_cmpgt_epi16(a, d)), km_num(-1));
        DenseVector second = _mm_andnot_si128(_mm_cmpgt_epi16(b, d), km_num(-1));
        return _mm_blendv_epi8(_mm_blendv_epi8(c, u, second), l, first);
    }
    case 8:
        return KM_SHR(KM_ADD(KM_SHL(l, 1), sum), 2);
    case 9:
        return KM_SHR(KM_ADD(KM_SHL(u, 1), sum), 2);
    case 10:
        return KM_SHR(KM_ADD(KM_ADD(sum, l), c), 2);
    case 11:
        return KM_SHR(KM_ADD(KM_ADD(sum, u), c), 2);
    case 12:
        return KM_SHR(KM_ADD(l, c), 1);
    case 13:
        return KM_SHR(KM_ADD(u, c), 1);
    case 14:
        return c;
    default:
        return km_clip(KM_SHR(KM_SUB(KM_ADD(KM_SHL(sum, 1), sum), KM_SHL(c, 1)), 2), km_num(0),
                       km_num(255));
    }
}
static QLI_DENSE_TARGET inline DenseVector km_price(DenseVector error)
{
    DenseVector n = KM_ADD(_mm_abs_epi16(km_wrap(error)), km_num(1));
    DenseVector lut = _mm_setr_epi8(0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3);
    DenseVector low = _mm_shuffle_epi8(lut, _mm_and_si128(n, km_num(15)));
    DenseVector hi = KM_SHR(n, 4);
    DenseVector high = KM_ADD(_mm_shuffle_epi8(lut, hi), km_num(4));
    DenseVector log = _mm_blendv_epi8(low, high, _mm_cmpgt_epi16(hi, km_num(0)));
    return KM_ADD(km_num(1), KM_SHL(log, 1));
}
static QLI_DENSE_TARGET inline uint32_t km_total(DenseVector v)
{
    int16_t x[8];
    uint32_t n = 0;
    fit_store128(x, v);
    for (unsigned j = 0; j < 8; ++j) {
        n += (uint16_t)x[j];
    }
    return n;
}
static QLI_DENSE_TARGET inline DenseVector km_errors(const int16_t s[4][256], unsigned i, unsigned mode)
{
    DenseVector x = fit_load128((s[0] + i));
    DenseVector l = fit_load128((s[1] + i));
    DenseVector u = fit_load128((s[2] + i));
    DenseVector c = fit_load128((s[3] + i));
    return km_wrap(KM_SUB(x, km_predict(mode, l, u, c)));
}
static QLI_DENSE_TARGET inline int64_t km_sum32(DenseVector v)
{
    int32_t a[4];
    int64_t n = 0;
    fit_store128(a, v);
    for (unsigned i = 0; i < 4; ++i) {
        n += a[i];
    }
    return n;
}

static QLI_DENSE_TARGET void qli_sse42_dense_moments(const int16_t s[3][4][256], unsigned n, const unsigned mode[2], unsigned mask,
                       int64_t out[2][9])
{
    mask &= DENSE_BOTH;
    if (!mask) {
        return;
    }
    unsigned count = (n + 7u) & ~7u;
    DenseVector one = km_num(1);
    if (mask & DENSE_RED) {
        DenseVector accum[4] = {km_num(0), km_num(0), km_num(0), km_num(0)};
        for (unsigned i = 0; i < count; i += 8) {
            DenseVector e = km_errors(s[1], i, mode[0]);
            DenseVector g = km_errors(s[0], i, mode[0]);
            DenseVector terms[4] = {_mm_madd_epi16(g, one), _mm_madd_epi16(e, one),
                                    _mm_madd_epi16(g, g), _mm_madd_epi16(e, g)};
            for (unsigned j = 0; j < 4; ++j) {
                accum[j] = _mm_add_epi32(accum[j], terms[j]);
            }
        }
        out[0][0] = n;
        out[0][1] = km_sum32(accum[0]);
        out[0][2] = 0;
        out[0][3] = km_sum32(accum[1]);
        out[0][4] = km_sum32(accum[2]);
        out[0][5] = 0;
        out[0][6] = 0;
        out[0][7] = km_sum32(accum[3]);
        out[0][8] = 0;
    }
    if (mask & DENSE_BLUE) {
        DenseVector accum[8];
        for (unsigned j = 0; j < 8; ++j) {
            accum[j] = km_num(0);
        }
        for (unsigned i = 0; i < count; i += 8) {
            DenseVector e = km_errors(s[2], i, mode[1]);
            DenseVector g = km_errors(s[0], i, mode[1]);
            DenseVector r = km_errors(s[1], i, mode[1]);
            DenseVector terms[8] = {_mm_madd_epi16(g, one), _mm_madd_epi16(r, one),
                                    _mm_madd_epi16(e, one), _mm_madd_epi16(g, g),
                                    _mm_madd_epi16(r, r),   _mm_madd_epi16(g, r),
                                    _mm_madd_epi16(e, g),   _mm_madd_epi16(e, r)};
            for (unsigned j = 0; j < 8; ++j) {
                accum[j] = _mm_add_epi32(accum[j], terms[j]);
            }
        }
        out[1][0] = n;
        for (unsigned j = 0; j < 8; ++j) {
            out[1][j + 1] = km_sum32(accum[j]);
        }
    }
}

static QLI_DENSE_TARGET void qli_sse42_dense_costs(const int16_t s[3][4][256], unsigned n, const unsigned mode[2],
                     const int32_t a[2][3], unsigned mask, uint64_t out[2][3])
{
    mask &= DENSE_BOTH;
    if (!mask) {
        return;
    }
    unsigned count = (n + 7u) & ~7u;
    if (mask & DENSE_RED) {
        DenseVector accum[3] = {km_num(0), km_num(0), km_num(0)};
        DenseVector k = km_num(a[0][0]), ag = km_num(a[0][1]);
        for (unsigned i = 0; i < count; i += 8) {
            DenseVector e = km_errors(s[1], i, mode[0]);
            DenseVector g = km_errors(s[0], i, mode[0]);
            DenseVector kg = KM_SHR(_mm_mullo_epi16(k, g), 4);
            DenseVector cg = KM_SHR(_mm_mullo_epi16(ag, g), 4);
            accum[0] = KM_ADD(accum[0], km_price(e));
            accum[1] = KM_ADD(accum[1], km_price(KM_SUB(e, kg)));
            accum[2] = KM_ADD(accum[2], km_price(KM_SUB(e, cg)));
        }
        for (unsigned j = 0; j < 3; ++j) {
            out[0][j] = (uint64_t)km_total(accum[j]) - (count - n);
        }
    }
    if (mask & DENSE_BLUE) {
        DenseVector accum[3] = {km_num(0), km_num(0), km_num(0)};
        DenseVector k = km_num(a[1][0]), ag = km_num(a[1][1]), ar = km_num(a[1][2]);
        for (unsigned i = 0; i < count; i += 8) {
            DenseVector e = km_errors(s[2], i, mode[1]);
            DenseVector g = km_errors(s[0], i, mode[1]);
            DenseVector r = km_errors(s[1], i, mode[1]);
            DenseVector kg = KM_SHR(_mm_mullo_epi16(k, g), 4);
            DenseVector cg = KM_SHR(_mm_mullo_epi16(ag, g), 4);
            DenseVector cr = KM_SHR(_mm_mullo_epi16(ar, r), 4);
            accum[0] = KM_ADD(accum[0], km_price(e));
            accum[1] = KM_ADD(accum[1], km_price(KM_SUB(e, kg)));
            accum[2] = KM_ADD(accum[2], km_price(KM_SUB(KM_SUB(e, cg), cr)));
        }
        for (unsigned j = 0; j < 3; ++j) {
            out[1][j] = (uint64_t)km_total(accum[j]) - (count - n);
        }
    }
}

static QLI_DENSE_TARGET void qli_sse42_dense_mode_costs(const int16_t s[3][4][256], unsigned n, const int a[3], unsigned mask,
                          uint32_t out[2][16])
{
    mask &= DENSE_BOTH;
    if (!mask) {
        return;
    }
    unsigned count = (n + 7u) & ~7u;
    DenseVector k0 = (mask & DENSE_RED) ? km_num(a[0]) : km_num(0);
    DenseVector k1 = (mask & DENSE_BLUE) ? km_num(a[1]) : km_num(0);
    DenseVector k2 = (mask & DENSE_BLUE) ? km_num(a[2]) : km_num(0);
    for (unsigned mode = 0; mode < 16; ++mode) {
        DenseVector red = km_num(0), blue = km_num(0);
        for (unsigned i = 0; i < count; i += 8) {
            DenseVector g = km_errors(s[0], i, mode);
            DenseVector r = km_errors(s[1], i, mode);
            if (mask & DENSE_RED) {
                DenseVector correction = KM_SHR(_mm_mullo_epi16(k0, g), 4);
                red = KM_ADD(red, km_price(KM_SUB(r, correction)));
            }
            if (mask & DENSE_BLUE) {
                DenseVector b = km_errors(s[2], i, mode);
                DenseVector cg = KM_SHR(_mm_mullo_epi16(k1, g), 4);
                DenseVector cr = KM_SHR(_mm_mullo_epi16(k2, r), 4);
                blue = KM_ADD(blue, km_price(KM_SUB(KM_SUB(b, cg), cr)));
            }
        }
        if (mask & DENSE_RED) {
            out[0][mode] = km_total(red) - (count - n);
        }
        if (mask & DENSE_BLUE) {
            out[1][mode] = km_total(blue) - (count - n);
        }
    }
}

static QLI_DENSE_TARGET DenseVector dense_corrected_price(DenseVector error, DenseVector green_error,
                                         DenseVector red_error, DenseCorrectionPair pair)
{
    DenseVector corrected = error;
    if (pair.green != 0) {
        DenseVector green_correction = KM_SHR(_mm_mullo_epi16(km_num(pair.green), green_error), 4);
        corrected = KM_SUB(corrected, green_correction);
    }
    if (pair.red != 0) {
        DenseVector red_correction = KM_SHR(_mm_mullo_epi16(km_num(pair.red), red_error), 4);
        corrected = KM_SUB(corrected, red_correction);
    }
    return km_price(corrected);
}

static QLI_DENSE_TARGET void dense_target_costs(const int16_t samples[3][4][256], unsigned n, unsigned vector_count,
                               const unsigned mode[2], const int32_t a[3][2][3], unsigned target,
                               uint64_t out[3][2][3], unsigned *distinct)
{
    DenseCorrectionPair pairs[7];
    unsigned mapping[3][3];
    unsigned pair_count = dense_build_pairs(a, target, pairs, mapping);
    DenseVector accum[7];
    for (unsigned pair = 0; pair < pair_count; ++pair) {
        accum[pair] = km_num(0);
    }

    for (unsigned i = 0; i < vector_count; i += 8) {
        DenseVector error = km_errors(samples[target + 1], i, mode[target]);
        DenseVector green_error = km_errors(samples[0], i, mode[target]);
        DenseVector red_error = target == 0 ? km_num(0) : km_errors(samples[1], i, mode[target]);

        for (unsigned pair = 0; pair < pair_count; ++pair) {
            accum[pair] =
                KM_ADD(accum[pair], dense_corrected_price(error, green_error, red_error, pairs[pair]));
        }
    }

    uint64_t costs[7];
    unsigned padding = vector_count - n;
    for (unsigned pair = 0; pair < pair_count; ++pair) {
        costs[pair] = (uint64_t)km_total(accum[pair]) - padding;
    }

    for (unsigned layout = 0; layout < 3; ++layout) {
        for (unsigned slot = 0; slot < 3; ++slot) {
            out[layout][target][slot] = costs[mapping[layout][slot]];
        }
    }
    *distinct = pair_count;
}

static QLI_DENSE_TARGET void qli_sse42_dense_shared_costs(const int16_t samples[3][4][256], unsigned n, const unsigned mode[2],
                            const int32_t a[3][2][3], uint64_t out[3][2][3], unsigned distinct[2])
{
    if (n == 0) {
        for (unsigned layout = 0; layout < 3; ++layout) {
            for (unsigned target = 0; target < 2; ++target) {
                for (unsigned slot = 0; slot < 3; ++slot) {
                    out[layout][target][slot] = 0;
                }
            }
        }
        if (distinct != NULL) {
            distinct[0] = 0;
            distinct[1] = 0;
        }
        return;
    }

    unsigned vector_count = (n + 7u) & ~7u;
    unsigned distinct_counts[2];
    dense_target_costs(samples, n, vector_count, mode, a, 0, out, &distinct_counts[0]);
    dense_target_costs(samples, n, vector_count, mode, a, 1, out, &distinct_counts[1]);
    if (distinct != NULL) {
        distinct[0] = distinct_counts[0];
        distinct[1] = distinct_counts[1];
    }
}

static QLI_DENSE_TARGET void qli_sse42_dense_seed_scores(const int16_t s[4][256], unsigned n, uint32_t scores[16],
                           uint32_t magnitudes[16])
{
    if (n == 0) {
        for (unsigned mode = 0; mode < 16; ++mode) {
            scores[mode] = 0;
            magnitudes[mode] = 0;
        }
        return;
    }

    const unsigned padded_n = (n + 7u) & ~7u;
    const uint32_t padding = (uint32_t)(padded_n - n);
    for (unsigned mode = 0; mode < 16; ++mode) {
        DenseVector price_sum = km_num(0);
        DenseVector magnitude_sum = km_num(0);
        for (unsigned offset = 0; offset < padded_n; offset += 8u) {
            DenseVector error = km_errors(s, offset, mode);
            price_sum = KM_ADD(price_sum, km_price(error));
            magnitude_sum = KM_ADD(magnitude_sum, _mm_abs_epi16(error));
        }
        scores[mode] = km_total(price_sum) - padding;
        magnitudes[mode] = km_total(magnitude_sum);
    }
}

static QLI_DENSE_TARGET unsigned qli_sse42_dense_seed_best(const int16_t s[4][256], unsigned n, uint32_t *magnitude)
{
    const unsigned padded = (n + 7u) & ~7u;
    unsigned best = 0;
    uint32_t least = UINT32_MAX;
#define DENSE_SEED_ADD(sum, mode) \
    sum = KM_ADD(sum, _mm_abs_epi16(km_wrap(KM_SUB(x, km_predict(mode, l, u, c)))))
#define DENSE_SEED_GROUP(base) do { \
    DenseVector a = km_num(0), b = km_num(0), d = km_num(0), e = km_num(0); \
    for (unsigned i = 0; i < padded; i += 8) { \
        DenseVector x = fit_load128(s[0] + i), l = fit_load128(s[1] + i); \
        DenseVector u = fit_load128(s[2] + i), c = fit_load128(s[3] + i); \
        DENSE_SEED_ADD(a, (base) + 0); \
        DENSE_SEED_ADD(b, (base) + 1); \
        DENSE_SEED_ADD(d, (base) + 2); \
        DENSE_SEED_ADD(e, (base) + 3); \
    } \
    const uint32_t scores[4] = {km_total(a), km_total(b), km_total(d), km_total(e)}; \
    for (unsigned k = 0; k < 4; ++k) { \
        if (scores[k] < least) { \
            least = scores[k]; \
            best = (base) + k; \
        } \
    } \
} while (0)
    DENSE_SEED_GROUP(0);
    if (least) DENSE_SEED_GROUP(4);
    if (least) DENSE_SEED_GROUP(8);
    if (least) DENSE_SEED_GROUP(12);
#undef DENSE_SEED_GROUP
#undef DENSE_SEED_ADD
    *magnitude = least;
    return best;
}

static QLI_DENSE_TARGET DenseVector dense_row_errors(DenseVector current, DenseVector left, DenseVector upper,
                                    DenseVector corner, unsigned mode)
{
    DenseVector prediction = km_predict(mode, _mm_cvtepu8_epi16(left),
                                        _mm_cvtepu8_epi16(upper), _mm_cvtepu8_epi16(corner));
    return km_wrap(KM_SUB(_mm_cvtepu8_epi16(current), prediction));
}

static QLI_DENSE_TARGET __m128i dense_load_bytes(const uint8_t *source, unsigned count)
{
    if (count == 16) {
        return fit_load128(source);
    }

    uint8_t scratch[16] = {0};
    memcpy(scratch, source, count);
    return fit_load128(scratch);
}

static QLI_DENSE_TARGET __m128i dense_load_optional_bytes(const uint8_t *source, unsigned count)
{
    if (source == NULL) {
        return _mm_setzero_si128();
    }
    return dense_load_bytes(source, count);
}

static QLI_DENSE_TARGET void qli_sse42_dense_score_row(const uint8_t *current, const uint8_t *upper, unsigned width, unsigned x,
                         unsigned count, unsigned mode, int8_t *out)
{
    (void)width;
    if (count == 0) {
        return;
    }

    __m128i current_bytes = dense_load_bytes(current + x, count);
    __m128i upper_bytes = dense_load_optional_bytes(upper == NULL ? NULL : upper + x, count);
    __m128i left_bytes;
    __m128i corner_bytes;

    if (x == 0) {
        left_bytes = _mm_slli_si128(current_bytes, 1);
        corner_bytes = _mm_slli_si128(upper_bytes, 1);
    } else {
        left_bytes = dense_load_bytes(current + x - 1, count);
        corner_bytes = dense_load_optional_bytes(upper == NULL ? NULL : upper + x - 1, count);
    }

    DenseVector lower = dense_row_errors(current_bytes, left_bytes, upper_bytes, corner_bytes, mode);
    DenseVector upper_errors = dense_row_errors(
        _mm_srli_si128(current_bytes, 8), _mm_srli_si128(left_bytes, 8),
        _mm_srli_si128(upper_bytes, 8), _mm_srli_si128(corner_bytes, 8), mode);
    __m128i result = _mm_packs_epi16(lower, upper_errors);

    if (count == 16) {
        fit_store128(out, result);
    } else {
        uint8_t scratch[16];
        fit_store128(scratch, result);
        memcpy(out, scratch, count);
    }
}

static QLI_DENSE_TARGET void qli_sse42_dense_seed_bank_scores(const uint16_t counts[256], const float prices[256][8], double out[8])
{
    __m128d sum[4] = {_mm_setzero_pd(), _mm_setzero_pd(), _mm_setzero_pd(), _mm_setzero_pd()};
    for (unsigned s = 0; s < 256; ++s) {
        if (!counts[s]) continue;
        const __m128d n = _mm_set1_pd((double)counts[s]);
        for (unsigned k = 0; k < 4; ++k) {
            __m128i price = _mm_setzero_si128();
            memcpy(&price, prices[s] + 2*k, 2 * sizeof(float));
            sum[k] = _mm_add_pd(sum[k], _mm_mul_pd(n, _mm_cvtps_pd(_mm_castsi128_ps(price))));
        }
    }
    for (unsigned k = 0; k < 4; ++k) _mm_storeu_pd(out + 2*k, sum[k]);
}

static QLI_DENSE_TARGET void qli_sse42_dense_final_bank_scores(const uint16_t counts[256], const float prices[256][8], double out[8])
{
    __m128d sum[4] = {_mm_setzero_pd(), _mm_setzero_pd(), _mm_setzero_pd(), _mm_setzero_pd()};
    for (unsigned s = 0; s < 256; ++s) {
        if (!counts[s]) continue;
        const __m128 n = _mm_set1_ps((float)counts[s]);
        for (unsigned k = 0; k < 2; ++k) {
            const __m128 v = _mm_mul_ps(n, _mm_loadu_ps(prices[s] + 4*k));
            sum[2*k] = _mm_add_pd(sum[2*k], _mm_cvtps_pd(v));
            sum[2*k+1] = _mm_add_pd(sum[2*k+1], _mm_cvtps_pd(_mm_movehl_ps(v, v)));
        }
    }
    for (unsigned k = 0; k < 4; ++k) _mm_storeu_pd(out + 2*k, sum[k]);
}
#undef KM_ADD
#undef KM_SUB
#undef KM_MIN
#undef KM_MAX
#undef KM_SHL
#undef KM_SHR

static const QliDenseKernels qli_dense_sse42 = {
    .seed_scores = qli_sse42_dense_seed_scores,
    .seed_best = qli_sse42_dense_seed_best,
    .moments = qli_sse42_dense_moments,
    .costs = qli_sse42_dense_costs,
    .mode_costs = qli_sse42_dense_mode_costs,
    .shared_costs = qli_sse42_dense_shared_costs,
    .score_row = qli_sse42_dense_score_row,
    .seed_bank_scores = qli_sse42_dense_seed_bank_scores,
    .final_bank_scores = qli_sse42_dense_final_bank_scores
};
#undef QLI_DENSE_TARGET
#undef QLI_DENSE_DISPATCH
#endif


static const uint8_t dense_error_price[256] = {
     1,  3,  3,  5,  5,  5,  5,  7,  7,  7,  7,  7,  7,  7,  7,  9,
     9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 15,
    15, 15, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
     9,  9,  7,  7,  7,  7,  7,  7,  7,  7,  5,  5,  5,  5,  3,  3,
};

static inline int dense_sample_error(const int16_t s[4][256], unsigned i, unsigned mode)
{
    return signed_error((uint8_t)s[0][i], tile_predict(mode, s[1][i], s[2][i], s[3][i]));
}

static void dense_sample_errors(const int16_t s[4][256], unsigned n, unsigned mode, int8_t out[256])
{
#define DENSE_ERRORS(M) case M: \
    for (unsigned i = 0; i < n; ++i) out[i] = (int8_t)dense_sample_error(s, i, M); \
    break;
    switch (mode) {
        DENSE_ERRORS(0) DENSE_ERRORS(1) DENSE_ERRORS(2) DENSE_ERRORS(3)
        DENSE_ERRORS(4) DENSE_ERRORS(5) DENSE_ERRORS(6) DENSE_ERRORS(7)
        DENSE_ERRORS(8) DENSE_ERRORS(9) DENSE_ERRORS(10) DENSE_ERRORS(11)
        DENSE_ERRORS(12) DENSE_ERRORS(13) DENSE_ERRORS(14) DENSE_ERRORS(15)
    }
#undef DENSE_ERRORS
}

static void dense_fit_errors(const int16_t s[3][4][256], unsigned n, unsigned mode,
                             unsigned target, int8_t errors[3][256])
{
    dense_sample_errors(s[0], n, mode, errors[0]);
    if (target) dense_sample_errors(s[1], n, mode, errors[1]);
    dense_sample_errors(s[target + 1], n, mode, errors[2]);
}

void qli_dense_moments(const int16_t s[3][4][256], unsigned n, const unsigned mode[2],
                       unsigned mask, int64_t out[2][9])
{
    for (unsigned target = 0; target < 2; ++target) {
        if (!(mask & (1u << target))) continue;
        int8_t errors[3][256];
        dense_fit_errors(s, n, mode[target], target, errors);
        int32_t gsum = 0, rsum = 0, esum = 0, gg = 0, rr = 0, gr = 0, eg = 0, er = 0;
        for (unsigned i = 0; i < n; ++i) {
            const int g = errors[0][i], r = target ? errors[1][i] : 0, e = errors[2][i];
            gsum += g;
            rsum += r;
            esum += e;
            gg += g * g;
            rr += r * r;
            gr += g * r;
            eg += e * g;
            er += e * r;
        }
        const int64_t result[] = {n, gsum, rsum, esum, gg, rr, gr, eg, er};
        memcpy(out[target], result, sizeof(result));
    }
}

void qli_dense_costs(const int16_t s[3][4][256], unsigned n, const unsigned mode[2],
                     const int32_t a[2][3], unsigned mask, uint64_t out[2][3])
{
    for (unsigned target = 0; target < 2; ++target) {
        if (!(mask & (1u << target))) continue;
        int8_t errors[3][256];
        dense_fit_errors(s, n, mode[target], target, errors);
        unsigned plain = 0, green = 0, both = 0;
        for (unsigned i = 0; i < n; ++i) {
            const int g = errors[0][i], r = target ? errors[1][i] : 0, e = errors[2][i];
            plain += dense_error_price[(uint8_t)e];
            green += dense_error_price[(uint8_t)(e - tile_floor16(a[target][0] * g))];
            both += dense_error_price[(uint8_t)(e - tile_floor16(a[target][1] * g) - tile_floor16(a[target][2] * r))];
        }
        out[target][0] = plain;
        out[target][1] = green;
        out[target][2] = both;
    }
}

void qli_dense_mode_costs(const int16_t s[3][4][256], unsigned n, const int a[3],
                          unsigned mask, uint32_t out[2][16])
{
    if (!(mask & DENSE_BOTH)) return;
    for (unsigned mode = 0; mode < 16; ++mode) {
        int8_t errors[3][256];
        dense_sample_errors(s[0], n, mode, errors[0]);
        dense_sample_errors(s[1], n, mode, errors[1]);
        if (mask & DENSE_BLUE) dense_sample_errors(s[2], n, mode, errors[2]);
        unsigned red = 0, blue = 0;
        for (unsigned i = 0; i < n; ++i) {
            const int g = errors[0][i], r = errors[1][i];
            if (mask & DENSE_RED)
                red += dense_error_price[(uint8_t)(r - tile_floor16(a[0] * g))];
            if (mask & DENSE_BLUE) {
                const int b = errors[2][i];
                blue += dense_error_price[(uint8_t)(b - tile_floor16(a[1] * g) - tile_floor16(a[2] * r))];
            }
        }
        if (mask & DENSE_RED) out[0][mode] = red;
        if (mask & DENSE_BLUE) out[1][mode] = blue;
    }
}

void qli_dense_shared_costs(const int16_t s[3][4][256], unsigned n, const unsigned mode[2],
                            const int32_t a[3][2][3], uint64_t out[3][2][3], unsigned distinct[2])
{
    if (!n) {
        memset(out, 0, sizeof(uint64_t[3][2][3]));
        if (distinct) distinct[0] = distinct[1] = 0;
        return;
    }
    for (unsigned target = 0; target < 2; ++target) {
        DenseCorrectionPair pairs[7];
        unsigned mapping[3][3];
        unsigned count = dense_build_pairs(a, target, pairs, mapping);
        unsigned costs[7] = {0};
        int8_t errors[3][256];
        dense_fit_errors(s, n, mode[target], target, errors);
        for (unsigned i = 0; i < n; ++i) {
            const int g = errors[0][i], r = target ? errors[1][i] : 0, e = errors[2][i];
            for (unsigned p = 0; p < count; ++p) {
                int corrected = e - tile_floor16(pairs[p].green * g) - tile_floor16(pairs[p].red * r);
                costs[p] += dense_error_price[(uint8_t)corrected];
            }
        }
        for (unsigned layout = 0; layout < 3; ++layout)
            for (unsigned slot = 0; slot < 3; ++slot)
                out[layout][target][slot] = costs[mapping[layout][slot]];
        if (distinct) distinct[target] = count;
    }
}

void qli_dense_seed_scores(const int16_t s[4][256], unsigned n, uint32_t scores[16],
                           uint32_t magnitudes[16])
{
    for (unsigned mode = 0; mode < 16; ++mode) {
        int8_t errors[256];
        dense_sample_errors(s, n, mode, errors);
        unsigned price = 0, magnitude = 0;
        for (unsigned i = 0; i < n; ++i) {
            const int error = errors[i];
            price += dense_error_price[(uint8_t)error];
            magnitude += (unsigned)abs(error);
        }
        scores[mode] = price;
        magnitudes[mode] = magnitude;
    }
}

unsigned qli_dense_seed_best(const int16_t s[4][256], unsigned n, uint32_t *magnitude)
{
    unsigned best = 0, least = UINT32_MAX;
#define DENSE_SEED_GROUP(base) do { \
    unsigned a = 0, b = 0, c = 0, d = 0; \
    for (unsigned i = 0; i < n; ++i) { \
        a += (unsigned)abs(dense_sample_error(s, i, (base) + 0)); \
        b += (unsigned)abs(dense_sample_error(s, i, (base) + 1)); \
        c += (unsigned)abs(dense_sample_error(s, i, (base) + 2)); \
        d += (unsigned)abs(dense_sample_error(s, i, (base) + 3)); \
    } \
    const unsigned scores[4] = {a, b, c, d}; \
    for (unsigned k = 0; k < 4; ++k) { \
        if (scores[k] < least) { least = scores[k]; best = (base) + k; } \
    } \
} while (0)
    DENSE_SEED_GROUP(0);
    if (least) DENSE_SEED_GROUP(4);
    if (least) DENSE_SEED_GROUP(8);
    if (least) DENSE_SEED_GROUP(12);
#undef DENSE_SEED_GROUP
    *magnitude = least;
    return best;
}

void qli_dense_score_row(const uint8_t *current, const uint8_t *upper, unsigned width, unsigned x,
                         unsigned count, unsigned mode, int8_t *out)
{
    (void)width;
    if (!count) return;
    const uint8_t left = x ? current[x - 1] : 0, corner = x && upper ? upper[x - 1] : 0;
    current += x;
    if (upper) upper += x;
    static const uint8_t zero[16] = {0};
    const uint8_t *above = upper ? upper : zero;
    out[0] = (int8_t)signed_error(current[0], tile_predict(mode, left, above[0], corner));
#define DENSE_SCORE_ROW(M) case M: \
    for (unsigned i = 1; i < count; ++i) { \
        uint8_t p = tile_predict(M, current[i-1], above[i], above[i-1]); \
        out[i] = (int8_t)signed_error(current[i], p); \
    } \
    break;
    switch (mode) {
        DENSE_SCORE_ROW(0) DENSE_SCORE_ROW(1) DENSE_SCORE_ROW(2) DENSE_SCORE_ROW(3)
        DENSE_SCORE_ROW(4) DENSE_SCORE_ROW(5) DENSE_SCORE_ROW(6) DENSE_SCORE_ROW(7)
        DENSE_SCORE_ROW(8) DENSE_SCORE_ROW(9) DENSE_SCORE_ROW(10) DENSE_SCORE_ROW(11)
        DENSE_SCORE_ROW(12) DENSE_SCORE_ROW(13) DENSE_SCORE_ROW(14) DENSE_SCORE_ROW(15)
    }
#undef DENSE_SCORE_ROW
}

void qli_dense_seed_bank_scores(const uint16_t counts[256], const float prices[256][8], double out[8])
{
    memset(out, 0, sizeof(double[8]));
    for (unsigned s = 0; s < 256; ++s) {
        if (!counts[s]) continue;
        for (unsigned k = 0; k < 8; ++k)
            out[k] += (double)counts[s] * (double)prices[s][k];
    }
}

void qli_dense_final_bank_scores(const uint16_t counts[256], const float prices[256][8], double out[8])
{
    memset(out, 0, sizeof(double[8]));
    for (unsigned s = 0; s < 256; ++s) {
        if (!counts[s]) continue;
        for (unsigned k = 0; k < 8; ++k) {
            const float cost = (float)counts[s] * prices[s][k];
            out[k] += (double)cost;
        }
    }
}

const QliDenseKernels qli_dense_portable = {
    .seed_scores = qli_dense_seed_scores,
    .seed_best = qli_dense_seed_best,
    .moments = qli_dense_moments,
    .costs = qli_dense_costs,
    .mode_costs = qli_dense_mode_costs,
    .shared_costs = qli_dense_shared_costs,
    .score_row = qli_dense_score_row,
    .seed_bank_scores = qli_dense_seed_bank_scores,
    .final_bank_scores = qli_dense_final_bank_scores
};

static const int32_t dense_log_floor[257] = {
    0,        94364,    188362,   281996,   375269,   468184,   560744,   652952,   744809,
    836319,   927485,   1018308,  1108792,  1198939,  1288751,  1378232,  1467382,  1556206,
    1644705,  1732881,  1820738,  1908276,  1995500,  2082410,  2169009,  2255299,  2341283,
    2426962,  2512339,  2597416,  2682195,  2766679,  2850868,  2934765,  3018373,  3101693,
    3184727,  3267477,  3349946,  3432134,  3514044,  3595678,  3677037,  3758124,  3838940,
    3919487,  3999767,  4079782,  4159533,  4239022,  4318251,  4397221,  4475935,  4554393,
    4632598,  4710551,  4788254,  4865708,  4942915,  5019877,  5096595,  5173070,  5249304,
    5325299,  5401057,  5476578,  5551863,  5626916,  5701736,  5776326,  5850687,  5924820,
    5998727,  6072408,  6145866,  6219102,  6292117,  6364912,  6437489,  6509849,  6581994,
    6653924,  6725640,  6797145,  6868440,  6939525,  7010401,  7081071,  7151535,  7221795,
    7291851,  7361705,  7431358,  7500811,  7570066,  7639123,  7707983,  7776648,  7845119,
    7913396,  7981482,  8049377,  8117081,  8184597,  8251925,  8319066,  8386022,  8452793,
    8519380,  8585784,  8652007,  8718049,  8783912,  8849595,  8915101,  8980430,  9045583,
    9110562,  9175366,  9239997,  9304456,  9368744,  9432862,  9496810,  9560590,  9624202,
    9687648,  9750927,  9814042,  9876992,  9939779,  10002404, 10064867, 10127169, 10189311,
    10251295, 10313119, 10374787, 10436297, 10497652, 10558851, 10619897, 10680788, 10741527,
    10802114, 10862549, 10922834, 10982970, 11042956, 11102794, 11162484, 11222027, 11281425,
    11340677, 11399784, 11458747, 11517567, 11576245, 11634780, 11693174, 11751428, 11809542,
    11867516, 11925353, 11983051, 12040612, 12098036, 12155325, 12212478, 12269497, 12326381,
    12383133, 12439751, 12496238, 12552593, 12608817, 12664910, 12720874, 12776709, 12832415,
    12887994, 12943445, 12998769, 13053968, 13109040, 13163988, 13218811, 13273510, 13328086,
    13382539, 13436870, 13491079, 13545167, 13599135, 13652982, 13706710, 13760319, 13813810,
    13867182, 13920437, 13973575, 14026597, 14079503, 14132293, 14184969, 14237530, 14289977,
    14342311, 14394532, 14446641, 14498637, 14550522, 14602296, 14653960, 14705514, 14756958,
    14808293, 14859519, 14910637, 14961647, 15012550, 15063347, 15114037, 15164620, 15215099,
    15265472, 15315741, 15365906, 15415967, 15465924, 15515779, 15565531, 15615181, 15664729,
    15714177, 15763523, 15812769, 15861915, 15910961, 15959909, 16008757, 16057507, 16106159,
    16154714, 16203171, 16251532, 16299796, 16347964, 16396036, 16444013, 16491895, 16539683,
    16587376, 16634976, 16682482, 16729895, 16777216,
};

typedef struct {
    uint64_t low, high;
} DenseByteBounds;

static int dense_payload_bounds(const uint32_t *counts, const uint16_t *freq, unsigned alphabet,
                                unsigned bits, DenseByteBounds *bounds)
{
    const int64_t unit = INT64_C(1) << 24;
    const unsigned scale = 1u << bits;
    const int32_t low_correction = bits == 14 ? -47321 : -11822;
    const int32_t high_correction = bits == 14 ? 47229 : 11816;
    uint64_t events = 0;
    unsigned sum = 0, used = 0;
    int64_t lower = 0, upper = 0;
    *bounds = (DenseByteBounds){0};
    for (unsigned i = 0; i < alphabet; ++i) {
        unsigned n = counts[i], f = freq[i];
        if ((n != 0) != (f != 0) || f > scale - sum || n > (UINT64_C(1) << 25) - events) return 0;
        if (!n) continue;
        sum += f;
        events += n;
        ++used;
        unsigned log = 0;
        for (unsigned v = f; v > 1; v >>= 1)
            ++log;
        unsigned power = 1u << log, scaled = f << 8;
        unsigned floor_index = (scaled >> log) - 256;
        unsigned ceil_index = ((scaled + power - 1) >> log) - 256;
        int64_t base = (bits - log) * unit;
        int32_t ceiling = dense_log_floor[ceil_index] + (ceil_index != 0 && ceil_index != 256);
        lower += n * (base - ceiling + low_correction);
        upper += n * (base - dense_log_floor[floor_index] + high_correction);
    }
    if (used && sum != scale) return 0;
    if (used <= 1) return 1;
    lower -= (int64_t)(events < 4 ? events : 4) * 8 * unit;
    if (upper < 0) return 0;
    bounds->low = lower >= 0 ? (uint64_t)(lower / (8 * unit)) + 1 : 0;
    bounds->high = (uint64_t)(upper / (8 * unit));
    return bounds->low <= bounds->high;
}

static int dense_byte_model(const uint32_t counts[256], size_t n, RansModel *model,
                            DenseByteBounds *bounds)
{
    uint64_t histogram[256];
    for (unsigned i = 0; i < 256; ++i)
        histogram[i] = counts[i];
    if (!qlr_model_normalize(histogram, n, model) ||
        !dense_payload_bounds(counts, model->frequency, 256, 12, bounds))
        return 0;
    size_t framing = 48 + qlr_model_descriptor_bytes(model);
    bounds->low += framing;
    bounds->high += framing;
    return 1;
}

int qli_dense_write_magnitude(const uint8_t *src, size_t n, uint8_t **frame, size_t *size)
{
    *frame = NULL;
    *size = 0;
    if (n > (1u << 25) || (n && !src)) return 0;
    uint32_t bytes[256] = {0}, literals[256] = {0}, pairs[4096] = {0};
    uint16_t *tokens = n ? qli_malloc(n * sizeof(*tokens)) : NULL;
    uint8_t *literal = n ? qli_malloc(n) : NULL;
    uint8_t *ordinary = NULL, *wide = NULL;
    size_t nt = 0, nl = 0, no = 0, nw = 0, nd = 0, descriptor_used;
    uint8_t descriptor[9000];
    RansModel byte_model, literal_model;
    QliWideModel wide_model;
    DenseByteBounds old, lit, token;
    int ok = 0;
    if (n && (!tokens || !literal)) goto done;
    for (size_t i = 0; i < n;) {
        unsigned a = src[i++], symbol = 4095;
        ++bytes[a];
        if (i < n && a < 64 && src[i] < 64 && (a != 63 || src[i] != 63)) {
            unsigned b = src[i++];
            ++bytes[b];
            symbol = a | (b << 6);
        } else {
            literal[nl++] = (uint8_t)a;
            ++literals[a];
        }
        tokens[nt++] = (uint16_t)symbol;
        ++pairs[symbol];
    }
    if (!dense_byte_model(bytes, n, &byte_model, &old) ||
        !dense_byte_model(literals, nl, &literal_model, &lit) ||
        !qli_dense_wide_model_hist(pairs, descriptor, sizeof(descriptor), &nd) ||
        !qli_wide_model_read(descriptor, nd, &descriptor_used, &wide_model) ||
        descriptor_used != nd || !dense_payload_bounds(pairs, wide_model.freq, 4096, 14, &token))
        goto done;

    uint64_t wide_low = 28 + 48 + nd + token.low + lit.low;
    uint64_t wide_high = 28 + 48 + nd + token.high + lit.high;
    size_t old_events = byte_model.used > 1 ? n : 0;
    size_t wide_events = (wide_model.used > 1 ? nt : 0) + (literal_model.used > 1 ? nl : 0);
    int write_ordinary = 1, write_wide = 1;
    if (wide_events > old_events)
        write_wide = 0;
    else if (wide_events < old_events) {
        if (wide_high * UINT64_C(1000) <= old.low * UINT64_C(1004))
            write_ordinary = 0;
        else if (wide_low * UINT64_C(1000) > old.high * UINT64_C(1004))
            write_wide = 0;
    } else {
        if (wide_high < old.low)
            write_ordinary = 0;
        else if (wide_low >= old.high)
            write_wide = 0;
    }
    if (!write_wide) {
        qli_free(tokens);
        qli_free(literal);
        tokens = NULL;
        literal = NULL;
    }
    if (write_ordinary && !qli_four_encode(src, n, &byte_model, NULL, &ordinary, &no)) goto done;
    if (write_wide && !qli_wide_write(tokens, nt, literal, nl, descriptor, nd, &wide_model,
                                      &literal_model, &wide, &nw, NULL))
        goto done;
    if (!write_ordinary || (write_wide && nw * UINT64_C(1000) <= no * UINT64_C(1004) &&
                            (wide_events < old_events || (wide_events == old_events && nw < no)))) {
        *frame = wide;
        *size = nw;
        wide = NULL;
    } else {
        *frame = ordinary;
        *size = no;
        ordinary = NULL;
    }
    ok = 1;
done:
    qli_free(tokens);
    qli_free(literal);
    qli_free(ordinary);
    qli_free(wide);
    return ok;
}

typedef struct { QliFourWorkspace entropy; QliEncodeBuffer symbols, encoded; } QliTagWorkspace;
int qli_dense_tag_packet(const uint8_t *tags, const uint16_t *counts, size_t tiles,
                         unsigned alphabet, uint8_t **file, size_t *size, QliTagWorkspace *workspace);

static inline void qli_dense_tag_workspace_clear(QliTagWorkspace *workspace)
{
    qli_encode_buffer_clear(&workspace->entropy.scratch);
    qli_encode_buffer_clear(&workspace->entropy.frame);
    qli_encode_buffer_clear(&workspace->symbols);
    qli_encode_buffer_clear(&workspace->encoded);
}

typedef struct {
    double cost;
    unsigned id;
} DenseModelNode;
static int dense_model_less(DenseModelNode a, DenseModelNode b)
{
    return a.cost < b.cost || (a.cost == b.cost && a.id < b.id);
}
static void dense_model_push(DenseModelNode *heap, unsigned *n, DenseModelNode v)
{
    unsigned i = (*n)++;
    while (i) {
        unsigned p = (i - 1) / 2;
        if (!dense_model_less(v, heap[p])) {
            break;
        }
        heap[i] = heap[p];
        i = p;
    }
    heap[i] = v;
}
static DenseModelNode dense_model_pop(DenseModelNode *heap, unsigned *n)
{
    DenseModelNode result = heap[0], v = heap[--*n];
    unsigned i = 0;
    while (2 * i + 1 < *n) {
        unsigned k = 2 * i + 1;
        if (k + 1 < *n && dense_model_less(heap[k + 1], heap[k])) {
            ++k;
        }
        if (!dense_model_less(heap[k], v)) {
            break;
        }
        heap[i] = heap[k];
        i = k;
    }
    if (*n) {
        heap[i] = v;
    }
    return result;
}
int qli_dense_wide_model_hist(const uint32_t *hist, uint8_t *out, size_t cap, size_t *bytes)
{
    if (bytes) {
        *bytes = 0;
    }
    if (!hist || !out || !bytes) {
        return 0;
    }
    uint64_t total = 0;
    for (unsigned i = 0; i < 4096; ++i) {
        total += hist[i];
    }
    if (total > (1u << 25)) {
        return 0;
    }
    uint16_t freq[4096] = {0};
    unsigned sum = 0, used = 0;
    DenseModelNode heap[4096];
    unsigned nh = 0;
    for (unsigned i = 0; i < 4096; ++i) {
        if (hist[i]) {
            unsigned f = (unsigned)((uint64_t)hist[i] * 16384 / total);
            if (!f) {
                f = 1;
            }
            freq[i] = (uint16_t)f;
            sum += f;
            ++used;
        }
    }
    int add = sum < 16384;
    if (used && sum != 16384) {
        for (unsigned i = 0; i < 4096; ++i) {
            if (freq[i] && (add || freq[i] > 1)) {
                double f = freq[i], cost = (double)hist[i] * log2(add ? (f + 1) / f : f / (f - 1));
                dense_model_push(heap, &nh, (DenseModelNode){add ? -cost : cost, i});
            }
        }
        while (sum != 16384) {
            if (!nh) {
                return 0;
            }
            DenseModelNode n = dense_model_pop(heap, &nh);
            unsigned i = n.id;
            if (add) {
                ++freq[i];
                ++sum;
            } else {
                --freq[i];
                --sum;
            }
            if (add || freq[i] > 1) {
                double f = freq[i], cost = (double)hist[i] * log2(add ? (f + 1) / f : f / (f - 1));
                dense_model_push(heap, &nh, (DenseModelNode){add ? -cost : cost, i});
            }
        }
    }
    uint8_t data[9000];
    size_t at = 0;
    if (!used) {
        data[at++] = 0;
    } else if (used == 1) {
        unsigned id = 0;
        while (!freq[id]) {
            ++id;
        }
        data[at++] = 1;
        data[at++] = (uint8_t)id;
        data[at++] = (uint8_t)(id >> 8);
    } else {
        if (3 + 2 * used <= 513) {
            data[at++] = 2;
            data[at++] = (uint8_t)used;
            data[at++] = (uint8_t)(used >> 8);
            for (unsigned i = 0; i < 4096; ++i) {
                if (freq[i]) {
                    data[at++] = (uint8_t)i;
                    data[at++] = (uint8_t)(i >> 8);
                }
            }
        } else {
            data[at++] = 3;
            memset(data + at, 0, 512);
            for (unsigned i = 0; i < 4096; ++i) {
                if (freq[i]) {
                    data[at + i / 8] |= (uint8_t)(1u << (i % 8));
                }
            }
            at += 512;
        }
        unsigned emitted = 0;
        for (unsigned i = 0; i < 4096; ++i) {
            if (freq[i] && ++emitted < used) {
                unsigned f = freq[i];
                data[at++] = (uint8_t)(f < 128 ? f : (f & 127) | 128);
                if (f >= 128) {
                    data[at++] = (uint8_t)(f >> 7);
                }
            }
        }
    }
    if (at > cap) {
        return 0;
    }
    memcpy(out, data, at);
    *bytes = at;
    return 1;
}
int dense_wide_model(const uint8_t *values, size_t count, uint8_t *out, size_t cap, size_t *bytes)
{
    if (bytes) {
        *bytes = 0;
    }
    if (count > (1u << 25) || (count && !values)) {
        return 0;
    }
    uint32_t hist[4096] = {0};
    for (size_t i = 0; i < count;) {
        unsigned s = 4095;
        if (count - i >= 2 && values[i] < 64 && values[i + 1] < 64 &&
            (values[i] != 63 || values[i + 1] != 63)) {
            s = values[i] | ((unsigned)values[i + 1] << 6);
            i += 2;
        } else {
            ++i;
        }
        ++hist[s];
    }
    return qli_dense_wide_model_hist(hist, out, cap, bytes);
}

enum { DENSE_TAG_CAP = 1u << 26, DENSE_TAG_BYTE_CAP = 1u << 28 };
static int dense_tag_shape(const uint16_t *counts, size_t tiles, unsigned alphabet, size_t *total)
{
    *total = 0;
    if ((tiles && !counts) || tiles > DENSE_TAG_CAP ||
        (alphabet != 3 && alphabet != 27 && alphabet != 81)) {
        return 0;
    }
    for (size_t i = 0; i < tiles; ++i) {
        if (!counts[i] || counts[i] > 256 || *total > DENSE_TAG_CAP - counts[i]) {
            return 0;
        }
        *total += counts[i];
    }
    return 1;
}
int qli_dense_tag_packet(const uint8_t *tags, const uint16_t *counts, size_t tiles, unsigned alphabet,
                         uint8_t **file, size_t *size, QliTagWorkspace *workspace)
{
    if (!file || !size) {
        return 0;
    }
    *file = NULL;
    *size = 0;
    size_t total = 0;
    if (!dense_tag_shape(counts, tiles, alphabet, &total) || (total && !tags)) {
        return 0;
    }
    uint8_t *desc = NULL, *mixed = NULL, *dc = NULL, *mc = NULL, *out = NULL;
    size_t nd = 0, nm = 0, cursor = 0, used = 0;
    int ok = 0;
    size_t desc_capacity = tiles ? tiles : 1, mixed_capacity = total ? total : 1;
    if (workspace) {
        if (!qli_encode_buffer_reserve(&workspace->symbols, desc_capacity + mixed_capacity, 0)) goto done;
        desc = workspace->symbols.data;
        mixed = desc + desc_capacity;
    } else {
        desc = qli_malloc(desc_capacity);
        mixed = qli_malloc(mixed_capacity);
        if (!desc || !mixed) goto done;
    }
    for (size_t t = 0; t < tiles; ++t) {
        unsigned first = tags[cursor];
        int same = 1;
        for (unsigned j = 0; j < counts[t]; ++j) {
            unsigned v = tags[cursor + j];
            if (v >= alphabet) {
                goto done;
            }
            same &= v == first;
        }
        desc[t] = (uint8_t)(same ? first : alphabet);
        if (!same) {
            memcpy(mixed + used, tags + cursor, counts[t]);
            used += counts[t];
        }
        cursor += counts[t];
    }
    if (workspace) {
        if (!qli_four_encode(desc, tiles, NULL, &workspace->entropy, &dc, &nd) ||
            nd > DENSE_TAG_BYTE_CAP - 8 ||
            !qli_encode_buffer_reserve(&workspace->encoded, 8 + nd, 0)) goto done;
        out = workspace->encoded.data;
        memcpy(out, "QTC1", 4);
        qli_write_le32(out + 4, (uint32_t)nd);
        memcpy(out + 8, dc, nd);
        if (!qli_four_encode(mixed, used, NULL, &workspace->entropy, &mc, &nm) ||
            nm > DENSE_TAG_BYTE_CAP - 8 - nd ||
            !qli_encode_buffer_reserve(&workspace->encoded, 8 + nd + nm, 8 + nd)) goto done;
        out = workspace->encoded.data;
        memcpy(out + 8 + nd, mc, nm);
    } else {
        if (!write_four_separate(desc, tiles, &dc, &nd, NULL) ||
            !write_four_separate(mixed, used, &mc, &nm, NULL) || nd > DENSE_TAG_BYTE_CAP - 8 ||
            nm > DENSE_TAG_BYTE_CAP - 8 - nd) goto done;
        out = qli_malloc(8 + nd + nm);
        if (!out) goto done;
        memcpy(out, "QTC1", 4);
        qli_write_le32(out + 4, (uint32_t)nd);
        memcpy(out + 8, dc, nd);
        memcpy(out + 8 + nd, mc, nm);
    }
    *file = out;
    *size = 8 + nd + nm;
    out = NULL;
    ok = 1;
done:
    if (!workspace) {
        qli_free(desc);
        qli_free(mixed);
        qli_free(dc);
        qli_free(mc);
        qli_free(out);
    }
    return ok;
}

typedef struct {
    uint8_t *data;
    size_t size;
} FinishPart;
typedef struct {
    const QliDenseField *field;
    uint8_t *tags, *values;
    size_t counts[32], starts[32];
    FinishPart map, magnitude[32];
    unsigned map_coded;
} FinishShared;

typedef struct {
    size_t offset, size;
} FinishStoredPart;

typedef struct {
    size_t count, start, tiles, tile_start;
    uint16_t key;
    union {
        struct { FinishPart plain, tiled; } owned;
        struct { FinishStoredPart plain, tiled; } stored;
    };
} FinishGroup;
typedef struct {
    unsigned policy, groups;
    FinishGroup *group;
    size_t bytes[2];
} DenseFinishPolicy;

static void finish_shared_free(FinishShared *s)
{
    if (!s) return;
    qli_free(s->tags);
    qli_free(s->values);
    qli_free(s->map.data);
    for (unsigned i = 0; i < 32; ++i)
        qli_free(s->magnitude[i].data);
    qli_free(s);
}
static void dense_finish_policy_free(DenseFinishPolicy *p)
{
    if (!p) {
        return;
    }
    if (p->group) {
        for (unsigned i = 0; i < p->groups; ++i) {
            qli_free(p->group[i].owned.plain.data);
            qli_free(p->group[i].owned.tiled.data);
        }
    }
    qli_free(p->group);
    qli_free(p);
}
static unsigned finish_key(const QliDenseField *f, unsigned policy, size_t tile)
{
    if (!policy) return 0;
    if (policy == 1) return f->map[(f->channels == 1 ? 0 : 1) * f->tiles + tile] >> 4;
    unsigned key = 0;
    for (unsigned c = 0; c < f->channels; ++c)
        key |= (f->map[c * f->tiles + tile] >> 4) << (3 * c);
    return key;
}
static int finish_add(size_t *total, size_t bytes)
{
    if (bytes > (1u << 28) || *total > (1u << 28) - bytes) return 0;
    *total += bytes;
    return 1;
}
static int dense_finish_shared_create(const QliDenseField *f, FinishShared **result)
{
    *result = NULL;
    FinishShared *s = qli_calloc(1, sizeof(*s));
    if (!s) {
        return 0;
    }
    s->field = f;
    s->tags = qli_malloc(f->pixels);
    if (!s->tags) {
        goto fail;
    }
    size_t offset = 0, tile = 0;
    for (unsigned y = 0; y < f->h; y += 16) {
        for (unsigned x = 0; x < f->w; x += 16, ++tile) {
            size_t n = qli_dense_tile_pixels(x, y, f->w, f->h);
            for (size_t i = 0; i < n; ++i) {
                unsigned tag = 0, power = 1;
                for (unsigned c = 0; c < f->channels; ++c) {
                    unsigned z = f->symbols[c * f->pixels + offset + i];
                    if (z) {
                        tag += (z & 1 ? 2 : 1) * power;
                        ++s->counts[c * 8 + (f->map[c * f->tiles + tile] >> 4)];
                    }
                    power *= 3;
                }
                s->tags[offset + i] = (uint8_t)tag;
            }
            offset += n;
        }
    }
    if (offset != f->pixels || tile != f->tiles) {
        goto fail;
    }
    size_t total = 0, cursor[32];
    for (unsigned i = 0; i < f->streams; ++i) {
        s->starts[i] = cursor[i] = total;
        total += s->counts[i];
    }
    s->values = qli_malloc(total ? total : 1);
    if (!s->values) {
        goto fail;
    }
    for (unsigned c = 0; c < f->channels; ++c) {
        offset = 0;
        tile = 0;
        for (unsigned y = 0; y < f->h; y += 16) {
            for (unsigned x = 0; x < f->w; x += 16, ++tile) {
                size_t n = qli_dense_tile_pixels(x, y, f->w, f->h);
                unsigned bank = c * 8 + (f->map[c * f->tiles + tile] >> 4);
                for (size_t i = 0; i < n; ++i) {
                    unsigned z = f->symbols[c * f->pixels + offset + i];
                    if (z) {
                        s->values[cursor[bank]++] = (uint8_t)((z - 1) / 2);
                    }
                }
                offset += n;
            }
        }
    }
    for (unsigned i = 0; i < f->streams; ++i) {
        if (cursor[i] != s->starts[i] + s->counts[i] ||
            !write_four_separate(s->values + s->starts[i], s->counts[i], &s->magnitude[i].data,
                             &s->magnitude[i].size, NULL)) {
            goto fail;
        }
    }
    if (!write_four_separate(f->map, f->maps, &s->map.data, &s->map.size, NULL)) {
        goto fail;
    }
    s->map_coded = s->map.size < f->maps;
    *result = s;
    return 1;
fail:
    finish_shared_free(s);
    return 0;
}
static unsigned finish_group_count(const QliDenseField *f, unsigned policy,
                                   size_t counts[4096], size_t tiles[4096])
{
    size_t tile = 0;
    for (unsigned y = 0; y < f->h; y += 16)
        for (unsigned x = 0; x < f->w; x += 16, ++tile) {
            unsigned key = finish_key(f, policy, tile);
            counts[key] += qli_dense_tile_pixels(x, y, f->w, f->h);
            ++tiles[key];
        }
    unsigned logical = policy == 0 ? 1 : policy == 1 ? 8 : 1u << (3 * f->channels);
    unsigned groups = 0;
    for (unsigned key = 0; key < logical; ++key)
        groups += policy != 2 || counts[key] != 0;
    return groups;
}

static int finish_group_tags(const FinishShared *s, unsigned policy, FinishGroup *groups,
                             unsigned group_count, const size_t counts[4096],
                             const size_t tiles[4096], uint8_t *tags,
                             uint16_t *tile_counts, uint16_t *shapes)
{
    const QliDenseField *f = s->field;
    const unsigned logical = policy == 0 ? 1 : policy == 1 ? 8 : 1u << (3 * f->channels);
    int lookup[4096];
    for (unsigned key = 0; key < logical; ++key) lookup[key] = -1;
    size_t cursor[4096], tile_cursor[4096], at = 0, tile_at = 0;
    unsigned g = 0;
    for (unsigned key = 0; key < logical; ++key) {
        if (policy == 2 && !counts[key]) continue;
        lookup[key] = (int)g;
        groups[g] = (FinishGroup){ .count = counts[key], .start = at, .tiles = tiles[key],
                                   .tile_start = tile_at, .key = (uint16_t)key };
        cursor[g] = at;
        tile_cursor[g] = tile_at;
        at += counts[key];
        tile_at += tiles[key];
        ++g;
    }
    if (at != f->pixels || tile_at != f->tiles) return 0;
    size_t tile = 0;
    at = 0;
    for (unsigned y = 0; y < f->h; y += 16)
        for (unsigned x = 0; x < f->w; x += 16, ++tile) {
            size_t n = qli_dense_tile_pixels(x, y, f->w, f->h);
            int group = lookup[finish_key(f, policy, tile)];
            if (group < 0) return 0;
            memcpy(tags + cursor[group], s->tags + at, n);
            cursor[group] += n;
            size_t slot = tile_cursor[group]++;
            tile_counts[slot] = (uint16_t)n;
            if (shapes) {
                unsigned tw = f->w - x < 16 ? f->w - x : 16;
                unsigned th = f->h - y < 16 ? f->h - y : 16;
                shapes[slot] = (uint16_t)(tw | (th << 8));
            }
            at += n;
        }
    for (g = 0; g < group_count; ++g) {
        const FinishGroup *b = groups + g;
        if (cursor[g] != b->start + b->count || tile_cursor[g] != b->tile_start + b->tiles)
            return 0;
    }
    return 1;
}

static size_t finish_overhead(const FinishShared *s, unsigned policy, unsigned groups)
{
    const QliDenseField *f = s->field;
    return 44 + (s->map_coded ? s->map.size : f->maps) + f->coefficient_bytes +
           4 * (groups + f->streams) + 4 + (policy == 2 ? 4 + 2 * groups : 0);
}

static int dense_finish_policy_create(const FinishShared *s, unsigned policy,
                                      DenseFinishPolicy **result)
{
    *result = NULL;
    const QliDenseField *f = s->field;
    size_t counts[4096] = {0}, tiles[4096] = {0};
    unsigned groups = finish_group_count(f, policy, counts, tiles);
    DenseFinishPolicy *p = qli_calloc(1, sizeof(*p));
    if (!p) {
        return 0;
    }
    p->policy = policy;
    p->groups = groups;
    uint8_t *tags = NULL;
    uint16_t *tile_counts = NULL;
    int ok = 0;
    p->group = qli_calloc(p->groups, sizeof(*p->group));
    tags = qli_malloc(f->pixels);
    tile_counts = qli_malloc(f->tiles * sizeof(*tile_counts));
    if (!p->group || !tags || !tile_counts) {
        goto done;
    }
    if (!finish_group_tags(s, policy, p->group, groups, counts, tiles, tags, tile_counts, NULL))
        goto done;
    unsigned alphabet = f->channels == 1 ? 3 : f->channels == 3 ? 27 : 81;
    size_t common = finish_overhead(s, policy, groups);
    for (unsigned i = 0; i < f->streams; ++i) {
        if (!finish_add(&common, s->magnitude[i].size)) {
            goto done;
        }
    }
    p->bytes[0] = p->bytes[1] = common;
    for (unsigned g = 0; g < groups; ++g) {
        FinishGroup *b = p->group + g;
        if (!write_four_separate(tags + b->start, b->count, &b->owned.plain.data, &b->owned.plain.size, NULL) ||
            !qli_dense_tag_packet(tags + b->start, tile_counts + b->tile_start, b->tiles, alphabet,
                                  &b->owned.tiled.data, &b->owned.tiled.size, NULL)) {
            goto done;
        }
        if (b->owned.tiled.size >= b->owned.plain.size) {
            qli_free(b->owned.tiled.data);
            b->owned.tiled = (FinishPart){0};
        }
        if (!finish_add(&p->bytes[0], b->owned.plain.size) ||
            !finish_add(&p->bytes[1], b->owned.tiled.data ? b->owned.tiled.size : b->owned.plain.size)) {
            goto done;
        }
    }
    *result = p;
    p = NULL;
    ok = 1;
done:
    qli_free(tags);
    qli_free(tile_counts);
    dense_finish_policy_free(p);
    return ok;
}
static size_t write_strip_header(uint8_t *out, const char *marker, const FinishShared *s,
                                 unsigned layout, unsigned groups, unsigned policy,
                                 const FinishGroup *group)
{
    const QliDenseField *f = s->field;
    size_t map_size = s->map_coded ? s->map.size : f->maps;
    const uint8_t *map = s->map_coded ? s->map.data : f->map;
    memcpy(out, marker, 4);
    qli_write_le32(out + 4, f->w);
    qli_write_le32(out + 8, f->h);
    out[12] = (uint8_t)f->channels;
    out[13] = (uint8_t)layout;
    out[14] = 4;
    out[15] = 16;
    qli_write_le32(out + 16, (uint32_t)f->maps);
    qli_write_le32(out + 20, groups + f->streams);
    qli_write_le32(out + 24, f->raw_crc);
    qli_write_le32(out + 28, f->log);
    qli_write_le32(out + 32, (uint32_t)f->coefficient_bytes);
    qli_write_le32(out + 36, policy | (s->map_coded << 8));
    qli_write_le32(out + 40, (uint32_t)map_size);
    memcpy(out + 44, map, map_size);
    memcpy(out + 44 + map_size, f->coefficients, f->coefficient_bytes);
    size_t at = 44 + map_size + f->coefficient_bytes;
    if (policy == 2) {
        qli_write_le32(out + at, groups);
        at += 4;
        for (unsigned g = 0; g < groups; ++g, at += 2)
            qli_write_le16(out + at, group[g].key);
    }
    return at;
}

static size_t write_strip_parts(uint8_t *out, size_t lengths, size_t at,
                                const FinishPart *parts, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        qli_write_le32(out + lengths + 4 * i, (uint32_t)parts[i].size);
        memcpy(out + at, parts[i].data, parts[i].size);
        at += parts[i].size;
    }
    return at;
}

static int dense_finish_assemble(const FinishShared *s, const DenseFinishPolicy *p, unsigned kind,
                                 const FinishPart *values, uint8_t **file, size_t *size)
{
    *file = NULL;
    *size = 0;
    const QliDenseField *f = s->field;
    size_t total = p->bytes[kind];
    for (unsigned i = 0; i < f->streams; ++i) {
        total -= s->magnitude[i].size;
        if (!finish_add(&total, values[i].size)) {
            return 0;
        }
    }
    uint8_t *out = qli_calloc(1, total);
    if (!out) {
        return 0;
    }
    size_t lengths = write_strip_header(out, kind ? "QWT1" : "QWI1", s, 1,
                                        p->groups, p->policy, p->group);
    size_t at = lengths + 4 * (p->groups + f->streams);
    for (unsigned g = 0; g < p->groups; ++g) {
        FinishPart part = kind && p->group[g].owned.tiled.data ? p->group[g].owned.tiled : p->group[g].owned.plain;
        qli_write_le32(out + lengths + 4 * g, (uint32_t)part.size);
        memcpy(out + at, part.data, part.size);
        at += part.size;
    }
    at = write_strip_parts(out, lengths + 4 * p->groups, at, values, f->streams);
    if (at != total - 4) {
        qli_free(out);
        return 0;
    }
    qli_write_le32(out + at, qint_crc32(out, at));
    *file = out;
    *size = total;
    return 1;
}
static int dense_finish_better(size_t n, unsigned kind, unsigned policy, size_t best, unsigned bk,
                               unsigned bp)
{
    return n < best || (n == best && (kind < bk || (kind == bk && policy < bp)));
}

int qli_dense_finish_sample(const QliDenseField *f, size_t incumbent, uint8_t **file, size_t *size)
{
    FinishShared *s = NULL;
    DenseFinishPolicy *best = NULL, *p = NULL;
    FinishPart wide[32] = {0}, parts[32] = {0};
    unsigned bk = 2, bp = 3;
    size_t best_size = SIZE_MAX;
    uint8_t *out = NULL;
    size_t n = 0;
    int ok = 0;
    *file = NULL;
    *size = 0;
    if (!dense_finish_shared_create(f, &s)) {
        goto done;
    }
    for (unsigned policy = 0; policy < 3; ++policy) {
        if (!dense_finish_policy_create(s, policy, &p)) {
            goto done;
        }
        unsigned kind = p->bytes[1] < p->bytes[0];
        if (dense_finish_better(p->bytes[kind], kind, policy, best_size, bk, bp)) {
            dense_finish_policy_free(best);
            best = p;
            p = NULL;
            bk = kind;
            bp = policy;
            best_size = best->bytes[kind];
        }
        dense_finish_policy_free(p);
        p = NULL;
    }
    if (best_size >= incumbent) {
        ok = 1;
        goto done;
    }
    for (unsigned i = 0; i < f->streams; ++i) {
        uint8_t descriptor[9000];
        size_t nd = 0;
        size_t wide_events;
        if (!dense_wide_model(s->values + s->starts[i], s->counts[i], descriptor, sizeof(descriptor),
                              &nd) ||
            !wide_encode(s->values + s->starts[i], s->counts[i], descriptor, nd, &wide[i].data,
                         &wide[i].size, &wide_events)) {
            goto done;
        }
        FinishPart old = s->magnitude[i];
        if (old.size < 18) {
            goto done;
        }
        size_t events = (old.data[16] | ((unsigned)old.data[17] << 8)) > 1 ? s->counts[i] : 0;
        int choose =
            wide[i].size * UINT64_C(1000) <= old.size * UINT64_C(1004) &&
            (wide_events < events || (wide_events == events && wide[i].size < old.size));
        parts[i] = choose ? wide[i] : old;
        if (!choose) {
            qli_free(wide[i].data);
            wide[i] = (FinishPart){0};
        }
    }
    if (!dense_finish_assemble(s, best, bk, parts, &out, &n)) {
        goto done;
    }
    *file = out;
    *size = n;
    out = NULL;
    ok = 1;
done:
    qli_free(out);
    dense_finish_policy_free(p);
    dense_finish_policy_free(best);
    finish_shared_free(s);
    for (unsigned i = 0; i < 32; ++i) {
        qli_free(wide[i].data);
    }
    return ok;
}

typedef struct {
    uint8_t *buffer;
    const uint8_t *residual;
    size_t bytes;
    unsigned stride, channels, green;
} QliFastRows;

int qli_prepare_fast_rows(const uint8_t *rgba, uint32_t w, uint32_t h,
                          unsigned mode, QliFastRows *rows);
void qli_release_fast_rows(const QliFastRows *rows);

int qli_write_fast_frame(uint32_t w, uint32_t h, unsigned channels, unsigned mode,
                         unsigned green, const uint8_t *palette, unsigned colors,
                         const uint8_t *packed, size_t packed_size, uint8_t **out, size_t *bytes);

int qli_try_solid(const uint8_t *rgba, size_t size, uint32_t w, uint32_t h,
                  uint8_t **out, size_t *bytes);

int qli_write_fast_frame(uint32_t w, uint32_t h, unsigned channels, unsigned mode,
                         unsigned green, const uint8_t *palette, unsigned colors,
                         const uint8_t *packed, size_t packed_size, uint8_t **out, size_t *bytes)
{
    if (packed_size > (1u << 28) || colors > 256) return 0;
    const size_t palette_bytes = (size_t)colors * 4;
    size_t size = FAST_HEADER + palette_bytes + packed_size + FAST_FOOTER;
    uint8_t *file = qli_malloc(size);
    if (!file) return 0;
    memset(file, 0, FAST_HEADER);
    memcpy(file, "QPF1", 4);
    qli_write_le32(file + 4, w);
    qli_write_le32(file + 8, h);
    file[12] = (uint8_t)channels;
    file[13] = (uint8_t)mode;
    file[14] = (uint8_t)green;
    qli_write_le32(file + 16, colors);
    qli_write_le32(file + 20, (uint32_t)packed_size);
    if (palette_bytes) memcpy(file + FAST_HEADER, palette, palette_bytes);
    if (packed_size) memcpy(file + FAST_HEADER + palette_bytes, packed, packed_size);
    qli_write_le32(file + size - FAST_FOOTER,
                   qli_wrapped_crc32(file, FAST_HEADER + palette_bytes, packed_size));
    *out = file;
    *bytes = size;
    return 1;
}

int qli_try_solid(const uint8_t *rgba, size_t size, uint32_t w, uint32_t h,
                  uint8_t **out, size_t *bytes)
{
    if (out) *out = NULL;
    if (bytes) *bytes = 0;
    size_t pixels;
    if (!out || !bytes || !rgba || !fast_shape(w, h, &pixels) || size != pixels * 4)
        return -1;
    uint32_t first = qli_read_le32(rgba);
    for (size_t i = 1; i < pixels; ++i)
        if (qli_read_le32(rgba + i * 4) != first) return 0;
    return qli_write_fast_frame(w, h, 4, FAST_SOLID, 0, rgba, 1, NULL, 0, out, bytes) ? 1 : -1;
}

static int fast_palette(const uint8_t *rgba, uint32_t w, uint32_t h, size_t pixels,
                         uint8_t **file, size_t *bytes)
{
    uint8_t *indices = qli_malloc(pixels);
    if (!indices) return -1;
    uint32_t keys[1024] = {0};
    unsigned slots[1024] = {0}, count = 0;
    uint8_t palette[1024], *packed = NULL;
    size_t packed_size = 0;
    int result = -1;
    for (size_t i = 0; i < pixels; ++i) {
        uint32_t key = qli_read_le32(rgba + i * 4);
        unsigned slot = (key * UINT32_C(2654435761)) >> 22;
        while (slots[slot] && keys[slot] != key) slot = (slot + 1) & 1023u;
        if (!slots[slot]) {
            if (count == 256) {
                result = 0;
                goto done;
            }
            keys[slot] = key;
            memcpy(palette + (size_t)count * 4, rgba + i * 4, 4);
            slots[slot] = ++count;
        }
        indices[i] = (uint8_t)(slots[slot] - 1);
    }
    unsigned mode = count == 1 ? FAST_SOLID : FAST_PALETTE;
    if (mode == FAST_PALETTE &&
        qli_pack_below(indices, pixels, w <= UINT16_MAX ? w : 0, SIZE_MAX,
                       &packed, &packed_size, NULL, NULL) != 1)
        goto done;
    result = qli_write_fast_frame(w, h, 4, mode, 0, palette, count, packed, packed_size,
                                  file, bytes) ? 1 : -1;
done:
    qli_free(indices);
    qli_free(packed);
    return result;
}

static uint8_t fast_row_error(const uint8_t *row, const uint8_t *up, size_t x,
                              unsigned channels, unsigned filter)
{
    int left = x >= channels ? row[x - channels] : 0;
    int above = up ? up[x] : 0;
    int corner = up && x >= channels ? up[x - channels] : 0;
    return (uint8_t)(row[x] - fast_predict(filter, left, above, corner));
}

int qli_prepare_fast_rows(const uint8_t *rgba, uint32_t w, uint32_t h,
                          unsigned mode, QliFastRows *rows)
{
    size_t pixels = (size_t)w * h;
    unsigned channels = qli_rgba_channels(rgba, pixels);
    unsigned green = mode != FAST_RAW && channels >= 3;
    size_t row_bytes = (size_t)w * channels, size = pixels * channels;
    size_t map_size = mode == FAST_ROW ? h : 0;
    unsigned stride = row_bytes <= UINT16_MAX ? (unsigned)row_bytes : 0;
    if (mode == FAST_RAW && channels == 4) {
        *rows = (QliFastRows){NULL, rgba, size, stride, channels, 0};
        return 1;
    }

    uint8_t *residual = qli_malloc(size + map_size);
    if (!residual) return 0;
    uint8_t *compact = mode == FAST_ROW ? qli_malloc(size) : residual;
    if (!compact) {
        qli_free(residual);
        return 0;
    }
    for (size_t i = 0; i < pixels; ++i) {
        const uint8_t *p = rgba + 4 * i;
        uint8_t *q = compact + channels * i;
        unsigned base = green ? p[1] : 0;
        q[0] = (uint8_t)(p[0] - base);
        if (channels != 1) {
            q[1] = p[1];
            q[2] = (uint8_t)(p[2] - base);
            if (channels == 4) q[3] = p[3];
        }
    }
    if (mode == FAST_LEFT) {
        for (uint32_t y = 0; y < h; ++y) {
            uint8_t *row = residual + (size_t)y * row_bytes;
            for (size_t x = row_bytes; x-- > channels;)
                row[x] = (uint8_t)(row[x] - row[x - channels]);
        }
    } else if (mode == FAST_ROW) {
        for (uint32_t y = 0; y < h; ++y) {
            const uint8_t *row = compact + (size_t)y * row_bytes;
            uint8_t *dst = residual + map_size + (size_t)y * row_bytes;
            const uint8_t *up = y ? row - row_bytes : NULL;
            uint64_t best = UINT64_MAX;
            unsigned filter = 0;
            for (unsigned candidate = 0; candidate < 4; ++candidate) {
                uint64_t score = 0;
                for (size_t x = 0; x < row_bytes; ++x) {
                    unsigned error = fast_row_error(row, up, x, channels, candidate);
                    score += error < 128 ? error : 256 - error;
                }
                if (score < best) {
                    best = score;
                    filter = candidate;
                }
            }
            residual[y] = (uint8_t)filter;
            for (size_t x = 0; x < row_bytes; ++x)
                dst[x] = fast_row_error(row, up, x, channels, filter);
        }
        qli_free(compact);
    }
    *rows = (QliFastRows){residual, residual, size + map_size, stride, channels, green};
    return 1;
}

void qli_release_fast_rows(const QliFastRows *rows)
{
    qli_free(rows->buffer);
}

static int fast_form(const uint8_t *rgba, uint32_t w, uint32_t h, size_t pixels,
                      unsigned mode, uint8_t **out, size_t *bytes)
{
    if (mode == FAST_PALETTE) return fast_palette(rgba, w, h, pixels, out, bytes);
    QliFastRows rows;
    if (!qli_prepare_fast_rows(rgba, w, h, mode, &rows)) return -1;
    uint8_t *packed = NULL;
    size_t size = 0;
    int ok = qli_pack_below(rows.residual, rows.bytes, rows.stride, SIZE_MAX,
                                &packed, &size, NULL, NULL) == 1 &&
             qli_write_fast_frame(w, h, rows.channels, mode, rows.green, NULL, 0, packed, size,
                                  out, bytes);
    qli_free(packed);
    qli_release_fast_rows(&rows);
    return ok ? 1 : -1;
}

int fast_encode_alloc(const uint8_t *rgba, uint32_t w, uint32_t h, unsigned mode,
                      uint8_t **file, size_t *bytes)
{
    if (!file || !bytes) return 0;
    *file = NULL;
    *bytes = 0;
    size_t pixels;
    if (!rgba || !fast_shape(w, h, &pixels) || (mode > FAST_PALETTE && mode != FAST_AUTO))
        return 0;
    if (mode != FAST_AUTO) return fast_form(rgba, w, h, pixels, mode, file, bytes) == 1;
    for (unsigned choice = FAST_RAW; choice <= FAST_PALETTE; ++choice) {
        uint8_t *trial = NULL;
        size_t size = 0;
        int result = fast_form(rgba, w, h, pixels, choice, &trial, &size);
        if (result < 0) {
            qli_free(*file);
            *file = NULL;
            *bytes = 0;
            return 0;
        }
        if (result == 1 && (!*file || size < *bytes)) {
            qli_free(*file);
            *file = trial;
            *bytes = size;
        } else {
            qli_free(trial);
        }
    }
    return *file != NULL;
}

typedef struct {
    const uint8_t *data;
    size_t bytes;
    QliWriterPacket packet;
} HistoryRecord;

static int history_pack_record(QliWriterMemory *memory, const uint8_t *data, size_t count,
                               HistoryRecord *record, int action)
{
    QliWriterPacket packet;
    const int ok = action ? qli_encode_action_packet(memory, data, count, &packet) :
                            qli_encode_byte_packet(memory, data, count, &packet);
    if (!ok) return 0;
    if (packet.bytes < count) {
        *record = (HistoryRecord){packet.data, packet.bytes, packet};
    } else {
        qli_writer_free(memory, packet.data, packet.capacity);
        *record = (HistoryRecord){.data = data, .bytes = count};
    }
    return 1;
}

static uint8_t *history_write_record(uint8_t *out, const HistoryRecord *record)
{
    out[0] = record->packet.data != NULL;
    qli_write_le32(out + 1, (uint32_t)record->bytes);
    memcpy(out + 5, record->data, record->bytes);
    return out + 5 + record->bytes;
}

static size_t capture_phrase_matches(const uint8_t *actions, size_t count, uint32_t *history,
                                     uint32_t *positions, uint8_t *lengths, uint16_t *distances,
                                     size_t capacity)
{
    enum { MAX_PHRASE_LENGTH = 255 };
    memset(history, 0, QLI_HISTORY_SLOTS * sizeof(*history));
    size_t matches = 0;
    size_t at = 0;
    while (count - at >= 4) {
        const uint8_t *p = actions + at;
        const uint32_t word = qli_read_le32(p);
        const unsigned slot = (word * UINT32_C(0x9e3779b1)) >> 16;
        const uint32_t previous = history[slot];
        history[slot] = (uint32_t)at + 1;
        if (previous) {
            const size_t start = previous - 1u;
            const size_t distance = at - start;
            if (distance <= QLI_HISTORY_SLOTS && memcmp(actions + start, p, 4) == 0) {
                const size_t available = count - at;
                const size_t limit = available < MAX_PHRASE_LENGTH ? available : MAX_PHRASE_LENGTH;
                size_t length = 4;
                while (length < limit && actions[start + length] == p[length]) ++length;
                if (matches == capacity) return SIZE_MAX;
                positions[matches] = (uint32_t)at;
                lengths[matches] = (uint8_t)(length - 4);
                distances[matches] = (uint16_t)(distance - 1);
                ++matches;
                at += length;
                continue;
            }
        }
        ++at;
    }
    return matches;
}

static size_t phrase_write_gap(uint8_t *out, size_t value)
{
    size_t n = 0;
    while (value >= 128) {
        out[n++] = (uint8_t)(value | 128u);
        value >>= 7;
    }
    out[n++] = (uint8_t)value;
    return n;
}

static QliWriterPacket pack_history_phrase(QliWriterMemory *memory, const uint8_t *actions,
                                          size_t count, size_t limit)
{
    enum { RECORDS = 5, PREFIX = 12 + RECORDS * 5 };
    QliWriterPacket result = {0};
    if (count < 5 || limit < PREFIX) return result;
    const size_t capacity = count / 4;
    const size_t history_size = QLI_HISTORY_SLOTS * sizeof(uint32_t);
    const size_t position_size = capacity * sizeof(uint32_t);
    const size_t distance_size = capacity * sizeof(uint16_t);
    uint32_t *history = qli_writer_allocate(memory, history_size);
    uint32_t *positions = qli_writer_allocate(memory, position_size);
    uint16_t *distance = qli_writer_allocate(memory, distance_size);
    uint8_t *length = qli_writer_allocate(memory, capacity);
    uint8_t *work = NULL;
    size_t work_size = 0;
    HistoryRecord records[RECORDS] = {0};
    if (!history || !positions || !distance || !length) goto done;
    const size_t matches = capture_phrase_matches(actions, count, history, positions, length,
                                                  distance, capacity);
    if (!matches || matches > capacity) goto done;
    qli_writer_free(memory, history, history_size);
    history = NULL;
    size_t matched = 0;
    for (size_t i = 0; i < matches; ++i) matched += 4u + length[i];
    const size_t literals = count - matched;
    if (!literals) goto done;
    
    const size_t gap_cap = literals + matches;
    work_size = literals + gap_cap + 2 * matches;
    work = qli_writer_allocate(memory, work_size);
    if (!work) goto done;
    uint8_t *gaps = work + literals;
    uint8_t *low = gaps + gap_cap;
    uint8_t *high = low + matches;
    size_t used = 0, gap_bytes = 0, from = 0;
    for (size_t i = 0; i < matches; ++i) {
        const size_t gap = positions[i] - from;
        memcpy(work + used, actions + from, gap);
        used += gap;
        gap_bytes += phrase_write_gap(gaps + gap_bytes, gap);
        low[i] = (uint8_t)distance[i];
        high[i] = (uint8_t)(distance[i] >> 8);
        from = positions[i] + 4u + length[i];
    }
    memcpy(work + used, actions + from, count - from);
    qli_writer_free(memory, positions, position_size);
    positions = NULL;
    qli_writer_free(memory, distance, distance_size);
    distance = NULL;

    const uint8_t *parts[RECORDS] = {work, gaps, length, low, high};
    const size_t sizes[RECORDS] = {literals, gap_bytes, matches, matches, matches};
    size_t bytes = PREFIX;
    for (unsigned i = 0; i < RECORDS; ++i) {
        if (!history_pack_record(memory, parts[i], sizes[i], &records[i], i == 0) ||
            records[i].bytes > limit - bytes) goto done;
        bytes += records[i].bytes;
    }
    uint8_t *data = qli_writer_allocate(memory, bytes);
    if (!data) goto done;
    result = (QliWriterPacket){data, bytes, bytes};
    qli_write_le32(data, (uint32_t)literals);
    qli_write_le32(data + 4, (uint32_t)matches);
    qli_write_le32(data + 8, (uint32_t)gap_bytes);
    uint8_t *out = data + 12;
    for (unsigned i = 0; i < RECORDS; ++i) out = history_write_record(out, &records[i]);
    assert(out == data + bytes);
done:
    for (unsigned i = 0; i < RECORDS; ++i)
        qli_writer_free(memory, records[i].packet.data, records[i].packet.capacity);
    qli_writer_free(memory, work, work_size);
    qli_writer_free(memory, length, capacity);
    qli_writer_free(memory, distance, distance_size);
    qli_writer_free(memory, positions, position_size);
    qli_writer_free(memory, history, history_size);
    return result;
}

static void error_bytes(uint32_t value,uint32_t prediction,uint8_t *out,unsigned channels)
{
    unsigned green=((value>>8)&255)-((prediction>>8)&255);
    out[0]=(uint8_t)green;
    out[1]=(uint8_t)((value&255)-(prediction&255)-green);
    out[2]=(uint8_t)(((value>>16)&255)-((prediction>>16)&255)-green);
    if(channels==4) out[3]=(uint8_t)((value>>24)-(prediction>>24));
}

static QLI_INLINE unsigned history_equal_run(const uint8_t *current, const uint8_t *reference,
                                  unsigned limit,unsigned channels)
{
    unsigned n = 0;
    for (; limit - n >= 4; n += 4) {
        if (memcmp(current + channels*n, reference + channels*n, channels*4)) break;
    }
    while (n < limit && pixel(current, n,channels) == pixel(reference, n,channels)) ++n;
    return n;
}

static QLI_INLINE unsigned history_solid_run(const uint8_t *current, uint32_t color, unsigned limit,unsigned channels)
{
    uint8_t colors[16];
    for (unsigned j=0;j<4;++j) memcpy(colors+j*channels,&color,channels);
    unsigned n = 0;
    for (; limit - n >= 4; n += 4) {
        if (memcmp(current + channels*n, colors, channels*4)) break;
    }
    while (n < limit && pixel(current, n,channels) == color) ++n;
    return n;
}

static QLI_INLINE unsigned history_match(const uint8_t *image,size_t at,unsigned w,unsigned y,unsigned x,
                      unsigned *which,unsigned channels)
{
    unsigned bound=w-x<SPAN ? w-x : SPAN;
    unsigned best=0;
    if (x) {
        uint32_t left=pixel(image,at-1,channels);
        best=history_solid_run(image+channels*at,left,bound,channels);
        *which=0;
    }
    if (y) {
        const int shifts[3]={0,-1,1};
        for (unsigned k=0;k<3 && best<bound;++k) {
            int source=(int)x+shifts[k];
            if (source<0 || (unsigned)source>=w) continue;
            unsigned limit=w-(unsigned)source;
            if (limit>bound) limit=bound;
            if (limit<=best) continue;
            size_t previous=(size_t)(y-1)*w+(unsigned)source;
            unsigned n=history_equal_run(image+channels*at,image+channels*previous,limit,channels);
            if (n>best) {
                best=n;
                *which=k+1;
            }
        }
    }
    return best;
}

static inline unsigned history_hash(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    uint32_t hash = a * UINT32_C(0x9e3779b1);
    hash = (hash ^ b) * UINT32_C(0x85ebca6b);
    hash = (hash ^ c) * UINT32_C(0xc2b2ae35);
    hash = (hash ^ d) * UINT32_C(0x9e3779b1);
    return hash >> 16;
}

static unsigned history_slot(const uint8_t *image,size_t at,unsigned channels,int virtual_alpha)
{
    return history_hash(history_key(pixel(image,at,channels),virtual_alpha),
                        history_key(pixel(image,at+1,channels),virtual_alpha),
                        history_key(pixel(image,at+2,channels),virtual_alpha),
                        history_key(pixel(image,at+3,channels),virtual_alpha));
}

static unsigned history_length(const uint8_t *image,size_t at,size_t earlier,unsigned limit,unsigned channels)
{
    return history_equal_run(image+channels*at,image+channels*earlier,limit,channels);
}

static QLI_INLINE int capture(const uint8_t *image,size_t bytes,unsigned w,unsigned h,
                uint8_t *tokens,uint8_t *errors,size_t *count,size_t *literals,
                uint8_t *lengths,uint16_t *distances,size_t capacity,size_t *copies,
                uint32_t *history,unsigned channels,int virtual_alpha)
{
    if (!count || !literals || !copies || !history || !lengths || !distances) return 0;
    *count=*literals=*copies=0;
    memset(history,0,QLI_HISTORY_SLOTS*sizeof(*history));
    if (!image || !tokens || !errors || !history_shape(w,h,bytes,channels)) return 0;
    ColorCache cache={0};
    for (unsigned y=0;y<h;++y) for (unsigned x=0;x<w;) {
        size_t at=(size_t)y*w+x;
        unsigned mode=0;
        unsigned length=history_match(image,at,w,y,x,&mode,channels);
        if (w-x>=MIN_REUSE) {
            unsigned key=history_slot(image,at,channels,virtual_alpha);
            uint32_t previous=history[key];
            history[key]=(uint32_t)at+1;
            if (previous) {
                size_t earlier=previous-1u;
                size_t distance=at-earlier;
                if (distance<=QLI_HISTORY_SLOTS) {
                    unsigned limit=w-x<MAX_REUSE ? w-x : MAX_REUSE;
                    unsigned long_run=history_length(image,at,earlier,limit,channels);
                    if (long_run>=length+MIN_REUSE) {
                        if (*copies==capacity) return 0;
                        lengths[*copies]=(uint8_t)(long_run-MIN_REUSE);
                        distances[(*copies)++]=(uint16_t)(distance-1);
                        tokens[(*count)++]=REUSE;
                        x+=long_run;
                        continue;
                    }
                }
            }
        }
        if (length) {
            tokens[(*count)++]=(uint8_t)(mode*SPAN+length-1);
            x+=length;
            continue;
        }
        uint32_t value=pixel(image,at,channels);
        unsigned id=history_color_slot(history_key(value,virtual_alpha));
        if (id<CACHE-2 && cache.present[id] && cache.colors[id]==value) {
            tokens[(*count)++]=(uint8_t)(128+id);
        } else {
            tokens[(*count)++]=MISS;
            error_bytes(value,predict(image,w,y,x,channels),errors+channels*(*literals)++,channels);
            remember(&cache,value,virtual_alpha);
        }
        ++x;
    }
    return 1;
}

static int history_capture(const uint8_t *image,size_t bytes,unsigned w,unsigned h,
                    uint8_t *tokens,uint8_t *errors,size_t *count,size_t *literals,
                    uint8_t *lengths,uint16_t *distances,size_t capacity,size_t *copies,
                    uint32_t *history,unsigned channels,int virtual_alpha)
{
    if (channels==3) {
        if (virtual_alpha)
            return capture(image,bytes,w,h,tokens,errors,count,literals,lengths,distances,
                           capacity,copies,history,3,1);
        return capture(image,bytes,w,h,tokens,errors,count,literals,lengths,distances,
                       capacity,copies,history,3,0);
    }
    return capture(image,bytes,w,h,tokens,errors,count,literals,lengths,distances,
                   capacity,copies,history,channels,virtual_alpha);
}

static int encode_history_frame(const uint8_t *raw, size_t raw_bytes, unsigned w, unsigned h,
                                size_t heap, size_t limit, size_t fail_at, uint8_t **output,
                                size_t *bytes, PaletteStats *stats, int allow_phrases)
{
    if (!output || !bytes || !stats || *output || *bytes) return -1;
    *stats = (PaletteStats){0};
    unsigned channels = 4;
    if (raw_bytes == 3 * (size_t)w * h) channels = 3;
    if (!raw || !history_dimensions(w, h) || raw_bytes != channels * (size_t)w * h ||
        limit > QLI_HISTORY_FILE_CAP) return -1;
    int virtual_alpha=0;
    virtual_alpha=channels==3;

    QliWriterMemory memory = {.cap = heap, .fail_at = fail_at};
    const size_t pixels = (size_t)w * h, cap = pixels / 8;
    const size_t capacity = cap ? cap : 1;
    const size_t distance_size = capacity * sizeof(uint16_t);
    const size_t history_size = QLI_HISTORY_SLOTS * sizeof(uint32_t);
    size_t actions = 0, literals = 0, copies = 0;
    uint8_t *tokens = qli_writer_allocate(&memory, pixels);
    uint8_t *errors = qli_writer_allocate(&memory, raw_bytes);
    uint8_t *lengths = qli_writer_allocate(&memory, capacity);
    uint16_t *distance = qli_writer_allocate(&memory, distance_size);
    uint32_t *history = qli_writer_allocate(&memory, history_size);
    uint8_t *planes = NULL, *split = NULL;
    HistoryRecord records[QLI_HISTORY_RECORDS] = {0};
    QliWriterPacket phrase = {0};
    int status = -1;
    if (!tokens || !errors || !lengths || !distance || !history ||
        !history_capture(raw, raw_bytes, w, h, tokens, errors, &actions, &literals,
                         lengths, distance, cap, &copies, history, channels, virtual_alpha)) goto done;
    qli_writer_free(&memory, history, history_size);
    history = NULL;
    if (copies) {
        split = qli_writer_allocate(&memory, 2 * copies);
        if (!split) goto done;
        for (size_t i = 0; i < copies; ++i) {
            split[i] = (uint8_t)distance[i];
            split[copies + i] = (uint8_t)(distance[i] >> 8);
        }
    }
    qli_writer_free(&memory, distance, distance_size);
    distance = NULL;
    planes = qli_writer_allocate(&memory, channels * literals);
    if (!planes) goto done;
    for (size_t i = 0; i < literals; ++i)
        for (unsigned channel = 0; channel < channels; ++channel)
            planes[channel * literals + i] = errors[channels * i + channel];
    qli_writer_free(&memory, errors, raw_bytes);
    errors = NULL;

    if (!history_pack_record(&memory, tokens, actions, &records[0], 1)) goto done;
    const unsigned first = copies ? 4 : 1;
    if (copies && (!history_pack_record(&memory, lengths, copies, &records[1], 0) ||
                   !history_pack_record(&memory, split, copies, &records[2], 0) ||
                   !history_pack_record(&memory, split + copies, copies, &records[3], 0))) goto done;
    for (unsigned channel = 0; channel < channels; ++channel)
        if (!history_pack_record(&memory, planes + channel * literals, literals,
                                 &records[first + channel], 0)) goto done;

    size_t size = QLI_HISTORY_HEADER + 5 * (first + channels) + 4;
    for (unsigned i = 0; i < first + channels; ++i) size += records[i].bytes;
    if (allow_phrases) {
        phrase = pack_history_phrase(&memory, tokens, actions, records[0].bytes + 4);
        if (phrase.data) size = size - (5 + records[0].bytes) + phrase.bytes;
    }
    if (size > limit) goto done;
    uint8_t *file = qli_writer_allocate(&memory, size);
    if (!file) goto done;
    if (virtual_alpha) memcpy(file, phrase.data ? "QRVP" : "QRV3", 4);
    else if (channels == 3) memcpy(file, phrase.data ? "QRHP" : "QRH3", 4);
    else memcpy(file, phrase.data ? "QCHR" : "QCRH", 4);
    qli_write_le32(file + 4, w);
    qli_write_le32(file + 8, h);
    qli_write_le32(file + 12, (uint32_t)actions);
    qli_write_le32(file + 16, (uint32_t)literals);
    qli_write_le32(file + 20, qint_crc32(raw, raw_bytes));
    qli_write_le32(file + 24, (uint32_t)copies);
    uint8_t *out = file + QLI_HISTORY_HEADER;
    if (phrase.data) {
        memcpy(out, phrase.data, phrase.bytes);
        out += phrase.bytes;
    }
    for (unsigned i = phrase.data != NULL; i < first + channels; ++i)
        out = history_write_record(out, &records[i]);
    assert(out == file + size - 4);
    qli_write_le32(out, qint_crc32(file, size - 4));
    *output = file;
    *bytes = size;
    status = 1;
done:
    qli_writer_free(&memory, phrase.data, phrase.capacity);
    for (unsigned i = 0; i < QLI_HISTORY_RECORDS; ++i)
        qli_writer_free(&memory, records[i].packet.data, records[i].packet.capacity);
    qli_writer_free(&memory, tokens, pixels);
    qli_writer_free(&memory, errors, raw_bytes);
    qli_writer_free(&memory, lengths, capacity);
    qli_writer_free(&memory, distance, distance_size);
    qli_writer_free(&memory, history, history_size);
    qli_writer_free(&memory, planes, channels * literals);
    qli_writer_free(&memory, split, 2 * copies);
    *stats = memory.stats;
    return status;
}

int qli_encode_repeat(const uint8_t *raw, size_t raw_bytes, unsigned w, unsigned h,
                      size_t heap, size_t limit, size_t fail_at,
                      uint8_t **output, size_t *bytes, PaletteStats *stats)
{
    return encode_history_frame(raw, raw_bytes, w, h, heap, limit, fail_at, output, bytes, stats, 0);
}

int qli_encode_history(const uint8_t *raw, size_t raw_bytes, unsigned w, unsigned h,
                       size_t heap, size_t limit, size_t fail_at,
                       uint8_t **output, size_t *bytes, PaletteStats *stats)
{
    return encode_history_frame(raw, raw_bytes, w, h, heap, limit, fail_at, output, bytes, stats, 1);
}

static void *allocate_icon_packet(size_t size, void *user)
{
    (void)user;
    return qli_malloc(size);
}

static void free_icon_packet(void *data, void *user)
{
    (void)user;
    qli_free(data);
}

static int put_icon_record(QliWriterMemory *memory, uint8_t **cursor, size_t *room,
                           const uint8_t *source, size_t count)
{
    QliWriterPacket packet = {0};
    if (count && !qli_encode_byte_packet(memory, source, count, &packet)) return 0;
    int compressed = packet.data && packet.bytes < count;
    size_t bytes = compressed ? packet.bytes : count;
    int fits = *room >= ICON_RECORD && bytes <= *room - ICON_RECORD;
    if (fits) {
        uint8_t *p = *cursor;
        p[0] = (uint8_t)compressed;
        qli_write_le32(p + 1, (uint32_t)bytes);
        memcpy(p + ICON_RECORD, compressed ? packet.data : source, bytes);
        *cursor += ICON_RECORD + bytes;
        *room -= ICON_RECORD + bytes;
    }
    qli_writer_free(memory, packet.data, packet.capacity);
    return fits;
}

static QlicLiteStatus encode_icon_body(const QlicLiteDescription *d, const uint8_t *rgba, size_t bytes,
                                   size_t stride, const QlicLiteLimits *limits, uint8_t **output,
                                   size_t *written)
{
    unsigned w = d->width, h = d->height;
    if (w > ICON_SIDE || h > ICON_SIDE || d->bits != 8 || d->channels != 4 ||
        d->alpha != 1 || d->authority || d->icc_size || d->has_cicp ||
        d->has_mastering || d->has_light || stride != (size_t)w * 4)
        return QLIC_LITE_INVALID;

    size_t cap = limits->file_bytes < ICON_FILE_CAP ? limits->file_bytes : ICON_FILE_CAP;
    if (bytes != stride * h || cap < ICON_MIN_FILE)
        return QLIC_LITE_LIMIT;
    const uint8_t *hidden = icon_hidden_color(rgba, bytes);
    const int common = hidden != NULL;
    IconStreams stream;
    size_t alpha = 0, green = 0, sides = 0;
    for (unsigned y = 0; y < h; ++y) {
        for (unsigned x = 0; x < w; ++x) {
            size_t i = (size_t)y * w + x;
            const uint8_t *p = rgba + 4 * i;
            const uint8_t *left = x ? p - 4 : NULL;
            const uint8_t *above = y ? p - stride : NULL;
            const uint8_t *diag = x && y ? p - stride - 4 : NULL;
            int delta = icon_alpha_class(p) -
                icon_gradient(icon_alpha_class(left), icon_alpha_class(above), icon_alpha_class(diag));
            unsigned event = (unsigned)(delta < 0 ? delta + 3 : delta) << 3;
            if (p[3] && p[3] != 255)
                stream.alpha[alpha++] = (uint8_t)(p[3] -
                    icon_gradient(icon_alpha_value(left), icon_alpha_value(above), icon_alpha_value(diag)));
            if (p[3] || !common) {
                left = icon_visible(left, common);
                above = icon_visible(above, common);
                diag = icon_visible(diag, common);
                uint8_t error = (uint8_t)(p[1] - icon_color(1, left, above, diag));
                if (error) {
                    event |= 1;
                    stream.green[green++] = error;
                }
                int correction = error < 128 ? error : error - 256;
                uint8_t red = (uint8_t)(p[0] -
                    icon_clamp(icon_color(0, left, above, diag) + correction, 0, 255));
                uint8_t blue = (uint8_t)(p[2] -
                    icon_clamp(icon_color(2, left, above, diag) + correction, 0, 255));
                if (red) {
                    event |= 2;
                    stream.sides[sides++] = red;
                }
                if (blue) {
                    event |= 4;
                    stream.sides[sides++] = blue;
                }
            }
            stream.events[i] = (uint8_t)event;
        }
    }
    size_t pixels = (size_t)w * h;
    size_t head = ICON_HEADER + 3 * (size_t)common;
    size_t room = head + 4 * ICON_RECORD + pixels + alpha + green + sides + 4;
    if (room > cap) room = cap;
    uint8_t *out = qli_malloc(room);
    if (!out) return QLIC_LITE_ALLOCATOR;
    memcpy(out, "QIE1", 4);
    out[4] = (uint8_t)(w - 1);
    out[5] = (uint8_t)(h - 1);
    out[6] = (uint8_t)common;
    qli_write_le32(out + 7, qint_crc32(rgba, bytes));
    if (common) memcpy(out + ICON_HEADER, hidden, 3);
    uint8_t *cursor = out + head;
    room -= head + 4;
    const QlicLiteAllocator allocator = {allocate_icon_packet, free_icon_packet, NULL};
    QliWriterMemory memory = {.allocator = &allocator};
    if (!put_icon_record(&memory, &cursor, &room, stream.events, pixels) ||
        !put_icon_record(&memory, &cursor, &room, stream.alpha, alpha) ||
        !put_icon_record(&memory, &cursor, &room, stream.green, green) ||
        !put_icon_record(&memory, &cursor, &room, stream.sides, sides)) {
        qli_free(out);
        return QLIC_LITE_LIMIT;
    }
    qli_write_le32(cursor, qint_crc32(out, (size_t)(cursor - out)));
    *written = (size_t)(cursor - out) + 4;
    *output = out;
    return QLIC_LITE_OK;
}

QlicLiteStatus qli_encode_icon(const QlicLiteDescription *description, const uint8_t *rgba, size_t bytes,
                          size_t stride, const QlicLiteLimits *limits, const QlicLiteAllocator *allocator,
                          QlicLiteFile *file, QlicLiteStats *stats)
{
    return qli_encode_owned(description, rgba, bytes, stride, limits, allocator,
                            file, stats, encode_icon_body);
}

#ifdef QLI_NUMERIC_AVX2

static QLI_NUMERIC_TARGET inline __m128i numeric_load16(const void *p)
{
    __m128i value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static QLI_NUMERIC_TARGET inline void numeric_store16(void *p, __m128i value)
{
    memcpy(p, &value, sizeof(value));
}

static QLI_NUMERIC_TARGET inline __m256i numeric_load32(const void *p)
{
    __m256i value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static QLI_NUMERIC_TARGET inline void numeric_store32(void *p, __m256i value)
{
    memcpy(p, &value, sizeof(value));
}
#endif
#ifdef QLI_NUMERIC_SSE42

static QLI_SSE42_TARGET inline __m128i sse42_numeric_load16(const void *p)
{
    __m128i value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static QLI_SSE42_TARGET inline void sse42_numeric_store16(void *p, __m128i value)
{
    memcpy(p, &value, sizeof(value));
}

static QLI_SSE42_TARGET inline SsePair sse42_numeric_load32(const void *p)
{
    SsePair value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static QLI_SSE42_TARGET inline void sse42_numeric_store32(void *p, SsePair value)
{
    memcpy(p, &value, sizeof(value));
}
#endif

static inline unsigned bit_width(unsigned value)
{
    return value ? 32u-(unsigned)__builtin_clz(value) : 0;
}

enum { NUMERIC_BANKS=16, NUMERIC_TILE=8, NUMERIC_SPAN=256 };
enum { WAVE_CONTEXTS=18, WAVE_KEYS=32*18, WAVE_MODEL_BYTES=32*542, WAVE_LOGS=8192 };

typedef struct {
    union {
        uint64_t hist[WAVE_KEYS*256];
        uint64_t emit[WAVE_KEYS*256];
    };
    uint32_t prefix[32][WAVE_CONTEXTS+1][256];
    uint32_t sample_totals[32][WAVE_CONTEXTS+1];
    uint16_t north[16384+3];
    uint8_t map[WAVE_KEYS];
} WaveScratch;
enum { DIRECTION_KEYS=3*WAVE_KEYS, DIRECTION_BOOKS=32, DIRECTION_SYMBOLS=256 };

typedef struct {
    union {
        uint64_t hist[DIRECTION_KEYS][DIRECTION_SYMBOLS];
        uint64_t emit[DIRECTION_KEYS*DIRECTION_SYMBOLS];
    };
    uint32_t old[DIRECTION_BOOKS*3][DIRECTION_SYMBOLS];
    uint32_t prefix[DIRECTION_BOOKS+1][3][DIRECTION_SYMBOLS];
    uint32_t assigned[DIRECTION_BOOKS][DIRECTION_SYMBOLS];
    float price[DIRECTION_SYMBOLS][DIRECTION_BOOKS];
    uint8_t map[DIRECTION_KEYS];
} DirectionScratch;

static double wave_log[WAVE_LOGS];

static void fill_wave_log(void)
{
    for (unsigned i = 1; i < WAVE_LOGS; ++i)
        wave_log[i] = i * log2((double)i);
}

#if defined(_WIN32)
static INIT_ONCE wave_log_once = INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK init_wave_log(PINIT_ONCE once, PVOID parameter, PVOID *context)
{
    (void)once;
    (void)parameter;
    (void)context;
    fill_wave_log();
    return TRUE;
}

static int prepare_wave_log(void)
{
    return InitOnceExecuteOnce(&wave_log_once, init_wave_log, NULL, NULL);
}
#else
static pthread_once_t wave_log_once = PTHREAD_ONCE_INIT;

static int prepare_wave_log(void)
{
    return pthread_once(&wave_log_once, fill_wave_log) == 0;
}
#endif

typedef struct {
    uint8_t *data;
    size_t used, cap;
    QliWriterMemory *memory;
} NumericOutput;

typedef struct {
    uint16_t count[16], live;
    uint8_t seed, bank;
} NumericGroup;

enum { READY_MODEL_BYTES=16*(34+2*255) };
typedef struct { unsigned n, zero, positive, mag; } CueStats;
typedef struct {
    CueStats stats[16][49];
    uint32_t hist[32][256];
    uint32_t fields[32*256], reciprocal[32*256];
} NumericConditional;

typedef struct {
    uint8_t *plane, *error, *cue[2];
    NumericConditional *cond;
    unsigned guides;
    int diagonal, packed_rgb;
    WaveScratch *wave;
    DirectionScratch *dir;
    int logical;
    uint32_t *words;
    uint8_t *mode, *map, *flat, *constant, *queue, *packet;
    NumericGroup *groups;
    int reused_plane, fitted, vector;
    int8_t fit[2][10];
    size_t pixels, modes, blocks, tiles, packet_cap;
} NumericScratch;

static int append(NumericOutput *out,const void *data,size_t bytes)
{
    if (bytes>out->cap-out->used) return 0;
    if (bytes) memcpy(out->data+out->used,data,bytes);
    out->used+=bytes;
    return 1;
}

static int write_numeric_record(NumericOutput *out,const uint8_t *data,size_t bytes,unsigned kind)
{
    uint8_t head[5]={(uint8_t)kind};
    qli_write_le32(head+1,(uint32_t)bytes);
    if (sizeof(head)+bytes>out->cap-out->used) return 0;
    return append(out,head,sizeof(head)) && append(out,data,bytes);
}

static int packet_record(NumericOutput *out, const uint8_t *data, size_t count, int action)
{
    QliWriterPacket packet;
    int ok = action ? qli_encode_action_packet(out->memory, data, count, &packet)
                    : qli_encode_byte_packet(out->memory, data, count, &packet);
    if (!ok) return 0;
    ok = packet.bytes < count ? write_numeric_record(out, packet.data, packet.bytes, 1)
                              : write_numeric_record(out, data, count, 0);
    qli_writer_free(out->memory, packet.data, packet.capacity);
    return ok;
}

static int map_record(NumericOutput *out, const uint8_t *data, size_t count)
{
    return count <= 50 ? write_numeric_record(out, data, count, 0)
                       : packet_record(out, data, count, 0);
}

static size_t packed_record(uint8_t *file,size_t cap,const uint8_t *symbols,
                            size_t count,const uint64_t hist[256])
{
    RansModel model;
    if (!qlr_model_normalize(hist,count,&model) || !model.used) return 0;
    memset(file,0,RANS_HEADER_BYTES);
    memcpy(file,model.used==1 ? "QBS1" : "QVP1",4);
    file[4]=model.used==1 ? 4 : QVP_LANES;
    file[5]=QLIC_RANS_SCALE_BITS;
    qli_write_le64(file+6,count);
    size_t end=RANS_HEADER_BYTES;
    if (!qlr_model_write(&model,file,cap,&end)) return 0;
    if (model.used==1) {
        memset(file+end,0,32);
        for (unsigned lane=0;lane<4;++lane) qli_write_le32(file+end+4*lane,RANS_BYTE_L);
        return end+32;
    }
    uint64_t reciprocal[256];
    for (unsigned s=0;s<256;++s)
        if (model.frequency[s]) reciprocal[s]=(UINT64_C(1)<<32)/model.frequency[s];
    uint32_t state[QVP_LANES];
    for (unsigned lane=0;lane<QVP_LANES;++lane) state[lane]=QVP_LOW;
    uint8_t *tail=file+cap;
    for (size_t i=count;i--;) {
        unsigned lane=(unsigned)i&(QVP_LANES-1), s=symbols[i];
        uint32_t f=model.frequency[s], x=state[lane];
        if (x>=(f<<20)) {
            tail-=2;
            qli_write_le16(tail,(uint16_t)x);
            x>>=16;
        }
        uint32_t q=(uint32_t)((x*reciprocal[s])>>32), rem=x-q*f;
        if (rem>=f) { ++q; rem-=f; }
        state[lane]=(q<<12)+rem+model.cumulative[s];
    }
    size_t bytes=(size_t)(file+cap-tail);
    size_t start=end+4*QVP_LANES;
    memmove(file+start,tail,bytes);
    for (unsigned lane=0;lane<QVP_LANES;++lane)
        qli_write_le32(file+end+4*lane,state[lane]);
    return start+bytes;
}

static unsigned wrapped_magnitude(unsigned value) { return value<128 ? value : 256-value; }

static void symbol_classes(uint8_t table[256])
{
    static const unsigned cut[]={0,1,2,3,7,15,31,63};
    for (unsigned v=0;v<256;++v) {
        unsigned m=wrapped_magnitude(v), r=0;
        while (r<8 && m>cut[r]) ++r;
        table[v]=(uint8_t)(r==0 ? 0 : r==8 ? 15 : 2*r-1+(v>=128));
    }
}

static int flat_at(const NumericScratch *s,size_t index)
{
    size_t span=index/NUMERIC_SPAN;
    return (s->flat[span/8]>>(span%8))&1;
}

static int uniform(const uint8_t *p,size_t size)
{
    unsigned first=p[0];
    const uint64_t same=UINT64_C(0x0101010101010101)*first;
    if (size>=8) {
        uint64_t word;
        memcpy(&word,p,8);
        if (word!=same) return 0;
        p+=8;
        size-=8;
    }
    for (;size>=8;p+=8,size-=8) {
        uint64_t word;
        memcpy(&word,p,sizeof(word));
        if (word!=same) return 0;
    }
    for (size_t i=0;i<size;++i) if (p[i]!=first) return 0;
    return 1;
}

static size_t flat_spans(NumericScratch *s)
{
    memset(s->flat,0,(s->blocks+7)/8);
    size_t count=0;
    for (size_t span=0;span<s->blocks;++span) {
        size_t start=span*NUMERIC_SPAN, end=start+NUMERIC_SPAN;
        if (end>s->pixels) end=s->pixels;
        uint8_t v=s->error[start];
        if (uniform(s->error+start,end-start)) {
            s->flat[span/8]|=(uint8_t)(1u<<(span%8));
            s->constant[count++]=(uint8_t)v;
        }
    }
    return count;
}

static unsigned span_end(unsigned at,unsigned limit)
{
    return at+NUMERIC_TILE<limit ? at+NUMERIC_TILE : limit;
}

static unsigned score(const NumericGroup *group,const int16_t price[16])
{
    unsigned sum=0;
    for (unsigned i=0;i<16;++i) sum+=(unsigned)(group->count[i]*price[i]);
    return sum;
}

typedef struct {
    unsigned count, magnitude;
} TileStats;

static TileStats stage_scalar(const NumericScratch *s,NumericGroup *group,const uint8_t *classes,
                              uint8_t *stage,unsigned w,unsigned h,unsigned x,unsigned y)
{
    TileStats stats={0,0};
    for (unsigned r=y;r<span_end(y,h);++r) {
        size_t end=(size_t)r*w+span_end(x,w);
        for (size_t at=(size_t)r*w+x;at<end;) {
            size_t next=(at|255u)+1;
            if (next>end) next=end;
            if (!flat_at(s,at)) {
                for (size_t p=at;p<next;++p) {
                    unsigned v=s->error[p];
                    stage[stats.count++]=(uint8_t)v;
                    ++group->count[classes[v]];
                    stats.magnitude+=wrapped_magnitude(v);
                }
            }
            at=next;
        }
    }
    return stats;
}

static size_t bank_map(NumericScratch *s,unsigned w,unsigned h,size_t totals[16])
{
    uint8_t classes[256];
    symbol_classes(classes);
    unsigned seed[16][16]={0}, sizes[16]={0};
    size_t staged=0;
    s->reused_plane=1;
    NumericGroup *group=s->groups;
    for (unsigned y=0;y<h;y+=NUMERIC_TILE)
        for (unsigned x=0;x<w;x+=NUMERIC_TILE,++group) {
            memset(group,0,sizeof(*group));
            TileStats stats;
                stats=stage_scalar(s,group,classes,s->plane+staged,w,h,x,y);
            unsigned count=stats.count;
            group->live=(uint16_t)count;
            staged+=count;
            group->bank=255;
            if (!count) continue;
            unsigned mean=stats.magnitude/count;
            unsigned id=mean ? bit_width(mean) : 0;
            if (2u*group->count[0]>=count) id+=8;
            group->seed=(uint8_t)id;
            group->bank=(uint8_t)id;
            sizes[id]+=count;
            for (unsigned i=0;i<16;++i) seed[id][i]+=group->count[i];
        }
    int16_t price[16][16];
    for (unsigned id=0;id<16;++id)
        if (sizes[id])
            for (unsigned i=0;i<16;++i)
                price[id][i]=(int16_t)nearbyint(-256*log2((seed[id][i]+.5)/(sizes[id]+8.0)));
    size_t live=0;
    memset(s->map,0,(s->tiles+1)/2);
    for (size_t i=0;i<s->tiles;++i) {
        group=&s->groups[i];
        if (group->bank==255) continue;
        unsigned best=group->seed, minimum=score(group,price[best]);
        for (unsigned id=0;id<16;++id) {
            if (!sizes[id]) continue;
            unsigned value=score(group,price[id]);
            if (value<minimum || (value==minimum && best!=group->seed && id<best)) {
                best=id;
                minimum=value;
            }
        }
        group->bank=(uint8_t)best;
        s->map[live/2]|=(uint8_t)(best<<(4*(live%2)));
        ++live;
        totals[best]+=group->live;
    }
    return (live+1)/2;
}

#ifdef QLI_NUMERIC_AVX2
static QLI_NUMERIC_TARGET __m128i cue_bytes(__m128i v) {
    const __m128i zero=_mm_setzero_si128(), one=_mm_set1_epi8(1), two=_mm_set1_epi8(2);
    __m128i mag=_mm_abs_epi8(v);
    __m128i above1=_mm_andnot_si128(_mm_cmpeq_epi8(_mm_min_epu8(mag,one),mag),two);
    __m128i above4=_mm_andnot_si128(_mm_cmpeq_epi8(_mm_min_epu8(mag,_mm_set1_epi8(4)),mag),two);
    __m128i sign=_mm_and_si128(_mm_cmpgt_epi8(zero,v),one);
    __m128i code=_mm_add_epi8(_mm_add_epi8(one,sign),_mm_add_epi8(above1,above4));
    return _mm_andnot_si128(_mm_cmpeq_epi8(v,zero),code);
}
static QLI_NUMERIC_TARGET size_t cue_blocks(const uint8_t *error,uint8_t *out,size_t pixels)
{
    size_t at=0;
    for (;pixels-at>=16;at+=16)
        numeric_store16(out+at,cue_bytes(numeric_load16(error+at)));
    return at;
}
#endif
#ifdef QLI_NUMERIC_SSE42
static QLI_SSE42_TARGET __m128i sse42_cue_bytes(__m128i v) {
    const __m128i zero=_mm_setzero_si128(), one=_mm_set1_epi8(1), two=_mm_set1_epi8(2);
    __m128i mag=_mm_abs_epi8(v);
    __m128i above1=_mm_andnot_si128(_mm_cmpeq_epi8(_mm_min_epu8(mag,one),mag),two);
    __m128i above4=_mm_andnot_si128(_mm_cmpeq_epi8(_mm_min_epu8(mag,_mm_set1_epi8(4)),mag),two);
    __m128i sign=_mm_and_si128(_mm_cmpgt_epi8(zero,v),one);
    __m128i code=_mm_add_epi8(_mm_add_epi8(one,sign),_mm_add_epi8(above1,above4));
    return _mm_andnot_si128(_mm_cmpeq_epi8(v,zero),code);
}
static QLI_SSE42_TARGET size_t sse42_cue_blocks(const uint8_t *error,uint8_t *out,size_t pixels)
{
    size_t at=0;
    for (;pixels-at>=16;at+=16)
        sse42_numeric_store16(out+at,sse42_cue_bytes(sse42_numeric_load16(error+at)));
    return at;
}
#endif

static void capture_cue(NumericScratch *s) {
    uint8_t *out=s->cue[s->guides++];
    size_t at=0;
#ifdef QLI_NUMERIC_SSE42
    if (s->vector) at=QLI_NUMERIC_FN(s->vector, cue_blocks)(s->error,out,s->pixels);
#endif
    const uint64_t lanes=UINT64_C(0x0001000100010001);
    const uint64_t bytes=lanes*255;
    const uint64_t guard=lanes*32768;
    for (;s->pixels-at>=4;at+=4) {
        uint64_t raw=qli_read_le32(s->error+at);
        raw=(raw|(raw<<16))&UINT64_C(0x0000ffff0000ffff);
        raw=(raw|(raw<<8))&bytes;
        const uint64_t sign=(raw&(lanes*128))>>7;
        const uint64_t mag=(raw^(sign*255))+sign;
        const uint64_t nz=((guard+mag-lanes)&guard)>>15;
        const uint64_t above1=((guard+mag-2*lanes)&guard)>>15;
        const uint64_t above4=((guard+mag-5*lanes)&guard)>>15;
        const uint64_t cue=nz+sign+2*(above1+above4);
        const uint64_t pairs=(cue&UINT64_C(0x000000ff000000ff)) |
                             ((cue>>8)&UINT64_C(0x0000ff000000ff00));
        qli_write_le32(out+at,(uint32_t)(pairs|(pairs>>16)));
    }
    for (;at<s->pixels;++at) {
        unsigned v=s->error[at], m=wrapped_magnitude(v);
        out[at]=(uint8_t)(v ? 1+2*((m>1)+(m>4))+(v>=128) : 0);
    }
}

static unsigned cue_key(const NumericScratch *s,size_t at) {
    return s->guides==2 ? 7*s->cue[0][at]+s->cue[1][at] : s->cue[0][at];
}

static void observe(CueStats *count,unsigned v) {
    ++count->n;
    count->zero+=!v;
    count->positive+=v && v<128;
    count->mag+=wrapped_magnitude(v);
}

static unsigned assign_model(const CueStats *s) {
    if (!s->n) return 0;
    unsigned z=4*s->zero/s->n;
    if (z==4) z=3;
    unsigned nz=s->n-s->zero;
    unsigned mag=(s->mag>nz)+(s->mag>3*nz)+(s->mag>8*nz);
    return 8*z+2*mag+(2*s->positive>=nz);
}

typedef struct {
    unsigned zero, positive, negative, tail;
} ReadyPrice;

static void add_stats(CueStats *to,const CueStats *from) {
    to->n+=from->n;
    to->zero+=from->zero;
    to->positive+=from->positive;
    to->mag+=from->mag;
}

static unsigned price_bits(double probability) {
    return (unsigned)(-1024.0*log2(probability)+0.5);
}

static ReadyPrice price_stats(const CueStats *s) {
    unsigned nz=s->n-s->zero;
    double pzero=(s->zero+.5)/(s->n+1.0);
    double positive=(s->positive+.5)/(nz+1.0);
    double stop=(nz+.5)/(s->mag+1.0);
    double nonzero=(1.0-pzero)*stop;
    return (ReadyPrice){price_bits(pzero),price_bits(nonzero*positive),
                        price_bits(nonzero*(1.0-positive)),price_bits(1.0-stop)};
}

static uint64_t ready_cost(const CueStats *s,const ReadyPrice *p) {
    unsigned nz=s->n-s->zero;
    return (uint64_t)s->zero*p->zero+(uint64_t)s->positive*p->positive+
           (uint64_t)(nz-s->positive)*p->negative+(uint64_t)(s->mag-nz)*p->tail;
}

static void choose_ready(const NumericConditional *c,const CueStats bank[16],unsigned width,
                         uint8_t map[16*49],uint8_t route[16][64]) {
    CueStats center[32]={0};
    for (unsigned b=0;b<16;++b)
        for (unsigned j=0;j<width;++j) {
            const CueStats *stats=&c->stats[b][j];
            if (stats->n>=8) add_stats(&center[assign_model(stats)],stats);
        }
    ReadyPrice cost[32];
    unsigned used[32], count=0;
    for (unsigned k=0;k<32;++k) if (center[k].n) {
        cost[k]=price_stats(&center[k]);
        used[count++]=k;
    }
    for (unsigned b=0;b<16;++b) {
        memset(route[b],0,64);
        for (unsigned j=0;j<width;++j) {
            const CueStats *stats=&c->stats[b][j];
            unsigned model=assign_model(stats->n>=8 ? stats : &bank[b]);
            if (stats->n>=8) {
                uint64_t best=ready_cost(stats,&cost[model]);
                for (unsigned i=0;i<count;++i) {
                    unsigned id=used[i];
                    uint64_t candidate=ready_cost(stats,&cost[id]);
                    if (candidate<best) {
                        best=candidate;
                        model=id;
                    }
                }
            }
            route[b][j]=map[b*width+j]=(uint8_t)model;
        }
    }
}

typedef struct {
    unsigned group,first,end,split;
    double gain;
} WaveRange;

static double wave_cost(unsigned n)
{
    return n<WAVE_LOGS ? wave_log[n] : n*log2((double)n);
}

static void wave_split(const WaveScratch *work,WaveRange *range)
{
    range->gain=0;
    const uint32_t (*prefix)[256]=work->prefix[range->group];
    const uint32_t *totals=work->sample_totals[range->group];
    unsigned first=range->first,end=range->end;
    for (unsigned split=first+1;split<end;++split) {
        unsigned left=totals[split]-totals[first], right=totals[end]-totals[split];
        if (left<32 || right<32) continue;
        double gain=wave_cost(left+right)-wave_cost(left)-wave_cost(right);
        for (unsigned v=0;v<256;++v) {
            unsigned a=prefix[split][v]-prefix[first][v], b=prefix[end][v]-prefix[split][v];
            if (a && b) gain-=wave_cost(a+b)-wave_cost(a)-wave_cost(b);
        }
        if (gain>range->gain) {
            range->gain=gain;
            range->split=split;
        }
    }
}

static unsigned wave_fit(NumericScratch *s,unsigned groups)
{
    WaveScratch *work=s->wave;
    WaveRange ranges[32];
    unsigned n=0;
    for (unsigned g=0;g<groups;++g) {
        memset(work->prefix[g][0],0,256*sizeof(uint32_t));
        work->sample_totals[g][0]=0;
        unsigned live=0;
        for (unsigned c=0;c<WAVE_CONTEXTS;++c) {
            const uint64_t *source=work->hist+(g*WAVE_CONTEXTS+c)*256;
            const uint32_t *before=work->prefix[g][c];
            uint32_t *next=work->prefix[g][c+1];
            unsigned total=work->sample_totals[g][c];
            for (unsigned v=0;v<256;++v) {
                unsigned sample=(unsigned)(source[v]>>32);
                live+=(unsigned)source[v];
                next[v]=before[v]+sample;
                total+=sample;
            }
            work->sample_totals[g][c+1]=total;
        }
        if (!live) continue;
        ranges[n]=(WaveRange){g,0,WAVE_CONTEXTS,0,0};
        wave_split(work,ranges+n++);
    }
    while (n && n<32) {
        unsigned pick=0;
        for (unsigned i=1;i<n;++i)
            if (ranges[i].gain>ranges[pick].gain) pick=i;
        WaveRange old=ranges[pick];
        if (old.gain<128) break;
        memmove(ranges+pick+2,ranges+pick+1,(n-pick-1)*sizeof(*ranges));
        ranges[pick]=(WaveRange){old.group,old.first,old.split,0,0};
        ranges[pick+1]=(WaveRange){old.group,old.split,old.end,0,0};
        wave_split(work,ranges+pick);
        wave_split(work,ranges+pick+1);
        ++n;
    }
    memset(work->map,255,groups*WAVE_CONTEXTS);
    memset(s->cond->hist,0,sizeof(s->cond->hist));
    for (unsigned i=0;i<n;++i) {
        const WaveRange *r=ranges+i;
        for (unsigned c=r->first;c<r->end;++c) {
            unsigned key=r->group*WAVE_CONTEXTS+c;
            work->map[key]=(uint8_t)i;
            const uint64_t *row=work->hist+key*256;
            for (unsigned v=0;v<256;++v) s->cond->hist[i][v]+=(uint32_t)row[v];
        }
    }
    return n;
}

static int wave_record(NumericOutput *out, const uint8_t *data, size_t count)
{
    return count ? packet_record(out, data, count, 1) : write_numeric_record(out, data, 0, 0);
}

typedef struct {
    unsigned mass, slots, cutoff, sum, assigned;
    int minimum_each;
} WaveFrequency;

static WaveFrequency wave_frequency(const uint32_t *hist,unsigned total,unsigned used,int packed_rgb)
{
    WaveFrequency f={.mass=total,.slots=4096-used,.minimum_each=1};
    if (!packed_rgb) return f;
    f.slots=4096;
    f.minimum_each=0;
    for (;;) {
        const unsigned cutoff=(f.mass-1)/f.slots;
        if (cutoff==f.cutoff) return f;
        f.cutoff=cutoff;
        unsigned small=0, reserved=0;
        for (unsigned v=0;v<256;++v) {
            if (hist[v] && hist[v]<=cutoff) {
                small+=hist[v];
                ++reserved;
            }
        }
        f.mass=total-small;
        f.slots=4096-reserved;
    }
}

static inline unsigned wave_frequency_next(WaveFrequency *f,unsigned count)
{
    if (count<=f->cutoff) return 1;
    f->sum+=count;
    unsigned rounding=f->minimum_each ? 0 : f->mass/2;
    const unsigned top=(unsigned)(((uint64_t)f->slots*f->sum+rounding)/f->mass);
    unsigned freq=top-f->assigned+(unsigned)f->minimum_each;
    f->assigned=top;
    return freq;
}

static int wave_models(NumericOutput *out,NumericConditional *cond,unsigned models,int packed_rgb)
{
    uint8_t data[WAVE_MODEL_BYTES],*end=data;
    memset(cond->fields,0,models*256*sizeof(*cond->fields));
    memset(cond->reciprocal,0,models*256*sizeof(*cond->reciprocal));
    for (unsigned k=0;k<models;++k) {
        uint32_t *hist=cond->hist[k], used=0, total=0;
        for (unsigned v=0;v<256;++v) {
            total+=hist[v];
            used+=hist[v]!=0;
        }
        if (!total) return 0;
        uint8_t *mask=end;
        memset(mask,0,32);
        end+=32;
        WaveFrequency model=wave_frequency(hist,total,used,packed_rgb);
        uint32_t occupied=0,last=0;
        for (unsigned v=0;v<256;++v) if (hist[v]) {
            unsigned f=wave_frequency_next(&model,hist[v]);
            ++occupied;
            unsigned idx=256*k+v;
            const unsigned top=last+f;
            cond->fields[idx]=(f<<12)|last;
            cond->reciprocal[idx]=f==1 ? UINT32_MAX : (uint32_t)((UINT64_C(1)<<32)/f);
            last=top;
            mask[v/8]|=(uint8_t)(1u<<(v%8));
            if (occupied<used) {
                qli_write_le16(end,(uint16_t)f);
                end+=2;
            }
        }
    }
    uint8_t prefix[3]={(uint8_t)models};
    qli_write_le16(prefix+1,(uint16_t)(end-data));
    return append(out,prefix,sizeof(prefix)) && wave_record(out,data,(size_t)(end-data));
}

static unsigned wave_context(unsigned west)
{
    unsigned bin=31u-(unsigned)__builtin_clz(west*west+1);
    return bin<17 ? bin : 17;
}

static uint64_t wave_count(unsigned lane,unsigned sample)
{
    return 1+(((uint64_t)((lane&7)==sample))<<32);
}

static void direction_fold(NumericScratch *s,unsigned keys)
{
    for (unsigned k=0;k<keys;++k)
        for (unsigned v=0;v<256;++v)
            s->wave->hist[256*k+v]=s->dir->hist[3*k][v]+s->dir->hist[3*k+1][v]+s->dir->hist[3*k+2][v];
}

static void direction_old_counts(NumericScratch *s,unsigned keys,unsigned books)
{
    memset(s->dir->old,0,3*books*sizeof(s->dir->old[0]));
    for (unsigned k=0;k<keys;++k) {
        unsigned book=s->wave->map[k];
        if (book>=books) continue;
        for (unsigned d=0;d<3;++d) {
            uint32_t *out=s->dir->old[3*book+d];
            const uint64_t *row=s->dir->hist[3*k+d];
            for (unsigned v=0;v<256;++v) out[v]+=(uint32_t)row[v];
        }
    }
}

static unsigned direction_seed(NumericScratch *s,unsigned books,unsigned limit)
{
    DirectionScratch *work=s->dir;
    unsigned order[32],live=0;
    double mean[32];
    for (unsigned k=0;k<books;++k) {
        uint64_t total=0, magnitude=0;
        for (unsigned d=0;d<3;++d)
            for (unsigned v=0;v<256;++v) {
                unsigned count=work->old[3*k+d][v];
                total+=count;
                magnitude+=(uint64_t)count*(v<128 ? v : 256-v);
            }
        if (!total) continue;
        mean[k]=(double)magnitude/(double)total;
        unsigned at=live++;
        while (at && mean[order[at-1]]>mean[k]) {
            order[at]=order[at-1];
            --at;
        }
        order[at]=k;
    }
    if (!live) return 0;
    unsigned totals[33][3]={{0}};
    memset(work->prefix[0],0,sizeof(work->prefix[0]));
    for (unsigned i=0;i<live;++i)
        for (unsigned d=0;d<3;++d) {
            unsigned total=totals[i][d];
            for (unsigned v=0;v<256;++v) {
                unsigned count=work->old[3*order[i]+d][v];
                work->prefix[i+1][d][v]=work->prefix[i][d][v]+count;
                total+=count;
            }
            totals[i+1][d]=total;
        }
    double costs[33][33];
    uint8_t begins[33][33], widths[33][33];
    for (unsigned i=0;i<=live;++i)
        for (unsigned k=0;k<=limit;++k) costs[i][k]=HUGE_VAL;
    costs[0][0]=0;
    for (unsigned end=1;end<=live;++end) {
        double pooled[32],split[32];
        for (unsigned begin=0;begin<end;++begin) {
            unsigned a=totals[end][0]-totals[begin][0];
            unsigned b=totals[end][1]-totals[begin][1];
            unsigned c=totals[end][2]-totals[begin][2];
            double one=wave_cost(a+b+c),three=wave_cost(a)+wave_cost(b)+wave_cost(c);
            for (unsigned v=0;v<256;++v) {
                unsigned x=work->prefix[end][0][v]-work->prefix[begin][0][v];
                unsigned y=work->prefix[end][1][v]-work->prefix[begin][1][v];
                unsigned z=work->prefix[end][2][v]-work->prefix[begin][2][v];
                one-=wave_cost(x+y+z);
                three-=wave_cost(x)+wave_cost(y)+wave_cost(z);
            }
            pooled[begin]=one;
            split[begin]=(a && b && c) ? three : HUGE_VAL;
        }
        for (unsigned width=1;width<=3;width+=2) {
            const double *interval=width==1 ? pooled : split;
            for (unsigned begin=0;begin<end;++begin)
                for (unsigned k=width;k<=limit;++k) {
                    double value=costs[begin][k-width]+interval[begin];
                    if (value<costs[end][k]) {
                        costs[end][k]=value;
                        begins[end][k]=(uint8_t)begin;
                        widths[end][k]=(uint8_t)width;
                    }
                }
        }
    }
    unsigned models=1;
    for (unsigned k=2;k<=limit;++k)
        if (costs[live][k]<costs[live][models]) models=k;
    unsigned remaining=models;
    for (unsigned end=live;end;) {
        unsigned begin=begins[end][remaining],width=widths[end][remaining];
        unsigned base=remaining-width;
        if (width==1) {
            for (unsigned v=0;v<256;++v) {
                unsigned sum=0;
                for (unsigned d=0;d<3;++d) sum+=work->prefix[end][d][v]-work->prefix[begin][d][v];
                s->cond->hist[base][v]=sum;
            }
        } else {
            for (unsigned d=0;d<3;++d)
                for (unsigned v=0;v<256;++v)
                    s->cond->hist[base+d][v]=work->prefix[end][d][v]-work->prefix[begin][d][v];
        }
        remaining=base;
        end=begin;
    }
    return models;
}

static void direction_normalize(NumericConditional *cond,unsigned books,int packed_rgb)
{
    for (unsigned k=0;k<books;++k) {
        uint32_t *row=cond->hist[k];
        unsigned total=0,used=0;
        for (unsigned v=0;v<256;++v) {
            total+=row[v];
            used+=row[v]!=0;
        }
        WaveFrequency model=wave_frequency(row,total,used,packed_rgb);
        for (unsigned v=0;v<256;++v)
            if (row[v]) row[v]=wave_frequency_next(&model,row[v]);
    }
}

static unsigned direction_best(const DirectionScratch *work,const uint32_t *row,
                               const uint8_t *symbols,unsigned occupied,unsigned books)
{
    float best=-HUGE_VALF;
    unsigned choice=0;
    for (unsigned k=0;k<books;k+=8) {
        float scores[8];
        memset(scores,0,sizeof(scores));
        for (unsigned i=0;i<occupied;++i) {
            unsigned v=symbols[i];
            float count=(float)row[v];
            for (unsigned j=0;j<8;++j) {
                float price=count*work->price[v][k+j];
                scores[j]+=price;
            }
        }
        unsigned count=books-k<8 ? books-k : 8;
        for (unsigned j=0;j<count;++j) if (scores[j]>best) {
            best=scores[j];
            choice=k+j;
        }
    }
    return choice;
}

static unsigned direction_assign(NumericScratch *s,unsigned rows,unsigned books,int logical)
{
    DirectionScratch *work=s->dir;
    memset(work->price,0,sizeof(work->price));
    for (unsigned k=0;k<books;++k) {
        unsigned sum=0;
        for (unsigned v=0;v<256;++v) sum+=s->cond->hist[k][v];
        float total=log2f((float)sum+128.0f);
        for (unsigned v=0;v<256;++v) {
            unsigned count=s->cond->hist[k][v], twice=2*count+1;
            float log_count=twice<WAVE_LOGS ? (float)(wave_log[twice]/twice)-1.0f : log2f((float)count+0.5f);
            work->price[v][k]=log_count-total;
        }
    }
    memset(work->assigned,0,books*sizeof(work->assigned[0]));
    uint8_t touched[32]={0};
    for (unsigned k=0;k<rows;++k) {
        uint32_t values[256];
        const uint32_t *row;
        if (logical) {
            for (unsigned v=0;v<256;++v) values[v]=(uint32_t)work->hist[k][v];
            row=values;
            work->map[k]=255;
        } else row=work->old[k];
        uint8_t symbols[256];
        unsigned occupied=0;
        for (unsigned v=0;v<256;++v)
            if (row[v]) symbols[occupied++]=(uint8_t)v;
        if (!occupied) continue;
        unsigned best=direction_best(work,row,symbols,occupied,books);
        touched[best]=1;
        for (unsigned i=0;i<occupied;++i) {
            unsigned v=symbols[i];
            work->assigned[best][v]+=row[v];
        }
        if (logical) work->map[k]=(uint8_t)best;
    }
    uint8_t remap[32];
    unsigned used=0;
    for (unsigned k=0;k<books;++k) if (touched[k]) {
        remap[k]=(uint8_t)used;
        memcpy(s->cond->hist[used++],work->assigned[k],sizeof(work->assigned[k]));
    }
    if (logical)
        for (unsigned k=0;k<rows;++k)
            if (work->map[k]!=255) work->map[k]=remap[work->map[k]];
    return used;
}

static unsigned direction_fit(NumericScratch *s,unsigned keys,unsigned old_books)
{
    unsigned limit=s->pixels>=1048576 ? 32 : old_books<8 ? 8 : old_books;
    direction_old_counts(s,keys,old_books);
    unsigned books=direction_seed(s,old_books,limit);
    if (!books) return 0;
    direction_normalize(s->cond,books,s->packed_rgb);
    for (unsigned round=0;round<2;++round)
        books=direction_assign(s,3*old_books,books,0);
    direction_normalize(s->cond,books,s->packed_rgb);
    return direction_assign(s,3*keys,books,1);
}
#ifdef QLI_NUMERIC_SSE42
static const uint8_t wave_emit_shuffle[256][16]={
    {128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128},
    {128,128,128,128,128,128,128,128,128,128,128,128,128,128,0,1},
    {128,128,128,128,128,128,128,128,128,128,128,128,128,128,2,3},
    {128,128,128,128,128,128,128,128,128,128,128,128,0,1,2,3},
    {128,128,128,128,128,128,128,128,128,128,128,128,128,128,4,5},
    {128,128,128,128,128,128,128,128,128,128,128,128,0,1,4,5},
    {128,128,128,128,128,128,128,128,128,128,128,128,2,3,4,5},
    {128,128,128,128,128,128,128,128,128,128,0,1,2,3,4,5},
    {128,128,128,128,128,128,128,128,128,128,128,128,128,128,6,7},
    {128,128,128,128,128,128,128,128,128,128,128,128,0,1,6,7},
    {128,128,128,128,128,128,128,128,128,128,128,128,2,3,6,7},
    {128,128,128,128,128,128,128,128,128,128,0,1,2,3,6,7},
    {128,128,128,128,128,128,128,128,128,128,128,128,4,5,6,7},
    {128,128,128,128,128,128,128,128,128,128,0,1,4,5,6,7},
    {128,128,128,128,128,128,128,128,128,128,2,3,4,5,6,7},
    {128,128,128,128,128,128,128,128,0,1,2,3,4,5,6,7},
    {128,128,128,128,128,128,128,128,128,128,128,128,128,128,8,9},
    {128,128,128,128,128,128,128,128,128,128,128,128,0,1,8,9},
    {128,128,128,128,128,128,128,128,128,128,128,128,2,3,8,9},
    {128,128,128,128,128,128,128,128,128,128,0,1,2,3,8,9},
    {128,128,128,128,128,128,128,128,128,128,128,128,4,5,8,9},
    {128,128,128,128,128,128,128,128,128,128,0,1,4,5,8,9},
    {128,128,128,128,128,128,128,128,128,128,2,3,4,5,8,9},
    {128,128,128,128,128,128,128,128,0,1,2,3,4,5,8,9},
    {128,128,128,128,128,128,128,128,128,128,128,128,6,7,8,9},
    {128,128,128,128,128,128,128,128,128,128,0,1,6,7,8,9},
    {128,128,128,128,128,128,128,128,128,128,2,3,6,7,8,9},
    {128,128,128,128,128,128,128,128,0,1,2,3,6,7,8,9},
    {128,128,128,128,128,128,128,128,128,128,4,5,6,7,8,9},
    {128,128,128,128,128,128,128,128,0,1,4,5,6,7,8,9},
    {128,128,128,128,128,128,128,128,2,3,4,5,6,7,8,9},
    {128,128,128,128,128,128,0,1,2,3,4,5,6,7,8,9},
    {128,128,128,128,128,128,128,128,128,128,128,128,128,128,10,11},
    {128,128,128,128,128,128,128,128,128,128,128,128,0,1,10,11},
    {128,128,128,128,128,128,128,128,128,128,128,128,2,3,10,11},
    {128,128,128,128,128,128,128,128,128,128,0,1,2,3,10,11},
    {128,128,128,128,128,128,128,128,128,128,128,128,4,5,10,11},
    {128,128,128,128,128,128,128,128,128,128,0,1,4,5,10,11},
    {128,128,128,128,128,128,128,128,128,128,2,3,4,5,10,11},
    {128,128,128,128,128,128,128,128,0,1,2,3,4,5,10,11},
    {128,128,128,128,128,128,128,128,128,128,128,128,6,7,10,11},
    {128,128,128,128,128,128,128,128,128,128,0,1,6,7,10,11},
    {128,128,128,128,128,128,128,128,128,128,2,3,6,7,10,11},
    {128,128,128,128,128,128,128,128,0,1,2,3,6,7,10,11},
    {128,128,128,128,128,128,128,128,128,128,4,5,6,7,10,11},
    {128,128,128,128,128,128,128,128,0,1,4,5,6,7,10,11},
    {128,128,128,128,128,128,128,128,2,3,4,5,6,7,10,11},
    {128,128,128,128,128,128,0,1,2,3,4,5,6,7,10,11},
    {128,128,128,128,128,128,128,128,128,128,128,128,8,9,10,11},
    {128,128,128,128,128,128,128,128,128,128,0,1,8,9,10,11},
    {128,128,128,128,128,128,128,128,128,128,2,3,8,9,10,11},
    {128,128,128,128,128,128,128,128,0,1,2,3,8,9,10,11},
    {128,128,128,128,128,128,128,128,128,128,4,5,8,9,10,11},
    {128,128,128,128,128,128,128,128,0,1,4,5,8,9,10,11},
    {128,128,128,128,128,128,128,128,2,3,4,5,8,9,10,11},
    {128,128,128,128,128,128,0,1,2,3,4,5,8,9,10,11},
    {128,128,128,128,128,128,128,128,128,128,6,7,8,9,10,11},
    {128,128,128,128,128,128,128,128,0,1,6,7,8,9,10,11},
    {128,128,128,128,128,128,128,128,2,3,6,7,8,9,10,11},
    {128,128,128,128,128,128,0,1,2,3,6,7,8,9,10,11},
    {128,128,128,128,128,128,128,128,4,5,6,7,8,9,10,11},
    {128,128,128,128,128,128,0,1,4,5,6,7,8,9,10,11},
    {128,128,128,128,128,128,2,3,4,5,6,7,8,9,10,11},
    {128,128,128,128,0,1,2,3,4,5,6,7,8,9,10,11},
    {128,128,128,128,128,128,128,128,128,128,128,128,128,128,12,13},
    {128,128,128,128,128,128,128,128,128,128,128,128,0,1,12,13},
    {128,128,128,128,128,128,128,128,128,128,128,128,2,3,12,13},
    {128,128,128,128,128,128,128,128,128,128,0,1,2,3,12,13},
    {128,128,128,128,128,128,128,128,128,128,128,128,4,5,12,13},
    {128,128,128,128,128,128,128,128,128,128,0,1,4,5,12,13},
    {128,128,128,128,128,128,128,128,128,128,2,3,4,5,12,13},
    {128,128,128,128,128,128,128,128,0,1,2,3,4,5,12,13},
    {128,128,128,128,128,128,128,128,128,128,128,128,6,7,12,13},
    {128,128,128,128,128,128,128,128,128,128,0,1,6,7,12,13},
    {128,128,128,128,128,128,128,128,128,128,2,3,6,7,12,13},
    {128,128,128,128,128,128,128,128,0,1,2,3,6,7,12,13},
    {128,128,128,128,128,128,128,128,128,128,4,5,6,7,12,13},
    {128,128,128,128,128,128,128,128,0,1,4,5,6,7,12,13},
    {128,128,128,128,128,128,128,128,2,3,4,5,6,7,12,13},
    {128,128,128,128,128,128,0,1,2,3,4,5,6,7,12,13},
    {128,128,128,128,128,128,128,128,128,128,128,128,8,9,12,13},
    {128,128,128,128,128,128,128,128,128,128,0,1,8,9,12,13},
    {128,128,128,128,128,128,128,128,128,128,2,3,8,9,12,13},
    {128,128,128,128,128,128,128,128,0,1,2,3,8,9,12,13},
    {128,128,128,128,128,128,128,128,128,128,4,5,8,9,12,13},
    {128,128,128,128,128,128,128,128,0,1,4,5,8,9,12,13},
    {128,128,128,128,128,128,128,128,2,3,4,5,8,9,12,13},
    {128,128,128,128,128,128,0,1,2,3,4,5,8,9,12,13},
    {128,128,128,128,128,128,128,128,128,128,6,7,8,9,12,13},
    {128,128,128,128,128,128,128,128,0,1,6,7,8,9,12,13},
    {128,128,128,128,128,128,128,128,2,3,6,7,8,9,12,13},
    {128,128,128,128,128,128,0,1,2,3,6,7,8,9,12,13},
    {128,128,128,128,128,128,128,128,4,5,6,7,8,9,12,13},
    {128,128,128,128,128,128,0,1,4,5,6,7,8,9,12,13},
    {128,128,128,128,128,128,2,3,4,5,6,7,8,9,12,13},
    {128,128,128,128,0,1,2,3,4,5,6,7,8,9,12,13},
    {128,128,128,128,128,128,128,128,128,128,128,128,10,11,12,13},
    {128,128,128,128,128,128,128,128,128,128,0,1,10,11,12,13},
    {128,128,128,128,128,128,128,128,128,128,2,3,10,11,12,13},
    {128,128,128,128,128,128,128,128,0,1,2,3,10,11,12,13},
    {128,128,128,128,128,128,128,128,128,128,4,5,10,11,12,13},
    {128,128,128,128,128,128,128,128,0,1,4,5,10,11,12,13},
    {128,128,128,128,128,128,128,128,2,3,4,5,10,11,12,13},
    {128,128,128,128,128,128,0,1,2,3,4,5,10,11,12,13},
    {128,128,128,128,128,128,128,128,128,128,6,7,10,11,12,13},
    {128,128,128,128,128,128,128,128,0,1,6,7,10,11,12,13},
    {128,128,128,128,128,128,128,128,2,3,6,7,10,11,12,13},
    {128,128,128,128,128,128,0,1,2,3,6,7,10,11,12,13},
    {128,128,128,128,128,128,128,128,4,5,6,7,10,11,12,13},
    {128,128,128,128,128,128,0,1,4,5,6,7,10,11,12,13},
    {128,128,128,128,128,128,2,3,4,5,6,7,10,11,12,13},
    {128,128,128,128,0,1,2,3,4,5,6,7,10,11,12,13},
    {128,128,128,128,128,128,128,128,128,128,8,9,10,11,12,13},
    {128,128,128,128,128,128,128,128,0,1,8,9,10,11,12,13},
    {128,128,128,128,128,128,128,128,2,3,8,9,10,11,12,13},
    {128,128,128,128,128,128,0,1,2,3,8,9,10,11,12,13},
    {128,128,128,128,128,128,128,128,4,5,8,9,10,11,12,13},
    {128,128,128,128,128,128,0,1,4,5,8,9,10,11,12,13},
    {128,128,128,128,128,128,2,3,4,5,8,9,10,11,12,13},
    {128,128,128,128,0,1,2,3,4,5,8,9,10,11,12,13},
    {128,128,128,128,128,128,128,128,6,7,8,9,10,11,12,13},
    {128,128,128,128,128,128,0,1,6,7,8,9,10,11,12,13},
    {128,128,128,128,128,128,2,3,6,7,8,9,10,11,12,13},
    {128,128,128,128,0,1,2,3,6,7,8,9,10,11,12,13},
    {128,128,128,128,128,128,4,5,6,7,8,9,10,11,12,13},
    {128,128,128,128,0,1,4,5,6,7,8,9,10,11,12,13},
    {128,128,128,128,2,3,4,5,6,7,8,9,10,11,12,13},
    {128,128,0,1,2,3,4,5,6,7,8,9,10,11,12,13},
    {128,128,128,128,128,128,128,128,128,128,128,128,128,128,14,15},
    {128,128,128,128,128,128,128,128,128,128,128,128,0,1,14,15},
    {128,128,128,128,128,128,128,128,128,128,128,128,2,3,14,15},
    {128,128,128,128,128,128,128,128,128,128,0,1,2,3,14,15},
    {128,128,128,128,128,128,128,128,128,128,128,128,4,5,14,15},
    {128,128,128,128,128,128,128,128,128,128,0,1,4,5,14,15},
    {128,128,128,128,128,128,128,128,128,128,2,3,4,5,14,15},
    {128,128,128,128,128,128,128,128,0,1,2,3,4,5,14,15},
    {128,128,128,128,128,128,128,128,128,128,128,128,6,7,14,15},
    {128,128,128,128,128,128,128,128,128,128,0,1,6,7,14,15},
    {128,128,128,128,128,128,128,128,128,128,2,3,6,7,14,15},
    {128,128,128,128,128,128,128,128,0,1,2,3,6,7,14,15},
    {128,128,128,128,128,128,128,128,128,128,4,5,6,7,14,15},
    {128,128,128,128,128,128,128,128,0,1,4,5,6,7,14,15},
    {128,128,128,128,128,128,128,128,2,3,4,5,6,7,14,15},
    {128,128,128,128,128,128,0,1,2,3,4,5,6,7,14,15},
    {128,128,128,128,128,128,128,128,128,128,128,128,8,9,14,15},
    {128,128,128,128,128,128,128,128,128,128,0,1,8,9,14,15},
    {128,128,128,128,128,128,128,128,128,128,2,3,8,9,14,15},
    {128,128,128,128,128,128,128,128,0,1,2,3,8,9,14,15},
    {128,128,128,128,128,128,128,128,128,128,4,5,8,9,14,15},
    {128,128,128,128,128,128,128,128,0,1,4,5,8,9,14,15},
    {128,128,128,128,128,128,128,128,2,3,4,5,8,9,14,15},
    {128,128,128,128,128,128,0,1,2,3,4,5,8,9,14,15},
    {128,128,128,128,128,128,128,128,128,128,6,7,8,9,14,15},
    {128,128,128,128,128,128,128,128,0,1,6,7,8,9,14,15},
    {128,128,128,128,128,128,128,128,2,3,6,7,8,9,14,15},
    {128,128,128,128,128,128,0,1,2,3,6,7,8,9,14,15},
    {128,128,128,128,128,128,128,128,4,5,6,7,8,9,14,15},
    {128,128,128,128,128,128,0,1,4,5,6,7,8,9,14,15},
    {128,128,128,128,128,128,2,3,4,5,6,7,8,9,14,15},
    {128,128,128,128,0,1,2,3,4,5,6,7,8,9,14,15},
    {128,128,128,128,128,128,128,128,128,128,128,128,10,11,14,15},
    {128,128,128,128,128,128,128,128,128,128,0,1,10,11,14,15},
    {128,128,128,128,128,128,128,128,128,128,2,3,10,11,14,15},
    {128,128,128,128,128,128,128,128,0,1,2,3,10,11,14,15},
    {128,128,128,128,128,128,128,128,128,128,4,5,10,11,14,15},
    {128,128,128,128,128,128,128,128,0,1,4,5,10,11,14,15},
    {128,128,128,128,128,128,128,128,2,3,4,5,10,11,14,15},
    {128,128,128,128,128,128,0,1,2,3,4,5,10,11,14,15},
    {128,128,128,128,128,128,128,128,128,128,6,7,10,11,14,15},
    {128,128,128,128,128,128,128,128,0,1,6,7,10,11,14,15},
    {128,128,128,128,128,128,128,128,2,3,6,7,10,11,14,15},
    {128,128,128,128,128,128,0,1,2,3,6,7,10,11,14,15},
    {128,128,128,128,128,128,128,128,4,5,6,7,10,11,14,15},
    {128,128,128,128,128,128,0,1,4,5,6,7,10,11,14,15},
    {128,128,128,128,128,128,2,3,4,5,6,7,10,11,14,15},
    {128,128,128,128,0,1,2,3,4,5,6,7,10,11,14,15},
    {128,128,128,128,128,128,128,128,128,128,8,9,10,11,14,15},
    {128,128,128,128,128,128,128,128,0,1,8,9,10,11,14,15},
    {128,128,128,128,128,128,128,128,2,3,8,9,10,11,14,15},
    {128,128,128,128,128,128,0,1,2,3,8,9,10,11,14,15},
    {128,128,128,128,128,128,128,128,4,5,8,9,10,11,14,15},
    {128,128,128,128,128,128,0,1,4,5,8,9,10,11,14,15},
    {128,128,128,128,128,128,2,3,4,5,8,9,10,11,14,15},
    {128,128,128,128,0,1,2,3,4,5,8,9,10,11,14,15},
    {128,128,128,128,128,128,128,128,6,7,8,9,10,11,14,15},
    {128,128,128,128,128,128,0,1,6,7,8,9,10,11,14,15},
    {128,128,128,128,128,128,2,3,6,7,8,9,10,11,14,15},
    {128,128,128,128,0,1,2,3,6,7,8,9,10,11,14,15},
    {128,128,128,128,128,128,4,5,6,7,8,9,10,11,14,15},
    {128,128,128,128,0,1,4,5,6,7,8,9,10,11,14,15},
    {128,128,128,128,2,3,4,5,6,7,8,9,10,11,14,15},
    {128,128,0,1,2,3,4,5,6,7,8,9,10,11,14,15},
    {128,128,128,128,128,128,128,128,128,128,128,128,12,13,14,15},
    {128,128,128,128,128,128,128,128,128,128,0,1,12,13,14,15},
    {128,128,128,128,128,128,128,128,128,128,2,3,12,13,14,15},
    {128,128,128,128,128,128,128,128,0,1,2,3,12,13,14,15},
    {128,128,128,128,128,128,128,128,128,128,4,5,12,13,14,15},
    {128,128,128,128,128,128,128,128,0,1,4,5,12,13,14,15},
    {128,128,128,128,128,128,128,128,2,3,4,5,12,13,14,15},
    {128,128,128,128,128,128,0,1,2,3,4,5,12,13,14,15},
    {128,128,128,128,128,128,128,128,128,128,6,7,12,13,14,15},
    {128,128,128,128,128,128,128,128,0,1,6,7,12,13,14,15},
    {128,128,128,128,128,128,128,128,2,3,6,7,12,13,14,15},
    {128,128,128,128,128,128,0,1,2,3,6,7,12,13,14,15},
    {128,128,128,128,128,128,128,128,4,5,6,7,12,13,14,15},
    {128,128,128,128,128,128,0,1,4,5,6,7,12,13,14,15},
    {128,128,128,128,128,128,2,3,4,5,6,7,12,13,14,15},
    {128,128,128,128,0,1,2,3,4,5,6,7,12,13,14,15},
    {128,128,128,128,128,128,128,128,128,128,8,9,12,13,14,15},
    {128,128,128,128,128,128,128,128,0,1,8,9,12,13,14,15},
    {128,128,128,128,128,128,128,128,2,3,8,9,12,13,14,15},
    {128,128,128,128,128,128,0,1,2,3,8,9,12,13,14,15},
    {128,128,128,128,128,128,128,128,4,5,8,9,12,13,14,15},
    {128,128,128,128,128,128,0,1,4,5,8,9,12,13,14,15},
    {128,128,128,128,128,128,2,3,4,5,8,9,12,13,14,15},
    {128,128,128,128,0,1,2,3,4,5,8,9,12,13,14,15},
    {128,128,128,128,128,128,128,128,6,7,8,9,12,13,14,15},
    {128,128,128,128,128,128,0,1,6,7,8,9,12,13,14,15},
    {128,128,128,128,128,128,2,3,6,7,8,9,12,13,14,15},
    {128,128,128,128,0,1,2,3,6,7,8,9,12,13,14,15},
    {128,128,128,128,128,128,4,5,6,7,8,9,12,13,14,15},
    {128,128,128,128,0,1,4,5,6,7,8,9,12,13,14,15},
    {128,128,128,128,2,3,4,5,6,7,8,9,12,13,14,15},
    {128,128,0,1,2,3,4,5,6,7,8,9,12,13,14,15},
    {128,128,128,128,128,128,128,128,128,128,10,11,12,13,14,15},
    {128,128,128,128,128,128,128,128,0,1,10,11,12,13,14,15},
    {128,128,128,128,128,128,128,128,2,3,10,11,12,13,14,15},
    {128,128,128,128,128,128,0,1,2,3,10,11,12,13,14,15},
    {128,128,128,128,128,128,128,128,4,5,10,11,12,13,14,15},
    {128,128,128,128,128,128,0,1,4,5,10,11,12,13,14,15},
    {128,128,128,128,128,128,2,3,4,5,10,11,12,13,14,15},
    {128,128,128,128,0,1,2,3,4,5,10,11,12,13,14,15},
    {128,128,128,128,128,128,128,128,6,7,10,11,12,13,14,15},
    {128,128,128,128,128,128,0,1,6,7,10,11,12,13,14,15},
    {128,128,128,128,128,128,2,3,6,7,10,11,12,13,14,15},
    {128,128,128,128,0,1,2,3,6,7,10,11,12,13,14,15},
    {128,128,128,128,128,128,4,5,6,7,10,11,12,13,14,15},
    {128,128,128,128,0,1,4,5,6,7,10,11,12,13,14,15},
    {128,128,128,128,2,3,4,5,6,7,10,11,12,13,14,15},
    {128,128,0,1,2,3,4,5,6,7,10,11,12,13,14,15},
    {128,128,128,128,128,128,128,128,8,9,10,11,12,13,14,15},
    {128,128,128,128,128,128,0,1,8,9,10,11,12,13,14,15},
    {128,128,128,128,128,128,2,3,8,9,10,11,12,13,14,15},
    {128,128,128,128,0,1,2,3,8,9,10,11,12,13,14,15},
    {128,128,128,128,128,128,4,5,8,9,10,11,12,13,14,15},
    {128,128,128,128,0,1,4,5,8,9,10,11,12,13,14,15},
    {128,128,128,128,2,3,4,5,8,9,10,11,12,13,14,15},
    {128,128,0,1,2,3,4,5,8,9,10,11,12,13,14,15},
    {128,128,128,128,128,128,6,7,8,9,10,11,12,13,14,15},
    {128,128,128,128,0,1,6,7,8,9,10,11,12,13,14,15},
    {128,128,128,128,2,3,6,7,8,9,10,11,12,13,14,15},
    {128,128,0,1,2,3,6,7,8,9,10,11,12,13,14,15},
    {128,128,128,128,4,5,6,7,8,9,10,11,12,13,14,15},
    {128,128,0,1,4,5,6,7,8,9,10,11,12,13,14,15},
    {128,128,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
};
#endif

#ifdef QLI_NUMERIC_AVX2

static QLI_NUMERIC_TARGET __m128i wave_load(const uint8_t *data,unsigned off,unsigned n)
{
    if (n==16) return numeric_load16(data);
    uint8_t tail[16]={0};
    memcpy(tail+off,data,n);
    return numeric_load16(tail);
}

static QLI_NUMERIC_TARGET int wave_all_live(const NumericScratch *s,unsigned y,unsigned w)
{
    size_t start=(size_t)y*w/2048,end=((size_t)(y+16)*w-1)/2048;
    for (size_t i=start;i<=end;++i)
        if (s->flat[i]) return 0;
    return 1;
}

static QLI_NUMERIC_TARGET inline __m128i direction_stage(const NumericScratch *s,unsigned w,unsigned y,size_t pos,
                                      unsigned begin,unsigned off,unsigned len,__m128i error)
{
    const __m128i one=_mm_set1_epi8(1);
    if (!y) return one;
    const __m128i indices=_mm_setr_epi8(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
    const unsigned first=begin==0;
    __m128i prediction=_mm_sub_epi8(wave_load(s->plane+pos,off,len),error);
    __m128i west=wave_load(s->plane+pos+first-1,off+first,len-first);
    __m128i north=wave_load(s->plane+pos-w,off,len);
    __m128i northwest=wave_load(s->plane+pos-w+first-1,off+first,len-first);
    __m128i gradient=_mm_subs_epu8(_mm_adds_epu8(north,_mm_subs_epu8(west,northwest)),
                                   _mm_subs_epu8(northwest,west));
    __m128i has_gradient=y<2 ? _mm_setzero_si128() :
        _mm_cmpgt_epi8(indices,_mm_set1_epi8((char)((int)off+1-(int)begin)));
    if (begin>=2 && y>=2) has_gradient=_mm_set1_epi8(-1);
    __m128i use_gradient=_mm_andnot_si128(_mm_cmpeq_epi8(gradient,prediction),has_gradient);
    __m128i value=_mm_blendv_epi8(_mm_avg_epu8(west,north),gradient,use_gradient);
    if (first) {
        __m128i has_mid=_mm_cmpgt_epi8(indices,_mm_set1_epi8((char)off));
        value=_mm_blendv_epi8(prediction,value,has_mid);
    }
    const __m128i bias=_mm_set1_epi8((char)128);
    value=_mm_xor_si128(value,bias);
    prediction=_mm_xor_si128(prediction,bias);
    return _mm_add_epi8(one,_mm_sub_epi8(_mm_cmpgt_epi8(prediction,value),
                                         _mm_cmpgt_epi8(value,prediction)));
}

static QLI_NUMERIC_TARGET inline __m128i activity_guide(const uint8_t *table,__m128i key,unsigned guides)
{
    const __m128i a=_mm_shuffle_epi8(numeric_load16(table),key);
    if (guides==1) return a;
    const __m128i b=_mm_shuffle_epi8(numeric_load16((table+16)),key);
    const __m128i c=_mm_shuffle_epi8(numeric_load16((table+32)),key);
    const __m128i d=_mm_shuffle_epi8(numeric_load16((table+48)),key);
    __m128i mid=_mm_slli_epi16(key,3);
    return _mm_blendv_epi8(_mm_blendv_epi8(a,b,mid),_mm_blendv_epi8(c,d,mid),_mm_slli_epi16(key,2));
}

static QLI_NUMERIC_TARGET inline void activity_stage(const NumericScratch *s,unsigned w,unsigned y,unsigned rows,int step,
                                  const uint8_t *route,__m128i values[16],__m128i groups[16],__m128i directions[16],int full)
{
    const __m128i indices=_mm_setr_epi8(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
    const __m128i absent=_mm_set1_epi8((char)255);
    for (unsigned k=0;k<16;++k) {
        int x=step-4*(int)k;
        int begin=x<0 ? 0 : x, end=x+16<(int)w ? x+16 : (int)w;
        if (!full && (k>=rows || end<=begin)) {
            values[k]=_mm_setzero_si128();
            groups[k]=absent;
            if (s->logical) directions[k]=_mm_setzero_si128();
            continue;
        }
        unsigned off=(unsigned)(begin-x), len=(unsigned)(end-begin);
        size_t pos=(size_t)(y+k)*w+(unsigned)begin;
        values[k]=wave_load(s->error+pos,off,len);
        if (s->logical) directions[k]=direction_stage(s,w,y+k,pos,(unsigned)begin,off,len,values[k]);
        __m128i id=_mm_setzero_si128();
        if (s->guides) {
            __m128i key=wave_load(s->cue[0]+pos,off,len);
            if (s->guides==2) {
                __m128i other=wave_load(s->cue[1]+pos,off,len);
                key=_mm_add_epi8(_mm_sub_epi8(_mm_slli_epi16(key,3),key),other);
            }
            id=activity_guide(route,key,s->guides);
        }
        if (!full) {
            __m128i active=_mm_setzero_si128();
            unsigned at=off;
            while (at<off+len) {
                size_t p=pos+at-off;
                unsigned n=256-(unsigned)p%256;
                if (n>off+len-at) n=off+len-at;
                if (!flat_at(s,p)) {
                    __m128i after=_mm_cmpgt_epi8(indices,_mm_set1_epi8((char)((int)at-1)));
                    __m128i before=_mm_cmpgt_epi8(_mm_set1_epi8((char)(at+n)),indices);
                    active=_mm_or_si128(active,_mm_and_si128(after,before));
                }
                at+=n;
            }
            id=_mm_blendv_epi8(absent,id,active);
        }
        groups[k]=id;
    }
    diagonal_transpose(values);
    diagonal_transpose(groups);
    if (s->logical) diagonal_transpose(directions);
}

static QLI_NUMERIC_TARGET inline void activity_keys(uint32_t *out,__m128i e,__m128i g,__m128i west,__m128i direction,int logical,int full)
{
    const __m256i one=_mm256_set1_epi32(1);
    __m256i id=_mm256_cvtepu8_epi32(g), a=_mm256_cvtepu16_epi32(west);
    a=_mm256_add_epi32(_mm256_mullo_epi32(a,a),one);
    a=_mm256_srli_epi32(_mm256_castps_si256(_mm256_cvtepi32_ps(a)),23);
    a=_mm256_min_epi32(_mm256_sub_epi32(a,_mm256_set1_epi32(127)),_mm256_set1_epi32(17));
    a=_mm256_add_epi32(a,_mm256_mullo_epi32(id,_mm256_set1_epi32(WAVE_CONTEXTS)));
    if (logical) a=_mm256_add_epi32(_mm256_mullo_epi32(a,_mm256_set1_epi32(3)),
                                  _mm256_cvtepu8_epi32(direction));
    a=_mm256_or_si256(_mm256_slli_epi32(a,8),_mm256_cvtepu8_epi32(e));
    if (!full) a=_mm256_or_si256(a,_mm256_cmpeq_epi32(id,_mm256_set1_epi32(255)));
    numeric_store32(out,a);
}

typedef __m256i NumericActivityHistory;

static QLI_NUMERIC_TARGET inline size_t activity_block(NumericScratch *s,unsigned w,unsigned y,unsigned rows,unsigned step,
                                    unsigned steps,const uint8_t *route,uint32_t *out,const uint64_t weights[16],
                                    NumericActivityHistory *last,NumericActivityHistory *before,int full)
{
    __m128i values[16],groups[16],directions[16];
    activity_stage(s,w,y,rows,(int)step,route,values,groups,directions,full);
    NumericActivityHistory recent=*last, older=*before;
    const __m256i offsets=_mm256_setr_epi16(0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60);
    const __m256i band=_mm256_cmpgt_epi16(_mm256_set1_epi16((short)(4*rows)),offsets);
    const __m256i limit=_mm256_set1_epi16((short)w), minus_one=_mm256_set1_epi16(-1);
    uint16_t *north=s->wave->north;
    uint64_t *hist=s->logical ? s->dir->emit : s->wave->hist;
    size_t count=0;
    for (unsigned j=0;j<steps;++j,out+=16) {
        unsigned t=step+j;
        __m128i e=values[j], g=groups[j];
        __m128i d=s->logical ? directions[j] : _mm_setzero_si128();
        __m128i lo=_mm256_castsi256_si128(recent), hi=_mm256_extracti128_si256(recent,1);
        activity_keys(out,e,g,lo,d,s->logical,full);
        activity_keys(out+8,_mm_srli_si128(e,8),_mm_srli_si128(g,8),hi,_mm_srli_si128(d,8),s->logical,full);
        if (full) {
            for (unsigned k=0;k<16;++k) hist[out[k]]+=weights[k];
        } else {
            for (unsigned k=0;k<16;++k) if (out[k]!=UINT32_MAX) {
                hist[out[k]]+=weights[k];
                ++count;
            }
        }
        unsigned top=0;
        if (full || t<w) top=north[t+2]>north[t+3] ? north[t+2] : north[t+3];
        __m256i neighbors=_mm256_max_epu16(recent,older);
        __m256i cross=_mm256_permute2x128_si256(neighbors,neighbors,0x08);
        __m256i diagonal=_mm256_alignr_epi8(neighbors,cross,14);
        diagonal=_mm256_insert_epi16(diagonal,(int)top,0);
        __m256i error=_mm256_cvtepi8_epi16(e);
        __m256i folded=_mm256_xor_si256(_mm256_slli_epi16(error,1),_mm256_srai_epi16(error,15));
        older=recent;
        recent=_mm256_avg_epu16(recent,_mm256_avg_epu16(_mm256_slli_epi16(folded,3),diagonal));
        if (!full) {
            __m256i x=_mm256_sub_epi16(_mm256_set1_epi16((short)t),offsets);
            __m256i valid=_mm256_and_si256(band,_mm256_and_si256(
                _mm256_cmpgt_epi16(x,minus_one),_mm256_cmpgt_epi16(limit,x)));
            recent=_mm256_and_si256(recent,valid);
        }
        if (full || (rows==16 && t-60<w)) north[t-60]=(uint16_t)_mm256_extract_epi16(recent,15);
    }
    *last=recent;
    *before=older;
    return full ? 16*steps : count;
}

static QLI_NUMERIC_TARGET size_t wave_activity_simd(NumericScratch *s,unsigned w,unsigned h,const uint8_t route[16][64])
{
    memset(s->wave->north,0,2*(size_t)(w+3));
    uint32_t *out=s->words;
    size_t count=0;
    for (unsigned y=0;y<h;y+=16) {
        unsigned rows=h-y<16 ? h-y : 16;
        uint64_t weights[16];
        for (unsigned k=0;k<16;++k) weights[k]=wave_count(k,(y/16)&7);
        int interior=rows==16 && wave_all_live(s,y,w);
        NumericActivityHistory recent={0}, older={0};
        for (unsigned t=0;t<w+60;t+=16) {
            unsigned steps=w+60-t<16 ? w+60-t : 16;
            if (interior && t>=60 && t+16<=w)
                count+=activity_block(s,w,y,16,t,16,route[0],out,weights,&recent,&older,1);
            else
                count+=activity_block(s,w,y,rows,t,steps,route[0],out,weights,&recent,&older,0);
            out+=16*steps;
        }
    }
    return count;
}

static QLI_NUMERIC_TARGET inline __m128i emit_gather_words(__m256i values,int high)
{
    __m128 low = _mm_castsi128_ps(_mm256_castsi256_si128(values));
    __m128 upper = _mm_castsi128_ps(_mm256_extracti128_si256(values,1));
    return _mm_castps_si128(high ? _mm_shuffle_ps(low,upper,_MM_SHUFFLE(3,1,3,1))
                                : _mm_shuffle_ps(low,upper,_MM_SHUFFLE(2,0,2,0)));
}

static QLI_NUMERIC_TARGET inline __attribute__((always_inline)) __m256i emit_finish8(
    __m256i state,__m256i entry,__m256i reciprocal,__m256i freq)
{
    const __m256i one=_mm256_set1_epi32(1), scale=_mm256_set1_epi32(4096);
    const __m256i high=_mm256_set1_epi64x((int64_t)UINT64_C(0xffffffff00000000));
    __m256i even=_mm256_mul_epu32(state,reciprocal);
    __m256i odd=_mm256_mul_epu32(_mm256_srli_epi64(state,32),_mm256_srli_epi64(reciprocal,32));
    __m256i q=_mm256_or_si256(_mm256_srli_epi64(even,32),_mm256_and_si256(odd,high));
    __m256i r=_mm256_sub_epi32(state,_mm256_mullo_epi32(q,freq));
    __m256i adjust=_mm256_cmpgt_epi32(r,_mm256_sub_epi32(freq,one));
    q=_mm256_sub_epi32(q,adjust);
    return _mm256_add_epi32(_mm256_add_epi32(state,_mm256_mullo_epi32(q,_mm256_sub_epi32(scale,freq))),
                            _mm256_and_si256(entry,_mm256_set1_epi32(4095)));
}

static QLI_NUMERIC_TARGET inline __attribute__((always_inline)) __m256i emit_update8(const uint64_t *table,__m256i key,__m256i state,uint8_t **tail,int partial)
{
    const __m256i scale=_mm256_set1_epi32(4096);
    const __m256i sign=_mm256_set1_epi32(INT32_MIN);
    const __m128i low=_mm256_castsi256_si128(key),hi=_mm256_extracti128_si256(key,1);
    __m256i a,b;
    if (partial) {
        const __m128i minus_one=_mm_set1_epi32(-1);
        const __m256i identity=_mm256_set1_epi64x(4096u<<12);
        __m256i active_a=_mm256_cvtepi32_epi64(_mm_cmpgt_epi32(low,minus_one));
        __m256i active_b=_mm256_cvtepi32_epi64(_mm_cmpgt_epi32(hi,minus_one));
        a=_mm256_mask_i32gather_epi64(identity,(const long long *)table,low,active_a,8);
        b=_mm256_mask_i32gather_epi64(identity,(const long long *)table,hi,active_b,8);
    } else {
        a=_mm256_i32gather_epi64((const long long *)table,low,8);
        b=_mm256_i32gather_epi64((const long long *)table,hi,8);
    }
    __m256i entry=_mm256_set_m128i(emit_gather_words(b,0),emit_gather_words(a,0));
    __m256i reciprocal=_mm256_set_m128i(emit_gather_words(b,1),emit_gather_words(a,1));
    __m256i freq=_mm256_srli_epi32(entry,12);
    __m256i threshold=_mm256_slli_epi32(freq,20);
    __m256i below=_mm256_cmpgt_epi32(_mm256_xor_si256(threshold,sign),_mm256_xor_si256(state,sign));
    __m256i refill=_mm256_andnot_si256(_mm256_or_si256(below,_mm256_cmpeq_epi32(freq,scale)),
                                      _mm256_set1_epi32(-1));
    unsigned mask=(unsigned)_mm256_movemask_ps(_mm256_castsi256_ps(refill));
    if (mask) {
        __m256i low=_mm256_and_si256(state,_mm256_set1_epi32(65535));
        __m128i words=_mm_packus_epi32(_mm256_castsi256_si128(low),_mm256_extracti128_si256(low,1));
        __m128i shuffle=numeric_load16(wave_emit_shuffle[mask]);
        numeric_store16((*tail-16),_mm_shuffle_epi8(words,shuffle));
        *tail-=2u*qlic_count_bits(mask);
        state=_mm256_blendv_epi8(state,_mm256_srli_epi32(state,16),refill);
    }
    return emit_finish8(state,entry,reciprocal,freq);
}

static QLI_NUMERIC_TARGET inline __m256i emit_step8(const uint64_t *table,const uint32_t *keys,__m256i key,__m256i state,uint8_t **tail)
{
    unsigned absent=(unsigned)_mm256_movemask_ps(_mm256_castsi256_ps(key));
    if (!absent) return emit_update8(table,key,state,tail,0);
    if (absent==255) return state;
    if (qlic_count_bits(absent)<=5) return emit_update8(table,key,state,tail,1);
    uint32_t x[8];
    numeric_store32(x,state);
    for (unsigned k=8;k--;) {
        if (absent&(1u<<k)) continue;
        uint32_t entry=(uint32_t)table[keys[k]], f=entry>>12, a=x[k];
        if (f<4096 && a>=f<<20) {
            *tail-=2;
            qli_write_le16(*tail,(uint16_t)a);
            a>>=16;
        }
        x[k]=(a/f<<12)+a%f+(entry&4095u);
    }
    return numeric_load32(x);
}

typedef struct {
    __m256i entry,reciprocal,frequency,refill;
} EmitHalf;

static QLI_NUMERIC_TARGET inline __attribute__((always_inline)) EmitHalf emit_prepare_full(const uint64_t *table,
                                                                        __m256i key,__m256i state)
{
    __m256i a=_mm256_i32gather_epi64((const long long *)table,_mm256_castsi256_si128(key),8);
    __m256i b=_mm256_i32gather_epi64((const long long *)table,_mm256_extracti128_si256(key,1),8);
    __m256i entry=_mm256_set_m128i(emit_gather_words(b,0),emit_gather_words(a,0));
    __m256i reciprocal=_mm256_set_m128i(emit_gather_words(b,1),emit_gather_words(a,1));
    __m256i frequency=_mm256_srli_epi32(entry,12);
    __m256i refill=_mm256_cmpgt_epi32(_mm256_add_epi32(_mm256_srli_epi32(state,20),
                                                       _mm256_set1_epi32(1)),frequency);
    return (EmitHalf){entry,reciprocal,frequency,refill};
}

static QLI_NUMERIC_TARGET inline __attribute__((always_inline)) __m256i emit_finish_full(EmitHalf p,__m256i state)
{
    state=_mm256_blendv_epi8(state,_mm256_srli_epi32(state,16),p.refill);
    return emit_finish8(state,p.entry,p.reciprocal,p.frequency);
}

static QLI_NUMERIC_TARGET inline __m128i emit_pair_words(__m256i state,unsigned mask)
{
    __m256i low=_mm256_and_si256(state,_mm256_set1_epi32(65535));
    __m128i words=_mm_packus_epi32(_mm256_castsi256_si128(low),_mm256_extracti128_si256(low,1));
    return _mm_shuffle_epi8(words,numeric_load16(wave_emit_shuffle[mask]));
}

static QLI_NUMERIC_TARGET inline __attribute__((always_inline)) uint8_t *emit_pair_full(
    const uint64_t *table,__m256i key_l,__m256i key_h,__m256i *lower,__m256i *upper,uint8_t *tail)
{
    __m256i a=*lower,b=*upper;
    EmitHalf p=emit_prepare_full(table,key_l,a),q=emit_prepare_full(table,key_h,b);
    unsigned low=(unsigned)_mm256_movemask_ps(_mm256_castsi256_ps(p.refill));
    unsigned high=(unsigned)_mm256_movemask_ps(_mm256_castsi256_ps(q.refill));
    *lower=emit_finish_full(p,a);
    *upper=emit_finish_full(q,b);
    if (low|high) {
        uint8_t *middle=tail-2u*qlic_count_bits(high);
        numeric_store16((tail-16),emit_pair_words(b,high));
        numeric_store16((middle-16),emit_pair_words(a,low));
        tail=middle-2u*qlic_count_bits(low);
    }
    return tail;
}

static QLI_NUMERIC_TARGET uint8_t *numeric_emit_words(const uint64_t *table,const uint32_t *keys,size_t slots,
                                   uint32_t ans[16],uint8_t *tail)
{
    __m256i lower=numeric_load32(ans);
    __m256i upper=numeric_load32(ans+8);
    while (slots) {
        slots-=16;
        const uint32_t *group=keys+slots;
        __m256i lo=numeric_load32(group);
        __m256i hi=numeric_load32(group+8);
        if (!_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_or_si256(lo,hi)))) {
            tail=emit_pair_full(table,lo,hi,&lower,&upper,tail);
        } else {
            upper=emit_step8(table,group+8,hi,upper,&tail);
            lower=emit_step8(table,group,lo,lower,&tail);
        }
    }
    numeric_store32(ans,lower);
    numeric_store32(ans+8,upper);
    return tail;
}
#endif
#ifdef QLI_NUMERIC_SSE42

static QLI_SSE42_TARGET __m128i sse42_wave_load(const uint8_t *data,unsigned off,unsigned n)
{
    if (n==16) return sse42_numeric_load16(data);
    uint8_t tail[16]={0};
    memcpy(tail+off,data,n);
    return sse42_numeric_load16(tail);
}

static QLI_SSE42_TARGET int sse42_wave_all_live(const NumericScratch *s,unsigned y,unsigned w)
{
    size_t start=(size_t)y*w/2048,end=((size_t)(y+16)*w-1)/2048;
    for (size_t i=start;i<=end;++i)
        if (s->flat[i]) return 0;
    return 1;
}

static QLI_SSE42_TARGET inline __m128i sse42_direction_stage(const NumericScratch *s,unsigned w,unsigned y,size_t pos,
                                      unsigned begin,unsigned off,unsigned len,__m128i error)
{
    const __m128i one=_mm_set1_epi8(1);
    if (!y) return one;
    const __m128i indices=_mm_setr_epi8(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
    const unsigned first=begin==0;
    __m128i prediction=_mm_sub_epi8(sse42_wave_load(s->plane+pos,off,len),error);
    __m128i west=sse42_wave_load(s->plane+pos+first-1,off+first,len-first);
    __m128i north=sse42_wave_load(s->plane+pos-w,off,len);
    __m128i northwest=sse42_wave_load(s->plane+pos-w+first-1,off+first,len-first);
    __m128i gradient=_mm_subs_epu8(_mm_adds_epu8(north,_mm_subs_epu8(west,northwest)),
                                   _mm_subs_epu8(northwest,west));
    __m128i has_gradient=y<2 ? _mm_setzero_si128() :
        _mm_cmpgt_epi8(indices,_mm_set1_epi8((char)((int)off+1-(int)begin)));
    if (begin>=2 && y>=2) has_gradient=_mm_set1_epi8(-1);
    __m128i use_gradient=_mm_andnot_si128(_mm_cmpeq_epi8(gradient,prediction),has_gradient);
    __m128i value=_mm_blendv_epi8(_mm_avg_epu8(west,north),gradient,use_gradient);
    if (first) {
        __m128i has_mid=_mm_cmpgt_epi8(indices,_mm_set1_epi8((char)off));
        value=_mm_blendv_epi8(prediction,value,has_mid);
    }
    const __m128i bias=_mm_set1_epi8((char)128);
    value=_mm_xor_si128(value,bias);
    prediction=_mm_xor_si128(prediction,bias);
    return _mm_add_epi8(one,_mm_sub_epi8(_mm_cmpgt_epi8(prediction,value),
                                         _mm_cmpgt_epi8(value,prediction)));
}

static QLI_SSE42_TARGET inline __m128i sse42_activity_guide(const uint8_t *table,__m128i key,unsigned guides)
{
    const __m128i a=_mm_shuffle_epi8(sse42_numeric_load16(table),key);
    if (guides==1) return a;
    const __m128i b=_mm_shuffle_epi8(sse42_numeric_load16((table+16)),key);
    const __m128i c=_mm_shuffle_epi8(sse42_numeric_load16((table+32)),key);
    const __m128i d=_mm_shuffle_epi8(sse42_numeric_load16((table+48)),key);
    __m128i mid=_mm_slli_epi16(key,3);
    return _mm_blendv_epi8(_mm_blendv_epi8(a,b,mid),_mm_blendv_epi8(c,d,mid),_mm_slli_epi16(key,2));
}

static QLI_SSE42_TARGET inline void sse42_activity_stage(const NumericScratch *s,unsigned w,unsigned y,unsigned rows,int step,
                                  const uint8_t *route,__m128i values[16],__m128i groups[16],__m128i directions[16],int full)
{
    const __m128i indices=_mm_setr_epi8(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
    const __m128i absent=_mm_set1_epi8((char)255);
    for (unsigned k=0;k<16;++k) {
        int x=step-4*(int)k;
        int begin=x<0 ? 0 : x, end=x+16<(int)w ? x+16 : (int)w;
        if (!full && (k>=rows || end<=begin)) {
            values[k]=_mm_setzero_si128();
            groups[k]=absent;
            if (s->logical) directions[k]=_mm_setzero_si128();
            continue;
        }
        unsigned off=(unsigned)(begin-x), len=(unsigned)(end-begin);
        size_t pos=(size_t)(y+k)*w+(unsigned)begin;
        values[k]=sse42_wave_load(s->error+pos,off,len);
        if (s->logical) directions[k]=sse42_direction_stage(s,w,y+k,pos,(unsigned)begin,off,len,values[k]);
        __m128i id=_mm_setzero_si128();
        if (s->guides) {
            __m128i key=sse42_wave_load(s->cue[0]+pos,off,len);
            if (s->guides==2) {
                __m128i other=sse42_wave_load(s->cue[1]+pos,off,len);
                key=_mm_add_epi8(_mm_sub_epi8(_mm_slli_epi16(key,3),key),other);
            }
            id=sse42_activity_guide(route,key,s->guides);
        }
        if (!full) {
            __m128i active=_mm_setzero_si128();
            unsigned at=off;
            while (at<off+len) {
                size_t p=pos+at-off;
                unsigned n=256-(unsigned)p%256;
                if (n>off+len-at) n=off+len-at;
                if (!flat_at(s,p)) {
                    __m128i after=_mm_cmpgt_epi8(indices,_mm_set1_epi8((char)((int)at-1)));
                    __m128i before=_mm_cmpgt_epi8(_mm_set1_epi8((char)(at+n)),indices);
                    active=_mm_or_si128(active,_mm_and_si128(after,before));
                }
                at+=n;
            }
            id=_mm_blendv_epi8(absent,id,active);
        }
        groups[k]=id;
    }
    sse42_diagonal_transpose(values);
    sse42_diagonal_transpose(groups);
    if (s->logical) sse42_diagonal_transpose(directions);
}

static QLI_SSE42_TARGET inline void sse42_activity_keys(uint32_t *out,__m128i e,__m128i g,__m128i west,__m128i direction,int logical,int full)
{
    const SsePair one=sse_set1_epi32(1);
    SsePair id=sse_cvtepu8_epi32(g), a=sse_cvtepu16_epi32(west);
    a=sse_add_epi32(sse_mullo_epi32(a,a),one);
    a=sse_srli_epi32(sse_castps_si256(sse_cvtepi32_ps(a)),23);
    a=sse_min_epi32(sse_sub_epi32(a,sse_set1_epi32(127)),sse_set1_epi32(17));
    a=sse_add_epi32(a,sse_mullo_epi32(id,sse_set1_epi32(WAVE_CONTEXTS)));
    if (logical) a=sse_add_epi32(sse_mullo_epi32(a,sse_set1_epi32(3)),
                                  sse_cvtepu8_epi32(direction));
    a=sse_or_si128(sse_slli_epi32(a,8),sse_cvtepu8_epi32(e));
    if (!full) a=sse_or_si128(a,sse_cmpeq_epi32(id,sse_set1_epi32(255)));
    sse42_numeric_store32(out,a);
}

typedef SsePair sse42_NumericActivityHistory;

static QLI_SSE42_TARGET inline size_t sse42_activity_block(NumericScratch *s,unsigned w,unsigned y,unsigned rows,unsigned step,
                                    unsigned steps,const uint8_t *route,uint32_t *out,const uint64_t weights[16],
                                    sse42_NumericActivityHistory *last,sse42_NumericActivityHistory *before,int full)
{
    __m128i values[16],groups[16],directions[16];
    sse42_activity_stage(s,w,y,rows,(int)step,route,values,groups,directions,full);
    sse42_NumericActivityHistory recent=*last, older=*before;
    const SsePair offsets=sse_setr_epi16(0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60);
    const SsePair band=sse_cmpgt_epi16(sse_set1_epi16((short)(4*rows)),offsets);
    const SsePair limit=sse_set1_epi16((short)w), minus_one=sse_set1_epi16(-1);
    uint16_t *north=s->wave->north;
    uint64_t *hist=s->logical ? s->dir->emit : s->wave->hist;
    size_t count=0;
    for (unsigned j=0;j<steps;++j,out+=16) {
        unsigned t=step+j;
        __m128i e=values[j], g=groups[j];
        __m128i d=s->logical ? directions[j] : _mm_setzero_si128();
        __m128i lo=sse_castsi256_si128(recent), hi=sse_extracti128_si256(recent,1);
        sse42_activity_keys(out,e,g,lo,d,s->logical,full);
        sse42_activity_keys(out+8,_mm_srli_si128(e,8),_mm_srli_si128(g,8),hi,_mm_srli_si128(d,8),s->logical,full);
        if (full) {
            for (unsigned k=0;k<16;++k) hist[out[k]]+=weights[k];
        } else {
            for (unsigned k=0;k<16;++k) if (out[k]!=UINT32_MAX) {
                hist[out[k]]+=weights[k];
                ++count;
            }
        }
        unsigned top=0;
        if (full || t<w) top=north[t+2]>north[t+3] ? north[t+2] : north[t+3];
        SsePair neighbors=sse_max_epu16(recent,older);
        SsePair cross=sse_permute2x128_si256(neighbors,neighbors,0x08);
        SsePair diagonal=sse_alignr_epi8(neighbors,cross,14);
        diagonal=sse_insert_epi16(diagonal,(int)top,0);
        SsePair error=sse_cvtepi8_epi16(e);
        SsePair folded=sse_xor_si128(sse_slli_epi16(error,1),sse_srai_epi16(error,15));
        older=recent;
        recent=sse_avg_epu16(recent,sse_avg_epu16(sse_slli_epi16(folded,3),diagonal));
        if (!full) {
            SsePair x=sse_sub_epi16(sse_set1_epi16((short)t),offsets);
            SsePair valid=sse_and_si128(band,sse_and_si128(
                sse_cmpgt_epi16(x,minus_one),sse_cmpgt_epi16(limit,x)));
            recent=sse_and_si128(recent,valid);
        }
        if (full || (rows==16 && t-60<w)) north[t-60]=(uint16_t)sse_extract_epi16(recent,15);
    }
    *last=recent;
    *before=older;
    return full ? 16*steps : count;
}

static QLI_SSE42_TARGET size_t sse42_wave_activity_simd(NumericScratch *s,unsigned w,unsigned h,const uint8_t route[16][64])
{
    memset(s->wave->north,0,2*(size_t)(w+3));
    uint32_t *out=s->words;
    size_t count=0;
    for (unsigned y=0;y<h;y+=16) {
        unsigned rows=h-y<16 ? h-y : 16;
        uint64_t weights[16];
        for (unsigned k=0;k<16;++k) weights[k]=wave_count(k,(y/16)&7);
        int interior=rows==16 && sse42_wave_all_live(s,y,w);
        sse42_NumericActivityHistory recent={0}, older={0};
        for (unsigned t=0;t<w+60;t+=16) {
            unsigned steps=w+60-t<16 ? w+60-t : 16;
            if (interior && t>=60 && t+16<=w)
                count+=sse42_activity_block(s,w,y,16,t,16,route[0],out,weights,&recent,&older,1);
            else
                count+=sse42_activity_block(s,w,y,rows,t,steps,route[0],out,weights,&recent,&older,0);
            out+=16*steps;
        }
    }
    return count;
}

static QLI_SSE42_TARGET inline __m128i sse42_emit_gather_words(SsePair values,int high)
{
    __m128 low = _mm_castsi128_ps(sse_castsi256_si128(values));
    __m128 upper = _mm_castsi128_ps(sse_extracti128_si256(values,1));
    return _mm_castps_si128(high ? _mm_shuffle_ps(low,upper,_MM_SHUFFLE(3,1,3,1))
                                : _mm_shuffle_ps(low,upper,_MM_SHUFFLE(2,0,2,0)));
}

static QLI_SSE42_TARGET inline __attribute__((always_inline)) SsePair sse42_emit_finish8(
    SsePair state,SsePair entry,SsePair reciprocal,SsePair freq)
{
    const SsePair one=sse_set1_epi32(1), scale=sse_set1_epi32(4096);
    const SsePair high=sse_set1_epi64x((int64_t)UINT64_C(0xffffffff00000000));
    SsePair even=sse_mul_epu32(state,reciprocal);
    SsePair odd=sse_mul_epu32(sse_srli_epi64(state,32),sse_srli_epi64(reciprocal,32));
    SsePair q=sse_or_si128(sse_srli_epi64(even,32),sse_and_si128(odd,high));
    SsePair r=sse_sub_epi32(state,sse_mullo_epi32(q,freq));
    SsePair adjust=sse_cmpgt_epi32(r,sse_sub_epi32(freq,one));
    q=sse_sub_epi32(q,adjust);
    return sse_add_epi32(sse_add_epi32(state,sse_mullo_epi32(q,sse_sub_epi32(scale,freq))),
                            sse_and_si128(entry,sse_set1_epi32(4095)));
}

static QLI_SSE42_TARGET inline __attribute__((always_inline)) SsePair sse42_emit_update8(const uint64_t *table,SsePair key,SsePair state,uint8_t **tail,int partial)
{
    const SsePair scale=sse_set1_epi32(4096);
    const SsePair sign=sse_set1_epi32(INT32_MIN);
    const __m128i low=sse_castsi256_si128(key),hi=sse_extracti128_si256(key,1);
    SsePair a,b;
    if (partial) {
        const __m128i minus_one=_mm_set1_epi32(-1);
        const SsePair identity=sse_set1_epi64x(4096u<<12);
        SsePair active_a=sse_cvtepi32_epi64(_mm_cmpgt_epi32(low,minus_one));
        SsePair active_b=sse_cvtepi32_epi64(_mm_cmpgt_epi32(hi,minus_one));
        a=sse_mask_i32gather_epi64(identity,(const long long *)table,low,active_a,8);
        b=sse_mask_i32gather_epi64(identity,(const long long *)table,hi,active_b,8);
    } else {
        a=sse_i32gather_epi64((const long long *)table,low,8);
        b=sse_i32gather_epi64((const long long *)table,hi,8);
    }
    SsePair entry=sse_set_m128i(sse42_emit_gather_words(b,0),sse42_emit_gather_words(a,0));
    SsePair reciprocal=sse_set_m128i(sse42_emit_gather_words(b,1),sse42_emit_gather_words(a,1));
    SsePair freq=sse_srli_epi32(entry,12);
    SsePair threshold=sse_slli_epi32(freq,20);
    SsePair below=sse_cmpgt_epi32(sse_xor_si128(threshold,sign),sse_xor_si128(state,sign));
    SsePair refill=sse_andnot_si128(sse_or_si128(below,sse_cmpeq_epi32(freq,scale)),
                                      sse_set1_epi32(-1));
    unsigned mask=(unsigned)sse_movemask_ps(sse_castsi256_ps(refill));
    if (mask) {
        SsePair low=sse_and_si128(state,sse_set1_epi32(65535));
        __m128i words=_mm_packus_epi32(sse_castsi256_si128(low),sse_extracti128_si256(low,1));
        __m128i shuffle=sse42_numeric_load16(wave_emit_shuffle[mask]);
        sse42_numeric_store16((*tail-16),_mm_shuffle_epi8(words,shuffle));
        *tail-=2u*qlic_count_bits(mask);
        state=sse_blendv_epi8(state,sse_srli_epi32(state,16),refill);
    }
    return sse42_emit_finish8(state,entry,reciprocal,freq);
}

static QLI_SSE42_TARGET inline SsePair sse42_emit_step8(const uint64_t *table,const uint32_t *keys,SsePair key,SsePair state,uint8_t **tail)
{
    unsigned absent=(unsigned)sse_movemask_ps(sse_castsi256_ps(key));
    if (!absent) return sse42_emit_update8(table,key,state,tail,0);
    if (absent==255) return state;
    if (qlic_count_bits(absent)<=5) return sse42_emit_update8(table,key,state,tail,1);
    uint32_t x[8];
    sse42_numeric_store32(x,state);
    for (unsigned k=8;k--;) {
        if (absent&(1u<<k)) continue;
        uint32_t entry=(uint32_t)table[keys[k]], f=entry>>12, a=x[k];
        if (f<4096 && a>=f<<20) {
            *tail-=2;
            qli_write_le16(*tail,(uint16_t)a);
            a>>=16;
        }
        x[k]=(a/f<<12)+a%f+(entry&4095u);
    }
    return sse42_numeric_load32(x);
}

typedef struct {
    SsePair entry,reciprocal,frequency,refill;
} sse42_EmitHalf;

static QLI_SSE42_TARGET inline __attribute__((always_inline)) sse42_EmitHalf sse42_emit_prepare_full(const uint64_t *table,
                                                                        SsePair key,SsePair state)
{
    SsePair a=sse_i32gather_epi64((const long long *)table,sse_castsi256_si128(key),8);
    SsePair b=sse_i32gather_epi64((const long long *)table,sse_extracti128_si256(key,1),8);
    SsePair entry=sse_set_m128i(sse42_emit_gather_words(b,0),sse42_emit_gather_words(a,0));
    SsePair reciprocal=sse_set_m128i(sse42_emit_gather_words(b,1),sse42_emit_gather_words(a,1));
    SsePair frequency=sse_srli_epi32(entry,12);
    SsePair refill=sse_cmpgt_epi32(sse_add_epi32(sse_srli_epi32(state,20),
                                                       sse_set1_epi32(1)),frequency);
    return (sse42_EmitHalf){entry,reciprocal,frequency,refill};
}

static QLI_SSE42_TARGET inline __attribute__((always_inline)) SsePair sse42_emit_finish_full(sse42_EmitHalf p,SsePair state)
{
    state=sse_blendv_epi8(state,sse_srli_epi32(state,16),p.refill);
    return sse42_emit_finish8(state,p.entry,p.reciprocal,p.frequency);
}

static QLI_SSE42_TARGET inline __m128i sse42_emit_pair_words(SsePair state,unsigned mask)
{
    SsePair low=sse_and_si128(state,sse_set1_epi32(65535));
    __m128i words=_mm_packus_epi32(sse_castsi256_si128(low),sse_extracti128_si256(low,1));
    return _mm_shuffle_epi8(words,sse42_numeric_load16(wave_emit_shuffle[mask]));
}

static QLI_SSE42_TARGET inline __attribute__((always_inline)) uint8_t *sse42_emit_pair_full(
    const uint64_t *table,SsePair key_l,SsePair key_h,SsePair *lower,SsePair *upper,uint8_t *tail)
{
    SsePair a=*lower,b=*upper;
    sse42_EmitHalf p=sse42_emit_prepare_full(table,key_l,a),q=sse42_emit_prepare_full(table,key_h,b);
    unsigned low=(unsigned)sse_movemask_ps(sse_castsi256_ps(p.refill));
    unsigned high=(unsigned)sse_movemask_ps(sse_castsi256_ps(q.refill));
    *lower=sse42_emit_finish_full(p,a);
    *upper=sse42_emit_finish_full(q,b);
    if (low|high) {
        uint8_t *middle=tail-2u*qlic_count_bits(high);
        sse42_numeric_store16((tail-16),sse42_emit_pair_words(b,high));
        sse42_numeric_store16((middle-16),sse42_emit_pair_words(a,low));
        tail=middle-2u*qlic_count_bits(low);
    }
    return tail;
}

static QLI_SSE42_TARGET uint8_t *sse42_numeric_emit_words(const uint64_t *table,const uint32_t *keys,size_t slots,
                                   uint32_t ans[16],uint8_t *tail)
{
    SsePair lower=sse42_numeric_load32(ans);
    SsePair upper=sse42_numeric_load32(ans+8);
    while (slots) {
        slots-=16;
        const uint32_t *group=keys+slots;
        SsePair lo=sse42_numeric_load32(group);
        SsePair hi=sse42_numeric_load32(group+8);
        if (!sse_movemask_ps(sse_castsi256_ps(sse_or_si128(lo,hi)))) {
            tail=sse42_emit_pair_full(table,lo,hi,&lower,&upper,tail);
        } else {
            upper=sse42_emit_step8(table,group+8,hi,upper,&tail);
            lower=sse42_emit_step8(table,group,lo,lower,&tail);
        }
    }
    sse42_numeric_store32(ans,lower);
    sse42_numeric_store32(ans+8,upper);
    return tail;
}
#endif
static inline unsigned row_direction(const uint8_t *plane, unsigned w,
                                     unsigned x, unsigned y, unsigned error)
{
    if (!x || !y) return 1;
    unsigned predicted = (uint8_t)(plane[x] - error);
    unsigned west = plane[x-1], north = (plane-w)[x];
    unsigned guide = (west + north + 1) >> 1;
    if (x >= 2 && y >= 2) {
        int gradient = (int)(west + north) - (plane-w)[x-1];
        unsigned clipped = gradient < 0 ? 0 : gradient > 255 ? 255 : (unsigned)gradient;
        if (clipped != predicted) guide = clipped;
    }
    return 1u + (guide > predicted) - (predicted > guide);
}

static size_t row_wave_activity(NumericScratch *s, unsigned w, unsigned h,
                                const uint8_t route[16][64])
{
    uint16_t *north = s->wave->north;
    memset(north, 0, (w+3)*sizeof(*north));
    uint64_t *hist = s->logical ? s->dir->emit : s->wave->hist;
    size_t count = 0;
    for (unsigned y = 0; y < h; ++y) {
        size_t pos = (size_t)y*w;
        const uint8_t *plane = s->plane + pos;
        const uint8_t *errors = s->error + pos;
        uint32_t *events = s->words + pos;
        uint64_t weight = wave_count(y & 15, (y/16) & 7);
        unsigned west = 0;
        for (unsigned x = 0; x < w; ++x) {
            unsigned e = errors[x];
            unsigned key = UINT32_MAX;
            if (!flat_at(s, pos+x)) {
                unsigned id = s->guides ? route[0][cue_key(s, pos+x)] : 0;
                unsigned context = id*WAVE_CONTEXTS + wave_context(west);
                if (s->logical) context = 3*context + row_direction(plane, w, x, y, e);
                key = (context<<8) | e;
                hist[key] += weight;
                ++count;
            }
            events[x] = key;
            unsigned a = north[x+2], b = north[x+3];
            unsigned zigzag = e < 128 ? 2*e : 2*(256-e)-1;
            west = (west + ((8*zigzag + (a > b ? a : b) + 1)>>1) + 1)>>1;
            north[x] = (uint16_t)west;
        }
    }
    return count;
}

static uint8_t *row_wave_emit(const NumericScratch *s, unsigned w, unsigned h,
                              uint32_t ans[16], uint8_t *tail)
{
    const uint64_t *table = s->logical ? s->dir->emit : s->wave->emit;
    for (unsigned band = (h+15)/16; band--;) {
        unsigned y = band*16;
        unsigned rows = h-y < 16 ? h-y : 16;
        for (unsigned step = w+4*(rows-1); step--;) {
            unsigned first = step < w ? 0 : (step-w)/4+1;
            unsigned last = step/4+1;
            if (last > rows) last = rows;
            for (unsigned lane = last; lane > first;) {
                --lane;
                unsigned x = step-4*lane;
                uint32_t key = s->words[(size_t)(y+lane)*w+x];
                if (key == UINT32_MAX) continue;
                uint64_t entry = table[key];
                unsigned field = (uint32_t)entry, freq = field>>12;
                uint32_t state = ans[lane];
                if ((state>>20) >= freq) {
                    tail -= 2;
                    qli_write_le16(tail, (uint16_t)state);
                    state >>= 16;
                }
                uint32_t q = (uint32_t)(((uint64_t)state*(entry>>32))>>32);
                q += state-q*freq >= freq;
                ans[lane] = state + q*(4096-freq) + (field&4095);
            }
        }
    }
    return tail;
}

static void wave_prepare_emit(NumericScratch *s,unsigned keys,const uint8_t *map,uint64_t *emit)
{
    for (unsigned k=0;k<keys;++k) {
        unsigned id=map[k];
        if (id==255) continue;
        const uint32_t *field=s->cond->fields+256*id;
        const uint32_t *reciprocal=s->cond->reciprocal+256*id;
        uint64_t *table=emit+256*k;
        for (unsigned v=0;v<256;++v)
            table[v]=field[v]|((uint64_t)reciprocal[v]<<32);
    }
}

static void fit_bankless_guides(NumericScratch *s,unsigned w,unsigned h,
                                uint8_t map[16*49],uint8_t route[16][64])
{
    NumericConditional *c=s->cond;
    CueStats bank[16]={0};
    memset(c->stats,0,sizeof(c->stats));
    for (unsigned y=0;y<h;y+=8) {
        const unsigned rows=span_end(y,h)-y;
        for (unsigned x=0;x<w;x+=8) {
            const unsigned sample=(x/8+y/8)%rows;
            const size_t row=(size_t)(y+sample)*w;
            const unsigned end=span_end(x,w);
            for (unsigned col=x;col<end;++col) {
                const size_t at=row+col;
                if (flat_at(s,at)) continue;
                observe(&c->stats[0][cue_key(s,at)],s->error[at]);
            }
        }
    }
    const unsigned width=s->guides==2 ? 49 : 7;
    for (unsigned k=0;k<width;++k) add_stats(bank,&c->stats[0][k]);
    choose_ready(c,bank,width,map,route);
}

static size_t numeric_wave_slots(unsigned w, unsigned h)
{
    return (size_t)((h+15)/16)*(w+60)*16;
}

static int wave_packets(NumericOutput *out,NumericScratch *s,unsigned w,unsigned h)
{
    uint8_t map[16*49],route[16][64];
    if (s->guides) {
        fit_bankless_guides(s,w,h,map,route);
        if (!map_record(out,map,s->guides==2 ? 49 : 7)) return 0;
    }
    unsigned old_count=s->guides ? 32 : 1;
    unsigned keys=old_count*WAVE_CONTEXTS;
    unsigned routes=s->logical ? 3*keys : keys;
    uint64_t *table=s->logical ? s->dir->emit : s->wave->emit;
    memset(table,0,(size_t)routes*256*sizeof(*table));
    size_t count;
#ifdef QLI_NUMERIC_SSE42
    if (s->vector) count=QLI_NUMERIC_FN(s->vector, wave_activity_simd)(s,w,h,route);
    else
#endif
    count=row_wave_activity(s,w,h,route);
    if (!count) return 0;
    if (s->logical) direction_fold(s,keys);
    unsigned models=wave_fit(s,old_count);
    if (!models) return 0;
    if (s->logical) models=direction_fit(s,keys,models);
    const uint8_t *mapping=s->logical ? s->dir->map : s->wave->map;
    if (!models || !wave_record(out,mapping,routes) || !wave_models(out,s->cond,models,s->packed_rgb)) return 0;
    uint32_t ans[16];
    for (unsigned k=0;k<16;++k) ans[k]=65536;
    uint8_t *tail=s->packet+s->packet_cap;
    wave_prepare_emit(s,routes,mapping,table);
#ifdef QLI_NUMERIC_SSE42
    if (s->vector) tail=QLI_NUMERIC_FN(s->vector, numeric_emit_words)(table,s->words,numeric_wave_slots(w,h),ans,tail);
    else
#endif
    tail=row_wave_emit(s,w,h,ans,tail);
    tail-=64;
    for (unsigned k=0;k<16;++k) qli_write_le32(tail+4*k,ans[k]);
    return append(out,tail,(size_t)(s->packet+s->packet_cap-tail));
}

static int banks(NumericOutput *out,NumericScratch *s,unsigned w,unsigned h)
{
    if (s->diagonal) return wave_packets(out,s,w,h);
    size_t totals[NUMERIC_BANKS]={0}, cursor[NUMERIC_BANKS];
    size_t maps=bank_map(s,w,h,totals);
    if (!map_record(out,s->map,maps)) return 0;
    size_t offset=0;
    for (unsigned id=0;id<NUMERIC_BANKS;++id) {
        cursor[id]=offset;
        offset+=totals[id];
    }
    uint64_t hist[NUMERIC_BANKS][256]={{0}};
    const uint8_t *stage=s->plane;
    for (size_t i=0;i<s->tiles;++i) {
        unsigned id=s->groups[i].bank, n=s->groups[i].live;
        if (!n) continue;
        memcpy(s->queue+cursor[id],stage,n);
        uint64_t *counts=hist[id];
        for (unsigned j=0;j<n;++j) ++counts[stage[j]];
        cursor[id]+=n;
        stage+=n;
    }
    offset=0;
    for (unsigned id=0;id<NUMERIC_BANKS;++id) {
        size_t count=totals[id];
        const uint8_t *symbols=s->queue+offset;
        size_t bytes=count>50 ? packed_record(s->packet,s->packet_cap,symbols,count,hist[id]) : 0;
        int ok=bytes && bytes<count ? write_numeric_record(out,s->packet,bytes,1) : write_numeric_record(out,symbols,count,0);
        if (!ok) return 0;
        offset+=count;
    }
    return 1;
}

static void byte_scalar(const uint8_t *src,size_t stride,size_t y,size_t start,size_t stop,
                        unsigned scores[8],int mode,uint8_t *out)
{
    const uint8_t *current=src+y*stride;
    const uint8_t *above=y ? current-stride : current;
    for (size_t x=start;x<stop;++x) {
        int w=x ? current[x-1] : y ? above[x] : 128;
        int n=y ? above[x] : w;
        int nw=x && y ? above[x-1] : n;
        int ne=y && x+1<stride ? above[x+1] : n;
        if (out) {
            out[x]=(uint8_t)(current[x]-scalar_predict(mode,w,n,nw,ne,255));
        } else {
            for (int i=0;i<8;++i) {
                unsigned e=(uint8_t)(current[x]-scalar_predict(i,w,n,nw,ne,255));
                scores[i]+=wrapped_magnitude(e);
            }
        }
    }
}

static uint8_t byte_best(const unsigned scores[8])
{
    unsigned best=0;
    for (unsigned i=1;i<8;++i) if (scores[i]<scores[best]) best=i;
    return (uint8_t)best;
}

#ifdef QLI_NUMERIC_AVX2

static QLI_NUMERIC_TARGET __m128i byte_load16(const uint8_t *p)
{
    return numeric_load16(p);
}

static QLI_NUMERIC_TARGET __m256i byte_load32(const uint8_t *p)
{
    return numeric_load32(p);
}

static QLI_NUMERIC_TARGET __m256i byte_gradient32(__m256i w,__m256i n,__m256i nw)
{
    __m256i middle=_mm256_min_epu8(_mm256_max_epu8(nw,_mm256_min_epu8(w,n)),
                                  _mm256_max_epu8(w,n));
    return _mm256_sub_epi8(_mm256_add_epi8(w,n),middle);
}

static QLI_NUMERIC_TARGET __m128i byte_gradient16(__m128i w,__m128i n,__m128i nw)
{
    __m128i middle=_mm_min_epu8(_mm_max_epu8(nw,_mm_min_epu8(w,n)),_mm_max_epu8(w,n));
    return _mm_sub_epi8(_mm_add_epi8(w,n),middle);
}

static QLI_NUMERIC_TARGET __m128i numeric_pack16(__m256i words)
{
    return _mm_packus_epi16(_mm256_castsi256_si128(words),_mm256_extracti128_si256(words,1));
}


static QLI_NUMERIC_TARGET __m256i byte_pack32(__m256i low,__m256i high)
{
    return _mm256_permute4x64_epi64(_mm256_packus_epi16(low,high),0xd8);
}

static QLI_NUMERIC_TARGET __m128i byte_predict16(int mode,__m128i w,__m128i n,__m128i nw,__m128i ne)
{
    switch (mode) {
    case 0: return byte_gradient16(w,n,nw);
    case 1: return w;
    case 2: return n;
    default: {
        return numeric_pack16(vector_predict(mode,_mm256_cvtepu8_epi16(w),_mm256_cvtepu8_epi16(n),
                           _mm256_cvtepu8_epi16(nw),_mm256_cvtepu8_epi16(ne),_mm256_set1_epi16(255)));
    }
    }
}

static QLI_NUMERIC_TARGET __m128i byte_sad16(__m128i actual,__m128i prediction)
{
    return _mm_sad_epu8(_mm_abs_epi8(_mm_sub_epi8(actual,prediction)),_mm_setzero_si128());
}

static QLI_NUMERIC_TARGET __m256i byte_sad32(__m256i actual,__m256i prediction)
{
    return _mm256_sad_epu8(_mm256_abs_epi8(_mm256_sub_epi8(actual,prediction)),_mm256_setzero_si256());
}

static QLI_NUMERIC_TARGET void byte_score_pair(const uint8_t *src,size_t stride,size_t rows,unsigned a[8],unsigned b[8])
{
    __m256i sums[8];
    for (int i=0;i<8;++i) sums[i]=_mm256_setzero_si256();
    const __m256i limit=_mm256_set1_epi16(255);
    for (size_t y=0;y<rows;++y) {
        const uint8_t *p=src+y*stride,*above=p-stride;
        __m256i q=byte_load32(p),w=byte_load32(p-1),n=byte_load32(above);
        __m256i nw=byte_load32(above-1),ne=byte_load32(above+1);
        sums[0]=_mm256_add_epi64(sums[0],byte_sad32(q,byte_gradient32(w,n,nw)));
        sums[1]=_mm256_add_epi64(sums[1],byte_sad32(q,w));
        sums[2]=_mm256_add_epi64(sums[2],byte_sad32(q,n));
        __m256i wl=_mm256_cvtepu8_epi16(_mm256_castsi256_si128(w));
        __m256i nl=_mm256_cvtepu8_epi16(_mm256_castsi256_si128(n));
        __m256i nwl=_mm256_cvtepu8_epi16(_mm256_castsi256_si128(nw));
        __m256i nel=_mm256_cvtepu8_epi16(_mm256_castsi256_si128(ne));
        __m256i wh=_mm256_cvtepu8_epi16(_mm256_extracti128_si256(w,1));
        __m256i nh=_mm256_cvtepu8_epi16(_mm256_extracti128_si256(n,1));
        __m256i nwh=_mm256_cvtepu8_epi16(_mm256_extracti128_si256(nw,1));
        __m256i neh=_mm256_cvtepu8_epi16(_mm256_extracti128_si256(ne,1));
        #pragma clang loop unroll(full)
        for (int i=3;i<8;++i) {
            __m256i l=vector_predict(i,wl,nl,nwl,nel,limit), r=vector_predict(i,wh,nh,nwh,neh,limit);
            sums[i]=_mm256_add_epi64(sums[i],byte_sad32(q,byte_pack32(l,r)));
        }
    }
    for (int i=0;i<8;++i) {
        __m256i s=_mm256_add_epi64(sums[i],_mm256_srli_si256(sums[i],8));
        a[i]+=(unsigned)_mm_cvtsi128_si32(_mm256_castsi256_si128(s));
        b[i]+=(unsigned)_mm_cvtsi128_si32(_mm256_extracti128_si256(s,1));
    }
}

static QLI_NUMERIC_TARGET void byte_score_single(const uint8_t *src,size_t stride,size_t rows,unsigned scores[8])
{
    __m128i sums[8];
    for (int i=0;i<8;++i) sums[i]=_mm_setzero_si128();
    for (size_t y=0;y<rows;++y) {
        const uint8_t *p=src+y*stride,*above=p-stride;
        __m128i q=byte_load16(p),w=byte_load16(p-1),n=byte_load16(above);
        __m128i nw=byte_load16(above-1),ne=byte_load16(above+1);
        #pragma clang loop unroll(full)
        for (int i=0;i<8;++i)
            sums[i]=_mm_add_epi64(sums[i],byte_sad16(q,byte_predict16(i,w,n,nw,ne)));
    }
    for (int i=0;i<8;++i)
        scores[i]+=(unsigned)_mm_cvtsi128_si32(_mm_add_epi64(sums[i],_mm_srli_si128(sums[i],8)));
}

static QLI_NUMERIC_TARGET unsigned byte_zero_pair(const uint8_t *src,size_t stride,size_t rows)
{
    unsigned zero=3;
    for (size_t y=0;y<rows && zero;++y) {
        const uint8_t *p=src+y*stride,*n=p-stride;
        __m256i predicted=byte_gradient32(byte_load32(p-1),byte_load32(n),byte_load32(n-1));
        unsigned same=(unsigned)_mm256_movemask_epi8(_mm256_cmpeq_epi8(predicted,byte_load32(p)));
        zero&=((same&65535u)==65535u) + 2u*((same>>16)==65535u);
    }
    return zero;
}

static QLI_NUMERIC_TARGET int byte_zero_single(const uint8_t *src,size_t stride,size_t rows)
{
    for (size_t y=0;y<rows;++y) {
        const uint8_t *p=src+y*stride,*n=p-stride;
        __m128i predicted=byte_gradient16(byte_load16(p-1),byte_load16(n),byte_load16(n-1));
        if (_mm_movemask_epi8(_mm_cmpeq_epi8(predicted,byte_load16(p)))!=65535) return 0;
    }
    return 1;
}

static QLI_NUMERIC_TARGET void byte_emit(const uint8_t *src,uint8_t *out,size_t stride,size_t x,size_t right,
                      size_t y,size_t bottom,int vector,int mode,int zero)
{
    for (;y<bottom;++y) {
        if (zero) {
            numeric_store16((out+y*stride+x),_mm_setzero_si128());
        } else if (vector && y) {
            const uint8_t *p=src+y*stride+x,*n=p-stride;
            __m128i pred=byte_predict16(mode,byte_load16(p-1),byte_load16(n),
                                       byte_load16(n-1),byte_load16(n+1));
            numeric_store16((out+y*stride+x),_mm_sub_epi8(byte_load16(p),pred));
        } else {
            byte_scalar(src,stride,y,x,right,NULL,mode,out+y*stride);
        }
    }
}

static QLI_NUMERIC_TARGET void numeric_modes_vector(const uint8_t *src,uint8_t *out,uint8_t *mode,unsigned width,unsigned height)
{
    for (size_t y=0;y<height;y+=16) {
        size_t bottom=y+16<height ? y+16 : height;
        for (size_t x=0;x<width;) {
            size_t right=x+16<width ? x+16 : width;
            unsigned a[8]={0};
            int interior=x && right<width;
            if (interior && x+32<width) {
                unsigned b[8]={0};
                unsigned zero=y ? byte_zero_pair(src+y*width+x,width,bottom-y) : 0;
                size_t first=y ? y : 1;
                if (!y) {
                    byte_scalar(src,width,0,x,right,a,0,NULL);
                    byte_scalar(src,width,0,right,x+32,b,0,NULL);
                }
                const uint8_t *p=first<bottom ? src+first*width+x : NULL;
                if (!zero && p) byte_score_pair(p,width,bottom-first,a,b);
                if (zero==1) byte_score_single(p+16,width,bottom-first,b);
                if (zero==2) byte_score_single(p,width,bottom-first,a);
                *mode=byte_best(a);
                byte_emit(src,out,width,x,right,y,bottom,1,*mode++,zero&1);
                *mode=byte_best(b);
                byte_emit(src,out,width,right,x+32,y,bottom,1,*mode++,zero&2);
                x+=32;
                continue;
            }
            {
                int zero=interior && y && byte_zero_single(src+y*width+x,width,bottom-y);
                if (interior && !zero) {
                    size_t first=y ? y : 1;
                    if (!y) byte_scalar(src,width,0,x,right,a,0,NULL);
                    if (first<bottom) byte_score_single(src+first*width+x,width,bottom-first,a);
                } else if (!interior) {
                    for (size_t r=y;r<bottom;++r) byte_scalar(src,width,r,x,right,a,0,NULL);
                }
                *mode=byte_best(a);
                byte_emit(src,out,width,x,right,y,bottom,interior,*mode++,zero);
                x=right;
            }
        }
    }
}
#endif
#ifdef QLI_NUMERIC_SSE42

static QLI_SSE42_TARGET __m128i sse42_byte_load16(const uint8_t *p)
{
    return sse42_numeric_load16(p);
}

static QLI_SSE42_TARGET SsePair sse42_byte_load32(const uint8_t *p)
{
    return sse42_numeric_load32(p);
}

static QLI_SSE42_TARGET SsePair sse42_byte_gradient32(SsePair w,SsePair n,SsePair nw)
{
    SsePair middle=sse_min_epu8(sse_max_epu8(nw,sse_min_epu8(w,n)),
                                  sse_max_epu8(w,n));
    return sse_sub_epi8(sse_add_epi8(w,n),middle);
}

static QLI_SSE42_TARGET __m128i sse42_byte_gradient16(__m128i w,__m128i n,__m128i nw)
{
    __m128i middle=_mm_min_epu8(_mm_max_epu8(nw,_mm_min_epu8(w,n)),_mm_max_epu8(w,n));
    return _mm_sub_epi8(_mm_add_epi8(w,n),middle);
}

static QLI_SSE42_TARGET __m128i sse42_numeric_pack16(SsePair words)
{
    return _mm_packus_epi16(sse_castsi256_si128(words),sse_extracti128_si256(words,1));
}


static QLI_SSE42_TARGET SsePair sse42_byte_pack32(SsePair low,SsePair high)
{
    return sse_permute4x64_epi64(sse_packus_epi16(low,high),0xd8);
}

static QLI_SSE42_TARGET __m128i sse42_byte_predict16(int mode,__m128i w,__m128i n,__m128i nw,__m128i ne)
{
    switch (mode) {
    case 0: return sse42_byte_gradient16(w,n,nw);
    case 1: return w;
    case 2: return n;
    default: {
        return sse42_numeric_pack16(sse42_vector_predict(mode,sse_cvtepu8_epi16(w),sse_cvtepu8_epi16(n),
                           sse_cvtepu8_epi16(nw),sse_cvtepu8_epi16(ne),sse_set1_epi16(255)));
    }
    }
}

static QLI_SSE42_TARGET __m128i sse42_byte_sad16(__m128i actual,__m128i prediction)
{
    return _mm_sad_epu8(_mm_abs_epi8(_mm_sub_epi8(actual,prediction)),_mm_setzero_si128());
}

static QLI_SSE42_TARGET SsePair sse42_byte_sad32(SsePair actual,SsePair prediction)
{
    return sse_sad_epu8(sse_abs_epi8(sse_sub_epi8(actual,prediction)),sse_setzero_si256());
}

static QLI_SSE42_TARGET void sse42_byte_score_pair(const uint8_t *src,size_t stride,size_t rows,unsigned a[8],unsigned b[8])
{
    SsePair sums[8];
    for (int i=0;i<8;++i) sums[i]=sse_setzero_si256();
    const SsePair limit=sse_set1_epi16(255);
    for (size_t y=0;y<rows;++y) {
        const uint8_t *p=src+y*stride,*above=p-stride;
        SsePair q=sse42_byte_load32(p),w=sse42_byte_load32(p-1),n=sse42_byte_load32(above);
        SsePair nw=sse42_byte_load32(above-1),ne=sse42_byte_load32(above+1);
        sums[0]=sse_add_epi64(sums[0],sse42_byte_sad32(q,sse42_byte_gradient32(w,n,nw)));
        sums[1]=sse_add_epi64(sums[1],sse42_byte_sad32(q,w));
        sums[2]=sse_add_epi64(sums[2],sse42_byte_sad32(q,n));
        SsePair wl=sse_cvtepu8_epi16(sse_castsi256_si128(w));
        SsePair nl=sse_cvtepu8_epi16(sse_castsi256_si128(n));
        SsePair nwl=sse_cvtepu8_epi16(sse_castsi256_si128(nw));
        SsePair nel=sse_cvtepu8_epi16(sse_castsi256_si128(ne));
        SsePair wh=sse_cvtepu8_epi16(sse_extracti128_si256(w,1));
        SsePair nh=sse_cvtepu8_epi16(sse_extracti128_si256(n,1));
        SsePair nwh=sse_cvtepu8_epi16(sse_extracti128_si256(nw,1));
        SsePair neh=sse_cvtepu8_epi16(sse_extracti128_si256(ne,1));
        #pragma clang loop unroll(full)
        for (int i=3;i<8;++i) {
            SsePair l=sse42_vector_predict(i,wl,nl,nwl,nel,limit), r=sse42_vector_predict(i,wh,nh,nwh,neh,limit);
            sums[i]=sse_add_epi64(sums[i],sse42_byte_sad32(q,sse42_byte_pack32(l,r)));
        }
    }
    for (int i=0;i<8;++i) {
        SsePair s=sse_add_epi64(sums[i],sse_srli_si256(sums[i],8));
        a[i]+=(unsigned)_mm_cvtsi128_si32(sse_castsi256_si128(s));
        b[i]+=(unsigned)_mm_cvtsi128_si32(sse_extracti128_si256(s,1));
    }
}

static QLI_SSE42_TARGET void sse42_byte_score_single(const uint8_t *src,size_t stride,size_t rows,unsigned scores[8])
{
    __m128i sums[8];
    for (int i=0;i<8;++i) sums[i]=_mm_setzero_si128();
    for (size_t y=0;y<rows;++y) {
        const uint8_t *p=src+y*stride,*above=p-stride;
        __m128i q=sse42_byte_load16(p),w=sse42_byte_load16(p-1),n=sse42_byte_load16(above);
        __m128i nw=sse42_byte_load16(above-1),ne=sse42_byte_load16(above+1);
        #pragma clang loop unroll(full)
        for (int i=0;i<8;++i)
            sums[i]=_mm_add_epi64(sums[i],sse42_byte_sad16(q,sse42_byte_predict16(i,w,n,nw,ne)));
    }
    for (int i=0;i<8;++i)
        scores[i]+=(unsigned)_mm_cvtsi128_si32(_mm_add_epi64(sums[i],_mm_srli_si128(sums[i],8)));
}

static QLI_SSE42_TARGET unsigned sse42_byte_zero_pair(const uint8_t *src,size_t stride,size_t rows)
{
    unsigned zero=3;
    for (size_t y=0;y<rows && zero;++y) {
        const uint8_t *p=src+y*stride,*n=p-stride;
        SsePair predicted=sse42_byte_gradient32(sse42_byte_load32(p-1),sse42_byte_load32(n),sse42_byte_load32(n-1));
        unsigned same=(unsigned)sse_movemask_epi8(sse_cmpeq_epi8(predicted,sse42_byte_load32(p)));
        zero&=((same&65535u)==65535u) + 2u*((same>>16)==65535u);
    }
    return zero;
}

static QLI_SSE42_TARGET int sse42_byte_zero_single(const uint8_t *src,size_t stride,size_t rows)
{
    for (size_t y=0;y<rows;++y) {
        const uint8_t *p=src+y*stride,*n=p-stride;
        __m128i predicted=sse42_byte_gradient16(sse42_byte_load16(p-1),sse42_byte_load16(n),sse42_byte_load16(n-1));
        if (_mm_movemask_epi8(_mm_cmpeq_epi8(predicted,sse42_byte_load16(p)))!=65535) return 0;
    }
    return 1;
}

static QLI_SSE42_TARGET void sse42_byte_emit(const uint8_t *src,uint8_t *out,size_t stride,size_t x,size_t right,
                      size_t y,size_t bottom,int vector,int mode,int zero)
{
    for (;y<bottom;++y) {
        if (zero) {
            sse42_numeric_store16((out+y*stride+x),_mm_setzero_si128());
        } else if (vector && y) {
            const uint8_t *p=src+y*stride+x,*n=p-stride;
            __m128i pred=sse42_byte_predict16(mode,sse42_byte_load16(p-1),sse42_byte_load16(n),
                                       sse42_byte_load16(n-1),sse42_byte_load16(n+1));
            sse42_numeric_store16((out+y*stride+x),_mm_sub_epi8(sse42_byte_load16(p),pred));
        } else {
            byte_scalar(src,stride,y,x,right,NULL,mode,out+y*stride);
        }
    }
}

static QLI_SSE42_TARGET void sse42_numeric_modes_vector(const uint8_t *src,uint8_t *out,uint8_t *mode,unsigned width,unsigned height)
{
    for (size_t y=0;y<height;y+=16) {
        size_t bottom=y+16<height ? y+16 : height;
        for (size_t x=0;x<width;) {
            size_t right=x+16<width ? x+16 : width;
            unsigned a[8]={0};
            int interior=x && right<width;
            if (interior && x+32<width) {
                unsigned b[8]={0};
                unsigned zero=y ? sse42_byte_zero_pair(src+y*width+x,width,bottom-y) : 0;
                size_t first=y ? y : 1;
                if (!y) {
                    byte_scalar(src,width,0,x,right,a,0,NULL);
                    byte_scalar(src,width,0,right,x+32,b,0,NULL);
                }
                const uint8_t *p=first<bottom ? src+first*width+x : NULL;
                if (!zero && p) sse42_byte_score_pair(p,width,bottom-first,a,b);
                if (zero==1) sse42_byte_score_single(p+16,width,bottom-first,b);
                if (zero==2) sse42_byte_score_single(p,width,bottom-first,a);
                *mode=byte_best(a);
                sse42_byte_emit(src,out,width,x,right,y,bottom,1,*mode++,zero&1);
                *mode=byte_best(b);
                sse42_byte_emit(src,out,width,right,x+32,y,bottom,1,*mode++,zero&2);
                x+=32;
                continue;
            }
            {
                int zero=interior && y && sse42_byte_zero_single(src+y*width+x,width,bottom-y);
                if (interior && !zero) {
                    size_t first=y ? y : 1;
                    if (!y) byte_scalar(src,width,0,x,right,a,0,NULL);
                    if (first<bottom) sse42_byte_score_single(src+first*width+x,width,bottom-first,a);
                } else if (!interior) {
                    for (size_t r=y;r<bottom;++r) byte_scalar(src,width,r,x,right,a,0,NULL);
                }
                *mode=byte_best(a);
                sse42_byte_emit(src,out,width,x,right,y,bottom,interior,*mode++,zero);
                x=right;
            }
        }
    }
}
#endif
#define SCORE_LANES UINT64_C(0x0001000100010001)
#define SCORE_BYTES (SCORE_LANES * 255)

static inline uint64_t score_load(const uint8_t *bytes)
{
    uint32_t word;
    memcpy(&word, bytes, sizeof(word));
    uint64_t lanes = word;
    lanes = (lanes | (lanes << 16)) & UINT64_C(0x0000ffff0000ffff);
    return (lanes | (lanes << 8)) & SCORE_BYTES;
}

static inline uint64_t score_at_least(uint64_t a, uint64_t b)
{
    const uint64_t guard = SCORE_LANES * 32768;
    return (((guard + a - b) & guard) >> 15) * 65535;
}

static inline uint64_t score_min(uint64_t a, uint64_t b)
{
    return a ^ ((a ^ b) & score_at_least(a, b));
}

static inline uint64_t score_max(uint64_t a, uint64_t b)
{
    return b ^ ((a ^ b) & score_at_least(a, b));
}

static inline uint64_t score_thirds(uint64_t lanes)
{
    const uint64_t even = UINT64_C(0x000000ff000000ff);
    const uint64_t halfwords = UINT64_C(0x0000ffff0000ffff);
    uint64_t lo = (((lanes & halfwords) * 683) >> 11) & even;
    uint64_t hi = (((((lanes >> 16) & halfwords) * 683) >> 11) & even) << 16;
    return lo | hi;
}

static inline uint64_t score_magnitude(uint64_t samples, uint64_t predicted)
{
    uint64_t error = (SCORE_LANES * 256 + samples - predicted) & SCORE_BYTES;
    uint64_t negative = (error & (SCORE_LANES * 128)) >> 7;
    return (error ^ (negative * 255)) + negative;
}

static void score_interior(const uint8_t *current, size_t stride, size_t rows,
                           unsigned score[8])
{
    uint64_t m0=0, m1=0, m2=0, m3=0, m4=0, m5=0, m6=0, m7=0;
    for (size_t row=0; row<rows; ++row) {
        const uint8_t *p=current+row*stride, *above=p-stride;
        for (unsigned x=0; x<16; x+=4) {
            uint64_t q=score_load(p+x), w=score_load(p+x-1), n=score_load(above+x);
            uint64_t nw=score_load(above+x-1), ne=score_load(above+x+1);
            uint64_t wn=w+n, round=2*SCORE_LANES;
            uint64_t low=score_min(w,n), high=wn-low;
            uint64_t middle=score_min(score_max(nw,low),high);
            uint64_t biased=3*wn+round+512*SCORE_LANES-2*nw;
            biased=score_min(score_max(biased,512*SCORE_LANES),1532*SCORE_LANES);
            m0+=score_magnitude(q,wn-middle);
            m1+=score_magnitude(q,w);
            m2+=score_magnitude(q,n);
            m3+=score_magnitude(q,((wn+nw+ne+round)>>2)&SCORE_BYTES);
            m4+=score_magnitude(q,((wn+2*w+round)>>2)&SCORE_BYTES);
            m5+=score_magnitude(q,score_thirds(wn+ne+SCORE_LANES));
            m6+=score_magnitude(q,((wn+w+ne+round)>>2)&SCORE_BYTES);
            m7+=score_magnitude(q,((biased-512*SCORE_LANES)>>2)&SCORE_BYTES);
        }
    }
    const uint64_t packed[8]={m0,m1,m2,m3,m4,m5,m6,m7};
    for (unsigned i=0; i<8; ++i) {
        uint64_t value=packed[i];
        score[i]+=(uint16_t)value+(uint16_t)(value>>16)+
                  (uint16_t)(value>>32)+(uint16_t)(value>>48);
    }
}

#undef SCORE_BYTES
#undef SCORE_LANES

static int plain_gradient_zero(const uint8_t *current, size_t stride, size_t rows)
{
    for (size_t row=0; row<rows; ++row) {
        const uint8_t *p=current+row*stride;
        const uint8_t *above=p-stride;
        for (unsigned x=0; x<16; ++x)
            if (p[x]!=scalar_predict(0,(p-1)[x],above[x],(above-1)[x],0,255)) return 0;
    }
    return 1;
}

static void numeric_modes(const uint8_t *src, uint8_t *out, uint8_t *mode,
                        unsigned width, unsigned height)
{
    for (size_t y=0; y<height; y+=16) {
        size_t bottom=y+16<height ? y+16 : height;
        for (size_t x=0; x<width; x+=16) {
            size_t right=x+16<width ? x+16 : width;
            int interior=x && right<width;
            if (interior && y && plain_gradient_zero(src+y*width+x,width,bottom-y)) {
                *mode++=0;
                for (size_t row=y; row<bottom; ++row) memset(out+row*width+x,0,16);
                continue;
            }
            unsigned score[8]={0};
            if (interior) {
                size_t first=y;
                if (!first) {
                    byte_scalar(src,width,0,x,right,score,0,NULL);
                    first=1;
                }
                if (first<bottom) score_interior(src+first*width+x,width,bottom-first,score);
            } else {
                for (size_t row=y; row<bottom; ++row)
                    byte_scalar(src,width,row,x,right,score,0,NULL);
            }
            unsigned pick=byte_best(score);
            *mode++=(uint8_t)pick;
            for (size_t row=y; row<bottom; ++row)
                byte_scalar(src,width,row,x,right,NULL,(int)pick,out+row*width);
        }
    }
}


typedef struct { unsigned x,y; uint8_t contrast; } FitPoint;
static const int8_t fit_prior[10]={24,24,-16,0,0,0,0,0,0,0};

static void fit_load(const uint8_t *p,ptrdiff_t w,int tap[10])
{
    tap[0]=p[-1]; tap[1]=p[-w]; tap[2]=p[-w-1]; tap[3]=p[-w+1];
    tap[4]=p[-2]; tap[5]=p[-w-2]; tap[6]=p[-2*w-1]; tap[7]=p[-2*w];
    tap[8]=p[-2*w+1]; tap[9]=p[-2*w+2];
}

static unsigned fit_coordinates(const NumericScratch *s,unsigned w,unsigned h,
                                FitPoint points[1024],unsigned hist[256])
{
    if (w<36 || h<34) return 0;
    unsigned used=0;
    for (unsigned gy=0;gy<32;++gy) {
        for (unsigned gx=0;gx<32;++gx) {
            unsigned l=gx*(w-4)/32,t=gy*(h-2)/32;
            unsigned dw=(gx+1)*(w-4)/32-l,dh=(gy+1)*(h-2)/32-t;
            unsigned x=2+l+(2*((3*gx+gy)&3)+1)*dw/8;
            unsigned y=2+t+(2*((gx+3*gy+1)&3)+1)*dh/8;
            size_t at=(size_t)y*w+x;
            if (flat_at(s,at)) continue;
            unsigned d=(unsigned)abs((int)s->plane[at-1]-s->plane[at-w]);
            points[used++]=(FitPoint){x,y,(uint8_t)d};
            ++hist[d];
        }
    }
    return used;
}

static unsigned fit_median(const unsigned hist[256],unsigned n)
{
    unsigned low=0,high=0,sum=hist[0],pos=0;
    while (sum<=(n-1)/2) sum+=hist[++pos];
    low=pos;
    while (sum<=n/2) sum+=hist[++pos];
    high=pos;
    return (low+high)/2;
}

static void fit_solve(double a[9][10],int8_t out[10])
{
    for (unsigned i=0;i<9;++i) {
        for (unsigned j=i+1;j<9;++j) {
            double q=a[j][i]/a[i][i];
            for (unsigned k=i+1;k<10;++k) a[j][k]-=q*a[i][k];
        }
    }
    double x[9];
    for (int i=8;i>=0;--i) {
        double v=a[i][9];
        for (unsigned j=(unsigned)i+1;j<9;++j) v-=a[i][j]*x[j];
        x[i]=v/a[i][i];
    }
    int8_t weights[10];
    int remainder=32;
    for (unsigned i=0;i<9;++i) {
        double value=nearbyint(32*x[i]);
        if (!(value>=-64 && value<=64)) return;
        weights[i ? i+1 : 0]=(int8_t)value;
        remainder-=(int)value;
    }
    if (remainder < -64 || remainder > 64) return;
    weights[1]=(int8_t)remainder;
    memcpy(out,weights,sizeof(weights));
}

static void fit_vectors(NumericScratch *s,unsigned w,unsigned h)
{
    FitPoint points[1024];
    unsigned hist[256]={0};
    unsigned n=fit_coordinates(s,w,h,points,hist);
    memcpy(s->fit[0],fit_prior,sizeof(fit_prior));
    memcpy(s->fit[1],fit_prior,sizeof(fit_prior));
    if (n<32) return;
    unsigned middle=fit_median(hist,n), members[2]={0};
    double mat[2][9][10]={0};
    for (unsigned t=0;t<n;++t) {
        FitPoint p=points[t];
        size_t at=(size_t)p.y*w+p.x;
        int tap[10], features[9];
        fit_load(s->plane+at,w,tap);
        unsigned half=p.contrast>middle;
        int value=s->plane[at], original=(uint8_t)(value-s->error[at]);
        double weight=1.0/(1.0+abs(value-original)/8.0);
        double target=(value-tap[1])*weight;
        ++members[half];
        for (unsigned i=0;i<9;++i) features[i]=tap[i ? i+1 : 0]-tap[1];
        for (unsigned i=0;i<9;++i) {
            double v=features[i]*weight;
            for (unsigned j=0;j<=i;++j) mat[half][i][j]+=v*features[j];
            mat[half][i][9]+=features[i]*target;
        }
    }
    for (unsigned half=0;half<2;++half) {
        if (members[half]<32) continue;
        for (unsigned i=0;i<9;++i) {
            for (unsigned j=0;j<i;++j) mat[half][j][i]=mat[half][i][j];
            mat[half][i][i]+=256.0;
            mat[half][i][9]+=8.0*fit_prior[i ? i+1 : 0];
        }
        fit_solve(mat[half],s->fit[half]);
    }
}

static unsigned fit_pick(const unsigned score[3])
{
    unsigned best=0;
    if (score[1]<score[best]) best=1;
    if (score[2]<score[best]) best=2;
    return best;
}

#ifdef QLI_NUMERIC_SSE42
typedef struct { _Alignas(32) uint8_t pair[2][5][32]; int word; } FitWeights;
typedef unsigned FitScorer(const uint8_t *,uint8_t *,unsigned,unsigned,const FitWeights *);
#endif

#ifdef QLI_NUMERIC_AVX2

static QLI_NUMERIC_TARGET __m128i fit_price(__m128i e)
{
    const __m128i low=_mm_setr_epi8(1,3,5,5,7,7,7,7,9,9,9,9,9,9,9,9);
    const __m128i high=_mm_setr_epi8(0,11,13,13,15,15,15,15,17,17,17,17,17,17,17,17);
    const __m128i nibble=_mm_set1_epi8(15);
    __m128i m=_mm_abs_epi8(e);
    __m128i lo=_mm_shuffle_epi8(low,_mm_and_si128(m,nibble));
    __m128i hi=_mm_shuffle_epi8(high,_mm_and_si128(_mm_srli_epi16(m,4),nibble));
    return _mm_sad_epu8(_mm_max_epu8(lo,hi),_mm_setzero_si128());
}

static QLI_NUMERIC_TARGET void fit_weights(FitWeights *out,const int8_t fit[2][10])
{
    out->word=1;
    for (unsigned k=0;k<2;++k) {
        int low=16,high=16;
        for (unsigned t=0;t<10;++t) {
            int term=255*(int)fit[k][t];
            if (term<0) low+=term;
            else high+=term;
        }
        if (low<INT16_MIN || high>INT16_MAX) out->word=0;
        for (unsigned t=0;t<5;++t) {
            uint16_t pair=(uint8_t)fit[k][2*t]|((uint16_t)(uint8_t)fit[k][2*t+1]<<8);
            numeric_store32(out->pair[k][t],_mm256_set1_epi16((int16_t)pair));
        }
    }
}


static QLI_NUMERIC_TARGET inline __attribute__((always_inline)) unsigned
fit_tile_kernel(const uint8_t *p,uint8_t *old,unsigned w,unsigned rows,
                const FitWeights *fit,int word)
{
    const ptrdiff_t s=w;
    const ptrdiff_t offset[10]={-1,-s,-s-1,-s+1,-2,-s-2,-2*s-1,-2*s,-2*s+1,-2*s+2};
    __m128i values[2][16],scores[3]={_mm_setzero_si128(),_mm_setzero_si128(),_mm_setzero_si128()};
    for (unsigned y=0;y<rows;++y) {
        const uint8_t *q=p+(size_t)y*w;
        __m256i taps[5];
        for (unsigned t=0;t<5;++t) {
            __m128i a=byte_load16(q+offset[2*t]),b=byte_load16(q+offset[2*t+1]);
            taps[t]=_mm256_set_m128i(_mm_unpackhi_epi8(a,b),_mm_unpacklo_epi8(a,b));
        }
        __m128i actual=byte_load16(q);
        scores[0]=_mm_add_epi64(scores[0],fit_price(byte_load16(old+(size_t)y*w)));
        for (unsigned k=0;k<2;++k) {
            __m128i prediction;
            if (word) {
                __m256i sum=_mm256_set1_epi16(16);
                for (unsigned t=0;t<5;++t)
                    sum=_mm256_add_epi16(sum,_mm256_maddubs_epi16(taps[t],numeric_load32(fit->pair[k][t])));
                sum=_mm256_srai_epi16(sum,5);
                prediction=_mm_packus_epi16(_mm256_castsi256_si128(sum),_mm256_extracti128_si256(sum,1));
            } else {
                __m256i lo=_mm256_set1_epi32(16),hi=lo;
                for (unsigned t=0;t<5;++t) {
                    __m256i pair=_mm256_maddubs_epi16(taps[t],numeric_load32(fit->pair[k][t]));
                    lo=_mm256_add_epi32(lo,_mm256_cvtepi16_epi32(_mm256_castsi256_si128(pair)));
                    hi=_mm256_add_epi32(hi,_mm256_cvtepi16_epi32(_mm256_extracti128_si256(pair,1)));
                }
                __m256i word=_mm256_packs_epi32(_mm256_srai_epi32(lo,5),_mm256_srai_epi32(hi,5));
                word=_mm256_permute4x64_epi64(word,0xd8);
                prediction=_mm_packus_epi16(_mm256_castsi256_si128(word),_mm256_extracti128_si256(word,1));
            }
            values[k][y]=_mm_sub_epi8(actual,prediction);
            scores[k+1]=_mm_add_epi64(scores[k+1],fit_price(values[k][y]));
        }
    }
    unsigned score[3];
    for (unsigned k=0;k<3;++k)
        score[k]=(unsigned)_mm_cvtsi128_si32(_mm_add_epi64(scores[k],_mm_srli_si128(scores[k],8)));
    unsigned best=fit_pick(score);
    if (best)
        for (unsigned y=0;y<rows;++y) numeric_store16((old+(size_t)y*w),values[best-1][y]);
    return best;
}

static QLI_NUMERIC_TARGET unsigned fit_tile_word(const uint8_t *p,uint8_t *old,unsigned w,unsigned rows,const FitWeights *fit)
{
    return fit_tile_kernel(p,old,w,rows,fit,1);
}

static QLI_NUMERIC_TARGET unsigned fit_tile_wide(const uint8_t *p,uint8_t *old,unsigned w,unsigned rows,const FitWeights *fit)
{
    return fit_tile_kernel(p,old,w,rows,fit,0);
}
#endif
#ifdef QLI_NUMERIC_SSE42

static QLI_SSE42_TARGET __m128i sse42_fit_price(__m128i e)
{
    const __m128i low=_mm_setr_epi8(1,3,5,5,7,7,7,7,9,9,9,9,9,9,9,9);
    const __m128i high=_mm_setr_epi8(0,11,13,13,15,15,15,15,17,17,17,17,17,17,17,17);
    const __m128i nibble=_mm_set1_epi8(15);
    __m128i m=_mm_abs_epi8(e);
    __m128i lo=_mm_shuffle_epi8(low,_mm_and_si128(m,nibble));
    __m128i hi=_mm_shuffle_epi8(high,_mm_and_si128(_mm_srli_epi16(m,4),nibble));
    return _mm_sad_epu8(_mm_max_epu8(lo,hi),_mm_setzero_si128());
}

static QLI_SSE42_TARGET void sse42_fit_weights(FitWeights *out,const int8_t fit[2][10])
{
    out->word=1;
    for (unsigned k=0;k<2;++k) {
        int low=16,high=16;
        for (unsigned t=0;t<10;++t) {
            int term=255*(int)fit[k][t];
            if (term<0) low+=term;
            else high+=term;
        }
        if (low<INT16_MIN || high>INT16_MAX) out->word=0;
        for (unsigned t=0;t<5;++t) {
            uint16_t pair=(uint8_t)fit[k][2*t]|((uint16_t)(uint8_t)fit[k][2*t+1]<<8);
            sse42_numeric_store32(out->pair[k][t],sse_set1_epi16((int16_t)pair));
        }
    }
}


static QLI_SSE42_TARGET inline __attribute__((always_inline)) unsigned
sse42_fit_tile_kernel(const uint8_t *p,uint8_t *old,unsigned w,unsigned rows,
                const FitWeights *fit,int word)
{
    const ptrdiff_t s=w;
    const ptrdiff_t offset[10]={-1,-s,-s-1,-s+1,-2,-s-2,-2*s-1,-2*s,-2*s+1,-2*s+2};
    __m128i values[2][16],scores[3]={_mm_setzero_si128(),_mm_setzero_si128(),_mm_setzero_si128()};
    for (unsigned y=0;y<rows;++y) {
        const uint8_t *q=p+(size_t)y*w;
        SsePair taps[5];
        for (unsigned t=0;t<5;++t) {
            __m128i a=sse42_byte_load16(q+offset[2*t]),b=sse42_byte_load16(q+offset[2*t+1]);
            taps[t]=sse_set_m128i(_mm_unpackhi_epi8(a,b),_mm_unpacklo_epi8(a,b));
        }
        __m128i actual=sse42_byte_load16(q);
        scores[0]=_mm_add_epi64(scores[0],sse42_fit_price(sse42_byte_load16(old+(size_t)y*w)));
        for (unsigned k=0;k<2;++k) {
            __m128i prediction;
            if (word) {
                SsePair sum=sse_set1_epi16(16);
                for (unsigned t=0;t<5;++t)
                    sum=sse_add_epi16(sum,sse_maddubs_epi16(taps[t],sse42_numeric_load32(fit->pair[k][t])));
                sum=sse_srai_epi16(sum,5);
                prediction=_mm_packus_epi16(sse_castsi256_si128(sum),sse_extracti128_si256(sum,1));
            } else {
                SsePair lo=sse_set1_epi32(16),hi=lo;
                for (unsigned t=0;t<5;++t) {
                    SsePair pair=sse_maddubs_epi16(taps[t],sse42_numeric_load32(fit->pair[k][t]));
                    lo=sse_add_epi32(lo,sse_cvtepi16_epi32(sse_castsi256_si128(pair)));
                    hi=sse_add_epi32(hi,sse_cvtepi16_epi32(sse_extracti128_si256(pair,1)));
                }
                SsePair word=sse_packs_epi32(sse_srai_epi32(lo,5),sse_srai_epi32(hi,5));
                word=sse_permute4x64_epi64(word,0xd8);
                prediction=_mm_packus_epi16(sse_castsi256_si128(word),sse_extracti128_si256(word,1));
            }
            values[k][y]=_mm_sub_epi8(actual,prediction);
            scores[k+1]=_mm_add_epi64(scores[k+1],sse42_fit_price(values[k][y]));
        }
    }
    unsigned score[3];
    for (unsigned k=0;k<3;++k)
        score[k]=(unsigned)_mm_cvtsi128_si32(_mm_add_epi64(scores[k],_mm_srli_si128(scores[k],8)));
    unsigned best=fit_pick(score);
    if (best)
        for (unsigned y=0;y<rows;++y) sse42_numeric_store16((old+(size_t)y*w),values[best-1][y]);
    return best;
}

static QLI_SSE42_TARGET unsigned sse42_fit_tile_word(const uint8_t *p,uint8_t *old,unsigned w,unsigned rows,const FitWeights *fit)
{
    return sse42_fit_tile_kernel(p,old,w,rows,fit,1);
}

static QLI_SSE42_TARGET unsigned sse42_fit_tile_wide(const uint8_t *p,uint8_t *old,unsigned w,unsigned rows,const FitWeights *fit)
{
    return sse42_fit_tile_kernel(p,old,w,rows,fit,0);
}
#endif
static unsigned fit_tile_scalar(const uint8_t *p,uint8_t *old,unsigned w,unsigned rows,
                                const int8_t fit[2][10],const uint8_t price[256])
{
    uint8_t values[2][256];
    unsigned score[3]={0};
    for (unsigned y=0;y<rows;++y) {
        for (unsigned x=0;x<16;++x) {
            size_t i=(size_t)y*w+x;
            int tap[10];
            fit_load(p+i,w,tap);
            score[0]+=price[old[i]];
            for (unsigned k=0;k<2;++k) {
                int sum=16;
                for (unsigned t=0;t<10;++t) sum+=tap[t]*fit[k][t];
                unsigned prediction=sum<0 ? 0u : sum>=8192 ? 255u : (unsigned)sum>>5;
                uint8_t e=(uint8_t)(p[i]-prediction);
                values[k][16*y+x]=e;
                score[1+k]+=price[e];
            }
        }
    }
    unsigned best=fit_pick(score);
    if (best)
        for (unsigned y=0;y<rows;++y) memcpy(old+(size_t)y*w,values[best-1]+16*y,16);
    return best;
}

static unsigned fitted_tiles(NumericScratch *s,unsigned w,unsigned h)
{
    fit_vectors(s,w,h);
    uint8_t price[256];
#ifdef QLI_NUMERIC_SSE42
    FitWeights weights;
    FitScorer *score=NULL;
    if (s->vector) {
        QLI_NUMERIC_FN(s->vector, fit_weights)(&weights,s->fit);
        score=weights.word ? QLI_NUMERIC_FN(s->vector, fit_tile_word) :
                             QLI_NUMERIC_FN(s->vector, fit_tile_wide);
    } else
#endif
    for (unsigned e=0;e<256;++e) price[e]=(uint8_t)(1+2*bit_width(wrapped_magnitude(e)));
    unsigned stride=(w+15)/16, count=0;
    for (unsigned y=16;y<h;y+=16) {
        unsigned rows=h-y<16 ? h-y : 16;
        for (unsigned x=16;x+18<=w;x+=16) {
            size_t at=(size_t)y*w+x;
            unsigned r=0;
            for (;r<rows;++r) {
                size_t start=at+(size_t)r*w;
                if (flat_at(s,start) || flat_at(s,start+15)) break;
            }
            if (r!=rows) continue;
            unsigned pick;
#ifdef QLI_NUMERIC_SSE42
            if (score) pick=score(s->plane+at,s->error+at,w,rows,&weights);
            else
#endif
            pick=fit_tile_scalar(s->plane+at,s->error+at,w,rows,s->fit,price);
            if (pick) {
                s->mode[(size_t)(y/16)*stride+x/16]=(uint8_t)(7+pick);
                ++count;
            }
        }
    }
    return count;
}

static int numeric(NumericOutput *out,NumericScratch *s,unsigned w,unsigned h)
{
#ifdef QLI_NUMERIC_SSE42
    if (s->vector) QLI_NUMERIC_FN(s->vector, numeric_modes_vector)(s->plane,s->error,s->mode,w,h);
    else
#endif
    numeric_modes(s->plane,s->error,s->mode,w,h);
    size_t nflat=flat_spans(s);
    s->fitted=s->logical && fitted_tiles(s,w,h);
    unsigned mode_bits=s->fitted ? 4u : 3u;
    size_t bytes=(mode_bits*s->modes+7)/8;
    memset(s->map,0,bytes);
    for (size_t i=0;i<s->modes;++i) {
        size_t position=mode_bits*i;
        unsigned bit=(unsigned)(position%8), value=(unsigned)s->mode[i]<<bit;
        s->map[position/8]|=(uint8_t)value;
        if (bit+mode_bits>8) s->map[position/8+1]|=(uint8_t)(value>>8);
    }
    if (!map_record(out,s->map,bytes) ||
        (s->fitted && !append(out,s->fit,sizeof(s->fit)))) return 0;
    uint8_t head[20]={0};
    qli_write_le32(head,(uint32_t)s->pixels);
    qli_write_le32(head+4,(uint32_t)s->blocks);
    qli_write_le32(head+8,(uint32_t)nflat);
    qli_write_le32(head+12,11);
    size_t start=out->used;
    if (!append(out,head,sizeof(head)) || !append(out,s->flat,(s->blocks+7)/8) ||
        !append(out,s->constant,nflat)) return 0;
    size_t payload=out->used;
    if (!banks(out,s,w,h)) return 0;
    qli_write_le32(out->data+start+16,(uint32_t)(out->used-payload));
    return 1;
}

static unsigned plane_sample(const uint8_t *rgba,unsigned c)
{
    static const uint8_t order[]={1,0,2,3};
    unsigned value=rgba[order[c]];
    return c==1 || c==2 ? (value+128-rgba[1])&255 : value;
}

enum {
    GREEN_HALF_RED=8, BLUE_HALF_GREEN, GREEN_HALF_BLUE, RED_HALF_GREEN,
    RED_GREEN_CENTER, BLUE_FROM_CENTER, BLUE_FROM_AVERAGE, COLOR_SAMPLES
};

static const uint8_t rgb_basis[7][3] = {
    {0,1,2}, {4,5,2}, {6,7,1},
    {4,GREEN_HALF_RED,BLUE_HALF_GREEN}, {6,GREEN_HALF_BLUE,RED_HALF_GREEN},
    {RED_GREEN_CENTER,5,BLUE_FROM_CENTER}, {0,1,BLUE_FROM_AVERAGE},
};

static inline unsigned red_green_center(const uint8_t *p)
{
    unsigned difference=(p[1]+128u-p[0])&255u;
    return (p[0]+((difference+1)>>1)-64u)&255u;
}

static inline unsigned chain_sample(const uint8_t *p,unsigned c)
{
    switch (c) {
    case 4: return p[0];
    case 5: return (p[1]+128u-p[0])&255u;
    case 6: return p[2];
    case 7: return (p[1]+128u-p[2])&255u;
    case GREEN_HALF_RED: return (p[1]+128u-((p[0]+1u)>>1))&255u;
    case BLUE_HALF_GREEN: return (p[2]+128u-((p[1]+1u)>>1))&255u;
    case GREEN_HALF_BLUE: return (p[1]+128u-((p[2]+1u)>>1))&255u;
    case RED_HALF_GREEN: return (p[0]+128u-((p[1]+1u)>>1))&255u;
    case RED_GREEN_CENTER: return red_green_center(p);
    case BLUE_FROM_CENTER: return (p[2]+128u-red_green_center(p))&255u;
    case BLUE_FROM_AVERAGE: return (p[2]+128u-((p[0]+p[1]+1u)>>1))&255u;
    default: return plane_sample(p,c);
    }
}

#ifdef QLI_NUMERIC_AVX2
typedef struct { __m128i a,b,c,d; } RgbaBlock;

static QLI_NUMERIC_TARGET inline __m128i rgba_channel_mask(unsigned channel, unsigned step)
{
    return _mm_setr_epi8((char)channel,(char)(channel+step),(char)(channel+2*step),(char)(channel+3*step),
                             -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1);
}

static QLI_NUMERIC_TARGET inline __m128i rgba_extract(RgbaBlock pixels,__m128i mask)
{
    const __m128i ab=_mm_unpacklo_epi32(_mm_shuffle_epi8(pixels.a,mask),
                                               _mm_shuffle_epi8(pixels.b,mask));
    const __m128i cd=_mm_unpacklo_epi32(_mm_shuffle_epi8(pixels.c,mask),
                                               _mm_shuffle_epi8(pixels.d,mask));
    return _mm_unpacklo_epi64(ab,cd);
}
static QLI_NUMERIC_TARGET unsigned plane_blocks(const uint8_t *rgba,uint8_t *plane,
                                                   size_t n,unsigned channel,unsigned first,unsigned step)
{
    size_t i=0;
    static const uint8_t components[8]={1,0,2,3,0,1,2,1};
    static const uint8_t references[8]={0,1,1,0,0,0,0,2};
    const int difference=channel==1 || channel==2 || channel==5 || channel==7;
    const __m128i mask=rgba_channel_mask(components[channel],step);
    const __m128i reference=rgba_channel_mask(references[channel],step);
    const __m128i bias=_mm_set1_epi8((char)128);
    const __m128i base=_mm_set1_epi8((char)first);
    __m128i changed=_mm_setzero_si128();
    for (;n-i>=16;i+=16) {
        const uint8_t *p=rgba+step*i;
        const RgbaBlock pixels={
            numeric_load16(p),numeric_load16(p+16),numeric_load16(p+32),numeric_load16(p+48)
        };
        __m128i value=rgba_extract(pixels,mask);
        if (difference)
            value=_mm_xor_si128(_mm_sub_epi8(value,rgba_extract(pixels,reference)),bias);
        numeric_store16(plane+i,value);
        changed=_mm_or_si128(changed,_mm_xor_si128(value,base));
    }
    return !_mm_testz_si128(changed,changed);
}

static QLI_NUMERIC_TARGET unsigned rgb_plane_blocks(const uint8_t *rgb,uint8_t *const planes[3],
                                                size_t n,unsigned basis,const unsigned first[3])
{
    const __m128i red=rgba_channel_mask(0,3),green=rgba_channel_mask(1,3),blue=rgba_channel_mask(2,3);
    const __m128i zero=_mm_setzero_si128(), bias=_mm_set1_epi8((char)128);
    const __m128i initial[3]={
        _mm_set1_epi8((char)first[0]),_mm_set1_epi8((char)first[1]),_mm_set1_epi8((char)first[2])
    };
    __m128i changed[3]={zero,zero,zero};
    for (size_t i=0;i<n;i+=16) {
        const uint8_t *p=rgb+3*i;
        const __m128i a=numeric_load16(p),b=numeric_load16(p+16),c=numeric_load16(p+32);
        const RgbaBlock pixels={a,_mm_alignr_epi8(b,a,12),_mm_alignr_epi8(c,b,8),_mm_srli_si128(c,4)};
        const __m128i r=rgba_extract(pixels,red),g=rgba_extract(pixels,green),z=rgba_extract(pixels,blue);
        __m128i base,d1,d2;
        switch (basis) {
        case 1:
            base=r; d1=_mm_sub_epi8(g,r); d2=_mm_sub_epi8(z,g);
            break;
        case 2:
            base=z; d1=_mm_sub_epi8(g,z); d2=_mm_sub_epi8(r,g);
            break;
        case 3:
            base=r; d1=_mm_sub_epi8(g,_mm_avg_epu8(r,zero)); d2=_mm_sub_epi8(z,_mm_avg_epu8(g,zero));
            break;
        case 4:
            base=z; d1=_mm_sub_epi8(g,_mm_avg_epu8(z,zero)); d2=_mm_sub_epi8(r,_mm_avg_epu8(g,zero));
            break;
        case 5:
            d1=_mm_sub_epi8(g,r);
            base=_mm_add_epi8(r,_mm_sub_epi8(_mm_avg_epu8(_mm_xor_si128(d1,bias),zero),_mm_set1_epi8(64)));
            d2=_mm_sub_epi8(z,base);
            break;
        case 6:
            base=g; d1=_mm_sub_epi8(r,g); d2=_mm_sub_epi8(z,_mm_avg_epu8(r,g));
            break;
        default:
            base=g; d1=_mm_sub_epi8(r,g); d2=_mm_sub_epi8(z,g);
            break;
        }
        const __m128i color[3]={base,_mm_xor_si128(d1,bias),_mm_xor_si128(d2,bias)};
        for (unsigned c=0;c<3;++c) {
            numeric_store16(planes[c]+i,color[c]);
            changed[c]=_mm_or_si128(changed[c],_mm_xor_si128(color[c],initial[c]));
        }
    }
    return (unsigned)!_mm_testz_si128(changed[0],changed[0]) |
           ((unsigned)!_mm_testz_si128(changed[1],changed[1])<<1) |
           ((unsigned)!_mm_testz_si128(changed[2],changed[2])<<2);
}
#endif
#ifdef QLI_NUMERIC_SSE42
typedef struct { __m128i a,b,c,d; } sse42_RgbaBlock;

static QLI_SSE42_TARGET inline __m128i sse42_rgba_channel_mask(unsigned channel, unsigned step)
{
    return _mm_setr_epi8((char)channel,(char)(channel+step),(char)(channel+2*step),(char)(channel+3*step),
                             -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1);
}

static QLI_SSE42_TARGET inline __m128i sse42_rgba_extract(sse42_RgbaBlock pixels,__m128i mask)
{
    const __m128i ab=_mm_unpacklo_epi32(_mm_shuffle_epi8(pixels.a,mask),
                                               _mm_shuffle_epi8(pixels.b,mask));
    const __m128i cd=_mm_unpacklo_epi32(_mm_shuffle_epi8(pixels.c,mask),
                                               _mm_shuffle_epi8(pixels.d,mask));
    return _mm_unpacklo_epi64(ab,cd);
}
static QLI_SSE42_TARGET unsigned sse42_plane_blocks(const uint8_t *rgba,uint8_t *plane,
                                                   size_t n,unsigned channel,unsigned first,unsigned step)
{
    size_t i=0;
    static const uint8_t components[8]={1,0,2,3,0,1,2,1};
    static const uint8_t references[8]={0,1,1,0,0,0,0,2};
    const int difference=channel==1 || channel==2 || channel==5 || channel==7;
    const __m128i mask=sse42_rgba_channel_mask(components[channel],step);
    const __m128i reference=sse42_rgba_channel_mask(references[channel],step);
    const __m128i bias=_mm_set1_epi8((char)128);
    const __m128i base=_mm_set1_epi8((char)first);
    __m128i changed=_mm_setzero_si128();
    for (;n-i>=16;i+=16) {
        const uint8_t *p=rgba+step*i;
        const sse42_RgbaBlock pixels={
            sse42_numeric_load16(p),sse42_numeric_load16(p+16),sse42_numeric_load16(p+32),sse42_numeric_load16(p+48)
        };
        __m128i value=sse42_rgba_extract(pixels,mask);
        if (difference)
            value=_mm_xor_si128(_mm_sub_epi8(value,sse42_rgba_extract(pixels,reference)),bias);
        sse42_numeric_store16(plane+i,value);
        changed=_mm_or_si128(changed,_mm_xor_si128(value,base));
    }
    return !_mm_testz_si128(changed,changed);
}

static QLI_SSE42_TARGET unsigned sse42_rgb_plane_blocks(const uint8_t *rgb,uint8_t *const planes[3],
                                                size_t n,unsigned basis,const unsigned first[3])
{
    const __m128i red=sse42_rgba_channel_mask(0,3),green=sse42_rgba_channel_mask(1,3),blue=sse42_rgba_channel_mask(2,3);
    const __m128i zero=_mm_setzero_si128(), bias=_mm_set1_epi8((char)128);
    const __m128i initial[3]={
        _mm_set1_epi8((char)first[0]),_mm_set1_epi8((char)first[1]),_mm_set1_epi8((char)first[2])
    };
    __m128i changed[3]={zero,zero,zero};
    for (size_t i=0;i<n;i+=16) {
        const uint8_t *p=rgb+3*i;
        const __m128i a=sse42_numeric_load16(p),b=sse42_numeric_load16(p+16),c=sse42_numeric_load16(p+32);
        const sse42_RgbaBlock pixels={a,_mm_alignr_epi8(b,a,12),_mm_alignr_epi8(c,b,8),_mm_srli_si128(c,4)};
        const __m128i r=sse42_rgba_extract(pixels,red),g=sse42_rgba_extract(pixels,green),z=sse42_rgba_extract(pixels,blue);
        __m128i base,d1,d2;
        switch (basis) {
        case 1:
            base=r; d1=_mm_sub_epi8(g,r); d2=_mm_sub_epi8(z,g);
            break;
        case 2:
            base=z; d1=_mm_sub_epi8(g,z); d2=_mm_sub_epi8(r,g);
            break;
        case 3:
            base=r; d1=_mm_sub_epi8(g,_mm_avg_epu8(r,zero)); d2=_mm_sub_epi8(z,_mm_avg_epu8(g,zero));
            break;
        case 4:
            base=z; d1=_mm_sub_epi8(g,_mm_avg_epu8(z,zero)); d2=_mm_sub_epi8(r,_mm_avg_epu8(g,zero));
            break;
        case 5:
            d1=_mm_sub_epi8(g,r);
            base=_mm_add_epi8(r,_mm_sub_epi8(_mm_avg_epu8(_mm_xor_si128(d1,bias),zero),_mm_set1_epi8(64)));
            d2=_mm_sub_epi8(z,base);
            break;
        case 6:
            base=g; d1=_mm_sub_epi8(r,g); d2=_mm_sub_epi8(z,_mm_avg_epu8(r,g));
            break;
        default:
            base=g; d1=_mm_sub_epi8(r,g); d2=_mm_sub_epi8(z,g);
            break;
        }
        const __m128i color[3]={base,_mm_xor_si128(d1,bias),_mm_xor_si128(d2,bias)};
        for (unsigned c=0;c<3;++c) {
            sse42_numeric_store16(planes[c]+i,color[c]);
            changed[c]=_mm_or_si128(changed[c],_mm_xor_si128(color[c],initial[c]));
        }
    }
    return (unsigned)!_mm_testz_si128(changed[0],changed[0]) |
           ((unsigned)!_mm_testz_si128(changed[1],changed[1])<<1) |
           ((unsigned)!_mm_testz_si128(changed[2],changed[2])<<2);
}
#endif

static unsigned rgb_planes(const uint8_t *rgb,uint8_t *const planes[3],size_t n,unsigned basis,int vector)
{
    const uint8_t *components=rgb_basis[basis];
    const unsigned first[3]={chain_sample(rgb,components[0]),chain_sample(rgb,components[1]),
                             chain_sample(rgb,components[2])};
    unsigned changed=0;
    size_t i=0;
#ifdef QLI_NUMERIC_SSE42
    if (vector && n>=16) {
        i=n&~(size_t)15;
        changed=QLI_NUMERIC_FN(vector, rgb_plane_blocks)(rgb,planes,i,basis,first);
    }
#else
    (void)vector;
#endif
    for (;i<n;++i) {
        const uint8_t *p=rgb+3*i;
        for (unsigned c=0;c<3;++c) {
            unsigned value=chain_sample(p,components[c]);
            planes[c][i]=(uint8_t)value;
            changed|=(unsigned)(value!=first[c])<<c;
        }
    }
    return changed;
}

static QLI_INLINE unsigned plane_bytes(
    const uint8_t *rgba,uint8_t *plane,size_t n,unsigned channel,unsigned step,int vector)
{
    const unsigned first=chain_sample(rgba,channel);
    size_t i=0;
    unsigned different=0;
#ifdef QLI_NUMERIC_SSE42
    if (vector && n>=16) {
        i=n&~(size_t)15;
        different=QLI_NUMERIC_FN(vector, plane_blocks)(rgba,plane,i,channel,first,step);
    }
#else
    (void)vector;
#endif
    for (;i<n;++i) {
        const unsigned value=chain_sample(rgba+step*i,channel);
        plane[i]=(uint8_t)value;
        different|=value^first;
    }
    return different;
}

static unsigned channel(const uint8_t *rgba,uint8_t *plane,size_t n,unsigned c,unsigned step,int vector)
{
    switch (c) {
    case 0: return plane_bytes(rgba,plane,n,0,step,vector);
    case 1: return plane_bytes(rgba,plane,n,1,step,vector);
    case 2: return plane_bytes(rgba,plane,n,2,step,vector);
    default: return plane_bytes(rgba,plane,n,3,step,vector);
    }
}

static unsigned color_channel(const uint8_t *rgba,uint8_t *plane,size_t n,
                              unsigned c,unsigned basis,unsigned step,int vector)
{
    if (!basis || c==3) return channel(rgba,plane,n,c,step,vector);
    if (basis==1) {
        if (!c) return plane_bytes(rgba,plane,n,4,step,vector);
        if (c==1) return plane_bytes(rgba,plane,n,5,step,vector);
        return plane_bytes(rgba,plane,n,2,step,vector);
    }
    if (!c) return plane_bytes(rgba,plane,n,6,step,vector);
    if (c==1) return plane_bytes(rgba,plane,n,7,step,vector);
    return plane_bytes(rgba,plane,n,1,step,vector);
}

static inline unsigned color_error(const uint8_t *p,const uint8_t *north,unsigned c,unsigned step)
{
    int w=(int)chain_sample(p-step,c),n=(int)chain_sample(north,c);
    int nw=(int)chain_sample(north-step,c);
    int lo=w<n ? w : n, hi=w>n ? w : n;
    int estimate=w+n-nw;
    int pred=estimate<lo ? lo : estimate>hi ? hi : estimate;
    return ((unsigned)((int)chain_sample(p,c)-pred))&255u;
}

static inline unsigned error_cost(const uint8_t *p,const uint8_t *north,unsigned c,unsigned step)
{
    unsigned e=color_error(p,north,c,step);
    unsigned mag=e<128 ? e : 256-e;
    return bit_width(mag);
}

static unsigned score_color(const uint8_t *rgba,unsigned w,unsigned h,uint32_t score[3],unsigned step)
{
    score[0]=score[1]=score[2]=0;
    for (unsigned y=1;y<h;y+=8) {
        const uint8_t *row=rgba+step*(size_t)y*w;
        for (unsigned x=1;x<w;x+=8) {
            const uint8_t *p=row+step*x,*north=p-step*w;
            unsigned rg=error_cost(p,north,1,step), bg=error_cost(p,north,2,step);
            score[0]+=error_cost(p,north,0,step)+rg+bg;
            score[1]+=error_cost(p,north,4,step)+error_cost(p,north,5,step)+bg;
            score[2]+=error_cost(p,north,6,step)+error_cost(p,north,7,step)+rg;
        }
    }
    unsigned best=0;
    for (unsigned i=1;i<3;++i) if (score[i]<score[best]) best=i;
    unsigned gain=score[0]-score[best];
    return gain && gain*64>=score[0] ? best : 0;
}

static unsigned score_rgb_color(const uint8_t *rgb,unsigned w,unsigned h)
{
    const unsigned x_step=(w+27)/28, y_step=(h+27)/28;
    uint16_t hist[COLOR_SAMPLES][256]={{0}};
    unsigned count=0;
    for (unsigned y=1+y_step/2;y<h;y+=y_step) {
        const uint8_t *row=rgb+3*(size_t)y*w;
        for (unsigned x=1+(y/y_step)%x_step;x<w;x+=x_step) {
            const uint8_t *pixel=row+3*x;
            for (unsigned c=0;c<COLOR_SAMPLES;++c)
                if (c!=3) ++hist[c][color_error(pixel,pixel-3*(size_t)w,c,3)];
            ++count;
        }
    }
    uint32_t legacy_scores[3];
    if (count<128) return score_color(rgb,w,h,legacy_scores,3);

    double cost[COLOR_SAMPLES]={0};
    for (unsigned c=0;c<COLOR_SAMPLES;++c)
        for (unsigned v=0;v<256;++v) cost[c]-=wave_log[hist[c][v]];
    double scores[sizeof(rgb_basis)/sizeof(rgb_basis[0])];
    for (unsigned k=0;k<sizeof(rgb_basis)/sizeof(rgb_basis[0]);++k) {
        const uint8_t *c=rgb_basis[k];
        scores[k]=cost[c[0]]+cost[c[1]]+cost[c[2]];
    }
    unsigned best=3;
    for (unsigned k=4;k<sizeof(rgb_basis)/sizeof(rgb_basis[0]);++k)
        if (scores[k]<scores[best]) best=k;
    double margin=count/16.0;
    if (scores[0]-scores[best]>=margin &&
        scores[1]-scores[best]>=margin &&
        scores[2]-scores[best]>=margin) return best;
    unsigned basis=score_color(rgb,w,h,legacy_scores,3);
    return scores[basis]-scores[best]>=margin ? best : basis;
}

static size_t writer_bound(NumericScratch *s,size_t n,unsigned w,unsigned h,size_t file_cap,
                           size_t *frame_cap,size_t *work_bytes,size_t *wave_slots)
{
    s->pixels=n;
    s->blocks=(n+NUMERIC_SPAN-1)/NUMERIC_SPAN;
    s->tiles=(size_t)((w+7)/8)*((h+7)/8);
    s->modes=(size_t)((w+15)/16)*((h+15)/16);
    size_t modes=(s->modes+1)/2, maps=(s->tiles+1)/2;
    if (maps<modes) maps=modes;
    s->packet_cap=2*n+1200;
    size_t work=4*n+s->tiles*sizeof(NumericGroup)+s->modes+maps+(s->blocks+7)/8+s->blocks+n+s->packet_cap;
    size_t cap=file_cap<4*n+68 ? file_cap : 4*n+68;
    *frame_cap=cap;
    *work_bytes=work;
    const size_t legacy_models = 32 * (sizeof(RansModel) + sizeof(uint32_t));
    size_t bound=work+cap+4*maps+1200+sizeof(NumericConditional)+legacy_models+
                 4*READY_MODEL_BYTES+1200;
    size_t slots=numeric_wave_slots(w,h);
    bound+=slots*sizeof(uint32_t)+sizeof(WaveScratch)+sizeof(wave_log)+8*WAVE_MODEL_BYTES+4096;
    if (w>=34 && h>=17) bound+=sizeof(DirectionScratch)+8*DIRECTION_KEYS+4096;
    *wave_slots=slots;
    return bound;
}

size_t qli_numeric_bound(unsigned w,unsigned h,size_t file_cap)
{
    if (!w || !h || w>16384 || h>16384 || (size_t)w*h>QVP_CAP || file_cap<68) return 0;
    NumericScratch s={0};
    size_t frame,work,slots;
    return writer_bound(&s,(size_t)w*h,w,h,file_cap,&frame,&work,&slots);
}

static int encode_color(const uint8_t *rgba,size_t length,unsigned w,unsigned h,
                        QliWriterMemory *budget,size_t file_cap,uint8_t **data,
                        size_t *size,size_t *peak,int size_status)
{
    *data=NULL; *size=*peak=0;
    if (!rgba || !w || !h || w>16384 || h>16384 || (size_t)w*h>QVP_CAP) return 0;
    size_t n=(size_t)w*h;
    unsigned channels = 4;
    if (length == 3*n) channels = 3;
    if (length!=channels*n || file_cap<68) return 0;
    NumericScratch s={.packed_rgb=channels==3};
#ifdef QLI_NUMERIC_SSE42
    s.vector=qli_numeric_tier(qli_cpu_capabilities());
#endif
    size_t cap,work,slots;
    size_t bound=writer_bound(&s,n,w,h,file_cap,&cap,&work,&slots);
    size_t maps=(s.tiles+1)/2, modes=(s.modes+1)/2;
    if (maps<modes) maps=modes;
    if (bound > budget->cap) {
        heap_limit_hit = 1;
        return 0;
    }
    if (!prepare_wave_log()) return 0;
    uint32_t score[3];
    const unsigned basis=channels==3 && n>=16384 ? score_rgb_color(rgba,w,h) :
        score_color(rgba,w,h,score,channels);
    const int directional=w>=34 && h>=17;
    const size_t words_bytes = slots * sizeof(*s.words);
    uint8_t *memory = qli_writer_allocate(budget, work);
    uint8_t *frame = qli_writer_allocate(budget, cap);
    s.cond = qli_writer_allocate(budget, sizeof(*s.cond));
    s.wave = qli_writer_allocate(budget, sizeof(*s.wave));
    s.dir = directional ? qli_writer_allocate(budget, sizeof(*s.dir)) : NULL;
    s.words = qli_writer_allocate(budget, words_bytes);
    int status = 0;
    if (!memory || !frame || !s.cond || !s.wave || !s.words || (directional && !s.dir))
        goto done;

    s.plane=memory;
    s.error=s.plane+n;
    s.groups=(NumericGroup *)(s.error+n);
    s.mode=(uint8_t *)(s.groups+s.tiles);
    s.map=s.mode+s.modes;
    s.flat=s.map+maps;
    s.constant=s.flat+(s.blocks+7)/8;
    s.queue=s.constant+s.blocks;
    s.packet=s.queue+n;
    s.cue[0]=s.packet+s.packet_cap; s.cue[1]=s.cue[0]+n;
    memset(frame,0,64);
    memcpy(frame,"QAX1",4);
    qli_write_le32(frame+4,w); qli_write_le32(frame+8,h);
    frame[12]=8; frame[13]=(uint8_t)channels; frame[14]=(uint8_t)basis;
    qli_write_le64(frame+16,n);
    qli_write_le32(frame+28,64);
    NumericOutput out = {frame, 64, cap - 4, budget};
    uint8_t *planes[3]={s.plane,s.cue[0],s.cue[1]};
    unsigned changed=0;
    if (channels==3) {
        changed=rgb_planes(rgba,planes,n,basis,s.vector);
        s.cue[0]=planes[0];
        s.cue[1]=planes[1];
    }
    int logical_frame=0;
    for (unsigned c=0;c<channels;++c) {
        size_t start=out.used;
        unsigned kind=2, different;
        if (channels==3) {
            s.plane=planes[c];
            different=(changed>>c)&1u;
        } else different=color_channel(rgba,s.plane,n,c,basis,channels,s.vector);
        if (!different) {
            uint8_t v=(uint8_t)s.plane[0];
            if (!append(&out,&v,1)) goto fail;
        } else {
            size_t end=out.cap;
            if (n<out.cap-out.used) out.cap=out.used+n;
            s.reused_plane=0;
            s.diagonal=c<3;
            s.logical=s.diagonal && directional;
            int compressed=numeric(&out,&s,w,h);
            out.cap=end;
            if (compressed && out.used-start<n) {
                kind=s.logical ? 37u+(unsigned)s.logical : s.diagonal ? 20u : 4u;
                logical_frame|=s.logical;
                if (s.fitted) frame[15]|=(uint8_t)(1u<<c);
                if (c<2) capture_cue(&s);
            } else {
                if (n>out.cap-start) goto fail;
                kind=0;
                if (s.reused_plane) color_channel(rgba,s.plane,n,c,basis,4,s.vector);
                memcpy(frame+start,s.plane,n);
                out.used=start+n;
            }
        }
        qli_write_le32(frame+32+8*c,kind);
        qli_write_le32(frame+36+8*c,(uint32_t)(out.used-start));
    }
    if (logical_frame) memcpy(frame,"QFL1",4);
    qli_write_le32(frame+24,qint_crc32(rgba,length));
    qli_write_le32(frame+out.used,qint_crc32(frame,out.used));
    *data = frame;
    *size = out.used + 4;
    *peak = bound;
    frame = NULL;
    status = 1;
    goto done;
fail:
    status = size_status ? 2 : 0;
done:
    qli_writer_free(budget, s.cond, sizeof(*s.cond));
    qli_writer_free(budget, s.wave, sizeof(*s.wave));
    qli_writer_free(budget, s.words, words_bytes);
    qli_writer_free(budget, s.dir, sizeof(*s.dir));
    qli_writer_free(budget, memory, work);
    qli_writer_free(budget, frame, cap);
    assert(budget->stats.live == (status == 1 ? cap : 0));
    return status;
}

int qli_encode_numeric(const uint8_t *rgba,size_t length,unsigned w,unsigned h,size_t heap_cap,
                   size_t file_cap,uint8_t **data,size_t *size,size_t *peak)
{
    QliWriterMemory budget = {.cap = heap_cap};
    return encode_color(rgba,length,w,h,&budget,file_cap,data,size,peak,0);
}

int qli_encode_numeric_capped(const uint8_t *rgba,size_t length,unsigned w,unsigned h,size_t heap_cap,
                   size_t file_cap,uint8_t **data,size_t *size,size_t *peak)
{
    if (file_cap<68) {
        *data=NULL;
        *size=*peak=0;
        return 2;
    }
    QliWriterMemory budget = {.cap = heap_cap};
    return encode_color(rgba,length,w,h,&budget,file_cap,data,size,peak,1);
}

#undef QLI_NUMERIC_AVX2
#undef QLI_NUMERIC_SSE42
#undef QLI_NUMERIC_FN
#undef QLI_NUMERIC_TARGET

static int palette_compare(const void *left, const void *right)
{
    const uint32_t a = *(const uint32_t *)left, b = *(const uint32_t *)right;
    return (a > b) - (a < b);
}

static unsigned palette_location(uint32_t color, const uint32_t *keys, const uint16_t *used)
{
    unsigned index = (color * UINT32_C(2654435761)) >> 22;
    while (used[index] && keys[index] != color) index = (index + 1) & 1023;
    return index;
}

static void palette_promote(uint8_t *labels,unsigned w,unsigned h)
{
    unsigned end=0;
#ifdef QLI_PALETTE_X86
    const __m128i zero=_mm_setzero_si128(),one=_mm_set1_epi8(1);
    const __m128i odd=_mm_set1_epi16(-256);
    if (w>=16 && (qli_cpu_capabilities()&QLI_CPU_SSE42)) end=w&~15u;
#endif
    for (unsigned y=h;y--;) {
        uint8_t *row=labels+(size_t)y*w;
        const uint8_t *north=y ? row-w : NULL;
        for (unsigned x=w;x-->end;) {
            unsigned n=north ? north[x] : 0;
            unsigned other=(x&1) ? row[x-1] : (north && x ? north[x-1] : n);
            unsigned source=palette_exchange(other,n,0);
            row[x]=(uint8_t)palette_exchange(palette_exchange(row[x],n,0),source ? source : 1,1);
        }
#ifdef QLI_PALETTE_X86
        for (unsigned x=end;x;) {
            x-=16;
            __m128i value=palette_load16(row+x);
            __m128i n=north ? palette_load16(north+x) : zero;
            __m128i nw=_mm_slli_si128(n,1),left=_mm_slli_si128(value,1);
            if (north) nw=_mm_or_si128(nw,_mm_cvtsi32_si128(north[x ? x-1 : 0]));
            __m128i other=palette_select(nw,left,odd);
            value=palette_exchange_vector(palette_exchange_vector(value,n,zero),palette_second_vector(n,other),one);
            memcpy(row+x,&value,sizeof(value));
        }
#endif
    }
}

static uint8_t *palette_make_file(QliWriterMemory *memory, const uint8_t *symbols, size_t pixels,
                                  const uint32_t *palette, unsigned count, unsigned w, unsigned h,
                                  unsigned mode, uint32_t crc, unsigned channels, size_t *bytes)
{
    QliWriterPacket encoded;
    if (!qli_encode_byte_packet(memory, symbols, pixels, &encoded)) return NULL;
    const unsigned compressed = encoded.bytes < pixels;
    const size_t payload = compressed ? encoded.bytes : pixels;
    const size_t size = PALETTE_HEADER + channels * count + 5 + payload + 4;
    uint8_t *file = qli_writer_allocate(memory, size);
    if (file) {
        memcpy(file, channels == 3 ? "QPK3" : "QPK7", 4);
        qli_write_le32(file + 4, w);
        qli_write_le32(file + 8, h);
        file[12] = (uint8_t)count;
        file[13] = (uint8_t)(count >> 8);
        file[14] = (uint8_t)mode;
        qli_write_le32(file + 15, crc);
        for (unsigned i = 0; i < count; ++i) {
            uint8_t *entry = file + PALETTE_HEADER + channels * i;
            if (channels == 4) qli_write_le32(entry, palette[i]);
            else {
                qli_write_le16(entry, (uint16_t)palette[i]);
                entry[2] = (uint8_t)(palette[i] >> 16);
            }
        }
        uint8_t *record = file + PALETTE_HEADER + channels * count;
        record[0] = (uint8_t)compressed;
        qli_write_le32(record + 1, (uint32_t)payload);
        memcpy(record + 5, compressed ? encoded.data : symbols, payload);
        qli_write_le32(file + size - 4, qint_crc32(file, size - 4));
        *bytes = size;
    }
    qli_writer_free(memory, encoded.data, encoded.capacity);
    return file;
}

int qli_encode_palette(const uint8_t *raw, size_t raw_bytes, unsigned w, unsigned h,
                       size_t heap, size_t limit, size_t fail_at,
                       uint8_t **output, size_t *bytes, PaletteStats *stats)
{
    if (!output || !bytes || !stats || *output || *bytes) return -1;
    *stats = (PaletteStats){0};
    unsigned channels = 4;
    if (raw_bytes == 3 * (size_t)w * h) channels = 3;
    if (!raw || !palette_dimensions(w, h) || raw_bytes != channels * (size_t)w * h ||
        limit > PALETTE_FILE_CAP) return -1;

    QliWriterMemory memory = {.cap = heap, .fail_at = fail_at};
    uint32_t keys[1024] = {0}, palette[256];
    uint16_t used[1024] = {0};
    unsigned count = 0;
    const size_t pixels = (size_t)w * h;
    uint8_t *labels = qli_writer_allocate(&memory, pixels), *best = NULL, *other = NULL;
    size_t best_size = 0, other_size = 0;
    int status = -1;
    if (!labels) goto done;
    for (size_t i = 0; i < pixels; ++i) {
        const uint8_t *sample = raw + channels * i;
        uint32_t color = channels == 4 ? qli_read_le32(sample)
            : qli_read_le16(sample) | (uint32_t)sample[2] << 16;
        if (channels==3) color|=UINT32_C(0xff000000);
        const unsigned index = palette_location(color, keys, used);
        if (!used[index]) {
            if (count == 256) {
                status = 0;
                goto done;
            }
            keys[index] = palette[count] = color;
            used[index] = (uint16_t)(++count);
        }
        labels[i] = (uint8_t)(used[index] - 1);
    }
    qsort(palette, count, sizeof(*palette), palette_compare);
    uint8_t remap[256];
    for (unsigned i = 0; i < count; ++i) {
        const unsigned index = palette_location(palette[i], keys, used);
        remap[used[index] - 1] = (uint8_t)i;
    }
    for (size_t i = 0; i < pixels; ++i) labels[i] = remap[labels[i]];
    const uint32_t crc = qint_crc32(raw, raw_bytes);
    best = palette_make_file(&memory, labels, pixels, palette, count, w, h, 0, crc, channels, &best_size);
    if (!best) goto done;
    palette_promote(labels, w, h);
    other = palette_make_file(&memory, labels, pixels, palette, count, w, h, 1, crc, channels, &other_size);
    if (!other) goto done;
    if (other_size < best_size) {
        qli_writer_free(&memory, best, best_size);
        best = other;
        best_size = other_size;
        other = NULL;
    }
    if (best_size > limit) goto done;
    *output = best;
    *bytes = best_size;
    best = NULL;
    status = 1;
done:
    qli_writer_free(&memory, labels, pixels);
    qli_writer_free(&memory, best, best_size);
    qli_writer_free(&memory, other, other_size);
    *stats = memory.stats;
    return status;
}

typedef struct {
    size_t complete_bytes, emitted_streams;
    unsigned pack_kind;
} QliFastPrice;

int qli_priced_fast_below(const uint8_t *rgba, size_t input_size, uint32_t width,
                          uint32_t height, unsigned mode, size_t limit,
                          uint8_t **out, size_t *bytes, QliFastPrice *price);

int qli_priced_fast_below(const uint8_t *rgba, size_t input_size, uint32_t width,
                          uint32_t height, unsigned mode, size_t limit,
                          uint8_t **out, size_t *bytes, QliFastPrice *price)
{
    if (out) *out = NULL;
    if (bytes) *bytes = 0;
    if (price) *price = (QliFastPrice){0};
    size_t pixels;
    if (!rgba || !out || !bytes || !price || mode > FAST_LEFT ||
        !fast_shape(width, height, &pixels) || input_size != 4 * pixels)
        return -1;

    QliFastRows rows;
    if (!qli_prepare_fast_rows(rgba, width, height, mode, &rows)) return -1;
    const size_t overhead = FAST_HEADER + FAST_FOOTER;
    size_t measured = 0, packed_bytes = 0;
    uint8_t *packed = NULL;
    int result = qli_pack_below(rows.residual, rows.bytes, rows.stride,
                                limit > overhead ? limit - overhead : 0, &packed, &packed_bytes,
                                &measured, &price->pack_kind);
    if (measured) price->complete_bytes = overhead + measured;
    if (result == 1) {
        price->emitted_streams = price->pack_kind;
        if (!qli_write_fast_frame(width, height, rows.channels, mode, rows.green, NULL, 0,
                                  packed, packed_bytes, out, bytes))
            result = -1;
    }
    qli_free(packed);
    qli_release_fast_rows(&rows);
    return result;
}

static int icon_route_encode(const uint8_t *raw, size_t bytes, unsigned w, unsigned h,
                             size_t heap, size_t limit, CopyResult *out)
{
    if (w != ICON_SIDE || h != ICON_SIDE || bytes != ICON_PIXELS * 4 || !raw ||
        !heap || !limit || limit > (1u << 27) || !out ||
        out->selection.samples.file.data || out->selection.palette || out->copy ||
        !icon_hidden_color(raw, bytes))
        return qli_select_base(raw, bytes, w, h, heap, limit, out);

    QlicLiteLimits caps;
    qlic_lite_limits_default(&caps);
    caps.heap_bytes = heap;
    caps.file_bytes = limit;
    QlicLiteDescription description = {.width = w, .height = h, .bits = 8, .channels = 4, .alpha = 1};
    QlicLiteFile file = {0};
    QlicLiteStats stats = {0};
    QlicLiteStatus status = qli_encode_icon(&description, raw, bytes, (size_t)w * 4,
                                  &caps, NULL, &file, &stats);
    if (status != QLIC_LITE_OK) {
        int ok = qli_select_base(raw, bytes, w, h, heap, limit, out);
        if (ok && out->peak < stats.peak_bytes) out->peak = stats.peak_bytes;
        return ok;
    }

    memset(out, 0, sizeof(*out));
    out->selection.samples.file = file;
    out->selection.samples.dense = 1;
    out->peak = stats.peak_bytes;
    return 1;
}

static int opaque_directional(const QlicLiteFile *file)
{
    const uint8_t *data=file->data;
    if (file->bytes<68 || memcmp(data,"QFL1",4) || qli_read_le32(data+32)!=38 ||
        qli_read_le32(data+40)!=38 || qli_read_le32(data+48)!=38)
        return 0;
    return data[13]==3 || (file->bytes>=69 && qli_read_le32(data+56)==2 &&
                           qli_read_le32(data+60)==1 && data[file->bytes-5]==255);
}

static size_t numerical_price(const QlicLiteFile *file)
{
    return file->bytes
        + (file->bytes>=68 && file->data[13]==3)
        ;
}

static size_t history_file_limit(const QlicLiteFile *file, size_t pixels, size_t limit)
{
    const size_t price=numerical_price(file);
    size_t bytes = price - 1;
    if (pixels >= (1u << 17) && price >= 64 &&
        ((price <= pixels/4 && qli_read_le32(file->data+56) == 4) ||
         (price <= pixels/2 && opaque_directional(file)))) {
        unsigned divisor = 25;
        if (!memcmp(file->data,"QFL1",4)) divisor = qli_read_le32(file->data+56) == 4 ? 8 : 12;
        bytes = price + price/divisor;
    }
    return bytes < limit ? bytes : limit;
}

typedef struct {
    unsigned matches, repeated_strips, left, above;
} RepeatSample;

static uint32_t repeat_sample_pixel(const uint8_t *p,unsigned channels)
{
    if (channels==3) return qli_read_le16(p) | (uint32_t)p[2]<<16;
    return qli_read_le32(p);
}

static RepeatSample repeat_sample(const uint8_t *raw,unsigned w,unsigned h,unsigned channels)
{
    enum { ROWS = 8, STRIPS = 4, LENGTH = 8 };
    RepeatSample sample = {0};
    for (unsigned r = 0; r < ROWS; ++r) {
        size_t y = 1 + ((2*r+1)*(h-1)) / (2*ROWS);
        const uint8_t *row = raw + channels*y*w;
        for (unsigned k = 0; k < STRIPS; ++k) {
            unsigned x = 1 + ((2*k+1)*(w-LENGTH)) / (2*STRIPS);
            unsigned repeated = 0;
            for (unsigned j = 0; j < LENGTH; ++j) {
                const uint8_t *p = row + channels*(x+j);
                const uint32_t v = repeat_sample_pixel(p,channels);
                unsigned horizontal = v==repeat_sample_pixel(p-channels,channels);
                unsigned vertical = v==repeat_sample_pixel(p-channels*w,channels);
                repeated += horizontal | vertical;
                sample.left += horizontal;
                sample.above += vertical;
            }
            sample.matches += repeated;
            sample.repeated_strips += repeated == LENGTH;
        }
    }
    return sample;
}

static int sampled_repeats(const uint8_t *raw, unsigned w, unsigned h, unsigned channels,
                           int low_rate, int faster_history, int allow_column_first, unsigned *orientation)
{
    const RepeatSample s = repeat_sample(raw,w,h,channels);
    const unsigned left = s.left, above = s.above;
    *orientation = (above >= 224 && above >= left + 8) ||
                   (faster_history && above >= 176 && 4*above >= 3*left);
    const int row_candidate = 5*s.matches >= 4*256 || (low_rate && s.repeated_strips >= 8);
    if (!row_candidate && allow_column_first && above>=176 && above>=2*left) {
        *orientation = 2;
        return 1;
    }
    return row_candidate;
}

static void qli_free_rgba(HistoryFile *out)
{
    if (!out) return;
    qli_free_base(&out->base);
    free(out->history);
    memset(out, 0, sizeof(*out));
}

static int history_base_encode(const uint8_t *raw,
                             size_t bytes, unsigned w, unsigned h, size_t heap,
                             size_t limit, HistoryFile *out)
{
    if (!out || out->history || !raw || !w || !h || w > 16384 || h > 16384) return 0;
    const size_t n=(size_t)w*h;
    unsigned channels=4;
    if (bytes==3*n) channels=3;
    if (bytes!=channels*n) return 0;
    CopyResult *base = &out->base;
    if (!icon_route_encode(raw, bytes, w, h, heap, limit, base)) return 0;

    const QlicLiteFile *file = &base->selection.samples.file;
    out->peak = base->peak;
    if (base->copy || base->selection.palette || !file->data || file->bytes <= 4 || numerical_price(file) > (1u << 16) ||
        base->peak >= heap || (memcmp(file->data, "QAX1", 4) && memcmp(file->data, "QFL1", 4))) return 1;
    const size_t history_cap = history_file_limit(file,n,limit);
    const size_t price=numerical_price(file);
    unsigned orientation = 0;
    const int eligible = n <= 1024 ? 4*price >= 3*n :
                         n <= (1u << 22) && w >= 16 && h >= 8 &&
                         sampled_repeats(raw,w,h,channels,price<=n/2,history_cap>=price,
                                         history_cap>=price && opaque_directional(file),&orientation);
    if (!eligible) return 1;
    out->try_transpose = orientation != 0;
    if (orientation == 2) return 1;

    uint8_t *candidate = NULL;
    size_t size = 0;
    PaletteStats stats = {0};
    const size_t available = heap - base->peak;
    int status = qli_encode_history(raw, bytes, w, h, available, history_cap,
                                      0, &candidate, &size, &stats);
    if (stats.peak <= available) out->peak = base->peak + stats.peak;
    if (status != 1 || !candidate || size < 4 || size > history_cap ||
        qli_opaque_file_price(candidate,size)>history_cap ||
        stats.peak > available || stats.live != size) {
        if (candidate) free(candidate);
        return 1;
    }
    qli_free_base(base);
    out->history = candidate;
    out->bytes = size;
    return 1;
}

static int affine_route_encode(const uint8_t *raw,
                             size_t bytes, unsigned w, unsigned h, size_t heap,
                             size_t limit, HistoryFile *out)
{
    if (!history_base_encode(raw, bytes, w, h, heap, limit, out)) return 0;
    if ((size_t)w * h > QAF_PIXEL_CAP || out->peak >= heap) return 1;

    const CopyResult *base = &out->base;
    size_t before = out->history ? out->bytes : base->copy ? base->bytes :
                    base->selection.palette ? base->selection.palette_size : base->selection.samples.file.bytes;
    before=qli_opaque_file_price(out->history ? out->history : base->copy ? base->copy :
        base->selection.palette ? base->selection.palette : base->selection.samples.file.data,before);
    if (before <= QAF_HEADER + QAF_RECORD + QAF_CRC) return 1;
    size_t cap = before - 1;
    if (cap > QAF_MAX_FILE) cap = QAF_MAX_FILE;
    if (cap > heap - out->peak) cap = heap - out->peak;
    uint8_t scratch[QAF_MAX_FILE];
    unsigned channels=4;
    if (bytes==3*(size_t)w*h) channels=3;
    size_t size = qaf_pack(raw, w, h, scratch, cap, channels);
    if (!size) return 1;
    uint8_t *file = malloc(size);
    if (!file) return 1;
    memcpy(file, scratch, size);
    size_t peak = out->peak + size;
    qli_free_rgba(out);
    out->history = file;
    out->bytes = size;
    out->peak = peak;
    return 1;
}

static int history_route_encode_base(const uint8_t *raw,
                             size_t bytes, unsigned w, unsigned h, size_t heap,
                             size_t limit, HistoryFile *out)
{
    if (!affine_route_encode(raw,bytes,w,h,heap,limit,out)) return 0;
    const size_t virtual_bytes=4*(size_t)w*h;
    if (!out->try_transpose || out->history ||
        virtual_bytes <= 4096 || virtual_bytes > (1u << 22) || out->peak > heap ||
        bytes >= heap - out->peak) return 1;
    const QlicLiteFile *base = &out->base.selection.samples.file;
    if (!base->data || base->bytes <= 9 || (memcmp(base->data,"QAX1",4) && memcmp(base->data,"QFL1",4))) return 1;

    const size_t history_cap = history_file_limit(base,(size_t)w*h,limit);
    if (history_cap <= 8) return 1;
    uint8_t *rotated = malloc(bytes);
    if (!rotated) return 1;
    const size_t available = heap - out->peak;
    const size_t trial_heap = available - bytes;
    if (bytes!=virtual_bytes) transpose_rgb(rotated,raw,w,h);
    else
        transpose_copy(rotated,raw,w,h);
    uint8_t *child = NULL;
    size_t size = 0;
    PaletteStats stats = {0};
    int status = qli_encode_history(rotated,bytes,h,w,trial_heap,history_cap-8,
                                      0,&child,&size,&stats);
    free(rotated);
    size_t trial_peak = bytes + (stats.peak <= trial_heap ? stats.peak : trial_heap);
    size_t minimum=61;
    if (bytes!=virtual_bytes) minimum=52;
    if (status != 1 || !child || size < minimum || size > history_cap-8 ||
        qli_opaque_file_price(child,size)>history_cap-8 ||
        stats.peak > trial_heap || stats.live != size || size*2+8 > available) {
        if (child) free(child);
        out->peak += trial_peak;
        return 1;
    }
    uint8_t *frame = malloc(size+8);
    if (!frame) {
        free(child);
        out->peak += trial_peak;
        return 1;
    }
    if (trial_peak < 2*size+8) trial_peak = 2*size+8;
    memcpy(frame,"QCTH",4);
    memcpy(frame+4,child,size);
    uint32_t crc = qint_crc32(frame,size+4);
    memcpy(frame+size+4,&crc,4);
    free(child);
    const size_t peak = out->peak+trial_peak;
    qli_free_rgba(out);
    out->history = frame;
    out->bytes = size+8;
    out->peak = peak;
    return 1;
}


static int qli_encode_rgba(const uint8_t *raw,
                             size_t bytes,unsigned w,unsigned h,size_t heap,
                             size_t limit,HistoryFile *out)
{
    if (!history_route_encode_base(raw,bytes,w,h,heap,limit,out)) return 0;
    CopyResult *base = &out->base;
    const QlicLiteFile *file = &base->selection.samples.file;
    const size_t pixels = (size_t)w*h;
    unsigned channels=4;
    if (bytes==3*pixels) channels=3;
    if (out->history || base->copy || base->selection.palette || !file->data ||
        file->bytes < 4 || memcmp(file->data,"QFL1",4) || out->peak >= heap ||
        pixels < 65536 || pixels > 1048576 || w < 16 || h < 8 ||
        4*numerical_price(file) > 3*pixels) return 1;

    const RepeatSample sample = repeat_sample(raw,w,h,channels);
    if (sample.matches < 128 && sample.repeated_strips < 8) return 1;
    QlicLiteLimits caps;
    qlic_lite_limits_default(&caps);
    caps.heap_bytes = heap-out->peak;
    const size_t price=numerical_price(file);
    caps.file_bytes = price+price/12;
    if (caps.file_bytes > limit) caps.file_bytes = limit;
    const QlicLiteDescription desc = {.width=w,.height=h,.bits=8,.channels=channels,.alpha=channels==4};
    QlicLiteFile trial = {0};
    QlicLiteStats stats = {0};
    const QlicLiteStatus status = qli_encode_structure(&desc,raw,bytes,(size_t)w*channels,
                                                       &caps,NULL,&trial,&stats);
    if (stats.peak_bytes <= caps.heap_bytes) out->peak += stats.peak_bytes;
    if (status != QLIC_LITE_OK || !trial.data || trial.bytes < 4 ||
        trial.bytes > caps.file_bytes || stats.peak_bytes > caps.heap_bytes ||
        stats.retained_bytes < trial.bytes ||
        (memcmp(trial.data,"QWI1",4) && memcmp(trial.data,"QWNL",4) &&
         memcmp(trial.data,"QWT1",4) && memcmp(trial.data,"QEP1",4))) {
        qlic_lite_file_free(&trial);
        return 1;
    }
    const size_t peak = out->peak;
    qli_free_rgba(out);
    out->base.selection.samples.file = trial;
    out->base.selection.samples.dense = 1;
    out->base.peak = out->peak = peak;
    return 1;
}

typedef enum {
    SAMPLE_SMALL, SAMPLE_PALETTE, SAMPLE_REGULAR, SAMPLE_RAW, SAMPLE_DENSE
} SampleRoute;

static void sample_route_values(const uint8_t *p, int values[4])
{
    values[0] = p[1];
    values[1] = (p[0] - p[1] + 128) & 255;
    values[2] = (p[2] - p[1] + 128) & 255;
    values[3] = p[3];
}

static SampleRoute sample_route(const uint8_t *raw, unsigned w, unsigned h, size_t pixels)
{
    if (pixels <= 4096) return SAMPLE_SMALL;
    uint32_t keys[1024] = {0};
    uint8_t occupied[1024] = {0};
    unsigned colors = 0;
    for (size_t i = 0; i < pixels; ++i) {
        uint32_t key = qli_read_le32(raw + 4 * i);
        unsigned slot = (key * UINT32_C(2654435761)) >> 22;
        while (occupied[slot] && keys[slot] != key) slot = (slot + 1) & 1023u;
        if (!occupied[slot]) {
            occupied[slot] = 1;
            keys[slot] = key;
            if (++colors == 257) break;
        }
    }
    if (colors <= 256) return SAMPLE_PALETTE;

    uint32_t values[4][256] = {{0}}, errors[4][256] = {{0}};
    unsigned samples = 0;
    for (unsigned cy = 0; cy < 4; ++cy) {
        unsigned y0 = h * cy / 4, y1 = h * (cy + 1) / 4;
        unsigned sh = y1 - y0 < 16 ? y1 - y0 : 16;
        unsigned sy = y0 + (y1 - y0 - sh) / 2;
        for (unsigned cx = 0; cx < 4; ++cx) {
            unsigned x0 = w * cx / 4, x1 = w * (cx + 1) / 4;
            unsigned sw = x1 - x0 < 16 ? x1 - x0 : 16;
            unsigned sx = x0 + (x1 - x0 - sw) / 2;
            for (unsigned y = sy; y < sy + sh; ++y) {
                for (unsigned x = sx; x < sx + sw; ++x) {
                    const uint8_t *p = raw + 4 * ((size_t)y * w + x);
                    int v[4], l[4] = {0}, u[4] = {0}, corner[4] = {0};
                    sample_route_values(p, v);
                    if (x) sample_route_values(p - 4, l);
                    if (y) sample_route_values(p - 4 * (size_t)w, u);
                    if (x && y) sample_route_values(p - 4 * ((size_t)w + 1), corner);
                    for (unsigned c = 0; c < 4; ++c) {
                        int lo = l[c] < u[c] ? l[c] : u[c];
                        int hi = l[c] > u[c] ? l[c] : u[c];
                        int p0 = l[c] + u[c] - corner[c];
                        int prediction = p0 < lo ? lo : p0 > hi ? hi : p0;
                        ++values[c][p[c]];
                        ++errors[c][(v[c] - prediction) & 255];
                    }
                    ++samples;
                }
            }
        }
    }
    uint64_t raw_pairs[4] = {0}, error_pairs[4] = {0};
    unsigned alpha_values = 0;
    for (unsigned c = 0; c < 4; ++c) {
        for (unsigned v = 0; v < 256; ++v) {
            uint32_t count = values[c][v], error = errors[c][v];
            if (count) raw_pairs[c] += (uint64_t)count * (count - 1);
            if (error) error_pairs[c] += (uint64_t)error * (error - 1);
            if (c == 3) alpha_values += count != 0;
        }
    }
    if (samples < 256) return SAMPLE_DENSE;
    uint64_t pairs = (uint64_t)samples * (samples - 1);
    const unsigned order[3] = {1, 0, 2};
    int regular = 1, noise = 1;
    for (unsigned c = 0; c < 3; ++c) {
        regular &= 8 * error_pairs[c] >= 7 * pairs;
        noise &= 8 * error_pairs[c] <= 9 * raw_pairs[order[c]];
    }
    if (alpha_values > 1) noise &= 8 * error_pairs[3] <= 9 * raw_pairs[3];
    return regular ? SAMPLE_REGULAR : noise ? SAMPLE_RAW : SAMPLE_DENSE;
}

static int write_small_sample(const uint8_t *raw, unsigned w, unsigned h,
                               uint8_t **file, size_t *size)
{
    if (!fast_encode_alloc(raw, w, h, FAST_AUTO, file, size)) return 0;
    uint8_t *simple = NULL;
    size_t bytes = 0;
    if (!qps_encode_alloc(raw, w, h, &simple, &bytes)) {
        qli_free(*file);
        *file = NULL;
        *size = 0;
        return 0;
    }
    if (bytes < *size) {
        qli_free(*file);
        *file = simple;
        *size = bytes;
    } else {
        qli_free(simple);
    }
    return 1;
}

static int write_dense_sample(const uint8_t *raw, size_t bytes, unsigned w, unsigned h,
                               uint8_t **out, size_t *size)
{
    QliDenseImage *image = NULL;
    QliDenseFit *fit = NULL;
    QliDenseField *selected = NULL, *field = NULL;
    uint8_t *best = NULL, *trial = NULL;
    size_t best_size = SIZE_MAX, selected_size = SIZE_MAX, trial_size = 0;
    int ok = 0;
    if (!qli_dense_prepare(raw, bytes, w, h, &image) || !qli_dense_fit_sample(image, &fit))
        goto done;
    for (unsigned i = 0; i < 3; ++i) {
        QliDenseFit view = { .source = image, .state = {fit->state[i]}, .count = 1 };
        if (!qli_dense_field_create(&view, &field) ||
            !qli_dense_write_tiles(field, &trial, &trial_size))
            goto done;
        if (trial_size < selected_size) {
            qli_dense_field_free(selected);
            selected = field;
            selected_size = trial_size;
            field = NULL;
        }
        qli_dense_field_free(field);
        field = NULL;
        if (trial_size < best_size) {
            qli_free(best);
            best = trial;
            best_size = trial_size;
            trial = NULL;
        }
        qli_free(trial);
        trial = NULL;
    }
    qli_dense_fit_free(fit);
    fit = NULL;
    qli_dense_image_free(image);
    image = NULL;
    if (!qli_dense_finish_sample(selected, best_size, &trial, &trial_size)) goto done;
    if (trial) {
        qli_free(best);
        best = trial;
        best_size = trial_size;
        trial = NULL;
    }
    *out = best;
    *size = best_size;
    best = NULL;
    ok = 1;
done:
    qli_free(best);
    qli_free(trial);
    qli_dense_field_free(selected);
    qli_dense_field_free(field);
    qli_dense_fit_free(fit);
    qli_dense_image_free(image);
    return ok;
}

int qli_encode_sample_core(const uint8_t *raw, size_t bytes, unsigned w, unsigned h,
                            uint8_t **file, size_t *size)
{
    if (file) *file = NULL;
    if (size) *size = 0;
    size_t pixels;
    if (!file || !size || !raw || !fast_shape(w, h, &pixels) || bytes != 4 * pixels) return 0;
    int solid = qli_try_solid(raw, bytes, w, h, file, size);
    if (solid) return solid > 0;
    SampleRoute route = sample_route(raw, w, h, pixels);
    if (route == SAMPLE_RAW) return fast_encode_alloc(raw, w, h, FAST_RAW, file, size);

    uint8_t *best = NULL, *trial = NULL;
    size_t best_size = 0, trial_size = 0;
    int ok = route == SAMPLE_DENSE ? write_dense_sample(raw, bytes, w, h, &best, &best_size) :
                                    write_small_sample(raw, w, h, &best, &best_size);
    if (!ok) goto fail;
    if (route == SAMPLE_DENSE) {
        for (unsigned mode = FAST_RAW; mode <= FAST_LEFT; ++mode) {
            QliFastPrice price;
            int priced = qli_priced_fast_below(raw, bytes, w, h, mode, best_size,
                                               &trial, &trial_size, &price);
            if (priced < 0) goto fail;
            if (priced == 1) {
                qli_free(best);
                best = trial;
                best_size = trial_size;
                trial = NULL;
            }
        }
    } else if (route == SAMPLE_PALETTE || route == SAMPLE_REGULAR) {
        if (!write_dense_sample(raw, bytes, w, h, &trial, &trial_size)) goto fail;
        if (trial_size < best_size) {
            qli_free(best);
            best = trial;
            best_size = trial_size;
            trial = NULL;
        }
        qli_free(trial);
    }
    *file = best;
    *size = best_size;
    return 1;
fail:
    qli_free(best);
    qli_free(trial);
    return 0;
}

typedef struct {
    uint8_t *next;
    uint32_t pending;
    unsigned bits;
} SampleBitWriter;

static void write_sample_bits(SampleBitWriter *writer, unsigned value, unsigned bits)
{
    writer->pending |= (uint32_t)value << writer->bits;
    writer->bits += bits;
    while (writer->bits >= 8) {
        *writer->next++ = (uint8_t)writer->pending;
        writer->pending >>= 8;
        writer->bits -= 8;
    }
}

static unsigned read_sample(const uint8_t *sample, unsigned bytes_per_sample)
{
    return bytes_per_sample == 1 ? sample[0] : qli_read_le16(sample);
}

static void extract_rgba(const QlicLiteDescription *d, const uint8_t *samples, size_t stride,
                         unsigned bytes_per_sample, unsigned low_bits, uint8_t *rgba)
{
    unsigned step = d->channels * bytes_per_sample;
    for (uint32_t y = 0; y < d->height; ++y) {
        const uint8_t *sample = samples + (size_t)y * stride;
        for (uint32_t x = 0; x < d->width; ++x, sample += step, rgba += 4) {
            if (d->channels <= 2) {
                rgba[0] = rgba[1] = rgba[2] =
                    (uint8_t)(read_sample(sample, bytes_per_sample) >> low_bits);
                rgba[3] = d->channels == 2
                    ? (uint8_t)(read_sample(sample + bytes_per_sample, bytes_per_sample) >> low_bits)
                    : 255;
            } else {
                for (unsigned c = 0; c < 3; ++c)
                    rgba[c] = (uint8_t)(read_sample(sample + c * bytes_per_sample,
                                                   bytes_per_sample) >> low_bits);
                rgba[3] = d->channels == 4
                    ? (uint8_t)(read_sample(sample + 3 * bytes_per_sample, bytes_per_sample) >> low_bits)
                    : 255;
            }
        }
    }
}

static void copy_sample_rows(uint8_t *out, const uint8_t *in, size_t row_bytes,
                             size_t stride, uint32_t rows)
{
    if (row_bytes == stride) {
        memcpy(out, in, row_bytes * rows);
        return;
    }
    for (uint32_t y = 0; y < rows; ++y)
        memcpy(out + y * row_bytes, in + y * stride, row_bytes);
}

QlicLiteStatus qli_write_sample_image(const QlicLiteDescription *d, const uint8_t *samples,
                                 size_t stride, const QlicLiteLimits *limits,
                                 uint8_t **output, size_t *written, QliSampleCore *encode)
{
    const size_t pixels = (size_t)d->width * d->height;
    const unsigned bytes_per_sample = d->bits == 8 ? 1 : 2;
    const unsigned low_bits = d->bits - 8;
    const int rgba8 = d->bits == 8 && d->channels == 4;
    const int bare = rgba8 && d->alpha == 1 &&
                     !d->authority && !d->has_mastering && !d->has_light;
    const uint8_t *rgba = samples;
    uint8_t *scratch = NULL, *upper = NULL, *result = NULL;
    size_t upper_size = 0;
    QlicLiteStatus status = QLIC_LITE_INTERNAL;
    if (!rgba8 || stride != (size_t)d->width * 4) {
        scratch = qli_malloc(pixels * 4);
        if (!scratch) return QLIC_LITE_ALLOCATOR;
        if (rgba8) copy_sample_rows(scratch, samples, (size_t)d->width * 4, stride, d->height);
        else extract_rgba(d, samples, stride, bytes_per_sample, low_bits, scratch);
        rgba = scratch;
    }
    int ok = encode(rgba, pixels * 4, d->width, d->height, &upper, &upper_size);
    qli_free(scratch);
    if (!ok) goto done;
    if (bare) {
        *output = upper;
        *written = upper_size;
        upper = NULL;
        status = QLIC_LITE_OK;
        goto done;
    }

    QlhLimits description_limits = qli_description_limits(limits);
    size_t descriptor_size = 0;
    int checked = qlh_record_size(d, &description_limits, &descriptor_size);
    if (checked != QLH_OK) {
        status = checked == QLH_LIMIT ? QLIC_LITE_LIMIT : QLIC_LITE_INVALID;
        goto done;
    }
    const size_t count = pixels * d->channels;
    const size_t full_samples = qli_packed_bytes(count, d->bits);
    const size_t low_samples = qli_packed_bytes(count, low_bits);
    const int use_core = upper_size < full_samples && low_samples < full_samples - upper_size;
    const size_t core_size = use_core ? upper_size : 0;
    const size_t tail_size = use_core ? low_samples : full_samples;
    if (!use_core) {
        qli_free(upper);
        upper = NULL;
    }
    const size_t total = 28 + descriptor_size + core_size + tail_size;
    if (total > limits->file_bytes || total > ((size_t)1 << 28)) {
        status = QLIC_LITE_LIMIT;
        goto done;
    }
    result = qli_malloc(total);
    if (!result) {
        status = QLIC_LITE_ALLOCATOR;
        goto done;
    }
    size_t descriptor_written = 0;
    checked = qlh_write(d, &description_limits, result + 24, descriptor_size, &descriptor_written);
    if (checked != QLH_OK || descriptor_written != descriptor_size) goto done;
    memcpy(result, "QHI1", 4);
    result[4] = 1;
    result[5] = (uint8_t)use_core;
    result[6] = result[7] = 0;
    qli_write_le32(result + 8, (uint32_t)descriptor_size);
    qli_write_le32(result + 12, (uint32_t)core_size);
    qli_write_le32(result + 16, (uint32_t)tail_size);
    qli_write_le32(result + 20, 0);
    uint8_t *core = result + 24 + descriptor_size;
    if (core_size) memcpy(core, upper, core_size);
    unsigned bits = use_core ? low_bits : d->bits;
    size_t row_values = (size_t)d->width * d->channels;
    if (bits == 8) {
        copy_sample_rows(core + core_size, samples, row_values, stride, d->height);
    } else if (bits) {
        SampleBitWriter writer = {core + core_size, 0, 0};
        unsigned mask = (1u << bits) - 1;
        for (uint32_t y = 0; y < d->height; ++y) {
            const uint8_t *sample = samples + (size_t)y * stride;
            for (size_t x = 0; x < row_values; ++x, sample += bytes_per_sample)
                write_sample_bits(&writer, read_sample(sample, bytes_per_sample) & mask, bits);
        }
        if (writer.bits) *writer.next = (uint8_t)writer.pending;
    }
    qli_write_le32(result + total - 4, qint_crc32(result, total - 4));
    *output = result;
    *written = total;
    result = NULL;
    status = QLIC_LITE_OK;
done:
    qli_free(upper);
    qli_free(result);
    return status;
}

static QlicLiteStatus encode_samples_body(const QlicLiteDescription *d, const uint8_t *samples,
                                     size_t input_size, size_t stride, const QlicLiteLimits *limits,
                                     uint8_t **output, size_t *written)
{
    (void)input_size;
    return qli_write_sample_image(d, samples, stride, limits, output, written, qli_encode_sample_core);
}

QlicLiteStatus qli_encode_samples(const QlicLiteDescription *d, const uint8_t *samples, size_t bytes,
                             size_t stride, const QlicLiteLimits *limits, const QlicLiteAllocator *allocator,
                             QlicLiteFile *file, QlicLiteStats *stats)
{
    return qli_encode_owned(d, samples, bytes, stride, limits, allocator, file, stats,
                            encode_samples_body);
}

#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42 && (defined(_M_X64) || defined(__x86_64__))
#define QLI_SCOUT_X86 1
#else
#define QLI_SCOUT_X86 0
#endif

enum { SCOUT_SLICES = 4, SCOUT_SLICE = 1024, SCOUT_SLOTS = 1024,
       SCOUT_MIN_REPEAT = 8, SCOUT_MAX_REPEAT = 128 };

static uint32_t scout_pixel(const uint8_t *image,size_t at,unsigned channels)
{
    if (channels==3) {
        const uint8_t *p=image+3*at;
        return UINT32_C(0xff000000) | qli_read_le16(p) | (uint32_t)p[2]<<16;
    }
    uint32_t value;
    memcpy(&value,image+4*at,4);
    return value;
}

static uint32_t scout_predict(const uint8_t *image,unsigned w,unsigned y,unsigned x,unsigned channels)
{
    size_t at=(size_t)y*w+x;
    uint32_t north=y ? scout_pixel(image,at-w,channels) : 0;
    uint32_t west=x ? scout_pixel(image,at-1,channels) : north;
    if (!y) north=west;
    uint32_t diagonal=x && y ? scout_pixel(image,at-w-1,channels) : north;
    return history_gradient(west,north,diagonal,4);
}

static void scout_errors(uint32_t value,uint32_t prediction,uint8_t *out)
{
    unsigned green=((value>>8)&255)-((prediction>>8)&255);
    out[0]=(uint8_t)green;
    out[1]=(uint8_t)((value&255)-(prediction&255)-green);
    out[2]=(uint8_t)(((value>>16)&255)-((prediction>>16)&255)-green);
    out[3]=(uint8_t)((value>>24)-(prediction>>24));
}

static unsigned scout_match(const uint8_t *image,size_t at,unsigned w,unsigned y,unsigned x,unsigned channels)
{
    unsigned which;
    return history_match(image,at,w,y,x,&which,channels);
}

static unsigned scout_history_slot(const uint8_t *image,size_t at,unsigned channels)
{
    return history_hash(scout_pixel(image,at,channels),scout_pixel(image,at+1,channels),
                        scout_pixel(image,at+2,channels),scout_pixel(image,at+3,channels));
}

static unsigned scout_history_length(const uint8_t *image,size_t at,size_t earlier,unsigned limit,unsigned channels)
{
    unsigned length=0;
    while (length<limit && scout_pixel(image,earlier+length,channels)==scout_pixel(image,at+length,channels))
        ++length;
    return length;
}

static void scout_structure_row(const uint8_t *row, size_t stride, int native,
                                unsigned *repeat, unsigned *sharp)
{
#if QLI_SCOUT_X86
    if (native) {
        const __m128i zero = _mm_setzero_si128(), threshold = _mm_set1_epi8(31);
        for (unsigned dx=0; dx<64; dx+=16) {
            const uint8_t *p = row+dx;
            __m128i v, west, north;
            memcpy(&v,p,sizeof(v));
            memcpy(&west,p-4,sizeof(west));
            memcpy(&north,p-stride,sizeof(north));
            const __m128i same = _mm_or_si128(_mm_cmpeq_epi32(v,west),_mm_cmpeq_epi32(v,north));
            const __m128i dw = _mm_sub_epi8(_mm_max_epu8(v,west),_mm_min_epu8(v,west));
            const __m128i dn = _mm_sub_epi8(_mm_max_epu8(v,north),_mm_min_epu8(v,north));
            const __m128i nearw = _mm_cmpeq_epi32(_mm_subs_epu8(dw,threshold),zero);
            const __m128i nearn = _mm_cmpeq_epi32(_mm_subs_epu8(dn,threshold),zero);
            *repeat += qlic_count_bits((unsigned)_mm_movemask_ps(_mm_castsi128_ps(same)));
            *sharp += 4-qlic_count_bits((unsigned)_mm_movemask_ps(_mm_castsi128_ps(_mm_or_si128(nearw,nearn))));
        }
        return;
    }
#else
    (void)native;
#endif
    const uint8_t *north = row-stride, *west = row-4;
    for (unsigned dx=0; dx<64; dx+=4) {
        unsigned dw=0, dn=0;
        for (unsigned c=dx; c<dx+4; ++c) {
            const int w = (int)row[c]-west[c], n = (int)row[c]-north[c];
            dw |= (unsigned)(w<0 ? -w : w);
            dn |= (unsigned)(n<0 ? -n : n);
        }
        *repeat += !dw || !dn;
        *sharp += dw>31 && dn>31;
    }
}

static unsigned structure_score(const uint8_t *raw, unsigned w, unsigned h, unsigned channels)
{
    if (w <= 16 || h <= 1 || (size_t)w * h < 65536) return 0;
    size_t stride=channels*(size_t)w;
    unsigned repeat=0,sharp=0;
    int native=0;
#if QLI_SCOUT_X86
    native=(qli_cpu_capabilities() & QLI_CPU_SSE42)!=0;
#endif
    for(unsigned gy=0;gy<8;++gy) {
        unsigned y=1+(2*gy+1)*(h-1)/16;
        for(unsigned gx=0;gx<8;++gx) {
            unsigned x=1+(2*gx+1)*(w-16)/16;
            const uint8_t *row=raw+(size_t)y*stride+channels*x;
            if (channels == 3) {
                for (unsigned dx=0; dx<48; dx+=3) {
                    const uint8_t *p=row+dx,*west=p-3,*north=p-stride;
                    unsigned dw=0,dn=0;
                    for (unsigned c=0; c<3; ++c) {
                        const int a=(int)p[c]-west[c],b=(int)p[c]-north[c];
                        dw|=(unsigned)(a<0 ? -a : a);
                        dn|=(unsigned)(b<0 ? -b : b);
                    }
                    repeat+=!dw || !dn;
                    sharp+=dw>31 && dn>31;
                }
                continue;
            }
            scout_structure_row(row,stride,native,&repeat,&sharp);
        }
    }
    return repeat|(sharp<<16);
}

static unsigned scout_log2(unsigned value)
{
#if defined(__GNUC__) || defined(__clang__)
    return 31u - (unsigned)__builtin_clz(value);
#else
    unsigned result = 0;
    while (value >>= 1) ++result;
    return result;
#endif
}

static unsigned scout_error_price(const uint8_t *image, unsigned w, unsigned y, unsigned x,unsigned channels)
{
    uint8_t errors[4];
    scout_errors(scout_pixel(image, (size_t)y * w + x,channels), scout_predict(image, w, y, x,channels), errors);
    unsigned bits = 0;
    for (unsigned channel = 0; channel < 4; ++channel) {
        const unsigned error = errors[channel];
        if (error) bits += 3 + 2 * scout_log2(error <= 128 ? error : 256 - error);
    }
    return bits;
}

static int scout_few_literals(const uint8_t *image, unsigned w, size_t pixels,unsigned channels)
{
    enum { WINDOWS = 16, WIDTH = 128 };
    ColorCache cache = {0};
    unsigned literals = 0;
    for (unsigned window = 0; window < WINDOWS; ++window) {
        size_t at = window * (pixels - WIDTH) / (WINDOWS - 1);
        const size_t end = at + WIDTH;
        unsigned y = (unsigned)(at / w), x = (unsigned)(at % w);
        while (at < end) {
            const size_t row = (size_t)y * w;
            const unsigned stop = end - row < w ? (unsigned)(end - row) : w;
            while (x < stop) {
                const unsigned length = scout_match(image, row + x, w, y, x,channels);
                if (length) {
                    x += length < stop - x ? length : stop - x;
                    continue;
                }
                const uint32_t value = scout_pixel(image, row + x,channels);
                const unsigned slot = history_color_slot(value);
                if (slot == CACHE - 1 || !cache.present[slot] || cache.colors[slot] != value) {
                    ++literals;
                    remember(&cache, value, 0);
                }
                ++x;
            }
            at = row + stop;
            ++y;
            x = 0;
        }
    }
    return 8 * literals <= WINDOWS * WIDTH;
}

static unsigned scout_slice_price(const uint8_t *image, unsigned w, size_t start,
                                  unsigned *covered,unsigned channels)
{
    ColorCache cache = {0};
    uint16_t positions[SCOUT_SLOTS] = {0};
    const size_t end = start + SCOUT_SLICE;
    unsigned copy = 0;
    for (size_t at = start; at < end;) {
        const unsigned y = (unsigned)(at / w), x = (unsigned)(at % w);
        unsigned bound = (unsigned)(end - at);
        if (bound > w - x) bound = w - x;
        unsigned length = scout_match(image, at, w, y, x,channels);
        if (length > bound) length = bound;
        if (bound >= SCOUT_MIN_REPEAT) {
            const unsigned key = scout_history_slot(image, at,channels) & (SCOUT_SLOTS - 1);
            const unsigned previous = positions[key];
            positions[key] = (uint16_t)(at - start + 1);
            if (previous) {
                const size_t earlier = start + previous - 1;
                const unsigned limit = bound < SCOUT_MAX_REPEAT ? bound : SCOUT_MAX_REPEAT;
                const unsigned run = scout_history_length(image, at, earlier, limit,channels);
                if (run >= length + SCOUT_MIN_REPEAT) {
                    copy += 4 + scout_log2(run) + scout_log2((unsigned)(at - earlier));
                    *covered += run;
                    at += run;
                    continue;
                }
            }
        }
        if (length) {
            copy += 3 + scout_log2(length);
            at += length;
            continue;
        }
        const uint32_t color = scout_pixel(image, at,channels);
        const unsigned slot = history_color_slot(color);
        if (slot < CACHE - 2 && cache.present[slot] && cache.colors[slot] == color) {
            copy += 8;
        } else {
            copy += 1 + scout_error_price(image, w, y, x,channels);
            remember(&cache, color, 0);
        }
        ++at;
    }
    return copy;
}

static int scout_keep_repeat(const uint8_t *image, unsigned w, size_t pixels,unsigned channels)
{
    const size_t room = pixels - SCOUT_SLICE;
    const unsigned samples = SCOUT_SLICES * SCOUT_SLICE;
    unsigned covered = 0, total_numeric = 0, total_copy = 0;
    int low = INT_MAX, high = INT_MIN;
    for (unsigned i = 0; i < SCOUT_SLICES; ++i) {
        const size_t start = room * (2 * i + 1) / (2 * SCOUT_SLICES);
        const unsigned copy = scout_slice_price(image, w, start, &covered,channels);
        unsigned numeric = 0, y = (unsigned)(start / w), x = (unsigned)(start % w);
        for (unsigned j = 0; j < SCOUT_SLICE; ++j) {
            numeric += scout_error_price(image, w, y, x,channels);
            if (++x == w) {
                ++y;
                x = 0;
            }
        }
        const int gap = (int)numeric - (int)copy;
        if (gap < low) low = gap;
        if (gap > high) high = gap;
        total_numeric += numeric;
        total_copy += copy;
    }
    if (pixels > 4194304) return 4 * covered >= samples && total_numeric <= 2 * samples;
    const int gap = 4 * ((int)total_numeric - (int)total_copy) - (high - low);
    return gap >= -672 || 4 * covered >= 3 * samples;
}

#undef QLI_SCOUT_X86

size_t qli_opaque_file_price(const uint8_t *data,size_t bytes)
{
    if (bytes >= 68 && data[13]==3 &&
        (!memcmp(data,"QAX1",4) || !memcmp(data,"QFL1",4)))
        return bytes+1;
    if (bytes >= 19 && !memcmp(data,"QPK3",4))
        return bytes+qli_read_le16(data+12);
    if (bytes >= 28 && (!memcmp(data,"QRV3",4) || !memcmp(data,"QRVP",4)))
        return bytes+qli_opaque_history_record_size(qli_read_le32(data+16));
    if (bytes >= 16 && !memcmp(data,"QAF3",4))
        return bytes+3*((bytes-16)/11);
    return bytes;
}


static void release_numeric_result(QrouteFile *result)
{
    if (result->dense) qlic_lite_file_free(&result->file);
    else if (result->file.data) free(result->file.data);
    *result = (QrouteFile){0};
}

static void release_selection(Encoded *out)
{
    release_numeric_result(&out->samples);
    if (out->palette) free(out->palette);
    *out = (Encoded){0};
}

static void try_structure(QnlEncode encode,
                          const uint8_t *raw, size_t bytes, unsigned w, unsigned h,
                          size_t heap, size_t limit, QrouteFile *result)
{
    unsigned channels=4;
    if (bytes == 3*(size_t)w*h) channels=3;
    const unsigned features = structure_score(raw, w, h, channels);
    const unsigned repeat = features & 65535, sharp = features >> 16;
    if (repeat < 512 || !sharp || 16 * sharp < 1024 - repeat) return;

    QlicLiteLimits caps;
    qlic_lite_limits_default(&caps);
    caps.heap_bytes = heap;
    caps.file_bytes = limit;
    const QlicLiteDescription description = {.width = w, .height = h, .bits = 8,
                                        .channels = channels, .alpha = channels==4};
    QlicLiteStats stats = {0};
    QlicLiteStatus status = encode(&description, raw, bytes, channels * (size_t)w, &caps,
                              NULL, &result->file, &stats);
    result->peak = stats.peak_bytes;
    const QlicLiteFile *file = &result->file;
    if (status == QLIC_LITE_OK && stats.peak_bytes <= heap && file->data &&
        file->bytes >= 4 && file->bytes <= limit &&
        (!memcmp(file->data, "QWNL", 4) || !memcmp(file->data, "QWT1", 4))) {
        result->dense = 1;
    } else {
        qlic_lite_file_free(&result->file);
    }
}

static int numeric_result_valid(const QlicLiteFile *file, size_t peak, size_t heap, size_t limit)
{
    return file->data && file->bytes >= 4 && file->bytes <= limit && peak <= heap;
}

static int select_numeric(QnlEncode structure,
                          const uint8_t *raw, size_t bytes, unsigned w, unsigned h,
                          size_t heap, size_t limit, QrouteFile *result)
{
    if (!heap || !limit) return 0;
    try_structure(structure, raw, bytes, w, h, heap, limit, result);
    if (result->dense) return 1;

    size_t peak = 0;
    if (bytes == 3*(size_t)w*h && limit <= 3*(size_t)w*h+68) --limit;
    if (!qli_encode_numeric(raw, bytes, w, h, heap, limit, &result->file.data,
                        &result->file.bytes, &peak) ||
        !numeric_result_valid(&result->file, peak, heap, limit) || result->peak > heap) {
        release_numeric_result(result);
        return 0;
    }
    if (peak > result->peak) result->peak = peak;
    return 1;
}

static int palette_result_valid(int status, const uint8_t *file, size_t bytes,
                                const PaletteStats *stats, size_t heap, size_t limit)
{
    return stats->peak <= heap && stats->live <= stats->peak &&
           (status != 1 || (file && bytes >= 4 && bytes <= limit && stats->live == bytes));
}

static int select_ordinary(const uint8_t *raw, size_t bytes,
                           unsigned w, unsigned h, size_t heap, size_t limit, Encoded *out)
{
    if (!select_numeric(qli_encode_structure, raw, bytes, w, h, heap, limit,
                        &out->samples)) return 0;
    out->peak = out->samples.peak;
    if (out->peak == heap) return 1;

    PaletteStats stats = {0};
    uint8_t *palette = NULL;
    size_t size = 0;
    const size_t remaining = heap - out->peak;
    int status = qli_encode_palette(raw, bytes, w, h, remaining, limit, 0,
                                &palette, &size, &stats);
    if (!palette_result_valid(status, palette, size, &stats, remaining, limit)) {
        if (palette) free(palette);
        release_selection(out);
        return 0;
    }
    out->peak += stats.peak;
    size_t price = status == 1 ? qli_opaque_file_price(palette,size) : SIZE_MAX;
    if (price <= limit && price < qli_opaque_file_price(out->samples.file.data,out->samples.file.bytes)) {
        release_numeric_result(&out->samples);
        out->palette = palette;
        out->palette_size = size;
    } else if (palette) {
        free(palette);
    }
    return 1;
}

static int select_bounded(const uint8_t *raw, size_t bytes,
                          unsigned w, unsigned h, size_t heap, size_t limit, Encoded *out)
{
    PaletteStats stats = {0};
    uint8_t *palette = NULL;
    size_t size = 0;
    int status = qli_encode_palette(raw, bytes, w, h, heap, limit, 0,
                                &palette, &size, &stats);
    if (!palette_result_valid(status, palette, size, &stats, heap, limit)) {
        if (palette) free(palette);
        return 0;
    }
    out->peak = stats.peak;
    size_t price = status == 1 ? qli_opaque_file_price(palette,size) : SIZE_MAX;
    if (price > limit) {
        if (palette) free(palette);
        if (!select_numeric(qli_encode_capped_structure, raw, bytes, w, h,
                            heap, limit, &out->samples)) return 0;
        if (out->samples.peak > out->peak) out->peak = out->samples.peak;
        return 1;
    }

    if (select_numeric(qli_encode_capped_structure, raw, bytes, w, h,
                       heap - stats.live, price, &out->samples)) {
        const size_t bound = stats.live + out->samples.peak;
        if (bound > out->peak) out->peak = bound;
        free(palette);
        return 1;
    }
    out->palette = palette;
    out->palette_size = size;
    
    out->peak = heap;
    return 1;
}

static int select_with_numeric_bound(const uint8_t *raw, size_t bytes,
                                     unsigned w, unsigned h, size_t heap, size_t limit,
                                     Encoded *out)
{
    if (limit < (size_t)w*h*4 + 68)
        return select_ordinary(raw, bytes, w, h, heap, limit, out);
    const size_t bound = qli_numeric_bound(w, h, limit);
    if (!bound || bound >= heap)
        return select_ordinary(raw, bytes, w, h, heap, limit, out);

    QrouteFile *file = &out->samples;
    try_structure(qli_encode_structure, raw, bytes, w, h, heap, limit, file);
    if (!file->dense && bound > file->peak) file->peak = bound;
    if (file->peak > heap) {
        release_selection(out);
        return 0;
    }
    out->peak = file->peak;
    PaletteStats stats = {0};
    uint8_t *palette = NULL;
    size_t size = 0;
    size_t price = SIZE_MAX;
    if (out->peak < heap) {
        const size_t remaining = heap - out->peak;
        const int status = qli_encode_palette(raw, bytes, w, h, remaining, limit, 0,
                                          &palette, &size, &stats);
        if (!palette_result_valid(status, palette, size, &stats, remaining, limit)) {
            if (palette) free(palette);
            release_selection(out);
            return 0;
        }
        out->peak += stats.peak;
        if (status == 1) price = qli_opaque_file_price(palette,size);
        if (price > limit && palette) {
            free(palette);
            palette = NULL;
        }
    }
    if (!file->dense) {
        size_t peak = 0;
        if (!palette) {
            if (qli_encode_numeric(raw, bytes, w, h, heap, limit, &file->file.data,
                               &file->file.bytes, &peak) &&
                numeric_result_valid(&file->file, peak, heap, limit)) return 1;
            release_selection(out);
            return 0;
        }
        const size_t remaining = heap - stats.live;
        size_t numeric_limit=price;
        if (bytes == 3*(size_t)w*h) --numeric_limit;
        const int status = qli_encode_numeric_capped(raw, bytes, w, h, remaining, numeric_limit,
                                                 &file->file.data, &file->file.bytes, &peak);
        if (status == 1 && numeric_result_valid(&file->file, peak, remaining, numeric_limit)) {
            free(palette);
            return 1;
        }
        if (status != 2) {
            free(palette);
            release_selection(out);
            return select_ordinary(raw, bytes, w, h, heap, limit, out);
        }
    } else if (!palette || price >=
                           qli_opaque_file_price(file->file.data,file->file.bytes)) {
        if (palette) free(palette);
        return 1;
    }
    const size_t peak = out->peak;
    release_selection(out);
    out->palette = palette;
    out->palette_size = size;
    out->peak = peak;
    return 1;
}

static void qli_free_base(CopyResult *out)
{
    if (!out) return;
    release_selection(&out->selection);
    if (out->copy) free(out->copy);
    *out = (CopyResult){0};
}

static int qli_select_base(const uint8_t *raw, size_t bytes,
                    unsigned w, unsigned h, size_t heap, size_t limit, CopyResult *out)
{
    if (!out || out->selection.samples.file.data || out->selection.palette ||
        out->copy || !raw || !w || !h || w > 16384 || h > 16384 ||
        !heap || !limit || limit > (1u << 27)) return 0;
    const size_t pixels = (size_t)w * h;
    unsigned channels=4;
    if (bytes == 3*pixels) channels=3;
    if (pixels > (1u << 24) || bytes != channels * pixels) return 0;
    *out = (CopyResult){0};

    if ((pixels > 4194304 || (pixels > 65536 && scout_few_literals(raw, w, pixels,channels))) &&
        scout_keep_repeat(raw, w, pixels,channels)) {
        PaletteStats stats = {0};
        uint8_t *copy = NULL;
        size_t size = 0;
        int status = qli_encode_repeat(raw, bytes, w, h, heap, limit, 0, &copy, &size, &stats);
        if (!palette_result_valid(status, copy, size, &stats, heap, limit)) {
            if (copy) free(copy);
            return 0;
        }
        out->peak = stats.peak;
        if (status == 1) {
            const size_t price = qli_opaque_file_price(copy,size);
            if (pixels > 4194304 && price <= 3 * pixels / 32 && size >= 28 &&
                !memcmp(copy, channels==3 ? "QRV3" : "QCRH", 4) && qli_read_le32(copy + 16) > 1) {
                out->copy = copy;
                out->bytes = size;
                return 1;
            }
            if (select_bounded(raw, bytes, w, h, heap - stats.live, price,
                               &out->selection)) {
                const size_t used = stats.live + out->selection.peak;
                if (used > out->peak) out->peak = used;
                free(copy);
                return 1;
            }
            out->copy = copy;
            out->bytes = size;
            out->peak = heap;
            return 1;
        }
        if (copy) free(copy);
    }
    if (!select_with_numeric_bound(raw, bytes, w, h, heap, limit, &out->selection)) return 0;
    if (out->selection.peak > out->peak) out->peak = out->selection.peak;
    return 1;
}

#if QLI_MAX_CPU_TIER >= QLI_TIER_SSE42 && (defined(_M_X64) || defined(__x86_64__))
#define QLI_STRUCTURAL_X86 1
#else
#define QLI_STRUCTURAL_X86 0
#endif

static int structural_native(void)
{
#if QLI_STRUCTURAL_X86
    return (qli_cpu_capabilities() & QLI_CPU_SSE42) != 0;
#else
    return 0;
#endif
}

static void structural_tags(const uint8_t *const channels[4], unsigned count, size_t n,
                            uint8_t *out, int native)
{
    size_t i = 0;
#if QLI_STRUCTURAL_X86
    if (native) {
        const __m128i zero = _mm_setzero_si128(), one = _mm_set1_epi8(1);
        for (; n - i >= 16; i += 16) {
            __m128i tag = zero;
            for (unsigned c = count; c--;) {
                __m128i value;
                memcpy(&value, channels[c] + i, sizeof(value));
                __m128i sign = _mm_add_epi8(_mm_and_si128(value, one), one);
                sign = _mm_andnot_si128(_mm_cmpeq_epi8(value, zero), sign);
                tag = _mm_add_epi8(_mm_add_epi8(tag, _mm_add_epi8(tag, tag)), sign);
            }
            memcpy(out + i, &tag, sizeof(tag));
        }
    }
#else
    (void)native;
#endif
    for (; i < n; ++i) {
        unsigned tag = 0;
        for (unsigned c = count; c--;) {
            unsigned value = channels[c][i];
            tag = 3 * tag + (value ? 1 + (value & 1) : 0);
        }
        out[i] = (uint8_t)tag;
    }
}

static int structural_gather(const uint8_t *src, size_t n, uint8_t *out, size_t cap,
                             size_t *written, int native)
{
    size_t i = 0, at = 0;
#if QLI_STRUCTURAL_X86
    if (native) {
        const __m128i zero = _mm_setzero_si128(), one = _mm_set1_epi8(1), low = _mm_set1_epi8(127);
        for (; n - i >= 16; i += 16) {
            __m128i value;
            memcpy(&value, src + i, sizeof(value));
            const unsigned mask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(value, zero)) ^ 65535;
            if (!mask) continue;
            if (mask == 65535) {
                if (cap - at < 16) return 0;
                const __m128i magnitude = _mm_and_si128(_mm_srli_epi16(_mm_sub_epi8(value, one), 1), low);
                memcpy(out + at, &magnitude, sizeof(magnitude));
                at += 16;
            } else {
                if (qlic_count_bits(mask) > cap - at) return 0;
                for (unsigned j = 0; j < 16; ++j)
                    if (src[i + j]) out[at++] = (uint8_t)((src[i + j] - 1) / 2);
            }
        }
    }
#else
    (void)native;
#endif
    for (; i < n; ++i) {
        if (!src[i]) continue;
        if (at == cap) return 0;
        out[at++] = (uint8_t)((src[i] - 1) / 2);
    }
    *written = at;
    return 1;
}
#undef QLI_STRUCTURAL_X86


typedef struct {
    uint8_t *data;
    size_t size, capacity;
} FinishStore;

static int finish_store_append(FinishStore *s, const FinishPart *part, FinishStoredPart *stored)
{
    const size_t limit = (size_t)1 << 29;
    if (!part->data || !part->size || part->size > limit || s->size > limit - part->size) return 0;
    size_t needed = s->size + part->size;
    QliEncodeBuffer buffer = {s->data, s->capacity};
    if (!qli_encode_buffer_reserve(&buffer, needed, s->size)) return 0;
    s->data = buffer.data;
    s->capacity = buffer.capacity;
    memcpy(s->data + s->size, part->data, part->size);
    *stored = (FinishStoredPart){s->size, part->size};
    s->size = needed;
    return 1;
}

typedef struct {
    unsigned policy, groups;
    FinishGroup *group;
    FinishStore payload;
    size_t bytes[2];
    int neighbor;
} FinishPolicy;

static void finish_policy_free(FinishPolicy *p)
{
    if (!p) return;
    qli_free(p->payload.data);
    qli_free(p->group);
    qli_free(p);
}
static int finish_shared_prepare(const QliStructuralField *structure, FinishShared **result)
{
    const QliDenseField *f = &structure->field;
    *result = NULL;
    FinishShared *s = qli_calloc(1, sizeof(*s));
    if (!s) return 0;
    s->field = f;
    s->tags = qli_malloc(f->pixels);
    if (!s->tags) goto fail;
    for (unsigned bank = 0; bank < f->streams; ++bank) {
        for (unsigned z = 1; z < 256; ++z)
            s->counts[bank] += structure->joint[bank][z];
    }
    const int native = structural_native();
    size_t offset = 0, tile = 0;
    for (unsigned y = 0; y < f->h; y += 16)
        for (unsigned x = 0; x < f->w; x += 16, ++tile) {
            size_t n = qli_dense_tile_pixels(x, y, f->w, f->h);
            const uint8_t *channels[4] = {0};
            for (unsigned c = 0; c < f->channels; ++c)
                channels[c] = f->symbols + c * f->pixels + offset;
            structural_tags(channels, f->channels, n, s->tags + offset, native);
            offset += n;
        }
    if (offset != f->pixels || tile != f->tiles) goto fail;
    *result = s;
    return 1;
fail:
    finish_shared_free(s);
    return 0;
}

static int finish_shared_values_only(FinishShared *s)
{
    const QliDenseField *f = s->field;
    size_t total = 0, cursor[32];
    for (unsigned i = 0; i < f->streams; ++i) {
        s->starts[i] = cursor[i] = total;
        total += s->counts[i];
    }
    s->values = qli_malloc(total ? total : 1);
    if (!s->values) return 0;
    const int native = structural_native();
    for (unsigned c = 0; c < f->channels; ++c) {
        size_t offset = 0, tile = 0;
        for (unsigned y = 0; y < f->h; y += 16)
            for (unsigned x = 0; x < f->w; x += 16, ++tile) {
                size_t n = qli_dense_tile_pixels(x, y, f->w, f->h);
                unsigned bank = c * 8 + (f->map[c * f->tiles + tile] >> 4);
                size_t end = s->starts[bank] + s->counts[bank], written = 0;
                if (cursor[bank] > end ||
                    !structural_gather(f->symbols + c * f->pixels + offset, n,
                                       s->values + cursor[bank], end - cursor[bank], &written, native))
                    return 0;
                cursor[bank] += written;
                offset += n;
            }
    }
    for (unsigned i = 0; i < f->streams; ++i) {
        if (cursor[i] != s->starts[i] + s->counts[i]) return 0;
    }
    return 1;
}

static int finish_shared_map(FinishShared *s)
{
    const QliDenseField *f = s->field;
    if (!qli_four_encode(f->map, f->maps, NULL, NULL, &s->map.data, &s->map.size)) return 0;
    s->map_coded = s->map.size < f->maps;
    return 1;
}

static int finish_shared_values(FinishShared *s)
{
    return finish_shared_values_only(s) && finish_shared_map(s);
}

enum { QNW_SLOTS = 1024, QNW_LOW = 1 << 23, QNW_KEYS = 243, QNW_ALPHABET = 81 };

typedef struct {
    const uint8_t *tags;
    unsigned w, h;
} QnlTile;

typedef struct {
    uint32_t hist[QNW_KEYS * QNW_ALPHABET];
    uint16_t frequency[(QNW_KEYS + 1) * QNW_ALPHABET];
    uint16_t start[(QNW_KEYS + 1) * QNW_ALPHABET];
    uint8_t bytes[(QNW_KEYS + 1) * QNW_ALPHABET];
} QnlLaws;

static unsigned qnl_key(const uint8_t *p, unsigned w, unsigned x, unsigned y)
{
    unsigned n = y ? p[-(int)w] : 0, l = x ? p[-1] : 0;
    return 3 * n + (l == n ? 0 : l == 0 ? 1 : 2);
}

static void qnl_normalize(const uint32_t *counts, unsigned alphabet, uint32_t total,
                          uint16_t *frequency, uint16_t *start)
{
    unsigned present = 0;
    for (unsigned s = 0; s < alphabet; ++s)
        present += counts[s] != 0;
    uint32_t sum = 0, support = 0;
    unsigned previous = 0;
    for (unsigned s = 0; s < alphabet; ++s) {
        sum += counts[s];
        support += counts[s] != 0;
        unsigned end = (unsigned)(((uint64_t)(QNW_SLOTS - present) * sum) / total) + support;
        start[s] = (uint16_t)previous;
        frequency[s] = (uint16_t)(end - previous);
        previous = end;
    }
}

static unsigned qnl_fit(QnlLaws *model, const QnlTile *tiles, size_t n, unsigned alpha,
                        uint8_t chosen[QNW_KEYS], uint8_t route[QNW_KEYS])
{
    unsigned keys = 3 * alpha;
    memset(model->hist, 0, (size_t)keys * alpha * sizeof(*model->hist));
    for (size_t t = 0; t < n; ++t) {
        unsigned w = tiles[t].w, h = tiles[t].h;
        const uint8_t *p = tiles[t].tags;
        for (unsigned y = 0; y < h; ++y)
            for (unsigned x = 0; x < w; ++x, ++p) {
                unsigned k = qnl_key(p, w, x, y);
                ++model->hist[(size_t)k * alpha + *p];
            }
    }
    uint32_t full[QNW_ALPHABET] = {0}, sparse[QNW_ALPHABET] = {0}, total = 0, sparse_count = 0;
    unsigned tables = 0;
    for (unsigned k = 0; k < keys; ++k) {
        const uint32_t *row = model->hist + (size_t)k * alpha;
        uint32_t count = 0;
        for (unsigned s = 0; s < alpha; ++s) {
            count += row[s];
            full[s] += row[s];
        }
        total += count;
        route[k] = 0;
        if (count >= 32) {
            chosen[tables++] = (uint8_t)k;
            route[k] = (uint8_t)tables;
            qnl_normalize(row, alpha, count, model->frequency + (size_t)tables * alpha,
                          model->start + (size_t)tables * alpha);
        } else {
            sparse_count += count;
            for (unsigned s = 0; s < alpha; ++s)
                sparse[s] += row[s];
        }
    }
    qnl_normalize(sparse_count ? sparse : full, alpha, sparse_count ? sparse_count : total,
                  model->frequency, model->start);
    return tables;
}

static int qnl_write_record(const uint8_t *src, size_t n, QliFourWorkspace *workspace, uint8_t *out,
                            size_t cap, size_t *written)
{
    uint8_t *packed = NULL;
    size_t bytes = 0;
    if (!qli_four_encode(src, n, NULL, workspace, &packed, &bytes)) return -1;
    int coded = bytes < n;
    size_t used = coded ? bytes : n;
    if (cap < 5 || used > cap - 5) return 0;
    out[0] = (uint8_t)coded;
    qli_write_le32(out + 1, (uint32_t)used);
    memcpy(out + 5, coded ? packed : src, used);
    *written = used + 5;
    return 1;
}

static size_t qnl_payload(const QnlTile *tiles, size_t n, unsigned alpha, const QnlLaws *model,
                          const uint8_t *route, uint8_t *out, size_t minimum, size_t cap)
{
    if (minimum > cap || cap - minimum < 16) return 0;
    uint32_t state[4] = {QNW_LOW, QNW_LOW, QNW_LOW, QNW_LOW};
    size_t at = cap;
    for (size_t b = (n + 3) / 4; b--;) {
        unsigned lanes = (unsigned)(n - 4 * b < 4 ? n - 4 * b : 4);
        const QnlTile *block = tiles + 4 * b;
        unsigned maxw = 0, maxh = 0;
        for (unsigned lane = 0; lane < lanes; ++lane) {
            if (block[lane].w > maxw) maxw = block[lane].w;
            if (block[lane].h > maxh) maxh = block[lane].h;
        }
        for (unsigned y = maxh; y--;)
            for (unsigned x = maxw; x--;)
                for (unsigned lane = lanes; lane--;) {
                    unsigned w = block[lane].w;
                    if (y >= block[lane].h || x >= w) continue;
                    const uint8_t *p = block[lane].tags + (size_t)y * w + x;
                    size_t i = (size_t)route[qnl_key(p, w, x, y)] * alpha + *p;
                    uint32_t f = model->frequency[i], s = state[lane];
                    uint32_t limit = ((QNW_LOW >> 10) << 8) * f;
                    while (s >= limit) {
                        if (at == minimum + 16) return 0;
                        out[--at] = (uint8_t)s;
                        s >>= 8;
                    }
                    state[lane] = ((s / f) << 10) + (s % f) + model->start[i];
                }
    }
    at -= 16;
    for (unsigned lane = 0; lane < 4; ++lane)
        qli_write_le32(out + at + 4 * lane, state[lane]);
    memmove(out + minimum, out + at, cap - at);
    return minimum + cap - at;
}

static int qnl_group(const uint8_t *tags, const uint16_t *shape, size_t tiles, unsigned alpha,
                     const uint8_t *original, size_t bytes, size_t prefix, QnlTile *mixed,
                     QnlLaws *model, QliFourWorkspace *workspace, QliEncodeBuffer *buffer,
                     size_t *result)
{
    *result = 0;
    if (bytes - prefix <= 32) return 1;
    size_t active = 0;
    for (size_t i = 0; i < tiles; ++i) {
        unsigned w = shape[i] & 255, h = shape[i] >> 8;
        size_t count = w * h;
        int same = prefix != 0;
        if (same)
            for (size_t j = 1; j < count; ++j)
                if (tags[j] != tags[0]) {
                    same = 0;
                    break;
                }
        if (!same) mixed[active++] = (QnlTile){tags, w, h};
        tags += count;
    }
    if (!active) return 1;
    size_t cap = bytes - 1;
    if (!qli_encode_buffer_reserve(buffer, cap, 0)) return 0;
    uint8_t chosen[QNW_KEYS], route[QNW_KEYS];
    unsigned tables = qnl_fit(model, mixed, active, alpha, chosen, route);
    size_t end = prefix + 6 + tables;
    if (end > cap || cap - end < 26) return 1;
    uint8_t *out = buffer->data;
    memcpy(out, original, prefix);
    memcpy(out + prefix, "QNL1", 4);
    out[prefix + 4] = 1;
    out[prefix + 5] = (uint8_t)tables;
    memcpy(out + prefix + 6, chosen, tables);
    size_t fields = (size_t)(tables + 1) * alpha;
    for (unsigned shift = 0; shift <= 8; shift += 8) {
        for (size_t i = 0; i < fields; ++i)
            model->bytes[i] = (uint8_t)(model->frequency[i] >> shift);
        size_t wrote = 0;
        int okay =
            qnl_write_record(model->bytes, fields, workspace, out + end, cap - end - 16, &wrote);
        if (okay < 0) return 0;
        if (!okay) return 1;
        end += wrote;
    }
    *result = qnl_payload(mixed, active, alpha, model, route, out, end, cap);
    return 1;
}

static int qnl_policy(FinishPolicy *p, const uint8_t *tags, const uint16_t *shapes,
                      unsigned alphabet, QliFourWorkspace *workspace)
{
    size_t max_tiles = 0;
    for (unsigned g = 0; g < p->groups; ++g)
        if (p->group[g].tiles > max_tiles) max_tiles = p->group[g].tiles;
    QnlTile *mixed = qli_malloc(max_tiles * sizeof(*mixed));
    QnlLaws *model = qli_malloc(sizeof(*model));
    QliEncodeBuffer buffer = {0};
    int ok = 0;
    if (!mixed || !model) goto done;
    for (unsigned g = 0; g < p->groups; ++g) {
        FinishGroup *b = p->group + g;
        FinishStoredPart old = b->stored.tiled.size ? b->stored.tiled : b->stored.plain;
        const uint8_t *payload = p->payload.data + old.offset;
        size_t prefix = 0, bytes = 0;
        if (b->stored.tiled.size) {
            if (old.size < 8 || memcmp(payload, "QTC1", 4)) goto done;
            prefix = 8 + (size_t)qli_read_le32(payload + 4);
            if (prefix > old.size) goto done;
        }
        if (!qnl_group(tags + b->start, shapes + b->tile_start, b->tiles, alphabet, payload,
                       old.size, prefix, mixed, model, workspace, &buffer, &bytes))
            goto done;
        if (bytes) {
            memcpy(p->payload.data + old.offset, buffer.data, bytes);
            b->stored.tiled = (FinishStoredPart){old.offset, bytes};
            p->bytes[1] -= old.size - bytes;
            p->neighbor = 1;
        }
    }
    ok = 1;
done:
    qli_free(buffer.data);
    qli_free(model);
    qli_free(mixed);
    return ok;
}

static int finish_policy_create_impl(const FinishShared *s, unsigned policy,
                                     const uint8_t *all_mixed, FinishPolicy **result)
{
    *result = NULL;
    const QliDenseField *f = s->field;
    size_t counts[4096] = {0}, tiles[4096] = {0};
    unsigned groups = finish_group_count(f, policy, counts, tiles);
    FinishPolicy *p = qli_calloc(1, sizeof(*p));
    if (!p) return 0;
    p->policy = policy;
    p->groups = groups;
    uint8_t *tags = NULL;
    uint16_t *tile_counts = NULL, *shapes = NULL;
    int ok = 0;
    FinishPart plain = {0}, tiled = {0};
    QliTagWorkspace workspace = {0};
    p->group = qli_calloc(p->groups, sizeof(*p->group));
    tags = qli_malloc(f->pixels);
    tile_counts = qli_malloc(f->tiles * sizeof(*tile_counts));
    if (!p->group || !tags || !tile_counts) goto done;
    shapes = qli_malloc(f->tiles * sizeof(*shapes));
    if (!shapes || !finish_group_tags(s, policy, p->group, groups, counts, tiles, tags, tile_counts, shapes))
        goto done;
    unsigned alphabet = f->channels == 1 ? 3 : f->channels == 3 ? 27 : 81;
    p->bytes[0] = p->bytes[1] = finish_overhead(s, policy, groups);
    for (unsigned g = 0; g < groups; ++g) {
        FinishGroup *b = p->group + g;
        tiled = (FinishPart){0};
        if (!qli_four_encode(tags + b->start, b->count, NULL, &workspace.entropy, &plain.data,
                             &plain.size) ||
            !finish_add(&p->bytes[0], plain.size) ||
            !finish_store_append(&p->payload, &plain, &b->stored.plain) ||
            ((!all_mixed || !(all_mixed[g >> 3] & (1u << (g & 7)))) &&
             !qli_dense_tag_packet(tags + b->start, tile_counts + b->tile_start, b->tiles, alphabet,
                                   &tiled.data, &tiled.size, &workspace)))
            goto done;
        if (tiled.size >= plain.size) tiled = (FinishPart){0};
        if (!finish_add(&p->bytes[1], tiled.data ? tiled.size : plain.size) ||
            (tiled.data && !finish_store_append(&p->payload, &tiled, &b->stored.tiled)))
            goto done;
    }
    if (p->bytes[1] < p->bytes[0] &&
        !qnl_policy(p, tags, shapes, alphabet, &workspace.entropy))
        goto done;
    *result = p;
    p = NULL;
    ok = 1;
done:
    qli_dense_tag_workspace_clear(&workspace);
    qli_free(tags);
    qli_free(tile_counts);
    qli_free(shapes);
    finish_policy_free(p);
    return ok;
}
static int finish_assemble(const FinishShared *s, const FinishPolicy *p, unsigned kind, size_t cap,
                           int direct_gray, uint8_t **file, size_t *size)
{
    *file = NULL;
    *size = 0;
    const QliDenseField *f = s->field;
    size_t total = p->bytes[kind];
    if (total > cap) return 0;
    uint8_t *out = qli_calloc(1, total);
    if (!out) return 0;
    const char *marker = kind ? (p->neighbor ? "QWNL" : "QWT1") : "QWI1";
    size_t lengths = write_strip_header(out, marker, s, direct_gray ? 2 : 1,
                                        p->groups, p->policy, p->group);
    size_t at = lengths + 4 * (p->groups + f->streams);
    for (unsigned g = 0; g < p->groups; ++g) {
        FinishStoredPart part =
            kind && p->group[g].stored.tiled.size ? p->group[g].stored.tiled : p->group[g].stored.plain;
        if (part.offset > p->payload.size || part.size > p->payload.size - part.offset) {
            qli_free(out);
            return 0;
        }
        qli_write_le32(out + lengths + 4 * g, (uint32_t)part.size);
        memcpy(out + at, p->payload.data + part.offset, part.size);
        at += part.size;
    }
    at = write_strip_parts(out, lengths + 4 * p->groups, at, s->magnitude, f->streams);
    if (at != total - 4) {
        qli_free(out);
        return 0;
    }
    qli_write_le32(out + at, qint_crc32(out, at));
    *file = out;
    *size = total;
    return 1;
}

static inline size_t compact_stride(unsigned S) { return (size_t)3u * (size_t)S + 1u; }

static inline void compact_count_tile(uint32_t *dst, unsigned S, const uint8_t *tags, unsigned n)
{
    unsigned i;
    uint8_t first;
    uint32_t *uniform;
    uint32_t *descriptor;

    if (n == 0u) {
        return;
    }

    first = tags[0];
    uniform = dst + S;
    descriptor = uniform + S;

    for (i = 1u; i < n; ++i) {
        if (tags[i] != first) {
            ++descriptor[S];
            for (i = 0u; i < n; ++i) {
                ++dst[tags[i]];
            }
            return;
        }
    }

    ++descriptor[first];
    uniform[first] += (uint32_t)n;
}

static inline void compact_add(uint32_t *dst, const uint32_t *src, unsigned S)
{
    size_t i;
    const size_t count = compact_stride(S);

    for (i = 0u; i < count; ++i) {
        dst[i] += src[i];
    }
}

static inline void compact_expand(const uint32_t *src, unsigned S, uint32_t direct[256],
                                  uint32_t descriptor[256], uint32_t mixed[256])
{
    unsigned i;
    const uint32_t *const uniform = src + S;
    const uint32_t *const compact_descriptor = uniform + S;

    memset(direct, 0, 256u * sizeof(*direct));
    memset(descriptor, 0, 256u * sizeof(*descriptor));
    memset(mixed, 0, 256u * sizeof(*mixed));

    for (i = 0u; i < S; ++i) {
        mixed[i] = src[i];
        direct[i] = src[i] + uniform[i];
    }
    for (i = 0u; i <= S; ++i) {
        descriptor[i] = compact_descriptor[i];
    }
}

typedef struct {
    uint32_t direct[256], descriptor[256], mixed[256];
} GroupHist;
static int policy_qbs_estimate(const uint32_t histogram[256], double *bytes)
{
    uint64_t counts[256];
    QlicRansModelStats model;
    for (unsigned v = 0; v < 256; ++v)
        counts[v] = histogram[v];
    if (!qlic_rans_measure_histogram(counts, &model) || !isfinite(model.modeled_bits) ||
        model.modeled_bits < 0)
        return 0;
    *bytes = 16.0 + 32.0 + (double)model.descriptor_bytes +
             (model.used_symbols <= 1 ? 0.0 : model.modeled_bits / 8.0);
    return isfinite(*bytes) && *bytes >= 0;
}
typedef struct {
    QnlTile tile;
    uint32_t next[2];
    unsigned fold;
} TagSample;

typedef struct {
    uint32_t count[2][QNW_KEYS * QNW_ALPHABET];
} TagSampleCounts;

static double sampled_tag_credit(const TagSample *samples, uint32_t first, unsigned list,
                                 unsigned symbols, TagSampleCounts *work)
{
    unsigned tiles[2] = {0};
    for (uint32_t i = first; i != UINT32_MAX; i = samples[i].next[list])
        ++tiles[samples[i].fold];
    if (tiles[0] < 4 || tiles[1] < 4) return 0;

    const unsigned cells = 3 * symbols * symbols;
    memset(work->count[0], 0, cells * sizeof(uint32_t));
    memset(work->count[1], 0, cells * sizeof(uint32_t));
    for (uint32_t i = first; i != UINT32_MAX; i = samples[i].next[list]) {
        const QnlTile *tile = &samples[i].tile;
        uint32_t *count = work->count[samples[i].fold];
        const uint8_t *p = tile->tags;
        for (unsigned y = 0; y < tile->h; ++y)
            for (unsigned x = 0; x < tile->w; ++x, ++p)
                ++count[qnl_key(p, tile->w, x, y) * symbols + *p];
    }

    uint32_t marginal[2][QNW_ALPHABET] = {0};
    unsigned totals[2] = {0};
    for (unsigned i = 0; i < cells; i += symbols)
        for (unsigned s = 0; s < symbols; ++s) {
            marginal[0][s] += work->count[0][i + s];
            marginal[1][s] += work->count[1][i + s];
        }
    for (unsigned s = 0; s < symbols; ++s) {
        totals[0] += marginal[0][s];
        totals[1] += marginal[1][s];
    }
    double gain = 0;
    for (unsigned f = 0; f < 2; ++f) {
        double prior[QNW_ALPHABET];
        for (unsigned s = 0; s < symbols; ++s)
            prior[s] = (marginal[f][s] + 0.5) / (totals[f] + 0.5 * symbols);
        for (unsigned i = 0; i < cells; i += symbols) {
            const uint32_t *train = work->count[f] + i, *test = work->count[1 - f] + i;
            unsigned n = 0;
            for (unsigned s = 0; s < symbols; ++s)
                n += train[s];
            if (n < 4) continue;
            const double norm = 1.0 / (n + 8);
            for (unsigned s = 0; s < symbols; ++s)
                if (test[s]) gain += test[s] * log2((train[s] / prior[s] + 8) * norm);
        }
    }
    if (gain <= 0) return 0;
    uint32_t low[256] = {0}, high[256] = {0};
    uint32_t row[QNW_ALPHABET], root[QNW_ALPHABET] = {0};
    uint16_t frequency[QNW_ALPHABET], start[QNW_ALPHABET];
    unsigned tables = 0, root_total = 0;
    for (unsigned i = 0; i < cells; i += symbols) {
        unsigned n = 0;
        for (unsigned s = 0; s < symbols; ++s) {
            row[s] = work->count[0][i + s] + work->count[1][i + s];
            n += row[s];
        }
        if (n < 4) {
            root_total += n;
            for (unsigned s = 0; s < symbols; ++s)
                root[s] += row[s];
            continue;
        }
        ++tables;
        qnl_normalize(row, symbols, n, frequency, start);
        for (unsigned s = 0; s < symbols; ++s) {
            ++low[frequency[s] & 255];
            ++high[frequency[s] >> 8];
        }
    }
    if (!root_total) {
        root_total = totals[0] + totals[1];
        for (unsigned s = 0; s < symbols; ++s)
            root[s] = marginal[0][s] + marginal[1][s];
    }
    qnl_normalize(root, symbols, root_total, frequency, start);
    for (unsigned s = 0; s < symbols; ++s) {
        ++low[frequency[s] & 255];
        ++high[frequency[s] >> 8];
    }
    double low_bytes, high_bytes;
    if (!policy_qbs_estimate(low, &low_bytes) || !policy_qbs_estimate(high, &high_bytes)) return 0;
    const double raw = (double)(tables + 1) * symbols;
    const double cost = 32 + tables + fmin(raw, low_bytes) + fmin(raw, high_bytes);
    return fmax(0, gain - cost);
}

static uint32_t tag_sample_hash(uint32_t block)
{
    uint32_t h = (block + 1) * UINT32_C(0x9e3779b9);
    h = (h ^ (h >> 16)) * UINT32_C(0x85ebca6b);
    return h ^ (h >> 13);
}

static int prefer_green_neighbors(const FinishShared *s, unsigned symbols,
                                  const int16_t lookup[4096], unsigned groups, double gap)
{
    const QliDenseField *f = s->field;
    const size_t capacity = 2 * ((f->tiles + 15) / 16);
    TagSample *samples = qli_malloc(capacity * sizeof(*samples));
    TagSampleCounts *counts = qli_malloc(sizeof(*counts));
    uint32_t fine[4096], green[8];
    int prefer = 0;
    if (!samples || !counts) goto done;
    memset(fine, 255, groups * sizeof(*fine));
    memset(green, 255, sizeof(green));
    size_t offset = 0, tile = 0;
    uint32_t used = 0;
    unsigned first = 0, second = 0;
    for (unsigned y = 0; y < f->h; y += 16)
        for (unsigned x = 0; x < f->w; x += 16, ++tile) {
            const unsigned w = f->w - x < 16 ? f->w - x : 16, h = f->h - y < 16 ? f->h - y : 16;
            const unsigned pos = (unsigned)tile & 15;
            const uint8_t *tags = s->tags + offset;
            offset += (size_t)w * h;
            if (!pos) {
                const uint32_t hash = tag_sample_hash((uint32_t)(tile >> 4));
                first = hash & 15;
                second = (first + 1 + ((hash >> 4) % 15)) & 15;
            }
            if (pos != first && pos != second) continue;
            unsigned i = 1;
            while (i < w * h && tags[i] == tags[0])
                ++i;
            if (i == w * h) continue;
            const unsigned key = finish_key(f, 2, tile), g = (key >> 3) & 7;
            const int slot = lookup[key];
            if (slot < 0 || (unsigned)slot >= groups || used >= capacity) goto done;
            samples[used] = (TagSample){{tags, w, h}, {fine[slot], green[g]}, pos == second};
            fine[slot] = green[g] = used++;
        }
    double green_credit = 0, fine_credit = 0;
    for (unsigned i = 0; i < 8; ++i)
        green_credit += sampled_tag_credit(samples, green[i], 1, symbols, counts);
    if (green_credit <= gap + 1024) goto done;
    for (unsigned i = 0; i < groups; ++i) {
        fine_credit += sampled_tag_credit(samples, fine[i], 0, symbols, counts);
        if (green_credit <= gap + 1024 + fine_credit) goto done;
    }
    prefer = 1;
done:
    qli_free(counts);
    qli_free(samples);
    return prefer;
}

static int finish_choose_policy_planned(const FinishShared *s, double shared_bytes,
                                        unsigned *chosen, double scores[3], uint8_t *all_mixed,
                                        int capped)
{
    *chosen = 0;
    memset(all_mixed, 0, 512);
    const QliDenseField *f = s->field;
    const unsigned symbols = f->channels == 1 ? 3 : f->channels == 3 ? 27 : 81;
    const size_t stride = compact_stride(symbols);
    int16_t lookup[4096];
    uint16_t keys[4096];
    unsigned groups = 0;
    uint32_t *hist = NULL;
    int ok = 0;
    for (unsigned k = 0; k < 4096; ++k)
        lookup[k] = -1;
    for (size_t tile = 0; tile < f->tiles; ++tile)
        lookup[finish_key(f, 2, tile)] = 0;
    for (unsigned k = 0; k < 4096; ++k)
        if (lookup[k] == 0) {
            keys[groups] = (uint16_t)k;
            lookup[k] = (int16_t)groups++;
        }
    if (!groups || groups > 4096) goto done;
    hist = qli_calloc(((size_t)groups + 9) * stride, sizeof(*hist));
    if (!hist) goto done;
    size_t tile = 0, offset = 0;
    for (unsigned y = 0; y < f->h; y += 16)
        for (unsigned x = 0; x < f->w; x += 16, ++tile) {
            size_t n = qli_dense_tile_pixels(x, y, f->w, f->h);
            unsigned key = finish_key(f, 2, tile);
            if (lookup[key] < 0 || n > f->pixels - offset) goto done;
            compact_count_tile(hist + ((size_t)9 + (unsigned)lookup[key]) * stride, symbols,
                               s->tags + offset, (unsigned)n);
            offset += n;
        }
    if (offset != f->pixels || tile != f->tiles) goto done;
    const unsigned green = f->channels == 1 ? 0 : 1;
    for (unsigned g = 0; g < groups; ++g) {
        const uint32_t *part = hist + ((size_t)9 + g) * stride;
        unsigned bank = (keys[g] >> (3 * green)) & 7;
        compact_add(hist, part, symbols);
        compact_add(hist + (size_t)(1 + bank) * stride, part, symbols);
    }
    const unsigned starts[3] = {0, 1, 9}, counts[3] = {1, 8, groups};
    for (unsigned policy = 0; policy < 3; ++policy) {
        size_t framing = 44 + f->coefficient_bytes + 4 * (counts[policy] + f->streams) + 4 +
                         (policy == 2 ? 4 + 2 * groups : 0);
        scores[policy] = (double)framing + shared_bytes;
        for (unsigned g = 0; g < counts[policy]; ++g) {
            GroupHist expanded;
            double direct, descriptor, mixed;
            compact_expand(hist + (size_t)(starts[policy] + g) * stride, symbols, expanded.direct,
                           expanded.descriptor, expanded.mixed);
            if (!policy_qbs_estimate(expanded.direct, &direct)) goto done;
            uint32_t any_uniform = 0;
            for (unsigned v = 0; v < symbols; ++v)
                any_uniform |= expanded.descriptor[v];
            if (!any_uniform)
                scores[policy] += direct;
            else {
                if (!policy_qbs_estimate(expanded.descriptor, &descriptor) ||
                    !policy_qbs_estimate(expanded.mixed, &mixed))
                    goto done;
                double tiled = 8.0 + descriptor + mixed;
                scores[policy] += direct < tiled ? direct : tiled;
            }
        }
        if (!isfinite(scores[policy])) goto done;
    }
    for (unsigned policy = 1; policy < 3; ++policy)
        if (scores[policy] < scores[*chosen]) *chosen = policy;
    if (!capped && *chosen == 2 && f->channels >= 3 && f->tiles >= 256 &&
        prefer_green_neighbors(s, symbols, lookup, groups, scores[1] - scores[2]))
        *chosen = 1;
    if (all_mixed) {
        for (unsigned g = 0; g < counts[*chosen]; ++g) {
            const uint32_t *descriptor =
                hist + ((size_t)starts[*chosen] + g) * stride + 2 * symbols;
            uint32_t any_uniform = 0;
            for (unsigned v = 0; v < symbols; ++v)
                any_uniform |= descriptor[v];
            if (!any_uniform) all_mixed[g >> 3] |= (uint8_t)(1u << (g & 7));
        }
    }
    ok = 1;
done:
    qli_free(hist);
    return ok;
}

typedef struct {
    uint32_t joint[32][256];
    double qep_bytes, split_bytes, policy_bytes[3];
    unsigned policy, split;
} StructuralPlan;

static int structural_plan(const FinishShared *s, const QliStructuralField *structure,
                           StructuralPlan *plan, uint8_t all_mixed[512], int capped)
{
    const QliDenseField *f = s->field;
    uint32_t map[256] = {0};
    for (size_t i = 0; i < f->maps; ++i)
        ++map[f->map[i]];
    double map_bytes;
    if (!policy_qbs_estimate(map, &map_bytes)) return 0;
    double shared = fmin(map_bytes, (double)f->maps);
    plan->qep_bytes = (double)(40 + f->maps + f->coefficient_bytes + 4 * f->streams + 4);
    memcpy(plan->joint, structure->joint, f->streams * sizeof(plan->joint[0]));
    for (unsigned stream = 0; stream < f->streams; ++stream) {
        const uint32_t *joint = plan->joint[stream];
        uint32_t magnitude[256] = {0};
        for (unsigned z = 1; z < 256; ++z)
            magnitude[(z - 1) / 2] += joint[z];
        double whole, part;
        if (!policy_qbs_estimate(joint, &whole) || !policy_qbs_estimate(magnitude, &part)) return 0;
        plan->qep_bytes += whole;
        shared += part;
    }
    if (!finish_choose_policy_planned(s, shared, &plan->policy, plan->policy_bytes, all_mixed,
                                      capped))
        return 0;
    plan->split_bytes =
        fmin(plan->policy_bytes[0], fmin(plan->policy_bytes[1], plan->policy_bytes[2]));
    plan->split = plan->split_bytes < plan->qep_bytes;
    return 1;
}

static QlicLiteStatus structural_write_split(FinishShared *s, unsigned policy,
                                        const uint8_t all_mixed[512], int capped, int direct_gray,
                                        size_t cap, uint8_t **file, size_t *size)
{
    FinishPolicy *p = NULL;
    QlicLiteStatus status = QLIC_LITE_INTERNAL;
    if (capped ? !finish_shared_map(s) : !finish_shared_values(s)) goto done;
    if (!finish_policy_create_impl(s, policy, all_mixed, &p)) goto done;
    if (capped) {
        if (p->bytes[1] >= p->bytes[0]) {
            status = QLIC_LITE_INVALID;
            goto done;
        }
        if (p->bytes[1] > cap) {
            status = QLIC_LITE_LIMIT;
            goto done;
        }
        if (!finish_shared_values_only(s)) goto done;
    }
    for (unsigned i = 0; i < s->field->streams; ++i) {
        FinishPart *part = s->magnitude + i;
        if (!qli_dense_write_magnitude(s->values + s->starts[i], s->counts[i], &part->data,
                                       &part->size))
            goto done;
        if (capped) {
            if (!finish_add(&p->bytes[0], part->size) || !finish_add(&p->bytes[1], part->size))
                goto done;
            if (p->bytes[1] > cap) {
                status = QLIC_LITE_LIMIT;
                goto done;
            }
        }
    }
    if (!capped) {
        for (unsigned i = 0; i < s->field->streams; ++i)
            if (!finish_add(&p->bytes[0], s->magnitude[i].size) ||
                !finish_add(&p->bytes[1], s->magnitude[i].size))
                goto done;
    }
    unsigned kind = p->bytes[1] < p->bytes[0];
    if (finish_assemble(s, p, kind, cap, direct_gray, file, size)) status = QLIC_LITE_OK;
done:
    finish_policy_free(p);
    return status;
}

QlicLiteStatus qli_structural_finish(const QliStructuralField *structure, int capped, int direct_gray,
                                size_t cap, uint8_t **file, size_t *size)
{
    *file = NULL;
    *size = 0;
    uint8_t all_mixed[512];
    FinishShared *s = NULL;
    StructuralPlan *plan = qli_calloc(1, sizeof(*plan));
    QlicLiteStatus status = QLIC_LITE_INTERNAL;
    if (!plan || !finish_shared_prepare(structure, &s) ||
        !structural_plan(s, structure, plan, all_mixed, capped))
        goto done;
    unsigned policy = plan->policy, split = plan->split;
    qli_free(plan);
    plan = NULL;
    if (split) {
        status = structural_write_split(s, policy, all_mixed, capped, direct_gray, cap, file, size);
    } else if (capped) {
        status = QLIC_LITE_INVALID;
    } else {
        finish_shared_free(s);
        s = NULL;
        if (qli_structural_write_tiles(&structure->field, direct_gray, file, size)) {
            if (*size <= cap)
                status = QLIC_LITE_OK;
            else {
                qli_free(*file);
                *file = NULL;
                *size = 0;
            }
        }
    }
done:
    qli_free(plan);
    finish_shared_free(s);
    return status;
}

static QlicLiteStatus encode_structural_field(const uint8_t *raw, size_t bytes, unsigned w, unsigned h,
                                         int capped, size_t cap, uint8_t **out, size_t *size)
{
    *out = NULL;
    *size = 0;
    QliStructuralField *field = NULL;
    int direct_gray = 0;
    if (!qli_dense_prepare_structure(raw, bytes, w, h, &direct_gray, &field))
        return QLIC_LITE_INTERNAL;
    QlicLiteStatus status = qli_structural_finish(field, capped, direct_gray, cap, out, size);
    qli_structural_field_free(field);
    if (status == QLIC_LITE_OK && bytes == 3 * (size_t)w * h && *size >= 48 &&
        (!memcmp(*out,"QWNL",4) || !memcmp(*out,"QWT1",4) || !memcmp(*out,"QWI1",4)
         || !memcmp(*out,"QEP1",4)
        )) {
        (*out)[14] = 3;
        qli_write_le32(*out + *size - 4, qint_crc32(*out, *size - 4));
    }
    return status;
}

static int encode_structure_core(const uint8_t *raw, size_t bytes, unsigned w, unsigned h,
                                  uint8_t **out, size_t *size)
{
    return encode_structural_field(raw, bytes, w, h, 0, SIZE_MAX, out, size) == QLIC_LITE_OK;
}

static QlicLiteStatus encode_structure_image(const QlicLiteDescription *d, const uint8_t *raw, size_t bytes,
                                        size_t stride, const QlicLiteLimits *limits, uint8_t **out,
                                        size_t *size)
{
    if (d->bits==8 && d->channels==3 && !d->alpha && stride==(size_t)d->width*3 &&
        bytes==stride*d->height && !d->authority && !d->has_mastering && !d->has_light)
        return encode_structural_field(raw, bytes, d->width, d->height, 0,
                                       SIZE_MAX, out, size);
    (void)bytes;
    return qli_write_sample_image(d, raw, stride, limits, out, size, encode_structure_core);
}

QlicLiteStatus qli_encode_structure(const QlicLiteDescription *d, const uint8_t *raw, size_t bytes,
                               size_t stride, const QlicLiteLimits *limits,
                               const QlicLiteAllocator *allocator, QlicLiteFile *file, QlicLiteStats *stats)
{
    return qli_encode_owned(d, raw, bytes, stride, limits, allocator, file, stats, encode_structure_image);
}

static QlicLiteStatus encode_capped_structure(const QlicLiteDescription *d, const uint8_t *raw, size_t bytes,
                                         size_t stride, const QlicLiteLimits *limits, uint8_t **out,
                                         size_t *size)
{
    int layout = d->channels == 4 && d->alpha == 1;
    layout |= d->channels == 3 && d->alpha == 0;
    if (d->bits != 8 || !layout || d->authority || d->has_mastering ||
        d->has_light || stride != (size_t)d->width * d->channels) return QLIC_LITE_INVALID;
    (void)bytes;
    return encode_structural_field(raw, stride * d->height, d->width, d->height, 1,
                                   limits->file_bytes, out, size);
}

QlicLiteStatus qli_encode_capped_structure(const QlicLiteDescription *d, const uint8_t *raw, size_t bytes,
                                      size_t stride, const QlicLiteLimits *limits,
                                      const QlicLiteAllocator *allocator, QlicLiteFile *file, QlicLiteStats *stats)
{
    QlicLiteStatus status = qli_encode_owned(d, raw, bytes, stride, limits, allocator, file, stats,
                                      encode_capped_structure);
    if (stats && status == QLIC_LITE_LIMIT && !stats->limit_denials && stats->requests)
        ++stats->limit_denials;
    return status;
}
