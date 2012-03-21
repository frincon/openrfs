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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <error.h>
#include <time.h>
#include <pthread.h>
#include "opendfs.h"
#include "utils.h"
#include "queue.h"

static const char time_format[] = "%Y.%m.%d.%H.%M.%s";

pthread_mutex_t _queue_mutex = PTHREAD_MUTEX_INITIALIZER;

const char create_queue[] = "CREATE TABLE IF NOT EXISTS QUEUE ("
  "FILE TEXT NOT NULL,"
  "TIMESTAMP TEXT NOT NULL,"
  "OPERATION INTEGER NOT NULL," "CONSTRAINT QUEUE_PK PRIMARY KEY (FILE))";

const char create_queue_index1[] =
  "CREATE INDEX IF NOT EXISTS QUEUE_IX1 ON QUEUE(TIMESTAMP ASC)";

const char insert_queue[] = "INSERT INTO QUEUE VALUES(?, ?, ?)";
const char update_queue[] =
  "UPDATE QUEUE SET TIMESTAMP=?, OPERATION=? WHERE FILE=?";

const char select_queue[] =
  "SELECT FILE, TIMESTAMP, OPERATION FROM QUEUE ORDER BY TIMESTAMP ASC LIMIT 1";

const char select_queue2[] =
  "SELECT FILE, TIMESTAMP, OPERATION FROM QUEUE WHERE FILE=?";

const char delete_queue[] = "DELETE FROM QUEUE WHERE FILE=?";

sqlite3_stmt *insert_stmt;
sqlite3_stmt *update_stmt;
sqlite3_stmt *delete_stmt;
sqlite3_stmt *select_stmt;
sqlite3_stmt *select_file_stmt;

/* la base de datos sqlite 3 */
sqlite3 *database;

int _queue_mutex_flag = 0;

void
queue_init ()
{
  int ret;
  const char *errmsg;
  char *error2;

  if (database != NULL)
    {
      error (0, 0, "Se ha llamado 2 veces a queue init()");
      return;
    }
  ret = sqlite3_open (config.database, &database);
  if (ret != SQLITE_OK)
    {
      errmsg = sqlite3_errmsg (database);
      error (EXIT_FAILURE, 0, "Error abriendo la base de datos: %s", errmsg);
    }

  /* Creamos la tabla para la cola */
  ret = sqlite3_exec (database, create_queue, NULL, NULL, &error2);
  if (ret != SQLITE_OK)
    error (EXIT_FAILURE, 0, "Error creando la tabla de cola: %s", error2);

  sqlite3_free (error2);
  ret = sqlite3_exec (database, create_queue_index1, NULL, NULL, &error2);
  if (ret != SQLITE_OK)
    error (EXIT_FAILURE, 0,
	   "Error creando el indice de la tabla de cola: %s", error2);

  sqlite3_free (error2);

  /* Creamos los prepared statement para usarlo luego */
  ret = sqlite3_prepare_v2 (database, insert_queue, -1, &insert_stmt, NULL);
  if (ret != SQLITE_OK)
    {
      errmsg = sqlite3_errmsg (database);
      error (EXIT_FAILURE, 0, "Error preparando las instrucciones: %s",
	     errmsg);
    }

  ret = sqlite3_prepare_v2 (database, update_queue, -1, &update_stmt, NULL);
  if (ret != SQLITE_OK)
    {
      errmsg = sqlite3_errmsg (database);
      error (EXIT_FAILURE, 0, "Error preparando las instrucciones: %s",
	     errmsg);
    }

  ret = sqlite3_prepare_v2 (database, select_queue, -1, &select_stmt, NULL);
  if (ret != SQLITE_OK)
    {
      errmsg = sqlite3_errmsg (database);
      error (EXIT_FAILURE, 0, "Error preparando las instrucciones: %s",
	     errmsg);
    }

  ret = sqlite3_prepare_v2 (database, delete_queue, -1, &delete_stmt, NULL);
  if (ret != SQLITE_OK)
    {
      errmsg = sqlite3_errmsg (database);
      error (EXIT_FAILURE, 0, "Error preparando las instrucciones: %s",
	     errmsg);
    }

  ret = sqlite3_prepare_v2 (database, select_queue2, -1, &select_file_stmt,
			    NULL);
  if (ret != SQLITE_OK)
    {
      errmsg = sqlite3_errmsg (database);
      error (EXIT_FAILURE, 0, "Error preparando las instrucciones: %s",
	     errmsg);
    }

}

void
queue_add_operation (queue_operation * op)
{
  int ret;
  time_t date;
  struct tm *date_tm;

  char date_formatted[50];

  pthread_mutex_lock (&_queue_mutex);

  date = time (NULL);
  date_tm = gmtime (&date);
  if (strftime (date_formatted, 50, time_format, date_tm) == 0)
    {
      error (EXIT_FAILURE, 0, "Error creando la fecha");
    }

  if (database == NULL || insert_stmt == NULL)
    error (EXIT_FAILURE, 0, "No se ha iniciado la cola");

  ret = sqlite3_bind_text (insert_stmt, 1, op->file, -1, SQLITE_STATIC);
  if (ret != SQLITE_OK)
    error (EXIT_FAILURE, 0, "No se ha podido añadir un parámetro");

  ret = sqlite3_bind_text (insert_stmt, 2, date_formatted, -1,
			   SQLITE_TRANSIENT);
  if (ret != SQLITE_OK)
    error (EXIT_FAILURE, 0, "No se ha podido añadir un parámetro");

  ret = sqlite3_bind_int (insert_stmt, 3, op->operation);
  if (ret != SQLITE_OK)
    error (EXIT_FAILURE, 0, "No se ha podido añadir un parámetro");

  ret = sqlite3_step (insert_stmt);
  if (ret == SQLITE_CONSTRAINT)
    {
      // El fichero está en la base de datos

      ret = sqlite3_bind_text (update_stmt, 1, date_formatted, -1,
			       SQLITE_TRANSIENT);
      if (ret != SQLITE_OK)
	error (EXIT_FAILURE, 0, "No se ha podido añadir un parámetro");

      ret = sqlite3_bind_int (update_stmt, 2, op->operation);
      if (ret != SQLITE_OK)
	error (EXIT_FAILURE, 0, "No se ha podido añadir un parámetro");

      ret = sqlite3_bind_text (update_stmt, 3, op->file, -1, SQLITE_STATIC);
      if (ret != SQLITE_OK)
	error (EXIT_FAILURE, 0, "No se ha podido añadir un parámetro");

      ret = sqlite3_step (update_stmt);
      if (ret != SQLITE_DONE)
	error (EXIT_FAILURE, 0, "Error al ejecutar el update");
      sqlite3_reset (update_stmt);
    }
  else if (ret != SQLITE_DONE)
    {
      error (EXIT_FAILURE, 0, "Error al ejecutar el insert");
    }
  sqlite3_reset (insert_stmt);

  pthread_mutex_unlock (&_queue_mutex);

}

