/**
 * Implementation-file: tcp_server.c
 **/

//#define _POSIX_C_SOURCE 200112L
//#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "../../include/tcp/tcp_server.h"
#include "../../include/task_scheduler/task_scheduler.h"
#include "../../include/event_watcher/event_watcher.h"
#include "../../include/logging/logging.h"

static int set_nonblocking_fd(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;
    return 0;
}

int8_t tcp_server_init(tcp_server_t *self, const char *port)
{
    if (!self || !port)
    {
        LOG_ERROR("[TCP] Invalid parameters");
        return -1;
    }
    
    memset(self, 0, sizeof(*self));
    
    self->port = port;
    self->state = TCP_SERVER_INIT;
    self->listen_fd = -1;
    
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int ret = getaddrinfo(NULL, port, &hints, &res);
    if (ret != 0)
    {
        LOG_ERROR("[TCP] getaddrinfo failed: %s", gai_strerror(ret));
        self->state = TCP_SERVER_ERROR;
        return -1;
    }

    int listen_fd = -1;
    struct addrinfo *rp;

    for (rp = res; rp != NULL; rp = rp->ai_next)
    {
        listen_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listen_fd == -1) continue;
        
        int opt = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;
        }
        
        close(listen_fd);
        listen_fd = -1;
    }
    
    freeaddrinfo(res);

    if (listen_fd == -1)
    {
        LOG_ERROR("[TCP] bind/listen failed on port %s", port);
        self->state = TCP_SERVER_ERROR;
        return -1;
    }

    if (set_nonblocking_fd(listen_fd) != 0)
    {
        LOG_ERROR("[TCP] fcntl failed: %s", strerror(errno));
        close(listen_fd);
        self->state = TCP_SERVER_ERROR;
        return -1;
    }

    if (listen(listen_fd, LISTEN_BACKLOG) < 0)
    {
        LOG_ERROR("[TCP] listen failed: %s", strerror(errno));
        close(listen_fd);
        self->state = TCP_SERVER_ERROR;
        return -1;
    }

    self->listen_fd = listen_fd;
    self->state = TCP_SERVER_LISTENING;
    self->node.work = tcp_server_work;
    
    task_scheduler_add(&self->node);
    event_watcher_reg_fd(listen_fd);
    
    LOG_INFO("[TCP] Listening on port %s (fd=%d)", port, listen_fd);
    return 0;
}

/**
 * FIXED: Removed duplicate select() call
 * Now relies on scheduler's select() result
 * FIXED: Proper error handling on fcntl failure
 */
int8_t tcp_server_work(task_node_t *node)
{
    if (!node) return -1;

    tcp_server_t *self = container_of(node, tcp_server_t, node);
    if (!self || self->state != TCP_SERVER_LISTENING || self->listen_fd < 0)
    {
        return 0;
    }

    /* Accept all pending connections (non-blocking) */
    while (1)
    {
        struct sockaddr_storage client_addr;
        socklen_t addrlen = sizeof(client_addr);
        
        int client_fd = accept(self->listen_fd, 
                              (struct sockaddr*)&client_addr, 
                              &addrlen);
        
        if (client_fd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break; /* No more clients waiting */
            }
            
            LOG_ERROR("[TCP] accept failed: %s", strerror(errno));
            break;
        }

        /* FIXED: Proper error handling on fcntl failure */
        if (set_nonblocking_fd(client_fd) != 0)
        {
            LOG_ERROR("[TCP] fcntl failed for client fd=%d: %s", 
                     client_fd, strerror(errno));
            close(client_fd);
            continue; /* Try next connection */
        }

        LOG_INFO("[TCP] Accepted client fd=%d", client_fd);

        /* Hand over to HTTP layer */
        if (self->upper_http_layer && 
            self->cb_to_http_layer.tcp_on_newly_accepted_client)
        {
            self->cb_to_http_layer.tcp_on_newly_accepted_client(
                self->upper_http_layer, 
                client_fd
            );
        }
        else
        {
            LOG_WARN("[TCP] No HTTP callback registered, closing fd=%d", client_fd);
            close(client_fd);
        }
    }

    return 0;
}

void tcp_server_close(tcp_server_t *self)
{
    if (!self) return;
    
    if (self->listen_fd >= 0)
    {
        event_watcher_dereg_fd(self->listen_fd);
        close(self->listen_fd);
        LOG_INFO("[TCP] Closed listen socket fd=%d", self->listen_fd);
    }
    
    self->listen_fd = -1;
    self->state = TCP_SERVER_DONE;
}
