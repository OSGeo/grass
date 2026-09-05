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

/*!
   \brief Close a pipe opened by G_popen_read() or G_popen_write() and wait
   for the child

   \param state pipe state filled in by G_popen_read() or G_popen_write()

   \return exit status of the child, as returned by G_wait()
   \return G_POPEN_NO_CHILD if there was no child to wait for
 */
int G_popen_close(struct Popen *state)
{
    int status = G_POPEN_NO_CHILD;

    if (state->fp) {
        fclose(state->fp);
        state->fp = NULL;
    }

    if (state->pid != -1) {
        status = G_wait(state->pid);
        /* The child has been reaped, so a second close does not wait again
           and report a failure for a pipe that was closed successfully. */
        state->pid = -1;
    }

    return status;
}
