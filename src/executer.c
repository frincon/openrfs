/*
 * executer.c
 *
 *  Created on: 27/02/2012
 *      Author: frincon
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <librsync.h>

#include <sys/types.h>
#include <fcntl.h>

#include "opendfs.h"
#include "queue.h"
#include "executer.h"

enum {
	_EXECUTER_OK_EXISTS = 0, _EXECUTER_OK_NOT_EXISTS = 1, _EXECUTER_REVERT = 2
};

enum {
	_EXECUTER_PART = 0, _EXECUTER_FINISH = 1
};

enum executer_rs_type {
	_EXECUTER_SOCKET = 0, _EXECUTER_FILE = 1
};

#define _EXECUTER_DEFAULT_OUT 1024

typedef struct _executer_rs {
	int sock;
	int type;
} executer_rs;

bool _executer_send_buffer(int sock, void *buf, size_t size) {
	int ret = 0;
	size_t writed = 0;

	while (writed < size) {
		ret = write(sock, buf, size - writed);
		if (ret < 0)
			return -1;
		writed += ret;
		buf = buf + ret;
	}
	return writed == size;

}

bool _executer_send_part(int sock, char *buf, size_t size) {
	char *buf2;
	size_t total = size + sizeof(size_t) + sizeof(_EXECUTER_PART);

	buf2 = malloc(total);
	memset(buf2, 0x0, total);
	*buf2 = _EXECUTER_PART;
	buf2 += sizeof(_EXECUTER_PART);
	*buf2 = size;
	buf2 += sizeof(size_t);
	memcpy(buf2, buf, size);
	buf2 -= sizeof(size_t) + sizeof(_EXECUTER_PART);

	bool resultado = _executer_send_buffer(sock, buf2, total);
	free(buf2);
	return resultado;
}

bool _executer_send_finish(int sock) {
	int buf;
	buf = _EXECUTER_FINISH;
	return (_executer_send_buffer(sock, &buf, sizeof(int)));
}

void _executer_real_path(const char *file, char **path) {
	char *path2;
	path2 = malloc(strlen(config.path) + strlen(file) + 1);
	strcpy(path2, config.path);
	strcat(path2, file);

	printf("executer: _executer_real_path: file: %s path: %s\n", file, path2);

	*path = path2;
}

void _executer_conflict_path(const char *file, char **path) {
	char date_formatted[50];
	const char time_format[] = "%Y_%m_%d_%H_%M_%s";
	char *path2;
	char *base;
	time_t date;
	struct tm *date_tm;

	base = basename(file);

	date = time(NULL);
	date_tm = gmtime(&date);
	strftime(date_formatted, 50, time_format, date_tm);

	path2 = malloc(
			strlen(config.conflicts) + strlen(base) + strlen(date_formatted)
					+ 3);

	strcpy(path2, config.conflicts);
	strcat(path2, "/");
	strcat(path2, base);
	strcat(path2, ".");
	strcat(path2, date_formatted);

	printf("executer: _executer_conflict_path: file: %s path: %s\n", file,
			path2);

	*path = path2;
}

int _executer_delete(int sock, mensaje *mens, const char *file) {
	int ret;
	struct stat buf;
	char *path;
	char *newpath;

	printf("executer: _executer_delete: %s\n", file);

	_executer_real_path(file, &path);

	printf("executer: _executer_delete: path: %s\n", path);

//Miramos si existe el fichero:
	ret = lstat(path, &buf);
	if (ret != 0) {
		warn("No se ha encontrado un fichero a borrar");
		return EXIT_SUCCESS;
	}

//Existe, lo movemos, cambiandolo de nombre

	_executer_conflict_path(file, &newpath);

	if (rename(path, newpath) != 0) {
		error(EXIT_FAILURE, errno, "Error moviendo el fichero");
	}
	free(newpath);
	free(path);

	return EXIT_SUCCESS;
}

int _executer_copy_conflict(const char *file) {
	char *source;
	char *target;
	char *command;
	int ret;

	char copy_command[] = "cp -a %s %s";

// Cojemos los path
	_executer_real_path(file, &source);
	_executer_conflict_path(file, &target);

	command = malloc(
			strlen(copy_command) + strlen(source) + strlen(target) - 4 + 1);
	sprintf(command, copy_command, source, target);

	ret = system(command);

	free(source);
	free(target);

	return ret;

}

rs_result _executer_in_callback(rs_job_t *job, rs_buffers_t *buf, void *opaque) {

	static char buff[RS_DEFAULT_BLOCK_LEN];
	int leido;
	int haymas;
	executer_rs *executer = (executer_rs *) opaque;

	char *buf2;

	buf2 = 0x0;

	//Primero copiamos si queda algo
	if (buf->avail_in > 0) {
		buf2 = malloc(buf->avail_in);
		memcpy(buf2, buf->next_in, buf->avail_in);
	}

	memset(&buff, 0x0, RS_DEFAULT_BLOCK_LEN);
	//Si se ha alcanzado al final, se devuelve done
	if (buf->eof_in)
		return RS_DONE;

	if (executer->type == _EXECUTER_SOCKET) {
		//Comprobamos que hay mas datos y los leemos, si no ponemos
		leido = read(executer->sock, &haymas, sizeof(haymas));
		if (leido != sizeof(haymas))
			error(EXIT_FAILURE, 0, "Error de protocolo");

		if (haymas == _EXECUTER_PART) {
			//Hay mas se lee
			size_t size;

			//Se lee el tamaño
			leido = read(executer->sock, &size, sizeof(size));
			if (leido != sizeof(size))
				error(EXIT_FAILURE, 0, "Error de protocolo");

			//Se lee el contenido
			char *buf3;
			buf3 = malloc(size);
			leido = read(executer->sock, buf3, size);
			if (leido != sizeof(size))
				error(EXIT_FAILURE, 0, "Error de protocolo");

			if (size + buf->avail_in > RS_DEFAULT_BLOCK_LEN)
				error(EXIT_FAILURE, 0, "Esto hay que arreglarlo!!!");

			if (buf->avail_in > 0) {
				//Habia, copiamos
				memcpy(&buff, buf2, buf->avail_in);
			}
			memcpy((&buff) + buf->avail_in, buf3, size);
			buf->avail_in = size + buf->avail_in;
			buf->next_in = &buff;
			free(buf3);
			if (buf2 != 0)
				free(buf2);
			return RS_DONE;
		} else {
			// No hay mas, se envia el final
			buf->eof_in = true;
			return RS_DONE;
		}
	} else {
		char *buf3;
		buf3 = malloc(RS_DEFAULT_BLOCK_LEN - buf->avail_in);
		leido = read(executer->sock, buf3,
				RS_DEFAULT_BLOCK_LEN - buf->avail_in);
		if (leido == 0) {
			//No hay mas, se envia el final
			buf->eof_in = true;
			return RS_DONE;
		} else if (leido > 0) {
			//Hay mas se envia
			if (buf->avail_in > 0) {
				//Habia, copiamos
				memcpy(&buff, buf2, buf->avail_in);
			}
			memcpy((&buff) + buf->avail_in, buf3, leido);
			buf->avail_in = leido + buf->avail_in;
			buf->next_in = &buff;
			free(buf3);
			if (buf2 != 0)
				free(buf2);
			return RS_DONE;
		} else {
			error(EXIT_FAILURE, 0, "Error leyendo el fichero");
		}
	}

}

rs_result _executer_out_callback(rs_job_t *job, rs_buffers_t *buf, void *opaque) {
	static char buff[_EXECUTER_DEFAULT_OUT];
	executer_rs *executer = (executer_rs *) opaque;

	if (buf->avail_out < 20) {
		//Enviamos ya lo que hay
		if (executer->type == _EXECUTER_FILE) {
			if (!_executer_send_buffer(executer->sock, &buff,
					_EXECUTER_DEFAULT_OUT - buf->avail_out))
				error(EXIT_FAILURE, 0, "Error escribiendo fichero");
		} else {
			if (!_executer_send_part(executer->sock, &buff,
					_EXECUTER_DEFAULT_OUT - buf->avail_out))
				error(EXIT_FAILURE, 0, "Error escribiendo socket");
		}
		buf->avail_out = _EXECUTER_DEFAULT_OUT;
		buf->next_out = &buff;
		return RS_DONE;
	} else {
		//Todavia no enviamos
		return RS_DONE;
	}

}

int _executer_send_ok(int sock, mensaje *mens, const char *file, bool conflict) {
	bool exists = false;
	struct stat bufstat;
	char *realpath;

	printf("executer: _executer_send_ok: %s\n", file);

	_executer_real_path(file, &realpath);

	exists = !lstat(realpath, &bufstat);
	// Primero creamos
	if (conflict && exists) {
		//Copiamos a conflict
		if (_executer_copy_conflict(file)) {
			error(EXIT_FAILURE, 0, "No se ha podido copiar un conflict.");
		}
	}

	// Muy bien, enviamos un OK para que empiece el rsync
	if (exists) {
		rs_job_t *sigjob;
		rs_buffers_t rsbuf;

		printf(
				"executer: _executer_send_ok: El fichero existe, enviando signature\n",
				file);

		//Creamos la firma y la enviamos
		int operacion = _EXECUTER_OK_EXISTS;
		if (!_executer_send_buffer(sock, &operacion, sizeof(int)))
			error(EXIT_FAILURE, 0, "Error mandando OK");

		int file = open(realpath, O_RDONLY);

		executer_rs inrs, outrs;
		inrs.sock = file;
		inrs.type = _EXECUTER_FILE;

		outrs.sock = sock;
		outrs.type = _EXECUTER_SOCKET;

		sigjob = rs_sig_begin(RS_DEFAULT_BLOCK_LEN, RS_DEFAULT_STRONG_LEN);
		rs_result result = rs_job_drive(sigjob, &rsbuf, _executer_in_callback,
				&inrs, _executer_out_callback, &outrs);
		rs_job_free(sigjob);
		if (result != RS_DONE)
			error(EXIT_FAILURE, 0, "Error en rsync");

		close(file);
		_executer_send_finish(sock);

		//Recibimos el delta y parcheamos
		//TODO

	} else {
		int operacion = _EXECUTER_OK_NOT_EXISTS;
		if (!_executer_send_buffer(sock, &operacion, sizeof(int)))
			error(EXIT_FAILURE, 0, "Error mandando OK_NOT_EXISTS");
	}

	free(realpath);

	return EXIT_SUCCESS;
}

int _executer_send_revert(int sock, mensaje *mens, const char *file) {
	printf("executer: _executer_send_revert: %s\n", file);
	return EXIT_SUCCESS;
}

int executer_receive(int sock, mensaje *mens, const char *file) {
// Bien, me han enviado un fichero, debo comprobar en que estado está aquí.

	/* Puede haber varias posibilidades:
	 *
	 *	Borrado -> No tocado (Copiar y Borrar) *
	 *	Borrado -> Borrado (Nada que hacer) *
	 *	Borrado -> Modificado(Antes) (Copiar y Borrar) *
	 *	Borrado -> Modificado(Despues) (Enviar al otro) *
	 *	Modificado -> No tocado (Modificar) *
	 *	Modificado -> Borrado(Antes) (Modificar)
	 *	Modificado -> Borrado(Despues) (Nada que hacer, cuando toque se borrará)
	 *	Modificado -> Modificado(Antes) (Copiar y Modificar)
	 *	Modificado -> Modificado(Despues) (Enviar al otro (copiar y modificar)
	 */

	int ret;
	queue_operation operation;

	ret = queue_get_operation_for_file(file, &operation);
	if (ret != 0) {
		time_t time1 = mktime(&(mens->time));
		time_t time2 = mktime(&(operation.time));

		switch (mens->operacion) {
		case OPENDFS_DELETE:
			switch (operation.operation) {
			case OPENDFS_DELETE:
				//Borrado -> Borrado (Nada que hacer)
				return EXIT_SUCCESS;
				break;
			case OPENDFS_MODIFY:
				//Puede que sea antes o despues
				if (time2 > time1) {
					//Borrado -> Modificado(Despues) (Enviar al otro)
					return _executer_send_revert(sock, mens, file);
				} else {
					//Borrado -> Modificado(Antes) (Copiar y Borrar)
					return _executer_delete(sock, mens, file);
				}
				break;
			}
			break;
		case OPENDFS_MODIFY:
			switch (operation.operation) {
			case OPENDFS_DELETE:
				if (time2 > time1) {
					// Modificado -> Borrado(Despues)
					return EXIT_SUCCESS;
				} else {
					//Modificado -> Borrado(Antes) (Modificar)
					return _executer_send_ok(sock, mens, file, false);
				}
				break;
			case OPENDFS_MODIFY:
				if (time2 > time1) {
					//Modificado -> Modificado(Despues)
					return _executer_send_revert(sock, mens, file);
				} else {
					//Modificado -> Modificado(Antes) (Copiar y Modificar)
					return _executer_send_ok(sock, mens, file, true);
				}
				break;
			}
			break;
		}
	} else {
		switch (mens->operacion) {
		case OPENDFS_DELETE:
			//Borrado -> No tocado (Copiar y Borrar)
			return _executer_delete(sock, mens, file);
			break;
		case OPENDFS_MODIFY:
			//Modificado -> No tocado (Modificar)
			return _executer_send_ok(sock, mens, file, false);
			break;
		}
	}

	return EXIT_FAILURE;
}

