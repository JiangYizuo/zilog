/*
 * zilog_time.c
 *
 *  Created on: Mar 24, 2017
 *      Author: yizuo
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "zilog_time.h"
size_t* zilog_time_init(off_t jiffies_offset)
{
    char* p_jiffies;
    int mem_fd = open("/dev/mem", O_RDONLY);
    jiffies_offset -= 0xc000000000000000;
    p_jiffies = (char*)mmap(NULL, 0x10000, PROT_READ, MAP_SHARED, mem_fd, (off_t)(jiffies_offset - (jiffies_offset%0x10000)));
    return (size_t*)(p_jiffies + (jiffies_offset%0x10000));
}

int get_rdtsc()
{
    asm("rdtsc");
    return 0;
}
