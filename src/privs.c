/*
 * privs.c
 *
 *  Created on: Jan 15, 2015
 *      Author: frincon
 */
#include <config.h>


#include <sys/types.h>
#include <fuse.h>
#include <sys/fsuid.h>
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */


static __thread uid_t oldfsuid;
static __thread gid_t oldfsgid;

void
openrfs_drop_privs ()
{
  struct fuse_context *context;
  context = fuse_get_context ();

  oldfsuid = geteuid();
  oldfsgid = getegid();

  syscall(SYS_setresgid, -1, context->gid, -1);
  syscall(SYS_setresuid, -1, context->uid, -1);
  /*
  oldfsuid = setfsuid (context->uid);
  oldfsgid = setfsuid (context->gid);
  */
}

void
openrfs_restore_privs ()
{
	  syscall(SYS_setresuid, -1, oldfsuid, -1);
	  syscall(SYS_setresgid, -1, oldfsgid, -1);
}
