/*
 * queue.h
 *
 *  Created on: 24/02/2012
 *      Author: frincon
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <config.h>

typedef struct _queue_operation {
	char *file;
	struct tm time;
	int operation;
} queue_operation;

void queue_init();
void queue_add_operation(queue_operation *operation);
int queue_get_operation(queue_operation *operation);
int queue_get_operation_for_file(const char *file, queue_operation *operation);
void queue_close();

#endif /* QUEUE_H_ */
