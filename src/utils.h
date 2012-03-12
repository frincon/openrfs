/*
 *
 * opendfs -- Open source distributed file system
 *
 * Copyright (C) 2012 by Fernando Rincon <frm.rincon@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef UTILS_H_
#define UTILS_H_

#ifndef OPENDFS_TRACE_LEVEL
#define OPENDFS_TRACE_LEVEL UTILS_INFO
#endif /* ! OPENDFS_TRACE_LEVEL */

enum
{
  UTILS_ALL, UTILS_TRACE, UTILS_DEBUG, UTILS_INFO, UTILS_WARNING, UTILS_ERROR,
  UTILS_FATAL
};

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
