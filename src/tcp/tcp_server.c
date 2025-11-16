/**
 * Implementation-file: tcp_server.c
 **/

#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE

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


static int set_nonblocking_fd(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;

    return 0;
}

int8_t tcp_server_init(tcp_server_t *self, const char *port)
{
    if (!self || !port) return -1;
    memset(self, 0, sizeof(*self));
    
    self->port  = port;
    self->state = TCP_SERVER_INIT; /* Start the state-machine */
    
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; /* prefer AF_INET/AF_INET6 but allow both */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int ret = getaddrinfo(NULL, port, &hints, &res);
    if (ret != 0) {
        printf("[TCP] >> getaddrinfo failed: %s\n", gai_strerror(ret));
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
        if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(listen_fd);
        listen_fd = -1;
    }
    freeaddrinfo(res);

    if (listen_fd == -1)
    {
        printf("[TCP] >> bind/listen failed on port %s\n", port);
        self->state = TCP_SERVER_ERROR;

        return -1;
    }

    if (set_nonblocking_fd(listen_fd) != 0)
    {
        perror("[TCP] >> fcntl");
        close(listen_fd);
        self->state = TCP_SERVER_ERROR;
        
        return -1;
    }

    if (listen(listen_fd, LISTEN_BKLOG) < 0)
    {
        perror("[TCP] >> listen");
        close(listen_fd);
        self->state = TCP_SERVER_ERROR;

        return -1;
    }

    self->listen_fd = listen_fd;
    self->state = TCP_SERVER_LISTENING;
    self->node.work = tcp_server_work;
    //self->node.active = 1; Le Problem
    task_scheduler_add(&self->node);
	task_scheduler_reg_fd(listen_fd);
	
    printf("[TCP] >> Listening on port %s\n", port);

    return 0;
}

int8_t tcp_server_work(task_node_t *node)
{
    if (!node) return -1;

    tcp_server_t *self = container_of(node, tcp_server_t, node);
    if (!self || self->state != TCP_SERVER_LISTENING) return 0;

    //fd_set readfds;
    //FD_ZERO(&readfds);
    //FD_SET(self->listen_fd, &readfds);
    //int max_fd = self->listen_fd;

    // Use select with 0 timeout to poll
    //struct timeval timeout = {0, 0};
	/**
	 * ALREADY CHECKING select() IN SCHEDULER
	 * SHOULD JUST BE ABLE TO ACCEPT
	 **/
	//int ready = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
    /*if (ready < 0)
    {
        if (errno != EINTR)
            perror("[TCP] select");
        return 0;
    }

    if (!FD_ISSET(self->listen_fd, &readfds))
        return 0; // No pending connections
	*/

    // Accept all pending connections (non-blocking)
    while (1)
    {
        struct sockaddr_storage client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_fd = accept(self->listen_fd, (struct sockaddr*)&client_addr, &addrlen);
        if (client_fd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break; // No more clients waiting
            perror("[TCP] accept");
            break;
        }

        // Make client socket non-blocking
        if (fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK) < 0)
            perror("[TCP] fcntl");

        printf("[TCP] Accepted client fd=%d\n", client_fd);

        // Hand over to HTTP
        if (self->upper_http_layer && self->cb_to_http_layer.tcp_on_newly_accepted_client)
        {
            self->cb_to_http_layer.tcp_on_newly_accepted_client(self->upper_http_layer, client_fd);
        }
        else
        {
            printf("[TCP] No HTTP callback, closing fd=%d\n", client_fd);
            close(client_fd);
        }
    }

    return 0;
}

void tcp_server_close(tcp_server_t *self)
{
    if (!self) return;
    if (self->listen_fd >= 0) close(self->listen_fd);
    self->listen_fd = -1;
    self->state = TCP_SERVER_ERROR;
}
