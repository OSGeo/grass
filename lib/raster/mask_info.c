/*
 *************************************************************
 * char * Rast_mask_info ()
 *
 *   returns a printable text of mask information
 *
 ************************************************************
 * Rast__mask_info (name, subproject)
 *
 *      char name[GNAME_MAX], subproject[GMAPSET_MAX];
 *
 * function:
 *   determine the status off the automatic masking
 *   and the name of the cell file which forms the mask
 *
 *   (the mask file is actually MASK in the current subproject,
 *   but is usually a reclassed cell file, and the reclass
 *   name and subproject are returned)
 *
 * returns:
 *   -1   no masking (name, subproject undefined)
 *        name, subproject are undefined
 *
 *    1   mask file present, masking on
 *        name, subproject hold mask file name, subproject
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
    char subproject[GMAPSET_MAX];

    switch (Rast__mask_info(name, subproject)) {
    case 1:
	sprintf(text, _("<%s> in subproject <%s>"), name, subproject);
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

int Rast__mask_info(char *name, char *subproject)
{
    char rname[GNAME_MAX], rsubproject[GMAPSET_MAX];

    strcpy(name, "MASK");
    strcpy(subproject, G_subproject());

    if (!G_find_raster(name, subproject))
	return -1;

    if (Rast_is_reclass(name, subproject, rname, rsubproject) > 0) {
	strcpy(name, rname);
	strcpy(subproject, rsubproject);
    }

    return 1;
}
