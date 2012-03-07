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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

#include "opendfs.h"
#include "receiver.h"
#include "server.h"

bool _server_is_stopped = false;

pthread_t _server_thread;

int
_server_make_socket (uint16_t port)
{
  int sock;
  struct sockaddr_in name;

  /* Create the socket. */
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      exit (EXIT_FAILURE);
    }

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
      perror ("bind");
      exit (EXIT_FAILURE);
    }

  return sock;
}

/*
 * Start the receive thread and wait for it
 */
int
_server_start_receive (int sock)
{
  pthread_t receiver_thread;
  receiver_init (sock, &receiver_thread);

  pthread_join (receiver_thread, NULL);
  return EXIT_SUCCESS;
}

/*
 * The thread function, read for clients
 */
void *
_server_run (void *ptr)
{
  int *temp = (int *) ptr;
  int sock = *temp;
  fd_set active_fd_set, read_fd_set;
  struct sockaddr_in clientname;
  socklen_t size;

  FD_ZERO (&active_fd_set);
  FD_SET (sock, &active_fd_set);

  read_fd_set = active_fd_set;
  while (!_server_is_stopped)
    {
      int new_socket;
      char buffer[MAGIC_LEN];
      int bytes_readed;
      new_socket = accept (sock, (struct sockaddr *) &clientname, &size);
      if (new_socket < 0)
	error (EXIT_FAILURE, errno, "Error accepting connection");

      fprintf (stderr, MY_NAME ":%s:%s: Connect from host %s, port %hd.\n",
	       __FILE__, __FUNCTION__, inet_ntoa (clientname.sin_addr),
	       ntohs (clientname.sin_port));

      bytes_readed = read (new_socket, buffer, MAGIC_LEN - 1);
      buffer[MAGIC_LEN - 1] = '\0';
      if (bytes_readed != MAGIC_LEN - 1)
	{
	  error (0, 0, "Receive bad magic");
	  close (new_socket);
	}
      if (strcmp (MAGIC_SERVER, buffer) == 0)
	{
	  //Send magic
	  write (new_socket, MAGIC_SERVER, MAGIC_LEN - 1);

	  //Ok start receive thread
	  if (_server_start_receive (new_socket) != 0)
	    {
	      error (EXIT_FAILURE, 0, "Error starting receive thread");
	    }
	}
      else
	{
	  error (0, 0, "Receive bad magic");
	  close (new_socket);
	}

    }
  //Close de socket listening
  close (sock);
  return NULL;
}

/*
 * Init listen on port specified by configuration
 */
void
server_init (pthread_t * pthread)
{

  static int sock;

  // Arrancamos el servidor
  sock = _server_make_socket (config.port);
  if (listen (sock, 1) < 0)
    {
      error (EXIT_FAILURE, errno, "Error opening socket");
    }

  int ret;
  ret = pthread_create (&_server_thread, NULL, _server_run, &sock);
  if (pthread != NULL)
    *pthread = _server_thread;
}

void
server_stop ()
{
  _server_is_stopped = true;
  receiver_stop ();
}