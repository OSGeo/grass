#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/form.h>

#ifdef HAVE_SOCKET
#include <sys/types.h>
#ifdef __MINGW32__
#include <winsock.h>

#else /*__MINGW32__*/
#include <sys/socket.h>
#include <netinet/in.h>

#endif /*__MINGW32__*/
#endif /*HAVE_SOCKET */

#ifdef HAVE_SOCKET
static int make_socketpair(int *);
#endif

int first = 1;

/* the pipe to send data to GUI */
FILE *parent_send, *parent_recv;

#ifdef HAVE_SOCKET
int pipefd[2];

#define	pfd	pipefd[1]	/* parent's end */
#define	cfd	pipefd[0]	/* child's end */

#endif /*HAVE_SOCKET */

/* Open new form
 *
 *  returns: 0 success
 */
#ifdef __MINGW32__
int F_open(char *title, char *html)
{
    G_fatal_error("F_open is not supported on Windows");
    return 1;
}
#else
int F_open(char *title, char *html)
{
    /* parent */
    int c;

    /* common */
    static int pid;

#ifndef HAVE_SOCKET
    static int p1[2], p2[2];
#endif /*HAVE_SOCKET */
    int length;

    /* child */

    G_debug(2, "F_open(): title = %s", title);

    if (first) {
#ifdef HAVE_SOCKET
	if (make_socketpair(pipefd) < 0)
	    G_fatal_error("Cannot make socket pair");
#else
	if (pipe(p1) < 0 || pipe(p2) < 0)
	    G_fatal_error("Cannot open pipe");
#endif /*HAVE_SOCKET */

	if ((pid = fork()) < 0)
	    G_fatal_error("Cannot create fork");
    }

    if (pid == 0) {		/* Child */
	char command[2000], script[2000];

	G_debug(2, "CHILD");

	/* Note: If you are forking in a Tk based apllication  you
	 *       must  execl  before  doing any window operations in the
	 *       child or you will receive an error from the  X server */

	close(0);
	close(1);

#ifndef HAVE_SOCKET
	close(p1[1]);
	close(p2[0]);
	if (dup(p1[0]) != 0)
	    G_fatal_error("Form: cannot dup() input");
	if (dup(p2[1]) != 1)
	    G_fatal_error("Form: cannot dup() output");

#else
	close(pfd);
	if (dup(cfd) != 0)
	    G_fatal_error("Form: cannot dup() input");
	if (dup(cfd) != 1)
	    G_fatal_error("Form: cannot dup() output");

#endif /*HAVE_SOCKET */



	sprintf(command, "%s/etc/form/form", G_gisbase());
	sprintf(script, "%s/etc/form/form.tcl", G_gisbase());

	execl(command, "form", "-f", script, NULL);

	G_debug(2, "CHILD END\n");
	exit(0);

    }
    else {			/* Parent */
	G_debug(2, "PARENT");

	if (first) {
#ifndef HAVE_SOCKET
	    parent_send = fdopen(p1[1], "w");
	    close(p1[0]);
	    parent_recv = fdopen(p2[0], "r");
	    close(p2[1]);
#else
	    close(cfd);
	    parent_send = fdopen(pfd, "w");
	    parent_recv = fdopen(pfd, "r");
#endif /*HAVE_SOCKET */
	    first = 0;
	}

	G_debug(2, "PARENT HTML:\n%s\n", html);

	fprintf(parent_send, "O");
	length = strlen(title);
	fprintf(parent_send, "%d\n", length);
	fprintf(parent_send, "%s", title);
	length = strlen(html);
	fprintf(parent_send, "%d\n", length);
	fprintf(parent_send, "%s", html);
	fflush(parent_send);
	G_debug(2, "PARENT: Request sent\n");

	/* Wait for response */
	c = fgetc(parent_recv);
	G_debug(2, "PARENT: received %c\n", c);
    }

    return 0;
}
#endif

/* Clear old forms from window
 *
 */
void F_clear(void)
{
    char c;

    G_debug(2, "F_clear()");

    if (first)
	return;

    fprintf(parent_send, "C");
    fflush(parent_send);
    c = fgetc(parent_recv);
    G_debug(2, "PARENT: received %c\n", c);
}

void F_close(void)
{
    char c;

    G_debug(2, "F_close()");

    if (first)
	return;

    fprintf(parent_send, "D");
    fflush(parent_send);
    c = fgetc(parent_recv);
    G_debug(2, "PARENT: received %c\n", c);

    first = 1;
}

#ifdef HAVE_SOCKET
static int make_socketpair(int *fd)
{
    int n;

    if ((n = socketpair(AF_UNIX, SOCK_STREAM, IPPROTO_IP, fd)) < 0)
	return -1;
    else
	return 0;
}

#endif
