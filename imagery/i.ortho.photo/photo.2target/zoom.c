#include "globals.h"
#include "local_proto.h"

static int cancel(void);

int zoom(void)
{
    static int use = 1;
    static Objects objects[] = {
	MENU("CANCEL", cancel, &use),
	MENU("BOX", zoom_box, &use),
	MENU("POINT", zoom_point, &use),
	INFO("Select type of zoom", &use),
	{0}
    };

    Input_pointer(objects);
    return 0;			/* return, but don't QUIT */
}

static int cancel(void)
{
    return -1;
}

/* get target point for source point */
void source_to_target(double srx, double sry, double *trx, double *try)
{
    int i;
    double spx, spy, spz;	/* source photo */
    double trz;			/* target raster */

    G_debug(2, "source raster: %.0f %.0f", srx, sry);

    /* Photo coordinates of center on ZOOM1 */
    I_georef(srx, sry, &spx, &spy, group.E12, group.N12);
    G_debug(2, "source photo: %.3f %.3f", spx, spy);

    /* We need height but we don't know point on target ->
     * get aproximately the point on target and use that 
     * height for more precise position */
    spz = 0;
    for (i = 0; i < 3; i++) {
	I_inverse_ortho_ref(spx, spy, spz, trx, try, &trz,
			    &group.camera_ref,
			    group.XC, group.YC, group.ZC,
			    group.omega, group.phi, group.kappa);

	G_debug(2, "target raster: %.0f %.0f", *trx, *try);
	get_z_from_cell2(*try, *trx, &spz);
	G_debug(2, "target raster height: %.0f", spz);
    }

    G_debug(2, "target rast center: %.0f %.0f", *trx, *try);
}

void auto_zoom(void)
{
    double srx, sry;		/* source raster */
    double trx, try;		/* target raster */
    int vx, vy;
    double trl, trr;
    double width, magnific;

    if (autozoom_off)
	return;

    Compute_ortho_equation();
    if (group.con_equation_stat <= 0)
	return;

    /* Calc scale for target */
    sry = VIEW_MAP1_ZOOM->cell.head.north;
    srx = VIEW_MAP1_ZOOM->cell.head.west;
    source_to_target(srx, sry, &trl, &try);
    srx = VIEW_MAP1_ZOOM->cell.head.east;
    source_to_target(srx, sry, &trr, &try);
    width = trr - trl;		/* ZOOM1 width in target units */

    /* Calc magnification - relation between resolution
     * in zoom window and cell head */
    magnific = (VIEW_MAP1_ZOOM->right - VIEW_MAP1_ZOOM->left) / width;
    G_debug(3, "width = %.0f magnific = %f", width, magnific);

    /* Raster coordinates of center on ZOOM1 */
    srx = (VIEW_MAP1_ZOOM->cell.head.east +
	   VIEW_MAP1_ZOOM->cell.head.west) / 2;
    sry = (VIEW_MAP1_ZOOM->cell.head.north +
	   VIEW_MAP1_ZOOM->cell.head.south) / 2;

    source_to_target(srx, sry, &trx, &try);

    vx = easting_to_col(&VIEW_MAP2->cell.head, trx);
    vy = northing_to_row(&VIEW_MAP2->cell.head, try);
    G_debug(2, "target rast col, row: %d %d", vx, vy);

    vx = col_to_view(VIEW_MAP2, vx);
    vy = row_to_view(VIEW_MAP2, vy);

    G_debug(2, "target view col, row: %d %d", vx, vy);

    zoom_point2(vx, vy, 0, magnific);
}
