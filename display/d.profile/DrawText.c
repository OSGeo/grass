#include <grass/raster.h>
#include <grass/display.h>

/* DrawText.c 
 *
 * function defined:
 *
 * DrawText(size,row,col,text)
 *
 * int size,            - size of text (percent of window height) 
 *     row,             - text row  
 *     col;             - text col
 * char *text;          - text string
 *
 * PURPOSE: To display a text string in the currently selected on-screen
 * window, in much the same way that the Dtext program works.  Just like
 * Dtext the text size is expressed as a percentage of the window height.
 * The number of text rows and columns in a window are determined by the
 * text size.
 *
 * NOTES:
 *
 * 1) Assumes that R_open_driver has already been called.
 *
 * 2)
 *
 * Dave Johnson
 * DBA Systems, Inc.
 * 10560 Arrowhead Drive
 * Fairfax, Virginia 22030
 *
 */

int DrawText(int size, int row, int col, char *text)
{
    int b, t, l, r;
    int tsize;
    int dots_per_line;
    int cur_dot_row;

#ifdef DEBUG
    fprintf(stdout, "DrawText: %s\n", text);
#endif

    D_get_screen_window(&t, &b, &l, &r);
    dots_per_line = (int)(size / 100.0 * (float)(b - t));
    tsize = (int)(.8 * (float)dots_per_line);
    cur_dot_row = t + dots_per_line * row;
    R_text_size(tsize, tsize);
    R_move_abs(l + col * tsize, cur_dot_row);
    R_text(text);

    return 0;
}
