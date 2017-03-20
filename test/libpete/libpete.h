/*
 * libpete.h
 *
 *  Created on: Mar 3, 2015
 *      Author: root
 */

#ifndef LIB_PETE_H_
#define LIB_PETE_H_
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
int64_t  timeval_subtract(struct timeval* result, struct timeval* x, struct timeval* y);


#ifndef __x86_64__
#define GET_COST_USEC(expression) \
    do{\
        const char* name = #expression;\
        struct timeval t0, t1, td;\
        int64_t td_usec;\
        gettimeofday(&t0, 0);\
        expression;\
        gettimeofday(&t1, 0);\
        td_usec = timeval_subtract(&td, &t0, &t1);\
        printf("%s usec = %lld\n", name, td_usec);\
}while(0)

#define COMPARE_COST(expression1, expression2) \
    do{\
        const char* name1 = #expression1;\
        const char* name2 = #expression2;\
        struct timeval t0, t1, td;\
        int64_t td_usec1, td_usec2;\
        gettimeofday(&t0, 0);\
        expression1;\
        gettimeofday(&t1, 0);\
        td_usec1 = timeval_subtract(&td, &t0, &t1);\
        gettimeofday(&t0, 0);\
        expression2;\
        gettimeofday(&t1, 0);\
        td_usec2 = timeval_subtract(&td, &t0, &t1);\
        printf("%s usec = %lld\n", name1, td_usec1);\
        printf("%s usec = %lld\n", name2, td_usec2);\
        printf("acceleration: %2.0f%%\n", 100 * ((float)td_usec1 - (float)td_usec2)/(float)td_usec2);\
}while(0)
#else
#define GET_COST_USEC(expression) \
    do{\
        const char* name = #expression;\
        struct timeval t0, t1, td;\
        int64_t td_usec;\
        gettimeofday(&t0, 0);\
        expression;\
        gettimeofday(&t1, 0);\
        td_usec = timeval_subtract(&td, &t0, &t1);\
        printf("%s usec = %ld\n", name, td_usec);\
}while(0)

#define COMPARE_COST(expression1, expression2) \
    do{\
        const char* name1 = #expression1;\
        const char* name2 = #expression2;\
        struct timeval t0, t1, td;\
        int64_t td_usec1, td_usec2;\
        gettimeofday(&t0, 0);\
        expression1;\
        gettimeofday(&t1, 0);\
        td_usec1 = timeval_subtract(&td, &t0, &t1);\
        gettimeofday(&t0, 0);\
        expression2;\
        gettimeofday(&t1, 0);\
        td_usec2 = timeval_subtract(&td, &t0, &t1);\
        printf("%s usec = %ld\n", name1, td_usec1);\
        printf("%s usec = %ld\n", name2, td_usec2);\
        printf("acceleration: %2.0f%%\n", 100 * ((float)td_usec1 - (float)td_usec2)/(float)td_usec2);\
}while(0)
#endif
#endif /* LIB_PETE_H_ */
