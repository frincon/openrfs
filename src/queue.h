/*
 *
 * openrfs -- Open source distributed file system
 *
 * Copyright (C) 2012 by Fernando Rincon <frm.rincon@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
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
int queue_delete_operation ();
int queue_get_operation_for_file (const char *file,
				  queue_operation * operation);
void queue_close ();

#endif /* QUEUE_H_ */
