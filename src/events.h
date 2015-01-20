/*
 * events.h
 *
 *  Created on: Jan 20, 2015
 *      Author: frincon
 */

#ifndef SRC_EVENTS_H_
#define SRC_EVENTS_H_

#include "config.h"

#include <time.h>

void
openrfs_on_modify(const char *path, const struct timespec *tp);

void
openrfs_on_delete(const char *path, const struct timespec *tp);

#endif /* SRC_EVENTS_H_ */
