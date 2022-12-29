/*
 * find_3dcell (cell)
 *
 * Find the a 3dcell in the current mapset
 **************************************************************/
#include <string.h>
#include <grass/gis.h>
#include <grass/raster3d.h>


int g3_find_dsp_file(const char *cell, const char *file, const char *mset)
{
    char element[GNAME_MAX+10], name[GNAME_MAX], mapset[GMAPSET_MAX],
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
const char *check_get_any_dspname(const char *dspf, const char *g3f, const char *mset)
{
    if (!G_find_raster3d(g3f, ""))
	G_fatal_error("3D raster map <%s> not found", g3f);

    if (mset) {			/* otherwise must be reading only  */
	if (g3_find_dsp_file(g3f, dspf, mset)) {	/* already exists */
	    /* the parser should handle the overwrite check */
	}
    }

    return dspf;
}
