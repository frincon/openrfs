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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <confuse.h>
#include <librsync.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/fsuid.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <dirent.h>

#include <getopt.h>
#include <fuse.h>
#include <errno.h>
#include <stddef.h>
#include <ulockmgr.h>

#include "utils.h"
#include "openrfs_fuse.h"
#include "openrfs.h"
#include "events.h"

enum
{
  KEY_HELP, KEY_VERSION,
};

#define OPENRFS_OPT(t, p, v) { t, offsetof(configuration, p), v }


static void
convert_path (const char *path, char *path_resultado)
{
  // TODO Make it safer
  strcpy (path_resultado, config.path);
  strcat (path_resultado, "/");
  strcat (path_resultado, path);
}

static int
xmp_getattr (const char *path, struct stat *stbuf)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);
  openrfs_fuse_drop_privs ();
  res = lstat (path2, stbuf);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_fgetattr (const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
  int res;

  (void) path;

  openrfs_fuse_drop_privs ();
  res = fstat (fi->fh, stbuf);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_access (const char *path, int mask)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);

  openrfs_fuse_drop_privs ();
  res = access (path2, mask);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_readlink (const char *path, char *buf, size_t size)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);

  openrfs_fuse_drop_privs ();
  res = readlink (path2, buf, size - 1);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  buf[res] = '\0';
  return 0;
}

struct xmp_dirp
{
  DIR *dp;
  struct dirent *entry;
  off_t offset;
};

static int
xmp_opendir (const char *path, struct fuse_file_info *fi)
{
  int res;
  struct xmp_dirp *d = malloc (sizeof (struct xmp_dirp));
  if (d == NULL)
    return -ENOMEM;

  char path2[PATH_MAX];

  convert_path (path, path2);

  openrfs_fuse_drop_privs ();
  d->dp = opendir (path2);
  openrfs_fuse_restore_privs ();
  if (d->dp == NULL)
    {
      res = -errno;
      free (d);
      return res;
    }
  d->offset = 0;
  d->entry = NULL;

  fi->fh = (unsigned long) d;
  return 0;
}

static inline struct xmp_dirp *
get_dirp (struct fuse_file_info *fi)
{
  return (struct xmp_dirp *) (uintptr_t) fi->fh;
}

static int
xmp_readdir (const char *path, void *buf, fuse_fill_dir_t filler,
	     off_t offset, struct fuse_file_info *fi)
{
  struct xmp_dirp *d = get_dirp (fi);

  (void) path;
  openrfs_fuse_drop_privs ();
  if (offset != d->offset)
    {
      seekdir (d->dp, offset);
      d->entry = NULL;
      d->offset = offset;
    }
  while (1)
    {
      struct stat st;
      off_t nextoff;

      if (!d->entry)
	{
	  d->entry = readdir (d->dp);
	  if (!d->entry)
	    break;
	}

      memset (&st, 0, sizeof (st));
      st.st_ino = d->entry->d_ino;
      st.st_mode = d->entry->d_type << 12;
      nextoff = telldir (d->dp);
      if (filler (buf, d->entry->d_name, &st, nextoff))
	break;

      d->entry = NULL;
      d->offset = nextoff;
    }
  openrfs_fuse_restore_privs ();

  return 0;
}

static int
xmp_releasedir (const char *path, struct fuse_file_info *fi)
{
  struct xmp_dirp *d = get_dirp (fi);
  (void) path;
  openrfs_fuse_drop_privs ();
  closedir (d->dp);
  openrfs_fuse_restore_privs ();
  free (d);
  return 0;
}

static int
xmp_mknod (const char *path, mode_t mode, dev_t rdev)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);
  openrfs_fuse_drop_privs ();
  if (S_ISFIFO (mode))
    res = mkfifo (path2, mode);
  else
    res = mknod (path2, mode, rdev);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  openrfs_on_modify(path);
  return 0;
}

static int
xmp_mkdir (const char *path, mode_t mode)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);
  openrfs_fuse_drop_privs ();
  res = mkdir (path2, mode);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    {
      return -errno;
    }

  openrfs_on_modify(path);
  return 0;
}

static int
xmp_unlink (const char *path)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);
  openrfs_fuse_drop_privs ();
  res = unlink (path2);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  openrfs_on_delete(path);
  return 0;
}

static int
xmp_rmdir (const char *path)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);
  openrfs_fuse_drop_privs ();
  res = rmdir (path2);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  openrfs_on_delete(path);
  return 0;
}

static int
xmp_symlink (const char *from, const char *to)
{
  int res;
  char path2[PATH_MAX];

  convert_path (to, path2);
  openrfs_fuse_drop_privs ();
  res = symlink (from, path2);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  openrfs_on_modify(to);
  return 0;
}

static int
xmp_rename (const char *from, const char *to)
{
  int res;
  char from2[PATH_MAX];
  char to2[PATH_MAX];

  convert_path (from, from2);
  convert_path (to, to2);

  openrfs_fuse_drop_privs ();
  res = rename (from2, to2);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  // TODO Make more clever with one operation
  openrfs_on_delete(from);
  openrfs_on_modify(from);
  return 0;
}

