/*!
 * \file db/dbmi_client/shutdown.c
 * 
 * \brief DBMI Library (client) - shutdown database connection
 *
 * (C) 1999-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <stdlib.h>

#ifdef __MINGW32__
#include <process.h>
#else
#include <sys/wait.h>
#endif
#include <grass/dbmi.h>
#include "macros.h"

/*!
  \brief Closedown the driver, and free the driver structure

  <b>Note:</b> the management of the memory for the driver structure
  probably should be handled differently.
 
  db_start_driver() could take a pointer to driver structure as
  an argument, instead of returning the pointer to allocated
  then there would be no hidden free required

  \param driver db driver

  \return status (?)
*/
int db_shutdown_driver(dbDriver * driver)
{
#ifndef __MINGW32__
    int pid;
#endif
    int status;

#ifdef __MINGW32__
    db__set_protocol_fds(driver->send, driver->recv);
    DB_START_PROCEDURE_CALL(DB_PROC_SHUTDOWN_DRIVER);
#endif

    /* close the communication FILEs */
    fclose(driver->send);
    fclose(driver->recv);

    driver->send = NULL;
    driver->recv = NULL;

    /* wait for the driver to finish */
    status = -1;

#ifdef __MINGW32__
    /* TODO: convert status to something like from wait? */
    _cwait(&status, driver->pid, WAIT_CHILD);
#else
    /* TODO: Should not be here waitpid() ? */
    while ((pid = wait(&status)) > 0 && pid != driver->pid) {
    }
#endif

    driver->pid = 0;

    /* free the driver structure. THIS IS GOOFY */
    db_free(driver);

    return status;
}
