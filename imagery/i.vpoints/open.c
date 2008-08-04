#include <grass/gis.h>

/* open and close vector map for level one access */

FILE *open_vect(char *name, char *mapset)
{
    FILE *fd;
    char msg[128];

    fd = G_fopen_vector_old(name, mapset);
    if (fd == NULL) {
	sprintf(msg, "can't open vector map [%s]", name);
	G_fatal_error(msg);
    }
    if (dig_init(fd) < 0) {
	sprintf(msg, "can't initialize vector map [%s]", name);
	G_fatal_error(msg);
    }
    return fd;
}

int close_vect(FILE * fd)
{
    dig_fini(fd);
    fclose(fd);
}
