/*************************************************************
* I_list_groups (full)
* I_list_subgroups (group, full)
*************************************************************/
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <grass/imagery.h>
#include <grass/spawn.h>

static char *tempfile = NULL;


int I_list_groups (int full)
{
    char *element;
    int i;

    char buf[1024];
    char title[50];
    FILE *ls, *temp, *popen();
    struct Ref ref;
    int any;

    if (tempfile == NULL)
	tempfile = G_tempfile();

    element = "group";
    G__make_mapset_element (element);

    temp = fopen (tempfile, "w");
    if (temp == NULL)
	G_fatal_error ("can't open any temp files");
    fprintf (temp, "Available groups\n");
    fprintf (temp, "---------------------------------\n");

    any = 0;
    strcpy (buf, "cd ");
    G__file_name (buf+strlen(buf), element, "", G_mapset());
    strcat (buf, ";ls");
    if (!full) strcat (buf, " -C");
    if ((ls = popen (buf, "r")))
    {
	while (G_getl(buf, sizeof(buf), ls))
	{
	    any=1;
	    fprintf (temp, "%s", buf);
	    if (full)
	    {
		I_get_group_title (buf, title, sizeof(title));
		if (*title)
		    fprintf (temp, " (%s)", title);
		fprintf (temp, "\n");
		I_get_group_ref (buf, &ref);
		for (i = 0; i < ref.nfiles; i++)
		    fprintf (temp, "\t%s in %s\n", ref.file[i].name, ref.file[i].mapset);
		if (ref.nfiles <= 0)
		    fprintf (temp, "\t** empty **\n");
		I_free_group_ref (&ref);
	    }
	    else 
		fprintf (temp, "\n");
	}
	pclose (ls);
    }
    if (!any)
	fprintf (temp, "no group files available\n");
    fprintf (temp, "---------------------------------\n");
    fclose (temp);
    G_spawn(getenv("GRASS_PAGER"), getenv("GRASS_PAGER"), tempfile, NULL);
    remove ( tempfile );
    fprintf (stdout,"hit RETURN to continue -->");
    fflush(stdout);
    G_gets(buf);

    return 0;
}

int I_list_subgroups (char *group,int full)
{
    char element[100];
    int i;

    char buf[1024];
    FILE *ls, *temp, *popen();
    struct Ref ref;
    int any;

    if (tempfile == NULL)
	tempfile = G_tempfile();

    sprintf (element, "group/%s/subgroup", group);
    G__make_mapset_element (element);

    temp = fopen (tempfile, "w");
    if (temp == NULL)
	G_fatal_error ("Unable to open any temporary file");
    fprintf (temp, "Available subgroups in group %s\n", group);
    fprintf (temp, "---------------------------------\n");

    any = 0;
    strcpy (buf, "cd ");
    G__file_name (buf+strlen(buf), element, "", G_mapset());
    strcat (buf, ";ls");
    if (!full) strcat (buf, " -C");
    if ((ls = popen (buf, "r")))
    {
	while (G_getl(buf, sizeof(buf), ls))
	{
	    any=1;
	    fprintf (temp, "%s\n", buf);
	    if (full)
	    {
		I_get_subgroup_ref (group, buf, &ref);
		for (i = 0; i < ref.nfiles; i++)
		    fprintf (temp, "\t%s in %s\n", ref.file[i].name, ref.file[i].mapset);
		if (ref.nfiles <= 0)
		    fprintf (temp, "\t** empty **\n");
		I_free_group_ref (&ref);
	    }
	}
	pclose (ls);
    }
    if (!any)
	fprintf (temp, "no subgroup files available\n");
    fprintf (temp, "---------------------------------\n");
    fclose (temp);
    G_spawn(getenv("GRASS_PAGER"), getenv("GRASS_PAGER"), tempfile, NULL);
    remove ( tempfile );
    fprintf (stdout,"hit RETURN to continue -->");
    fflush(stdout);
    G_gets(buf);

    return 0;
}
