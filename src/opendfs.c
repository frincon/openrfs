/*
 ============================================================================
 Name        : prueba3.c
 Author      : Fernando Rincon
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>

#include <getopt.h>
#include <fuse.h>
#include <errno.h>
#include <stddef.h>
#include <ulockmgr.h>

#include "opendfs.h"
#include "queue.h"
#include "monitor.h"
#include "receiver.h"

const char magic_servidor[] = "OPENDFS_V0.1_MAGIC_SERVIDOR";
const char magic_cliente[] = "OPENDFS_V0.1_MAGIC_CLIENTE_";
const int magic_len = 28;

enum {
	KEY_HELP, KEY_VERSION,
};

#define MYFS_OPT(t, p, v) { t, offsetof(configuration, p), v }

void convert_path(const char *path, char **path_resultado) {
	char *path2;
	path2 = malloc(strlen(path) + strlen(config.path) + 2);\
	strcpy(path2, config.path);\
	strcat(path2, "/");\
	strcat(path2, path);

	*path_resultado = path2;
}

static int xmp_getattr(const char *path, struct stat *stbuf) {
	int res;
	char *path2;

	convert_path(path, &path2);
	// printf("opendfs: xmp_getattr: path: %s path2: %s\n", path, path2);
	res = lstat(path2, stbuf);
	free(path2);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_fgetattr(const char *path, struct stat *stbuf,
		struct fuse_file_info *fi) {
	int res;

	(void) path;

	res = fstat(fi->fh, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask) {
	int res;
	char *path2;

	convert_path(path, &path2);

	res = access(path2, mask);
	free(path2);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size) {
	int res;

	char *path2;

	convert_path(path, &path2);
	// printf("opendfs: xmp_readlink: path: %s path2: %s\n", path, path2);
	res = readlink(path2, buf, size - 1);
	free(path2);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}

struct xmp_dirp {
	DIR *dp;
	struct dirent *entry;
	off_t offset;
};

static int xmp_opendir(const char *path, struct fuse_file_info *fi) {
	int res;
	struct xmp_dirp *d = malloc(sizeof(struct xmp_dirp));
	if (d == NULL)
		return -ENOMEM;

	char *path2;

	convert_path(path, &path2);
	printf("opendfs: xmp_opendir: path: %s path2: %s\n", path, path2);

	d->dp = opendir(path2);
	free(path2);
	if (d->dp == NULL) {
		res = -errno;
		free(d);
		return res;
	}
	d->offset = 0;
	d->entry = NULL;

	fi->fh = (unsigned long) d;
	return 0;
}

static inline struct xmp_dirp *get_dirp(struct fuse_file_info *fi) {
	return (struct xmp_dirp *) (uintptr_t) fi->fh;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi) {
	struct xmp_dirp *d = get_dirp(fi);

	(void) path;
	if (offset != d->offset) {
		seekdir(d->dp, offset);
		d->entry = NULL;
		d->offset = offset;
	}
	while (1) {
		struct stat st;
		off_t nextoff;

		if (!d->entry) {
			d->entry = readdir(d->dp);
			if (!d->entry)
				break;
		}

		memset(&st, 0, sizeof(st));
		st.st_ino = d->entry->d_ino;
		st.st_mode = d->entry->d_type << 12;
		nextoff = telldir(d->dp);
		if (filler(buf, d->entry->d_name, &st, nextoff))
			break;

		d->entry = NULL;
		d->offset = nextoff;
	}

	return 0;
}

static int xmp_releasedir(const char *path, struct fuse_file_info *fi) {
	struct xmp_dirp *d = get_dirp(fi);
	(void) path;
	closedir(d->dp);
	free(d);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev) {
	int res;
	char *path2;

	convert_path(path, &path2);
	if (S_ISFIFO(mode))
		res = mkfifo(path2, mode);
	else
		res = mknod(path2, mode, rdev);
	free(path2);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode) {
	int res;
	char *path2;

	convert_path(path, &path2);
	res = mkdir(path2, mode);
	free(path2);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path) {
	int res;
	char *path2;

	convert_path(path, &path2);
	res = unlink(path2);
	free(path2);
	if (res == -1)
		return -errno;
	else {
		queue_operation op;
		op.file = path;
		op.operation = OPENDFS_DELETE;
		queue_add_operation(&op);
	}

	return 0;
}

static int xmp_rmdir(const char *path) {
	int res;
	char *path2;

	convert_path(path, &path2);
	res = rmdir(path2);
	free(path2);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to) {
	int res;

	//TODO

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to) {
	int res;

	// TODO
	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to) {
	int res;

	//TODO
	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode) {
	int res;
	char *path2;

	convert_path(path, &path2);
	res = chmod(path2, mode);
	free(path2);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid) {
	int res;
	char *path2;

	convert_path(path, &path2);
	res = lchown(path2, uid, gid);
	free(path2);

	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size) {
	int res;

	char *path2;

	convert_path(path, &path2);

	res = truncate(path2, size);
	free(path2);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_ftruncate(const char *path, off_t size,
		struct fuse_file_info *fi) {
	int res;

	(void) path;

	res = ftruncate(fi->fh, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2]) {
	int res;
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;
	char *path2;

	convert_path(path, &path2);
	res = utimes(path2, tv);
	free(path2);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	int fd;
	char *path2;

	convert_path(path, &path2);
	fd = open(path2, fi->flags, mode);
	free(path2);
	if (fd == -1)
		return -errno;

	fi->fh = fd;
	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
	int fd;
	char *path2;

	convert_path(path, &path2);
	fd = open(path2, fi->flags);
	free(path2);
	if (fd == -1)
		return -errno;

	fi->fh = fd;
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi) {
	int res;

	(void) path;
	res = pread(fi->fh, buf, size, offset);
	if (res == -1)
		res = -errno;

	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		off_t offset, struct fuse_file_info *fi) {
	int res;

	(void) path;
	res = pwrite(fi->fh, buf, size, offset);
	if (res == -1)
		res = -errno;

	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf) {
	int res;
	char *path2;

	convert_path(path, &path2);

	res = statvfs(path2, stbuf);
	free(path2);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_flush(const char *path, struct fuse_file_info *fi) {
	int res;

	(void) path;
	/* This is called from every close on an open file, so call the
	 close on the underlying filesystem.	But since flush may be
	 called multiple times for an open file, this must not really
	 close the file.  This is important if used on a network
	 filesystem like NFS which flush the data/metadata on close() */
	res = close(dup(fi->fh));
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi) {
	(void) path;
	close(fi->fh);

	//Comprobamos si se ha abierto en read_only
	if (fi->flags & (O_RDWR | O_WRONLY)) {
		queue_operation op;
		op.file = path;
		op.operation = OPENDFS_MODIFY;
		printf(
				"opendfs: xmp_release: %s closed write opened, sending queue, flags: %i\n",
				path, fi->flags);
		queue_add_operation(&op);
	} else {
		printf("opendfs: xmp_release: %s closed reanonly opened, flags: %i\n",
				path, fi->flags);
	}

	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		struct fuse_file_info *fi) {
	int res;
	(void) path;

#ifndef HAVE_FDATASYNC
	(void) isdatasync;
#else
	if (isdatasync)
	res = fdatasync(fi->fh);
	else
#endif
	res = fsync(fi->fh);
	if (res == -1)
		return -errno;

	return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
		size_t size, int flags)
{
	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
	return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
		size_t size)
{
	int res = lgetxattr(path, name, value, size);
	if (res == -1)
	return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	int res = llistxattr(path, list, size);
	if (res == -1)
	return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	int res = lremovexattr(path, name);
	if (res == -1)
	return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static int xmp_lock(const char *path, struct fuse_file_info *fi, int cmd,
		struct flock *lock) {
	(void) path;

	return ulockmgr_op(fi->fh, cmd, lock, &fi->lock_owner,
			sizeof(fi->lock_owner));
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
		.lock = xmp_lock
		,

		.flag_nullpath_ok = 1, };

static struct fuse_opt myfs_opts[] = {
		MYFS_OPT("port=%i", port, 0),
		MYFS_OPT("peername=%s", peerName, 0),
		MYFS_OPT("path=%s", path, 0),
		MYFS_OPT("peerport=%i", peerPort, 0),
		MYFS_OPT("database=%s", database, 0),
		MYFS_OPT("conflicts=%s", conflicts, 0),

		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END };

static int myfs_opt_proc(void *data, const char *arg, int key,
		struct fuse_args *outargs) {
	switch (key) {
	case KEY_HELP:
		fprintf(stderr, "usage: %s mountpoint [options]\n"
				"\n"
				"general options:\n"
				"    -o opt,[opt...]  mount options\n"
				"    -h   --help      print help\n"
				"    -V   --version   print version\n"
				"\n"
				"Myfs options:\n"
				"    -o mynum=NUM\n"
				"    -o mystring=STRING\n"
				"    -o mybool\n"
				"    -o nomybool\n"
				"    -n NUM           same as '-omynum=NUM'\n"
				"    --mybool=BOOL    same as 'mybool' or 'nomybool'\n",
				outargs->argv[0]);
		fuse_opt_add_arg(outargs, "-ho");
		fuse_main(outargs->argc, outargs->argv, &xmp_oper, NULL);
		exit(1);

	case KEY_VERSION:
		fprintf(stderr, "Myfs version %s\n", PACKAGE_VERSION);
		fuse_opt_add_arg(outargs, "--version");
		fuse_main(outargs->argc, outargs->argv, &xmp_oper, NULL);
		exit(0);
	}
	return 1;
}

int make_socket(uint16_t port) {
	int sock;
	struct sockaddr_in name;

	/* Create the socket. */
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/* Give the socket a name. */
	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	return sock;
}

void start_server(int *sock_servidor, int *sock_cliente) {
	int sock;
	fd_set active_fd_set, read_fd_set;
	int i;
	struct sockaddr_in clientname;
	socklen_t size;

	char buffer[magic_len];

	int new1 = 0, new2 = 0;

	// Arrancamos el servidor
	sock = make_socket(config.port);
	if (listen(sock, 1) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	FD_ZERO(&active_fd_set);
	FD_SET(sock, &active_fd_set);

	read_fd_set = active_fd_set;
	while (new1 == 0 || new2 == 0) {

		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		}

		/* Connection request on original socket. */
		int new;
		size = sizeof(clientname);
		new = accept(sock, (struct sockaddr *) &clientname, &size);
		if (new < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		fprintf(stderr, "Server: connect from host %s, port %hd.\n",
				inet_ntoa(clientname.sin_addr), clientname.sin_port);

		//Miramos si es conexión cliente o servidor
		int total;
		total = read(new, buffer, magic_len - 1);
		buffer[magic_len - 1] = '\0';
		if (total != magic_len - 1) {
			error(0, 0, "opendfs: start_server: ERROR en MAGIC");
			close(new);
		}
		if (strcmp(magic_servidor, buffer) == 0) {
			new2 = new;
		} else if (strcmp(magic_cliente, buffer) == 0) {
			new1 = new;
		} else {
			error(0, 0, "opendfs: start_server: ERROR en MAGIC");
			close(new);
		}

	}
	//Cerramos el listen
	close(sock);
	*sock_servidor = new1;
	*sock_cliente = new2;
}

void init_sockaddr(struct sockaddr_in *name, const char *hostname,
		uint16_t port) {
	struct hostent *hostinfo;

	name->sin_family = AF_INET;
	name->sin_port = htons(port);
	hostinfo = gethostbyname(hostname);
	if (hostinfo == NULL) {
		fprintf(stderr, "Unknown host %s.\n", hostname);
		exit(EXIT_FAILURE);
	}
	name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
}

void start_client(int *sock_servidor, int *sock_cliente) {
	int sock;
	struct sockaddr_in servername;
	/* Create the socket. */
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket (client)");
		exit(EXIT_FAILURE);
	}

	/* Connect to the server. */
	init_sockaddr(&servername, config.peerName, config.peerPort);
	if (0
			> connect(sock, (struct sockaddr *) &servername, sizeof(servername))) {
		perror("connect (client)");
		exit(EXIT_FAILURE);
	}

	//Enviamos el magic
	write(sock, magic_servidor, magic_len - 1);
	*sock_servidor = sock;

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket (client)");
		exit(EXIT_FAILURE);
	}

	/* Connect to the server. */
	init_sockaddr(&servername, config.peerName, config.peerPort);
	if (0
			> connect(sock, (struct sockaddr *) &servername, sizeof(servername))) {
		perror("connect (client)");
		exit(EXIT_FAILURE);
	}
	//enviamos el magic
	write(sock, magic_cliente, magic_len - 1);
	*sock_cliente = sock;

}

