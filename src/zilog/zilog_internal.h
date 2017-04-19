/*
 * zilog_internal.h
 *
 *  Created on: Nov 9, 2016
 *      Author: JiangYizuo
 */

#ifndef ZILOG_INTERNAL_H_
#define ZILOG_INTERNAL_H_
#include<stdint.h>
#include "zilog_time.h"

#define likely(x)  __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

#define ZILOG_CONFIG_PACKED 1
//#define ZILOG_INTERNAL_DEBUG printf
#define ZILOG_INTERNAL_DEBUG(...)
#define ZILOG_THREAD_BUFFER_BLOCK_SIZE_SCALE 16
#define ZILOG_THREAD_BUFFER_BLOCK_SIZE (1 << ZILOG_THREAD_BUFFER_BLOCK_SIZE_SCALE)
#define ZILOG_BLOCK_OFFSET(__offset) (((__offset - 1) & (ZILOG_THREAD_BUFFER_BLOCK_SIZE - 1)) + 1)

#define ZILOG_THREAD_BUFFER_BLOCK_LOAD_SIZE  \
    ( ZILOG_THREAD_BUFFER_BLOCK_SIZE - sizeof(zilog_block_header_t) )
#define ZILOG_THREAD_BUFFER_BLOCK_NUM (128) /*At least 32.*/
#define ZILOG_THREAD_BUFFER_SIZE (ZILOG_THREAD_BUFFER_BLOCK_SIZE*ZILOG_THREAD_BUFFER_BLOCK_NUM)
#define ZILOG_MAX_BUFFER_WR_GAP (ZILOG_THREAD_BUFFER_BLOCK_SIZE*(ZILOG_THREAD_BUFFER_BLOCK_NUM - 0))
#define ZILOG_BOUNDED_OFFSET(__offset) ((__offset) & (ZILOG_THREAD_BUFFER_SIZE - 1))
#define ZILOG_CAST_OFFSET(__offset) ZILOG_BOUNDED_OFFSET((__offset) -  ZILOG_BLOCK_OFFSET(__offset))
#define ZILOG_SEQUENCE_NUM(__offset) ((__offset) >> ZILOG_THREAD_BUFFER_BLOCK_SIZE_SCALE)
#define ZILOG_INVALID_ZILOG_SEQUENCE_NUM ((size_t)-1)
#define ZILOG_MAX_THREAD_NUM 16
#define USE_VA_ARG_MACRO 0


#define MAX_USLEEP_USEC (1 << 16)
#define INIT_USLEEP_USEC (64)

typedef struct{
    volatile size_t write_sequence_num; /*It is used to indicate if the content of this block is changed.*/
    size_t content_num;
    volatile size_t lock;
}zilog_block_header_t;

typedef struct{
    size_t __jiffies;
    uint16_t size;
#ifdef __x86_64__
#if !ZILOG_CONFIG_PACKED
    uint16_t gp_off;
    uint16_t gp_size;
    uint16_t reg_size;
#endif
#endif
    void* lunit;
}zilog_content_header_t;

typedef struct {
    int init_flag;
    uint8_t* buf;
    zilog_block_header_t* read_cursor;
    volatile size_t read_offset;
    volatile size_t write_offset;
    int size;
} zilog_buf_t;


#define SLEEP_TO_WAIT(__cond) \
    do{\
        int usec = INIT_USLEEP_USEC;\
        while(__cond){\
            usleep(usec);\
            usec = usec * 2;\
            usec = usec > MAX_USLEEP_USEC ? MAX_USLEEP_USEC : usec;\
        }\
    }while(0)

#define SLEEP_TO_WAIT_WITH_EXPRESSION(__cond, __e1, __e2) \
    do{\
        int usec = INIT_USLEEP_USEC;\
        __e1;\
        __e2;\
        while(__cond){\
            usleep(usec);\
            usec = usec * 2;\
            usec = usec > MAX_USLEEP_USEC ? MAX_USLEEP_USEC : usec;\
            __e1;\
            __e2;\
        }\
    }while(0)

extern zilog_buf_t lbuf;

#endif /* ZILOG_INTERNAL_H_ */
