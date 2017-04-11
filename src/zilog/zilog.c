/*
 * zilog.c
 *
 *  Created on: Nov 1, 2015
 *      Author: JiangYizuo
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <printf.h>
#include <malloc.h>
#include <syscall.h>
#include <pthread.h>
#include <time.h>
#include "zilog.h"
#include "zilog_internal.h"
#include "zilog_agent.h"

int zilog_debugLevels[ZILOG_MAX_SESSION_NUM];
/*
 * Each thread has a dedicated buffer to buffer log content.
 * The log agent fetches log content from these buffers.
 */
zilog_buf_t lbuf = {0,};
size_t *jiffies = NULL;
#define ZILOG_INSUFFICIENT_SPACE ((size_t)-1)
#define ZILOG_ALIGN(_offset, _aligned_size) \
        (((_offset) + ((_aligned_size) - 1)) & ~((_aligned_size) - 1))


typedef struct{
#ifdef __x86_64__
    uint16_t gp_off;
    uint16_t gp_size;
    uint16_t fp_size;
    uint16_t reg_size;
    uint16_t overflow_size;
    uint16_t string_off[ZILOG_MAX_ARG_NUM];
    uint16_t size;
    uint16_t packed_size;
#endif
    uint16_t string_len[ZILOG_MAX_ARG_NUM];
    const char* string[ZILOG_MAX_ARG_NUM];
}va_list_inf_t;

static void initialize_thread_buffers() {
    int i;
    pthread_t log_agent_thread;
    lbuf.size = ZILOG_THREAD_BUFFER_SIZE;
    lbuf.buf = (uint8_t*) malloc(
            sizeof(uint8_t) * lbuf.size);
    lbuf.write_offset = sizeof(zilog_block_header_t);
    lbuf.read_offset = sizeof(zilog_block_header_t);
    for (i = 0; i < ZILOG_THREAD_BUFFER_BLOCK_NUM; i++) {
        zilog_block_header_t* cur_block =
                (zilog_block_header_t*) (lbuf.buf + i
                        *ZILOG_THREAD_BUFFER_BLOCK_SIZE
                        * sizeof(uint8_t));
        zilog_content_header_t* first_content_header =
                (zilog_content_header_t*) ((uint8_t*) cur_block
                        + sizeof(zilog_block_header_t));
        /*write_sequence_num must be i as it impacts 'SynchronizationcPoint 1'*/
        cur_block->write_sequence_num = i;
        cur_block->content_num = 0;
        cur_block->lock = 0;
        first_content_header->size = 0;
        first_content_header->lunit = NULL;//ZILOG_INVALID_UNIT_ID;
        //printf("%d %p\n", i, cur_block);
    }
    lbuf.read_cursor = (zilog_block_header_t*)lbuf.buf;
    lbuf.init_flag = 1;
    //jiffies = zilog_time_init(0xffffffff81e09000);

    pthread_create(&log_agent_thread, NULL, zilog_agent_read_content, NULL);
}

#ifdef __x86_64__

/*
#define va_arg(ap, type)                                        /
    (*(type*)(__builtin_types_compatible_p(type, long double)   /
              ? (ap->overflow_arg_area += 16,                   /
                 ap->overflow_arg_area - 16)                    /
              : __builtin_types_compatible_p(type, double)      /
              ? (ap->fp_offset < 128 + 48                       /
                 ? (ap->fp_offset += 16,                        /
                    ap->reg_save_area + ap->fp_offset - 16)     /
                 : (ap->overflow_arg_area += 8,                 /
                    ap->overflow_arg_area - 8))                 /
              : (ap->gp_offset < 48                             /
                 ? (ap->gp_offset += 8,                         /
                    ap->reg_save_area + ap->gp_offset - 8)      /
                 : (ap->overflow_arg_area += 8,                 /
                    ap->overflow_arg_area - 8))                 /
        ))
             */
