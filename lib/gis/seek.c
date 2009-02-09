
#include <grass/config.h>
#include <stdio.h>
#include <sys/types.h>
#include <grass/gis.h>
#include <grass/glocale.h>

off_t G_ftell(FILE *fp)
{
#ifdef HAVE_LARGEFILES
    return ftello(fp);
#else
    return (off_t) ftell(fp);
#endif     
}

void G_fseek(FILE *fp, off_t offset, int whence)
{
#ifdef HAVE_LARGEFILES
    if (fseeko(fp, offset, whence) != 0)
	G_fatal_error(_("unable to seek"));
#else
    long loff = (long) offset;
    if ((off_t) loff != offset)
	G_fatal_error(_("seek offset out of range"));
    if (fseek(fp, loff, whence) != 0)
	G_fatal_error(_("unable to seek"));
#endif     
}

