/* Copyright (C) 2026 Ozzy M.
 * SPDX-License-Identifier: Apache-2.0
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

enum { QLH_OK, QLH_ARGUMENT, QLH_DESCRIPTION, QLH_LIMIT, QLH_LENGTH,
       QLH_CHECKSUM, QLH_CAPACITY, QLH_SAMPLE, QLH_OVERLAP };

typedef struct {
    uint64_t max_pixels, max_metadata, max_record, max_sample_bytes;
} QlhLimits;

int qlh_record_size(const QlicLiteDescription *, const QlhLimits *, size_t *);
int qlh_parse(const uint8_t *, size_t, const QlhLimits *, QlicLiteDescription *);
uint32_t qint_crc32(const uint8_t *, size_t);


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

enum { ALLOCATION_SLOTS = 256 };

typedef struct {
    void *pointer;
    size_t bytes;
} AllocationSlot;

typedef struct {
    AllocationSlot slots[ALLOCATION_SLOTS];
    size_t live_bytes, live_blocks;
    QlicLiteLimits limits;
    QlicLiteAllocator allocator;
    QlicLiteStats stats;
    QlicLiteStatus failure;
} AllocationContext;

static _Thread_local AllocationContext *active_allocations;

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

QLI_NOINLINE void *qli_malloc(size_t bytes)
{
    AllocationContext *context = active_allocations;
    if (!context || context->failure != QLIC_LITE_OK) return NULL;
    if (context->stats.requests == SIZE_MAX) {
        ++context->stats.limit_denials;
        record_failure(context, QLIC_LITE_LIMIT);
        return NULL;
    }

    ++context->stats.requests;
    size_t actual = bytes ? bytes : 1;
    if (context->stats.requests > context->limits.allocation_requests ||
        context->live_blocks == ALLOCATION_SLOTS ||
        actual > context->limits.heap_bytes - context->live_bytes) {
        ++context->stats.limit_denials;
        record_failure(context, QLIC_LITE_LIMIT);
        return NULL;
    }

    ++context->stats.allocator_calls;
    void *block = context->allocator.allocate(actual, context->allocator.user);
    if (!block) {
        ++context->stats.allocator_failures;
        record_failure(context, QLIC_LITE_ALLOCATOR);
        return NULL;
    }

    context->slots[context->live_blocks++] = (AllocationSlot){block, actual};
    context->live_bytes += actual;
    if (context->live_bytes > context->stats.peak_bytes)
        context->stats.peak_bytes = context->live_bytes;
    return block;
}

static unsigned find_allocation(const AllocationContext *context, const void *block)
{
    for (unsigned i = 0; i < context->live_blocks; ++i)
        if (context->slots[i].pointer == block) return i;
    return ALLOCATION_SLOTS;
}

void qli_free(void *block)
{
    if (!block) return;
    AllocationContext *context = active_allocations;
    if (!context) return;
    unsigned i = find_allocation(context, block);
    if (i == ALLOCATION_SLOTS) {
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
            context->stats.retained_bytes = context->live_bytes;
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

enum { FAST_RAW = 0, FAST_LEFT = 1, FAST_ROW = 2, FAST_PALETTE = 3,
       FAST_SOLID = 4, FAST_AUTO = 255, FAST_HEADER = 32, FAST_FOOTER = 4 };
int fast_decode_alloc(const uint8_t *file, size_t size,
                              size_t max_rgba_bytes, uint8_t **rgba,
                              uint32_t *width, uint32_t *height);
static inline int fast_shape(uint32_t w, uint32_t h, size_t *pixels) {
    if (!w || !h || w > (1u << 20) || h > (1u << 20) || (uint64_t)w * h > (1u << 25)) return 0;
    *pixels = (size_t)w * h; return 1;
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

enum {
    SYMBOLS = QLI_HUFFMAN_SYMBOLS,
    MAX_BITS = 24,
    PRIMARY_BITS = 10,
    MAX_DESCRIPTOR_BYTES = QLI_HUFFMAN_DESCRIPTOR_CAP,
    BYTE_HEADER_BYTES = 11
};

typedef QliCodebook Codebook;

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

static size_t uleb_size(size_t value) {
    size_t bytes = 1;
    while (value >= 128u) {
        value >>= 7;
        ++bytes;
    }
    return bytes;
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

#endif
#ifdef QLI_NUMERIC_SSE42

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

int pack_decode(const uint8_t *, size_t, size_t max_output, uint8_t **, size_t *);

enum { PACK_HEADER = 32, PACK_FOOTER = 4, MAX_BYTES = QLI_BYTE_INPUT_CAP,
       RAW = 0, HUFFMAN = 1, MATCH = 2 };

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
static uint8_t plane_predict(const uint8_t *plane, size_t pos, uint32_t x, uint32_t y, uint32_t w, unsigned mode) {
    return simple_predict(mode, x ? plane[pos - 1] : 0, y ? plane[pos - w] : 0, x && y ? plane[pos - w - 1] : 0);
}
static size_t tile_pixels(uint32_t tx, uint32_t ty, uint32_t w, uint32_t h) {
    uint32_t a = w - tx < TILE ? w - tx : TILE, b = h - ty < TILE ? h - ty : TILE;
    return (size_t)a * b;
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

#ifdef QLIC_WASM
QlicLiteImage *qlic_lite_wasm_decode(const uint8_t *data, size_t bytes)
{
    QlicLiteLimits limits;
    qlic_lite_limits_default(&limits);
    limits.heap_bytes = 1u << 28;
    QlicLiteImage *image = calloc(1, sizeof(*image));
    if (!image) return NULL;
    if (qlic_lite_decode(data, bytes, &limits, NULL, image, NULL) != QLIC_LITE_OK) {
        free(image);
        return NULL;
    }
    return image;
}

const uint8_t *qlic_lite_wasm_samples(const QlicLiteImage *image) { return image->samples; }
size_t qlic_lite_wasm_size(const QlicLiteImage *image) { return image->bytes; }
uint32_t qlic_lite_wasm_width(const QlicLiteImage *image) { return image->description.width; }
uint32_t qlic_lite_wasm_height(const QlicLiteImage *image) { return image->description.height; }
uint32_t qlic_lite_wasm_bits(const QlicLiteImage *image) { return image->description.bits; }
uint32_t qlic_lite_wasm_channels(const QlicLiteImage *image) { return image->description.channels; }

void qlic_lite_wasm_free(QlicLiteImage *image)
{
    qlic_lite_image_free(image);
    free(image);
}
#endif
