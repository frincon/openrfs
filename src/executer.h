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

#ifndef EXECUTER_H_
#define EXECUTER_H_

#include <stdbool.h>
#include <sys/stat.h>
#include <librsync.h>

typedef enum
{
  EXECUTER_MODIFY = 0, EXECUTER_DELETE = 1, EXECUTER_CREATE_DIR =
    2, EXECUTER_DELETE_DIR = 4
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
  bool copy_conflict;
  char *tempfile;
  struct stat *file_stat;
  struct stat *file_stat_before;	/* store stat at the beginning of the job */
  rs_signature_t *signature;
};

executer_result executer_work (executer_job_t * job, void *opaque);

executer_result executer_receive (int sock, mensaje * mens, const char *file);
executer_result executer_send (int sock, mensaje * mens, const char *file);

#endif /* EXECUTER_H_ */
