/**
 * Implementation file: task_scheduler.c
 **/

#include "../../include/task_scheduler/task_scheduler.h"
#include "../../include/logging/logging.h"
#include <stdint.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>

static task_scheduler_t g_task_scheduler;

int8_t task_scheduler_init(void)
{
    memset(&g_task_scheduler, 0, sizeof(g_task_scheduler));    
    LOG_INFO("[SCHEDULER] Initialized");
	
    return 0;
}

int8_t task_scheduler_deinit(void)
{
    g_task_scheduler.head = NULL;
    g_task_scheduler.count = 0;
    
    LOG_INFO("[SCHEDULER] Deinitialized");
    return 0;
}

int8_t task_scheduler_add(task_node_t *node)
{
    if (!node || !node->work) 
    {
        LOG_ERROR("[SCHEDULER] Cannot add NULL node or node without work function");
        return -1;
    }
    
    if (node->active) 
    {
        LOG_WARN("[SCHEDULER] Node already active, skipping add");
        return -1;
    }

    node->next = g_task_scheduler.head;
    g_task_scheduler.head = node;
    node->active = 1;
    g_task_scheduler.count++;

    LOG_DEBUG("[SCHEDULER] Added task, count=%d", g_task_scheduler.count);
    return 0;
}

int8_t task_scheduler_remove(task_node_t *node)
{
    if (!node)
    {
        LOG_ERROR("[SCHEDULER] Cannot remove NULL node");
        return -1;
    }

    task_node_t *curr = g_task_scheduler.head;
    task_node_t *prev = NULL;
    
    while (curr)
    {
        if (curr == node)
        {
            if (prev)
            {
                prev->next = curr->next;
            }
            else
            {
                g_task_scheduler.head = curr->next;
            }
            
            node->next = NULL;
            node->active = 0;
            g_task_scheduler.count--;

            LOG_DEBUG("[SCHEDULER] Removed task, count=%d", g_task_scheduler.count);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }

    LOG_WARN("[SCHEDULER] Task not found in list");
    return -1;
}

/**
 * FIXED: Better handling of node removal during iteration
 * Save next pointer before calling work function
 */
int8_t task_scheduler_work(void)
{
    task_node_t *node = g_task_scheduler.head;
    
    while (node)
    {
        /* Save next pointer BEFORE calling work - node might remove itself */
        task_node_t *next = node->next;
        
        if (node->active && node->work)
        {
            int8_t result = node->work(node);
            if (result != 0)
            {
                LOG_ERROR("[SCHEDULER] Task failed with error %d, removing", result);
                /* Only remove if node is still in the list */
                if (node->active)
                {
                    task_scheduler_remove(node);
                }
            }
        }
        
        node = next;
    }

    return 0;
}
