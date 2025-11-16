/**
 * Implementation-file: http_connection.c
 **/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "../../include/http/http_connection.h"
#include "../../include/http/http_server.h"
#include "../../include/task_scheduler/task_scheduler.h"
#include "../../include/weather/weather_server.h"
#include "../../include/weather/weather_connection.h"
#include "../../include/logging/logging.h"

/**
 * FIXED: Unified cleanup function for proper state management
 */
void http_connection_cleanup(http_connection_t *self)
{
    if (!self) return;
    
    LOG_DEBUG("[HTTP] Cleaning up connection fd=%d", self->fd);
    
    if (self->fd >= 0)
    {
        task_scheduler_dereg_fd(self->fd);
        close(self->fd);
        self->fd = -1;
    }
    
    if (self->parent && self->parent->active_count > 0)
    {
        self->parent->active_count--;
    }
    
    if (self->node.active)
    {
        task_scheduler_remove(&self->node);
    }
    
    self->state = HTTP_CONNECTION_IDLE;
    self->raw_http_buffer_len = 0;
    self->response_len = 0;
    self->sent_bytes = 0;
    
    memset(self->raw_http_buffer, 0, sizeof(self->raw_http_buffer));
    memset(self->response_buffer, 0, sizeof(self->response_buffer));
    memset(&self->parsed_request, 0, sizeof(self->parsed_request));
}

/**
 * FIXED: Better HTTP response formatting
 */
void http_connection_on_handled_request(struct http_connection *self, const char *weather_data)
{
    if (!self || !weather_data)
    {
        LOG_ERROR("[HTTP] Invalid parameters to on_handled_request");
        return;
    }
    
    LOG_INFO("[HTTP] Building response for fd=%d", self->fd);
    
    size_t data_len = strlen(weather_data);
    
    int written = snprintf(self->response_buffer,
                          sizeof(self->response_buffer),
                          "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: %zu\r\n"
                          "Connection: close\r\n"
                          "\r\n"
                          "%s",
                          data_len,
                          weather_data);
    
    if (written < 0 || (size_t)written >= sizeof(self->response_buffer))
    {
        LOG_ERROR("[HTTP] Response buffer overflow");
        http_connection_cleanup(self);
        return;
    }
    
    self->response_len = written;
    self->sent_bytes = 0;
    self->state = HTTP_CONNECTION_SENDING;
}

/**
 * FIXED: Validate HTTP method characters
 */
static int is_valid_http_method(const char *method)
{
    if (!method || strlen(method) == 0) return 0;
    
    /* Valid methods: GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH */
    if (strcmp(method, "GET") == 0 ||
        strcmp(method, "POST") == 0 ||
        strcmp(method, "PUT") == 0 ||
        strcmp(method, "DELETE") == 0 ||
        strcmp(method, "HEAD") == 0 ||
        strcmp(method, "OPTIONS") == 0 ||
        strcmp(method, "PATCH") == 0)
    {
        return 1;
    }
    
    return 0;
}

/**
 * FIXED: Better HTTP parsing with validation
 */
static int parse_http_request(const char *raw, http_connection_request_t *req)
{
    if (!raw || !req) return -1;
    
    memset(req, 0, sizeof(*req));
    
    /* Parse first line: "GET /path?query HTTP/1.1" */
    const char *space1 = strchr(raw, ' ');
    if (!space1)
    {
        LOG_WARN("[HTTP] Malformed request: no space after method");
        return -1;
    }
    
    /* Extract method */
    size_t method_len = space1 - raw;
    if (method_len >= sizeof(req->method) || method_len == 0)
    {
        LOG_WARN("[HTTP] Invalid method length: %zu", method_len);
        return -1;
    }
    
    strncpy(req->method, raw, method_len);
    req->method[method_len] = '\0';
    
    /* Validate method */
    if (!is_valid_http_method(req->method))
    {
        LOG_WARN("[HTTP] Unknown HTTP method: %s", req->method);
        return -1;
    }
    
    /* Find path */
    const char *path_start = space1 + 1;
    const char *space2 = strchr(path_start, ' ');
    if (!space2)
    {
        LOG_WARN("[HTTP] Malformed request: no space after path");
        return -1;
    }
    
    /* Extract HTTP version */
    const char *version_start = space2 + 1;
    const char *line_end = strstr(version_start, "\r\n");
    if (!line_end)
    {
        LOG_WARN("[HTTP] Malformed request: no CRLF after version");
        return -1;
    }
    
    size_t version_len = line_end - version_start;
    if (version_len >= sizeof(req->version))
    {
        version_len = sizeof(req->version) - 1;
    }
    
    strncpy(req->version, version_start, version_len);
    req->version[version_len] = '\0';
    
    /* Validate HTTP version */
    if (strcmp(req->version, "HTTP/1.1") != 0 && 
        strcmp(req->version, "HTTP/1.0") != 0)
    {
        LOG_WARN("[HTTP] Unsupported HTTP version: %s", req->version);
        return -1;
    }
    
    /* Check for query string */
    const char *query_start = strchr(path_start, '?');
    
    if (query_start && query_start < space2)
    {
        /* Has query string */
        size_t path_len = query_start - path_start;
        if (path_len >= sizeof(req->path))
        {
            path_len = sizeof(req->path) - 1;
        }
        
        strncpy(req->path, path_start, path_len);
        req->path[path_len] = '\0';
        
        size_t query_len = space2 - (query_start + 1);
        if (query_len >= sizeof(req->query))
        {
            query_len = sizeof(req->query) - 1;
        }
        
        strncpy(req->query, query_start + 1, query_len);
        req->query[query_len] = '\0';
    }
    else
    {
        /* No query string */
        size_t path_len = space2 - path_start;
        if (path_len >= sizeof(req->path))
        {
            path_len = sizeof(req->path) - 1;
        }
        
        strncpy(req->path, path_start, path_len);
        req->path[path_len] = '\0';
    }
    
    LOG_INFO("[HTTP] Parsed: %s %s %s (query: %s)", 
             req->method, req->path, req->version, 
             req->query[0] ? req->query : "(none)");
    
    return 0;
}

