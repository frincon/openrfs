/*
 * executer.h
 *
 *  Created on: 27/02/2012
 *      Author: frincon
 */

#ifndef EXECUTER_H_
#define EXECUTER_H_

int executer_receive(int sock, mensaje *mens, const char *file);
int executer_send(int sock, mensaje *mens, const char *file);

#endif /* EXECUTER_H_ */
