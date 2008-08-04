
/**
 * \file system.c
 *
 * \brief GIS Library - Command execution functions.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <grass/config.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#ifndef __MINGW32__
#include <sys/wait.h>
#endif
#include <grass/gis.h>
#include <grass/glocale.h>


/**
 * \brief Run a shell level command.
 *
 * This is essentially the UNIX <i>system()</i> call, except for the 
 * signal handling. During the call, user generated signals (intr, quit)
 * for the parent are ignored, but allowed for the child. Parent
 * signals are reset upon completion.<br>
 *
 * This routine is useful for menu type programs that need to run
 * external commands and allow these commands to be interrupted by
 * the user without killing the menu itself.<br>
 *
 * <b>Note:</b> if you want the signal settings to be the same for the
 * parent and the command being run, set them yourself and use
 * the UNIX <i>system()</i> call instead.
 *
 * \param[in] command
 * \return -1 on error
 * \return status on success
 */

int G_system(const char *command)
{
    int status;

#ifndef __MINGW32__
    int pid, w;
#endif
    RETSIGTYPE(*sigint) ();
#ifdef SIGQUIT
    RETSIGTYPE(*sigquit) ();
#endif

    sigint = signal(SIGINT, SIG_IGN);
#ifdef SIGQUIT
    sigquit = signal(SIGQUIT, SIG_IGN);
#endif

    fflush(stdout);
    fflush(stderr);

#ifdef __MINGW32__
    signal(SIGINT, SIG_DFL);
    _spawnlp(P_WAIT, "cmd.exe", "cmd.exe", "/c", command, NULL);
    status = 0;
#else
    if ((pid = fork()) == 0) {
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);

	execl("/bin/sh", "sh", "-c", command, NULL);
	_exit(127);
    }

    if (pid < 0) {
	G_warning(_("Can not create a new process!"));
	status = -1;
    }
    else {
	while ((w = wait(&status)) != pid && w != -1) ;

	if (w == -1)
	    status = -1;
    }

#endif

    signal(SIGINT, sigint);
#ifdef SIGQUIT
    signal(SIGQUIT, sigquit);
#endif

    return (status);
}
