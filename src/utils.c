/*
 *
 * opendfs -- Open source distributed file system
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

#include <config.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "opendfs.h"
#include "utils.h"



void
utils_log (int level, const char *file_name, const char *function,
	   const char *template, ...)
{
  va_list arguments;
  if (level >= OPENDFS_TRACE_LEVEL)
    {
      // TODO Make this with dynamic memory allocation
      char buf[1000];
      char message[1000];
      va_start (arguments, template);


      //Print message in buf
      vsnprintf (buf, 999, template, arguments);

      if (file_name != NULL && function != NULL)
	{
	  sprintf (message, "%s: %s:%s: %s\n", MY_NAME, file_name, function,
		   buf);
	}
      else if (file_name != NULL)
	{
	  sprintf (message, "%s: %s: %s\n", MY_NAME, file_name, buf);
	}
      else
	{
	  sprintf (message, "%s: %s\n", MY_NAME, buf);
	}

      va_end (arguments);
      fputs (message, stderr);
    }
}
