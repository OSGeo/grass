#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef __MINGW32__
#  include <io.h>
#  include <fcntl.h>
#  include <process.h>
static int popen_pid[1024];
#endif

#include <grass/gis.h>

FILE *G_popen(const char *cmd, const char *mode)
{
#ifndef __MINGW32__
    return popen(cmd, mode);
#else
    RETSIGTYPE(*sigint)(int);
    int pipe_fd[2];
    FILE *rv;
    int pid;
    int dir, odir;
    int oldfd;

    fflush(stdout);
    fflush(stderr);

    switch (mode[0]) {
    case 'r':	dir = 0;	break;
    case 'w':	dir = 1;	break;
    default:	return NULL;
    }
    odir = 1-dir;

    if (_pipe(pipe_fd, 256, O_BINARY) < 0)
	return NULL;

    sigint = signal(SIGINT, SIG_IGN);

    oldfd = _dup(odir);
    _dup2(pipe_fd[odir], odir);
    pid = spawnlp(_P_NOWAIT, "cmd", "cmd", "/c", cmd, (char *)NULL);
    _dup2(oldfd, odir);
    close(oldfd);

    signal(SIGINT, sigint);

    close(pipe_fd[odir]);
    popen_pid[pipe_fd[dir]] = pid;
    rv = fdopen(pipe_fd[dir], mode);

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
