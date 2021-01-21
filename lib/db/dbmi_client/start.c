/*!
 * \file db/dbmi_client/start.c
 * 
 * \brief DBMI Library (client) - open database connection
 *
 * (C) 1999-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __MINGW32__
#include <windows.h>
#include <process.h>
#include <fcntl.h>
#endif

#include <grass/spawn.h>
#include <grass/dbmi.h>

#define READ  0
#define WRITE 1

static void close_on_exec(int fd)
{
#ifndef __MINGW32__
    int flags = fcntl(fd, F_GETFD);
    fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
#endif
}

/*!
  \brief Initialize a new dbDriver for db transaction.
 
  If <i>name</i> is NULL, the db name will be assigned 
  connection.driverName.
  
  \param name driver name
  
  \return pointer to dbDriver structure
  \return NULL on error
*/
dbDriver *db_start_driver(const char *name)
{
    dbDriver *driver;
    dbDbmscap *list, *cur;
    const char *startup;
    int p1[2], p2[2];
    int pid;
    int stat;
    dbConnection connection;
    char ebuf[5];

    /* Set some environment variables which are later read by driver.
     * This is necessary when application is running without GISRC file and all
     * gis variables are set by application. 
     * Even if GISRC is set, application may change some variables during runtime,
     * if for example reads data from different gdatabase, location or mapset*/

    /* setenv() is not portable, putenv() is POSIX, putenv() in glibc 2.0-2.1.1 doesn't conform to SUSv2,
     * G_putenv() as well, but that is what we want, makes a copy of string */
    if (G_get_gisrc_mode() == G_GISRC_MODE_MEMORY) {
	G_debug(3, "G_GISRC_MODE_MEMORY\n");
	sprintf(ebuf, "%d", G_GISRC_MODE_MEMORY);
	G_putenv("GRASS_DB_DRIVER_GISRC_MODE", ebuf);	/* to tell driver that it must read variables */

	if (G_getenv_nofatal("DEBUG")) {
	    G_putenv("DEBUG", G_getenv_nofatal("DEBUG"));
	}
	else {
	    G_putenv("DEBUG", "0");
	}

	G_putenv("GISDBASE", G_getenv_nofatal("GISDBASE"));
	G_putenv("LOCATION_NAME", G_getenv_nofatal("LOCATION_NAME"));
	G_putenv("MAPSET", G_getenv_nofatal("MAPSET"));
    }
    else {
	/* Warning: GISRC_MODE_MEMORY _must_ be set to G_GISRC_MODE_FILE, because the module can be 
	 *          run from an application which previously set environment variable to G_GISRC_MODE_MEMORY */
	sprintf(ebuf, "%d", G_GISRC_MODE_FILE);
	G_putenv("GRASS_DB_DRIVER_GISRC_MODE", ebuf);
    }

    /* read the dbmscap file */
    if (NULL == (list = db_read_dbmscap()))
	return (dbDriver *) NULL;

    /* if name is empty use connection.driverName, added by RB 4/2000 */
    if (name[0] == '\0') {
	db_get_connection(&connection);
	if (NULL == (name = connection.driverName))
	    return (dbDriver *) NULL;
    }

    /* find this system name */
    for (cur = list; cur; cur = cur->next)
	if (strcmp(cur->driverName, name) == 0)
	    break;
    if (cur == NULL) {
	char msg[256];

	db_free_dbmscap(list);
	sprintf(msg, "%s: no such driver available", name);
	db_error(msg);
	return (dbDriver *) NULL;
    }

    /* allocate a driver structure */
    driver = (dbDriver *) db_malloc(sizeof(dbDriver));
    if (driver == NULL) {
	db_free_dbmscap(list);
	return (dbDriver *) NULL;
    }

    /* copy the relevant info from the dbmscap entry into the driver structure */
    db_copy_dbmscap_entry(&driver->dbmscap, cur);
    startup = driver->dbmscap.startup;

    /* free the dbmscap list */
    db_free_dbmscap(list);

    /* run the driver as a child process and create pipes to its stdin, stdout */

#ifdef __MINGW32__
#define pipe(fds) _pipe(fds, 250000, _O_BINARY | _O_NOINHERIT)
#endif

    /* open the pipes */
    if ((pipe(p1) < 0) || (pipe(p2) < 0)) {
	db_syserror("can't open any pipes");
	return (dbDriver *) NULL;
    }

    close_on_exec(p1[READ]);
    close_on_exec(p1[WRITE]);
    close_on_exec(p2[READ]);
    close_on_exec(p2[WRITE]);

    pid = G_spawn_ex(startup,
		     SF_BACKGROUND,
		     SF_REDIRECT_DESCRIPTOR, 0, p1[READ],
		     SF_CLOSE_DESCRIPTOR, p1[WRITE],
		     SF_REDIRECT_DESCRIPTOR, 1, p2[WRITE],
		     SF_CLOSE_DESCRIPTOR, p2[READ],
		     startup, NULL);

    /* create a child */
    if (pid < 0) {
	db_syserror("can't create fork");
	return (dbDriver *) NULL;
    }

    close(p1[READ]);
    close(p2[WRITE]);

    /* record driver process id in driver struct */
    driver->pid = pid;

    /* convert pipes to FILE* */
    driver->send = fdopen(p1[WRITE], "wb");
    driver->recv = fdopen(p2[READ], "rb");

    /* most systems will have to use unbuffered io to get the send/recv to work */
#ifndef USE_BUFFERED_IO
    setbuf(driver->send, NULL);
    setbuf(driver->recv, NULL);
#endif

    db__set_protocol_fds(driver->send, driver->recv);
    if (db__recv_return_code(&stat) != DB_OK || stat != DB_OK)
	driver = NULL;

    return driver;
}
