/*
 * metadata.h
 *
 *  Created on: Dec 3, 2014
 *      Author: frincon
 */

#ifndef SRC_METADATA_H_
#define SRC_METADATA_H_

#include <stdbool.h>
#include <limits.h>
#include <time.h>

enum orfs_message_type {
	ORFS_MESSAGE_CREATE_MODIFY,
	ORFS_MESSAGE_DELETE
};

typedef struct  {
	enum orfs_message_type type;
	char* file_name;
	time_t ts;
} orfs_message_t;

enum orfs_action {
	ORFS_ACTION_RECEIVE,
	ORFS_ACTION_COC
};

enum orfs_state {
	ORFS_STATE_EMPTY,
	ORFS_STATE_RECEIVE,
	ORFS_STATE_COC
};

typedef struct  {
	enum orfs_state state;
	bool exists;
	bool is_open;
	orfs_message_t message;
	time_t ts;
	char file_name[PATH_MAX];
} orfs_metadata_t;

int
metadata_get(char *filename, orfs_metadata_t *metadata);

#endif /* SRC_METADATA_H_ */
