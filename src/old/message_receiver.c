/*
 * message_receiver.c
 *
 *  Created on: Nov 30, 2014
 *      Author: frincon
 */

#include <stdbool.h>

#include "lock.h"
#include "metadata.h"
#include "message_receiver.h"

void
on_message_recived(orfs_message_t *message)
{

	orfs_metadata_t metadata;
	locker_t *locker;

	locker = lock_acquire(message->file_name, LOCK_STATE | LOCK_EXISTS | LOCK_OPEN_RW );
	metadata_get(message->file_name, &metadata);

	if (message->type == ORFS_MESSAGE_CREATE_MODIFY) {
		if(metadata.state == ORFS_STATE_EMPTY) {
			if(!metadata.exists) {
				enque_action(ORFS_ACTION_RECEIVE, message);
			} else {
				if(!metadata.is_open && compare_ts(metadata.ts, message->ts)<0) {
					enque_action(ORFS_ACTION_RECEIVE, message);
				} else if(metadata.is_open && compare_ts(metadata.ts, message->ts)<0) {
					enque_action(ORFS_ACTION_COC, message);
				}
			}
		} else if(metadata.state == ORFS_STATE_RECEIVE && compare_ts(metadata.message.ts, message->ts)<0) {
			remove_action(message->file_name);
			enque_action(ORFS_ACTION_RECEIVE, message);
		} else if(metadata.state == ORFS_STATE_COC && compre_ts(metadata.message.ts, message->ts)<0) {
			remove_action(metadata);
			enqueue_action(ORFS_ACTION_COC, message);
		}
	} else { // message->type = ORFS_MESSAGE_DELETE
		if ( metadata.exists) {
			if (!metadata.is_open && compare_ts(metadata.ts, message->ts)<0) {
				if (metadata.state == ORFS_STATE_EMPTY || compare_ts(metadata.message.ts, message->ts)<0) {
					remove_file(metadata);
				}
			} else if (metadata.is_open ) {
				if (metadata.state == ORFS_STATE_EMPTY && compare_ts(metadata.ts, message->ts) <0) {
					enqueue_action(ORFS_ACTION_COC, message);
				} else if (metadata.state == ORFS_STATE_COC && compare_ts(metadata.message.ts, message->ts)<0) {
					remove_action(metadata);
					enqueue_action(ORFS_ACTION_COC, message);
				} else if (!metadata.is_open && metadata.state == ORFS_STATE_RECEIVE && compare_ts(metadata.message.ts, message->ts) <0) {
					remove_action(metadata);
					remove_file(metadata);
				}
			}
		}
	}

	lock_release(locker);
}
