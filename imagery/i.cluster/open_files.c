#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "global.h"


int 
open_files (void) 
{
    char *name, *mapset;
    FILE *fd;
    int n, missing;

    I_init_group_ref (&ref);

    G_strip (group);
    if (!I_find_group (group))
      G_fatal_error(_("\n group <%s> not found"), group);

    G_strip (subgroup);
    if (!I_find_subgroup (group, subgroup))
      G_fatal_error(_("subgroup <%s> not found"), subgroup);

    I_free_group_ref (&ref);
    I_get_subgroup_ref (group, subgroup, &ref);

    missing = 0;
    for (n = 0; n < ref.nfiles; n++)
    {
	name = ref.file[n].name;
	mapset = ref.file[n].mapset;
	if (G_find_cell (name, mapset) == NULL)
	{
	    missing = 1;
	    G_warning(_("Raster map <%s> do not exists in subgroup <%s>"),
		      G_fully_qualified_name(name, mapset), subgroup);
	}
    }
    if (missing) exit(1);
    if (ref.nfiles <= 1)
    {
	if (ref.nfiles <= 0)
	    G_warning(_("Subgroup [%s] doesn't have any files"), subgroup);
	else
	    G_warning(_("Subgroup [%s] only has 1 file"), subgroup);
	G_fatal_error(_("The subgroup must have at least 2 files to run %s"),
                G_program_name());
    }

    cell = (CELL **) G_malloc (ref.nfiles * sizeof (CELL *));
    cellfd = (int *) G_malloc (ref.nfiles * sizeof (int));
    for (n=0; n < ref.nfiles; n++)
    {
	cell[n] = G_allocate_cell_buf();
	name   = ref.file[n].name;
	mapset = ref.file[n].mapset;
	if ((cellfd[n] = G_open_cell_old (name, mapset)) < 0)
	    G_fatal_error(_("Unable to proceed"));
    }

    I_init_signatures (&in_sig, ref.nfiles);
    if (insigfile) {
      fd = I_fopen_signature_file_old (group, subgroup, insigfile);
      if (fd == NULL)
	G_fatal_error(_("** Can't open seed singature file <%s> **"), insigfile);

      n = I_read_signatures (fd, &in_sig);
      fclose (fd);
      if (n < 0)
	G_fatal_error(_("** Can't read signature file <%s> **"), insigfile);

      if (in_sig.nsigs > 255)
	  G_fatal_error(_("** <%s> has too many signatures (limit is 255)"), insigfile);

      maxclass = in_sig.nsigs;
    }

    return 0;
}
