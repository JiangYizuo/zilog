/*
 * liblogging.h
 *
 *It is a fast, thread-safe logging library for C, it directly dumps argument list rather than printing formated string.
 *
 *  Created on: Oct 28, 2015
 *      Author: root
 */
#include<stdint.h>
#include <unistd.h>
#include "liblogging_agent.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIBLOGGING_H_
#define LIBLOGGING_H_
#define LIBLOGGING_MAX_ARG_NUM 32
#define LIBLOGGING_MAX_SESSION_NUM 1024
#define LIBLOGGING_MAX_DEBUG_LEVEL 8
#define LIBLOGGING_DEFAULT_DEBUG_LEVEL 1
#define LIBLOGGING_INVALID_UNIT_ID 0xffffffff
#define LIGLOGGING_CONF_COMPLEX_TYPES 1

typedef enum {
	LIBLOGGING_INT = 1,
	LIBLOGGING_SHORT = (1<<1),
	LIBLOGGING_CHAR = (1<<2),
	LIBLOGGING_LONG = (1<<3),
#ifndef __x86_64__
	LIBLOGGING_LONG_LONG = (1<<4),
#endif
	LIBLOGGING_POINTER = (1<<4),
	LIBLOGGING_STRING = (1<<5),
	LIBLOGGING_DOUBLE = (1<<6),
	LIBLOGGING_LONG_DOUBLE = (1<<7),
#if LIGLOGGING_CONF_COMPLEX_TYPES
	LIBLOGGING_FLOAT,
	LIBLOGGING_UNKNOWN
#endif
}liblogging_argtype_t;

typedef struct{
    /*format_str must be the first member.*/
    const char* format_str;
    const char* file;
    const char* fun;
    int line;
    uint16_t arg_total_size;
    uint8_t n_arg;
    uint8_t n_str;
#ifdef __x86_64__
    /*Used to calculate string offset in var_args of 64bit system.*/
    uint8_t non_float_before_str[LIBLOGGING_MAX_ARG_NUM];
    uint8_t float_before_str[LIBLOGGING_MAX_ARG_NUM];
#else
    uint16_t str_off[LIBLOGGING_MAX_ARG_NUM];
#endif
    size_t id;
    uint8_t arg_type[LIBLOGGING_MAX_ARG_NUM];
#ifdef __x86_64__
    uint16_t size_double;
    uint16_t size_long_double;
    uint16_t size_general;
#endif
#if !LIGLOGGING_CONF_COMPLEX_TYPES
    uint8_t arg_size[LIBLOGGING_MAX_ARG_NUM];
#endif
} logging_unit_t;

#define LL_PRINT(__FMT, __ARG)

extern int liblogging_debugLevels[LIBLOGGING_MAX_SESSION_NUM];
void initialize_logging_unit_t(logging_unit_t* lunit, const char* file, const char* fun, int line, const char* fmt, ...);
int liblogging_write_arguments(
		logging_unit_t* lunit,
		const char* fmt,
		...
		)__attribute__((format(printf,2,3)));


#define do_debug(__SECTION, __LEVEL) \
    ((__LEVEL) <= liblogging_debugLevels[__SECTION])

#define DO_PRINTF(...) \
    do{ \
        static logging_unit_t lunit = {NULL}; \
        if(lunit.format_str == NULL) \
            initialize_logging_unit_t(&lunit, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); \
        liblogging_write_arguments(&lunit, __VA_ARGS__);\
    }while(0)

#define HP_LOGGING(__SESSION, __LEVEL, ...) \
    do{ \
        static logging_unit_t lunit = {NULL}; \
        if(lunit.format_str == NULL) \
            initialize_logging_unit_t(&lunit, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); \
        if(do_debug(__SESSION, __LEVEL)) \
		liblogging_write_arguments(&lunit, __VA_ARGS__);\
    }while(0)

#ifdef __cplusplus
}
#endif
#endif /* LIBLOGGING_H_ */
