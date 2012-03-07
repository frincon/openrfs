/*
 * sender.c
 *
 *  Created on: 26/02/2012
 *      Author: frincon
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "opendfs.h"
#include "queue.h"
#include "sender.h"

void *_sender_run(void *ptr);
pthread_t _thread_sender;

int _is_sender_stopped = 0;

void sender_init(int *sock, pthread_t *pthread) {
	int ret;
	ret = pthread_create(&_thread_sender, NULL, _sender_run, sock);
	*pthread = _thread_sender;
}

void sender_stop() {

	_is_sender_stopped = 1;
	pthread_join(_thread_sender, NULL);
}

void *_sender_run(void *ptr) {

	int sock = (*(int *) ptr);
	printf("sender: _sender_run: sock: %i\n", sock);
	fflush(stdout);

	//Esta es la tarea de envio
	while (!_is_sender_stopped) {
		queue_operation operation;
		int ret;
		ret = queue_get_operation(&operation);
		if (ret != 0) {
			//Hay operación, hay que enviar
			mensaje mens;
			mens.operacion = operation.operation;
			mens.file_size = strlen(operation.file) + 1;
			mens.time = operation.time;

			printf("sender: _sender_run: Enviando mensaje %s\n",
					operation.file);

			write(sock, &mens, sizeof(mens));
			write(sock, operation.file, strlen(operation.file) + 1);

			executer_send(sock, &mens, operation.file);

			fflush(stdout);
		} else {
			sleep(1);
		}
	}
	close(sock);
	return NULL;
}
