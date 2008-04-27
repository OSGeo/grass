#include <grass/imagery.h>

FILE *I_fopen_sigset_file_new( char *group, char *subgroup, char *name)
{
    char element[GNAME_MAX*2];
    FILE *fd;

    sprintf (element, "subgroup/%s/sigset/%s", subgroup, name);

    fd = G_fopen_new_misc ("group", element, group);
    if (fd == NULL)
	G_warning ("unable to create signature file %s for subgroup %s of group %s",
		   name, subgroup, group);

    return fd;
}

FILE *I_fopen_sigset_file_old ( char *group, char *subgroup, char *name)
{
    char element[GNAME_MAX*2];
    FILE *fd;

    sprintf (element, "subgroup/%s/sigset/%s", subgroup, name);

    fd = G_fopen_old_misc ("group", element, group, G_mapset());
    if (fd == NULL)
	G_warning ("unable to open signature file %s for subgroup %s of group [%s in %s]",
		   name, subgroup, group, G_mapset());

    return fd;
}
