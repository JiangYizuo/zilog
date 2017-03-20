/*
 * liblogging_agent.c

 *
 *  Created on: Nov 8, 2016
 *      Author: root
 */
#include<stdio.h>
#include<stdarg.h>
#include<assert.h>
#include<printf.h>
#include<malloc.h>
#include<syscall.h>
#include<pthread.h>
#include "liblogging_agent.h"

static int read_content_in_block(liblogging_buf_t *lbuf, liblogging_block_header_t* block, size_t read_offset, size_t read_window_size, ...)
{
    va_list args;
    va_start(args, read_window_size);

    while(read_window_size){
        liblogging_content_header_t *content_header = (liblogging_content_header_t*)(lbuf->buf + BOUNDED_OFFSET(read_offset));
        uint8_t* buf;
        if(content_header->size == 0 || read_window_size < sizeof(liblogging_content_header_t))
            break;
        buf = lbuf->buf + BOUNDED_OFFSET(read_offset) + sizeof(liblogging_content_header_t);
        read_window_size -= content_header->size;
        read_offset += content_header->size;
        args->fp_offset = content_header->gp_size + content_header->gp_off;
        args->gp_offset = content_header->gp_off;
        args->reg_save_area = buf - content_header->gp_off;
        args->overflow_arg_area = buf + content_header->reg_size;
        vprintf(((logging_unit_t*)content_header->lunit)->format_str, args);
    }
    va_end(args);
    return 0;
}

static int read_content_from_log_buffer(liblogging_buf_t *lbuf)
{
    size_t write_offset;
    size_t read_offset;
    liblogging_block_header_t* read_cursor = (liblogging_block_header_t*)lbuf->buf;
    while(1){
        size_t read_window_size;
        write_offset = lbuf->write_offset;
        read_offset = lbuf->read_offset;

        if(write_offset == read_offset){
            /*No content to read, continue trying. To do: use conditional wait method, loops here cost time much!*/
            syscall(SYS_sched_yield);
            continue;
        }

        /*Calculate read window*/
        while(SEQUENCE_NUM(write_offset) > SEQUENCE_NUM(read_offset)){
            read_cursor = (liblogging_block_header_t*)(lbuf->buf + CAST_OFFSET(read_offset));
            /*Wait for completed contents, lock indicates some contents are not completely written yet.*/
            while(read_cursor->lock > 0)
                syscall(SYS_sched_yield);

            read_window_size = LIBLOGGING_THREAD_BUFFER_BLOCK_SIZE - BLOCK_OFFSET(read_offset);
            read_content_in_block(lbuf, read_cursor, read_offset, read_window_size);
            read_offset += read_window_size;

            read_offset += sizeof(liblogging_block_header_t);
        }
        if(SEQUENCE_NUM(write_offset) == SEQUENCE_NUM(read_offset))
        {
           read_cursor = (liblogging_block_header_t*)(lbuf->buf + CAST_OFFSET(read_offset));
           /*Wait for completed contents, lock indicates some contents are not completely written yet.*/
           while(read_cursor->lock > 0)
               syscall(SYS_sched_yield);

            read_window_size = write_offset - read_offset;
            read_content_in_block(lbuf, read_cursor, read_offset, read_window_size);
            read_offset += read_window_size;
        }
        lbuf->read_offset = read_offset;

    }
    return 0;
}

void* liblogging_agent_read_content(void* arg)
{
    read_content_from_log_buffer(&lbuf);
    return 0;
}
