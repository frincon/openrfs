/*
 *
 * opendfs -- Open source distributed file system
 *
 * Copyright (C) 2012 by Fernando Rincon <frm.rincon@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef MONITOR_H_
#define MONITOR_H_

#include <config.h>

#include <pthread.h>

#ifdef _OPENDFS_MONITOR_FUSE
void monitor_init (const char *path, const char *mountpoint);
#else
void monitor_init (const char *path, pthread_t * thread1,
		   pthread_t * thread2);
#endif
void monitor_stop ();

#endif /* MONITOR_H_ */
