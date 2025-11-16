/**
 * Header-file: http_connection.h
 **/

#ifndef __http_connection_h__
#define __http_connection_h__

#include <stdint.h>
#include "../../include/task_scheduler/task_scheduler.h"

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
    HTTP_CONNECTION_DONE       = 6
} http_connection_state_t;

typedef struct http_connection_request
{
	char method[16];
	char path[256];
	char query[256];
	char body[512];
} http_connection_request_t;

typedef struct http_connection_cb
{
	void (*weather_on_handled_request)(http_connection_t *self, const char *weather_data);
} http_connection_cb_t;


struct http_connection
{
    int fd;
    http_connection_state_t state;
    struct http_server *parent; // Mama server
    task_node_t node;

	char raw_http_buffer[1024]; // Raw http request goes here
	http_connection_request_t parsed_request;
	char http_on_handled_request_response_buffer[2048]; // Response to client
	http_connection_cb_t cb_from_weather_layer;	// Callback from weather layer
};

int8_t http_connection_work(task_node_t *node);
void http_connection_on_handled_request(struct http_connection *self, const char *weather_data);

#endif /* __http_connection_h__ */
