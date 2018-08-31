
/***********************************************************
* I_get_to_eol (line,len,fd)
*
* Reads from fd until the newline, copying the first len-1
* characters into line.  The newline is not copied.
* len should be the length of line in bytes. This allows for
* a NULL to be added at the end.
***********************************************************/
#include <grass/imagery.h>
#include <stdio.h>
int I_get_to_eol(char *line, int len, FILE * fd)
{
    int c;
    int n;

    n = len - 1;
    while ((c = fgetc(fd)) >= 0 && c != '\n')
	if (n-- > 0)
	    *line++ = c;
    if (len > 0)
	*line = 0;
    return c == '\n';
}
