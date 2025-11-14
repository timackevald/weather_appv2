/**
 * Implementation-file: http_server.c
 **/
 
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../../include/http/http_server.h"
#include "../../include/task_scheduler/task_scheduler.h"

int8_t http_server_init(http_server_t *self, struct weather_server *upper_weather_server_layer)
{
    if (!self) return -1;

    memset(self, 0, sizeof(*self));
    self->pool_size = POOL_SIZE;
    self->active_count = 0;
    self->upper_weather_server_layer = upper_weather_server_layer;

    /**
     * Init pool and mark spots as free, come swim!
     **/
    for (int i = 0; i < POOL_SIZE; i++)
    {
        self->child_http_connection[i].state       = HTTP_CONNECTION_IDLE;
        self->child_http_connection[i].parent      = self;
        self->child_http_connection[i].node.work   = http_connection_work;
        self->child_http_connection[i].node.active = 0;

    }

    /* Assign callback för TCP -> HTTP hand-off */
    self->cb_from_tcp_layer.tcp_on_newly_accepted_client = http_server_on_new_client_cb;
    return 0;
}

http_connection_t *http_server_allocate_pool_slot(http_server_t *self)
{
    if (!self) return NULL;

    for (int i = 0; i < POOL_SIZE; i++)
    {
        if (self->child_http_connection[i].state == HTTP_CONNECTION_IDLE)
        {
            printf("Found a spot (location [%d]) in the pool for child process...\n", i);
            return &self->child_http_connection[i];
        }
    }
    printf("[HTTP SERVER] >> Rejecting client, pool full!\n");
    return NULL;
}

/**
 * THIS IS A CALL BACK FUNCTION, THIS IS WHERE TCP HANDS OVER CONTROL TO HTTP
 **/
void http_server_on_new_client_cb(struct http_server *self, int fd)
{
    if (!self || fd < 0) return;

    http_connection_t *connection = http_server_allocate_pool_slot(self);
    if (!connection || self->active_count >= POOL_SIZE)
    {
        printf("[HTTP SERVER] >> Rejecting client, pool full!\n");
        close(fd);
    }

    connection->fd    = fd;
    connection->state = HTTP_CONNECTION_READING;
    connection->upper_weather_server_layer = self->upper_weather_server_layer;
    /**
     * All engines start, connection is active and registered with the scheduler,
     * it will now be called upon to do some work from time to time... 
     **/
    connection->node.active = 0;
    task_scheduler_add(&connection->node);
	task_scheduler_reg_fd(fd);
    self->active_count++; // One more kid in the pool...
    printf("[HTTP SERVER] >> Client connection fd=%d is active and working\n", fd);
}
