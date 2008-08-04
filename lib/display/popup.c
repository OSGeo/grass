
/****************************************************************************
 * D_popup(back_colr, text_colr, div_colr, top, left, percent_per_line, options)
 *    int back_colr ;           color of window
 *    int text_color ;          color of text and border
 *    int div_color ;           color of item divider lines
 *    int left, top ;           pixle coordinates of top-left corner
 *                              (Coordinate system is 0,0 lower left, 
 *                              100,100 upper right)
 *    int percent_per_line ;    percent of entire window per line of text
 *    char *options[] ;         array of text showing options.
 *                              The first entry is a title, not an option
 *
 * The bottom-right coordinates are calculated based on the top-left coors.,
 * the percent_per_line, the number of options, and the longest option text
 * length.  If necessary, the window is moved to make sure it is inside
 * the screen.
 *
 * - Current screen contents are stashed away in the area.
 * - Area is blanked with the background color and fringed with the
 *    text color.
 * - Options are drawn using the current font.
 * - User uses the mouse to choose the desired option.
 * - Area is restored with the original contents.
 * - Number of this option is returned to the calling program.
 ***************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>

#define Y_BORDER	5
#define X_BORDER	5


/*!
 * \brief pop-up menu
 *
 * This routine provides a pop-up type menu
 * on the graphics screen. The <b>bcolor</b> specifies the background color.
 * The <b>tcolor</b> is the text color. The <b>dcolor</b> specifies the color
 * of the line used to divide the menu items. The <b>top</b> and <b>left</b>
 * specify the placement of the top left corner of the menu on the screen. 0,0 is
 * at the bottom left of the screen, and 100,100 is at the top right. The
 * <b>size</b> of the text is given as a percentage of the vertical size of the
 * screen.  The <b>options</b> array is a NULL terminated array of character
 * strings.  The first is a menu title and the rest are the menu options (i.e.,
 * options[0] is the menu title, and options[1], options[2], etc., are the menu
 * options). The last option must be the NULL pointer.
 * The coordinates of the bottom right of the menu are calculated based on the
 * <b>top left</b> coordinates, the <b>size</b>, the number of
 * <b>options</b>, and the longest option text length. If necessary, the menu
 * coordinates are adjusted to make sure the menu is on the screen.
 * <i>D_popup(~)</i> does the following:
 * <ol>
 <li> Current screen contents under the menu are saved.
 </li>
 <li> Area is blanked with the background color and fringed with the text color.
 </li>
 <li> Menu options are drawn using the current font.
 </li>
 <li> User uses the mouse to choose the desired option.
 </li>
 <li> Menu is erased and screen is restored with the original contents.
 </li>
 <li> Number of the selected option is returned to the calling module.
 </li></ol>
 *
 *  \param bcolor
 *  \param tcolor
 *  \param dcolor
 *  \param top
 *  \param left
 *  \param size
 *  \param options[]
 *  \return int
 */

int D_popup(int back_colr, int text_colr, int div_colr,
	    int top, int left, int percent_per_line, char *options[])
{
    int t, l, b, r;
    int opt;
    int x, y;
    int button;
    int text_size;
    int text_raise;
    int n_options;
    int max_len;
    int len;
    char *panel;
    int dots_per_line, dots_per_char, height, width;

    /* Figure the number of options and the max length of options */
    max_len = 0;
    for (n_options = 0; options[n_options] != NULL; n_options++) {
	len = strlen(options[n_options]);
	if (max_len < len)
	    max_len = len;
    }

    /* Figure the dots per line and dots_per_char */
    height = R_screen_bot() - R_screen_top();
    width = R_screen_rite() - R_screen_left();
    dots_per_line = height * percent_per_line / 100;
    dots_per_char = width / (max_len + 2);
    /* we want the box to fit into window horizontally */

    t = R_screen_bot() - (R_screen_bot() - R_screen_top()) * top / 100;
    l = R_screen_left() + (R_screen_rite() - R_screen_left()) * left / 100;

    /* Figure the bottom and right of the window */
    text_size = (int)(.8 * (float)dots_per_line);
    if (text_size > dots_per_char)
	text_size = dots_per_char;

    text_raise = (dots_per_line - text_size + 1) / 2;
    if (text_raise == 0)
	text_raise = 1;
    b = Y_BORDER + t + dots_per_line * n_options;
    r = 2 * X_BORDER + l + text_size * max_len;

    /* Adjust, if necessary, to make sure window is all on screen */
    if (t < R_screen_top()) {
	b = b + (R_screen_top() - t);
	t = R_screen_top();
    }
    if (b > R_screen_bot()) {
	t = t - (b - R_screen_bot());
	b = R_screen_bot();
    }
    if (t < R_screen_top())
	G_fatal_error("popup window too big vertically\n");

    if (l < R_screen_left()) {
	r = r + (R_screen_left() - l);
	l = R_screen_left();
    }
    if (r > R_screen_rite()) {
	l = l - (r - R_screen_rite());
	r = R_screen_rite();
    }
    if (l < R_screen_left()) {
	/* actually, this should never happen */
	fprintf(stderr, "ERROR:\n");
	fprintf(stderr, "popup window too big horizontally\n");
	fprintf(stderr, "to fit into the graphics window.\n");
	fprintf(stderr, "Widen the graphics window.");
	fprintf(stderr, "\nExiting...\n");
	exit(1);
    }

    /* Make sure text is not drawn outside of window */
    R_set_window(t, b, l, r);

    /* Save the panel in some name */
    panel = G_tempfile();
    R_panel_save(panel, t, b, l, r);

    /* Clear the panel */
    R_standard_color(back_colr);
    R_box_abs(l, t, r, b);

    /* Draw border */
    R_standard_color(text_colr);
    R_move_abs(l + 1, t + 1);
    R_cont_abs(r - 1, t + 1);
    R_cont_abs(r - 1, b - 1);
    R_cont_abs(l + 1, b - 1);
    R_cont_abs(l + 1, t + 1);

    /* Prepare for text */
    R_text_size(text_size, text_size);

    /* list the options */
    for (opt = 1; opt <= n_options; opt++) {
	if (opt != n_options) {
	    R_standard_color(div_colr);
	    R_move_abs(l + 2, t + Y_BORDER + opt * dots_per_line);
	    R_cont_rel(r - l - 4, 0);
	}
	R_standard_color(text_colr);
	R_move_abs(l + X_BORDER,
		   t + Y_BORDER + opt * dots_per_line - text_raise);
	R_text(options[opt - 1]);
    }

    R_flush();

    x = (l + r) / 2;
    y = (t + b) / 2;

    while (1) {
	int n;

	R_get_location_with_pointer(&x, &y, &button);
	if (x > r
	    || x < l || y < t + Y_BORDER + dots_per_line || y > b - Y_BORDER)
	    continue;

	n = y - t - Y_BORDER;
	if (n % dots_per_line == 0)
	    continue;

	R_panel_restore(panel);
	R_panel_delete(panel);
	return (n / dots_per_line);
    }
}