static size_t calculate_arg_list_size_x86_64(
        va_list_inf_t *va_list_inf,
        zilog_unit_t* lunit,
        va_list args
        )
{
    uint16_t reg_save_size;
    uint16_t gp_cap = args->fp_offset - (uint16_t)args->gp_offset;
    uint16_t fp_cap = 128 + args->fp_offset - (uint16_t)args->fp_offset;
    uint16_t overflow_size = lunit->size_long_double;
    uint16_t general_size = lunit->size_general;
    uint16_t double_size = lunit->size_double;
    va_list_inf->gp_off = (uint16_t)args->gp_offset;

    if(gp_cap < general_size){
        overflow_size += general_size - gp_cap;
        reg_save_size = gp_cap;
    }else{
        reg_save_size = general_size;
    }
    va_list_inf->gp_size = reg_save_size;

    if(fp_cap < double_size){
        overflow_size += (double_size - fp_cap)>>1;
        reg_save_size += fp_cap;
        va_list_inf->fp_size = fp_cap;
    }else{
        reg_save_size += double_size;
        va_list_inf->fp_size = double_size;
    }
    va_list_inf->reg_size = reg_save_size;
    va_list_inf->overflow_size = overflow_size;
    return reg_save_size+overflow_size;
}

static uint16_t calculate_arg_list_string_offset_x86_64(
        va_list_inf_t *va_list_inf,
        zilog_unit_t* lunit,
        va_list args
        ){
    uint16_t gp_cap = args->fp_offset - args->gp_offset;
    uint16_t fp_cap = 128;
    uint16_t total_str_len = 0;
    int i=0;
    do{
        uint16_t non_float_off = ((uint16_t)lunit->non_float_before_str[i] << 3);
        char* str;
        if(gp_cap > non_float_off){
            /*String is in "reg_save_area".*/
            str = *(char**)(args->reg_save_area + (uint16_t)args->gp_offset + non_float_off);
            va_list_inf->string_off[i] = non_float_off;
        }
        else{
            uint16_t overflow_arg_area_off = non_float_off - gp_cap;
            /*String is in "overflow_arg_area"*/
            uint16_t float_off = ((uint16_t)lunit->float_before_str[i] << 4);

            if(fp_cap < float_off)
                overflow_arg_area_off += ((float_off - fp_cap)>>1);

            str = *(char**)(args->overflow_arg_area + overflow_arg_area_off);
            va_list_inf->string_off[i] = va_list_inf->reg_size + overflow_arg_area_off;
        }
        va_list_inf->string[i] = str;
        va_list_inf->string_len[i] = str?strlen(str) + 1:0; /*Including ending mark '\0'.*/
        total_str_len += va_list_inf->string_len[i];
    }while(++i<lunit->n_str);

    return total_str_len;
}

static size_t calculate_required_sapce(
        va_list_inf_t* va_list_inf,
        zilog_unit_t* lunit, va_list args){
    size_t req_size;
    uint16_t str_size = 0;
    size_t packed_size;
    req_size = calculate_arg_list_size_x86_64(va_list_inf, lunit, args);
    if(lunit->n_str > 0){
        str_size = calculate_arg_list_string_offset_x86_64(va_list_inf, lunit, args);
        req_size += str_size;
    }
    req_size += sizeof(zilog_content_header_t);
    req_size = ZILOG_ALIGN(req_size, sizeof(size_t));
    va_list_inf->size = req_size;
    packed_size = lunit->arg_packed_size + str_size;
    packed_size = ZILOG_ALIGN(packed_size, sizeof(size_t));
    va_list_inf->packed_size = packed_size + sizeof(zilog_content_header_t);
    return req_size;
}

#if !ZILOG_CONFIG_PACKED

static void local_memcpy(uint64_t* dst, uint64_t* src, uint16_t len){
    while(len >= 64)
    {
        len-=64;
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst[4] = src[4];
        dst[5] = src[5];
        dst[6] = src[6];
        dst[7] = src[7];
        dst+=8;
        src+=8;
    }

    if(len >= 32)
    {
        len-=32;
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst+=4;
        src+=4;
    }

    while(len > 0)
    {
        *dst++ = *src++;
        len -= 8;
    }
}

