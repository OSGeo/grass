#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#ifndef __MINGW32__
#  include <pwd.h>
#endif

int can_make_location(char *gisdbase, char *location)
{
    struct stat s;
    struct passwd *pwd;

    /* make sure this is a directory */
    if (stat(gisdbase, &s) != 0) {
	fprintf(stderr, "\n** %s not found **\n", gisdbase);
	return 0;
    }
    if (!(s.st_mode & S_IFDIR)) {
	fprintf(stderr, "\n** %s is not a directory **\n", gisdbase);
	return 0;
    }

    /* look for write permission */
    if (access(gisdbase, 2) == 0)
	return 1;

    fprintf(stderr, "\nNote\n");
    fprintf(stderr,
	    " You don't have permission under %s to create a new location\n",
	    gisdbase);
#ifndef __MINGW32__
    if ((pwd = getpwuid(s.st_uid)))
	fprintf(stderr, " See user %s about creating location %s\n",
		pwd->pw_name, location);
#endif
    return 0;
}
