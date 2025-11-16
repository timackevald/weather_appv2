/**
 * Header-file: weather_connection.h
 **/

#ifndef __weather_connection_h__
#define __weather_connection_h__

#include <stdint.h>
#include "../../include/task_scheduler/task_scheduler.h"

// Forward declare to avoid circular dependency
typedef struct weather_connection weather_connection_t;
typedef struct weather_server weather_server_t;          
struct http_connection_request;
struct http_connection;

typedef enum
{
    WEATHER_CONNECTION_IDLE       = 0,
    WEATHER_CONNECTION_PROCESSING = 1,
    WEATHER_CONNECTION_DONE       = 2
} weather_connection_state_t;

/* Callback structure for HTTP -> Weather */
typedef struct weather_connection_cb
{
    void (*http_on_new_request)(weather_connection_t *self, const struct http_connection_request *request);
} weather_connection_cb_t;

struct weather_connection
{
    weather_connection_state_t state;
    weather_server_t *parent;
    struct http_connection *lower_http_connection;  // Back reference to HTTP child
    task_node_t node;
    
    // Request info
    char request_type[32];  // "current", "forecast", etc.
    char city[64];
    
    // Response
    char response[512];
    
    // Callback FROM HTTP layer
    weather_connection_cb_t cb_from_http_layer;
};

int8_t weather_connection_work(task_node_t *node);
void weather_connection_on_request_cb(struct weather_connection *self, const struct http_connection_request *request);

#endif