static void local_memcpy_gp(uint64_t* dst, uint64_t* src, uint16_t len){
    if(len >= 32)
    {
        len-=32;
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst+=4;
        src+=4;
    }

    while(len > 0)
    {
        *dst++ = *src++;
        len -= 8;
    }
}

static void local_memcpy_fp(uint64_t* dst, uint64_t* src, uint16_t len){
    while(len >= 32)
    {
        len-=32;
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst+=4;
        src+=4;
    }

    if(len == 16)
    {
        dst[0] = src[0];
        dst[1] = src[1];
    }
}

static size_t write_args(zilog_content_header_t* content_header, uint8_t* buf, va_list_inf_t* va_list_inf,
        zilog_unit_t* lunit, va_list args) {

    size_t offset;

    content_header->gp_off = va_list_inf->gp_off;
    content_header->gp_size = va_list_inf->gp_size;
    content_header->reg_size = va_list_inf->reg_size;

    content_header->size = va_list_inf->size;
    content_header->lunit = lunit;
    //content_header->__jiffies = *jiffies;
    //printf("\t%ld", content_header->__jiffies);
    if(va_list_inf->gp_size == args->fp_offset - args->gp_offset)
        local_memcpy((uint64_t*)buf, args->reg_save_area + args->gp_offset, va_list_inf->reg_size);
    else{
        local_memcpy_gp((uint64_t*)buf, args->reg_save_area + args->gp_offset, va_list_inf->gp_size);
        if(va_list_inf->fp_size > 0)
            local_memcpy_fp((uint64_t*)(buf + va_list_inf->gp_size), args->reg_save_area + args->fp_offset, va_list_inf->fp_size);
    }
    offset = va_list_inf->reg_size;
    if(va_list_inf->overflow_size > 0){
        local_memcpy((uint64_t*)(buf + offset), args->overflow_arg_area, va_list_inf->overflow_size);
        offset+=va_list_inf->overflow_size;
    }
    if(lunit->n_str > 0){
        int i;
        for(i=0;i<lunit->n_str;i++){
            if(va_list_inf->string_len[i] > 0){
                char** p_str = (char**)(buf + va_list_inf->string_off[i]);
                char* str = (char*)buf + offset;
                memcpy(str, va_list_inf->string[i], va_list_inf->string_len[i]);
                *p_str = str;
                offset += va_list_inf->string_len[i];
            }
        }
    }
    assert(va_list_inf->size == ZILOG_ALIGN(sizeof(zilog_content_header_t) + offset, sizeof(size_t)));
    return offset;
}

