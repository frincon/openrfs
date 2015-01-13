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

#ifndef OPENRFS_H_
#define OPENRFS_H_

#include "config.h"
#include <time.h>

#define MY_NAME "openrfs"

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
  OPENRFS_DELETE, OPENRFS_MODIFY, OPENRFS_CREATE_DIR, OPENRFS_DELETE_DIR,
  OPENRFS_PING,
  OPENRFS_PONG
};

#define MAGIC_SERVER "OPENRFS_V0.1_MAGIC_SERVIDOR"
#define MAGIC_LEN 28

#endif
