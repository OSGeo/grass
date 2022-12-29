#include <stdio.h>
#include <grass/imagery.h>


int I_get_group_title(const char *group, char *title, int n)
{
    FILE *fd;

    *title = 0;
    G_suppress_warnings(1);
    fd = I_fopen_group_file_old(group, "TITLE");
    G_suppress_warnings(0);
    if (fd != NULL) {
	G_getl2(title, n, fd);
	fclose(fd);
    }

    return (fd != NULL);
}


int I_put_group_title(const char *group, const char *title)
{
    FILE *fd;

    fd = I_fopen_group_file_new(group, "TITLE");
    if (fd != NULL) {
	fprintf(fd, "%s\n", title);
	fclose(fd);
    }

    return (fd != NULL);
}
