
/**
 * \file asprintf.c
 *
 * \brief GIS Library - GRASS implementation of asprintf().
 *
 * Eric G. Miller - Thu, 2 May 2002 17:51:54 -0700
 *
 * I've got a sort of cheat for asprintf. We can't use vsnprintf for the
 * same reason we can't use snprintf ;-)  Comments welcome.
 *
 * We cheat by printing to a tempfile via vfprintf() and then reading it
 * back in. Probably not the most efficient way.
 *
 * <b>WARNING:</b> Temporarily, the G_asprintf macro cannot be used. See 
 * explanation in gisdefs.h.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Eric Miller - egm2 at jps net
 *
 * \date 2002-2008
 */

#define _GNU_SOURCE		/* enable asprintf */
#include <grass/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <assert.h>
#include <grass/gis.h>

#ifdef __MINGW32__
#include <windows.h>
#endif /* __MINGW32__ */


#ifndef G_asprintf

/**
 * \brief Safe replacement for <i>asprintf()</i>.
 *
 * Allocate a string large enough to hold the new output, including the 
 * terminating NULL, and returns a pointer to the first parameter. The 
 * pointer should be passed to <i>G_free()</i> to release the allocated 
 * storage when it is no longer needed.
 *
 * \param[out] out
 * \param[in] fmt
 * \return number of bytes written
 */

#ifdef HAVE_ASPRINTF

int G_vasprintf(char **out, const char *fmt, va_list ap)
{
    return vasprintf(out, fmt, ap);
}

#else

int G_vasprintf(char **out, const char *fmt, va_list ap)
{
    int ret_status = EOF;
    char dir_name[2001];
    char file_name[2000];
    FILE *fp = NULL;
    char *work = NULL;

    assert(out != NULL && fmt != NULL);

    /* Warning: tmpfile() does not work well on Windows (MinGW)
     *          if user does not have write access on the drive where 
     *          working dir is? */
#ifdef __MINGW32__
    /* file_name = G_tempfile(); */
    GetTempPath(2000, dir_name);
    GetTempFileName(dir_name, "asprintf", 0, file_name);
    fp = fopen(file_name, "w+");
#else
    fp = tmpfile();
#endif /* __MINGW32__ */

    if (fp) {
	int count;

	count = vfprintf(fp, fmt, ap);
	if (count >= 0) {
	    work = G_calloc(count + 1, sizeof(char));
	    if (work != NULL) {
		rewind(fp);
		ret_status = fread(work, sizeof(char), count, fp);
		if (ret_status != count) {
		    ret_status = EOF;
		    G_free(work);
		    work = NULL;
		}
	    }
	}
	fclose(fp);
#ifdef __MINGW32__
	unlink(file_name);
#endif /* __MINGW32__ */
    }
    *out = work;

    return ret_status;
}

#endif /* HAVE_ASPRINTF */

int G_asprintf(char **out, const char *fmt, ...)
{
    va_list ap;
    int count;

    va_start(ap, fmt);
    count = G_vasprintf(out, fmt, ap);
    va_end(ap);

    return count;
}

#endif /* G_asprintf */
