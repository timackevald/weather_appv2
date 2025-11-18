/**
 * Header-file: event_watcher.h
 **/

#ifndef __event_watcher_h__
#define __event_watcher_h__

#include <stdint.h>
#include "../../include/config/config.h"

typedef struct event_watcher
{
	int fds[MAX_FD];
	uint8_t fd_count;
	int timeout_ms;
} event_watcher_t;

int8_t event_watcher_init(void);
int8_t event_watcher_reg_fd(int fd);
int8_t event_watcher_dereg_fd(int fd);
int event_watcher_ready(void); 

#endif /* __event_watcher_h__  */
