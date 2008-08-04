/*
 ****************************************************************************
 *
 * LIBRARY:      unix_socks.c  -- Routines related to using UNIX domain 
 *               sockets for IPC mechanisms (such as XDRIVER).
 *
 * AUTHOR(S):    Eric G. Miller
 *
 * PURPOSE:      Historically GRASS has used FIFO for interprocess communic-
 *               ations for display functions.  Unfortunately, FIFO's are
 *               not available on all target platforms.  An attempt has been
 *               made to use IPC message passing, but the semantics are
 *               variable and it also isn't available on all target platforms.
 *               UNIX sockets, or local or domain sockets, are much more
 *               widely available and consistent.  NOTE: This implementation
 *               of UNIX sockets provides zero security checking so should
 *               not be used from untrusted clients.
 *
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/

#ifndef __MINGW32__		/* TODO */
#ifdef __MINGW32__

#include <grass/gis.h>
#include <windows.h>
#include <io.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/stat.h>

#define PIPE_TIMEOUT 5000
#define BUFSIZE 2048

/* ---------------------------------------------------------------------
 * _get_make_pipe_path(), builds and tests the path for the socket
 * directory.  Returns NULL on any failure, otherwise it returns the
 * directory path. The path will be like "/tmp/grass-$USER".
 * ---------------------------------------------------------------------*/
static char *_get_make_pipe_path(void)
{
    char *path, *user;
    const char *prefix = "c:/grass-";
    char *whoami = "mingw-anon-user";
    int len, status;
    struct _stat theStat;

    user = G_whoami();		/* Don't G_free () return value ever! */
    if (user == NULL) {
	user = whoami;
    }
    len = strlen(prefix) + strlen(user) + 1;
    path = G_malloc(len);
    sprintf(path, "%s%s", prefix, user);

    if ((status = G_lstat(path, &theStat)) != 0) {
	status = G_mkdir(path);
    }
    else {
	if (!S_ISDIR(theStat.st_mode)) {
	    status = -1;	/* not a directory ?? */
	}
	else {
	    status = chmod(path, S_IRWXU);	/* fails if we don't own it */
	}
    }

    if (status) {		/* something's wrong if non-zero */
	G_free(path);
	path = NULL;
    }

    return path;
}


 /* ----------------------------------------------------------------------
  * G_pipe_get_fname(), builds the full path for a UNIX socket.  Caller 
  * should G_free () the return value when it is no longer needed.  Returns
  * NULL on failure.
  * ---------------------------------------------------------------------*/
char *G_pipe_get_fname(char *name)
{
    char *path, *dirpath;
    int len;

    if (name == NULL)
	return NULL;

    dirpath = _get_make_pipe_path();

    if (dirpath == NULL)
	return NULL;

    len = strlen(dirpath) + strlen(name) + 2;
    path = G_malloc(len);
    sprintf(path, "%s/%s", dirpath, name);
    G_free(dirpath);

    return path;
}


/* -------------------------------------------------------------------
 * G_pipe_exists(char *): Returns 1 if path is to a UNIX socket that
 * already exists, 0 otherwise.
 * -------------------------------------------------------------------*/

int G_pipe_exists(char *name)
{
    int rv = 0;
    HANDLE hFile = hFile = CreateFile(name,
				      GENERIC_READ,
				      FILE_SHARE_READ,
				      NULL,
				      OPEN_EXISTING,
				      FILE_ATTRIBUTE_NORMAL,
				      NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
	if (name == NULL || (FILE_TYPE_PIPE != GetFileType(hFile))) {
	    rv = 0;
	}
	else {
	    rv = 1;
	    CloseFile(hFile);
	}
    }
    return (rv);
}


/* -----------------------------------------------------------------
 * G_pipe_bind (char *): Takes the full pathname for a UNIX socket
 * and returns the file descriptor to the socket after a successful
 * call to bind().  On error, it returns -1.  Check "errno" if you
 * want to find out why this failed (clear it before the call).
 * ----------------------------------------------------------------*/

HANDLE G_pipe_bind(char *name)
{
    HANDLE hPipe;

    if (name == NULL) {
	return -1;
    }
    if (G_pipe_exists(name)) {
	/*errno = EADDRINUSE; */
	return -1;
    }

    hPipe = CreateNamedPipe(name,	// pipe name 
			    PIPE_ACCESS_DUPLEX,	// read/write access 
			    PIPE_TYPE_MESSAGE |	// message type pipe 
			    PIPE_READMODE_MESSAGE |	// message-read mode 
			    PIPE_WAIT,	// blocking mode 
			    PIPE_UNLIMITED_INSTANCES,	// max. instances  
			    BUFSIZE,	// output buffer size 
			    BUFSIZE,	// input buffer size 
			    PIPE_TIMEOUT,	// client time-out 
			    NULL);	// no security attribute 

    if (hPipe == INVALID_HANDLE_VALUE) {
	return (-1);
    }
    return (hPipe);
}


/* ---------------------------------------------------------------------
 * G_pipe_listen(int, unsigned int): Wrapper around the listen() 
 * function.
 * --------------------------------------------------------------------*/

int G_pipe_listen(HANDLE hPipe, unsigned int queue_len)
{
    return (0);
}

/* -----------------------------------------------------------------------
 * G_pipe_accept (int sockfd):
 * Wrapper around the accept() function. No client info is returned, but
 * that's not generally useful for local sockets anyway.  Function returns
 * the file descriptor or an error code generated by accept().  Note,
 * this call will usually block until a connection arrives.  You can use
 * select() for a time out on the call.
 * ---------------------------------------------------------------------*/

HANDLE G_pipe_accept(HANDLE hPipe)
{
    BOOL fConnected;
    HANDLE rv = hPipe;

    fConnected = ConnectNamedPipe(hPipe, NULL) ?
	TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (fConnected) {
	rv = NULL;
    }
    return (rv);
}


/* ----------------------------------------------------------------------
 * G_pipe_connect (char *name):  Tries to connect to the unix socket
 * specified by "name".  Returns the file descriptor if successful, or
 * -1 if unsuccessful.  Global errno is set by connect() if return is -1
 * (though you should zero errno first, since this function doesn't set
 * it for a couple conditions).
 * --------------------------------------------------------------------*/

HANDLE G_pipe_connect(char *name)
{
    HANDLE hPipe = -1;

    if (!G_pipe_exists(name)) {
	return hPipe;
    }

    while (1) {
	hPipe = CreateFile(name,	// pipe name 
			   GENERIC_READ |	// read and write access 
			   GENERIC_WRITE, 0,	// no sharing 
			   NULL,	// no security attributes
			   OPEN_EXISTING,	// opens existing pipe 
			   0,	// default attributes 
			   NULL);	// no template file 

	if (hPipe != INVALID_HANDLE_VALUE) {
	    break;
	}
	if (GetLastError() != ERROR_PIPE_BUSY) {
	    return (-1);
	}
	/* Wait for 5 seconds */
	if (!WaitNamedPipe(name, PIPE_TIMEOUT)) {
	    return (-1);
	}
    }
    return (hPipe);
}

#endif /* __MINGW32__ */
#endif /* __MINGW32__ */
