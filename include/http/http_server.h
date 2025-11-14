/**
 * Header-file: http_server.h
 **/
 
#ifndef __http_server_h__
#define __http_server_h__

#include <stdint.h>
#include <sys/types.h>
#include "../../include/task_scheduler/task_scheduler.h"
#include "../../include/weather/weather_server.h"
#include "../../include/http/http_connection.h"

#define POOL_SIZE 32

typedef struct http_connection http_connection_t;
typedef struct http_server http_server_t;
typedef struct http_server_cb
{
    void (*tcp_on_newly_accepted_client)(struct http_server *self, int fd);
} http_server_cb_t;

struct http_server
{   
    uint8_t pool_size;
    uint8_t active_count;

    http_connection_t child_http_connection[POOL_SIZE];
    struct weather_server *upper_weather_server_layer;

    http_server_cb_t cb_from_tcp_layer;
    task_node_t node;
};

// int8_t http_server_work(task_node_t *node); NO NEED TO WORK RIGHT NOW!
int8_t http_server_init(http_server_t *self, struct weather_server *upper_weather_server_layer);
http_connection_t *http_server_allocate_pool_slot(http_server_t *self);
void http_server_on_new_client_cb(struct http_server *self, int fd); // Callback



#endif /* __http_server_h__ */
