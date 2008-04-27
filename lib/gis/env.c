/**
   \file env.c

   \brief GIS library - environment routines
   
   (C) 2001-2008 by the GRASS Development Team
   
   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.
   
   \author Original author CERL
*/

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>   /* for sleep() */
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#define ENV struct env

ENV
{
    int loc;
    char *name;
    char *value;
} ;

static ENV *env = NULL;
static ENV *env2 = NULL;
static int count = 0;
static int count2 = 0;
static int init[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static char *gisrc = NULL;
static int varmode = G_GISRC_MODE_FILE; /* where find/store variables */ 

static int read_env( int );
static int set_env ( const char *, const char *, int);
static int unset_env ( const char *, int);
static char *get_env( const char *, int);
static int write_env ( int );
static FILE *open_env ( const char *, int);

/**
   \brief Set where to find/store variables

   Modes:
    - G_GISRC_MODE_FILE
    - G_GISRC_MODE_MEMORY 

   \param mode mode to find/store variables (G_GISRC_MODE_FILE by default)

   \return
*/
void G_set_gisrc_mode ( int mode )
{
    varmode = mode; 
}

/**
   \brief Get info where variables are stored

   \param

   \return mode
*/
int G_get_gisrc_mode ( void )
{
    return ( varmode ); 
}

static int 
read_env ( int loc )
{
    char buf[200];
    char *name;
    char *value;

    FILE *fd;

    if ( loc == G_VAR_GISRC && varmode == G_GISRC_MODE_MEMORY ) return 0; /* don't use file for GISRC */
    
    if (init[loc])
	return 1;

    init[loc] = 1;

    if ((fd = open_env ("r", loc)))
    {
	while (G_getl2 (buf, sizeof buf, fd))
	{
	    for (name = value = buf; *value; value++)
		if (*value == ':')
		    break;
	    if (*value == 0)
		continue;

	    *value++ = 0;
	    G_strip (name);
	    G_strip (value);
	    if (*name && *value)
		set_env (name, value, loc);
	}
	fclose (fd);
    }

    return 0;
}

static int set_env ( const char *name, const char *value, int loc)
{
    int n;
    int empty;
    char *tv;

/* if value is NULL or empty string, convert into an unsetenv() */
    if(!value || !strlen(value))
    {
	unset_env (name, loc);
	return 0;
    }

    tv = G_store (value);
    G_strip (tv);
    if (*tv == 0)
    {
	G_free (tv);
	unset_env (name, loc);
	return 1;
    }

/*
* search the array
*   keep track of first empty slot
*   and look for name in the environment
*/
    empty = -1;
    for (n = 0; n < count; n++)
	if (!env[n].name)	/* mark empty slot found */
	    empty = n;
	else if (strcmp (env[n].name, name) == 0 && env[n].loc == loc)
	{
	    env[n].value = tv;
	    return 1;
	}

/* add name to env: to empty slot if any */
    if (empty >= 0)
    {
	env[empty].loc = loc;
	env[empty].name = G_store (name);
	env[empty].value = tv;
	return 0;
    }

/* must increase the env list and add in */
    if ((n = count++))
	env = (ENV *) G_realloc ((char *) env, count * sizeof (ENV));
    else
	env = (ENV *) G_malloc (sizeof (ENV));

    env[n].loc = loc;
    env[n].name = G_store (name);
    env[n].value = tv;

    return 0;
}

static int unset_env (const char *name, int loc)
{
    int n;

    for (n = 0; n < count; n++)
	if (env[n].name && (strcmp(env[n].name, name)==0) && env[n].loc == loc )
	{
	    G_free (env[n].name);
	    env[n].name = 0;
	    return 1;
	}

    return 0;
}

static char *get_env( const char *name, int loc)
{
    int n;

    for (n = 0; n < count; n++) {
	if (env[n].name && (strcmp(env[n].name, name)==0) && env[n].loc == loc)
	    return env[n].value;
    }

    return NULL;
}

static int write_env ( int loc )
{
    FILE *fd;
    int n;
    char dummy[2];
    void (*sigint)()
#ifdef SIGQUIT
        , (*sigquit)()
#endif
;

    if ( loc == G_VAR_GISRC && varmode == G_GISRC_MODE_MEMORY ) return 0; /* don't use file for GISRC */

/*
 * THIS CODE NEEDS TO BE PROTECTED FROM INTERRUPTS
 * If interrupted, it can wipe out the GISRC file
 */
    sigint  = signal (SIGINT,  SIG_IGN);
#ifdef SIGQUIT
    sigquit = signal (SIGQUIT, SIG_IGN);
#endif
    if((fd = open_env ("w", loc)))
    {
	for (n = 0; n < count; n++)
	    if (env[n].name && env[n].value && env[n].loc == loc
	    && (sscanf (env[n].value,"%1s", dummy) == 1))
		fprintf(fd,"%s: %s\n", env[n].name, env[n].value);
	fclose (fd);
    }

    signal (SIGINT,  sigint);
#ifdef SIGQUIT
    signal (SIGQUIT, sigquit);
#endif

    return 0;
}

static FILE *open_env ( const char *mode, int loc)
{
    char buf[1000];
    
    if ( loc == G_VAR_GISRC ) {
	if (!gisrc)
	    gisrc = getenv ("GISRC");

	if (!gisrc)
	{
	    G_fatal_error(_("GISRC - variable not set"));
	    return(NULL);
	}
	strcpy (buf, gisrc);
    } else if ( loc == G_VAR_MAPSET ) {
	/* Warning: G_VAR_GISRC must be previously read -> */
	/* TODO: better place ? */
	read_env ( G_VAR_GISRC );  

	sprintf ( buf, "%s/%s/VAR", G_location_path(), G_mapset() );
    }
		
    return fopen (buf, mode);
}

/**
   \brief Get enviroment variable

   Calls G_fatal_error() if name not set.

   \param name variable name

   \return char pointer to value for name
*/ 
char *G_getenv( const char *name)
{
    char *value;

    if ((value = G__getenv(name)))
	return value;

    G_fatal_error(_("G_getenv(): Variable %s not set"), name);
    return NULL;
}

/**
   \brief Read variable from specific place

   Locations:
    - G_VAR_GISRC
    - G_VAR_MAPSET

   G_fatal_error() is called when variable is not found.

   \param name variable name
   \param loc location id

   \return variable value
   \return NULL if not found
*/
char *G_getenv2( const char *name, int loc )
{
    char *value;

    if ((value = G__getenv2(name, loc)))
	return value;

    G_fatal_error(_("%s not set"), name);
    return NULL;
}

/**
   \brief Get environment variable

   \param name variable name
   
   \return char pointer to value for name
   \return NULL if name not set
*/
char *G__getenv ( const char *name)
{
    if (strcmp (name, "GISBASE") == 0)
       return getenv (name);

    read_env(G_VAR_GISRC);

    return get_env (name, G_VAR_GISRC);
}

/**
   \brief Get environment variable from specific place

   \param name variable name
   \param loc location id
   
   \return char pointer to value for name
   \return NULL if name not set
*/
char *G__getenv2 ( const char *name, int loc)
{
    if (strcmp (name, "GISBASE") == 0)
	return getenv (name);

    read_env( loc );

    return get_env (name, loc);
}

/**
   \brief Set environment variable

   If value is NULL, becomes an G_unsetenv().
   Updates .gisrc

   \param name variable name
   \param value variable value

   \return 0
*/
int G_setenv (const char *name, const char *value)
{
    read_env(G_VAR_GISRC);
    set_env (name, value, G_VAR_GISRC );
    write_env(G_VAR_GISRC);
    return 0;
}

/**
   \brief Set environment variable from specific place

   If value is NULL, becomes an G_unsetenv().
   Updates .gisrc

   \param name variable name
   \param value variable value
   \param loc location id

   \return 0
*/
int G_setenv2 (const char *name, const char *value, int loc)
{
    read_env(loc);
    set_env (name, value, loc);
    write_env(loc);
    return 0;
}

/**
   \brief Set environment name to value

   \param name variable name
   \param value variable value

   \return 0
*/
int G__setenv ( const char *name, const char *value)
{
    read_env(G_VAR_GISRC);
    set_env (name, value, G_VAR_GISRC );
    return 0;
}

/**
   \brief Set environment name to value from specific place

   \param name variable name
   \param value variable value
   \param loc location id

   \return 0
*/
int G__setenv2 (const char *name, const char *value, int loc)
{
    read_env(loc);
    set_env (name, value, loc);
    return 0;
}

/**
   \brief Remove name from environment

   Updates .gisrc

   \param name variable name

   \return 0
*/
int G_unsetenv ( const char *name)
{
    read_env(G_VAR_GISRC);
    unset_env (name, G_VAR_GISRC);
    write_env(G_VAR_GISRC);

    return 0;
}

/**
   \brief Remove name from environment from specific place

   Updates .gisrc

   \param name variable name

   \return 0
*/
int G_unsetenv2 ( const char *name, int loc)
{
    read_env(loc);
    unset_env (name, loc);
    write_env(loc);

    return 0;
}

/**
   \brief Writes current environment to .gisrc

   \param

   \return 0
*/
int 
G__write_env (void)
{
    if (init[G_VAR_GISRC])
	write_env(G_VAR_GISRC);

    return 0;
}

/**
   \brief Get variable name for index n.

   For example:
   \code
   for (n = 0; ; n++)
      if ((name = G__env_name(n)) == NULL)
        break;
   \endcode

   \param n index of variable

   \return pointer to variable name
*/
char *G__env_name (int n)
{
    int i;

    read_env(G_VAR_GISRC);
    if (n >= 0)
	for (i = 0; i < count; i++)
	    if (env[i].name && *env[i].name && (n-- == 0))
		return env[i].name;
    return NULL;
}

/**
   \brief Initialize init array for G_VAR_GISRC.

   \param

   \return 0
*/
int G__read_env (void)
{
    init[G_VAR_GISRC] = 0;

    return 0;
}

/**
   \brief Sets filename for gisrc

   \param name filename

   \return 0
*/
int G__set_gisrc_file( const char *name)
{
    gisrc = NULL;
    if (name && *name)
	gisrc = G_store(name);

    return 0;
}

/**
   \brief Get gisrc filename

   \param
   
   \return char pointer to filename
*/
char *G__get_gisrc_file (void)
{
    return gisrc;
}

/**
   \brief Set up alternative environment variables

   \param

   \return 0
*/
int G__create_alt_env (void)
{
    int i;

    /* copy env to env2 */
    env2 = env;
    count2 = count;
    env = NULL;
    count = 0;

    for (i=0; i < count2; i++)
	if (env2[count].name)
	    set_env (env2[count].name, env2[count].value, G_VAR_GISRC);

    return 0;
}

/**
   \brief Switch environments

   \param

   \return 0
*/
int G__switch_env (void)
{
    ENV *tmp;
    int n;

    n   = count;
    tmp = env;

    env   = env2;
    count = count2;

    env2   = tmp;
    count2 = n;

    return 0;
}
