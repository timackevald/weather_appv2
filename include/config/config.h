/**
 * Header-file: config.h
 **/
#ifndef __config_h__
#define __config_h__

#include <stdint.h>

/* Pool sizes - all using same size for consistency */
#define CONNECTION_POOL_SIZE 32

/* File descriptor limits */
#define MAX_FD 64

/* Buffer sizes */
#define HTTP_RAW_BUFFER_SIZE 2048
#define HTTP_RESPONSE_BUFFER_SIZE 4096
#define HTTP_METHOD_SIZE 16
#define HTTP_PATH_SIZE 256
#define HTTP_QUERY_SIZE 256
#define HTTP_BODY_SIZE 512

/* Weather buffer sizes */
#define WEATHER_REQUEST_TYPE_SIZE 32
#define WEATHER_CITY_SIZE 64
#define WEATHER_RESPONSE_SIZE 1024

/* TCP settings */
#define LISTEN_BACKLOG 32
#define DEFAULT_PORT "8080"

/* Event watcher settings */
#define EVENT_WATCHER_TIMEOUT_MS 10000

/* HTTP validation */
#define HTTP_MAX_HEADER_SIZE (HTTP_RAW_BUFFER_SIZE - 1)

#endif /* __config_h__ */
