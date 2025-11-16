/**
 * Implementation-file: weather_connection.c
 **/

#include "../../include/weather/weather_server.h"
#include "../../include/weather/weather_connection.h"
#include "../../include/http/http_connection.h"
#include "../../include/task_scheduler/task_scheduler.h"
#include "../../include/logging/logging.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/**
 * FIXED: Better input validation for city parameter
 */
static void sanitize_city_name(char *city, size_t max_len)
{
    if (!city) return;
    
    size_t len = strlen(city);
    for (size_t i = 0; i < len && i < max_len - 1; i++)
    {
        /* Only allow alphanumeric, space, dash, underscore */
        if (!isalnum(city[i]) && city[i] != ' ' && 
            city[i] != '-' && city[i] != '_')
        {
            city[i] = '_';
        }
    }
}

void weather_connection_on_request_cb(struct weather_connection *self, 
                                     const struct http_connection_request *request)
{
    if (!self || !request)
    {
        LOG_ERROR("[WEATHER CONN CB] Invalid parameters");
        return;
    }
    
    LOG_INFO("[WEATHER CONN CB] Received request: %s %s", 
             request->method, request->path);
    
    self->state = WEATHER_CONNECTION_PROCESSING;
    
    /* Route based on path */
    if (strcmp(request->path, "/weather") == 0)
    {
        strncpy(self->request_type, "current", sizeof(self->request_type) - 1);
    }
    else if (strcmp(request->path, "/forecast") == 0)
    {
        strncpy(self->request_type, "forecast", sizeof(self->request_type) - 1);
    }
    else if (strcmp(request->path, "/") == 0)
    {
        strncpy(self->request_type, "default", sizeof(self->request_type) - 1);
    }
    else
    {
        strncpy(self->request_type, "unknown", sizeof(self->request_type) - 1);
    }
    
    /* Parse city from query */
    const char *city_param = strstr(request->query, "city=");
    if (city_param)
    {
        const char *city_value = city_param + 5;
        size_t copy_len = 0;
        
        /* Find end of city parameter (& or end of string) */
        while (city_value[copy_len] && city_value[copy_len] != '&' && 
               copy_len < sizeof(self->city) - 1)
        {
            copy_len++;
        }
        
        strncpy(self->city, city_value, copy_len);
        self->city[copy_len] = '\0';
        
        /* FIXED: Sanitize user input */
        sanitize_city_name(self->city, sizeof(self->city));
    }
    else
    {
        strncpy(self->city, "Stockholm", sizeof(self->city) - 1);
    }
    
    self->city[sizeof(self->city) - 1] = '\0';
    
    task_scheduler_add(&self->node);
    
    if (self->parent)
    {
        self->parent->active_count++;
    }
    
    LOG_INFO("[WEATHER CONN CB] Processing for city: %s, type: %s", 
             self->city, self->request_type);
}

int8_t weather_connection_work(task_node_t *node)
{
    if (!node) return -1;
    
    weather_connection_t *self = container_of(node, weather_connection_t, node);
    if (!self)
    {
        LOG_ERROR("[WEATHER CONN] Invalid node");
        return -1;
    }
    
    switch (self->state)
    {
        case WEATHER_CONNECTION_PROCESSING:
        {
            LOG_DEBUG("[WEATHER CONN] Processing weather request");
            
            /* Route to backend based on request type */
            if (strcmp(self->request_type, "current") == 0)
            {
                /* TODO: Call actual weather API */
                snprintf(self->response, sizeof(self->response),
                    "Current weather in %s:\n"
                    "  Condition: Sunny\n"
                    "  Temperature: 20°C\n"
                    "  Humidity: 65%%\n"
                    "  Wind: 10 km/h\n",
                    self->city);
            }
            else if (strcmp(self->request_type, "forecast") == 0)
            {
                /* TODO: Call actual forecast API */
                snprintf(self->response, sizeof(self->response),
                    "5-day forecast for %s:\n"
                    "  Mon: Sunny, 18-22°C\n"
                    "  Tue: Cloudy, 16-20°C\n"
                    "  Wed: Rainy, 14-18°C\n"
                    "  Thu: Sunny, 17-21°C\n"
                    "  Fri: Partly Cloudy, 19-23°C\n",
                    self->city);
            }
            else if (strcmp(self->request_type, "default") == 0)
            {
                snprintf(self->response, sizeof(self->response),
                    "Weather API - Available Endpoints\n"
                    "==================================\n\n"
                    "GET /weather?city=NAME\n"
                    "  Get current weather for a city\n\n"
                    "GET /forecast?city=NAME\n"
                    "  Get 5-day forecast for a city\n\n"
                    "Example:\n"
                    "  curl http://localhost:8080/weather?city=Stockholm\n");
            }
            else
            {
                snprintf(self->response, sizeof(self->response),
                    "404 Not Found\n\n"
                    "Unknown endpoint: %s\n\n"
                    "Try:\n"
                    "  /weather?city=NAME\n"
                    "  /forecast?city=NAME\n",
                    self->request_type);
            }
            
            LOG_DEBUG("[WEATHER CONN] Generated response (%zu bytes)", 
                     strlen(self->response));
            
            /* FIXED: Save callback pointer before using it */
            http_connection_t *http_conn = self->lower_http_connection;
            if (http_conn && http_conn->cb_from_weather_layer.weather_on_handled_request)
            {
                LOG_DEBUG("[WEATHER CONN] Calling HTTP callback");
                http_conn->cb_from_weather_layer.weather_on_handled_request(
                    http_conn,
                    self->response
                );
            }
            else
            {
                LOG_ERROR("[WEATHER CONN] No HTTP callback available");
            }
            
            self->state = WEATHER_CONNECTION_DONE;
            return 0;
        }
        
        case WEATHER_CONNECTION_DONE:
        {
            LOG_DEBUG("[WEATHER CONN] Returning to pool");
            
            self->state = WEATHER_CONNECTION_IDLE;
            self->lower_http_connection = NULL;
            
            memset(self->request_type, 0, sizeof(self->request_type));
            memset(self->city, 0, sizeof(self->city));
            memset(self->response, 0, sizeof(self->response));
            
            if (self->node.active)
            {
                task_scheduler_remove(&self->node);
            }

            if (self->parent && self->parent->active_count > 0)
            {
                self->parent->active_count--;
            }
            
            return 0;
        }
        
        case WEATHER_CONNECTION_ERROR:
        case WEATHER_CONNECTION_IDLE:
        default:
            return 0;
    }
    
    return 0;
}
