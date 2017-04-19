/*
 * zilog_test.c
 *
 *  Created on: Nov 7, 2016
 *      Author: JiangYizuo
 */
//#include <valgrind/callgrind.h>
#include "../libpete/libpete.h"
#include <syslog.h>
#include <pthread.h>

#include "../../src/zilog/zilog.h"


#define EFFICIENCY_TEST_LOOP_ROUNDS 10000000



#define THREAD_NUM 8

#define CIRCLE_US 2500
#define FATAL_LOOP 1
#define ERROR_LOOP 2
#define WARN_LOOP 4
#define INFO_LOOP 16
#define DEBUG_LOOP 256
#define PRINT_CIRCLE 1000
#define STABLE_TEST_TIME 10000000

#define CHECK_RETVALUE(__func_call, __exp_value) \
    do{\
        const char* call_name = #__func_call;\
        int ret = __func_call;\
        if(ret != (__exp_value))\
            printf("ERROR: %s return %d\n", call_name, ret);\
    }while(0)

void test_sprintf_without_string()
{
    const char* str = "void";
    int i;
    char fbuf[256];
    float fv = 0.1;
    double dv = 0.2312;
    for(i=0;i<EFFICIENCY_TEST_LOOP_ROUNDS;i++){
        sprintf(fbuf, "%s, %s, %d, This is a test of high performance logging, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld\n",
                __FILE__, __FUNCTION__, __LINE__, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL);
        //syslog(LOG_INFO, "%s", fbuf);
    }
}

void test_sprintf_with_string()
{
    const char* str = "void";
    int i;
    char fbuf[256];
    float fv = 0.1;
    double dv = 0.2312;
    for(i=0;i<EFFICIENCY_TEST_LOOP_ROUNDS;i++){
        sprintf(fbuf, "%s, %s, %d, This is a test of high performance logging, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld, %s, %s\n",
                __FILE__, __FUNCTION__, __LINE__, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL, "first", "second");
        //syslog(LOG_INFO, "%s", fbuf);
    }
}

void test_zilog_without_string()
{
    const char* str = "void";
    int i;
    float fv = 0.1;
    double dv = 0.2312;
    //CALLGRIND_START_INSTRUMENTATION;
    for(i=0;i<EFFICIENCY_TEST_LOOP_ROUNDS;i++){
        ZILOG(1, ZILOG_PRIORITY_FATAL, "This is a test of high performance logging, %x, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld\n",
                -1, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL);
    }
    //CALLGRIND_STOP_INSTRUMENTATION;
}

void test_zilog_with_string()
{
    const char* str = "void";
    int i;
    float fv = 0.1;
    double dv = 0.2312;
    //CALLGRIND_START_INSTRUMENTATION;
    for(i=0;i<EFFICIENCY_TEST_LOOP_ROUNDS;i++){
        ZILOG(1, ZILOG_PRIORITY_FATAL, "This is a test of high performance logging, %x, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld, %s, %s\n",
                -1, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL, "first", "second");
    }
    //CALLGRIND_STOP_INSTRUMENTATION;
}
#include<assert.h>
void* loop_logging(){
    const char* str = "void";
    float fv = 0.1;
    double dv = 0.2312;
    int64_t loop_cnt = 1;
    struct timeval t0, t1, t2, td;
    int64_t td_usec;

    static volatile uint8_t aotmic_test[THREAD_NUM];
#define TEST_AOTMIC_MEM \
    aotmic_test[local_tid] = i&0xff;\
    assert(aotmic_test[local_tid] == (i&0xff));

    static size_t g_thread_id = 0;
    size_t local_tid = __sync_fetch_and_add(&g_thread_id, 1);

    printf("tid = %lu\n", pthread_self());
    aotmic_test[local_tid] = local_tid;
    gettimeofday(&t0, 0);
    t2 = t0;


    while(1){
        int64_t i;

        for (i = 0; i<FATAL_LOOP; i++){
            TEST_AOTMIC_MEM;
            ZILOG(1, ZILOG_PRIORITY_FATAL, "%d, %f, %s\n", 1, 0.2, "test");
        }
        for (i = 0; i<ERROR_LOOP; i++){
            TEST_AOTMIC_MEM;
            ZILOG(1, ZILOG_PRIORITY_ERROR, "This is a test of high performance logging, %x, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld\n",
                               -1, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL);
        }
        for (i = 0; i<WARN_LOOP; i++){
            TEST_AOTMIC_MEM;
            ZILOG(1, ZILOG_PRIORITY_WARN, "%d, %f, %s\n", 1, 0.2, "test");
        }
        for (i = 0; i<INFO_LOOP; i++){
            TEST_AOTMIC_MEM;
            ZILOG(1, ZILOG_PRIORITY_INFO, "This is a test of high performance logging, %x, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld, %s, %s\n",
                       1, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL, "first", "second");
        }
        for (i = 0; i<DEBUG_LOOP; i++){
            TEST_AOTMIC_MEM;
            ZILOG(1, ZILOG_PRIORITY_DEBUG, "This is a test of high performance logging, %x, %hu, %c, %d, %p, %3.3f, %5.5f, %ld, %lld, %s, %s\n",
                               1, (short)-1, 'a', (int)2, str, fv, dv, -7L, 8LL, "first", "second");
        }
        if(CIRCLE_US > 0)
            usleep(CIRCLE_US);
        if(loop_cnt == PRINT_CIRCLE){
            gettimeofday(&t1, 0);
            td_usec = timeval_subtract(&td, &t0, &t1);
            t0 = t1;
            printf("thread : %ld ------ logs per second: %0.0f\n", local_tid, (float)((FATAL_LOOP + ERROR_LOOP + WARN_LOOP + INFO_LOOP + DEBUG_LOOP) * PRINT_CIRCLE)*1000000/(float)td_usec);
            loop_cnt = 1;
            td_usec = timeval_subtract(&td, &t2, &t1);
            if(td_usec > STABLE_TEST_TIME){
                printf("thread : %ld, testing time is up, exit.\n", local_tid);
                (void)pthread_exit(NULL);
            }
        }else
            loop_cnt++;
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    pthread_t pthread[THREAD_NUM];

    int i;
    printf("Start to test time efficiency comparing with the std 'sprintf' method.\n");
    COMPARE_COST(test_sprintf_with_string(), test_zilog_with_string());
    COMPARE_COST(test_sprintf_without_string(), test_zilog_without_string());
    printf("Start stability test.\n");
    for(i=0;i<THREAD_NUM;i++){
        CHECK_RETVALUE(pthread_create(&pthread[i], NULL, loop_logging, NULL), 0);
        usleep(CIRCLE_US*PRINT_CIRCLE/THREAD_NUM);
    }

    for(i=0;i<THREAD_NUM;i++)
        CHECK_RETVALUE(pthread_join(pthread[i], NULL), 0);

    printf("Stability test passed.\n");
    return 0;
}
