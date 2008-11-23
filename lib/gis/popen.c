#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef __MINGW32__
#  include <io.h>
#  include <fcntl.h>
#  include <process.h>
#endif

#include <grass/gis.h>

#define READ      0
#define WRITE     1

FILE *G_popen(const char *cmd, const char *mode)
{
#ifndef __MINGW32__
    return popen(cmd, mode);
#else
    int thepipes[2];
    FILE *rv = NULL;

    fflush(stdout);
    fflush(stderr);

    /*setvbuf ( stdout, NULL, _IONBF, 0 ); */

    if (_pipe(thepipes, 256, O_BINARY) != -1) {
	execl("cmd", "cmd", "/c", cmd, (char *)NULL);
	close(thepipes[WRITE]);
	rv = fdopen(thepipes[READ], mode);
    }

    return rv;
#endif
}

int G_pclose(FILE *ptr)
{
#ifndef __MINGW32__
    return pclose(ptr);
#else
    RETSIGTYPE(*sigint)(int);
    int status;
    int f;

    f = fileno(ptr);
    fclose(ptr);

    sigint = signal(SIGINT, SIG_IGN);

    _cwait(&status, popen_pid[f], WAIT_CHILD);
    if (0 & status) {
	status = -1;
    }

    signal(SIGINT, sigint);

    return status;
#endif
}
