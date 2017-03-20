/*
 * liblogging_internal.h
 *
 *  Created on: Nov 9, 2016
 *      Author: root
 */

#ifndef LIBLOGGING_INTERNAL_H_
#define LIBLOGGING_INTERNAL_H_
#include<stdint.h>

typedef struct{
    size_t write_sequence_num; /*It is used to indicate if the content of this block is changed.*/
    size_t content_num;
    size_t lock;
}liblogging_block_header_t;

typedef struct{
    uint16_t size;
#ifdef __x86_64__
    uint16_t gp_off;
    uint16_t gp_size;
    uint16_t reg_size;
#endif
    void* lunit;
}liblogging_content_header_t;

typedef struct {
    int init_flag;
    uint8_t* buf;
    liblogging_block_header_t* read_cursor;
    int read_block_index;
    size_t read_offset;
    size_t write_offset;
    size_t read_block_cnt;
    int size;
} liblogging_buf_t;

//#define LIBLOGGING_INTERNAL_DEBUG printf
#define LIBLOGGING_INTERNAL_DEBUG(...)
#define LIBLOGGING_THREAD_BUFFER_BLOCK_SIZE_SCALE 14
#define LIBLOGGING_THREAD_BUFFER_BLOCK_SIZE (1 << LIBLOGGING_THREAD_BUFFER_BLOCK_SIZE_SCALE)
#define BLOCK_OFFSET(__offset) (((__offset - 1) & (LIBLOGGING_THREAD_BUFFER_BLOCK_SIZE - 1)) + 1)

#define LIBLOGGING_THREAD_BUFFER_BLOCK_LOAD_SIZE  \
    ( LIBLOGGING_THREAD_BUFFER_BLOCK_SIZE - sizeof(liblogging_block_header_t) )
#define LIBLOGGING_THREAD_BUFFER_BLOCK_NUM 2
#define LIBLOGGING_THREAD_BUFFER_SIZE (LIBLOGGING_THREAD_BUFFER_BLOCK_SIZE*LIBLOGGING_THREAD_BUFFER_BLOCK_NUM)
#define BOUNDED_OFFSET(__offset) ((__offset) & (LIBLOGGING_THREAD_BUFFER_SIZE - 1))
#define CAST_OFFSET(__offset) BOUNDED_OFFSET((__offset) -  BLOCK_OFFSET(__offset))
#define SEQUENCE_NUM(__offset) ((__offset) >> LIBLOGGING_THREAD_BUFFER_BLOCK_SIZE_SCALE)
#define LIBLOGGING_INVALID_SEQUENCE_NUM ((size_t)-1)
#define LIBLOGGING_MAX_THREAD_NUM 16
#define USE_VA_ARG_MACRO 0
extern liblogging_buf_t lbuf;

#endif /* LIBLOGGING_INTERNAL_H_ */
