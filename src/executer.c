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
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <librsync.h>

#include <sys/types.h>
#include <fcntl.h>
#include <libgen.h>

#include "opendfs.h"
#include "queue.h"
#include "utils.h"
#include "executer.h"

enum
{
  _EXECUTER_OK_EXISTS = 0, _EXECUTER_OK_NOT_EXISTS = 1, _EXECUTER_REVERT = 2
};

enum
{
  _EXECUTER_PART = 0, _EXECUTER_FINISH = 1
};

enum executer_rs_type
{
  _EXECUTER_SOCKET = 0, _EXECUTER_FILE = 1
};

#define _EXECUTER_DEFAULT_OUT 1024

typedef struct _executer_rs
{
  int sock;
  int type;
  char *buf;
  size_t size;
} executer_rs;

int
_executer_read_buffer (int sock, void *buf, size_t size)
{
  size_t leido;

  leido = 0;
  fprintf (stderr, "_executer_read_buffer: leyendo del buffer %i bytes...\n",
	   size);
  do
    {
      size_t leido2;
      leido2 = read (sock, buf, size);
      fprintf (stderr, "_executer_read_buffer: leido %i bytes...\n", leido2);
      if (leido2 == 0)
	{
	  return leido;
	}
      else if (leido2 < 0)
	{
	  utils_error ("Error reading from socket: %s", strerror (errno));
	  return leido2;
	}
      leido += leido2;
    }
  while (leido < size);
  return leido;
}

bool
_executer_send_buffer (int sock, void *buf, size_t size)
{
  int ret = 0;
  size_t writed = 0;

  while (writed < size)
    {
      ret = write (sock, buf, size - writed);
      if (ret < 0)
	return -1;
      writed += ret;
      buf = buf + ret;
    }
  return writed == size;

}

bool
_executer_send_part (int sock, char *buf, size_t size)
{
  char *buf2;
  int operacion;
  operacion = _EXECUTER_PART;
  size_t total = size + sizeof (size_t) + sizeof (operacion);

  buf2 = malloc (total);
  memset (buf2, 0x0, total);
  memcpy (buf2, &operacion, sizeof (operacion));
  // *buf2 = _EXECUTER_PART;
  buf2 += sizeof (_EXECUTER_PART);
  memcpy (buf2, &size, sizeof (size));
  //*buf2 = size;
  buf2 += sizeof (size_t);
  memcpy (buf2, buf, size);
  buf2 -= sizeof (size_t) + sizeof (_EXECUTER_PART);

  bool resultado = _executer_send_buffer (sock, buf2, total);
  free (buf2);
  return resultado;
}

bool
_executer_send_finish (int sock)
{
  int buf;
  buf = _EXECUTER_FINISH;
  return (_executer_send_buffer (sock, &buf, sizeof (int)));
}

void
_executer_real_path (const char *file, char **path)
{
  char *path2;
  path2 = malloc (strlen (config.path) + strlen (file) + 1);
  strcpy (path2, config.path);
  strcat (path2, file);

  utils_trace ("file: '%s' real path: '%s'", file, path2);

  *path = path2;
}

void
_executer_conflict_path (const char *file, char **path)
{
  char date_formatted[50];
  const char time_format[] = "%Y_%m_%d_%H_%M_%s";
  char *path2;
  char *base;
  time_t date;
  struct tm *date_tm;

  base = basename (file);

  date = time (NULL);
  date_tm = gmtime (&date);
  strftime (date_formatted, 50, time_format, date_tm);

  path2 =
    malloc (strlen (config.conflicts) + strlen (base) +
	    strlen (date_formatted) + 3);

  strcpy (path2, config.conflicts);
  strcat (path2, "/");
  strcat (path2, base);
  strcat (path2, ".");
  strcat (path2, date_formatted);

  utils_trace ("file: '%s' conflict path: '%s'");

  *path = path2;
}

