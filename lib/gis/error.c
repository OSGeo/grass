/*!
 * \file error.c
 */

/*
 **********************************************************************
 *
 * G_fatal_error (msg)
 *      char *msg          One line error message
 *
 * G_warning (msg)
 *      char *msg
 *
 *   Gives the message, msg, to the user.  Also print a message to 
 *   $GISBASE/GIS_ERROR_LOG if the file exists and is writeable; 
 *   and to the file $HOME/GIS_ERROR_LOG in the user's home directory 
 *   if $HOME is set and the file exists.
 *   G_warning() returns, which G_fatal_error() exits.
 *
 *   note:  By default, the message is handled by an internal routine
 *      which prints the message to the screen.   Using G_set_error_routine()
 *      the programmer can have the message handled by another routine.
 *      This is especially useful if the message should go to a particular
 *      location on the screen when using curses or to a location on
 *      a graphics device (monitor).
 *          
 **********************************************************************
 * G_set_error_routine (error_routine)
 *      int (*error_routine)()
 *
 *   Establishes error_routine as the routine that will handle 
 *   the printing of subsequent error messages. error_routine
 *   will be called like this:
 *      error_routine(msg, fatal)  
 *         char *msg ;
 **********************************************************************
 * G_unset_error_routine ()
 *
 *   After this call subsequent error messages will be handled in the
 *   default method.  Error messages are printed directly to the
 *   screen:
 *           ERROR: message
 *   -or-    WARNING: message
 *
 **********************************************************************/
/*
     Throughout the code the references to these routines attempt to
	send format strings and arguments.  It seems that we expect
	these to handle varargs, so now they do.    7-Mar-1999
		Bill Hughes
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/types.h>
#include <grass/glocale.h>
#include <grass/gis.h>

/*!
 * \def MSG
 *
 * \brief A message
 */
#define MSG  0
/*!
 * \def WARN
 *
 * \brief A warning message
 */
#define WARN 1
/*!
 * \def ERR
 *
 * \brief A fatal error message
 */
#define ERR  2


/* static int (*error)() = 0; */
static int (*ext_error)(const char *, int); /* Roger Bivand 17 June 2000 */
static int no_warn = 0;
static int no_sleep = 1;
static int message_id = 1;

static int print_word (FILE *, char **, int *, const int);
static void print_sentence (FILE *, const int, const char *);
static int print_error (const char *, const int);
static int mail_msg (const char *, int);
static int write_error (const char *, int, const char *, 
                        time_t, const char *);
static int log_error (const char *, int);

static int
vfprint_error (int type, const char *template, va_list ap)
{
    char buffer[2000];  /* G_asprintf does not work */

    vsprintf (buffer, template, ap);

    /* . */
    return print_error (buffer, type);
}

/*!
 * \fn void G_message (const char *msg,...)
 *
 * \brief Print a message to stderr
 *
 * The output format depends on enviroment variable GRASS_MESSAGE_FORMAT
 *
 * \param[in] msg string.  Cannot be NULL.
*/
void G_message (const char *msg,...)
{
    if( G_verbose() >= G_verbose_std() ) {
	va_list ap;
	va_start(ap, msg);
	vfprint_error (MSG, msg, ap);
	va_end(ap);
    }
}


/*!
 * \fn void G_verbose_message (const char *msg,...)
 *
 * \brief Print a message to stderr but only if module is in verbose mode
 *
 * The output format depends on enviroment variables
 *  GRASS_MESSAGE_FORMAT and GRASS_VERBOSE
 *
 * \param[in] msg string.  Cannot be NULL.
*/
void G_verbose_message (const char *msg, ...)
{
    if( G_verbose() > G_verbose_std() ) {
	va_list ap;
	va_start(ap, msg);
	vfprint_error (MSG, msg, ap);
	va_end(ap);
    }
}


/*!
 * \fn void G_important_message (const char *msg,...)
 *
 * \brief Print a message to stderr even in brief mode (verbosity=1)
 *
 * Ususally just G_percent()/G_clicker() would be shown at this level.
 * This allows important non-error/warning messages to display as well.
 *
 * The output format depends on enviroment variables
 *  GRASS_MESSAGE_FORMAT and GRASS_VERBOSE
 *
 * \param[in] msg string.  Cannot be NULL.
*/
void G_important_message(const char *msg, ...)
{
    if( G_verbose() > G_verbose_min() ) {
	va_list ap;
	va_start(ap, msg);
	vfprint_error (MSG, msg, ap);
	va_end(ap);
    }
}



/*!
 * \fn int G_fatal_error (const char *msg,...)
 *
 * \brief Print a fatal error message to stderr
 * 
 * The output format depends on enviroment variable
 * GRASS_MESSAGE_FORMAT
 *
 * By default, the message is handled by an internal routine which
 * prints the message to the screen. Using G_set_error_routine() the
 * programmer can have the message handled by another routine. This is
 * especially useful if the message should go to a particular location
 * on the screen when using curses or to a location on a graphics
 * device (monitor).
 *
 * \return Terminates with an exit status of EXIT_FAILURE if no external
 * routine is specified by G_set_error_routine()
 *
 * \param[in] msg string.  Cannot be NULL.
*/
int G_fatal_error (const char *msg,...)
{
    va_list ap;

    va_start(ap,msg);
    vfprint_error (ERR, msg, ap);
    va_end(ap);
    
    exit (EXIT_FAILURE);
}

