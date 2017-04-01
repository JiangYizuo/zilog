/*
 * zilog_test.c
 *
 *  Created on: Nov 7, 2016
 *      Author: JiangYizuo
 */
//#include <valgrind/callgrind.h>
#include "../libpete/libpete.h"
#include <syslog.h>

#include "../../src/zilog/zilog.h"
#define LOOP_ROUNDS 1000000
void test_logging_without_string()
{
    const char* str = "void";
    int i;
    char fbuf[256];
    float fv = 0.1;
    double dv = 0.2312;
    for(i=0;i<LOOP_ROUNDS;i++){
        sprintf(fbuf, "%s, %s, %d, This is a test of high performance logging, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld\n",
                __FILE__, __FUNCTION__, __LINE__, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL);
        //syslog(LOG_INFO, "%s", fbuf);
    }
}

void test_logging_with_string()
{
    const char* str = "void";
    int i;
    char fbuf[256];
    float fv = 0.1;
    double dv = 0.2312;
    for(i=0;i<LOOP_ROUNDS;i++){
        sprintf(fbuf, "%s, %s, %d, This is a test of high performance logging, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld, %s, %s\n",
                __FILE__, __FUNCTION__, __LINE__, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL, "first", "second");
        //syslog(LOG_INFO, "%s", fbuf);
    }
}

void test_hp_logging_without_string()
{
    const char* str = "void";
    int i;
    float fv = 0.1;
    double dv = 0.2312;
    //CALLGRIND_START_INSTRUMENTATION;
    HP_LOGGING(1,1, "%d, %d, %s", 1, 2, "test");
    for(i=0;i<LOOP_ROUNDS;i++){
        //HP_LOGGING(1, 1, "This is a test of high performance logging, %hu, %p, %c, %s, %s, %s, %d, %d\n", (short)1, str, 'a', "first", "second", "third", 2, 3);
        HP_LOGGING(1, 1, "This is a test of high performance logging, %x, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld\n",
                -1, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL);
    }
    //CALLGRIND_STOP_INSTRUMENTATION;
}

void test_hp_logging_with_string()
{
    const char* str = "void";
    int i;
    float fv = 0.1;
    double dv = 0.2312;
    //CALLGRIND_START_INSTRUMENTATION;
    //HP_LOGGING(1,1, "%d, %f, %s", 1, 0.2, "test");
    for(i=0;i<LOOP_ROUNDS;i++){
        //HP_LOGGING(1, 1, "This is a test of high performance logging, %hu, %p, %c, %s, %s, %s, %d, %d\n", (short)1, str, 'a', "first", "second", "third", 2, 3);
        HP_LOGGING(1, 1, "This is a test of high performance logging, %x, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld, %s, %s\n",
                -1, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL, "first", "second");
    }
    //CALLGRIND_STOP_INSTRUMENTATION;
}

void* loop_logging(void* arg){
    const char* str = "void";
    float fv = 0.1;
    double dv = 0.2312;
    while(1){
        HP_LOGGING(1,1, "%d, %f, %s\n", 1, 0.2, "test");
        HP_LOGGING(1, 1, "This is a test of high performance logging, %x, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld\n",
                               -1, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL);
        HP_LOGGING(1,1, "%d, %f, %s\n", 1, 0.2, "test");
        HP_LOGGING(1, 1, "This is a test of high performance logging, %x, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld, %s, %s\n",
                       -1, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL, "first", "second");
        HP_LOGGING(1, 1, "This is a test of high performance logging, %x, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld, %s, %s\n",
                               -1, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL, "first", "second");
        HP_LOGGING(1, 1, "This is a test of high performance logging, %x, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld, %s, %s\n",
                               -1, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL, "first", "second");
    }
    return NULL;
}
#define THREAD_NUM 8

#include <pthread.h>
int main(int argc, char* argv[])
{
    pthread_t pthread[THREAD_NUM];

    int i;
    HP_LOGGING(1,1, "%d, %f, %s\n", 1, 0.2, "test");
    COMPARE_COST(test_logging_with_string(), test_hp_logging_with_string());
    COMPARE_COST(test_logging_without_string(), test_hp_logging_without_string());
    for(i=0;i<THREAD_NUM;i++)
        pthread_create(&pthread[i], NULL, loop_logging, NULL);

    for(i=0;i<THREAD_NUM;i++)
        pthread_join(pthread[i], NULL);

    return 0;
}
