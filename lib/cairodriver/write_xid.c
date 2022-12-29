
#include <grass/glocale.h>

#include "cairodriver.h"

void cairo_write_xid(void)
{
#if CAIRO_HAS_XLIB_XRENDER_SURFACE
    FILE *fp;
    char buf[64];

    fp = fopen(ca.file_name, "w");
    if (!fp)
	G_fatal_error(_("Unable to open output file <%s>"), ca.file_name);

    sprintf(buf, "0x%08lx\n", (unsigned long) ca.win);

    if (fputs(buf, fp) < 0)
	G_fatal_error(_("Unable to write output file <%s>"), ca.file_name);

    fclose(fp);
#endif
}

