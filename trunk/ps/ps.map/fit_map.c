/* Function: fit_map_to_box
 **
 ** Author: Paul W. Carlson     5/92
 */

#include "ps_info.h"

int fit_map_to_box(void)
{

    /* make map fit in bounding box */
    PS.ew_to_x = PS.map_width * 72.0 / (PS.w.east - PS.w.west);
    PS.ns_to_y = PS.map_height * 72.0 / (PS.w.north - PS.w.south);
    if (PS.ew_to_x < PS.ns_to_y) {
	PS.map_pix_wide = 72.0 * PS.map_width;
	PS.map_pix_high = 72.0 * PS.map_height * PS.ew_to_x / PS.ns_to_y;
	PS.ns_to_y = PS.ew_to_x;
    }
    else {
	PS.map_pix_wide = 72.0 * PS.map_width * PS.ns_to_y / PS.ew_to_x;
	PS.map_pix_high = 72.0 * PS.map_height;
	PS.ew_to_x = PS.ns_to_y;
    }

    return 0;
}
