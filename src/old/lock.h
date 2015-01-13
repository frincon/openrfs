/*
 * lock.h
 *
 *  Created on: Dec 4, 2014
 *      Author: frincon
 */

#ifndef SRC_LOCK_H_
#define SRC_LOCK_H_

#define LOCK_STATE   0x1
#define LOCK_EXISTS  0x2
#define LOCK_OPEN_RW 0x4

#define LOCK_MAX_STATES 3

typedef void locker_t;

locker_t
*lock_acquire(char *filename, int flags);

void
lock_release(locker_t *locker);

#endif /* SRC_LOCK_H_ */
