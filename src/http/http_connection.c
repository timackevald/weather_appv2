/**
 * Implementation-file: http_connection.c
 **/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "../../include/http/http_connection.h"
#include "../../include/http/http_server.h"
#include "../../include/task_scheduler/task_scheduler.h"
#include "../../include/weather/weather_server.h"
#include "../../include/weather/weather_connection.h"
//#include "../../include/http/http_parser.h"


/**
 * THIS IA CALLBACK FUNCTION, WEATHER CHILD GIVES BACK RESPONSE TO HTTP CHILD
 **/
void http_connection_on_handled_request(struct http_connection *self, const char *weather_data)
{
	if (!self || !weather_data) return;
	printf("[HTTP CONNECTION] >> HTTP child now has weather childs response to client.\n");
	/**
	 * MOCK UP WITH snprintf() FOR NOW! USE http_parser-lib LATER INSTEAD!
	 **/
	snprintf(self->http_on_handled_request_response_buffer,
			 sizeof(self->http_on_handled_request_response_buffer),
		"HTTP/1.1 200 OK\r\n"
		"Content-type: text/plain\r\n"
		"Content-Length: %zu\r\n"
		"\r\n"
		"%s", strlen(weather_data), weather_data);
	self->state = HTTP_CONNECTION_SENDING;
	printf("[HTTP CONNECTION] >> HTTP child will try to send response to client...\n");
}

/**
 * THIS IS FOR TESTING, WILL BE REPLACED WITH http_parse-lib IN THE FUTURE!
 **/
static void parse_http_request(const char *raw, http_connection_request_t *req)
{
    memset(req, 0, sizeof(*req));
    
    // Parse first line: "GET /path?query HTTP/1.1"
    const char *space1 = strchr(raw, ' ');
    if (!space1) return;
    
    // Extract method
    size_t method_len = space1 - raw;
    if (method_len >= sizeof(req->method)) method_len = sizeof(req->method) - 1;
    strncpy(req->method, raw, method_len);
    
    // Find path
    const char *path_start = space1 + 1;
    const char *space2 = strchr(path_start, ' ');
    if (!space2) return;
    
    // Check for query string
    const char *query_start = strchr(path_start, '?');
    
    if (query_start && query_start < space2)
    {
        // Has query string
        size_t path_len = query_start - path_start;
        if (path_len >= sizeof(req->path)) path_len = sizeof(req->path) - 1;
        strncpy(req->path, path_start, path_len);
        
        size_t query_len = space2 - (query_start + 1);
        if (query_len >= sizeof(req->query)) query_len = sizeof(req->query) - 1;
        strncpy(req->query, query_start + 1, query_len);
    }
    else
    {
        // No query string
        size_t path_len = space2 - path_start;
        if (path_len >= sizeof(req->path)) path_len = sizeof(req->path) - 1;
        strncpy(req->path, path_start, path_len);
    }
    
    printf("[HTTP_PARSER] >> Method: %s, Path: %s, Query: %s\n", 
           req->method, req->path, req->query);
}

int8_t http_connection_work(task_node_t *node)
{
    http_connection_t *self = container_of(node, http_connection_t, node);
    
    printf("[HTTP CONNECTION WORK] >> Called, state=%d, fd=%d\n", self->state, self->fd);

    if (!self || self->fd < 0) return -1;

    switch (self->state)
    {
        case HTTP_CONNECTION_READING:
        {
            char buf[256];
            ssize_t r = read(self->fd, buf, sizeof(buf));
            
            printf("[HTTP CONNECTION] >> read() returned %ld, errno=%d\n", r, errno);

            if (r == 0)
            {
                printf("[HTTP CONNECTION] >> Client closed fd=%d\n", self->fd);
                task_scheduler_dereg_fd(self->fd);
                close(self->fd);
                self->fd = -1;
                self->state = HTTP_CONNECTION_IDLE;
                task_scheduler_remove(&self->node);
                return 0;
            }

            if (r < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    printf("[HTTP CONNECTION] >> No data yet (EAGAIN), will try again\n");
                    return 0;
                }
                printf("[HTTP CONNECTION] >> Read error: %d\n", errno);
                return 0;
            }

            printf("[HTTP CONNECTION] >> Received %ld bytes on fd=%d\n", r, self->fd);

            /**
             * IF BIGGER THAN BUFFER, CHOP IT!
             **/
            size_t copy_len = r < (ssize_t)sizeof(self->raw_http_buffer) ? (size_t)r : sizeof(self->raw_http_buffer) - 1;
            memcpy(self->raw_http_buffer, buf, copy_len);
            self->raw_http_buffer[copy_len] = '\0';
            self->state = HTTP_CONNECTION_PARSING;
            
            return 0;
        }
        
        case HTTP_CONNECTION_PARSING:
        {
            printf("[HTTP CONNECTION] >> HTTP child parsing request...\n");
            parse_http_request(self->raw_http_buffer, &self->parsed_request);
            self->state = HTTP_CONNECTION_PROCESSING;
            
            return 0;
        }

        case HTTP_CONNECTION_PROCESSING:
        {
            printf("[HTTP CONNECTION] >> HTTP child handing over request to weather child...\n");
            if (self->parent && self->parent->upper_weather_server_layer)
            {
                weather_connection_t *weather_connection =
                    weather_server_allocate_pool_slot(self->parent->upper_weather_server_layer);

                if (weather_connection)
                {
                    weather_connection->lower_http_connection = self;
                    printf("[HTTP CONNECTION] >> HTTP child got Weather child\n"
                           "[HTTP CONNECTION] >> Calling its callback...\n");
                    weather_connection->cb_from_http_layer.http_on_new_request(
                        weather_connection, &self->parsed_request);
                    printf("[HTTP CONNECTION] >> Returned from callback!\n");
                    self->state = HTTP_CONNECTION_WAITING;
                }
                else
                {
                    printf("[HTTP CONNECTION] >> Weather pool full, sending error\n");
                    snprintf(self->http_on_handled_request_response_buffer, 
                             sizeof(self->http_on_handled_request_response_buffer),
                        "HTTP/1.1 503 Service Unavailable\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: 21\r\n"
                        "\r\n"
                        "Service Unavailable\n");
                    self->state = HTTP_CONNECTION_SENDING;
                }
                
                return 0;
            }
            else
            {
                printf("[HTTP CONNECTION] >> No weather layer configured\n");
                snprintf(self->http_on_handled_request_response_buffer, 
                         sizeof(self->http_on_handled_request_response_buffer),
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 12\r\n"
                    "\r\n"
                    "Hello World\n");
                
                self->state = HTTP_CONNECTION_SENDING;
                return 0;
            }
        }
        
        case HTTP_CONNECTION_WAITING:
        {
            /**
             * WE WAIT FOR WEATHER CHILD PROCESS TO FINISH WORK!
             * Weather child will call our callback which changes state to SENDING
             **/
            return 0;
        }

        case HTTP_CONNECTION_SENDING:
        {
            printf("[HTTP CONNECTION] >> Sending response...\n");
            
            ssize_t written = write(self->fd, self->http_on_handled_request_response_buffer,
                                    strlen(self->http_on_handled_request_response_buffer));
            printf("[HTTP CONNECTION] >> Wrote %ld bytes\n", written);
            
            task_scheduler_dereg_fd(self->fd);
            close(self->fd);
            self->fd = -1;

            printf("[HTTP CONNECTION] >> Response sent, closing fd.\n");

            self->state = HTTP_CONNECTION_IDLE;
            task_scheduler_remove(&self->node);

            return 0;
        }

        default:
        {
            return 0;
        }
    }

    return 0;
}

