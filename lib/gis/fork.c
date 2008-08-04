#include <grass/config.h>

#include <unistd.h>
#include <grass/gis.h>

/*************************************************************
 * G_fork() 
 *
 * Issue a system fork() call and protect the child from all
 * signals (which it does by changing the process group for the child)
 *
 * returns:
 *     -1 fork failed.
 *      0 child
 *     >0 parent
 ************************************************************/

int G_fork(void)
{
#ifdef __MINGW32__
    return -1;
#else /* __MINGW32__ */
    int pid;

    pid = fork();

    /*
     * change the process group for the child (pid == 0)
     * note: we use the BSD calling sequence, since
     * it will work ok for ATT call which has no arguments
     */
    if (pid == 0)
#ifdef SETPGRP_VOID
	setpgrp();
#else
	setpgrp(0, getpid());
#endif

    return pid;

#endif /* __MINGW32__ */

}
