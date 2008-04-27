#include <unistd.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/spawn.h>


/* 
 * run_etc_support() - Run command in etc/support 
 *
 * RETURN: >0 success : 0 failure
 */
int run_etc_support(char *pgm, char *rast)
{
    char path[GPATH_MAX];
    int stat;

    sprintf(path, "%s/etc/%s", G_gisbase(), pgm);

    if ((stat = G_spawn(path, pgm, rast, NULL)))
	G_sleep(3);

    return stat;
}


/* 
 * run_system() - Run system command 
 *
 * RETURN: >0 success : 0 failure
 */
int run_system(char *pgm)
{
    int stat;

    if ((stat = G_spawn(pgm, pgm, NULL)))
	G_sleep(3);

    return stat;
}
