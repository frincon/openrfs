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

#ifndef UTILS_H_
#define UTILS_H_

#include <pthread.h>
#include <stdio.h>

#ifndef OPENRFS_TRACE_LEVEL
#define OPENRFS_TRACE_LEVEL UTILS_INFO
#endif /* ! OPENRFS_TRACE_LEVEL */



enum
{
  UTILS_ALL, UTILS_TRACE, UTILS_DEBUG, UTILS_INFO, UTILS_WARNING, UTILS_ERROR,
  UTILS_FATAL
};

void
utils_set_log_stream(FILE *stream);

void
utils_set_log_debug_level(int level);

void
utils_log (int level, const char *file_name, const char *function,
	   const char *template, ...);

#define utils_warn(fmt, arg...)                            \
    do { utils_log(UTILS_WARNING, __FILE__, __FUNCTION__,  fmt , ##arg);  \
    } while (0)

#define utils_debug(fmt, arg...)                            \
    do { utils_log(UTILS_DEBUG, __FILE__, __FUNCTION__,  fmt , ##arg);  \
    } while (0)

#define utils_fatal(fmt, arg...)                            \
    do { utils_log(UTILS_FATAL, __FILE__, __FUNCTION__,  fmt , ##arg);  \
    } while (0)

#define utils_error(fmt, arg...)                            \
    do { utils_log(UTILS_ERROR, __FILE__, __FUNCTION__,  fmt , ##arg);  \
    } while (0)

#define utils_info(fmt, arg...)                            \
    do { utils_log(UTILS_INFO, __FILE__, __FUNCTION__,  fmt , ##arg);  \
    } while (0)

#define utils_trace(fmt, arg...)                            \
    do { utils_log(UTILS_TRACE, __FILE__, __FUNCTION__,  fmt , ##arg);  \
    } while (0)

#endif /* ! UTILS_H_ */
