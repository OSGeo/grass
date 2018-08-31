#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <unistd.h>

#include <grass/gis.h>

#ifdef SIGPIPE
static RETSIGTYPE (*sigpipe)(int);
#endif

FILE *G_open_pager(struct Popen *pager)
{
    const char *program = getenv("GRASS_PAGER");
    FILE *fp;

    G_popen_clear(pager);

    if (!program)
	return stdout;

    if (!isatty(STDOUT_FILENO))
	return stdout;

#ifdef SIGPIPE
    sigpipe = signal(SIGPIPE, SIG_IGN);
#endif

    fp = G_popen_write(pager, program, NULL);

    return fp ? fp : stdout;
}

void G_close_pager(struct Popen *pager)
{
    G_popen_close(pager);

#ifdef SIGPIPE
    if (sigpipe)
	signal(SIGPIPE, sigpipe);
#endif
}

FILE *G_open_mail(struct Popen *mail)
{
    const char *user = G_whoami();
    const char *argv[3];
    FILE *fp;

    G_popen_clear(mail);

    if (!user || !*user)
	return NULL;

    argv[0] = "mail";
    argv[1] = user;
    argv[2] = NULL;

    fp = G_popen_write(mail, "mail", argv);

    return fp;
}

void G_close_mail(struct Popen *mail)
{
    G_popen_close(mail);
}

