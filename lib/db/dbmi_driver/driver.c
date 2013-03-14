/*!
 * \file db/dbmi_driver/driver.c
 * 
 * \brief DBMI Library (driver) - drivers
 *
 * (C) 1999-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 * \author Modified by Glynn Clements <glynn gclements.plus.com>,
 * Markus Neteler <neteler itc.it>,
 * Huidae Cho <grass4u gmail.com>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include "procs.h"
#define	DB_DRIVER_C
#include "dbstubs.h"

extern char *getenv();

/*!
  \brief Get driver (?)

  \param argc, argv arguments

  \return 0 on success
  \return 1 on failure
 */
int db_driver(int argc, char *argv[])
{
    int stat;
    int procnum;
    int i;
    int rfd, wfd;
    FILE *send, *recv;
    char *modestr;

    /* Read and set environment variables, see dbmi_client/start.c */
    if ((modestr = getenv("GRASS_DB_DRIVER_GISRC_MODE"))) {
	int mode;

	mode = atoi(modestr);

	if (mode == G_GISRC_MODE_MEMORY) {
	    G_set_gisrc_mode(G_GISRC_MODE_MEMORY);
	    G__setenv("DEBUG", getenv("DEBUG"));
	    G__setenv("GISDBASE", getenv("GISDBASE"));
	    G__setenv("LOCATION_NAME", getenv("LOCATION_NAME"));
	    G__setenv("MAPSET", getenv("MAPSET"));
	    G_debug(3, "Driver GISDBASE set to '%s'", G_getenv("GISDBASE"));
	}
    }

#ifdef __MINGW32__
    /* TODO: */
    /* We should close everything except stdin, stdout but _fcloseall()
     * closes open streams not file descriptors. _getmaxstdio too big number.
     * 
     * Because the pipes were created just before this driver was started 
     * the file descriptors should not be above a closed descriptor
     * until it was run from a multithread application and some descriptors 
     * were closed in the mean time. 
     * Also Windows documentation does not say that new file descriptor is 
     * the lowest available.
     */

    {
	int err_count = 0;
	int cfd = 3;

	while (1) {
	    if (close(cfd) == -1)
		err_count++;

	    /* no good reason for 10 */
	    if (err_count > 10)
		break;

	    cfd++;
	}
    }

    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    send = stdout;
    recv = stdin;

    /* THIS CODE IS FOR DEBUGGING WITH CODECENTER */

/**********************************************/
    if (argc == 3) {
	rfd = wfd = -1;
	sscanf(argv[1], "%d", &rfd);
	sscanf(argv[2], "%d", &wfd);
	send = fdopen(wfd, "w");
	if (send == NULL) {
	    db_syserror(argv[1]);
	    exit(1);
	}
	recv = fdopen(rfd, "r");
	if (recv == NULL) {
	    db_syserror(argv[2]);
	    exit(1);
	}
    }

/**********************************************/

    db_clear_error();
    db_auto_print_errors(1);
    db_auto_print_protocol_errors(1);
    db__init_driver_state();

#ifndef USE_BUFFERED_IO
    setbuf(recv, NULL);
    setbuf(send, NULL);
#endif
    db__set_protocol_fds(send, recv);

    if (db_driver_init(argc, argv) == DB_OK)
	db__send_success();
    else {
	db__send_failure();
	exit(1);
    }

    stat = DB_OK;
    /* get the procedure number */
    while (db__recv_procnum(&procnum) == DB_OK) {
#ifdef __MINGW32__
	if (procnum == DB_PROC_SHUTDOWN_DRIVER) {
	    db__send_procedure_ok(procnum);
	    break;
	}
#endif
	db_clear_error();

	/* find this procedure */
	for (i = 0; procedure[i].routine; i++)
	    if (procedure[i].procnum == procnum)
		break;

	/* if found, call it */
	if (procedure[i].routine) {
	    if ((stat = db__send_procedure_ok(procnum)) != DB_OK)
		break;		/* while loop */
	    if ((stat = (*procedure[i].routine) ()) != DB_OK)
		break;
	}
	else if ((stat =
		  db__send_procedure_not_implemented(procnum)) != DB_OK)
	    break;
    }

    db_driver_finish();

    exit(stat == DB_OK ? 0 : 1);
}
