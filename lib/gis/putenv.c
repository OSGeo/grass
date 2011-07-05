/*!
  \file lib/gis/putenv.c

  \brief GIS library - environment routines
  
  (C) 2001-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
  \author Updated for GRASS7 by Glynn Clements
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <grass/config.h>
#include <grass/gis.h>

#if !defined(HAVE_PUTENV) && !defined(HAVE_SETENV)
extern char **environ;
#endif

/*!
  \brief Sets the UNIX environment variable name to value
  
  \param name env name
  \param value env value
*/
void G_putenv(const char *name, const char *value)
{
    char buf[1024];

#if defined(HAVE_PUTENV)
    sprintf(buf, "%s=%s", name, value);
    putenv(G_store(buf));
#elif defined(HAVE_SETENV)
    setenv(name, value, 1);
#else
    static int first = 1;
    int i;
    char **newenv;
    char *env;

    if (first) {
	for (i = 0; environ[i]; i++) ;
	newenv = (char **)G_malloc((i + 1) * sizeof(char *));
	for (i = 0; env = environ[i], env; i++)
	    newenv[i] = G_store(env);
	newenv[i] = NULL;
	environ = newenv;
	first = 0;
    }

    for (i = 0; env = environ[i], env; i++) {
	char temp[4];

	if (sscanf(env, "%[^=]=%1s", buf, temp) < 1)
	    continue;

	if (strcmp(buf, name) != 0)
	    continue;

	G_free(env);
	sprintf(buf, "%s=%s", name, value);
	environ[i] = G_store(buf);

	return;
    }
    environ = (char **)G_realloc(environ, (i + 2) * sizeof(char *));
    sprintf(buf, "%s=%s", name, value);
    environ[i++] = G_store(buf);
    environ[i] = NULL;
#endif
}
