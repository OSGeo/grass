#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

int graphics(FILE * infile)
{
    char buff[128 + 1];
    int got_new;

    got_new = G_getl2(buff, 128, infile);
    G_strip(buff);

    while (got_new) {
	switch (*buff & 0177) {
	case 't':
	    do_text(buff);
	    got_new = G_getl2(buff, 128, infile);
	    break;
	case 's':
	    if (strncmp(buff, "sym", 3) == 0)
		do_symbol(buff);
	    else
		do_size(buff);
	    got_new = G_getl2(buff, 128, infile);
	    break;
	case 'r':
	    do_rotate(buff);
	    got_new = G_getl2(buff, 128, infile);
	    break;
	case 'p':
	    got_new = do_poly(buff, infile);
	    break;
	case 'c':
	    do_color(buff);
	    got_new = G_getl2(buff, 128, infile);
	    break;
	case 'm':
	    do_move(buff);
	    got_new = G_getl2(buff, 128, infile);
	    break;
	case 'd':
	    do_draw(buff);
	    got_new = G_getl2(buff, 128, infile);
	    break;
	case 'w':
	    do_linewidth(buff);
	    got_new = G_getl2(buff, 128, infile);
	    break;
	case 'i':
	    do_icon(buff);
	    got_new = G_getl2(buff, 128, infile);
	    break;
	case '#':
	case 0:		/* skip blank lines */
	    got_new = G_getl2(buff, 128, infile);
	    break;
	default:
	    G_warning(_("Problem parsing command [%s]"), buff);
	    got_new = G_getl2(buff, 128, infile);
	    break;
	}
	G_strip(buff);
    }

    return 0;
}
