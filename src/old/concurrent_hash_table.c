/*
 * concurrent_hash_table.c
 *
 *  Created on: Dec 11, 2014
 *      Author: frincon
 */

#include <stdlib.h>
#include <string.h>
#include "concurrent_hash_table.h"


struct concurrent_hash_table_t
{
	void *array[0xFFFF];
	uint16_t (*hash_function) (void *key);
	pthread_mutex_t mutex;
};

concurrent_hash_table_t *
concurrent_hash_table_init(uint16_t (*hash_function) (void *key))
{
	concurrent_hash_table_t *table;

	table = malloc(sizeof(concurrent_hash_table_t));
	memset(table->array, 0, sizeof(table->array));
	table->hash_function = hash_function;
	pthread_mutex_init(&table->mutex, NULL);
	return 0;
}


void
*concurrent_hash_table_get_value(concurrent_hash_table_t *table, void *key )
{
	uint16_t real_key = table->hash_function(key);
	return table->array[real_key];
}

void
concurrent_hash_table_set_value(concurrent_hash_table_t *table, void *key, void *value)
{
	uint16_t real_key = table->hash_function(key);
	// TODO Change to a real CAS
	pthread_mutex_lock(&table->mutex);
	table->array[real_key] = value;
	pthread_mutex_unlock(&table->mutex);
}

void
concurrent_hash_table_destroy(concurrent_hash_table_t *table)
{
	pthread_mutex_destroy(&table->mutex);
	free(table);
}

