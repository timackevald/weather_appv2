/**
 * Implementation-file: event_watcher.c
 **/

#include "../../include/event_watcher/event_watcher.h"
#include "../../include/logging/logging.h"
#include <bits/types/struct_timeval.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>

/**
 * Singelton object
 **/
static event_watcher_t g_event_watcher;

int8_t event_watcher_init(void)
{
	
	memset(&g_event_watcher, 0, sizeof(g_event_watcher));
	g_event_watcher.timeout_ms = EVENT_WATCHER_TIMEOUT_MS;

	for (int i = 0; i < MAX_FD; i++)
	{
		g_event_watcher.fds[i] = -1;
	}

	LOG_INFO("[EVENT_WATCHER] >> Init with %d ms timeout\n", EVENT_WATCHER_TIMEOUT_MS);
	return 0;
}

int8_t event_watcher_reg_fd(int fd)
{
	if (fd < 0) return -1;
	if (g_event_watcher.fd_count >= MAX_FD) return -1;

	// Is fd already registered?
	for (int i = 0; i < g_event_watcher.fd_count; i++)
	{
		if (g_event_watcher.fds[i] == fd) return 0;
	}

	/**
	 * No need to check if we're at max, done at entry!
	 **/
	g_event_watcher.fds[g_event_watcher.fd_count++] = fd; // Set and increment
	LOG_DEBUG("[EVENT_WATCHER] >> Registered fd=%d\n", fd);
  
	return 0;
}

int8_t event_watcher_dereg_fd(int fd)
{
	if (fd < 0) return -1;

	for (int i = 0; i < g_event_watcher.fd_count; i++)		
	{
		if (g_event_watcher.fds[i] == fd)
		{
			g_event_watcher.fds[i] = g_event_watcher.fds[g_event_watcher.fd_count - 1]; // i = last element
			g_event_watcher.fds[g_event_watcher.fd_count - 1] = -1; // fd = -1, inactive
			g_event_watcher.fd_count--;
			LOG_DEBUG("[EVENT_WATCHER] >> Unregistered fd=%d\n", fd);
			return 0;
		}
	}

	return -1; 
}

/**
 * Returns 1 if fd ready or at timeout
 **/
int event_watcher_ready(void)
{
	/**
	 * A  structure  type that can represent a set of file descriptors.
	 **/
	fd_set readfds;
	
	/**
	 * This  macro  clears (removes all file descriptors from) set.  It should
	 * be employed as the first step in initializing a file descriptor set.
	 **/
	FD_ZERO(&readfds);

	/**
	 * nfds:
	 * This argument should be set to the highest-numbered file descriptor in any
	 * of the three sets, plus 1.  The indicated file descriptors in each
	 * set are checked, up to this limit (but see BUGS).
	 **/
	int max_fd = -1;
	for (int i = 0; i < g_event_watcher.fd_count; i++) // Loop over registered fds
	{
		int fd = g_event_watcher.fds[i];
		if (fd >= 0)
		{
			FD_SET(fd, &readfds); // This macro adds the file descriptor fd to set
			if (fd > max_fd) max_fd = fd;			
		}
	}
	
	/**
	 * The timeout argument is a timeval structure (shown below)  that
	 * specifies  the  interval  that  select() should block waiting for a file
	 * descriptor to become ready.
	 **/ 
	struct timeval timeout;
	timeout.tv_sec  = 0;
	timeout.tv_usec = EVENT_WATCHER_TIMEOUT_MS; // 10 000 ms

	/**
	 * Will return n ready fds or 0 at timeout and -1 at error
	 **/
	int ready = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
	if (ready < 0 && errno != EINTR)
	{
		LOG_ERROR("[EVENT WATCHER] >> select failed\n %s", strerror(errno));
		return -1;
	}

	if (ready > 0)
	{
		LOG_DEBUG("[EVENT WATCHER] >> FDs ready %d\n", ready);
	}
	
	return ready;
}
