/**
 * Implementation file: task_scheduler.c
 **/

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sys/select.h>
#include <errno.h>

#include "../../include/task_scheduler/task_scheduler.h"

/**
 * Singelton object: 
 **/
static task_scheduler_t g_task_scheduler;

int8_t task_scheduler_init()
{
	memset(&g_task_scheduler, 0, sizeof(g_task_scheduler));
	
	return 0;
}

int8_t task_scheduler_deinit()
{
	g_task_scheduler.head = NULL;
	g_task_scheduler.count = 0;
	
	return 0;
}

int8_t task_scheduler_add(task_node_t *node)
{
	if (!node || !node->work) return -1;
	if (node->active) return -1;

	node->next = g_task_scheduler.head;
	g_task_scheduler.head = node;
	node->active = 1;
	g_task_scheduler.count++;

	return 0;	
}

int8_t task_scheduler_remove(task_node_t *node)
{
	if (!node) return -1;

	task_node_t *curr = g_task_scheduler.head;
	task_node_t *prev = NULL;
	while (curr)
	{
		if (curr == node) // Found node
		{
			if (prev) // Decouple node
			{
				prev->next = curr->next;
			}
			else
			{
				g_task_scheduler.head = curr->next;
			}
			node->next   = NULL;
			node->active = 0;
			g_task_scheduler.count--;

			return 0;
		}
		prev = curr;
		curr = curr->next;
	}

	return -1;
}

int8_t task_scheduler_work()
{
    //printf("[SCHEDULER] >> Running, count=%d\n", g_task_scheduler.count); // Debugg
	task_node_t *node = g_task_scheduler.head;
	while (node)
	{
		if (node->active && node->work)
		{
			int8_t work = node->work(node);
			if (work != 0)
			{
				printf("[SCHEDULER] >> Task failed, removing it from list.\n");
				task_scheduler_remove(node);
			}
		}
		node = node->next;
	}

	return 0;
}

int8_t task_scheduler_reg_fd(int fd)
{
	if (fd < 0) return -1;
	if (g_task_scheduler.fd_count >= 64) return -1;
	
	/* Check if already registered */
	for (int i = 0; i < g_task_scheduler.fd_count; i++)
	{
		if (g_task_scheduler.fds[i] == fd) return 0; /* Already registered */
	}
	
	g_task_scheduler.fds[g_task_scheduler.fd_count++] = fd;
	return 0;
}

int8_t task_scheduler_dereg_fd(int fd)
{
	if (fd < 0) return -1;
	
	for (int i = 0; i < g_task_scheduler.fd_count; i++)
	{
		if (g_task_scheduler.fds[i] == fd)
		{
			/* Shift remaining fds down */
			for (int j = i; j < g_task_scheduler.fd_count - 1; j++)
			{
				g_task_scheduler.fds[j] = g_task_scheduler.fds[j + 1];
			}
			g_task_scheduler.fd_count--;
			return 0;
		}
	}
	
	return -1;
}

/**
 * Use select() to wait for I/O activity on registered file descriptors
 * This blocks until there's activity or timeout, then runs all tasks
 **/
int8_t task_scheduler_run()
{
	fd_set readfds;
	FD_ZERO(&readfds);
	
	int max_fd = -1;
	
	/* Add all registered file descriptors to the set */
	for (int i = 0; i < g_task_scheduler.fd_count; i++)
	{
		int fd = g_task_scheduler.fds[i];
		FD_SET(fd, &readfds);
		if (fd > max_fd) max_fd = fd;
	}
	
	/* Set timeout to 10ms so we still poll regularly */
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;  /* 10ms */
	int ready = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
	
	if (ready < 0)
	{
		if (errno == EINTR) return 0;  /* Interrupted, just continue */
		perror("[SCHEDULER] >> select");
		return -1;
	}
	
	/* Run all tasks (they'll check their own fds) */
	return task_scheduler_work();
}
