/**
 * \file unix_sockets.c
 *
 * \brief Unix sockets support functions.
 *
 * Routines related to using UNIX domain sockets for IPC mechanisms 
 * (such as XDRIVER).<br>
 *
 * Historically GRASS has used FIFO for interprocess communications for 
 * display functions. Unfortunately, FIFO's are not available on all 
 * target platforms. An attempt has been made to use IPC message 
 * passing, but the semantics are variable and it also isn't available 
 * on all target platforms. UNIX sockets, or local or domain sockets, 
 * are much more widely available and consistent.<br>
 *
 * <b>Note:</b> This implementation of UNIX sockets provides zero 
 * security checking so should not be used from untrusted clients.<br>
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Eric G. Miller
 *
 * \date 1999-2006
 */

#include <grass/config.h>

#ifdef HAVE_SOCKET

#include <grass/gis.h>
#include <grass/version.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __MINGW32__
#define USE_TCP
#include <winsock2.h>
#include <ws2tcpip.h>
#define EADDRINUSE WSAEADDRINUSE
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#define INVALID_SOCKET (-1)
#endif

/** For systems where the *_LOCAL (POSIX 1g) is not defined 
 ** There's not really any difference between PF and AF in practice.
 **/

static char *_get_make_sock_path (void);

static void init_sockets(void)
{
#ifdef __MINGW32__
    static int ready;
    WSADATA wsadata;

    if (ready)
	return;

    ready = 1;

    WSAStartup(0x0001, &wsadata);
#endif
}

/* ---------------------------------------------------------------------
 * _get_make_sock_path(), builds and tests the path for the socket
 * directory.  Returns NULL on any failure, otherwise it returns the
 * directory path. The path will be like 
 * "/tmp/grass6-$USER-$GIS_LOCK".
 * ($GIS_LOCK is set in lib/init/init.sh to PID) 
 * ---------------------------------------------------------------------*/

static char *
_get_make_sock_path (void)
{
    char *path, *user, *lock;
    const char *prefix = "/tmp/grass6";
    int len, status;
    struct stat theStat;
    
    user = G_whoami(); /* Don't G_free () return value ever! */
    if (user == NULL)
        return NULL;
    else if (user[0] == '?') /* why's it do that? */
    {
        return NULL;
    }

    if ( (lock = getenv ( "GIS_LOCK" )) == NULL )
	G_fatal_error ("Cannot get GIS_LOCK enviroment variable value");

    len = strlen(prefix) + strlen(user) + strlen(lock) + 3;
    path = G_malloc (len);
    
    sprintf (path, "%s-%s-%s", prefix, user, lock);

    if ((status = G_lstat (path, &theStat)) != 0)
    {
        status = G_mkdir (path);
    }
    else 
    {
        if (!S_ISDIR (theStat.st_mode))
        {
            status = -1;  /* not a directory ?? */
        }
        else
        {
            status = chmod (path, S_IRWXU); /* fails if we don't own it */
        }
    }

    if (status) /* something's wrong if non-zero */
    {
        G_free (path);
        path = NULL;
    }

    return path;
}

#ifdef USE_TCP

#define PROTO PF_INET
typedef struct sockaddr_in sockaddr_t;

static int set_port(const char *name, int port)
{
    FILE *fp = fopen(name, "w");

    if (!fp)
	return -1;

    fprintf(fp, "%d\n", port);

    fclose(fp);

    return 0;
}

static int get_port(const char *name)
{
    FILE *fp = fopen(name, "r");
    int port;

    if (!fp)
	return -1;

    if (fscanf(fp, "%d", &port) != 1)
	port = -1;

    fclose(fp);

    return port;
}

static int save_port(int sockfd, const char *name)
{
    sockaddr_t addr;
    socklen_t size = sizeof(addr);

    if (getsockname(sockfd, (struct sockaddr *) &addr, &size) != 0)
	return -1;

    if (set_port(name, ntohs(addr.sin_port)) < 0)
	return -1;

    return 0;
}

static int make_address(sockaddr_t *addr, const char *name, int exists)
{
    int port = exists ? get_port(name) : 0;

    if (port < 0)
	return -1;

    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr->sin_port = htons((unsigned short) port);

    return 0;
}

#else

#define PROTO PF_UNIX
typedef struct sockaddr_un sockaddr_t;

static int make_address(sockaddr_t *addr, const char *name, int exists)
{
    addr->sun_family = AF_UNIX;

    /* The path to the unix socket must fit in sun_path[] */
    if (sizeof(addr->sun_path) < strlen(name) + 1)
        return -1;
    
    strncpy(addr->sun_path, name, sizeof(addr->sun_path) - 1);

    return 0;
}

