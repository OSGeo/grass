/*
 *************************************************************
 * char * Rast_mask_info ()
 *
 *   returns a printable text of mask information
 *
 ************************************************************
 * Rast__mask_info (name, mapset)
 *
 *      char name[GNAME_MAX], mapset[GMAPSET_MAX];
 *
 * function:
 *   determine the status off the automatic masking
 *   and the name of the cell file which forms the mask
 *
 *   (the mask file is actually MASK in the current mapset,
 *   but is usually a reclassed cell file, and the reclass
 *   name and mapset are returned)
 *
 * returns:
 *   -1   no masking (name, mapset undefined)
 *        name, mapset are undefined
 *
 *    1   mask file present, masking on
 *        name, mapset hold mask file name, mapset
 *
 ***************************************************************/

#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

char *Rast_mask_info(void)
{
    char text[GNAME_MAX + GMAPSET_MAX + 16];
    char name[GNAME_MAX];
    char mapset[GMAPSET_MAX];

    switch (Rast__mask_info(name, mapset)) {
    case 1:
	sprintf(text, _("<%s> in mapset <%s>"), name, mapset);
	break;
    case -1:
	strcpy(text, _("none"));
	break;
    default:
	strcpy(text, _("not known"));
	break;
    }

    return G_store(text);
}

int Rast__mask_info(char *name, char *mapset)
{
    char rname[GNAME_MAX], rmapset[GMAPSET_MAX];

    strcpy(name, "MASK");
    strcpy(mapset, G_mapset());

    if (!G_find_raster(name, mapset))
	return -1;

    if (Rast_is_reclass(name, mapset, rname, rmapset) > 0) {
	strcpy(name, rname);
	strcpy(mapset, rmapset);
    }

    return 1;
}
