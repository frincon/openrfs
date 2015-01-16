dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([openrfs],[0.1.snapshot])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([admin])
AC_CONFIG_FILES(Makefile src/Makefile)
AC_CONFIG_TESTDIR([.],[src])


AM_INIT_AUTOMAKE([-Wall])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_LN_S
AM_PROG_CC_C_O
AC_GNU_SOURCE

PKG_CHECK_MODULES([FUSE], [fuse])
#AC_DEFINE_UNQUOTED([_FILE_OFFSET_BITS],[64],[Define LFS support])
AC_DEFINE_UNQUOTED([FUSE_USE_VERSION],[26],[Define fuse version])
AM_MISSING_PROG([AUTOM4TE], [autom4te])
AC_USE_SYSTEM_EXTENSIONS
AC_CHECK_DECL(_DIRENT_HAVE_D_TYPE, [], AC_MSG_ERROR([need _DIRENT_HAVE_D_TYPE]),[#include <dirent.h>])
# TODO Check for __GNUC__ macro because the built in atomic functions need it
AC_CHECK_LIB(sqlite3, sqlite3_open, [], AC_MSG_ERROR([need SQLITE3 library]))
AC_CHECK_LIB(rsync, rs_trace_set_level, [], AC_MSG_ERROR([need librsync library]))
AC_CHECK_LIB(confuse, cfg_parse, [], AC_MSG_ERROR([need confuse library]))
#AC_CHECK_LIB(fuse, fuse_opt_parse, [], AC_MSG_ERROR([need fuse library]))
AC_CHECK_LIB(ulockmgr, ulockmgr_op, [], AC_MSG_ERROR([need ulockmgr library]))
AC_CHECK_LIB(pthread, pthread_create, [], AC_MSG_ERROR([need pthread library]))
AC_CHECK_FUNCS(setxattr fdatasync)

AC_SYS_LARGEFILE

AC_OUTPUT
