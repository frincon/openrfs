/*
 * error.c
 *
 *  Created on: Dec 4, 2014
 *      Author: frincon
 */


#include <stdio.h>
#include <stddef.h>
#include "error.h"

void
error_fatal (int ret)
{
	if (ret != 0) {
		perror(NULL);
		exit(1);
	}
}