executer_result
_executer_delete (executer_job_t * job)
{
  int ret;
  struct stat buf;
  char *path;
  char *newpath;

  int return_value = EXECUTER_END;

  _executer_real_path (job->file, &path);

  utils_debug ("Deleting file: '%s' whith real path '%s'", job->file, path);

//Miramos si existe el fichero:
  ret = lstat (path, &buf);
  if (ret != 0)
    {
      utils_warn ("Error making lstat in file '%s': %s", path,
		  strerror (errno));
      return_value = EXECUTER_ERROR;
    }

//Existe, lo movemos, cambiandolo de nombre

  _executer_conflict_path (job->file, &newpath);

  if (rename (path, newpath) != 0)
    {
      utils_error
	("Error deleting file '%s' (moving to conflicts directory): %s", path,
	 strerror (errno));
      return_value = EXECUTER_ERROR;
    }
  free (newpath);
  free (path);

  return return_value;
}

rs_result
_executer_in_callback (rs_job_t * job, rs_buffers_t * buf, void *opaque)
{
  size_t leido;
  int haymas;
  executer_rs *executer = (executer_rs *) opaque;

  char *buf2;

  buf2 = 0x0;

  fprintf (stderr, "_executer_in_callback: leyendo del buffer...\n");
  /* If not process anything, do nothing */
  if (buf->avail_in == executer->size)
    return RS_DONE;

  //Si se ha alcanzado al final, se devuelve done
  if (buf->eof_in)
    return RS_DONE;

  fprintf (stderr, "_executer_in_callback: Tenemos que leer...\n");
  //Primero copiamos si queda algo
  if (buf->avail_in > 0)
    {
      buf2 = malloc (buf->avail_in);
      memcpy (buf2, buf->next_in, buf->avail_in);
      fprintf (stderr, "_executer_in_callback: Habia %i bytes...\n",
	       buf->avail_in);
    }

  memset (executer->buf, 0x0, executer->size);

  if (executer->type == _EXECUTER_SOCKET)
    {
      fprintf (stderr, "_executer_in_callback: Es un socket...\n");
      //Comprobamos que hay mas datos y los leemos
      leido =
	_executer_read_buffer (executer->sock, &haymas, sizeof (haymas));
      if (leido != sizeof (haymas))
	{
	  utils_error ("Error reading from buffer");
	  return RS_IO_ERROR;
	}

      if (haymas == _EXECUTER_PART)
	{
	  utils_trace ("There are more");
	  //Hay mas se lee
	  size_t size;

	  //Se lee el tamaño
	  leido =
	    _executer_read_buffer (executer->sock, &size, sizeof (size));
	  if (leido != sizeof (size))
	    {
	      utils_error ("Error reading from buffer");
	      return RS_IO_ERROR;
	    }

	  //Se lee el contenido
	  char *buf3;
	  buf3 = malloc (size);
	  leido = _executer_read_buffer (executer->sock, buf3, size);
	  if (leido != size)
	    {
	      utils_error ("Error reading from buffer");
	      return RS_IO_ERROR;
	    }

	  utils_debug ("Readed %i bytes", leido);

	  while (size + buf->avail_in > executer->size)
	    {
	      // ampliamos el tamaño del executer
	      executer->size *= 2;
	      free (executer->buf);
	      executer->buf = malloc (executer->size);
	      fprintf (stderr,
		       "_executer_in_callback: Ampliando el buffer en total a %i bytes\n",
		       executer->size);
	    }

	  if (buf->avail_in > 0)
	    {
	      //Habia, copiamos
	      memcpy (executer->buf, buf2, buf->avail_in);
	    }
	  memcpy ((executer->buf) + buf->avail_in, buf3, size);
	  buf->avail_in = size + buf->avail_in;
	  buf->next_in = executer->buf;
	  free (buf3);
	  if (buf2 != 0)
	    free (buf2);
	  return RS_DONE;
	}
      else
	{
	  fprintf (stderr, "_executer_in_callback: NO HAY MAS...\n");
	  // No hay mas, se envia el final
	  buf->eof_in = true;
	  return RS_DONE;
	}
    }
  else
    {
      char *buf3;
      buf3 = malloc (executer->size - buf->avail_in);
      leido =
	_executer_read_buffer (executer->sock, buf3,
			       executer->size - buf->avail_in);
      if (leido == 0)
	{
	  fprintf (stderr,
		   "_executer_in_callback: Se ha alcanzado el final del fichero\n");
	  //No hay mas, se envia el final
	  buf->eof_in = true;
	  return RS_DONE;
	}
      else if (leido > 0)
	{
	  fprintf (stderr,
		   "_executer_in_callback: Se han leido %i bytes del fichero\n",
		   leido);
	  //Hay mas se envia
	  if (buf->avail_in > 0)
	    {
	      //Habia, copiamos
	      memcpy (executer->buf, buf2, buf->avail_in);
	    }
	  memcpy ((executer->buf) + buf->avail_in, buf3, leido);
	  buf->avail_in = leido + buf->avail_in;
	  buf->next_in = executer->buf;
	  free (buf3);
	  if (buf2 != 0)
	    free (buf2);
	  return RS_DONE;
	}
      else
	{
	  utils_error ("Error reading from file, cancelling job");
	  return RS_IO_ERROR;
	}
    }
}

