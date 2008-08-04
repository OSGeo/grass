
/****************************************************************************
 *
 * MODULE:       grocat
 * AUTHOR(S):    Paul Kelly
 * PURPOSE:      Copies stdin to stdout in line-buffered mode until end
 *               of file is received.
 *               Used with Tcl/Tk gronsole system to merge stdout and
 *               stderr streams to be caught by Tcl "open" command.
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int inchar, outchar;
    char inbuff[1024], outbuff[1024];

    /* stdin and stdout both line-buffered */
    if (setvbuf(stdin, inbuff, _IOLBF, sizeof(inbuff))) {
	fprintf(stderr, "grocat: Can't set stdin to line-buffered mode!\n");
	exit(EXIT_FAILURE);
    }
    if (setvbuf(stdout, outbuff, _IOLBF, sizeof(outbuff))) {
	fprintf(stderr, "grocat: Can't set stdout to line-buffered mode!\n");
	exit(EXIT_FAILURE);
    }

    while ((inchar = getc(stdin)) != EOF) {
	/* Read a character at a time from stdin until EOF
	 * and copy to stdout */
	outchar = putc(inchar, stdout);
	if (outchar != inchar) {
	    fprintf(stderr, "grocat: Error writing to stdout!\n");
	    exit(EXIT_FAILURE);
	}
    }

    /* Flush in case last line wasn't terminated properly or something */
    fflush(stdout);

    exit(EXIT_SUCCESS);
}
