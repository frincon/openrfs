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


#ifndef QUEUE_H_
#define QUEUE_H_

#include <config.h>

typedef struct _queue_operation
{
  char *file;
  struct tm time;
  int operation;
} queue_operation;

void queue_init ();
void queue_add_operation (queue_operation * operation);
int queue_get_operation (queue_operation * operation);
int queue_get_operation_for_file (const char *file,
				  queue_operation * operation);
void queue_close ();

#endif /* QUEUE_H_ */
