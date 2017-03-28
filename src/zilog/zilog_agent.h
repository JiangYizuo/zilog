/*
 * zilog_agent.h
 *
 *  Created on: Nov 8, 2016
 *      Author: JiangYizuo
 */

#ifndef ZILOG_AGENT_H_
#define ZILOG_AGENT_H_

#include "zilog.h"
#include "zilog_internal.h"
typedef struct {
    int lunit_num;
    zilog_unit_t** lunits;
}zilog_agent_t;


void* zilog_agent_read_content(void* arg);

#endif /* ZILOG_AGENT_H_ */