#else
static size_t write_args_packed(zilog_content_header_t* content_header, uint8_t* buf, va_list_inf_t* va_list_inf,
        zilog_unit_t* lunit, va_list args) {

    size_t offset = 0;

#define ZILOG_WRITE_ARG(__typed, __types) \
    do{\
        __typed argd = (__typed)va_arg(args, __types);\
        /*offset = ZILOG_ALIGN(offset, sizeof(__typed));*/ \
        /**(__typed*)(buf + offset) = (__typed)va_arg(args, __types);*/ \
        memcpy(buf + offset, &argd, sizeof(__typed));\
        offset += sizeof(__typed); \
    }while(0)

    size_t total_size = lunit->arg_packed_size;
    int i;
    uint16_t strcnt = 0;
    content_header->size = va_list_inf->packed_size;
    content_header->lunit = lunit;

    for (i = 0; i < lunit->n_arg; i++) {
        switch (lunit->arg_type[i]) {
        case ZILOG_INT:
            ZILOG_WRITE_ARG(int, int);
            ZILOG_INTERNAL_DEBUG("%d\t", *(int*)(buf + offset - sizeof(int)));
            break;
        case ZILOG_LONG:
            ZILOG_WRITE_ARG(long, long);
            ZILOG_INTERNAL_DEBUG("%ld\t", *(long*)(buf + offset - sizeof(long)));
            break;
#ifndef __x86_64__
        case ZILOG_LONG_LONG:
            ZILOG_WRITE_ARG(long long, long long);
            ZILOG_INTERNAL_DEBUG("%ld\t", *(long long*)(buf + offset - sizeof(long long)));
            break;
#endif
        case ZILOG_SHORT:
            ZILOG_WRITE_ARG(short, int);
            ZILOG_INTERNAL_DEBUG("%hd\t", *(short*)(buf + offset - sizeof(short)));
            break;
        case ZILOG_CHAR:
            ZILOG_WRITE_ARG(char, int);
            ZILOG_INTERNAL_DEBUG("%d\t", *(char*)(buf + offset - sizeof(char)));
            break;
        case ZILOG_FLOAT:
            ZILOG_WRITE_ARG(float, double);
            ZILOG_INTERNAL_DEBUG("%f\t", *(float*)(buf + offset - sizeof(float)));
            break;
        case ZILOG_DOUBLE:
            ZILOG_WRITE_ARG(double, double);
            ZILOG_INTERNAL_DEBUG("%f\t", *(double*)(buf + offset - sizeof(double)));
            break;
        case ZILOG_STRING:
            {
                const char* str = va_arg(args, const char*);
                size_t str_len = va_list_inf->string_len[strcnt++];
                ZILOG_INTERNAL_DEBUG("%s\t", str);
                memcpy(buf + total_size, str, str_len);
                total_size += str_len;
            }
            break;
        case ZILOG_POINTER:
            ZILOG_WRITE_ARG(void*, void*);
            break;
 //       case ZILOG_WCHAR:
 //           ZILOG_WRITE_ARG(wchar_t, int);
 //           break;
        default:
            ;
        }
    }

    assert(ZILOG_ALIGN(total_size, sizeof(size_t)) == va_list_inf->packed_size - sizeof(zilog_content_header_t));
    return total_size;
#undef ZILOG_WRITE_ARG
}
#endif

#else

static size_t calculate_required_sapce(
        va_list_inf_t* va_list_inf,
        zilog_unit_t* lunit,
        va_list args){
    size_t req_size;
    uint16_t i;
    req_size = lunit->arg_total_size;
    for(i=0;i<lunit->n_str;i++){
        const char* str = *(char**)((uint8_t*)args + lunit->str_off[i]);
        size_t str_size = strlen(str) + 1;
        va_list_inf->string_len[i] = str_size;
        va_list_inf->string[i] = str;
        req_size += str_size;
    }
    req_size += sizeof(zilog_content_header_t);
    req_size = ZILOG_ALIGN(req_size, sizeof(size_t));
    return req_size;
}

static size_t write_args(uint8_t* buf, va_list_inf_t* va_list_inf,
        zilog_unit_t* lunit, va_list args)
{
    int i;
    uint8_t* org_buf = buf;
    size_t total_size = lunit->arg_total_size;
    memcpy(buf, (void*)args, total_size);
    for(i=0;i<lunit->n_str;i++){
        const char* str = *(char**)((uint8_t*)org_buf + lunit->str_off[i]);
        uint8_t** real_str = (uint8_t**)(org_buf + lunit->str_off[i]);
        size_t str_size = va_list_inf->string_len[i];
        buf = org_buf + total_size;
        total_size += str_size;
        memcpy(buf, str, str_size);
        *real_str = buf;
    }
    if(0)
    {
        va_list args_fork;
        __asm__(
            "mov %[args_fork], %[org_buf]"
            : [args_fork] "=r" (args_fork)
            : [org_buf] "r" (org_buf)
            :
        );
        vprintf(lunit->format_str, args_fork);
    }
    return total_size;
}
#endif

#define ZILOG_LOCK_BLOCK(__block_lock) __sync_fetch_and_add(__block_lock, 2)
#define ZILOG_UNLOCK_BLOCK(__block_lock) __sync_fetch_and_add(__block_lock, -2)

