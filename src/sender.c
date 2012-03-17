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

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "opendfs.h"
#include "queue.h"
#include "sender.h"
#include "utils.h"

void *_sender_run (void *ptr);
pthread_t _thread_sender = NULL;

int _is_sender_stopped = 0;

int
sender_init (int sock, pthread_t * pthread)
{
  int ret;
  static int sock2;
  sock2 = sock;
  _is_sender_stopped = 0;
  ret = pthread_create (&_thread_sender, NULL, _sender_run, &sock2);
  *pthread = _thread_sender;
  return EXIT_SUCCESS;
}

void
sender_stop ()
{
  utils_debug ("Sender stopping");
  _is_sender_stopped = 1;
  utils_trace ("Thread: %i", _thread_sender);
  if (_thread_sender != NULL)
    pthread_join (_thread_sender, NULL);
  utils_trace ("Thread: %i", _thread_sender);
}

void *
_sender_run (void *ptr)
{

  int sock = (*(int *) ptr);
  fflush (stdout);

  utils_debug ("Sender thread up and running");

  //Esta es la tarea de envio
  while (!_is_sender_stopped)
    {
      queue_operation operation;
      int ret;
      ret = queue_get_operation (&operation);
      // utils_trace ("Sender get operation: %i", ret);
      if (ret != 0)
	{
	  int writed;
	  //Hay operación, hay que enviar
	  mensaje mens;
	  mens.operacion = operation.operation;
	  mens.file_size = strlen (operation.file) + 1;
	  mens.time = operation.time;

	  utils_trace ("Sending message %s", operation.file);

	  writed = write (sock, &mens, sizeof (mens));
	  if (writed != sizeof (mens))
	    {
	      error (0, errno, "The socket is not valid");
	      break;
	    }

	  writed = write (sock, operation.file, strlen (operation.file) + 1);
	  if (writed != strlen (operation.file) + 1)
	    {
	      error (0, errno, "The socket is not valid");
	      break;
	    }

	  executer_send (sock, &mens, operation.file);

	  fflush (stdout);
	}
      else
	{
	  mensaje mens;
	  int writed;
	  int readed;
	  // TODO make better whith pthread
	  sleep (1);
	  // Send Ping and receive pong
	  mens.operacion = OPENDFS_PING;
	  writed = write (sock, &mens, sizeof (mens));
	  if (writed != sizeof (mens))
	    {
	      utils_warn ("The socket is not valid, sending ping");
	      break;
	    }
	  readed = read (sock, &mens, sizeof (mens));
	  if (readed != sizeof (mens))
	    {
	      utils_warn ("Error reading pong. Closing sender");
	      break;
	    }
	  if (mens.operacion != OPENDFS_PONG)
	    {
	      utils_warn ("Error, no pong received. Closing sender");
	      break;
	    }
	}
      // Check for socket is open

    }
  utils_debug ("Closing sender socket");
  close (sock);
  _thread_sender = 0;
  return NULL;
}
