#include <curses.h>
#include <grass/gis.h>
#include "globals.h"
#include "local_proto.h"


int g_init(void)
{

    /* initialize graphics processor, define viewports */
    Init_graphics();

    /* load the color table 
       set_colors ();
       set_plane10 ();
     */
    /* clear both frames                    */
    /* select frame 1 to show and to modify 
       select_frame (1,3);
       clear ();
       select_frame (1,1);
     */
    /* select map_window 
       map_window();
     */

    return 0;
}
