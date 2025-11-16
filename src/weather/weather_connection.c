/**
 * Implementation-file: weather_connection.c
 **/

#include "../../include/weather/weather_connection.h"
#include "../../include/http/http_connection.h"
#include "../../include/task_scheduler/task_scheduler.h"
#include <stdio.h>
#include <string.h>

/**
 * THIS IS THE CALLBACK FUNCTION - HTTP CHILD HANDS PARSED REQUEST TO WEATHER CHILD
 **/
void weather_connection_on_request_cb(struct weather_connection *self, const struct http_connection_request *request)
{
    if (!self || !request) return;
    
    printf("[WEATHER_CONN_CB] >> Received request from HTTP child\n");
    printf("[WEATHER_CONN_CB] >> Method: %s, Path: %s, Query: %s\n", 
           request->method, request->path, request->query);
    
    self->state = WEATHER_CONNECTION_PROCESSING;
    
    // Route based on path using strcmp
    if (strcmp(request->path, "/weather") == 0)
    {
        strncpy(self->request_type, "current", sizeof(self->request_type) - 1);
        printf("[WEATHER_CONN_CB] >> Request type: current weather\n");
    }
    else if (strcmp(request->path, "/forecast") == 0)
    {
        strncpy(self->request_type, "forecast", sizeof(self->request_type) - 1);
        printf("[WEATHER_CONN_CB] >> Request type: forecast\n");
    }
    else if (strcmp(request->path, "/") == 0)
    {
        strncpy(self->request_type, "default", sizeof(self->request_type) - 1);
        printf("[WEATHER_CONN_CB] >> Request type: default\n");
    }
    else
    {
        strncpy(self->request_type, "unknown", sizeof(self->request_type) - 1);
        printf("[WEATHER_CONN_CB] >> Request type: unknown\n");
    }
    
    // Parse city from query (simple parsing: city=Stockholm)
    const char *city_param = strstr(request->query, "city=");
    if (city_param)
    {
        strncpy(self->city, city_param + 5, sizeof(self->city) - 1);
        self->city[sizeof(self->city) - 1] = '\0';
        
        // Remove any trailing & or other params
        char *end = strchr(self->city, '&');
        if (end) *end = '\0';
    }
    else
    {
        strncpy(self->city, "Stockholm", sizeof(self->city) - 1); // Default
    }
    
    self->node.active = 0;
    task_scheduler_add(&self->node);
    //self->parent->active_count++; FIX THIS LATER!
    
    printf("[WEATHER_CONN_CB] >> Weather child processing for city: %s\n", self->city);
}

int8_t weather_connection_work(task_node_t *node)
{
    weather_connection_t *self = container_of(node, weather_connection_t, node);
    
    printf("[WEATHER_CONN_WORK] >> Called, state=%d\n", self->state);
    
    if (!self) return -1;
    
    switch (self->state)
    {
        case WEATHER_CONNECTION_PROCESSING:
        {
            printf("[WEATHER_CONN] >> Processing weather request\n");
            
            // Route to backend state machines based on request type
            // TODO: This is where you'd call actual backend APIs
            if (strcmp(self->request_type, "current") == 0)
            {
                // TODO: Spawn backend API client child for current weather
                snprintf(self->response, sizeof(self->response),
                    "Current weather in %s: Sunny, 20°C\n", self->city);
            }
            else if (strcmp(self->request_type, "forecast") == 0)
            {
                // TODO: Spawn backend API client child for forecast
                snprintf(self->response, sizeof(self->response),
                    "5-day forecast for %s: Mostly sunny, 18-22°C\n", self->city);
            }
            else if (strcmp(self->request_type, "default") == 0)
            {
                snprintf(self->response, sizeof(self->response),
                    "Weather API\n\nAvailable endpoints:\n"
                    "  /weather?city=NAME - Current weather\n"
                    "  /forecast?city=NAME - 5-day forecast\n");
            }
            else
            {
                snprintf(self->response, sizeof(self->response),
                    "404 Not Found\n\nUnknown endpoint: %s\n", self->request_type);
            }
            
            printf("[WEATHER_CONN] >> Generated response: %s", self->response);
            
            /**
             * CALLBACK FUNCTION HERE! GIVING BACK CONTROL TO HTTP CHILD!
             **/
            if (self->lower_http_connection && 
                self->lower_http_connection->cb_from_weather_layer.weather_on_handled_request)
            {
                printf("[WEATHER_CONN] >> Calling HTTP child response callback\n");
                self->lower_http_connection->cb_from_weather_layer.weather_on_handled_request(
                    self->lower_http_connection,
                    self->response
                );
            }
            
            self->state = WEATHER_CONNECTION_DONE;
            return 0;
        }
        
        case WEATHER_CONNECTION_DONE:
        {
            printf("[WEATHER_CONN] >> Processing done, returning to pool\n");
            
            self->state = WEATHER_CONNECTION_IDLE;
            self->lower_http_connection = NULL;
            memset(self->request_type, 0, sizeof(self->request_type));
            memset(self->city, 0, sizeof(self->city));
            memset(self->response, 0, sizeof(self->response));
            
            task_scheduler_remove(&self->node);
            // self->parent->active_count--; FIX THIS LATER
            
            return 0;
        }
        
        default:
            return 0;
    }
    
    return 0;
}
