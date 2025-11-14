/**
 * Header-file: tcp_connection.h
 **/

#ifndef __tcp_connection_h__
#define __tcp_connection_h__

#include <stdint.h>
#include <unistd.h>
#include "../../include/task_scheduler/task_scheduler.h"

typedef struct tcp_connection
{
    int fd;         
} tcp_connection_t;

tcp_connection_t *tcp_connection_create(int fd);

void tcp_connection_destroy(tcp_connection_t *c);

int tcp_connection_set_nonblocking(tcp_connection_t *c);

/* Write helper (simple wrapper around write); returns number of bytes written or -1 */
ssize_t tcp_connection_write(tcp_connection_t *c, const void *buf, size_t len);

#endif /* __tcp_connection_h__ */