/**
 * FIXED: Check for complete HTTP request
 */
static int is_http_request_complete(const char *buffer)
{
    /* HTTP request is complete when we see \r\n\r\n (end of headers) */
    return strstr(buffer, "\r\n\r\n") != NULL;
}

int8_t http_connection_work(task_node_t *node)
{
    if (!node) return -1;

    http_connection_t *self = container_of(node, http_connection_t, node);
    if (!self || self->fd < 0)
    {
        LOG_ERROR("[HTTP] Invalid connection state");
        return -1;
    }

    switch (self->state)
    {
        case HTTP_CONNECTION_READING:
        {
            /* FIXED: Check buffer space */
            size_t available = sizeof(self->raw_http_buffer) - self->raw_http_buffer_len - 1;
            if (available == 0)
            {
                LOG_ERROR("[HTTP] Buffer full without complete request, fd=%d", self->fd);
                http_connection_cleanup(self);
                return 0;
            }

            ssize_t r = read(self->fd,
                           self->raw_http_buffer + self->raw_http_buffer_len,
                           available);

            if (r > 0)
            {
                self->raw_http_buffer_len += r;
                self->raw_http_buffer[self->raw_http_buffer_len] = '\0';
                
                LOG_DEBUG("[HTTP] Read %ld bytes from fd=%d", r, self->fd);

                /* FIXED: Check if request is complete */
                if (is_http_request_complete(self->raw_http_buffer))
                {
                    self->state = HTTP_CONNECTION_PARSING;
                    LOG_DEBUG("[HTTP] Complete request received");
                }
                else if (self->raw_http_buffer_len >= HTTP_MAX_HEADER_SIZE)
                {
                    LOG_ERROR("[HTTP] Request too large, fd=%d", self->fd);
                    http_connection_cleanup(self);
                }
            }
            else if (r == 0)
            {
                LOG_INFO("[HTTP] Client closed connection fd=%d", self->fd);
                http_connection_cleanup(self);
            }
            else if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                LOG_ERROR("[HTTP] read failed: %s", strerror(errno));
                http_connection_cleanup(self);
            }

            return 0;
        }

        case HTTP_CONNECTION_PARSING:
        {
            if (parse_http_request(self->raw_http_buffer, &self->parsed_request) != 0)
            {
                LOG_WARN("[HTTP] Failed to parse request, sending 400");
                
                const char *bad_request = 
                    "HTTP/1.1 400 Bad Request\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 12\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "Bad Request\n";
                
                strncpy(self->response_buffer, bad_request, sizeof(self->response_buffer) - 1);
                self->response_len = strlen(self->response_buffer);
                self->sent_bytes = 0;
                self->state = HTTP_CONNECTION_SENDING;
            }
            else
            {
                self->state = HTTP_CONNECTION_PROCESSING;
            }
            
            return 0;
        }

        case HTTP_CONNECTION_PROCESSING:
        {
            if (self->parent && self->parent->upper_weather_server_layer)
            {
                weather_connection_t *weather_conn =
                    weather_server_allocate_pool_slot(self->parent->upper_weather_server_layer);

                if (weather_conn)
                {
                    weather_conn->lower_http_connection = self;
                    weather_conn->cb_from_http_layer.http_on_new_request(
                        weather_conn, 
                        &self->parsed_request
                    );

                    self->state = HTTP_CONNECTION_WAITING;
                    return 0;
                }
                else
                {
                    LOG_WARN("[HTTP] Weather pool full");
                    
                    const char *unavailable = 
                        "HTTP/1.1 503 Service Unavailable\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: 21\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "Service Unavailable\n";
                    
                    strncpy(self->response_buffer, unavailable, sizeof(self->response_buffer) - 1);
                    self->response_len = strlen(self->response_buffer);
                    self->sent_bytes = 0;
                    self->state = HTTP_CONNECTION_SENDING;
                    return 0;
                }
            }

            /* No weather layer configured - send hello world */
            const char *hello = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 12\r\n"
                "Connection: close\r\n"
                "\r\n"
                "Hello World\n";
            
            strncpy(self->response_buffer, hello, sizeof(self->response_buffer) - 1);
            self->response_len = strlen(self->response_buffer);
            self->sent_bytes = 0;
            self->state = HTTP_CONNECTION_SENDING;
            return 0;
        }

        case HTTP_CONNECTION_WAITING:
        {
            /* Waiting for weather callback */
            return 0;
        }

        case HTTP_CONNECTION_SENDING:
        {
            size_t remaining = self->response_len - self->sent_bytes;
            
            ssize_t written = write(self->fd,
                                  self->response_buffer + self->sent_bytes,
                                  remaining);

            if (written > 0)
            {
                self->sent_bytes += written;
                LOG_DEBUG("[HTTP] Sent %ld bytes, total %zu/%zu", 
                         written, self->sent_bytes, self->response_len);

                if (self->sent_bytes >= self->response_len)
                {
                    LOG_INFO("[HTTP] Response complete for fd=%d", self->fd);
                    http_connection_cleanup(self);
                }
            }
            else if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
            {
                LOG_ERROR("[HTTP] write failed: %s", strerror(errno));
                http_connection_cleanup(self);
            }

            return 0;
        }

        case HTTP_CONNECTION_ERROR:
        case HTTP_CONNECTION_IDLE:
        case HTTP_CONNECTION_DONE:
        default:
            return 0;
    }

    return 0;
}
