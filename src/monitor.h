/*
 * monitor.h
 *
 *  Created on: 24/02/2012
 *      Author: frincon
 */

#ifndef MONITOR_H_
#define MONITOR_H_

#include <config.h>

#include <pthread.h>

#ifdef _OPENDFS_MONITOR_FUSE
void monitor_init(const char *path, const char *mountpoint);
#else
void monitor_init(const char *path, pthread_t *thread1, pthread_t *thread2);
#endif
void monitor_stop();

#endif /* MONITOR_H_ */
