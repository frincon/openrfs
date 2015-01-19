/*
 * setid.h
 *
 *  Created on: Jan 15, 2015
 *      Author: frincon
 */

#ifndef SRC_SETID_H_
#define SRC_SETID_H_

#include <sys/types.h>

void
openrfs_drop_privs (uid_t uid, gid_t gid);

void
openrfs_restore_privs ();


#endif /* SRC_SETID_H_ */
