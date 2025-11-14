/**
 * Implementation-file: tcp_connection.c
 **/

#include "../../include/tcp/tcp_connection.h"
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

tcp_connection_t *tcp_connection_create(int fd)
{
    if (fd < 0) return NULL;
    tcp_connection_t *connection = malloc(sizeof(*connection));
    if (!connection) return NULL;
    connection->fd = fd;
    /* caller usually sets non-blocking, but do it here for safety */
    tcp_connection_set_nonblocking(connection);
    return connection;
}

void tcp_connection_destroy(tcp_connection_t *connection)
{
    if (!connection) return;
    if (connection->fd >= 0) close(connection->fd);
    free(connection);
}

int tcp_connection_set_nonblocking(tcp_connection_t *connection)
{
    if (!connection) return -1;
    int flags = fcntl(connection->fd, F_GETFL, 0);
    if (flags == -1) return -1;
    if (fcntl(connection->fd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;
    return 0;
}

ssize_t tcp_connection_write(tcp_connection_t *connection, const void *buf, size_t len)
{
    if (!connection || connection->fd < 0) return -1;
    return write(connection->fd, buf, len);
}
