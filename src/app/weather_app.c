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
        LOG_ERROR("[APP] Cannot init NULL app");
        return -1;
    }

    memset(self, 0, sizeof(*self));

    /* Initialize logging */
    logging_init(loglvl); /* Change to LOG_LEVEL_DEBUG for verbose output */

    LOG_INFO("[APP] Starting initialization...");

    /* 1. Initialize scheduler */
    if (task_scheduler_init() != 0)
    {
        LOG_ERROR("[APP] Failed to init scheduler");
        return -1;
    }

    /* 2. Initialize weather server (top layer) */
    if (weather_server_init(&self->weather_layer) != 0)
    {
        LOG_ERROR("[APP] Failed to init weather server");
        return -1;
    }

    /* 3. Initialize HTTP server (middle layer) */
    if (http_server_init(&self->http_layer, &self->weather_layer) != 0)
    {
        LOG_ERROR("[APP] Failed to init HTTP server");
        return -1;
    }

    /* 4. Initialize TCP server (bottom layer) */
    if (tcp_server_init(&self->tcp_layer, DEFAULT_PORT) != 0)
    {
        LOG_ERROR("[APP] Failed to init TCP server");
        return -1;
    }

    /* 5. Wire up TCP → HTTP connection */
    self->tcp_layer.upper_http_layer = &self->http_layer;
    self->tcp_layer.cb_to_http_layer.tcp_on_newly_accepted_client =
        self->http_layer.cb_from_tcp_layer.tcp_on_newly_accepted_client;

    LOG_INFO("[APP] Initialization complete");
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
        LOG_ERROR("[APP] Cannot deinit NULL app");
        return -1;
    }

    LOG_INFO("[APP] Shutting down...");

    /* Close TCP server */
    tcp_server_close(&self->tcp_layer);

    /* Deinitialize scheduler */
    task_scheduler_deinit();

    LOG_INFO("[APP] Shutdown complete");
    return 0;
}
