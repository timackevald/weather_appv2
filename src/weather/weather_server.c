/**
 * Implementation-file: weather_server.c
 **/

 #include "../../include/weather/weather_server.h"
 #include <string.h>
 
 int8_t weather_server_init(weather_server_t *self)
 {
     if (!self) return -1;
     memset(self, 0, sizeof(*self));
     return 0;
 }
 