/*!
 *
 * \fn int G_warning (const char *msg, ...)
 *
 * \brief Print a warning message to stderr
 * 
 * The output format depends on enviroment variable
 * GRASS_MESSAGE_FORMAT
 *
 * A warning message can be suppressed by G_suppress_warnings()
 *
 * \param[in] msg string.  Cannot be NULL.
*/
int G_warning (const char *msg, ...)
{
    va_list ap;

    if (no_warn) return 0;

    va_start(ap,msg);
    vfprint_error (WARN, msg, ap);
    va_end(ap);

    return 0;
}

/*!
 * \fn int G_suppress_warnings (int flag)
 *
 * \brief Suppress printing a warning message to stderr
 * 
 * \param[in] flag a warning message will be suppressed if non-zero value is given
 * \return previous flag
*/
int G_suppress_warnings (int flag)
{
    int prev;

    prev = no_warn;
    no_warn = flag;
    return prev;
}

/*!
 * \fn int G_sleep_on_error (int flag)
 *
 * \brief Turn on/off no_sleep flag
 * 
 * \param flag if non-zero/zero value is given G_sleep() will be activated/deactivated
 *
 * \param[in] flag error  message will be suppressed if non-zero value is given
 * \return previous flag
*/
int G_sleep_on_error (int flag)
{
    int prev;

    prev = !no_sleep;
    no_sleep = !flag;
    return prev;
}

/*!
 * \fn int G_set_error_routine ( int (*error_routine)(char *, int))
 *
 * \brief Establishes error_routine as the routine that will handle
 * the printing of subsequent error messages.
 * 
 * \param[in] error_routine routine will be called like this: error_routine(msg,
 * fatal)
 *
 * \return always null
*/
int G_set_error_routine(int (*error_routine)(const char *, int))
{
    ext_error = error_routine; /* Roger Bivand 17 June 2000 */
    return 0;
}

/*!
 * \fn int G_unset_error_routine (void)
 *
 * \brief After this call subsequent error messages will be handled in the
 * default method.
 * 
 * Error messages are printed directly to the screen: ERROR: message or WARNING: message
 *
 * \return always returns 0.
*/
int G_unset_error_routine(void)
{
    ext_error = 0; /* Roger Bivand 17 June 2000 */

    return 0;
}

/* Print info to stderr and optionaly to log file and optionaly send mail */
static int print_error(const char *msg, const int type)
{
    static char *prefix_std[3];
    int fatal, format;
    
    if ( !prefix_std[0] ) { /* First time: set prefixes  */
        prefix_std[0] = "";
	prefix_std[1] = _("WARNING: ");
	prefix_std[2] = _("ERROR: ");
    }

    if ( type == ERR )
	fatal = TRUE;
    else /* WARN */
	fatal = FALSE;

    if ( (type == WARN || type == ERR) && ext_error) { /* Function defined by application */
	ext_error (msg, fatal);
    } else {
	char *w;
	int len, lead;

        format = G_info_format();

	if ( format != G_INFO_FORMAT_GUI ) {
	    if ( type == WARN || type == ERR ) { 
		log_error (msg, fatal);
	    }

	    fprintf(stderr,"%s", prefix_std[type] );
	    len = lead = strlen ( prefix_std[type] );
	    w = (char *)msg;

	    while (print_word(stderr,&w,&len,lead))
		    ;

	    if ( (type != MSG) && isatty(fileno(stderr))
		    && (G_info_format() == G_INFO_FORMAT_STANDARD) ) { /* Bell */
		fprintf(stderr,"\7");
		fflush (stderr);
		if (!no_sleep)
		    G_sleep (5);
	    } else if ( (type == WARN || type == ERR) && getenv("GRASS_ERROR_MAIL")) { /* Mail */
		mail_msg (msg, fatal);
	    }
	} else { /* GUI */
	    print_sentence ( stderr, type, msg );
	}
    }

    return 0;
}

static int log_error (const char *msg, int fatal)
{
    FILE *pwd;
    char cwd[1024];
    time_t clock;
    char *home;
    char *gisbase;

    /* get time */
    clock = time(NULL);

    /* get current working directory */
    sprintf(cwd,"?");
    if ( (pwd = G_popen ("pwd","r")) )
    {
	if (fgets(cwd, sizeof(cwd), pwd))
	{
	    char *c;

	    for (c = cwd; *c; c++)
		if (*c == '\n')
		    *c = 0;
	}
	G_pclose (pwd);
    }

    /* write the 2 possible error log files */
    if ((gisbase = G_gisbase ()))
	write_error (msg, fatal, gisbase, clock, cwd);

    home = G__home();
    if (home && gisbase && strcmp (home, gisbase))
	write_error (msg, fatal, home, clock, cwd);

    return 0;
}

