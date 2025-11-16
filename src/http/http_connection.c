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
    if (!node) return -1;

    http_connection_t *self = container_of(node, http_connection_t, node);
    if (!self || self->fd < 0) return -1;

    switch (self->state)
    {
        case HTTP_CONNECTION_READING:
        {
            ssize_t r = read(self->fd,
                             self->raw_http_buffer + self->raw_http_buffer_len,
                             sizeof(self->raw_http_buffer) - self->raw_http_buffer_len - 1);
			
			if (strstr(self->raw_http_buffer, "\r\n\r\n"))
			{
				self->state = HTTP_CONNECTION_PARSING;
			}
			else if (self->raw_http_buffer_len >= sizeof(self->raw_http_buffer) - 1)
			{
				// Buffer full but no complete request
				printf("[HTTP] Request too large, closing fd=%d\n", self->fd);
				task_scheduler_dereg_fd(self->fd);
                close(self->fd);
                self->fd = -1;
				if (self->parent)
				{
					self->parent->active_count--;
				}
                task_scheduler_remove(&self->node);
                self->state = HTTP_CONNECTION_IDLE;		
			}

            if (r > 0)
            {
                self->raw_http_buffer_len += r;
                self->raw_http_buffer[self->raw_http_buffer_len] = '\0';
                self->state = HTTP_CONNECTION_PARSING;
                printf("[HTTP] Read %ld bytes\n", r);
            }
            else if (r == 0)
            {
                // Client closed connection
                printf("[HTTP] Client closed fd=%d\n", self->fd);
                task_scheduler_dereg_fd(self->fd);
                close(self->fd);
                self->fd = -1;
				if (self->parent)
				{
					self->parent->active_count--;
				}
                task_scheduler_remove(&self->node);
                self->state = HTTP_CONNECTION_IDLE;

            }
            else if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                perror("[HTTP] read");
                task_scheduler_remove(&self->node);
            }

            return 0;
        }

        case HTTP_CONNECTION_PARSING:
        {
            // Use existing parse function (or replace with sturdy library later)
            parse_http_request(self->raw_http_buffer, &self->parsed_request);
            self->state = HTTP_CONNECTION_PROCESSING;
            return 0;
        }

        case HTTP_CONNECTION_PROCESSING:
        {
            if (self->parent && self->parent->upper_weather_server_layer)
            {
                weather_connection_t *weather_connection =
                    weather_server_allocate_pool_slot(self->parent->upper_weather_server_layer);

                if (weather_connection)
                {
                    weather_connection->lower_http_connection = self;
                    weather_connection->cb_from_http_layer.http_on_new_request(
                        weather_connection, &self->parsed_request);

                    self->state = HTTP_CONNECTION_WAITING;
                    return 0;
                }
                else
                {
                    // Weather pool full
                    snprintf(self->http_on_handled_request_response_buffer,
                             sizeof(self->http_on_handled_request_response_buffer),
                             "HTTP/1.1 503 Service Unavailable\r\n"
                             "Content-Type: text/plain\r\n"
                             "Content-Length: 21\r\n\r\n"
                             "Service Unavailable\n");
                    self->sent_bytes = 0;
                    self->state = HTTP_CONNECTION_SENDING;
                    return 0;
                }
            }

            // No weather layer configured
            snprintf(self->http_on_handled_request_response_buffer,
                     sizeof(self->http_on_handled_request_response_buffer),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Length: 12\r\n\r\n"
                     "Hello World\n");
            self->sent_bytes = 0;
            self->state = HTTP_CONNECTION_SENDING;
            return 0;
        }

        case HTTP_CONNECTION_WAITING:
        {
            // Waiting for weather callback to complete
            return 0;
        }

        case HTTP_CONNECTION_SENDING:
        {
            size_t total_len = strlen(self->http_on_handled_request_response_buffer);
            ssize_t written = write(self->fd,
                                    self->http_on_handled_request_response_buffer + self->sent_bytes,
                                    total_len - self->sent_bytes);

            if (written > 0)
            {
                self->sent_bytes += written;

                if (self->sent_bytes >= total_len)
                {
                    // Finished sending
                    task_scheduler_dereg_fd(self->fd);
                    close(self->fd);
                    self->fd = -1;
					if (self->parent)
					{
						self->parent->active_count--;
					}
                    task_scheduler_remove(&self->node);
                    self->state = HTTP_CONNECTION_IDLE;
                    self->raw_http_buffer_len = 0;
                    self->sent_bytes = 0;
                }
            }
            else if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                perror("[HTTP] write");
                task_scheduler_remove(&self->node);
            }

            return 0;
        }

        default:
            return 0;
    }

    return 0;
}
