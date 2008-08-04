
/****************************************************************************
 *
 * MODULE:       driver
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com>(original contributor)
 *               Jachym Cepicky <jachym les-ejk.cz>
 * PURPOSE:      graphics monitor driver
 * COPYRIGHT:    (C) 2006-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include "driverlib.h"
#include "driver.h"
#include "pad.h"

static jmp_buf save;

static RETSIGTYPE handle_sigpipe(int sig)
{
    longjmp(save, 1);
}

static RETSIGTYPE handle_sigterm(int sig)
{
    COM_Graph_close();
}

int LIB_main(int argc, char **argv)
{
    char *me;
    int _wfd;
    int _rfd;
    int eof;

    char c;
    pid_t pid;
    int foreground;
    int listenfd;

#ifndef __MINGW32__
    struct sigaction sigact;
#endif

    /* The calling syntax is as follows:
       monitor_name [-] ["input_fifo output_fifo"]

       The "-", if present, causes the monitor to run in foreground.
       Otherwise, once it has been determined that the monitor is not
       already running, we will fork and the parent will exit, so that
       the monitor is left running in background.
     */

    if (argc < 2) {
	G_warning("Usage:  %s <name> [-]", argv[0]);
	return 1;
    }

    /* whoami */
    me = argv[1];

    foreground = (argc >= 3 && argv[2][0] == '-');

#ifndef __MINGW32__
#ifdef SIGPIPE
    sigact.sa_handler = handle_sigpipe;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGPIPE, &sigact, NULL);
#endif
    sigact.sa_handler = handle_sigterm;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGTERM, &sigact, NULL);
#endif

    listenfd = prepare_connection_sock(me);

    G_message(_("Graphics driver [%s] started"), me);

#ifndef __MINGW32__
    if (!foreground) {
	pid = fork();
	if (pid) {
	    if (pid > 0) {	/* parent exits */
		exit(0);
	    }
	    else {		/* weren't able to fork */

		G_fatal_error("Error - Could not fork to start [%s]", me);
		exit(EXIT_FAILURE);
	    }
	}
	else {
	    /* change process groups to be shielded from keyboard signals */
#ifdef SETPGRP_VOID
	    setpgrp();
#else
	    setpgrp(0, getpid());
#endif
	}
    }				/* monitor runs */
#endif

    while (1) {			/* re-open upon EOF */
	for (;;) {
	    if (get_connection_sock(listenfd, &_rfd, &_wfd, COM_Work_stream())
		>= 0)
		break;

	    COM_Do_work(0);
	}

	command_init(_rfd, _wfd);

	COM_Client_Open();

	eof = 0;

	while (eof <= 0) {
	    COM_Do_work(1);

	    if (setjmp(save)) {
		G_warning("Monitor <%s>: Caught SIGPIPE", me);
		break;
	    }

	    if (get_command(&c) != 0)
		break;

	    if (process_command(c)) {
		G_warning("Monitor <%s>: Premature EOF", me);
		break;
	    }
	}

	/* read encountered EOF. close socket now */
	close(_wfd);
	close(_rfd);
	_rfd = _wfd = -1;	/* Set to invalid file descriptor number */

	COM_Client_Close();
    }

    return 0;
}
