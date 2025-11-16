/**
 * Implementation-file: logging.c
 **/

#include "../../include/logging/logging.h"

log_level_t g_log_level = LOG_LEVEL_INFO;

void logging_init(log_level_t level)
{
    g_log_level = level;
}
