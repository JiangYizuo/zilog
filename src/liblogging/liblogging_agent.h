/*
 * liblogging_agent.h
 *
 *  Created on: Nov 8, 2016
 *      Author: root
 */

#ifndef LIBLOGGING_AGENT_H_
#define LIBLOGGING_AGENT_H_
#include "liblogging.h"
#include "liblogging_internal.h"
typedef struct {
    int lunit_num;
    logging_unit_t** lunits;
}liblogging_agent_t;
#endif /* LIBLOGGING_AGENT_H_ */

void* liblogging_agent_read_content(void* arg);
