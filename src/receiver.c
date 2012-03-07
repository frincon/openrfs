/*
 * receiver.c
 *
 *  Created on: 26/02/2012
 *      Author: frincon
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include "opendfs.h"
#include "queue.h"
#include "executer.h"
#include "receiver.h"

pthread_t _thread_receiver;

int _is_receiver_stopped = 0;

void *_receiver_run(void *ptr) {
	fd_set active_fd_set, read_fd_set;
	int i;
	struct sockaddr_in clientname;
	socklen_t size;
	struct timeval timeout;

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	int sock = (*(int *) ptr);

	printf("receiver: _receiver_run: sock: %i\n", sock);
	fflush(stdout);
	//Esta es la tarea de envio
	while (!_is_receiver_stopped) {

		FD_ZERO(&active_fd_set);
		FD_SET(sock, &active_fd_set);
		read_fd_set = active_fd_set;

		int ret = select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);
		printf("receiver: _receiver_run: salido de select: ret: %i sock: %i\n", ret,
				sock);
		fflush(stdout);
		if (ret < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		} else if (FD_ISSET(sock, &read_fd_set)) {
			int leido;
			char *file;

			// Hay que leer
			mensaje mens;
			leido = read(sock, &mens, sizeof(mens));
			if (leido == 0) {
				//El socket se ha cerrado
				receiver_stop();
				break;
			}
			if (leido != sizeof(mens))
				error(
						EXIT_FAILURE,
						0,
						"Error leyendo datos del socket1, espedaro: %i, leido: %i",
						sizeof(mens), leido);
			file = malloc(mens.file_size);
			leido = read(sock, file, mens.file_size);
			if (leido != mens.file_size) {
				perror(NULL);
				error(
						EXIT_FAILURE,
						0,
						"Error leyendo datos del socket2, esperado: %i, leido: %i",
						mens.file_size, leido);
			}

			printf("receiver: _receiver_run: Leido del socket, operacion: %i, file: %s\n", mens.operacion,
					file);
			ret = executer_receive(sock, &mens, file);
			free(file);
			fflush(stdout);
		}
	}
	close(sock);
	return NULL;
}

void receiver_init(int *sock, pthread_t *pthread) {
	int ret;
	ret = pthread_create(&_thread_receiver, NULL, _receiver_run, sock);
	*pthread = _thread_receiver;
}

void receiver_stop() {
	_is_receiver_stopped = 1;
	pthread_join(_thread_receiver, NULL);
}