#endif
        
/**
 * \fn char *G_sock_get_fname (const char *name)
 *
 * \brief Builds full path for a UNIX socket.
 *
 * Caller should <i>G_free()</i> the return value when it is no longer 
 * needed.
 *
 * \param[in] name
 * \return NULL on error
 * \return Pointer to string socket path on success
 */

char *
G_sock_get_fname (const char *name)
{
    char *path, *dirpath;
    int len;

    if (name == NULL)
        return NULL;
    
    dirpath = _get_make_sock_path();
    
    if (dirpath == NULL)
        return NULL;

    len = strlen (dirpath) + strlen(name) + 2;
    path = G_malloc (len);
    sprintf (path, "%s/%s", dirpath, name);
    G_free (dirpath);

    return path;
}


/**
 * \fn int G_sock_exists (const char *name)
 *
 * \brief Checks socket existence.
 *
 * \param[in] name
 * \return 1 if <b>name</b> exists
 * \return 0 if <b>name</b> does not exist
 */

int
G_sock_exists (const char *name)
{
    struct stat theStat;

    if (name == NULL || stat (name, &theStat) != 0)
        return 0;

#ifdef USE_TCP
    if (S_ISREG (theStat.st_mode))
#else
    if (S_ISSOCK (theStat.st_mode))
#endif
        return 1;
    else
        return 0;
}


/**
 * \fn int G_sock_bind (const char *name)
 *
 * \brief Binds socket to file descriptor.
 *
 * Takes the full pathname for a UNIX socket and returns the file 
 * descriptor to the socket after a successful call to <i>bind()</i>.
 *
 * \param[in] name
 * \return -1 and "errno" is set on error
 * \return file descriptor on success
 */

int
G_sock_bind (const char *name)
{
    int sockfd;
    sockaddr_t addr;
    socklen_t size;

    if (name == NULL)
        return -1;

    init_sockets();
    
    /* Bind requires that the file does not exist. Force the caller
     * to make sure the socket is not in use.  The only way to test,
     * is a call to connect().
     */
    if (G_sock_exists (name))
    {
        errno = EADDRINUSE;
        return -1;
    }

    /* must always zero socket structure */
    memset (&addr, 0, sizeof(addr));

    size = sizeof(addr);

    if (make_address(&addr, name, 0) < 0)
	return -1;

    sockfd = socket (PROTO, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET)
	    return -1;

    if (bind (sockfd, (const struct sockaddr *) &addr, size) != 0)
        return -1;

#ifdef USE_TCP
    if (save_port(sockfd, name) < 0)
	return -1;
#endif

    return sockfd;
}

/**
 * \fn int G_sock_listen (int sockfd, unsigned int queue_len)
 *
 * \brief Wrapper function to <i>listen()</i>.
 *
 * \param[in] sockfd
 * \param[in] queue_len
 * \return 0 on success
 * \return -1 and "errno" set on error
 */

int
G_sock_listen (int sockfd, unsigned int queue_len)
{
    return listen (sockfd, queue_len);
}


/**
 * \fn int G_sock_accept (int sockfd)
 *
 * \brief Wrapper around <i>accept()</i>.
 *
 * <b>Note:</b> This call will usually block until a connection arrives. 
 * <i>select()</i> can be used for a time out on the call.
 *
 * \param[in] sockfd
 * \return -1 and "errno" set on error
 * \return file descriptor on success
 */

int
G_sock_accept (int sockfd)
{
    sockaddr_t addr;
    socklen_t len = sizeof(addr);

    return accept (sockfd, (struct sockaddr *) &addr, &len);
}
 

/**
 * \fn int G_sock_connect (const char *name)
 *
 * \brief Tries to connect to the UNIX socket specified by <b>name</b>.
 *
 * \param[in] name
 * \return -1 and "errno" set on error
 * \return file descriptor on success
 */

int
G_sock_connect (const char *name)
{
    int sockfd;
    sockaddr_t addr;

    init_sockets();

    if (!G_sock_exists (name))
        return -1;

    /* must always zero socket structure */
    memset (&addr, 0, sizeof(addr));

    if (make_address(&addr, name, 1) < 0)
	return -1;

    sockfd = socket (PROTO, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET)
	return -1;

    if (connect (sockfd, (struct sockaddr *) &addr, sizeof(addr)) != 0)
        return -1;
    else
	return sockfd;
}

/* vim: set softtabstop=4 shiftwidth=4 expandtab : */
#endif