rs_result
_executer_out_callback (rs_job_t * job, rs_buffers_t * buf, void *opaque)
{
  executer_rs *executer = (executer_rs *) opaque;

  if (buf->next_out == 0)
    {
      buf->next_out = executer->buf;
      buf->avail_out = executer->size;
    }

  if (buf->avail_out < 20 || (buf->avail_in == 0 && buf->eof_in))
    {
      fprintf (stderr,
	       "_executer_out_callback: Enviando %i butes del buffer...\n",
	       executer->size - buf->avail_out);
      //Enviamos ya lo que hay
      if (executer->type == _EXECUTER_FILE)
	{
	  if (!_executer_send_buffer
	      (executer->sock, executer->buf,
	       executer->size - buf->avail_out))
	    {
	      utils_error ("Error writing %i bytes, cancelling job",
			   executer->size - buf->avail_out);
	      return RS_IO_ERROR;
	    }
	}
      else
	{
	  if (!_executer_send_part
	      (executer->sock, executer->buf,
	       executer->size - buf->avail_out))
	    {
	      utils_error ("Error writing %i bytes, cancelling job",
			   executer->size - buf->avail_out);
	      return RS_IO_ERROR;
	    }
	}
      buf->avail_out = executer->size;
      buf->next_out = executer->buf;
      return RS_DONE;
    }
  else
    {
      //Todavia no enviamos
      return RS_DONE;
    }

}

int
_executer_create_temp (const char *realpath, char **temppath)
{
  char *base;
  char *dir;
  char *stemp;

  stemp = malloc (strlen (realpath) + 1);
  strcpy (stemp, realpath);

  //wait for delta and patch in same time
  base = basename (stemp);
  dir = dirname (stemp);
  *temppath = malloc (strlen (dir) + 2 + strlen (base) + 1 + 6 + 1);
  strcpy (*temppath, dir);
  strcat (*temppath, "/.");
  strcat (*temppath, base);
  strcat (*temppath, "-");
  strcat (*temppath, "XXXXXX");

  int fd = mkstemp (*temppath);
  free (stemp);
  return fd;
}

executer_result
_executer_change_permission (executer_job_t * job)
{
  struct stat *stat2;

  stat2 = (struct stat *) job->opaque;

  //Change mode
  chmod (job->realpath, stat2->st_mode);
  chown (job->realpath, stat2->st_uid, stat2->st_gid);
  //TODO check for errors
  return EXECUTER_END;
}

executer_result
_executer_receive_stat (executer_job_t * job)
{
  int leido;
  struct stat stat2;

  leido = _executer_read_buffer (job->peer_sock, &stat2, sizeof (stat2));
  if (leido != sizeof (stat2))
    {
      utils_error ("Error reading stat data from peer: %s", strerror (errno));
      return EXECUTER_ERROR;
    }

  job->opaque = &stat2;
  job->next_operation = _executer_change_permission;

  return EXECUTER_RUNNING;

}

