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

#ifdef _OPENDFS_MONITOR_INOTIFY

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <error.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <signal.h>
#include <string.h>

#include <poll.h>

#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/queue.h>
#include "monitor.h"
#include "opendfs.h"

#include "queue.h"

/* Inicializamos la cola de eventos */

TAILQ_HEAD (tail1head, entry) head;
     struct entry
     {
       TAILQ_ENTRY (entry) entries;
       struct inotify_event inot_ev;
     };

/* Estructura para guardar los watches */
     struct watch
     {
       int fd;
       char *path;
     };

/* Lista para guardar los watches */

     static LIST_HEAD (listhead, listentry) head2;
     struct listentry
     {
       LIST_ENTRY (listentry) entries2;
       struct watch wd;
     };

/* prototipos de las funciones internas */
     void *run (void *ptr);
     void *run2 (void *ptr);
     void addwatch (const char *path);
     int read_events ();
     void handle_event (struct entry *event);
     void inserta_lista (int fd, const char *path);

/* Cuando es 0 el programa debe parar */
     int isStop = 1;
/* contiene el file descriptor de inotify */
     int inotify_fd;

/* las dos pthread que se abren */
     pthread_t thread_inotify;
     pthread_t thread_queue;

/* Los mutex y condiciones */
     pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
     pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* numero de eventos que hay en la cola */
     int events = 0;

     char *_queue_path_monitor;

     void monitor_init (const char *path, pthread_t * thread1,
			pthread_t * thread2)
{
  int ret;
  inotify_fd = inotify_init ();
  if (inotify_fd == -1)
    {
      error (EXIT_FAILURE, 0, "Error inicializando inotify");
    }

  // Add the inotyfy watch recursively
  TAILQ_INIT (&head);
  LIST_INIT (&head2);
  _queue_path_monitor = malloc (strlen (path) + 1);
  strcpy (_queue_path_monitor, path);
  addwatch (path);

  ret = pthread_create (&thread_inotify, NULL, run, (void *) &inotify_fd);
  ret = pthread_create (&thread_queue, NULL, run2, NULL);
  *thread1 = thread_inotify;
  *thread2 = thread_queue;
}

void
addwatch (const char *path)
{
  DIR *dp;
  int ret;
  struct dirent *dirp;
  if ((dp = opendir (path)) == NULL)
    {
      error (EXIT_FAILURE, 0, "Error(%i) opening %s", errno, path);
    }

  while ((dirp = readdir (dp)) != NULL)
    {
      if (dirp->d_type == DT_DIR && strcmp (dirp->d_name, ".") != 0
	  && strcmp (dirp->d_name, "..") != 0)
	{
	  if (!(strcmp (path, config.path) == 0
		&& strcmp (dirp->d_name, config.conflicts)))
	    {
	      char path2[512];
	      strcpy (path2, path);
	      strcat (path2, "/");
	      strcat (path2, dirp->d_name);
	      addwatch (path2);
	    }

	}
    }
  closedir (dp);

  ret = inotify_add_watch (inotify_fd,
			   path,
			   IN_ATTRIB | IN_CLOSE_WRITE | IN_CREATE | IN_DELETE
			   | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF |
			   IN_MOVED_FROM | IN_MOVED_TO);

  //En la lista no ponemos el path relativo
  if (strcmp (_queue_path_monitor, path) == 0)
    {
      inserta_lista (ret, "");
    }
  else
    {
      inserta_lista (ret, path + strlen (_queue_path_monitor) + 1);
    }

}

char *
busca_lista (int fd)
{
  struct watch *w;
  struct listentry *entry;
  // Bien, tenemos que buscar en una lista ordenada de mayor a menor
  // realizar con algún algoritmo mejorado

  LIST_FOREACH (entry, &head2, entries2)
  {
    if (entry->wd.fd == fd)
      {
	w = &(entry->wd);
	return w->path;
      }
  }
  return NULL;
}

void
inserta_lista (int fd, const char *path)
{
  struct watch *w;
  struct listentry *newEntry;
  struct listentry *entry = NULL;
  struct listentry *entry2 = NULL;

  // Creamos la estructura
  newEntry = malloc (sizeof (struct listentry));
  newEntry->wd.fd = fd;
  newEntry->wd.path = malloc (strlen (path) + 1);
  strcpy (newEntry->wd.path, path);

  // Ya tenemos la estructura, la tenemos que introducir en la lista ordenadamente
  // en principio, los fds vendran ordenados así que insertamos el valor mayor delante
  entry = LIST_FIRST (&head2);
  while (entry != NULL && entry->wd.fd > fd)
    {
      entry2 = entry;
      entry = LIST_NEXT (entry, entries2);
      if (entry == NULL)
	break;
    }
  if (entry != NULL)
    {
      // Hay que insertar antes de la entrada
      LIST_INSERT_BEFORE (entry, newEntry, entries2);
    }
  else
    {
      // Pueden ocurrir dos cosas, que este vacia la lista o que haya que insertarlo al final
      if (entry2 != NULL)
	{
	  // La lista no esta vacia, se inserta despues de entry2
	  LIST_INSERT_AFTER (entry2, newEntry, entries2);
	}
      else
	{
	  LIST_INSERT_HEAD (&head2, newEntry, entries2);
	}
    }

}

