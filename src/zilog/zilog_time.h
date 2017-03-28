/*
 * zilog_time.h
 *
 *  Created on: Mar 24, 2017
 *      Author: yizuo
 */

#ifndef ZILOG_TIME_H_
#define ZILOG_TIME_H_

/*
 * ffffffff81e09000 D jiffies_64
 * */
size_t* zilog_time_init(off_t jiffies_offset);
int get_rdtsc();
#endif /*ZILOG_TIME_H_ */
