#include <unistd.h>
#include "orthophoto.h"

int run_etc_imagery(char *pgm, char *group)
{
    char buf[1024];
    int stat;

    /* Eventually this programs should live in $GIS/etc/imagery */
    /* sprintf (buf, "%s/etc/imagery/%s  %s", G_gisbase(), pgm, group); */

    /* but for now they live in etc/  */
    sprintf(buf, "%s/etc/%s  %s", G_gisbase(), pgm, group);

    if (stat = G_system(buf))
	G_sleep(3);
    return 0;
}

int run_system(char *pgm)
{
    char buf[1024];
    int stat;

    sprintf(buf, "%s", pgm);
    if (stat = G_system(buf))
	G_sleep(3);
    return ((int)stat);
}
