/**
 * Implementation file: task_scheduler.c
 **/

#include "../../include/task_scheduler/task_scheduler.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

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
	/**
	 * Nodes are embedded in module struct
	 * can't call free on them
	 **/
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
