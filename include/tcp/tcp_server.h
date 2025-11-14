/**
 * Header-file. tcp_server.h
 * 
 * This module implements a non-blocking TCP server that accepts
 * incoming connections and forwards each accepted socket to
 * the upper HTTP layer through a callback.
 *
 * It does NOT store or track active connections — it just listens,
 * accepts, and passes the new file descriptor upward.
 */

#ifndef __tcp_server_h__
#define __tcp_server_h__


#include "../../include/task_scheduler/task_scheduler.h"

#define LISTEN_BKLOG 32

/* Forward declaration so we can reference http_server_t in callbacks
   without including http_server.h directly (avoids circular include). */
struct http_server;

typedef enum
{
    TCP_SERVER_INIT      = 0,
    TCP_SERVER_RESOLVING = 1,
    TCP_SERVER_LISTENING = 2,
    TCP_SERVER_ERROR     = 3,
    TCP_SERVER_DONE      = 4,
} tcp_server_state_t;

typedef struct tcp_server_cb
{
    void (*tcp_on_newly_accepted_client)(struct http_server *upper, int fd);
} tcp_server_cb_t;

/**
 * - Holds a single listening socket.
 * - Calls into upper layer (HTTP server) on accept().
 * - Integrated into the scheduler via task_node_t.
 **/
typedef struct tcp_server
{
    int listen_fd;                       
    const char *port;                    
    tcp_server_state_t state;            
    struct http_server *upper_http_layer;
    tcp_server_cb_t cb_to_http_layer;
    task_node_t node;                    
} tcp_server_t;

int8_t tcp_server_init(tcp_server_t *self, const char *port);
int8_t tcp_server_work(task_node_t *node);           
void   tcp_server_close(tcp_server_t *self);

#endif /* __tcp_server_h__ */
