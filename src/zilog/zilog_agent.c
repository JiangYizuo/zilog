/*
 * zilog_agent.c

 *
 *  Created on: Nov 8, 2016
 *      Author: JiangYizuo
 */
#include<stdio.h>
#include<stdarg.h>
#include<assert.h>
#include<printf.h>
#include<malloc.h>
#include<syscall.h>
#include<pthread.h>
#include <syslog.h>
#include "zilog_agent.h"

static int read_content_in_block(zilog_buf_t *lbuf, zilog_block_header_t* block, size_t read_offset, size_t read_window_size, ...)
{
    va_list args;
    va_start(args, read_window_size);
    char fbuf[10240];
    /*Wait for completed contents, lock indicates some contents are not completely written yet.*/
    while(block->lock > 0)
        syscall(SYS_sched_yield);
    while(read_window_size){
        zilog_content_header_t *content_header = (zilog_content_header_t*)(lbuf->buf + ZILOG_BOUNDED_OFFSET(read_offset));
        uint8_t* buf;
        if(content_header->size == 0 || read_window_size < sizeof(zilog_content_header_t))
            break;
        buf = lbuf->buf + ZILOG_BOUNDED_OFFSET(read_offset) + sizeof(zilog_content_header_t);
        assert(read_window_size >= content_header->size);
        read_window_size -= content_header->size;
        read_offset += content_header->size;
        args->fp_offset = content_header->gp_size + content_header->gp_off;
        args->gp_offset = content_header->gp_off;
        args->reg_save_area = buf - content_header->gp_off;
        args->overflow_arg_area = buf + content_header->reg_size;
        vsnprintf(fbuf, 10240, ((zilog_unit_t*)content_header->lunit)->format_str, args);
        //syslog(LOG_INFO, "%s", fbuf);
    }
    va_end(args);
    return 0;
}

static int read_content_from_log_buffer(zilog_buf_t *lbuf)
{
    size_t write_offset;
    size_t read_offset;
    zilog_block_header_t* read_cursor = (zilog_block_header_t*)lbuf->buf;
    while(1){
        size_t read_window_size;
        write_offset = lbuf->write_offset;
        read_offset = lbuf->read_offset;
        assert(write_offset >= read_offset);
        if(write_offset == read_offset){
            /*No content to read, continue trying. To do: use conditional wait method, loops here cost CPU time much!*/
            syscall(SYS_sched_yield);
            continue;
        }

        /*Calculate read window*/
        while(ZILOG_SEQUENCE_NUM(write_offset) > ZILOG_SEQUENCE_NUM(read_offset)){

            read_window_size = ZILOG_THREAD_BUFFER_BLOCK_SIZE - ZILOG_BLOCK_OFFSET(read_offset);
            read_cursor = (zilog_block_header_t*)(lbuf->buf + ZILOG_CAST_OFFSET(read_offset));
            read_content_in_block(lbuf, read_cursor, read_offset, read_window_size);
            read_offset += read_window_size;
            read_offset += sizeof(zilog_block_header_t);
        }

        if(ZILOG_SEQUENCE_NUM(write_offset) == ZILOG_SEQUENCE_NUM(read_offset))
        {
            read_window_size = write_offset - read_offset;
            read_cursor = (zilog_block_header_t*)(lbuf->buf + ZILOG_CAST_OFFSET(read_offset));
            read_content_in_block(lbuf, read_cursor, read_offset, read_window_size);
            read_offset += read_window_size;
        }
        lbuf->read_offset = read_offset;

    }
    return 0;
}

void* zilog_agent_read_content(void* arg)
{
    read_content_from_log_buffer(&lbuf);
    return 0;
}
