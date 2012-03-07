dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(prueba3, 1.0)
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([admin])


AM_INIT_AUTOMAKE([-Wall])

AC_PROG_CC
AM_PROG_CC_C_O


AC_DEFINE_UNQUOTED([_FILE_OFFSET_BITS],[64],[Define LFS support])
AC_DEFINE_UNQUOTED([_OPENDFS_MONITOR_FUSE],[1],[Define to use monitor fuse])
AC_DEFINE_UNQUOTED([FUSE_USE_VERSION],[26],[Define fuse version])
AC_GNU_SOURCE
AC_CHECK_DECL(_DIRENT_HAVE_D_TYPE, [], AC_MSG_ERROR([need _DIRENT_HAVE_D_TYPE]),[#include <dirent.h>])
AC_CHECK_LIB(sqlite3, sqlite3_open, [], AC_MSG_ERROR([need SQLITE3 library]))
AC_CHECK_LIB(rsync, rs_trace_set_level, [], AC_MSG_ERROR([need librsync library]))
AC_CHECK_LIB(confuse, cfg_parse, [], AC_MSG_ERROR([need confuse library]))
AC_CHECK_LIB(fuse, fuse_opt_parse, [], AC_MSG_ERROR([need fuse library]))
AC_CHECK_LIB(ulockmgr, ulockmgr_op, [], AC_MSG_ERROR([need ulockmgr library]))

AC_SYS_LARGEFILE

AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT
