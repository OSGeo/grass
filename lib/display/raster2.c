
/********************************************************************
 * code in this file is designed to send raster data to the graphics
 * driver. It handles raster->color lookup translation, as well as
 * loading appropriate colormaps into the driver and the sending of
 * raster data to the plotter. The loading of colors is designed to
 * never send more colors than the hardware can support - even though
 * the GRASS drivers will allocate virtual colormaps to pretend there are more
 * This code effectively disables that driver feature/mistake.
 *
 * To simply plot raster data:
 *
 * to specify if overlay mode is to be used
 *   D_set_overlay_mode(flag)
 *      int flag;              /1=yes,0=no/
 *
 * to select a raster color for line drawing
 *   D_color (cat, colors)
 *      CELL cat
 *      struct Colors *colors; /color info/
 *
 *   D_color_of_type(raster, colors, data_type);
 *      void *raster;
 *      struct Colors *colors; /color info/
 *      RASTER_MAP_TYPE data_type;
 *
 * Note: the same Colors structure must be passed to all routines.
 *
 */
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>

int D__overlay_mode = 0;	/* external for now, but to be fixed later */


/*!
 * \brief Configure raster overlay mode
 *
 * This routine determines if D_draw_raster() draws in overlay mode
 * (locations with category 0 are left untouched) or not (colored with
 * the color for category 0).
 *
 * \param n 1 (TRUE) for overlay mode; 0 (FALSE) otherwise
 *
 * \return 0
 */
int D_set_overlay_mode(int n)
{
    D__overlay_mode = (n != 0);

    return 0;
}


/* this routine modifies the hardware colormap
 * provided that we are not using fixed mode colors.
 * For use by programs such as d.colors
 *
 * returns:
 *    0 error - in fixed mode,
 *              or cat not in min:max color range
 *    1 ok
 */

int D_color(CELL cat, struct Colors *colors)
{
    return D_c_color(cat, colors);
}

/* select color for line drawing */
int D_c_color(CELL cat, struct Colors *colors)
{
    return D_color_of_type(&cat, colors, CELL_TYPE);
}

/* select color for line drawing */

/*!
 * \brief 
 *
 * Same functionality as <tt>D_color()</tt> except that the <em>value</em> is type 
 * <tt>DCELL</tt>.  This implies that the floating-point interfaces to the <em>colors</em>
 *  are used by this routine.
 *
 *  \param value
 *  \param colors
 *  \return int
 */

int D_d_color(DCELL val, struct Colors *colors)
{
    return D_color_of_type(&val, colors, DCELL_TYPE);
}

/* select color for line drawing */

/*!
 * \brief 
 *
 * Same
 * functionality as <tt>D_color()</tt> except that the <em>value</em> is type <tt>FCELL</tt>. 
 * This implies that the floating-point interfaces to the <em>colors</em> are used by this routine.
 *
 *  \param value
 *  \param colors
 *  \return int
 */

int D_f_color(FCELL val, struct Colors *colors)
{
    return D_color_of_type(&val, colors, FCELL_TYPE);
}


/*!
 * \brief 
 *
 * If the <em>data_type</em> is CELL_TYPE,
 * calls D_color((CELL *value, colors);
 * If the <em>data_type</em> is FCELL_TYPE, calls D_f_color((FCELL *value,
 * colors);
 * If the <em>data_type</em> is DCELL_TYPE, calls D_d_color((DCELL *value,
 * colors);
 *
 *  \param value
 *  \param colors
 *  \param data_type
 *  \return int
 */

int D_color_of_type(const void *raster,
		    struct Colors *colors, RASTER_MAP_TYPE data_type)
{
    int r, g, b;

    Rast_get_color(raster, &r, &g, &b, colors, data_type);
    D_RGB_color((unsigned char)r, (unsigned char)g, (unsigned char)b);

    return 0;
}
