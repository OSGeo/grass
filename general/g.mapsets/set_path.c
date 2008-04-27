#include <string.h>
#include <stdlib.h>
#include "externs.h"
#include <grass/gis.h>

int set_mapset_path (void)
{
    char command[4096];
    int n;
    int i;
    int skip;
    int any;
    char *cur_mapset;

    if (nchoices == 0)  /* they didn't choose any, exit w/out change */
        return 0;

    cur_mapset = G_mapset();

/* build first part of command string */
    strcpy (command, "g.mapsets -p mapset=");
    any = 0;

/*
 * make sure current mapset is specified in the list
 * if not add it to the head of the list
 */

    skip = 0;
    for (n = 0; n < nchoices; n++)
        if (strcmp (cur_mapset, mapset_name[choice[n]]) == 0)
        {
            skip = 1;
            break;
        }
    if (!skip)
    {
	if (any++)
	    strcat(command, ",");
        strcat (command, cur_mapset);
    }

/*
 * output the list, removing duplicates
 */
    for (n = 0; n < nchoices; n++)
    {
        skip = 0;
        for (i = 0; i < n; i++)
            if (strcmp (mapset_name[choice[i]], mapset_name[choice[n]]) == 0)
            {
                skip = 1;
                break;
            }
        if (!skip)
        {
	    if (any++)
		strcat(command, ",");
            strcat (command, mapset_name[choice[n]]);
        }
    }

    fprintf (stdout,"\nMapset search list set to\n ");
    fflush (stdout);
    if (system (command) == 0)
    {
        return (0);
    }
    else
    {
        G_warning ("call to g.mapsets failed");
        return (-1);
    }
}
