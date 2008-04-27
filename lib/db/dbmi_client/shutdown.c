#include <stdlib.h>

#ifdef __MINGW32__
#include <process.h>
#else
#include <sys/wait.h>
#endif
#include <grass/dbmi.h>
#include "macros.h"

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
/* closedown the driver, and free the driver structure */
/* NOTE: the management of the memory for the driver structure
 * probably should be handled differently.
 *
 * db_start_driver() could take a pointer to driver structure as
 * an argument, instead of returning the pointer to allocated
 * then there would be no hidden free required
 */
int
db_shutdown_driver  (dbDriver *driver)

{
#ifndef __MINGW32__
    int pid;
#endif
    int status;

#ifdef __MINGW32__
    db__set_protocol_fds (driver->send, driver->recv);
    DB_START_PROCEDURE_CALL (DB_PROC_SHUTDOWN_DRIVER);
#endif

/* close the communication FILEs */
    fclose (driver->send);
    fclose (driver->recv);

    driver->send = NULL;
    driver->recv = NULL;

/* wait for the driver to finish */
    status = -1;

#ifdef __MINGW32__
    /* TODO: convert status to something like from wait? */
    _cwait( &status, driver->pid, WAIT_CHILD ); 
#else
    /* TODO: Should not be here waitpid() ? */
    while ((pid = wait(&status)) > 0 && pid != driver->pid)
	 {}
#endif

    driver->pid = 0;

/* free the driver structure. THIS IS GOOFY */
    free (driver);

    return status;
}