executer_result
_executer_move_temp (executer_job_t * job)
{
  char *tempfile = (char *) job->opaque;
  if (job->exists_real)
    {
      int ret = unlink (job->realpath);
      if (ret != 0)
	{
	  utils_error ("Error unlinking file '%s': %s", job->realpath,
		       strerror (errno));
	  return EXECUTER_ERROR;
	}
    }
  int ret = rename (tempfile, job->realpath);
  if (ret != 0)
    {
      utils_error ("Error renaming temporal file '%s' to '%s': %s", tempfile,
		   job->realpath, strerror (errno));
      return EXECUTER_ERROR;
    }

  //Change permis and owner
  job->next_operation = _executer_receive_stat;
  return EXECUTER_RUNNING;
}

executer_result
_executer_recive_delta (executer_job_t * job)
{
  char *temppath;
  rs_buffers_t rsbuf;
  executer_rs inrs;
  executer_rs outrs;
  int result;

  int tempfile = _executer_create_temp (job->realpath, &temppath);
  if (tempfile == 0)
    {
      utils_error ("Error creating temp file for file '%s': %s",
		   job->realpath, strerror (errno));
      return EXECUTER_ERROR;
    }

  FILE *origfile = fopen (job->realpath, "rb");
  if (origfile == NULL)
    {
      utils_error ("Error opening file '%s': %s", job->realpath,
		   strerror (errno));
      return EXECUTER_ERROR;
    }

  rs_job_t *patchjob = rs_patch_begin (rs_file_copy_cb, origfile);

  inrs.sock = job->peer_sock;
  inrs.type = _EXECUTER_SOCKET;
  inrs.size = _EXECUTER_DEFAULT_OUT;
  inrs.buf = malloc (inrs.size);

  outrs.sock = tempfile;
  outrs.type = _EXECUTER_FILE;
  outrs.size = RS_DEFAULT_BLOCK_LEN;
  outrs.buf = malloc (outrs.size);

  rsbuf.avail_in = 0;
  rsbuf.avail_out = 0;
  rsbuf.next_in = 0;
  rsbuf.next_out = 0;
  rsbuf.eof_in = false;

  result =
    rs_job_drive (patchjob, &rsbuf, _executer_in_callback, &inrs,
		  _executer_out_callback, &outrs);
  if (result != RS_DONE)
    {
      utils_error ("Error patching file");
      return EXECUTER_ERROR;
    }

  //Check if there are any data
  if (rsbuf.avail_out > 0)
    {
      _executer_in_callback (patchjob, &rsbuf, &inrs);
      _executer_out_callback (patchjob, &rsbuf, &outrs);
    }

  rs_job_free (patchjob);

  close (tempfile);
  fclose (origfile);
  free (outrs.buf);
  free (inrs.buf);

  job->next_operation = _executer_move_temp;
  job->opaque = temppath;

  return EXECUTER_RUNNING;
}

executer_result
_executer_send_signature (executer_job_t * job)
{

  rs_job_t *sigjob;
  executer_rs inrs;
  executer_rs outrs;
  rs_buffers_t rsbuf;

  int openfile = open (job->realpath, O_RDONLY);

  inrs.sock = openfile;
  inrs.type = _EXECUTER_FILE;
  inrs.size = RS_DEFAULT_BLOCK_LEN;
  inrs.buf = malloc (inrs.size);

  outrs.sock = job->peer_sock;
  outrs.type = _EXECUTER_SOCKET;
  outrs.size = _EXECUTER_DEFAULT_OUT;
  outrs.buf = malloc (outrs.size);

  sigjob = rs_sig_begin (RS_DEFAULT_BLOCK_LEN, RS_DEFAULT_STRONG_LEN);
  rs_result result =
    rs_job_drive (sigjob, &rsbuf, _executer_in_callback, &inrs,
		  _executer_out_callback, &outrs);

  // Free the memory
  rs_job_free (sigjob);
  free (inrs.buf);
  free (outrs.buf);
  close (openfile);

  if (result != RS_DONE)
    {
      utils_error ("Error sending signature");
      return EXECUTER_ERROR;
    }

  _executer_send_finish (job->peer_sock);

  job->next_operation = _executer_recive_delta;
  return EXECUTER_RUNNING;

}

