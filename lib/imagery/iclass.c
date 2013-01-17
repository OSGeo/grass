/*!
   \file lib/imagery/iclass.c

   \brief Imagery library - functions for wx.iclass

   Computation based on training areas for supervised classification.
   Based on i.class module (GRASS 6).

   Copyright (C) 1999-2007, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author David Satnik, Central Washington University (original author)
   \author Markus Neteler <neteler itc.it> (i.class module)
   \author Bernhard Reiter <bernhard intevation.de> (i.class module)
   \author Brad Douglas <rez touchofmadness.com>(i.class module)
   \author Glynn Clements <glynn gclements.plus.com> (i.class module)
   \author Hamish Bowman <hamish_b yahoo.com> (i.class module)
   \author Jan-Oliver Wagner <jan intevation.de> (i.class module)
   \author Anna Kratochvilova <kratochanna gmail.com> (rewriting for wx.iclass)
   \author Vaclav Petras <wenzeslaus gmail.com> (rewriting for wx.iclass)
 */

#include <grass/imagery.h>
#include <grass/glocale.h>
#include <grass/vector.h>

#include "iclass_local_proto.h"


/*!
   \brief Calculates statistical values for one class and multiple bands based on training areas.

   Calculates values statistical based on the cells
   that are within training areas. Creates raster map
   to display the cells of the image bands which fall
   within standard deviations from the means.

   \param statistics pointer to bands statistics
   \param refer pointer to band files structure
   \param map_info vector map with training areas
   \param layer_name vector layer
   \param group name of imagery group
   \param raster_name name of temporary raster map (to be created)

   \return number of processed training areas
   \return -1 on failure
 */
int I_iclass_analysis(IClass_statistics * statistics, struct Ref *refer,
		      struct Map_info *map_info, const char *layer_name,
		      const char *group, const char *raster_name)
{
    int ret;

    int category;

    struct Cell_head band_region;

    CELL **band_buffer;

    int *band_fd;

    IClass_perimeter_list perimeters;



    G_debug(1, "iclass_analysis(): group = %s", group);

    category = statistics->cat;

    /* region set to current workin region */
    G_get_set_window(&band_region);

    /* find perimeter points from vector map */
    ret =
	vector2perimeters(map_info, layer_name, category, &perimeters,
			  &band_region);
    if (ret < 0) {
	return -1;
    }
    else if (ret == 0) {
	G_warning(_("No areas in category %d"), category);
	return 0;
    }

    open_band_files(refer, &band_buffer, &band_fd);
    alloc_statistics(statistics, refer->nfiles);
    make_all_statistics(statistics, &perimeters, band_buffer, band_fd);
    create_raster(statistics, band_buffer, band_fd, raster_name);
    close_band_files(refer, band_buffer, band_fd);

    free_perimeters(&perimeters);
    return ret;
}



/*!
   \brief Read files for the specified group subgroup into the Ref structure.

   \param group_name name of imagery group
   \param subgroup_name name of imagery subgroup
   \param[out] refer pointer to band files structure

   \return 1 on success
   \return 0 on failure
 */
int I_iclass_init_group(const char *group_name, const char *subgroup_name,
			struct Ref *refer)
{
    int n;

    G_debug(3, "I_iclass_init_group(): group_name = %s, subgroup_name = %s",
	    group_name, subgroup_name);
    I_init_group_ref(refer);	/* called in I_get_group_ref */

    I_get_subgroup_ref(group_name, subgroup_name, refer);

    for (n = 0; n < refer->nfiles; n++) {
	if (G_find_raster(refer->file[n].name, refer->file[n].mapset) == NULL) {
	    G_warning(_("Raster map <%s@%s> in subgroup "
			"<%s> does not exist"), refer->file[n].name,
		      refer->file[n].mapset, subgroup_name);
	    I_free_group_ref(refer);
	    return 0;
	}
    }

    if (refer->nfiles <= 1) {
	G_warning(_("Subgroup <%s> does not have enough files (it has %d files)"),
		  subgroup_name, refer->nfiles);
	I_free_group_ref(refer);
	return 0;
    }

    return 1;
}

/*!
   \brief Create raster map based on statistics.

   \param statistics pointer to bands statistics
   \param refer pointer to band files structure
   \param raster_name name of temporary raster map (to be created)
 */
void I_iclass_create_raster(IClass_statistics * statistics, struct Ref *refer,
			    const char *raster_name)
{
    CELL **band_buffer;

    int *band_fd;

    int b;

    for (b = 0; b < statistics->nbands; b++) {
	band_range(statistics, b);
    }

    open_band_files(refer, &band_buffer, &band_fd);
    create_raster(statistics, band_buffer, band_fd, raster_name);
    close_band_files(refer, band_buffer, band_fd);
}