static uint8_t* start_with_new_block(
        volatile size_t** lock,
        zilog_buf_t* lbuf,
        size_t required_space_size,
        size_t free_space_size,
        size_t write_offset
        ) {
    uint8_t* buf;
    zilog_block_header_t* block_header;
    size_t write_offset_updated;
    write_offset_updated = write_offset + free_space_size;
    assert(ZILOG_BLOCK_OFFSET(write_offset_updated) == ZILOG_THREAD_BUFFER_BLOCK_SIZE);
    block_header = (zilog_block_header_t*)(lbuf->buf + ZILOG_BOUNDED_OFFSET(write_offset_updated));


    write_offset_updated += sizeof(zilog_block_header_t);

    buf = lbuf->buf + ZILOG_BOUNDED_OFFSET(write_offset_updated);
    write_offset_updated = write_offset_updated + required_space_size;
    /*while(write_offset_updated - lbuf->read_offset > ZILOG_MAX_BUFFER_WR_GAP){
        syscall(SYS_sched_yield);
    }*/
    while(block_header->lock)
        syscall(SYS_sched_yield);
    ZILOG_LOCK_BLOCK(&block_header->lock);
    /*To do, ABA problem impact analyze ?*/
    if(__sync_bool_compare_and_swap(&lbuf->write_offset, write_offset, write_offset_updated)){
        //assert(block_header->write_sequence_num == ZILOG_SEQUENCE_NUM(write_offset_updated) - ZILOG_THREAD_BUFFER_BLOCK_NUM);
        block_header->write_sequence_num = ZILOG_SEQUENCE_NUM(write_offset_updated);
        *lock = &block_header->lock;
        return buf;
    }
    else{
        ZILOG_UNLOCK_BLOCK(&block_header->lock);
        return NULL;
    }
}

