/*
 * openrfs_fuse.c
 *
 *  Created on: Jan 19, 2015
 *      Author: frincon
 */

#include "config.h"

#include "openrfs_fuse.h"

#include "privs.h"

#include <fuse.h>



void
openrfs_fuse_drop_privs()
{
	  struct fuse_context *context;
	  context = fuse_get_context ();

	  openrfs_drop_privs(context->uid, context->gid);
}

void
openrfs_fuse_restore_privs()
{
	openrfs_restore_privs();
}
