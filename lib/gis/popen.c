#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <grass/gis.h>
#include <grass/spawn.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#define pipe(fds) _pipe(fds, 4096, O_BINARY | O_NOINHERIT)
#endif

static FILE *do_popen(struct Popen *state, int wr, const char *program,
                      const char **args)
{
    int which = wr ? 0 : 1;
    const char *dir = wr ? "w" : "r";
    int pfd, cfd;
    int pipe_fds[2];
    const char *argv[2];

    state->fp = NULL;
    state->pid = -1;

    if (pipe(pipe_fds) < 0)
        return NULL;

    cfd = pipe_fds[wr ? 0 : 1];
    pfd = pipe_fds[wr ? 1 : 0];

    if (!args) {
        argv[0] = program;
        argv[1] = NULL;
        args = argv;
    }

    state->pid =
        G_spawn_ex(program, SF_ARGVEC, args, SF_REDIRECT_DESCRIPTOR, which, cfd,
                   SF_CLOSE_DESCRIPTOR, pfd, SF_BACKGROUND, NULL);

    if (state->pid == -1) {
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return NULL;
    }

    close(cfd);

    state->fp = fdopen(pfd, dir);

    return state->fp;
}

void G_popen_clear(struct Popen *state)
{
    state->fp = NULL;
    state->pid = -1;
}

FILE *G_popen_write(struct Popen *state, const char *program, const char **args)
{
    return do_popen(state, 1, program, args);
}

FILE *G_popen_read(struct Popen *state, const char *program, const char **args)
{
    return do_popen(state, 0, program, args);
}

void G_popen_close(struct Popen *state)
{
    if (state->fp)
        fclose(state->fp);

    if (state->pid != -1)
        G_wait(state->pid);
}