void create_conflicts() {
	DIR *dp;

	struct dirent *dirp;
	char path[512];
	strcpy(path, config.path);
	strcat(path, "/");
	strcat(path, CONFLICTS);
	if ((dp = opendir(path)) == NULL) {
		if (mkdir(path, 0700) != 0) {
			perror(NULL);
			error(EXIT_FAILURE, 0, "Error creando conflicts");
		}
	}

	config.conflicts = malloc(strlen(path) + 1);
	strcpy(config.conflicts, path);

}

int main(int argc, char **argv) {

	int sock_servidor;
	int sock_cliente;

	pthread_t sender_thread;
	pthread_t receiver_thread;

	rs_trace_set_level(RS_LOG_DEBUG);

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	memset(&config, 0, sizeof(config));
	if (fuse_opt_parse(&args, &config, myfs_opts, myfs_opt_proc))
		error(EXIT_FAILURE, 0, "Error en los parametros");

	if (config.port != 0) {
		start_server(&sock_servidor, &sock_cliente);
	} else {
		start_client(&sock_servidor, &sock_cliente);
	}

	printf("opendfs: main: Ya tenemos los dos sockets...\n");
	if (sock_cliente <= 0 || sock_servidor <= 0)
		error(EXIT_FAILURE, 0, "Error abriendo el socket");

	queue_init();

	//Ya tenemos el socket abierto... solo queda enviar y recivir
	sender_init(&sock_cliente, &sender_thread);
	receiver_init(&sock_servidor, &receiver_thread);

	//creamos el directorio .conflicts
	create_conflicts();

	int ret = fuse_main(args.argc, args.argv, &xmp_oper, NULL);

	sender_stop();
	receiver_stop();

	pthread_join(sender_thread, NULL);
	pthread_join(receiver_thread, NULL);

	return ret;
}

