/* quick_draw:
 ** uses libgsf to draw wire frame surfaces
 */
#include <stdlib.h>
#include <grass/gis.h>
#include "tk.h"
#include "interface.h"

int Nquick_draw_cmd(Nv_data * dc, Tcl_Interp * interp)
{
    int i, max;
    int *surf_list, *vol_list;

    GS_set_draw(GSD_BACK);
    GS_clear(dc->BGcolor);
    GS_ready_draw();
    surf_list = GS_get_surf_list(&max);

    max = GS_num_surfs();
    for (i = 0; i < max; i++) {
	if (check_blank(interp, surf_list[i]) == 0) {
	    GS_draw_wire(surf_list[i]);
	}
    }

    G_free(surf_list);

    vol_list = GVL_get_vol_list(&max);
    max = GVL_num_vols();
    for (i = 0; i < max; i++) {
	if (check_blank(interp, vol_list[i]) == 0) {
	    GVL_draw_wire(vol_list[i]);
	}
    }

    GS_done_draw();

/*** ACS_MODIFY flythrough  ONE LINE ***************/
    flythrough_postdraw_cb();

    return (TCL_OK);
}