executer_result
_executer_copy_conflict (executer_job_t * job)
{
  char *source;
  char *target;
  char *command;
  int ret;

  // TODO optimizar el copiar, porque se puede realizar un enlace duro y luego al parchear deslinkar y mover

  char copy_command[] = "cp -a %s %s";

// Cojemos los path
  _executer_real_path (job->file, &source);
  _executer_conflict_path (job->file, &target);

  command =
    malloc (strlen (copy_command) + strlen (source) + strlen (target) - 4 +
	    1);
  sprintf (command, copy_command, source, target);

  ret = system (command);

  free (source);
  free (target);

  if (ret != EXIT_SUCCESS)
    {
      utils_error ("Error copying file to conflicts");
      return EXECUTER_ERROR;
    }

  // The next operation is send signature
  job->next_operation = _executer_send_signature;
  return EXECUTER_RUNNING;

}

executer_result
_executer_receive_file (executer_job_t * job)
{
  FILE *file;
  int operation;
  bool protocol_error = false;

  // Receive a whole file
  file = fopen (job->realpath, "w");
  if (file == 0)
    {
      utils_error ("Error creating file '%s': %s", job->realpath,
		   strerror (errno));
      return EXECUTER_ERROR;
    }
  do
    {
      //Read the operation

      size_t leido;
      size_t size;
      char *buf;


      leido =
	_executer_read_buffer (job->peer_sock, &operation,
			       sizeof (operation));
      if (leido != sizeof (operation))
	{
	  protocol_error = true;
	  break;
	}

      if (operation == _EXECUTER_PART)
	{
	  int ret;
	  leido =
	    _executer_read_buffer (job->peer_sock, &size, sizeof (size));

	  if (leido != sizeof (size))
	    {
	      protocol_error = true;
	      break;
	    }

	  buf = malloc (size);
	  leido = _executer_read_buffer (job->peer_sock, buf, size);
	  if (leido != size)
	    {
	      protocol_error = true;
	      break;
	    }

	  ret = fwrite (buf, 1, size, file);

	  if (ret != size)
	    {
	      utils_error ("Error writing file: %s", strerror (errno));
	      protocol_error = true;
	      break;
	    }

	}

    }
  while (operation == _EXECUTER_PART);

  fclose (file);

  if (protocol_error)
    {
      utils_error ("Error receiving whole file, canceling job");
      return EXECUTER_ERROR;
    }

  job->next_operation = _executer_receive_stat;
  return EXECUTER_RUNNING;

}

executer_result
_executer_check_exists (executer_job_t * job)
{
  bool exists = false;
  bool *conflict;
  struct stat bufstat;

  conflict = (bool *) job->opaque;

  exists = !lstat (job->realpath, &bufstat);
  if (exists)
    {

      job->exists_real = true;
      // Send ok, the next steps are copy confict, send signature
      int operacion = _EXECUTER_OK_EXISTS;
      if (!_executer_send_buffer (job->peer_sock, &operacion, sizeof (int)))
	{
	  utils_error ("Error sending data to peer.");
	  return EXECUTER_ERROR;
	}

      if (*conflict)
	{
	  job->next_operation = _executer_copy_conflict;
	}
      else
	{
	  job->next_operation = _executer_send_signature;
	}
      return EXECUTER_RUNNING;
    }
  else
    {
      job->exists_real = false;
      //Not exists, need all file
      int operacion = _EXECUTER_OK_NOT_EXISTS;
      if (!_executer_send_buffer (job->peer_sock, &operacion, sizeof (int)))
	{
	  utils_error ("Error sending data to peer.");
	  return EXECUTER_ERROR;
	}

      job->next_operation = _executer_receive_file;
      return EXECUTER_RUNNING;
    }

}

int
_executer_send_revert (int sock, mensaje * mens, const char *file)
{
  utils_debug ("The file %s has operations after, send revert operation.");
  return EXIT_SUCCESS;
}

