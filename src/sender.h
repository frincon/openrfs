/*
 * sender.h
 *
 *  Created on: 26/02/2012
 *      Author: frincon
 */

#ifndef SENDER_H_
#define SENDER_H_

#include <config.h>
#include <pthread.h>

void sender_init(int *sock, pthread_t *pthread);
void sender_stop();

#endif /* SENDER_H_ */
