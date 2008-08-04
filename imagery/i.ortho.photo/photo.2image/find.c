#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef __MINGW32__
#include <process.h>
#else
#include <sys/wait.h>
#endif
#include <grass/gis.h>
#include "globals.h"


/*
 * run etc/i.find command in background to find all cell, vect files
 * in the target location.
 */
int find_target_files(void)
{
    int pid, w, status;

    select_target_env();
    pid = G_fork();		/* use G_fork() to inhibit signals */
    if (pid < 0) {
	perror("fork");
	exit(1);
    }

    /*
     * parent waits for child. this wait will be short since child
     * simply forks and exits. The grandchild runs in background
     * and grandma continues
     */
    if (pid) {
	while ((w = wait(&status)) != pid && w != -1) ;
    }
    else {
	char command[1024];

	sprintf(command, "%s/etc/i.find", G_gisbase());
	if (fork())
	    exit(0);		/* go into background */
	execl(command, "i.find",
	      G_location(), G_mapset(),
	      "cell", cell_list, "dig", vect_list, (char *)0);
    }
    select_current_env();

    return 0;
}
