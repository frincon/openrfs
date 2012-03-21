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

#ifndef OPENDFS_H_
#define OPENDFS_H_

#include <config.h>
#include <time.h>

#define MY_NAME "opendfs"

typedef struct _configuration
{

  int port;
  char *peer_name;
  char *path;
  int peer_port;
  char *database;
  char *conflicts;
  int debug_level;

} configuration;

configuration config;

typedef struct _mensaje
{
  int operacion;
  int file_size;
  struct tm time;
} mensaje;

enum
{
  OPENDFS_DELETE, OPENDFS_MODIFY, OPENDFS_CREATE_DIR, OPENDFS_PING,
  OPENDFS_PONG
};

#define MAGIC_SERVER "OPENDFS_V0.1_MAGIC_SERVIDOR"
#define MAGIC_LEN 28

#endif
