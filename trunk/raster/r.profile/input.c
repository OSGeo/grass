#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <grass/glocale.h>

#include "local_proto.h"

int input(char *blank1, char *word1, char *blank2, char *word2, char *rest,
	  FILE *infile)
{
    char buf[1024];
    char *b, *w1, *w2;
    int string_size;

    if (isatty(0))
	fprintf(stderr, "> ");

    *blank1 = *blank2 = 0;
    *word1 = *word2 = *rest = 0;

    if (!fgets(buf, 1024, infile)) {
	*buf = 0;
	return 0;
    }

    /* note ebuf and nbuf in main.c (w1 and w2 here) are only 256 chars, we
       check here to make sure we don't move the pointer past sizeof(buf) */
    if (strlen(buf) >= 1023)
	G_fatal_error(_("One coordinate pair per line, please"));

    b = buf;
    w1 = word1;
    w2 = word2;

    while (*b == ' ' || *b == '\t' || *b == ',')
	*blank1++ = *b++;
    *blank1 = 0;

    while (*b != '\n' && *b != ' ' && *b != '\t' && *b != ',')
	*word1++ = *b++;
    *word1 = 0;

    string_size = strlen(w1);
    G_debug(5, "strlen w1=%d  [%s]", string_size, w1);
    if (string_size > 255)
	G_fatal_error(_("One coordinate pair per line, please"));

    while (*b == ' ' || *b == '\t' || *b == ',')
	*blank2++ = *b++;
    *blank2 = 0;

    while (*b != '\n' && *b != ' ' && *b != '\t' && *b != ',')
	*word2++ = *b++;
    *word2 = 0;

    string_size = strlen(w2);
    G_debug(5, "strlen w2=%d  [%s]", string_size, w2);
    if (string_size > 255)
	G_fatal_error(_("One coordinate pair per line, please"));

    /* bug? really = and not ==? */
    /* not a bug: we are filling "rest" with the remaining fgets buffer. 
       This goes unused though so could be ripped out if a worry */
    while ((*rest++ = *b++)) ;

    if (isatty(0) && strcmp("end", w1) == 0 && *w2 == 0)
	return 0;

    return 1;
}