/* Funcion para el segundo thread que lee el queue */
void *
run2 (void *ptr)
{
  struct entry *event;
  while (isStop)
    {
      pthread_mutex_lock (&mutex);
      while (!(event = TAILQ_FIRST (&head)))
	{
	  pthread_cond_wait (&cond, &mutex);
	}
      while (event = TAILQ_FIRST (&head))
	{
	  TAILQ_REMOVE (&head, event, entries);
	  pthread_mutex_unlock (&mutex);
	  handle_event (event);
	  free (event);
	  pthread_mutex_lock (&mutex);
	}
      pthread_mutex_unlock (&mutex);
    }
  return NULL;
}

void
handle_event (struct entry *event)
{
  struct watch *w;
  int i = 0;
  const char *path;
  char pathcompleto[512];
  char *cur_event_filename = NULL;
  char *cur_event_file_or_dir = NULL;

  queue_operation operation;

  if (event->inot_ev.len)
    {
      cur_event_filename = event->inot_ev.name;
    }
  if (event->inot_ev.mask & IN_ISDIR)
    {
      cur_event_file_or_dir = "Dir";
    }
  else
    {
      cur_event_file_or_dir = "File";
    }

  // TODO Hacerlo mas eficiente
  path = busca_lista (event->inot_ev.wd);

  strcpy (pathcompleto, path);
  strcat (pathcompleto, "/");
  strcat (pathcompleto, cur_event_filename);

  switch (event->inot_ev.mask
	  & (IN_ALL_EVENTS | IN_UNMOUNT | IN_Q_OVERFLOW | IN_IGNORED))
    {
    case IN_MODIFY:
    case IN_ATTRIB:
    case IN_CLOSE_WRITE:
    case IN_MOVED_TO:
    case IN_CREATE:
      printf
	("monitor_inotify: handle_event: Fichero modificado o creado: %s/%s - evento: %i\n",
	 path, cur_event_filename,
	 (event->inot_ev.mask & (IN_ALL_EVENTS | IN_UNMOUNT | IN_Q_OVERFLOW |
				 IN_IGNORED)));
      operation.file = malloc (strlen (pathcompleto) + 1);
      strcpy (operation.file, pathcompleto);
      operation.operation = OPENDFS_MODIFY;
      queue_add_operation (&operation);
      free (operation.file);
      break;
    case IN_MOVED_FROM:
    case IN_DELETE:
      printf
	("monitor_inotify: handle_event: Fichero borrado o movido: %s/%s - evento: %i\n",
	 path, cur_event_filename,
	 (event->inot_ev.mask & (IN_ALL_EVENTS | IN_UNMOUNT | IN_Q_OVERFLOW |
				 IN_IGNORED)));
      operation.file = malloc (strlen (pathcompleto) + 1);
      strcpy (operation.file, pathcompleto);
      operation.operation = OPENDFS_DELETE;
      queue_add_operation (&operation);
      free (operation.file);
      break;
      // TODO Comprobar otros eventos
    }
  fflush (stdout);
}

void *
run (void *ptr)
{
  int cuantos;
  struct pollfd fd;
  sigset_t sigmask;
  struct timespec timeout;

  timeout.tv_sec = 1;
  timeout.tv_nsec = 0;

  fd.fd = inotify_fd;
  fd.events = POLLIN | POLLPRI | POLLHUP | POLLNVAL;

  sigemptyset (&sigmask);
  sigaddset (&sigmask, SIGINT);
  sigaddset (&sigmask, SIGTERM);

  while (isStop)
    {
      cuantos = ppoll (&fd, 1, &timeout, &sigmask);
      if (cuantos > 0)
	{
	  pthread_mutex_lock (&mutex);
	  cuantos = read_events ();
	  events = cuantos;
	  pthread_cond_signal (&cond);
	  pthread_mutex_unlock (&mutex);
	}
    }
  return NULL;
}

int
read_events ()
{
  char buffer[16384];
  size_t buffer_i;
  struct inotify_event *pevent;
  struct entry *event;
  ssize_t r;
  size_t event_size, q_event_size;
  int count = 0;

  r = read (inotify_fd, buffer, 16384);
  if (r <= 0)
    return r;
  buffer_i = 0;
  while (buffer_i < r)
    {
      /* Parse events and queue them. */
      pevent = (struct inotify_event *) &buffer[buffer_i];
      event_size = offsetof (struct inotify_event, name) + pevent->len;
      q_event_size = offsetof (struct entry, inot_ev.name) + pevent->len;
      event = (struct entry *) malloc (q_event_size);
      memmove (&(event->inot_ev), pevent, event_size);

      TAILQ_INSERT_TAIL (&head, event, entries);
      buffer_i += event_size;
      count++;
    }
  printf ("monitor_inotify: read_events: %d events queued\n", count);
  return count;
}

void
monitor_stop ()
{
  isStop = 0;
  pthread_join (thread_inotify, NULL);
  pthread_join (thread_queue, NULL);
}

#endif
