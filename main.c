/**
 * Le Main, monsieur! 
 **/
 
#include "include/app/weather_app.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    wa_t app;

    if (app_init(&app) != 0)
    {
        printf("Application failed to start.\n");
        return 1;
    }

    printf("Weather App Started. Listening on port 8080...\n");

    while (1)
    {
        app_work(&app);
        usleep(10000);
    }

    return 0;
}
