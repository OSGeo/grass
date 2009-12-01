#include <grass/glocale.h>

#include "cairodriver.h"

void cairo_read_xid(void)
{
#if CAIRO_HAS_XLIB_XRENDER_SURFACE
    FILE *fp;
    char buf[64];
    unsigned long xid;

    fp = fopen(ca.file_name, "r");
    if (!fp)
	G_fatal_error(_("Unable to open input file <%s>"), ca.file_name);

    if (!fgets(buf, sizeof(buf), fp))
	G_fatal_error(_("Unable to read input file <%s>"), ca.file_name);

    if (sscanf(buf, "%lx", &xid) != 1)
	G_fatal_error(_("Unable to parse input file <%s>"), ca.file_name);

    fclose(fp);

    ca.win = (Drawable) xid;
#endif
}

