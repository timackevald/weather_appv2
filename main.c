/**
 * Le Main, monsieur! 
 **/
 
#include "include/app/weather_app.h"
#include "include/task_scheduler/task_scheduler.h"
#include "include/logging/logging.h"
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

static volatile int g_running = 1;

static void signal_handler(int signum)
{
    (void)signum;
    g_running = 0;
}

int main(int argc, char *argv[])
{
    wa_t app;
	
	if (argc != 2)
	{
		printf("Usage: %s <log level [1 - 4]\n"
			   "LOG_LEVEL_DEBUG = 0\n"
			   "LOG_LEVEL_INFO  = 1\n"
			   "LOG_LEVEL_WARN  = 2\n"
			   "LOG_LEVEL_ERROR = 3\n", argv[0]);
		return -1;
	}

	char *end;
	int8_t loglvl = (int8_t)strtol(argv[1], &end, 10);
	if (*end != '\0') return -1;

    /* Set up signal handlers for graceful shutdown */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (app_init(&app, loglvl) != 0)
    {
        printf("Application failed to start.\n");
        return 1;
    }

    printf("Weather App Started. Listening on port %s...\n", DEFAULT_PORT);
    printf("Try: curl http://localhost:%s/weather?city=Stockholm\n", DEFAULT_PORT);
    printf("Press Ctrl+C to stop.\n\n");

    while (g_running)
    {
        task_scheduler_run();
    }

    printf("\nShutting down gracefully...\n");
    app_deinit(&app);

    return 0;
}
