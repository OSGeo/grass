/*
 * find_3dcell (cell)
 *
 * Find the a 3dcell in the current mapset
 **************************************************************/
#include <grass/gis.h>
#include <grass/raster3d.h>


int g3_find_dsp_file(cell, file, mset)
     char *cell, *mset;
     char *file;
{
    char element[100], name[GNAME_MAX], mapset[GMAPSET_MAX],
	tofind[GNAME_MAX];

    if (file == NULL || *file == 0)
	return 0;

    strcpy(tofind, file);

    if (G_name_is_fully_qualified(cell, name, mapset))
	sprintf(element, "grid3/%s/dsp", name);
    else
	sprintf(element, "grid3/%s/dsp", cell);

    return G_find_file(element, tofind, mset) != NULL;
}


/* return NULL on error: otherwise returns dspout */
char *check_get_any_dspname(dspf, g3f, mset)
     char *dspf, *g3f, *mset;
{
    char element[200], question[200];
    static char dspout[200];

    if (!G_find_raster3d(g3f, "")) {
	fprintf(stderr, "3D raster map <%s> not found", g3f);
	return (NULL);
    }

    if (mset) {			/* otherwise must be reading only  */

	if (g3_find_dsp_file(g3f, dspf, mset)) {	/* already exists */
	    sprintf(question, "\n** %s exists. ok to overwrite? ", dspf);
	    if (!G_yes(question, 0)) {
		if (NULL == G_ask_any("", dspout, element, "display", 1))
		    return (NULL);
		return (dspout);
	    }
	    /* or else just print a warning & use it as is */

	}

    }

    strcpy(dspout, dspf);
    return (dspout);
}
