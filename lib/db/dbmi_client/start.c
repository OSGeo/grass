#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __MINGW32__
#include <windows.h>
#include <process.h>
#include <fcntl.h>
#endif

#include <grass/dbmi.h>

#define READ  0
#define WRITE 1


/**
 * \fn dbDriver *db_start_driver (char *name)
 *
 * \brief Initialize a new dbDriver for db transaction.
 *
 * If <b>name</b> is NULL, the db name will be assigned 
 * connection.driverName.
 *
 * \param[in] char * driver name
 * \return NULL on error
 */

dbDriver *
db_start_driver (char *name)

{
    dbDriver *driver;
    dbDbmscap *list, *cur;
    char *startup;
    int p1[2], p2[2];
    int pid;
    int stat;
    dbConnection connection;
    char ebuf[5];
#ifdef __MINGW32__
    int stdin_orig, stdout_orig;
    int have_stdin, have_stdout;
    int stdin_fd, stdout_fd;
#endif

    /* Set some enviroment variables which are later read by driver.
     * This is necessary when application is running without GISRC file and all
     * gis variables are set by application. 
     * Even if GISRC is set, application may change some variables during runtime,
     * if for example reads data from different gdatabase, location or mapset*/
    
    /* setenv() is not portable, putenv() is POSIX, putenv() in glibc 2.0-2.1.1 doesn't conform to SUSv2,
     * G_putenv() as well, but that is what we want, makes a copy of string */
    if (  G_get_gisrc_mode() == G_GISRC_MODE_MEMORY ) 
    {
        G_debug (3, "G_GISRC_MODE_MEMORY\n" );
	sprintf ( ebuf, "%d", G_GISRC_MODE_MEMORY );
	G_putenv("GRASS_DB_DRIVER_GISRC_MODE", ebuf); /* to tell driver that it must read variables */
	
	if ( G__getenv ( "DEBUG" ) ) {
	    G_putenv( "DEBUG", G__getenv ( "DEBUG" ) );
	} else {
	    G_putenv("DEBUG", "0");
	}

	G_putenv( "GISDBASE", G__getenv("GISDBASE") );
	G_putenv( "LOCATION_NAME", G__getenv("LOCATION_NAME") );
	G_putenv( "MAPSET", G__getenv("MAPSET") );
    } 
    else 
    {
	/* Warning: GISRC_MODE_MEMORY _must_ be set to G_GISRC_MODE_FILE, because the module can be 
	 *          run from an application which previously set enviroment variable to G_GISRC_MODE_MEMORY */
	sprintf ( ebuf, "%d", G_GISRC_MODE_FILE );
	G_putenv("GRASS_DB_DRIVER_GISRC_MODE", ebuf);
    }
     
/* read the dbmscap file */
    if(NULL == (list = db_read_dbmscap()))
	return (dbDriver *) NULL;

/* if name is empty use connection.driverName, added by RB 4/2000 */
    if( name == '\0' )
    {
	db_get_connection( &connection );
	if(NULL == (name = connection.driverName) )
	   return (dbDriver *) NULL;
    }

/* find this system name */
    for (cur = list; cur; cur = cur->next)
	if (strcmp (cur->driverName, name) == 0)
	    break;
    if (cur == NULL)
    {
	char msg[256];

	db_free_dbmscap (list);
	sprintf (msg, "%s: no such driver available", name );
	db_error (msg);
	return (dbDriver *) NULL;
    }

/* allocate a driver structure */
    driver = (dbDriver *) db_malloc (sizeof(dbDriver));
    if (driver == NULL)
    {
	db_free_dbmscap (list);
	return (dbDriver *) NULL;
    }
    
/* copy the relevant info from the dbmscap entry into the driver structure */
    db_copy_dbmscap_entry (&driver->dbmscap, cur);
    startup = driver->dbmscap.startup;

/* free the dbmscap list */
    db_free_dbmscap (list);

/* run the driver as a child process and create pipes to its stdin, stdout */

#ifdef __MINGW32__
    /* create pipes (0 in array for reading, 1 for writing) */
    /* p1 : module -> driver, p2 driver -> module */

    /* I have seen problems with pipes on NT 5.1 probably related
     * to buffer size (psize, originaly 512 bytes). 
     * But I am not sure, some problems were fixed by bigger 
     * buffer but others remain. 
     * Simple test which failed on NT 5.1 worked on NT 5.2 
     * But there are probably other factors. 
     */
    /* More info about pipes from MSDN:
       - Anonymous pipes are implemented using a named pipe 
         with a unique name.
       - CreatePipe() - nSize :
                   ... The size is only a suggestion; the system uses 
                   the value to calculate an appropriate buffering 
                   mechanism. ...
         => that that the size specified is not significant 
       - If the pipe buffer is full before all bytes are written, 
         WriteFile does not return until another process or thread 
         uses ReadFile to make more buffer space available.
         (Which does not seem to be true on NT 5.1)
    */
    if( _pipe(p1, 250000, _O_BINARY) < 0 ||
        _pipe(p2, 250000, _O_BINARY) < 0 ) 
    {
        db_syserror ("can't open any pipes");
	return (dbDriver *) NULL;
    }

    /* convert pipes to FILE* */
    driver->send = fdopen (p1[WRITE], "wb");
    driver->recv = fdopen (p2[READ],  "rb");

    fflush (stdout);
    fflush (stderr);

    /* Set pipes for stdin/stdout driver */

    have_stdin = have_stdout = 1;

    if ( _fileno(stdin) < 0 ) 
    {
        have_stdin = 0;
        stdin_fd = 0; 
    }
    else
    {
        stdin_fd = _fileno(stdin);

        if ( (stdin_orig  = _dup(_fileno(stdin ))) < 0  ) 
        {
            db_syserror ("can't duplicate stdin");
	    return (dbDriver *) NULL;
	}
    
    }

    if ( _dup2(p1[0], stdin_fd) != 0 )
    {
        db_syserror ("can't duplicate pipe");
        return (dbDriver *) NULL;
    }

    if ( _fileno(stdout) < 0 ) 
    {
        have_stdout = 0;
        stdout_fd = 1; 
    }
    else
    {
        stdout_fd = _fileno(stdout);

        if ( (stdout_orig  = _dup(_fileno(stdout ))) < 0 ) 
        {
            db_syserror ("can't duplicate stdout");
	    return (dbDriver *) NULL;
	}
    
    }

    if ( _dup2(p2[1], stdout_fd) != 0 ) 
    {
        db_syserror ("can't duplicate pipe");
        return (dbDriver *) NULL;
    }

    /* Warning: the driver on Windows must have extension .exe
     *          otherwise _spawnl fails. The name used as _spawnl 
     *          parameter can be without .exe 
     */ 
    /* spawnl() works but the process inherits all handlers, 
     * that means, for example p1[WRITE] remains open and if 
     * module exits the pipe is not close and driver remains running. 
     * Using CreateProcess() + SetHandleInformation() does not help.
     * => currently the only know solution is to close all file 
     *    descriptors in driver (which is another problem)
     */

     pid = _spawnl ( _P_NOWAIT, startup, startup, NULL ); 
    
    /* This does not help. It runs but pipe remains open when close() is 
     * called in model but caling close() on that descriptor in driver gives 
     * error. */
    /* 
    {
        STARTUPINFO    si;
        PROCESS_INFORMATION  pi;

        GetStartupInfo(&si);

        SetHandleInformation ( stdin, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
        SetHandleInformation ( stdout, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
        SetHandleInformation ( driver->send, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation ( driver->recv, HANDLE_FLAG_INHERIT, 0);

        CreateProcess(NULL, startup, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    }
    */

    /* Reset stdin/stdout for module and close duplicates */
    if ( have_stdin )
    {
        if ( _dup2(stdin_orig, _fileno(stdin)) != 0 ) 
        {
            db_syserror ("can't reset stdin");
	    return (dbDriver *) NULL;
        }
        close ( stdin_orig );
    }


    if ( have_stdout )
    {
        if ( _dup2(stdout_orig, _fileno(stdout)) != 0 ) 
        {
            db_syserror ("can't reset stdout");
	    return (dbDriver *) NULL;
        }
        close ( stdout_orig );
    }

    if ( pid == -1 ) {
        db_syserror ("can't _spawnl");
	return (dbDriver *) NULL;
    }

    /* record driver process id in driver struct */
    driver->pid = pid;

    /* most systems will have to use unbuffered io to get the 
     *  send/recv to work */
#ifndef USE_BUFFERED_IO
	setbuf (driver->send, NULL);
	setbuf (driver->recv, NULL);
#endif

    db__set_protocol_fds (driver->send, driver->recv);
    if(db__recv_return_code(&stat) !=DB_OK || stat != DB_OK)
        driver =  NULL;

    return driver;

#else /* __MINGW32__ */

/* open the pipes */
    if ((pipe(p1) < 0 ) || (pipe(p2) < 0 ))
    {
        db_syserror ("can't open any pipes");
	return (dbDriver *) NULL;
    }

/* create a child */
    if ((pid = fork()) < 0)
    {
        db_syserror ("can't create fork");
	return (dbDriver *) NULL;
    }

    if (pid > 0)        /* parent */
    {
        close(p1[READ]);
        close(p2[WRITE]);

/* record driver process id in driver struct */
        driver->pid = pid;

/* convert pipes to FILE* */
	driver->send = fdopen (p1[WRITE], "wb");
	driver->recv = fdopen (p2[READ],  "rb");

/* most systems will have to use unbuffered io to get the send/recv to work */
#ifndef USE_BUFFERED_IO
	setbuf (driver->send, NULL);
	setbuf (driver->recv, NULL);
#endif

	db__set_protocol_fds (driver->send, driver->recv);
	if(db__recv_return_code(&stat) !=DB_OK || stat != DB_OK)
	    driver =  NULL;

	return driver;
    }
    else        /* child process */
    {
        close(p1[WRITE]);
        close(p2[READ]);

        close (0);
        close (1);

        if (dup(p1[READ]) != 0)
        {
            db_syserror("dup r");
            _exit (EXIT_FAILURE);
        }

        if (dup(p2[WRITE]) != 1)
        {
            db_syserror("dup w");
            _exit (EXIT_FAILURE);
        }

	execl ("/bin/sh", "sh", "-c", startup, NULL);

        db_syserror ("execl");
	return NULL; /* to keep lint, et. al. happy */
    }

#endif /* __MINGW32__ */
}
