/*
 * metadata.c
 *
 *  Created on: Dec 14, 2014
 *      Author: frincon
 */

#include <sys/stat.h>
#include <sys/xattr.h>
#include <stdbool.h>
#include <strings.h>

#include "lock.h"
#include "metadata.h"

int
metadata_get(char *filename, orfs_metadata_t *metadata)
{

	locker_t *locker;
	struct stat file_stat;
	int ret;

	locker = lock_acquire(filename, LOCK_EXISTS | LOCK_OPEN_RW | LOCK_STATE);

	strcpy(metadata->file_name, filename);

	ret = lstat(filename, &file_stat);
	if ( ret == 0)
	{
		metadata->exists = true;
		metadata->is_open = ;
		metadata->ts = file_stat.st_mtime;
	} else {
		metadata->exists = false;
		metadata->is_open = false;
	}

	metadata->message = ;
	metadata->state = ;

	metadata->exists
}


