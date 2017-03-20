/*
 * libpete.c
 *
 *  Created on: Mar 3, 2015
 *      Author: root
 */
#include "libpete.h"

int64_t  timeval_subtract(struct timeval* result, struct timeval* x, struct timeval* y)
{
        if(x->tv_sec>y->tv_sec)
                return -1;

        if((x->tv_sec==y->tv_sec) && (x->tv_usec>y->tv_usec))
                return -1;

        result->tv_sec = (y->tv_sec - x->tv_sec);
        result->tv_usec = (y->tv_usec - x->tv_usec);

        if(result->tv_usec<0)
        {
                result->tv_sec--;
                result->tv_usec+=1000000;
        }
       return  result->tv_sec * 1000000 + result->tv_usec;
}
