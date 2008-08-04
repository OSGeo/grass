
/****************************************************************************
 *
 * MODULE:       dbmi_base
 * AUTHOR(S):    CERL (Joel Jones + possible other original contributors)
 *               Radim Blazek <radim.blazek gmail.com>,
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Markus Neteler <neteler itc.it>
 * PURPOSE:      database management functions for modules and drivers
 * COPYRIGHT:    (C) 2003-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include "xdr.h"

#ifdef __MINGW32__
#define USE_STDIO 0
#define USE_READN 1
#else
#define USE_STDIO 1
#define USE_READN 0
#endif

#ifndef USE_STDIO
#include <unistd.h>
#endif

static FILE *_send, *_recv;

#if USE_READN

static ssize_t readn(int fd, void *buf, size_t count)
{
    ssize_t total = 0;

    while (total < count) {
	ssize_t n = read(fd, (char *)buf + total, count - total);

	if (n < 0)
	    return n;
	if (n == 0)
	    break;
	total += n;
    }

    return total;
}

static ssize_t writen(int fd, const void *buf, size_t count)
{
    ssize_t total = 0;

    while (total < count) {
	ssize_t n = write(fd, (const char *)buf + total, count - total);

	if (n < 0)
	    return n;
	if (n == 0)
	    break;
	total += n;
    }

    return total;
}

#endif

void db__set_protocol_fds(FILE * send, FILE * recv)
{
    _send = send;
    _recv = recv;
}

int db__send(const void *buf, size_t size)
{
#if USE_STDIO
    return fwrite(buf, 1, size, _send) == size;
#elif USE_READN
    return writen(fileno(_send), buf, size) == size;
#else
    return write(fileno(_send), buf, size) == size;
#endif
}

int db__recv(void *buf, size_t size)
{
#if USE_STDIO
#ifdef USE_BUFFERED_IO
    fflush(_send);
#endif
    return fread(buf, 1, size, _recv) == size;
#elif USE_READN
    return readn(fileno(_recv), buf, size) == size;
#else
    return read(fileno(_recv), buf, size) == size;
#endif
}
