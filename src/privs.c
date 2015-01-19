/*
 * privs.c
 *
 *  Created on: Jan 15, 2015
 *      Author: frincon
 */
#include <config.h>

#include "privs.h"

#include <sys/types.h>
#include <sys/fsuid.h>
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */


static __thread uid_t oldfsuid;
static __thread gid_t oldfsgid;

void
openrfs_drop_privs (uid_t uid, gid_t gid)
{

  oldfsuid = geteuid();
  oldfsgid = getegid();

  syscall(SYS_setresgid, -1, gid, -1);
  syscall(SYS_setresuid, -1, uid, -1);
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