static int
xmp_link (const char *from, const char *to)
{
  int res;
  char from2[PATH_MAX];
  char to2[PATH_MAX];

  convert_path (from, from2);
  convert_path (to, to2);
  openrfs_fuse_drop_privs ();
  res = link (from2, to2);
  openrfs_fuse_restore_privs ();

  if (res == -1)
    return -errno;

  // TODO This operation has to be handle in different way (inode number)
  openrfs_on_modify(from);
  return 0;
}

static int
xmp_chmod (const char *path, mode_t mode)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);
  openrfs_fuse_drop_privs ();
  res = chmod (path2, mode);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  // TODO This operation has to be handle more clever for performance to avoid check complete file
  openrfs_on_modify(path);
  return 0;
}

static int
xmp_chown (const char *path, uid_t uid, gid_t gid)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);
  openrfs_fuse_drop_privs ();
  res = lchown (path2, uid, gid);
  openrfs_fuse_restore_privs ();

  if (res == -1)
    return -errno;

  // TODO This operation has to be handle more clever for performance to avoid check complete file
  openrfs_on_modify(path);
  return 0;
}

static int
xmp_truncate (const char *path, off_t size)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);

  openrfs_fuse_drop_privs ();
  res = truncate (path2, size);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  openrfs_on_modify(path);
  return 0;
}

static int
xmp_ftruncate (const char *path, off_t size, struct fuse_file_info *fi)
{
  int res;

  openrfs_fuse_drop_privs ();
  res = ftruncate (fi->fh, size);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  openrfs_on_modify(path);
  return 0;
}

static int
xmp_utimens (const char *path, const struct timespec ts[2])
{
  int res;
  struct timeval tv[2];
  char path2[PATH_MAX];

  tv[0].tv_sec = ts[0].tv_sec;
  tv[0].tv_usec = ts[0].tv_nsec / 1000;
  tv[1].tv_sec = ts[1].tv_sec;
  tv[1].tv_usec = ts[1].tv_nsec / 1000;

  convert_path (path, path2);
  openrfs_fuse_drop_privs ();
  res = utimes (path2, tv);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  // TODO Should We do this remotely?
  return 0;
}

static int
xmp_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
  int fd;
  char path2[PATH_MAX];

  convert_path (path, path2);
  openrfs_fuse_drop_privs ();
  fd = open (path2, fi->flags, mode);
  openrfs_fuse_restore_privs ();
  if (fd == -1)
    return -errno;

  fi->fh = fd;
  return 0;
}

static int
xmp_open (const char *path, struct fuse_file_info *fi)
{
  int fd;
  char path2[PATH_MAX];

  convert_path (path, path2);
  openrfs_fuse_drop_privs ();
  fd = open (path2, fi->flags);
  openrfs_fuse_restore_privs ();
  if (fd == -1)
    return -errno;

  fi->fh = fd;
  return 0;
}

static int
xmp_read (const char *path, char *buf, size_t size, off_t offset,
	  struct fuse_file_info *fi)
{
  int res;

  openrfs_fuse_drop_privs ();
  res = pread (fi->fh, buf, size, offset);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    res = -errno;

  return res;
}

static int
xmp_write (const char *path, const char *buf, size_t size, off_t offset,
	   struct fuse_file_info *fi)
{
  int res;

  openrfs_fuse_drop_privs ();
  res = pwrite (fi->fh, buf, size, offset);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    res = -errno;

  return res;
}

static int
xmp_statfs (const char *path, struct statvfs *stbuf)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);

  openrfs_fuse_drop_privs ();
  res = statvfs (path2, stbuf);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_flush (const char *path, struct fuse_file_info *fi)
{
  int res;

  /* This is called from every close on an open file, so call the
     close on the underlying filesystem.        But since flush may be
     called multiple times for an open file, this must not really
     close the file.  This is important if used on a network
     filesystem like NFS which flush the data/metadata on close() */
  openrfs_fuse_drop_privs ();
  res = close (dup (fi->fh));
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  return 0;
}

static int
xmp_release (const char *path, struct fuse_file_info *fi)
{
  openrfs_fuse_drop_privs ();
  close (fi->fh);
  openrfs_fuse_restore_privs ();

  if (!(fi->flags & O_RDONLY)) {
	  openrfs_on_modify(path);
  }

  return 0;
}

static int
xmp_fsync (const char *path, int isdatasync, struct fuse_file_info *fi)
{
  int res;
  (void) path;

  openrfs_fuse_drop_privs ();
#ifndef HAVE_FDATASYNC
  (void) isdatasync;
#else
  if (isdatasync)
    res = fdatasync (fi->fh);
  else
#endif
    res = fsync (fi->fh);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int
xmp_setxattr (const char *path, const char *name, const char *value,
	      size_t size, int flags)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);
  openrfs_fuse_drop_privs ();
  res = lsetxattr (path2, name, value, size, flags);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  openrfs_on_modify(path);
  return 0;
}

static int
xmp_getxattr (const char *path, const char *name, char *value, size_t size)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);
  openrfs_fuse_drop_privs ();
  res = lgetxattr (path2, name, value, size);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;
  return res;
}

