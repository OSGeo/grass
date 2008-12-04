#include <unistd.h>
#include <grass/gis.h>
#include <grass/config.h>
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

/* this routine returns a name for the machine
 * it returns the empty string, if this info
 * not available (it never returns a NULL pointer)
 *
 * the name is stored in a static array and the pointer to this
 * array is returned.
 *
 */

const char *G__machine_name(void)
{
    static int initialized;
    static char name[128];

    if (G_is_initialized(&initialized))
	return name;

#if defined(HAVE_GETHOSTNAME)
    gethostname(name, sizeof(name));
    name[sizeof(name) - 1] = 0;	/* make sure NUL terminated */
#elif defined(HAVE_SYS_UTSNAME_H)
    {
	struct utsname attname;
	uname(&attname);
	strcpy(name, attname.nodename);
    }
#else
    strcpy(name, "unknown");
#endif

    G_initialize_done(&initialized);
    return name;
}
