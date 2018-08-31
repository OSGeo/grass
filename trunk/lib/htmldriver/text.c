
/* Text.c - save text string into last_text buffer */

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include "driverlib.h"
#include "htmlmap.h"

void HTML_Text(const char *text)
{
    int len = strlen(text);
    const char *s;
    char *d;

    if (len > html.last_text_len) {
	G_free(html.last_text);
	html.last_text = (char *)G_malloc(len + 1);
	html.last_text_len = len;
    }

    /* copy string to last_text area, make sure we don't copy \n */
    for (d = html.last_text, s = text; *s != '\0'; s++) {
	if (*s != '\n') {
	    *d = *s;
	    d++;
	}
    }
    *d = '\0';
}
