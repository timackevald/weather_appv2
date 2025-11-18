/**
 * Implementation file: weather_app.c
 **/
 
#include "../../include/app/weather_app.h"
#include "../../include/logging/logging.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

int8_t app_init(wa_t *self, int8_t loglvl)
{
    if (!self)
    {
        LOG_ERROR("[APP] >> Cannot init NULL app");
        return -1;
    }

    memset(self, 0, sizeof(*self));

    logging_init(loglvl);
    LOG_INFO("[APP] >> Starting initialization...");

    if (task_scheduler_init() != 0)
    {
        LOG_ERROR("[APP] >> Failed to init scheduler");
        return -1;
    }

	if (event_watcher_init() != 0)
	{
		LOG_ERROR("[APP] >> Failed to init event watcher");
		return -1;
	}	

    if (weather_server_init(&self->weather_layer) != 0)
    {
        LOG_ERROR("[APP] >> Failed to init weather server");
        return -1;
    }

    if (http_server_init(&self->http_layer, &self->weather_layer) != 0)
    {
        LOG_ERROR("[APP] >> Failed to init HTTP server");
        return -1;
    }

    if (tcp_server_init(&self->tcp_layer, DEFAULT_PORT) != 0)
    {
        LOG_ERROR("[APP] >> Failed to init TCP server");
        return -1;
    }

    self->tcp_layer.upper_http_layer = &self->http_layer;
    self->tcp_layer.cb_to_http_layer.tcp_on_newly_accepted_client =
        self->http_layer.cb_from_tcp_layer.tcp_on_newly_accepted_client;

    LOG_INFO("[APP] >> Initialization complete");
    return 0;
}

int8_t app_work(wa_t *self)
{
    (void)self;
    return task_scheduler_work();
}

int8_t app_deinit(wa_t *self)
{
    if (!self)
    {
        LOG_ERROR("[APP] >> Cannot deinit NULL app");
        return -1;
    }

    LOG_INFO("[APP] >> Shutting down...");
    tcp_server_close(&self->tcp_layer);
    task_scheduler_deinit();
    LOG_INFO("[APP] >> Shutdown complete");
    return 0;
}
