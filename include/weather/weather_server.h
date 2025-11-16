/**
 * Header-file: weather_server.h
 **/

#ifndef __weather_server_h__
#define __weather_server_h__

#include <stdint.h>
#include "../../include/weather/weather_connection.h"


#define WEATHER_POOL_SIZE 32

typedef struct weather_server
{
    uint8_t pool_size;
    uint8_t active_count;
    
    weather_connection_t child_weather_connection[WEATHER_POOL_SIZE];
} weather_server_t;

int8_t weather_server_init(weather_server_t *self);
weather_connection_t *weather_server_allocate_pool_slot(weather_server_t *self);

#endif