static uint8_t* get_space_from_log_buffer(zilog_priority_t priority, volatile size_t** lock,
        va_list_inf_t* va_list_inf,
        zilog_unit_t* lunit, va_list args, zilog_buf_t* log_buf) {
    uint8_t* buf;
    size_t required_space_size;
    size_t block_offset;
    size_t free_space_size;
    required_space_size =
            calculate_required_sapce(
                    va_list_inf,
                    lunit, args);
#if ZILOG_CONFIG_PACKED
    required_space_size = va_list_inf->packed_size;
#endif
    if(required_space_size > ZILOG_THREAD_BUFFER_BLOCK_LOAD_SIZE)
        /*Unable to allocate space due to a huge size required.*/
        return NULL;
    do{
        size_t write_offset;
        size_t write_secquence_unm_expected;
        zilog_block_header_t* block_header;
        static const size_t drop_threshold_map[] = {
            0, /*never drop*/
            0, /*never drop*/
            0, /*never drop*/
            ZILOG_MAX_BUFFER_WR_GAP, /*drop when buffer is full*/
            ZILOG_MAX_BUFFER_WR_GAP - ZILOG_THREAD_BUFFER_BLOCK_SIZE * 8,
            ZILOG_MAX_BUFFER_WR_GAP - ZILOG_THREAD_BUFFER_BLOCK_SIZE * 16,
        };
        size_t drop_threshold = drop_threshold_map[priority];
        write_offset = log_buf->write_offset;
        //assert(write_offset - read_offset <= ZILOG_THREAD_BUFFER_SIZE);
        block_offset =
                ZILOG_BLOCK_OFFSET(write_offset);
        free_space_size =
                ZILOG_THREAD_BUFFER_BLOCK_SIZE - block_offset;

        //printf("%ld, %ld , %ld, %ld\n", required_space_size, write_offset, log_buf->read_offset, required_space_size + write_offset - log_buf->read_offset);
        /*-1 is to handle a boundary case that free_space_size is 0.*/
        write_secquence_unm_expected = ZILOG_SEQUENCE_NUM(write_offset - 1);
        block_header = (zilog_block_header_t*)(log_buf->buf + ZILOG_CAST_OFFSET(write_offset));

        if(write_secquence_unm_expected != block_header->write_sequence_num){
            //return NULL;
            syscall(SYS_sched_yield);
            /*It is a new block which was allocated by other thread.*/
            /*SynchronizationcPoint 1: Wait here until all content are completely written into this block.*/
            continue;
        }

        if (required_space_size > free_space_size) {
            size_t write_offset_updated = write_offset + free_space_size + sizeof(zilog_block_header_t) + required_space_size;
            if(drop_threshold &&  (write_offset_updated > drop_threshold + log_buf->read_offset)){
                /*drop*/
                printf("drop priority: %d\t", priority);
                return NULL;
            }
            /*Wait until buffer frees.*/
            SLEEP_TO_WAIT(write_offset_updated > ZILOG_MAX_BUFFER_WR_GAP + log_buf->read_offset);
            if(free_space_size >= sizeof(zilog_content_header_t))
                /*Need to set a ending flag in the last content header, so the block must be locked to notify reader the content is not ready yet.*/
                ZILOG_LOCK_BLOCK(&block_header->lock);
            buf = start_with_new_block(lock, log_buf, required_space_size, free_space_size, write_offset);
            if(buf){
                //assert(log_buf->write_offset - log_buf->read_offset <= ZILOG_MAX_BUFFER_WR_GAP);
                if(free_space_size >= sizeof(zilog_content_header_t)){
                    zilog_content_header_t* content_header =
                            (zilog_content_header_t*)(log_buf->buf + ZILOG_BOUNDED_OFFSET(write_offset));
                    content_header->size = 0;
                    ZILOG_UNLOCK_BLOCK(&block_header->lock);
                }
                /*Updated log_buf as there isn't any other thread updated it.*/
                return buf;
            }
            else{
                if(free_space_size >= sizeof(zilog_content_header_t))
                    ZILOG_UNLOCK_BLOCK(&block_header->lock);
                /*Another thread updated log_buf, so recalculate free_space_size.*/
                continue;
            }
        }else{
            size_t write_offset_updated;
            buf = log_buf->buf + ZILOG_BOUNDED_OFFSET(write_offset);
            write_offset_updated = write_offset + required_space_size;

            if(drop_threshold && (write_offset_updated > drop_threshold + log_buf->read_offset)){
                /*drop*/
                printf("drop priority: %d\t", priority);
                return NULL;
            }
            /*SynchronizationcPoint 1: Wait here until all content has been completely written into this block.*/
            SLEEP_TO_WAIT(write_offset_updated > ZILOG_MAX_BUFFER_WR_GAP + log_buf->read_offset);
            ZILOG_LOCK_BLOCK(&block_header->lock);
            if(__sync_bool_compare_and_swap(&log_buf->write_offset, write_offset, write_offset_updated)){
                *lock = &block_header->lock;
                return buf;
            }
            else{
                /*Retry.*/
                ZILOG_UNLOCK_BLOCK(&block_header->lock);
                continue;
            }
        }

    }while(1);

    return NULL;
}
/*
 * 1 Calculate required space.
 * 2 Allocate space from log buffer.
 *      if block.busy > 0
 *          if
 *      2-1 Check if this block is busy, that means at least one thread is writing on it.
 *      2-2 if it is busy, wait until it is not busy.
 *      2-3 if it is not busy, try allocate space from it, and increase a busy lock of it.
 * 3 Write arguments to the allocated space.
 * */
/*1st call to start writing content, and a content header should be there in the beginning of the buffer.*/
int zilog_write_arguments(zilog_priority_t priority, zilog_unit_t* lunit, const char* __restrict fmt, ...) {
    zilog_content_header_t* content_header;
    zilog_buf_t* log_buf = &lbuf;
    va_list_inf_t va_list_inf;
    volatile size_t* lock;
    va_list args;
    va_start(args, fmt);
    uint8_t* buf = get_space_from_log_buffer(priority, &lock, &va_list_inf, lunit, args, log_buf);
    if(buf == NULL)
        return 1;

    content_header = (zilog_content_header_t*)buf;
#if ZILOG_CONFIG_PACKED
    write_args_packed(content_header,
            buf + sizeof(zilog_content_header_t),
            &va_list_inf,
            lunit, args);
#else
    write_args(content_header,
            buf + sizeof(zilog_content_header_t),
            &va_list_inf,
            lunit, args);
#endif
    assert( ZILOG_CAST_OFFSET(buf - log_buf->buf) == ZILOG_CAST_OFFSET( (buf - log_buf->buf) + content_header->size));
    ZILOG_UNLOCK_BLOCK(lock);
    va_end(args);

    return 0;
}

