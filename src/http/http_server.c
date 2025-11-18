/**
 * Implementation-file: http_server.c
 **/
 
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../../include/http/http_server.h"
#include "../../include/task_scheduler/task_scheduler.h"
#include "../../include/event_watcher/event_watcher.h"
#include "../../include/logging/logging.h"

int8_t http_server_init(http_server_t *self, struct weather_server *upper_weather_server_layer)
{
    if (!self)
    {
        LOG_ERROR("[HTTP SERVER] Cannot init NULL server");
        return -1;
    }

    memset(self, 0, sizeof(*self));
    self->pool_size = CONNECTION_POOL_SIZE;
    self->active_count = 0;
    self->upper_weather_server_layer = upper_weather_server_layer;

    /* Initialize pool */
    for (int i = 0; i < CONNECTION_POOL_SIZE; i++)
    {
        self->child_http_connection[i].state = HTTP_CONNECTION_IDLE;
        self->child_http_connection[i].fd = -1;
        self->child_http_connection[i].parent = self;
        self->child_http_connection[i].node.work = http_connection_work;
        self->child_http_connection[i].node.active = 0;
        self->child_http_connection[i].cb_from_weather_layer.weather_on_handled_request = 
            http_connection_on_handled_request;
    }

    /* Assign callback for TCP -> HTTP hand-off */
    self->cb_from_tcp_layer.tcp_on_newly_accepted_client = http_server_on_new_client_cb;
    
    LOG_INFO("[HTTP SERVER] Initialized with pool size %d", CONNECTION_POOL_SIZE);
    return 0; /* FIXED: Was missing! */
}

http_connection_t *http_server_allocate_pool_slot(http_server_t *self)
{
    if (!self) return NULL;

    for (int i = 0; i < CONNECTION_POOL_SIZE; i++)
    {
        if (self->child_http_connection[i].state == HTTP_CONNECTION_IDLE)
        {
            LOG_DEBUG("[HTTP SERVER] Allocated pool slot [%d]", i);
            return &self->child_http_connection[i];
        }
    }
    
    LOG_WARN("[HTTP SERVER] Pool full!");
    return NULL;
}

void http_server_on_new_client_cb(struct http_server *self, int fd)
{
    if (!self || fd < 0)
    {
        LOG_ERROR("[HTTP SERVER] Invalid callback parameters");
        return;
    }

    http_connection_t *conn = http_server_allocate_pool_slot(self);
    if (!conn || self->active_count >= CONNECTION_POOL_SIZE)
    {
        LOG_WARN("[HTTP SERVER] Pool full, rejecting fd=%d", fd);
        close(fd);
        return;
    }

    /* Initialize connection */
    conn->fd = fd;
    conn->state = HTTP_CONNECTION_READING;
    conn->raw_http_buffer_len = 0;
    conn->response_len = 0;
    conn->sent_bytes = 0;
    
    memset(conn->raw_http_buffer, 0, sizeof(conn->raw_http_buffer));
    memset(conn->response_buffer, 0, sizeof(conn->response_buffer));
    memset(&conn->parsed_request, 0, sizeof(conn->parsed_request));

    task_scheduler_add(&conn->node);
    event_watcher_reg_fd(fd);

    self->active_count++;
    
    LOG_INFO("[HTTP SERVER] Accepted client fd=%d, pool index=%ld, active=%d",
             fd, conn - &self->child_http_connection[0], self->active_count);
}
