#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <grass/config.h>
#include <grass/gis.h>

/*******************************************************************
 * G_putenv (name, value)
 *   const char *name, *value
 *
 * this routine sets the UNIX environment variable name to value
 ******************************************************************/

#if !defined(HAVE_PUTENV) && !defined(HAVE_SETENV)
extern char **environ;
#endif

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