static int initialize_debugLevels() {
    int i;
    for (i = 0; i < ZILOG_MAX_SESSION_NUM; i++)
        zilog_debugLevels[i] = ZILOG_DEFAULT_DEBUG_LEVEL;
    return i;
}

int set_debug_level(int session, int level) {
    if (session >= ZILOG_MAX_SESSION_NUM)
        return -1;
    zilog_debugLevels[session] = level;
    if (level > ZILOG_MAX_DEBUG_LEVEL)
        level = ZILOG_MAX_DEBUG_LEVEL;
    return level;
}

static uint16_t get_arguments_size_by_type(zilog_unit_t* lunit, int type, uint8_t* new_type) {

#define HAS_FLAG(_type) ((_type)&PA_FLAG_MASK)
#define IS_LONG_DOUBLE(_type)  ((_type)&PA_FLAG_LONG_DOUBLE)
#define IS_LONG(_type) ((_type)&PA_FLAG_LONG)
#define IS_LONG_LONG(_type) ((_type)&PA_FLAG_LONG_LONG)
#define IS_SHORT(_type) ((_type)&PA_FLAG_SHORT)
#define IS_STRING(type) ((type == PA_STRING))
#define BASE_TYPE_MASK 0xff
#define BASE_TYPE(_type) ((_type)&BASE_TYPE_MASK)
#define BASE_TYPE_NUM 8

    static const uint16_t sizes[] = { sizeof(int), sizeof(char),
            sizeof(wchar_t), sizeof(char*), sizeof(wchar_t*), sizeof(void*),
            sizeof(double), sizeof(double), };

    uint16_t size;
    int base_type = BASE_TYPE(type);
    assert(base_type < BASE_TYPE_NUM);
    switch (base_type) {
    case PA_INT:
#ifdef __x86_64__
        if(IS_LONG_LONG(type) || IS_LONG(type)){
            size = sizeof(long);
            *new_type = ZILOG_LONG;
            lunit->size_general += 8;
            break;
        }
#else
        if(IS_LONG_LONG(type)){
            size = sizeof(long long);
            *new_type = ZILOG_LONG_LONG;
            break;
        }
        if (IS_LONG(type)) {
            size = sizeof(long);
            *new_type = ZILOG_LONG;
            break;
        }
#endif
        if (IS_SHORT(type)) {
            *new_type = ZILOG_SHORT;
            size = sizeof(short);
#ifdef __x86_64__
            lunit->size_general += 8;
#endif
            break;
        }
        *new_type = ZILOG_INT;
#ifdef __x86_64__
        lunit->size_general += 8;
#endif
        size = sizeof(int);
        break;
    case PA_CHAR:
        *new_type = ZILOG_CHAR;
#ifdef __x86_64__
        lunit->size_general += 8;
#endif
        size = sizeof(char);
        break;
    case PA_FLOAT:
        if (IS_LONG_DOUBLE(type)) {
            *new_type = ZILOG_LONG_DOUBLE;
#ifdef __x86_64__
            lunit->size_long_double += 16;
#endif
            size = sizeof(long double);
        } else {
            *new_type = ZILOG_DOUBLE;
#ifdef __x86_64__
            lunit->size_double += 16;
#endif
            size = sizeof(float);
        }
        break;
    case PA_DOUBLE:
        if (IS_LONG_DOUBLE(type)) {
            *new_type = ZILOG_LONG_DOUBLE;
            size = sizeof(long double);
#ifdef __x86_64__
            lunit->size_long_double += 16;
#endif
        } else {
            *new_type = ZILOG_DOUBLE;
            size = sizeof(double);
#ifdef __x86_64__
            lunit->size_double += 16;
#endif
        }
        break;
    case PA_STRING:
        *new_type = ZILOG_STRING;
#ifdef __x86_64__
        lunit->size_general += 8;
#endif
#ifdef __x86_64__
        /*Strings are copied in the end of the content buffer, so the size is 0 to save memory space.*/
        size = 0;//sizeof(const char*);
#else
        size = sizeof(const char*);
#endif
        break;
    case PA_WSTRING:
    case PA_POINTER:
        *new_type = ZILOG_POINTER;
        size = sizeof(void*);
#ifdef __x86_64__
        lunit->size_general += 8;
#endif
        break;
    case PA_WCHAR:
        *new_type = ZILOG_INT;
        size = sizes[base_type];
#ifdef __x86_64__
        lunit->size_general += 8;
#endif
        break;
    default:
        break;
    }
    return size;
}

