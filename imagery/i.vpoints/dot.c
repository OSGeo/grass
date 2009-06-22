#include "globals.h"
#include <grass/display.h>

int dot(int x, int y)
{
    double vx[5], vy[5];

    vx[0] = x;
    vy[0] = y - dotsize;
    vx[1] = x - dotsize;
    vy[1] = y;
    vx[2] = x;
    vy[2] = y + dotsize;
    vx[3] = x + dotsize;
    vy[3] = y;
    vx[4] = x;
    vy[4] = y - dotsize;

    R_polygon_abs(vx, vy, 5);
    /*
       int i;

       for (i = 0; i < dotsize; i++)
       {
       R_move_abs (x-i, y+i-dotsize);
       R_cont_rel (i+i,0);
       R_move_abs (x-i, y+dotsize-i);
       R_cont_rel (i+i,0);
       }
       R_move_abs (x-dotsize, y);
       R_cont_rel (dotsize+dotsize, 0);
     */

    return 0;
}

int save_under_dot(int x, int y)
{
    R_panel_save(tempfile1, y - dotsize, y + dotsize, x - dotsize,
		 x + dotsize);

    return 0;
}

int restore_under_dot(void)
{
    R_panel_restore(tempfile1);

    return 0;
}

int release_under_dot(void)
{
    R_panel_delete(tempfile1);

    return 0;
}
