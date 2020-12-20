#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

int check_reclass(const char *name, const char *subproject, int force)
{
    char rname[GNAME_MAX], rsubproject[GMAPSET_MAX];
    char **rmaps;
    int nrmaps;

    if (Rast_is_reclassed_to(name, subproject, &nrmaps, &rmaps) > 0) {
	for (; *rmaps; rmaps++) {
	    /* force remove */
	    if (force)
		G_warning(_("Raster map <%s@%s> is a base map for <%s>. Remove forced."),
			  name, subproject, *rmaps);
	    else
		G_warning(_("Raster map <%s@%s> is a base map. Remove reclassed map <%s> first."),
			  name, subproject, *rmaps);
	}

	if (!force)
	    return 1;
    }

    if (Rast_is_reclass(name, subproject, rname, rsubproject) > 0 &&
	Rast_is_reclassed_to(rname, rsubproject, &nrmaps, &rmaps) > 0) {
	char path[GPATH_MAX];
	char *p = strchr(rname, '@');
	char *qname = G_fully_qualified_name(name, subproject);

	if (p)
	    *p = '\0';

	G_file_name_misc(path, "cell_misc", "reclassed_to", rname, rsubproject);

	if (nrmaps == 1 && !G_strcasecmp(rmaps[0], qname)) {

	    if (remove(path) < 0)
		G_warning(_("Removing information about reclassed map from <%s@%s> failed"),
			  rname, rsubproject);
	}
	else {
	    FILE *fp = fopen(path, "w");

	    if (fp) {
		for (; *rmaps; rmaps++)
		    if (G_strcasecmp(*rmaps, qname))
			fprintf(fp, "%s\n", *rmaps);
		fclose(fp);
	    }
	    else
		G_warning(_("Removing information about reclassed map from <%s@%s> failed"),
			  rname, rsubproject);
	}
    }

    return 0;
}
