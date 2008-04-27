#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "global.h"
#include <grass/vask.h>

#define NFILES 15

int ask_files (char *groupname)
{
    char line[NFILES][75];
    char result[NFILES][15];
    char *err[NFILES];
    int repeat;
    int duplicate;
    int list;
    char *name, *mapset;
    int i,k,f1,f2,ln;
    int any;
    int *r;
    char **nm;

    r = ref_list;
    nm = new_name;

    repeat = 0;
    for (i=0; i < NFILES; i++)
	err[i] = "";
    f1 = 0;
    for (f2 = f1; f1 < group.group_ref.nfiles; f1 = f2)
    {
	any = 0;
	ln = 2;
	V_clear();
	V_line (0, "Please select the file(s) you wish to rectify by naming an output file");
	if (!repeat)
	    for (i=0; i < NFILES; i++)
		result[i][0] = 0;
	repeat = 0;
	for (i=0; f2 < group.group_ref.nfiles && i < NFILES; i++, f2++)
	{
	    name   = group.group_ref.file[f2].name;
	    mapset = group.group_ref.file[f2].mapset;
	    if (G_find_cell (name, mapset))
	    {
		sprintf (line[i], "%s in %s", name, mapset);
		dots (line[i], 36);
		V_line (ln, line[i]);
		V_ques (result[i], 's', ln, 37, 14);
		V_const (err[i], 's', ln, 53, 25);
		any = 1;
		ln++;
	    }
	}
	if (!any)
	    break;
	V_line (ln+2, "(enter list by any name to get a list of existing raster maps)");
	V_intrpt_ok();
	if(!V_call())
	    exit(0);

/* check the files for illegal names and duplicate names */

	list = 0;
	duplicate = 0;
	for (i=0; i < NFILES; i++)
	{
	    err[i] = "";
	    G_strip (result[i]);
	    if (result[i][0])
	    {
		if (strcmp (result[i], "list") == 0)
		{
		    list = 1;
		    result[i][0] = 0;
		}
		else if (G_legal_filename (result[i]) < 0)
		{
		    err[i] = "** illegal name **";
		    repeat = 1;
		}
		else
		{
		    for (k = 0; k < i; k++)
		    {
			if (!result[k][0])
			    continue;
			if(strcmp (result[k], result[i]) != 0)
			    continue;
			err[i] = "** duplicate name **";
			duplicate = 1;
			break;
		    }
		    if (duplicate)
			continue;
		    for (k=0; k < group.group_ref.nfiles; k++)
		    {
			if (ref_list[k] < 0)
			    continue;
			if(strcmp (new_name[k], result[i]) != 0)
			    continue;
			err[i] = "** duplicate name **";
			duplicate = 1;
			break;
		    }
		}
	    }
	}
	if (duplicate)
	    repeat = 1;

/* list the raster maps in the target location. must switch
 * environments to do this
 */
	if (list)
	{
	    repeat = 1;
	    select_target_env();
	    G_set_list_hit_return(1);
	    G_list_element ("cell", "raster", G_mapset(), (int(*)())0);
	    select_current_env();
	}

	if (repeat)
	{
	    f2 = f1;
	    continue;
	}
/* check for existing raster maps
 * this check must occur in the target location, so we switch
 * environments to be in the target location
 */
	select_target_env();
	for (i = 0; i < NFILES; i++)
	{
	    if (result[i][0] && G_find_cell (result[i], G_mapset()))
	    {
		if (!repeat++)
		{
		    repeat = 1;
		    fprintf (stderr, "\n");
		    fprintf (stderr, "** The following raster maps already exist in\n");
		    fprintf (stderr, "** LOCATION %s, MAPSET %s:\n\n", G_location(), G_mapset());
		}
		fprintf (stderr, "%-18s%s", result[i], repeat%4?" ":"\n");
		err[i] = "** file exists **";
	    }
	}
	select_current_env();
	if (repeat)
	{
	    repeat = !G_yes ("\n\nOk to overwrite? ", 0);
	}
	if (repeat)
	{
	    f2 = f1;
	    continue;
	}

	for (i=0; i < NFILES; i++)
	{
	    if (result[i][0])
	    {
		*r++ = f1+i;
		*nm++ = G_store (result[i]);
	    }
	}
    }
    for (k = 0; k < group.group_ref.nfiles; k++)
	if (ref_list[k] >= 0)
	    return 1;
    fprintf (stderr, "No files selected! Bye\n");
    G_sleep(3);
    exit(0);
}

int dots (char *buf, int n)
{
    int k;

    for (k=0; *buf; k++)
	buf++;
    if (k >= n)
	return 1;
    *buf++ = ' ';
    k++;
    while (k < n)
    {
	*buf++ = k%2 ? '.' : ' ';
	k++;
    }
    *buf = 0;

    return 0;
}
