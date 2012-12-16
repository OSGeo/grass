/*
   exec.c --

   Loop through all files to be rectified and do the retification.
   Handles things like support files.
 */

#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "global.h"

int exec_rectify(char *extension, char *interp_method, char *angle_map)
{
    char *name;
    char *mapset;
    char *result;
    char *type = "raster";
    int n;
    struct Colors colr;
    struct Categories cats;
    struct History hist;
    int colr_ok, cats_ok;
    long start_time, rectify_time;
    double aver_z;
    int elevfd;
    struct cache *ebuffer;

    G_debug(1, "Open elevation raster: ");

    /* open elevation raster */
    select_target_env();
    G_set_window(&target_window);
    G_debug(1, "target window: rs=%d cs=%d n=%f s=%f w=%f e=%f\n",
	    target_window.rows, target_window.cols, target_window.north,
	    target_window.south, target_window.west, target_window.east);

    elevfd = Rast_open_old(elev_name, elev_mapset);
    if (elevfd < 0) {
	G_fatal_error(_("Could not open elevation raster"));
	return 1;
    }
    ebuffer = readcell(elevfd, seg_mb_elev, 1);
    select_target_env();
    Rast_close(elevfd);

    /* get an average elevation of the control points */
    /* this is used only if target cells have no elevation */
    get_aver_elev(&group.control_points, &aver_z);

    G_message("-----------------------------------------------");

    /* rectify each file */
    for (n = 0; n < group.group_ref.nfiles; n++) {
	if (!ref_list[n])
	    continue;

	name = group.group_ref.file[n].name;
	mapset = group.group_ref.file[n].mapset;
	result =
	    G_malloc(strlen(group.group_ref.file[n].name) + strlen(extension) + 1);
	strcpy(result, group.group_ref.file[n].name);
	strcat(result, extension);

	G_debug(2, "ORTHO RECTIFYING:");
	G_debug(2, "NAME %s", name);
	G_debug(2, "MAPSET %s", mapset);
	G_debug(2, "RESULT %s", result);
	G_debug(2, "select_current_env...");

	select_current_env();

	cats_ok = Rast_read_cats(name, mapset, &cats) >= 0;
	colr_ok = Rast_read_colors(name, mapset, &colr) > 0;

	/* Initialze History */
	if (Rast_read_history(name, mapset, &hist) < 0)
	    Rast_short_history(result, type, &hist);
	G_debug(2, "reading was fine...");

	time(&start_time);

	G_debug(2, "Starting the rectification...");

	if (rectify(name, mapset, ebuffer, aver_z, result, interp_method)) {
	    G_debug(2, "Done. Writing results...");
	    select_target_env();
	    if (cats_ok) {
		Rast_write_cats(result, &cats);
		Rast_free_cats(&cats);
	    }
	    if (colr_ok) {
		Rast_write_colors(result, G_mapset(), &colr);
		Rast_free_colors(&colr);
	    }
	    /* Write out History */
	    Rast_command_history(&hist);
	    Rast_write_history(result, &hist);

	    select_current_env();
	    time(&rectify_time);
	    report(rectify_time - start_time, 1);
	}
	else
	    report((long)0, 0);

	G_free(result);
    }
    
    close(ebuffer->fd);
    release_cache(ebuffer);

    if (angle_map) {
	camera_angle(angle_map);
    }
    
    return 0;
}