static uint16_t calculate_arguments_total_size(zilog_unit_t* lunit, int* arg_type) {
    uint16_t total_size = 0;
    uint16_t packed_size = 0;
#ifdef __x86_64__
    uint8_t non_float_num = 0;
    uint8_t float_num = 0;
    uint8_t n_str = 0;
#endif
    int i;
    if(lunit->n_arg){
        for (i = 0; i < lunit->n_arg; i++) {
            uint16_t size = get_arguments_size_by_type(lunit, arg_type[i], &lunit->arg_type[i]);
#if !ZILOG_CONF_COMPLEX_TYPES
            lunit->arg_size[i] = size;
#endif
            if(i > 0)
                total_size = ZILOG_ALIGN(total_size, size);
            if(ZILOG_STRING == lunit->arg_type[i]){
#ifdef __x86_64__
                lunit->non_float_before_str[n_str] = non_float_num;
                lunit->float_before_str[n_str] = float_num;
#else
                lunit->str_off[n_str] = total_size;
#endif
                n_str++;
            }
#ifdef __x86_64__
            if( ZILOG_DOUBLE != lunit->arg_type[i] &&
                ZILOG_LONG_DOUBLE != lunit->arg_type[i] &&
                ZILOG_FLOAT != lunit->arg_type[i])
                non_float_num += 1;
            else
                float_num += 1;
#endif
            /*It is an aligned size.*/
            total_size += size;
            packed_size += size;
        }
    }
    lunit->n_str = n_str;
    lunit->arg_total_size = total_size;
    lunit->arg_packed_size = packed_size;
    assert(total_size < 256);
    return total_size;
}

void initialize_zilog_unit(zilog_unit_t* lunit, const char* __restrict file,
        const char* __restrict fun, int line, const char* __restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);
    static size_t base_id = 0;
    int arg_type[ZILOG_MAX_ARG_NUM];
    size_t first_access;

    first_access = __sync_fetch_and_add((size_t*)&lunit->format_str, 1);
    if(first_access != 0){
        /*Wait until another thread initialized 'lunit'.*/
        while(lunit->format_str != fmt);
        return;
    }

    lunit->id = __sync_fetch_and_add(&base_id, 1);
    if (lunit->id == 0) {
        /*Initialize the log library as this is the first call of it.*/
        initialize_debugLevels();
        initialize_thread_buffers();
    }
    while(lbuf.init_flag == 0)
        usleep(0);
    lunit->n_arg = parse_printf_format(fmt, strlen(fmt), arg_type);
    lunit->file = file;
    lunit->fun = fun;
    lunit->line = line;
#ifdef __x86_64__
    lunit->size_double = 0;
    lunit->size_long_double = 0;
    lunit->size_general = 0;
#else
    lunit->n_str = 0;
#endif
    calculate_arguments_total_size(lunit, arg_type);
    lunit->format_str = fmt;
}
