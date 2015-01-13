/*
 * lock.c
 *
 *  Created on: Dec 4, 2014
 *      Author: frincon
 */

#include <pthread.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include "error.h"
#include "lock.h"
#include "concurrent_hash_table.h"

static bool lock_initialized = false;
static concurrent_hash_table_t *lock_table_exists;
static concurrent_hash_table_t *lock_table_not_exists;
static pthread_mutex_t lock_table_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutexattr_t mattr_recursive;

static void
initialize_mutex(pthread_mutex_t *mutex, int size)
{
	for(int i=0; i<size; i++)
	{
		pthread_mutex_init(mutex++, &mattr_recursive);
	};
}

static pthread_mutex_t
*find_value_or_create(char *filename)
{
	pthread_mutex_t *file_mutex = NULL;
	struct stat file_stat;
	int ret;
	concurrent_hash_table_t *lock_table;
	void *key;

	pthread_mutex_lock(&lock_table_mutex);

	ret = lstat(filename, &file_stat);
	if(ret == 0) {
		lock_table = lock_table_exists;
		key = &file_stat.st_ino;
	} else {
		lock_table = lock_table_not_exists;
		key = filename;
	}

	file_mutex = concurrent_hash_table_get_value(lock_table, key);
	if(file_mutex == NULL) {
		file_mutex = (pthread_mutex_t *) malloc(LOCK_MAX_STATES * sizeof(pthread_mutex_t));
		initialize_mutex(file_mutex, LOCK_MAX_STATES);
		concurrent_hash_table_set_value(lock_table, key, file_mutex);
	}

	pthread_mutex_unlock(&lock_table_mutex);

	return file_mutex;

}

static uint16_t
hash_function_inode(void * key)
{
	int ret;
	unsigned long long *real_key;
	real_key = (unsigned long long *) key;
	ret = (int)(*real_key & 0xFFFF);
	assert(ret>=0 && ret<=65535);
	return ret;
}

static uint16_t
hash_function_filename(void *key)
{
	   unsigned long hash = 5381;
	    int c;
	    int ret;
	    char *real_string;

	    real_string = (char *) key;

	    while (c = *real_string++)
	        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	    ret = (int)(hash & 0xFFFF);
	    assert(ret>=0 && ret<=65535);
	    return ret;
}

void
lock_init()
{
	lock_table_exists = concurrent_hash_table_init(hash_function_inode);
	lock_table_not_exists = concurrent_hash_table_init(hash_function_filename);
	pthread_mutexattr_init(&mattr_recursive);
	pthread_mutexattr_settype(&mattr_recursive,  PTHREAD_MUTEX_RECURSIVE);
	lock_initialized = true;
}

locker_t
*lock_acquire(char *filename, int flags)
{
	int ret;
	pthread_mutex_t *file_mutex;

	assert(lock_initialized);

	file_mutex = find_value_or_create(filename);
	for(int i=0; i<LOCK_MAX_STATES; i++) {
		int flag = 2^(i+1);
		if(flags & flag) {
			ret = pthread_mutex_lock(file_mutex + i);
			error_fatal(ret);
		}
	}

}

void
lock_release(locker_t *locker)
{
	pthread_mutex_t *file_mutex;
	// TODO
}

void
lock_destroy()
{
	pthread_mutexattr_destroy(&mattr_recursive);
}

