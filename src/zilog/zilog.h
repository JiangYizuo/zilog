/*
 * zilog.h
 *
 *It is a fast, thread-safe log library for C, it directly dumps argument list rather than printing formated string.
 *
 *  Created on: Oct 28, 2015
 *      Author: JiangYizuo
 */
#ifndef ZILOG_H_
#define ZILOG_H_

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZILOG_MAX_ARG_NUM 32
#define ZILOG_MAX_SESSION_NUM 1024
#define ZILOG_MAX_DEBUG_LEVEL 8
#define ZILOG_DEFAULT_DEBUG_LEVEL 128
#define ZILOG_INVALID_UNIT_ID 0xffffffff
#define ZILOG_CONF_COMPLEX_TYPES 1

typedef enum {
	ZILOG_INT = 1,
	ZILOG_SHORT = (1<<1),
	ZILOG_CHAR = (1<<2),
	ZILOG_LONG = (1<<3),
#ifndef __x86_64__
	ZILOG_LONG_LONG = (1<<4),
#endif
	ZILOG_POINTER = (1<<4),
	ZILOG_STRING = (1<<5),
	ZILOG_DOUBLE = (1<<6),
	ZILOG_LONG_DOUBLE = (1<<7),
#if ZILOG_CONF_COMPLEX_TYPES
	ZILOG_FLOAT,
	ZILOG_UNKNOWN
#endif
}zilog_argtype_t;

typedef enum {
    ZILOG_PRIORITY_FATAL = 0,
    ZILOG_PRIORITY_ERROR,
    ZILOG_PRIORITY_WARN,
    ZILOG_PRIORITY_INFO,
    ZILOG_PRIORITY_DEBUG,
    ZILOG_PRIORITY_ALL,
    ZILOG_PRIORITY_TOTAL /*Just a number of priorities.*/
}zilog_priority_t;

typedef struct{
    /*format_str must be the first member.*/
    const char* format_str;
    const char* file;
    const char* fun;
    int line;
    uint16_t arg_total_size;
    uint16_t arg_packed_size;
    uint8_t n_arg;
    uint8_t n_str;
#ifdef __x86_64__
    /*Used to calculate string offset in var_args of 64bit system.*/
    uint8_t non_float_before_str[ZILOG_MAX_ARG_NUM];
    uint8_t float_before_str[ZILOG_MAX_ARG_NUM];
#else
    uint16_t str_off[ZILOG_MAX_ARG_NUM];
#endif
    size_t id;
    uint8_t arg_type[ZILOG_MAX_ARG_NUM];
#ifdef __x86_64__
    uint16_t size_double;
    uint16_t size_long_double;
    uint16_t size_general;
#endif
#if !ZILOG_CONF_COMPLEX_TYPES
    uint8_t arg_size[ZILOG_MAX_ARG_NUM];
#endif
} zilog_unit_t;

#define LL_PRINT(__FMT, __ARG)

extern int zilog_debugLevels[ZILOG_MAX_SESSION_NUM];
void zilog_initialize_log_unit(zilog_unit_t* lunit, const char* file, const char* fun, int line, const char* fmt, ...);
int zilog_write_arguments(
        zilog_priority_t priority,
		zilog_unit_t* lunit,
		const char* fmt,
		...
		)__attribute__((format(printf,3,4)));


#define zilog_do_debug(__SECTION, __LEVEL) \
    ((__LEVEL) <= zilog_debugLevels[__SECTION])

#define DO_PRINTF(...) \
    do{ \
        static zilog_unit_t lunit = {NULL}; \
        if(lunit.format_str == NULL) \
            zilog_initialize_log_unit(&lunit, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); \
        zilog_write_arguments(&lunit, __VA_ARGS__);\
    }while(0)

#define ZILOG(__SESSION, __LEVEL, ...) \
    do{ \
        static zilog_unit_t lunit = {NULL}; \
        if(lunit.format_str == NULL) {\
            zilog_initialize_log_unit(&lunit, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); \
        }\
        if(zilog_do_debug(__SESSION, __LEVEL)){ \
            zilog_write_arguments(__LEVEL, &lunit, __VA_ARGS__);\
        }\
    }while(0)

#ifdef __cplusplus
}
#endif
#endif /* ZILOG_H_ */
