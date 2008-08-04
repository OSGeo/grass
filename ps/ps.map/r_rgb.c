/* Function: rgbfile
 **
 ** Author: Glynn Clements      Apr 2001
 ** Derived from groupfile by Paul W. Carlson
 */

#include <string.h>
#include "ps_info.h"
#include "group.h"
#include "local_proto.h"

int read_rgb(char *key, char *data)
{
    char names[3][100];
    char fullname[100];
    int i;

    if (sscanf(data, "%s %s %s", names[0], names[1], names[2]) != 3) {
	error(key, data, "illegal request (rgb)");
	return 0;
    }

    PS.do_raster = 0;
    PS.do_colortable = 0;
    if (PS.cell_fd >= 0) {
	G_close_cell(PS.cell_fd);
	G_free(PS.cell_name);
	G_free(PS.cell_mapset);
	G_free_colors(&PS.colors);
	PS.cell_fd = -1;
    }

    /* initialize group structure (for compatibility with PS_raster_plot()) */
    I_init_group_ref(&grp.ref);

    /*
     * not relevant here
     *
     if (I_get_group_ref(grp.group_name, &grp.ref) == 0)
     G_fatal_error(_("Can't get group information"));
     */

    grp.group_name = "RGB Group";

    /* get file names for R, G, & B */

    for (i = 0; i < 3; i++) {
	char *mapset, *name, *p;

	name = names[i];

	p = strchr(name, '@');
	if (p) {
	    *p = '\0';
	    mapset = p + 1;
	}
	else {
	    mapset = G_find_file2("cell", name, "");
	    if (!mapset) {
		error(name, "", "not found");
		return 0;
	    }
	}

	grp.name[i] = G_store(name);
	grp.mapset[i] = G_store(mapset);

	/* read in colors */
	if (G_read_colors(grp.name[i], grp.mapset[i], &(grp.colors[i])) == -1) {
	    sprintf(fullname, "%s in %s", grp.name[i], grp.mapset[i]);
	    error(fullname, "", "can't read color table");
	    return 0;
	}

	/* open raster maps for reading */
	if ((grp.fd[i] = G_open_cell_old(grp.name[i], grp.mapset[i])) < 0) {
	    sprintf(fullname, "%s in %s", grp.name[i], grp.mapset[i]);
	    error(fullname, "", "can't open raster map");
	    G_free_colors(&(grp.colors[i]));
	    return 0;
	}
    }

    strcpy(PS.celltitle, grp.group_name);
    G_strip(PS.celltitle);
    return 1;
}
