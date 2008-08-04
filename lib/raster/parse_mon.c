/* parse_mon - parse monitorcap entry */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include <grass/monitors.h>

static FILE *monitors = NULL;
static struct MON_CAP cap;

static char *substr(char *, char *);
static int read_line(FILE *, char *, int);

struct MON_CAP *R_parse_monitorcap(int field, char *key)
{
    int rewound;
    char line[1024];
    char *p;
    char file[500];
    char *gisbase;

    gisbase = G_gisbase();

    rewound = 0;
    if (!(field == MON_NEXT || field == MON_NAME ||
	  field == MON_PATH || field == MON_LINK || field == MON_CLOSE))
	return (NULL);
    if (monitors == NULL) {
	sprintf(file, "%s/etc/monitorcap", gisbase);
	if ((monitors = fopen(file, "r")) == NULL)
	    G_fatal_error("Unable to open %s", file);
    }
    else {
	if (field == MON_CLOSE) {
	    fclose(monitors);
	    monitors = NULL;
	    return (NULL);
	}
    }
    while (-1) {
	if (read_line(monitors, line, sizeof line)) {
	    if (field == MON_NEXT)
		return (NULL);
	    rewind(monitors);
	    if (read_line(monitors, line, sizeof line) || rewound)
		return (NULL);
	    rewound = -1;
	}
	cap.path = cap.comment = cap.link = cap.tty = cap.where = NULL;
	if ((cap.name = G_malloc(strlen(line) + 1)) == NULL)
	    return (NULL);
	strcpy(cap.name, line);
	if ((p = substr(":", cap.name)) != NULL) {
	    *p++ = '\0';
	    cap.path = p;
	    if ((p = substr(":", p)) != NULL) {
		*p++ = '\0';
		cap.comment = p;
		if ((p = substr(":", p)) != NULL) {
		    *p++ = '\0';
		    cap.link = p;
		    if ((p = substr(":", p)) != NULL) {
			*p++ = '\0';
			cap.tty = p;
			if ((p = substr(":", p)) != NULL) {
			    *p++ = 0;
			    cap.where = p;
			    if ((p = substr("\n", p)) != NULL)
				*p = '\0';
			}
		    }
		}
	    }
	}
	if (cap.path == NULL || cap.link == NULL || cap.where == NULL ||
	    cap.tty == NULL || cap.comment == NULL)
	    G_free(cap.name);
	else {
	    sprintf(line, "%s/%s", gisbase, cap.path);
	    cap.path = G_store(line);

	    if (field == MON_NEXT ||
		(field == MON_NAME && !strcmp(key, cap.name))
		|| (field == MON_PATH && !strcmp(key, cap.path))
		|| (field == MON_LINK && !strcmp(key, cap.link)))
		return (&cap);
	    else
		G_free(cap.name);
	}
    }
}

/* read_line - read a line, possibly continued with a "\" into a buffer */

static int read_line(FILE * file,	/* file from which to read */
		     char *line,	/* buffer in which to put it */
		     int size)
{				/* size of buffer */
    int length, full_line, eof, done;
    char c, last_c;

    *line = '\0';
    for (length = full_line = eof = 0; !full_line && !eof;) {	/*   one entire line at a time */
	while (1) {
	    eof = (fgets(line + length, size - length - 1, file) == NULL);
	    if (eof)
		break;
	    if (*(line + length) != '#')
		break;
	}
	length = strlen(line) - 1;
	if (*(line + length) == '\n') {
	    if (*(line + length - 1) == '\\')
		--length;
	    else
		full_line = -1;
	}
	else {
	    if (length != -1) {
		fprintf(stderr, "error:  input line too long\n");
		full_line = -1;
		last_c = c = ' ';
		for (done = 0; !done;) {
		    if ((c = getc(file)) != EOF) {
			if (c == '\n' && last_c != '\\')
			    done = -1;
			else
			    last_c = c;
		    }
		    else
			eof = done = -1;
		}
	    }
	}
    }
    return (eof);
}

/* substr - find substring in a string.  returns pointer to start of */
/* substring or NULL if not found */

static char *substr(char *string, char *buffer)
{
    int start, i, found;
    char c;

    start = i = found = 0;
    while ((c = *(buffer + start + i)) != '\0' && !found) {
	if (c == *(string + i)) {
	    if (*(string + ++i) == '\0')
		found = -1;
	}
	else {
	    start++;
	    i = 0;
	}
    }
    if (found)
	return (buffer + start);
    else
	return (NULL);
}
