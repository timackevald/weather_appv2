/**
 * Header file: weather_app.h
 **/

#ifndef __weather_app_h__
#define __weather_app_h__

#include "../../include/task_scheduler/task_scheduler.h"
#include "../../include/tcp/tcp_server.h"
#include "../../include/http/http_server.h"
#include "../../include/weather/weather_server.h"

typedef struct wa
{
    tcp_server_t   tcp_layer;
    http_server_t  http_layer;
    weather_server_t weather_layer;
} wa_t;

int8_t app_init(wa_t *self);
int8_t app_work(wa_t *self);

#endif /* __weather_app_h__ */