executer_result
_executer_send_stat (executer_job_t * job)
{
  struct stat stat2;
  int ret;

  ret = lstat (job->realpath, &stat2);
  if (ret != 0)
    {
      utils_error ("Error stat file '%s': %s", job->realpath,
		   strerror (errno));
      return EXECUTER_ERROR;
    }
  if (!_executer_send_buffer (job->peer_sock, &stat2, sizeof (stat2)))
    {
      utils_error ("Error sending stat data to peer: %s", strerror (errno));
      return EXECUTER_ERROR;
    }

  return EXECUTER_END;

}

executer_result
_executer_send_delta (executer_job_t * job)
{
  // The opaque pointer is to signature
  rs_signature_t *signature = (rs_signature_t *) job->opaque;
  rs_job_t *deltajob;
  char *realpath;
  executer_rs inrs;
  executer_rs outrs;
  rs_result result;
  rs_buffers_t rsbuf;

  deltajob = rs_delta_begin (signature);

  _executer_real_path (job->file, &realpath);
  int file2 = open (realpath, O_RDONLY);

  inrs.type = _EXECUTER_FILE;
  inrs.sock = file2;
  inrs.size = RS_DEFAULT_BLOCK_LEN;
  inrs.buf = malloc (inrs.size);
  outrs.type = _EXECUTER_SOCKET;
  outrs.sock = job->peer_sock;
  outrs.size = _EXECUTER_DEFAULT_OUT;
  outrs.buf = malloc (outrs.size);
  if (rs_build_hash_table (signature) != RS_DONE)
    {
      utils_error ("Error building hash table");
      return EXECUTER_ERROR;
    }
  result =
    rs_job_drive (deltajob, &rsbuf, _executer_in_callback, &inrs,
		  _executer_out_callback, &outrs);

  // Free the memory
  free (realpath);
  rs_job_free (deltajob);
  free (inrs.buf);
  free (outrs.buf);

  // Send finish to the socket
  if (!_executer_send_finish (job->peer_sock))
    {
      utils_error ("Error sending finish message: %s", strerror (errno));
      return EXECUTER_ERROR;
    }
  if (result != RS_DONE)
    {
      utils_error ("Error calculating delta");
      return EXECUTER_ERROR;
    }
  else
    {
      // Ok, send the stat
      job->next_operation = _executer_send_stat;
      // Ok, the delta is send, no need more process
      return EXECUTER_RUNNING;
    }

}

executer_result
_executer_recive_signature (executer_job_t * job)
{
  rs_signature_t *signature;
  rs_job_t *signature_job;
  executer_rs inrs;
  rs_result result;
  rs_buffers_t rsbuf;

  //Load the signature
  signature_job = rs_loadsig_begin (&signature);
  inrs.type = _EXECUTER_SOCKET;
  inrs.sock = job->peer_sock;
  inrs.size = RS_DEFAULT_BLOCK_LEN;
  inrs.buf = malloc (inrs.size);

  // Run librsync job
  result =
    rs_job_drive (signature_job, &rsbuf, _executer_in_callback, &inrs, NULL,
		  NULL);

  // Free the memory
  rs_job_free (signature_job);
  free (inrs.buf);

  if (result != RS_DONE)
    {
      utils_error ("Error carculating singnature");
      return EXECUTER_ERROR;
    }
  else
    {
      //After receive signature, we need send delta
      job->next_operation = _executer_send_delta;
      job->opaque = signature;
      return EXECUTER_RUNNING;
    }

}

executer_result
_executer_send_file (executer_job_t * job)
{
  size_t leido;
  FILE *file;
  char buf[_EXECUTER_DEFAULT_OUT];

  file = fopen (job->realpath, "r");
  if (file == 0)
    {
      utils_error ("Error opening file '%s' for read: %s", job->realpath,
		   strerror (errno));
      return EXECUTER_ERROR;
    }

  do
    {
      leido = fread (&buf, 1, _EXECUTER_DEFAULT_OUT, file);
      if (leido == 0)
	{
	  if (ferror (file))
	    {
	      utils_error ("Error reading file '%s': %s", job->realpath,
			   strerror (errno));
	      fclose (file);
	      return EXECUTER_ERROR;
	    }
	}
      if (!_executer_send_part (job->peer_sock, &buf, leido))
	{
	  utils_error ("Error sending file data to peer: %s",
		       strerror (errno));
	}
    }
  while (!feof (file));

  fclose (file);

  _executer_send_finish (job->peer_sock);

  job->next_operation = _executer_send_stat;
  return EXECUTER_RUNNING;

}

