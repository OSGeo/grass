#include <errno.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

void create_location(const char *location)
{
    int ret;

    ret = G_make_location_crs(location, &cellhd, projinfo, projunits, projsrid,
                              projwkt);
    if (ret == 0)
<<<<<<< HEAD
<<<<<<< HEAD
        G_message(_("Project <%s> created"), location);
    else if (ret == -1)
        G_fatal_error(_("Unable to create project <%s>: %s"), location,
=======
        G_message(_("Location <%s> created"), location);
    else if (ret == -1)
        G_fatal_error(_("Unable to create location <%s>: %s"), location,
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        G_message(_("Location <%s> created"), location);
    else if (ret == -1)
        G_fatal_error(_("Unable to create location <%s>: %s"), location,
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
                      strerror(errno));
    else if (ret == -2)
        G_fatal_error(_("Unable to create projection files: %s"),
                      strerror(errno));
    else
        /* Shouldn't happen */
<<<<<<< HEAD
<<<<<<< HEAD
        G_fatal_error(_("Unable to create project <%s>"), location);

    G_message(_("You can switch to the new project by\n`%s=%s`"),
              "g.mapset mapset=PERMANENT project", location);
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        G_fatal_error(_("Unable to create location <%s>"), location);

    G_message(_("You can switch to the new location by\n`%s=%s`"),
              "g.mapset mapset=PERMANENT location", location);
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
}

void modify_projinfo(void)
{
    const char *mapset = G_mapset();
    struct Cell_head old_cellhd;

    if (strcmp(mapset, "PERMANENT") != 0)
        G_fatal_error(
            _("You must select the PERMANENT mapset before updating the "
<<<<<<< HEAD
<<<<<<< HEAD
              "current project's coordinate reference system"
              " (current mapset is <%s>)"),
=======
              "current location's projection (current mapset is <%s>)"),
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
              "current location's projection (current mapset is <%s>)"),
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
            mapset);

    /* Read projection information from current location first */
    G_get_default_window(&old_cellhd);

    char path[GPATH_MAX];

    /* Write out the PROJ_INFO, PROJ_UNITS, and PROJ_EPSG if available. */
    if (projinfo != NULL) {
        G_file_name(path, "", "PROJ_INFO", "PERMANENT");
        G_write_key_value_file(path, projinfo);
    }

    if (projunits != NULL) {
        G_file_name(path, "", "PROJ_UNITS", "PERMANENT");
        G_write_key_value_file(path, projunits);
    }

    if (projepsg != NULL) {
        G_file_name(path, "", "PROJ_EPSG", "PERMANENT");
        G_write_key_value_file(path, projepsg);
    }

    if (projwkt != NULL) {
        G_write_projwkt(NULL, projwkt);
    }

    if (projsrid != NULL) {
        G_write_projsrid(NULL, projsrid);
    }

    if ((old_cellhd.zone != cellhd.zone) || (old_cellhd.proj != cellhd.proj)) {
        /* Recreate the default, and current window files if projection
         * number or zone have changed */
        G_put_element_window(&cellhd, "", "DEFAULT_WIND");
        G_put_element_window(&cellhd, "", "WIND");
        G_message(_(
            "Default region was updated to the new projection, but if you have "
            "multiple mapsets `g.region -d` should be run in each to update "
            "the "
            "region from the default"));
    }
    G_important_message(_("Projection information updated"));
}
