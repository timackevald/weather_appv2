/**
 * Header-file: weather_server.h
 **/

#ifndef __weather_server_h__
#define __weather_server_h__

#include <stdint.h>

typedef struct weather_server
{
    int dummy;
} weather_server_t;

int8_t weather_server_init(weather_server_t *self);

#endif
