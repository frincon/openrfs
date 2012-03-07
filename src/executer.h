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

#ifndef EXECUTER_H_
#define EXECUTER_H_

typedef enum
{
  EXECUTER_MODIFY = 0, EXECUTER_DELETE = 1, EXECUTER_CREATE_DIR = 2
} executer_operation;

typedef enum
{
  EXECUTER_RUNNING = 0, EXECUTER_END = 1, EXECUTER_ERROR = 2
} executer_result;

typedef struct executer_job executer_job_t;

struct executer_job
{
  executer_operation operation;	/* Which operation should i made */
  int peer_sock;		/* Socket which I/O */
  char *file;			/* name of file relative */
  char *realpath;		/* real path of file */
  struct tm time;		/* time of operation */
    executer_result (*next_operation) (executer_job_t *);
  int exists_real;
  mode_t mode;
  void *opaque;			/* pointer opaque send to the next operation */
};

executer_result executer_work (executer_job_t * job, void *opaque);

executer_result executer_receive (int sock, mensaje * mens, const char *file);
executer_result executer_send (int sock, mensaje * mens, const char *file);

#endif /* EXECUTER_H_ */
