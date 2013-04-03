#include <errno.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

void create_location(char *location)
{
    int ret;

    ret = G_make_location(location, &cellhd, projinfo, projunits);
    if (ret == 0)
	G_message(_("Location <%s> created"), location);
    else if (ret == -1)
	G_fatal_error(_("Unable to create location <%s>: %s"),
                      location, strerror(errno));
    else if (ret == -2)
        G_fatal_error(_("Unable to create projection files: %s"),
		    strerror(errno));
    else
	/* Shouldn't happen */
      G_fatal_error(_("Unable to create location <%s>"), location);

    G_message(_("You can switch to the new location by\n`%s=%s`"),
	      "g.mapset mapset=PERMANENT location", location);
}

void modify_projinfo()
{
    const char *mapset = G_mapset();
    struct Cell_head old_cellhd;
    
    if (strcmp(mapset, "PERMANENT") != 0)
	G_fatal_error(_("You must select the PERMANENT mapset before updating the "
			"current location's projection (current mapset is <%s>)."),
		      mapset);
    
    /* Read projection information from current location first */
    G_get_default_window(&old_cellhd);
    
    char path[GPATH_MAX];
	
    /* Write out the PROJ_INFO, and PROJ_UNITS if available. */
    if (projinfo != NULL) {
	G_file_name(path, "", "PROJ_INFO", "PERMANENT");
	G_write_key_value_file(path, projinfo);
    }
    
    if (projunits != NULL) {
	G_file_name(path, "", "PROJ_UNITS", "PERMANENT");
	G_write_key_value_file(path, projunits);
    }
    
    if ((old_cellhd.zone != cellhd.zone) ||
	(old_cellhd.proj != cellhd.proj)) {
	/* Recreate the default, and current window files if projection
	 * number or zone have changed */
	G__put_window(&cellhd, "", "DEFAULT_WIND");
	G__put_window(&cellhd, "", "WIND");
	G_message(_("Default region was updated to the new projection, but if you have "
		    "multiple mapsets `g.region -d` should be run in each to update the "
		    "region from the default"));
    }
    G_important_message(_("Projection information updated"));
}
