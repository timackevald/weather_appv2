/**
 * Implementation-file: http_connection.c
 **/

 #include <unistd.h>
 #include <stdio.h>
 #include <string.h>
 #include "../../include/http/http_connection.h"
 #include "../../include/task_scheduler/task_scheduler.h"
 
 int8_t http_connection_work(task_node_t *node)
 {
     http_connection_t *self = container_of(node, http_connection_t, node);
 
     if (!self || self->fd < 0) return -1;
 
     switch (self->state)
     {
         case HTTP_CONNECTION_READING:
         {
             char buf[256];
             ssize_t r = read(self->fd, buf, sizeof(buf));
 
             if (r == 0)
             {
                 printf("[HTTP_CONN] >> Client closed fd=%d\n", self->fd);
                 close(self->fd);
                 self->fd = -1;
                 self->state = HTTP_CONNECTION_IDLE;
                 task_scheduler_remove(&self->node);
                 return 0;
             }
 
             if (r < 0)
             {
                 /* Non-blocking wait */
                 return 0;
             }
 
             printf("[HTTP_CONN] >> Received %ld bytes on fd=%d\n", r, self->fd);
 
             /* Later you’ll parse HTTP request here */
             self->state = HTTP_CONNECTION_SENDING;
             return 0;
         }
 
         case HTTP_CONNECTION_SENDING:
         {
             const char *resp =
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/plain\r\n"
                 "Content-Length: 12\r\n"
                 "\r\n"
                 "Hello World\n";
 
             write(self->fd, resp, strlen(resp));
             close(self->fd);
             self->fd = -1;
 
             printf("[HTTP_CONN] >> Response sent, closing fd.\n");
 
             self->state = HTTP_CONNECTION_IDLE;
             task_scheduler_remove(&self->node);
 
             return 0;
         }
 
         default:
         {
            return 0;
         }
     }
 
     return 0;
 }
 

