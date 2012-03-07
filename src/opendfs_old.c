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

#include "opendfs.h"
#include "queue.h"
#include "monitor.h"
#include "receiver.h"

void handleCtrlc(int signal) {
	puts("Ctrl-C");
	sender_stop();
	receiver_stop();
	monitor_stop();
}

const char magic_servidor[] = "OPENDFS_V0.1_MAGIC_SERVIDOR";
const char magic_cliente[] = "OPENDFS_V0.1_MAGIC_CLIENTE_";
const int magic_len = 28;

static void parse_options(int argc, char *argv[]) {
	static const struct option options[] = {

	{ "configfile", required_argument, NULL, 'c' },

	{ "port", required_argument, NULL, 'p' },

	{ "peer", required_argument, NULL, 'r' },

	{ "peerport", required_argument, NULL, 'o' },

	{ "help", no_argument, NULL, 'h' },

	{ "path", required_argument, NULL, 'P' },

	{ "database", required_argument, NULL, 'd' },

#ifdef _OPENDFS_MONITOR_FUSE

			{ "mountpoint", required_argument, NULL, 'm' },

#endif

			{ NULL, 0, NULL, 0 } };

	for (;;) {
		int opt, idx;

		opt = getopt_long(argc, argv, "c:p:r:o:hP:d:C:", options, &idx);
		switch (opt) {
		case -1:
			goto options_done;

		case '?':
			exit(EXIT_FAILURE);
		case 'p':
			config.port = atoi(optarg);
			break;
		case 'r':
			config.peerName = malloc(strlen(optarg) + 1);
			strcpy(config.peerName, optarg);
			break;
		case 'o':
			config.peerPort = atoi(optarg);
			break;
		case 'P':
			config.path = malloc(strlen(optarg) + 1);
			strcpy(config.path, optarg);
			break;
		case 'd':
			config.database = malloc(strlen(optarg) + 1);
			strcpy(config.database, optarg);
			break;
#ifdef _OPENDFS_MONITOR_FUSE
		case 'm':
			config.mountpoint = malloc(strlen(optarg) + 1);
			strcoy(config.mountpoint, optarg);
			break;
#endif
		default:
			abort();
			break;
		}
	}
	options_done: if (config.path == NULL) {
		error(EXIT_FAILURE, 0, "Error en los parametros");
	} else if (!(config.peerName == NULL && config.peerPort == 0
			&& config.port != 0)
			&& !(config.peerName != NULL && config.peerPort != 0
					&& config.port == 0)) {
		error(EXIT_FAILURE, 0, "Error en los parametros");
	}
#ifdef _OPENDFS_MONITOR_FUSE
	if (config.mountpoint == NULL) {
		error(EXIT_FAILURE, 0, "Error en los parametros");
	}
#endif

	if (config.database == NULL) {
		config.database = malloc(strlen(DBFILE) + 1);
		strcpy(config.database, DBFILE);
	}
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

	parse_options(argc, argv);

#ifndef _OPENDFS_MONITOR_FUSE
	pthread_t monitor_thread1;
	pthread_t monitor_thread2;
#endif
	pthread_t sender_thread;
	pthread_t receiver_thread;

	struct sigaction sigCtrlc;

	memset(&sigCtrlc, 0, sizeof(sigCtrlc));
	sigCtrlc.sa_handler = &handleCtrlc;

	static cfg_opt_t opts[] = { CFG_INT("port", 9999, CFGT_NONE), CFG_END() };

	rs_trace_set_level(RS_LOG_DEBUG);
	int i;
	for (i = 0; i < argc; i++) {
		puts(argv[i]);
	}

	//creamos el directorio .conflicts
	create_conflicts();

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

#ifdef _OPENDFS_MONITOR_FUSE
	monitor_init(config.path, config.mountpoint);
#else
	monitor_init(config.path, &monitor_thread1, &monitor_thread2);
	sigaction(SIGINT, &sigCtrlc, NULL);
	sigaction(SIGTERM, &sigCtrlc, NULL);
#endif

#ifndef _OPENDFS_MONITOR_FUSE
	pthread_join(monitor_thread1, NULL);
	pthread_join(monitor_thread2, NULL);
#else
	sender_stop();
	receiver_stop();
#endif
	pthread_join(sender_thread, NULL);
	pthread_join(receiver_thread, NULL);

	return EXIT_SUCCESS;
}

