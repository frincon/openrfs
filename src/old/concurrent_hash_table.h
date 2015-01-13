/*
 * concurrent_hash_table.h
 *
 *  Created on: Dec 11, 2014
 *      Author: frincon
 */

#ifndef SRC_CONCURRENT_HASH_TABLE_H_
#define SRC_CONCURRENT_HASH_TABLE_H_

#include <stdint.h>
#include <pthread.h>


typedef struct concurrent_hash_table_t concurrent_hash_table_t;

concurrent_hash_table_t *
concurrent_hash_table_init(uint16_t (*hash_function) (void *key));

void
*concurrent_hash_table_get_value(concurrent_hash_table_t *table, void *key );

void
concurrent_hash_table_set_value(concurrent_hash_table_t *table, void *key, void *value);

void
concurrent_hash_table_destroy(concurrent_hash_table_t *table);

#endif /* SRC_CONCURRENT_HASH_TABLE_H_ */
