
#include <grass/config.h>

#ifdef HAVE_SOCKET

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>

#include <grass/gis.h>
#include "driverlib.h"

int prepare_connection_sock(const char *me)
{
    const char *connpath;
    int fd;

    connpath = G_sock_get_fname(me);
    if (!connpath)
	G_fatal_error("Couldn't get socket path");

    /* Now we must check and see whether or not someone */
    /* (possibly another invocation of ourself) is using our socket.    */

    if (G_sock_exists(connpath)) {
	if ((fd = G_sock_connect(connpath)) >= 0) {
	    close(fd);
	    G_warning("Graphics driver [%s] is already running", me);
	    G_fatal_error("Unable to start monitor <%s>", me);
	}

	if (unlink(connpath) < 0) {
	    G_warning("Failed to remove stale socket file: %s", connpath);
	    G_fatal_error("Unable to start monitor <%s>", me);
	}
    }

    /* We are free to run now.  No one is using our socket.                   */
    if ((fd = G_sock_bind(connpath)) < 0) {
	G_fatal_error("Can't bind to socket: error \"%s\"\n",
		      strerror(errno));
    }

    /* Now set up listen */
    if (G_sock_listen(fd, 1) != 0) {
	G_fatal_error("G_sock_listen: error \"%s\"\n", strerror(errno));
    }

    return fd;
}

int get_connection_sock(int listenfd, int *rfd, int *wfd, int other_fd)
{
    int fd;

#ifndef __MINGW32__
    if (other_fd >= 0) {
	fd_set waitset;

	FD_ZERO(&waitset);
	FD_SET(listenfd, &waitset);
	FD_SET(other_fd, &waitset);
	if (select(FD_SETSIZE, &waitset, NULL, NULL, NULL) < 0) {
	    perror("get_connection_sock: select");
	    exit(EXIT_FAILURE);
	}

	if (!FD_ISSET(listenfd, &waitset))
	    return -1;
    }
#endif

    /* G_sock_accept will block until a connection is requested */
    fd = G_sock_accept(listenfd);
    if (fd >= 0) {
	*rfd = fd;
	*wfd = dup(fd);
	return 0;
    }

    if (errno == EINTR)
	return -1;

    G_warning("G_sock_accept: error \"%s\"", strerror(errno));
    COM_Graph_close();
    exit(EXIT_FAILURE);
}

#endif /* HAVE_SOCKET */
