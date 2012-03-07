/*
 * receiver.h
 *
 *  Created on: 26/02/2012
 *      Author: frincon
 */

#ifndef RECEIVER_H_
#define RECEIVER_H_

#include <config.h>

#include <pthread.h>

void receiver_init(int *sock, pthread_t *pthread);
void receiver_stop();

#endif /* RECEIVER_H_ */
