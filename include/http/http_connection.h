/**
 * Header-file: http_connection.h
 **/

#ifndef __http_connection_h__
#define __http_connection_h__

#include <stdint.h>
#include "../../include/task_scheduler/task_scheduler.h"

typedef enum
{
    HTTP_CONNECTION_IDLE       = 0,
    HTTP_CONNECTION_READING    = 1,
    HTTP_CONNECTION_PROCESSING = 2,
    HTTP_CONNECTION_SENDING    = 3,
    HTTP_CONNECTION_DONE       = 4
} http_connection_state_t;

typedef struct http_connection
{
    int fd;
    http_connection_state_t state;
    struct weather_server *upper_weather_server_layer;
    struct http_server *parent; // Mama server
    task_node_t node;
} http_connection_t;

int8_t http_connection_work(task_node_t *node);

#endif /* __http_connection_h__ */