#include <string.h>
#include <grass/gis.h>
#include <stdio.h>
#include <stdlib.h>

/******************************************
 * $GISBASE/etc/echo [-n] [-e] args
 *
 * echos its args to stdout
 * suppressing the newline if -n specified
 * prints to stderr instead if -e specified
 *
 * replaces the standard UNIX echo which
 * varies from machine to machine
 *******************************************/

int main(int argc, char *argv[])
{
    int i;
    int newline;
    int any;
    FILE *stream = stdout;

    newline = 1;
    any = 0;

    for (i = 1; i < argc; i++)
	if (strcmp(argv[i], "-n") == 0)
	    newline = 0;
	else if (strcmp(argv[i], "-e") == 0)
	    stream = stderr;
	else
	    fprintf(stream, "%s%s", any++ ? " " : "", argv[i]);
    if (any && newline)
	fprintf(stream, "\n");

    exit(0);
}