executer_result
_executer_mkdir (executer_job_t * job)
{

  if (mkdir (job->realpath, job->mode) != 0)
    {
      utils_error ("Error creating directory '%s': %s", job->realpath,
		   strerror (errno));
      return EXECUTER_ERROR;
    }
  return EXECUTER_END;
}

/*
 * Read permissions bits,
 * opaque contains the next job
 */
executer_result
_executer_read_permission (executer_job_t * job)
{
  mode_t mode;

  if (_executer_read_buffer (job->peer_sock, &mode, sizeof (mode)) !=
      sizeof (mode))
    {
      utils_error ("Error reading from peer");
      return EXECUTER_ERROR;
    }

  job->mode = mode;

  if (job->operation == EXECUTER_CREATE_DIR)
    job->next_operation = _executer_mkdir;

  return EXECUTER_RUNNING;
}

executer_result
_executer_send_permission (executer_job_t * job)
{
  mode_t mode;
  struct stat stat2;
  if (lstat (job->realpath, &stat2) != 0)
    {
      utils_error ("Error reading stat from file '%s': %s", job->realpath,
		   strerror (errno));
      return EXECUTER_ERROR;
    }

  if (!_executer_send_buffer
      (job->peer_sock, &(stat2.st_mode), sizeof (stat2.st_mode)))
    {
      utils_error ("Error sendig stat information, cancelling job");
      return EXECUTER_ERROR;
    }

  return EXECUTER_END;
}

executer_result
_executer_wait_modify (executer_job_t * job)
{
  //Wait for read operation from the peer
  int operation;
  size_t leido =
    _executer_read_buffer (job->peer_sock, &operation, sizeof (operation));
  if (leido != sizeof (operation))
    {
      utils_error ("Error reading operation, cancelling job");
      return EXECUTER_ERROR;
    }
  switch (operation)
    {
    case _EXECUTER_OK_EXISTS:
      //In the other peer exists, send with librsync, next_operation is receive signature
      job->next_operation = _executer_recive_signature;
      return EXECUTER_RUNNING;
      break;
    case _EXECUTER_OK_NOT_EXISTS:
      job->next_operation = _executer_send_file;
      return EXECUTER_RUNNING;
      break;
    }
  utils_error ("Operation not permited: %i", operation);
  return EXECUTER_ERROR;
}

int
_executer_run_job (executer_job_t * job)
{
  int result;
  do
    {
      result = (job->next_operation) (job);
      if (result == EXECUTER_ERROR)
	{
	  // TODO Improve error handling
	  utils_error ("Error running job, job cancelled");
	  return EXIT_FAILURE;
	}
    }
  while (result == EXECUTER_RUNNING);
  return EXIT_SUCCESS;
}

executer_result
executer_send (int sock, mensaje * mens, const char *file)
{
  int ret;
  executer_job_t job;

  job.file = malloc (mens->file_size);
  strcpy (job.file, file);
  job.peer_sock = sock;
  job.time = mens->time;
  _executer_real_path (file, &(job.realpath));

  switch (mens->operacion)
    {
    case OPENDFS_DELETE:
      // TODO Esperar respuesta OK
      break;
    case OPENDFS_MODIFY:
      // Ok, the next step is wait for modification in the other peer
      job.operation = EXECUTER_MODIFY;
      job.next_operation = _executer_wait_modify;
      ret = _executer_run_job (&job);
      break;
    case OPENDFS_CREATE_DIR:
      job.operation = EXECUTER_CREATE_DIR;
      job.next_operation = _executer_send_permission;
      ret = _executer_run_job (&job);
    }

  free (job.file);
  free (job.realpath);
  return ret;
}

