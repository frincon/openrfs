/*
 * events.c
 *
 *  Created on: Jan 20, 2015
 *      Author: frincon
 */

#include "config.h"


#include "utils.h"

#include "events.h"

void
openrfs_on_modify(const char *path)
{
	utils_trace("On Modify received for path '%s'", path);
}

void
openrfs_on_delete(const char *path)
{
	utils_trace("On Delete received for path '%s'", path);
}
