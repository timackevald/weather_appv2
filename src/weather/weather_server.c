/**
 * Implementation-file: weather_server.c
 **/

#include "../../include/weather/weather_server.h"
#include "../../include/task_scheduler/task_scheduler.h"
#include <string.h>
#include <stdio.h>

int8_t weather_server_init(weather_server_t *self)
{
    if (!self) return -1;
    memset(self, 0, sizeof(*self));
    
    self->pool_size = WEATHER_POOL_SIZE;
    self->active_count = 0;
    
    for (int i = 0; i < WEATHER_POOL_SIZE; i++)
    {
        self->child_weather_connection[i].state = WEATHER_CONNECTION_IDLE;
        self->child_weather_connection[i].parent = self;
        self->child_weather_connection[i].node.work = weather_connection_work;
        self->child_weather_connection[i].node.active = 0;
        self->child_weather_connection[i].cb_from_http_layer.http_on_new_request = 
            weather_connection_on_request_cb;
    }
    
    printf("[WEATHER SERVER] >> Initialized with pool size %d\n", WEATHER_POOL_SIZE);
    return 0;
}

weather_connection_t *weather_server_allocate_pool_slot(weather_server_t *self)
{
    if (!self) return NULL;
    
    for (int i = 0; i < WEATHER_POOL_SIZE; i++)
    {
        if (self->child_weather_connection[i].state == WEATHER_CONNECTION_IDLE)
        {
            printf("[WEATHER SERVER] >> Allocated pool slot [%d]\n", i);
            return &self->child_weather_connection[i];
        }
    }
    
    printf("[WEATHER SERVER] >> Pool full!\n");
    return NULL;
}