executer_result
executer_receive (int sock, mensaje * mens, const char *file)
{

  int ret;
  executer_job_t job;
  bool conflict;

  job.file = malloc (mens->file_size);
  strcpy (job.file, file);
  job.peer_sock = sock;
  job.time = mens->time;
  _executer_real_path (file, &(job.realpath));

// Bien, me han enviado un fichero, debo comprobar en que estado está aquí.

  /* Puede haber varias posibilidades:
   *
   *    Borrado -> No tocado (Copiar y Borrar) *
   *    Borrado -> Borrado (Nada que hacer) *
   *    Borrado -> Modificado(Antes) (Copiar y Borrar) *
   *    Borrado -> Modificado(Despues) (Enviar al otro) *
   *    Modificado -> No tocado (Modificar) *
   *    Modificado -> Borrado(Antes) (Modificar)
   *    Modificado -> Borrado(Despues) (Nada que hacer, cuando toque se borrará)
   *    Modificado -> Modificado(Antes) (Copiar y Modificar)
   *    Modificado -> Modificado(Despues) (Enviar al otro (copiar y modificar)
   */

  queue_operation operation;

  ret = queue_get_operation_for_file (file, &operation);
  if (ret != 0)
    {
      time_t time1 = mktime (&(mens->time));
      time_t time2 = mktime (&(operation.time));

      switch (mens->operacion)
	{
	case OPENDFS_DELETE:
	  switch (operation.operation)
	    {
	    case OPENDFS_DELETE:
	      //Borrado -> Borrado (Nada que hacer)
	      return EXIT_SUCCESS;
	      break;
	    case OPENDFS_MODIFY:
	      //Puede que sea antes o despues
	      if (time2 > time1)
		{
		  //Borrado -> Modificado(Despues) (Enviar al otro)
		  job.next_operation = _executer_send_revert;
		  ret = _executer_run_job (&job);
		}
	      else
		{
		  //Borrado -> Modificado(Antes) (Copiar y Borrar)
		  job.next_operation = _executer_delete;
		  ret = _executer_run_job (&job);
		}
	      break;
	    }
	  break;
	case OPENDFS_MODIFY:
	  switch (operation.operation)
	    {
	    case OPENDFS_DELETE:
	      if (time2 > time1)
		{
		  // Modificado -> Borrado(Despues)
		  job.next_operation = _executer_send_revert;
		  ret = _executer_run_job (&job);
		}
	      else
		{
		  //Modificado -> Borrado(Antes) (Modificar)
		  job.next_operation = _executer_check_exists;
		  conflict = true;
		  job.opaque = &conflict;
		  ret = _executer_run_job (&job);
		}
	      break;
	    case OPENDFS_MODIFY:
	      if (time2 > time1)
		{
		  //Modificado -> Modificado(Despues)
		  job.next_operation = _executer_send_revert;
		  ret = _executer_run_job (&job);
		}
	      else
		{
		  //Modificado -> Modificado(Antes) (Copiar y Modificar)
		  job.next_operation = _executer_check_exists;
		  conflict = true;
		  job.opaque = &conflict;
		  ret = _executer_run_job (&job);
		}
	      break;
	    }
	  break;
	case OPENDFS_CREATE_DIR:
	  // TODO
	  error (EXIT_FAILURE, 0,
		 "Arreglar esto: se ha creado un directorio que hay en otro");
	  break;
	}
    }
  else
    {
      switch (mens->operacion)
	{
	case OPENDFS_DELETE:
	  //Borrado -> No tocado (Copiar y Borrar)
	  job.next_operation = _executer_delete;
	  ret = _executer_run_job (&job);
	  break;
	case OPENDFS_MODIFY:
	  //Modificado -> No tocado (Modificar sin copiar)
	  job.next_operation = _executer_check_exists;
	  conflict = false;
	  job.opaque = &conflict;
	  ret = _executer_run_job (&job);
	  break;
	case OPENDFS_CREATE_DIR:
	  job.next_operation = _executer_read_permission;
	  job.operation = EXECUTER_CREATE_DIR;
	  ret = _executer_run_job (&job);
	}
    }

  free (job.file);
  free (job.realpath);

  return ret;
}