void
_delete_file (const char *file)
{
  int ret;

  ret = sqlite3_bind_text (delete_stmt, 1, file, -1, SQLITE_TRANSIENT);
  if (ret != SQLITE_OK)
    error (EXIT_FAILURE, 0, "No se ha podido añadir un parámetro");
  ret = sqlite3_step (delete_stmt);
  if (ret != SQLITE_DONE)
    error (EXIT_FAILURE, 0, "Error al ejecutar el update");
  sqlite3_reset (delete_stmt);

}

int
queue_delete_operation (queue_operation * operation)
{
  queue_operation op;
  _queue_mutex_flag = 1;
  pthread_mutex_lock (&_queue_mutex);
  queue_get_operation (&op);
  if (memcmp (&(operation->time), &(op.time), sizeof (op.time)) == 0)
    {
      _delete_file (op.file);
    }
  else
    {
      utils_debug ("The operation was changed. Don't delete it");
    }
  pthread_mutex_unlock (&_queue_mutex);
  _queue_mutex_flag = 0;
  return EXIT_SUCCESS;
}


int
queue_get_operation (queue_operation * operation)
{
  int ret;
  const unsigned char *file;
  const unsigned char *timestamp;
  int op;
  int result;

  if (_queue_mutex_flag == 0)
    pthread_mutex_lock (&_queue_mutex);

  if (database == NULL || insert_stmt == NULL)
    error (EXIT_FAILURE, 0, "No se ha iniciado la cola");

  ret = sqlite3_step (select_stmt);
  switch (ret)
    {
    case SQLITE_ROW:
      // Hay fila, se procesa
      file = sqlite3_column_text (select_stmt, 0);
      timestamp = sqlite3_column_text (select_stmt, 1);
      op = sqlite3_column_int (select_stmt, 2);
      operation->file = malloc (strlen (file) + 1);
      strcpy (operation->file, file);
      operation->operation = op;

      memset (&operation->time, '\0', sizeof (operation->time));
      strptime (timestamp, time_format, &operation->time);

      //Además realizamos el delete
      //_delete_file (file);
      result = 1;
      break;
    case SQLITE_DONE:
      //No hay filas, nada que hacer
      result = 0;
      break;
    default:
      error (EXIT_FAILURE, 0, "Error recuperando fila");
      break;
    }
  sqlite3_reset (select_stmt);
  if (_queue_mutex_flag == 0)
    pthread_mutex_unlock (&_queue_mutex);
  return result;
}

int
queue_get_operation_for_file (const char *file2, queue_operation * operation)
{
  int ret;
  const unsigned char *file;
  const unsigned char *timestamp;
  int op;
  int result;

  pthread_mutex_lock (&_queue_mutex);

  if (database == NULL || select_file_stmt == NULL)
    error (EXIT_FAILURE, 0, "No se ha iniciado la cola");

  ret = sqlite3_bind_text (select_file_stmt, 1, file2, -1, SQLITE_TRANSIENT);
  if (ret != SQLITE_OK)
    error (EXIT_FAILURE, 0, "No se ha podido añadir un parámetro");

  ret = sqlite3_step (select_file_stmt);
  switch (ret)
    {
    case SQLITE_ROW:
      // Hay fila, se procesa
      file = sqlite3_column_text (select_file_stmt, 0);
      timestamp = sqlite3_column_text (select_file_stmt, 1);
      op = sqlite3_column_int (select_file_stmt, 2);
      operation->file = malloc (strlen (file) + 1);
      strcpy (operation->file, file);
      operation->operation = op;

      memset (&operation->time, '\0', sizeof (operation->time));
      strptime (timestamp, time_format, &operation->time);

      //Además realizamos el delete
      _delete_file (file);
      result = 1;
      break;
    case SQLITE_DONE:
      //No hay filas, nada que hacer
      result = 0;
      break;
    default:
      error (EXIT_FAILURE, 0, "Error recuperando fila");
      break;
    }
  sqlite3_reset (select_file_stmt);
  pthread_mutex_unlock (&_queue_mutex);
  return result;

}

void
queue_close ()
{
  int ret;
  sqlite3_finalize (insert_stmt);
  sqlite3_finalize (update_stmt);
  sqlite3_finalize (select_stmt);
  sqlite3_finalize (delete_stmt);
  sqlite3_finalize (select_file_stmt);
  ret = sqlite3_close (database);
  if (ret != SQLITE_OK)
    {
      error (0, 0, "Error cerrando la base de datos!!!");
    }
}
