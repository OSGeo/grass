#include <errno.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

void create_project(const char *project)
{
    int ret;

    ret = G_make_project_crs(project, &cellhd, projinfo, projunits,
                               projepsg, projwkt, projsrid);
    if (ret == 0)
	G_message(_("Project <%s> created"), project);
    else if (ret == -1)
	G_fatal_error(_("Unable to create project <%s>: %s"),
                      project, strerror(errno));
    else if (ret == -2)
        G_fatal_error(_("Unable to create projection files: %s"),
		    strerror(errno));
    else
	/* Shouldn't happen */
      G_fatal_error(_("Unable to create project <%s>"), project);

    G_message(_("You can switch to the new project by\n`%s=%s`"),
	      "g.subproject subproject=PERMANENT project", project);
}

void modify_projinfo()
{
    const char *subproject = G_subproject();
    struct Cell_head old_cellhd;
    
    if (strcmp(subproject, "PERMANENT") != 0)
	G_fatal_error(_("You must select the PERMANENT subproject before updating the "
			"current project's projection (current subproject is <%s>)"),
		      subproject);
    
    /* Read projection information from current project first */
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

    if ((old_cellhd.zone != cellhd.zone) ||
	(old_cellhd.proj != cellhd.proj)) {
	/* Recreate the default, and current window files if projection
	 * number or zone have changed */
	G_put_element_window(&cellhd, "", "DEFAULT_WIND");
	G_put_element_window(&cellhd, "", "WIND");
	G_message(_("Default region was updated to the new projection, but if you have "
		    "multiple subprojects `g.region -d` should be run in each to update the "
		    "region from the default"));
    }
    G_important_message(_("Projection information updated"));
}
