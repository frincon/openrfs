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
#include "utils.h"
#include "client.h"

#define _CLIENT_SLEEP_SECONDS 1

pthread_t _client_thread;

bool _client_is_stopped = false;

void *
_client_run (void *ptr)
{
  int sock;
  struct sockaddr_in servername;
  char buffer[MAGIC_LEN];
  int bytes_readed;

  utils_debug ("Client start and running");
  do
    {
      /* Create the socket. */
      sock = socket (PF_INET, SOCK_STREAM, 0);
      if (sock < 0)
	{
	  utils_fatal ("Error creating socket: %s", strerror (errno));
	  exit (EXIT_FAILURE);
	}

      /* Connect to the server. */
      init_sockaddr (&servername, config.peer_name, config.peer_port);
      if (0 >
	  connect (sock, (struct sockaddr *) &servername,
		   sizeof (servername)))
	{
	  utils_error ("Error connecting to %s:%i: %s ", config.peer_name,
		       config.peer_port, strerror (errno));
	  utils_debug ("Sleeping client for %i seconds",
		       _CLIENT_SLEEP_SECONDS);
	  close (sock);
	  sleep (_CLIENT_SLEEP_SECONDS);
	}
      else
	{

	  utils_debug ("Client connect to peer, sending magic");
	  //Send Magic
	  write (sock, MAGIC_SERVER, MAGIC_LEN - 1);

	  //Recive Magic
	  bytes_readed = read (sock, buffer, MAGIC_LEN - 1);
	  buffer[MAGIC_LEN - 1] = '\0';
	  if (bytes_readed != MAGIC_LEN - 1)
	    {
	      utils_error
		("Receive bad magic number from peer, closing socket");
	      close (sock);
	    }
	  utils_debug ("Client receive good magic. Start sender thread");
	  if (strcmp (MAGIC_SERVER, buffer) == 0)
	    {
	      //The magic is OK, start send thread
	      pthread_t thread;
	      if (0 > sender_init (sock, &thread))
		{
		  utils_error ("Error starting sender");
		  close (sock);
		}

	      utils_debug ("Client wait until sender thread finish");
	      //Wait for thread to termitate
	      pthread_join (thread, NULL);
	    }
	  else
	    {
	      utils_error
		("Receive bad magic number from peer, closing socket");
	      close (sock);
	    }
	}
    }
  while (!_client_is_stopped);
  return NULL;
}

/*
 * Start client socket and sender thread
 */
void
client_init (pthread_t * pthread)
{

  int ret;
  ret = pthread_create (&_client_thread, NULL, _client_run, NULL);
  if (pthread != NULL)
    *pthread = _client_thread;
}

void
client_stop ()
{
  utils_debug ("Client stopping");
  _client_is_stopped = true;
  sender_stop ();
  pthread_join (_client_thread, NULL);
}
