/**
 * Header file: task_scheduler.h
 **/

#ifndef __task_scheduler_h__
#define __task_scheduler_h__

#include <string.h>
#include <stddef.h>
#include <stdint.h>

/**
 * container_of macro - gets parent struct from member pointer
 */
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

typedef struct task_node task_node_t;

/**
 * Callback function that all modules will use to
 * call back to the scheduler
 **/
typedef int8_t (*task_scheduler_work_fn)(task_node_t *node);

struct task_node
{
	task_scheduler_work_fn work;
	task_node_t *next;
	uint8_t active;
	
};

typedef struct task_scheduler
{
	task_node_t *head;
	uint8_t     count;
	int         fds[64]; // Configure me!
	uint8_t     fd_count;
   
} task_scheduler_t;

int8_t task_scheduler_init();
int8_t task_scheduler_deinit();
int8_t task_scheduler_add(task_node_t *node);
int8_t task_scheduler_remove(task_node_t *node);
int8_t task_scheduler_work();
int8_t task_scheduler_reg_fd(int fd);
int8_t task_scheduler_dereg_fd(int fd);
int8_t task_scheduler_run();
	
#endif /* __task_scheduler_h__ */
