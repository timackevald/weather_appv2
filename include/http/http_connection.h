/**
 * Header-file: http_connection.h
 **/

#ifndef __http_connection_h__
#define __http_connection_h__

#include <stdint.h>
#include <time.h>
#include "../../include/task_scheduler/task_scheduler.h"
#include "../../include/config/config.h"

typedef struct http_connection http_connection_t;
typedef struct http_server http_server_t;

typedef enum
{
    HTTP_CONNECTION_IDLE       = 0,
    HTTP_CONNECTION_READING    = 1,
    HTTP_CONNECTION_PARSING    = 2,
    HTTP_CONNECTION_PROCESSING = 3,
    HTTP_CONNECTION_WAITING    = 4,
    HTTP_CONNECTION_SENDING    = 5,
    HTTP_CONNECTION_DONE       = 6,
    HTTP_CONNECTION_ERROR      = 7
} http_connection_state_t;

typedef struct http_connection_request
{
    char method[HTTP_METHOD_SIZE];
    char path[HTTP_PATH_SIZE];
    char query[HTTP_QUERY_SIZE];
    char version[16];
    char body[HTTP_BODY_SIZE];
} http_connection_request_t;

typedef struct http_connection_cb
{
    void (*weather_on_handled_request)(http_connection_t *self, const char *weather_data);
} http_connection_cb_t;

struct http_connection
{
    int fd;
    http_connection_state_t state;
    struct http_server *parent;
    task_node_t node;

    char raw_http_buffer[HTTP_RAW_BUFFER_SIZE];
    size_t raw_http_buffer_len;
    http_connection_request_t parsed_request;

    char response_buffer[HTTP_RESPONSE_BUFFER_SIZE];
    size_t response_len;
    size_t sent_bytes;

    http_connection_cb_t cb_from_weather_layer;

	time_t last_activity;
	int timeout_s;
};

int8_t http_connection_work(task_node_t *node);
void http_connection_on_handled_request(struct http_connection *self, const char *weather_data);
void http_connection_cleanup(http_connection_t *self);

#endif /* __http_connection_h__ */