/* Write a message to the log file */
static int write_error (const char *msg, int fatal,
                        const char *dir, time_t clock,
                        const char *cwd)
{
    char logfile[GNAME_MAX];
    FILE *log;

    if (dir == 0 || *dir == 0)
	return 1;
    sprintf (logfile, "%s/GIS_ERROR_LOG", dir) ;

    log = fopen (logfile,"r");
    if (!log)
    	/* GIS_ERROR_LOG file is not readable or does not exist */
	return 1;

    log = freopen(logfile, "a", log);
    if (!log)
    	/* the user doesn't have write permission */
	return 1;

    fprintf(log,"-------------------------------------\n");
    fprintf(log,"%-10s %s\n", "program:", G_program_name());
    fprintf(log,"%-10s %s\n", "user:", G_whoami());
    fprintf(log,"%-10s %s\n", "cwd:", cwd);
    fprintf(log,"%-10s %s\n", "date:", ctime(&clock));
    fprintf(log,"%-10s %s\n", fatal?"error:":"warning:", msg);
    fprintf(log,"-------------------------------------\n");

    fclose (log);

    return 0;
}

/* Mail a message */
static int mail_msg (const char *msg, int fatal)
{
    FILE *mail;
    char command[64];
    char *user;

    user = G_whoami();
    if (user == 0 || *user == 0)
	return 1;

    sprintf (command, "mail '%s'", G_whoami());
    if ( (mail = G_popen (command, "w")) )
    {
	fprintf(mail,"GIS %s: %s\n",fatal?"ERROR":"WARNING",msg);
	G_pclose (mail);
    }

    return 0;
}

/* Print one word, new line if necessary */
static int print_word (FILE *fd, char **word, int *len, const int lead)
{
    int  wlen, start, totlen;
    int  nl;
    char *w,*b;

    start = *len;
    w = *word;

    nl = 0;
    while (*w == ' ' || *w == '\t' || *w == '\n')
	if(*w++ == '\n')
	    nl++;

    wlen = 0;
    for (b = w; *b != 0 && *b != ' ' && *b != '\t' && *b != '\n'; b++)
	wlen++;

    if (wlen == 0)
    {
	fprintf (fd, "\n");
	return 0;
    }

    if ( start > lead ) { /* add space */
	totlen = start + wlen + 1;
    } else {
	totlen = start + wlen; 
    }
    
    if ( nl != 0 || totlen > 75)
    {
	while (--nl > 0)
	    fprintf (fd, "\n");
	fprintf (fd, "\n%*s",lead,"");
	start = lead;
    }

    if ( start > lead ) {
      fprintf (fd, " ");
      start++;
    }

    *len = start + wlen;

    fwrite(w, 1, wlen, fd);
    w += wlen;

    *word = w;

    return 1;
}

/* Print one message, prefix inserted before each new line */
static void print_sentence (FILE *fd, const int type, const char *msg)
{
    char prefix[100];
    const char *start;

    switch ( type ) {
	case MSG: 
    	    sprintf (prefix, "GRASS_INFO_MESSAGE(%d,%d): ", getpid(), message_id);
	    break;
	case WARN:
    	    sprintf (prefix, "GRASS_INFO_WARNING(%d,%d): ", getpid(), message_id);
	    break;
	case ERR:
    	    sprintf (prefix, "GRASS_INFO_ERROR(%d,%d): ", getpid(), message_id);
	    break;
    }

    start = msg;

    fprintf(stderr, "\n" );
    while ( *start != '\0' ) {
	const char *next = start;

	fprintf ( fd, "%s", prefix);

	while ( *next != '\0' ) {
	    next++;
		
	    if ( *next == '\n' ) {
	        next++;
		break;
	    }
	}
	
	fwrite (start, 1, next - start, fd);
	fprintf (fd, "\n" );
	start = next;
    }
    fprintf(stderr, "GRASS_INFO_END(%d,%d)\n", getpid(), message_id );
    message_id++;
}

/*!
 * \fn int G_info_format ( void ) 
 *
 * \brief Get current message format
 * 
 * Maybe set to either "standard" or "gui" (normally GRASS takes care)
 *
 * \return grass_info_format value
*/
int G_info_format ( void ) 
{
    static int grass_info_format = -1;
    char    *fstr;
    
    if ( grass_info_format < 0) {
        fstr = getenv( "GRASS_MESSAGE_FORMAT" );

        if ( fstr && G_strcasecmp(fstr,"gui") == 0 )
	    grass_info_format = G_INFO_FORMAT_GUI;
	else if ( fstr && G_strcasecmp(fstr,"silent") == 0 )
	    grass_info_format = G_INFO_FORMAT_SILENT;
	else
	    grass_info_format = G_INFO_FORMAT_STANDARD;
    }

    return grass_info_format;
}

