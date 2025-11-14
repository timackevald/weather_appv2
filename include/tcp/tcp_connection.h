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
    int fd;                 /* client socket file descriptor */
} tcp_connection_t;

/* Create a tcp_connection for an already-accepted fd.
 * Returns a heap-allocated tcp_connection_t (caller must free/destroy) or NULL.
 * You can replace this with a pool allocator for deterministic allocation.
 */
tcp_connection_t *tcp_connection_create(int fd);

/* Destroy (free) connection wrapper. Also closes the fd if it's open. */
void tcp_connection_destroy(tcp_connection_t *c);

/* Set underlying fd to non-blocking. Returns 0 on success. */
int tcp_connection_set_nonblocking(tcp_connection_t *c);

/* Write helper (simple wrapper around write); returns number of bytes written or -1 */
ssize_t tcp_connection_write(tcp_connection_t *c, const void *buf, size_t len);

#endif /* __tcp_connection_h__ */