static int
xmp_listxattr (const char *path, char *list, size_t size)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);
  openrfs_fuse_drop_privs ();
  res = llistxattr (path, list, size);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;
  return res;
}

static int
xmp_removexattr (const char *path, const char *name)
{
  int res;
  char path2[PATH_MAX];

  convert_path (path, path2);
  openrfs_fuse_drop_privs ();
  res = lremovexattr (path, name);
  openrfs_fuse_restore_privs ();
  if (res == -1)
    return -errno;

  // TODO Avoid check of complete file
  openrfs_on_modify(path);
  return 0;
}
#endif /* HAVE_SETXATTR */

static int
xmp_lock (const char *path, struct fuse_file_info *fi, int cmd,
	  struct flock *lock)
{
  int ret;
  (void) path;

  openrfs_fuse_drop_privs ();
  ret = ulockmgr_op (fi->fh, cmd, lock, &fi->lock_owner,
		     sizeof (fi->lock_owner));
  openrfs_fuse_restore_privs ();
  return ret;
}

static struct fuse_operations xmp_oper = {
  .getattr = xmp_getattr,
  .fgetattr = xmp_fgetattr,
  .access = xmp_access,
  .readlink = xmp_readlink,
  .opendir = xmp_opendir,
  .readdir = xmp_readdir,
  .releasedir = xmp_releasedir,
  .mknod = xmp_mknod,
  .mkdir = xmp_mkdir,
  .symlink = xmp_symlink,
  .unlink = xmp_unlink,
  .rmdir = xmp_rmdir,
  .rename = xmp_rename,
  .link = xmp_link,
  .chmod = xmp_chmod,
  .chown = xmp_chown,
  .truncate = xmp_truncate,
  .ftruncate = xmp_ftruncate,
  .utimens = xmp_utimens,
  .create = xmp_create,
  .open = xmp_open,
  .read = xmp_read,
  .write = xmp_write,
  .statfs = xmp_statfs,
  .flush = xmp_flush,
  .release = xmp_release,
  .fsync = xmp_fsync,
#ifdef HAVE_SETXATTR
  .setxattr = xmp_setxattr,
  .getxattr = xmp_getxattr,
  .listxattr = xmp_listxattr,
  .removexattr = xmp_removexattr,
#endif
  .lock = xmp_lock,

  .flag_nullpath_ok = 1,
};

static struct fuse_opt myfs_opts[] = {
  OPENRFS_OPT ("path=%s", path, 0),
  OPENRFS_OPT ("log-file=%s", log_file, 0),
  OPENRFS_OPT ("debug-level=%i", debug_level, OPENRFS_TRACE_LEVEL),
  FUSE_OPT_KEY ("-V", KEY_VERSION),
  FUSE_OPT_KEY ("--version", KEY_VERSION),
  FUSE_OPT_KEY ("-h", KEY_HELP),
  FUSE_OPT_KEY ("--help", KEY_HELP),
  FUSE_OPT_END
};

static int
myfs_opt_proc (void *data, const char *arg, int key,
	       struct fuse_args *outargs)
{
  switch (key)
    {
    case KEY_HELP:
      fprintf (stderr, "usage: %s mountpoint [options]\n"
	       "\n"
	       "general options:\n"
	       "    -o opt,[opt...]  mount options\n"
	       "    -h   --help      print help\n"
	       "    -V   --version   print version\n"
	       "\n"
	       "OpenRFS options:\n" "    -o path=STRING\n", outargs->argv[0]);
      fuse_opt_add_arg (outargs, "-ho");
      fuse_main (outargs->argc, outargs->argv, &xmp_oper, NULL);
      exit (1);

    case KEY_VERSION:
      fprintf (stderr, "OpenRFS version %s\n", PACKAGE_VERSION);
      fuse_opt_add_arg (outargs, "--version");
      fuse_main (outargs->argc, outargs->argv, &xmp_oper, NULL);
      exit (0);
    }
  return 1;
}

int
main (int argc, char **argv)
{

  struct fuse_args args = FUSE_ARGS_INIT (argc, argv);
  memset (&config, 0, sizeof (config));
  if (fuse_opt_parse (&args, &config, myfs_opts, myfs_opt_proc))
    {
      utils_error ("Command line arguments error");
      exit (EXIT_FAILURE);
    }

  if (config.path == NULL)
    {
      utils_error ("No path specified");
      exit (EXIT_FAILURE);
    }

  FILE *log_stream = NULL;
  if (config.log_file != NULL) {
	  log_stream = fopen(config.log_file, "a");
	  if(log_stream == NULL) {
		  utils_error ("The log file '%s' cannot be opened: %s", config.log_file, strerror(errno) );
		  exit (EXIT_FAILURE);
	  }

	  utils_set_log_stream(log_stream);
  }
  utils_set_log_debug_level(config.debug_level);

  int ret = fuse_main (args.argc, args.argv, &xmp_oper, NULL);

  if (log_stream != NULL) {
	  fclose(log_stream);
  }

  return ret;
}
