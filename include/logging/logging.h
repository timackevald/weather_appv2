/**
 * Header-file: logging.h
 */

#ifndef __logging_h__
#define __logging_h__

#include <stdio.h>

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO  = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_ERROR = 3
} log_level_t;

extern log_level_t g_log_level;

/* FIXED: Use GNU extension ##__VA_ARGS__ to handle empty variadic args in C99 */
#define LOG_DEBUG(fmt, ...) \
    do { if (g_log_level <= LOG_LEVEL_DEBUG) \
        printf("[DEBUG] " fmt "\n" , ##__VA_ARGS__); } while(0)

#define LOG_INFO(fmt, ...) \
    do { if (g_log_level <= LOG_LEVEL_INFO) \
        printf("[INFO] " fmt "\n" , ##__VA_ARGS__); } while(0)

#define LOG_WARN(fmt, ...) \
    do { if (g_log_level <= LOG_LEVEL_WARN) \
        printf("[WARN] " fmt "\n" , ##__VA_ARGS__); } while(0)

#define LOG_ERROR(fmt, ...) \
    do { if (g_log_level <= LOG_LEVEL_ERROR) \
        printf("[ERROR] " fmt "\n" , ##__VA_ARGS__); } while(0)

void logging_init(log_level_t level);

#endif /* __logging_h__ */
