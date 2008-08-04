#include <unistd.h>
#include <grass/gis.h>
/* this routine returns a name for the machine
 * it returns the empty string, if this info
 * not available (it never returns a NULL pointer)
 *
 * the name is stored in a static array and the pointer to this
 * array is returned.
 *
 * the contents of this array are reset upon each call
 *
 */

#include <grass/config.h>

#ifndef HAVE_GETHOSTNAME
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
static struct utsname attname;
#endif
#endif

char *G__machine_name(void)
{
    static char name[128];

    *name = 0;

#ifdef HAVE_GETHOSTNAME
    gethostname(name, sizeof(name));
    name[sizeof(name) - 1] = 0;	/* make sure null terminated */
#else
#ifdef HAVE_SYS_UTSNAME_H
    uname(&attname);
    strcpy(name, attname.nodename);
#endif
#endif

    return (name);
}