bool _executer_send_exists(int sock, const char *file) {

	int continua;
	rs_signature_t *signature;
	rs_job_t *job;
	rs_buffers_t rsbuf;
	rs_result result;
	executer_rs inrs;
	executer_rs outrs;
	char *realpath;

	//Cargamos la signature
	job = rs_loadsig_begin(&signature);
	inrs.type = _EXECUTER_SOCKET;
	inrs.sock = sock;
	result = rs_job_drive(job, &rsbuf, _executer_in_callback, &inrs, NULL,
			NULL);
	rs_job_free(job);
	if (result != RS_DONE)
		error(EXIT_FAILURE, 0, "Error en el rsync");

	//Ok ya tenemos la signature, enviamos el delta
	job = rs_delta_begin(&signature);

	_executer_real_path(file, &realpath);
	int file2 = open(realpath, O_RDONLY);

	inrs.type = _EXECUTER_FILE;
	inrs.sock = file2;
	outrs.type = _EXECUTER_SOCKET;
	outrs.sock = sock;
	result = rs_job_drive(job, &rsbuf, _executer_in_callback, &inrs,
			_executer_out_callback, &outrs);
	rs_job_free(job);
	if (result != RS_DONE)
		error(EXIT_FAILURE, 0, "Error en el rsync");

	//Muy bien, ya hemos enviado el delta, se supone que el otro extremo a parchado el fichero

	free(realpath);

	return true;

}

bool _executer_wait_modify(int sock, const char *file) {
	//Esperamos o que exista o que no exista
	int operacion;
	size_t leido = read(sock, &operacion, sizeof(operacion));
	if (leido != sizeof(operacion))
		error(EXIT_FAILURE, 0, "Error leyendo la operacion");
	switch (operacion) {
	case _EXECUTER_OK_EXISTS:
		//Existe, llamamos a send_exists
		return _executer_send_exists(sock, file);
		break;
	case _EXECUTER_OK_NOT_EXISTS:
		break;
	}
	return true;
}

int executer_send(int sock, mensaje *mens, const char *file) {

	switch (mens->operacion) {
	case OPENDFS_DELETE:
		// TODO Esperar respuesta OK
		break;
	case OPENDFS_MODIFY:
		if (!_executer_wait_modify(sock, file))
			error(EXIT_FAILURE, 0, "Error esperando respuesta");
	}
}

