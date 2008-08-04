#include <grass/gis.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/**********************************************************
 * G_gets (buf)
 *    char *buf      buffer to receive data
 *
 *  does a gets() from stdin. exits upon EOF.
 *  if stdin is a tty (ie, not a pipe or redirected)
 *  then ctrl-z is detected
 *
 * returns
 *  1 read ok
 *  0 ctrl-z entered. calling routine should re-print a prompt
 *    and call G_gets() again
 *
 * note: This is very useful for allowing a program to 
 *       reprompt after the program is restarted after
 *       being stopped with a ctrl-Z.
 *
 * sample use:
 *   do {
 *      fprintf (stderr, "Enter some input:  ") ;
 *   } while ( ! G_gets(buff) )  ;
 *
 *   If the user suspends the process at this prompt G_gets will return
 *   "0" causing the reprompting.
 ***********************************************************/

static int ctrlz = 0;
static void catch_ctrlz(int);
static void catch_int(int);


int G_gets(char *buf)
{
#ifdef SIGTSTP
    RETSIGTYPE(*sigtstp) ();
    int tty;
#endif
    char p[128];
    char *eof;

    ctrlz = 0;
#ifdef SIGTSTP
    if ((tty = isatty(0))) {
	sigtstp = signal(SIGTSTP, catch_ctrlz);
	if (sigtstp != (RETSIGTYPE(*)())SIG_DFL)
	    signal(SIGTSTP, sigtstp);
    }
#endif
    eof = fgets(p, 100, stdin);
    /* strip the EOL character      */
    if (strlen(p) > 1 && p[strlen(p) - 1] == '\n' && p[strlen(p) - 2] == '\r')
	p[strlen(p) - 2] = '\0';	/* Handles DOS/Windows "\r\n" */
    else
	p[strlen(p) - 1] = '\0';	/* Handles Unix "\n" or old Mac "\r" */
    /*      buf could be any length.  Any overflow will occur here. */
    strcpy(buf, p);

#ifdef SIGTSTP
    if (tty)
	signal(SIGTSTP, sigtstp);
#endif
    if (eof)
	return 1;
    if (ctrlz)
	return 0;

    exit(EXIT_SUCCESS);
}

static void catch_ctrlz(int n)
{
#ifdef __MINGW32__
    G_warning("catch_ctrlz: ignored Ctrl-z");
#else

    RETSIGTYPE(*sigint) ();

    /* having caught ctrlz - effect a ctrl-z using kill */
    ctrlz = 1;
    signal(n, SIG_DFL);
    kill(0, n);

    /* for berkley systems, ctrlz will not cause eof on read */
    sigint = signal(SIGINT, catch_int);
    kill(getpid(), SIGINT);
    signal(SIGINT, sigint);
#endif
}

static void catch_int(int n)
{
}
