/*
 * opendfs.h
 *
 *  Created on: 24/02/2012
 *      Author: frincon
 */

#ifndef OPENDFS_H_
#define OPENDFS_H_

#include <config.h>
#include <time.h>

typedef struct _configuration {

	int port;
	char *peerName;
	char *path;
	int peerPort;
	char *database;
	char *conflicts;

} configuration;

configuration config;

typedef struct _mensaje {
	int operacion;
	int file_size;
	struct tm time;
} mensaje;

enum {
	OPENDFS_DELETE = 0, OPENDFS_MODIFY = 1
};

#endif
