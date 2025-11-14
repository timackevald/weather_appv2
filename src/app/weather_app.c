/**
 * Implementation file: weather_app.c
 **/
 
 #include "../../include/app/weather_app.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 
int8_t app_init(wa_t *self)
{
    if (!self) return -1;

    memset(self, 0, sizeof(*self));

    // 1. INIT scheduler
    if (task_scheduler_init() != 0)
    {
        printf("[APP] >> Failed to init scheduler.\n");
        return -1;
    }

    // 2. INIT weather server (upper layer)
    if (weather_server_init(&self->weather_layer) != 0)
    {
        printf("[APP] >> Failed to init weather server.\n");
        return -1;
    }

    // 3. INIT http server (middle layer)
    if (http_server_init(&self->http_layer, &self->weather_layer) != 0)
    {
        printf("[APP] >> Failed to init http server.\n");
        return -1;
    }

    // 4. INIT tcp server
    if (tcp_server_init(&self->tcp_layer, "8080") != 0)
    {
        printf("[APP] >> Failed to init tcp server.\n");
        return -1;
    }

    // 5. Make TCP → HTTP connection (AFTER tcp_server_init!)
    self->tcp_layer.upper_http_layer = &self->http_layer;
    self->tcp_layer.cb_to_http_layer.tcp_on_newly_accepted_client =
        self->http_layer.cb_from_tcp_layer.tcp_on_newly_accepted_client;

    printf("[APP] >> Initialization done. System ready.\n");
    return 0;
}
 
int8_t app_work(wa_t *self)
{
    (void)self;
    return task_scheduler_work();
}
 
