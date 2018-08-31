
/****************************************************************************
 *
 * Function:     Rast_map_to_img_str() based on r.to.ppm
 * AUTHOR(S):    Bill Brown, USA-CERL (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jachym Cepicky <jachym les-ejk.cz>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 *               Soeren Gebbert
 * PURPOSE:      converts a GRASS raster map into an ARGB or
 *               gray scale unsigned char string
 * COPYRIGHT:    (C) 1999-2015 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#define DEF_RED 255
#define DEF_GRN 255
#define DEF_BLU 255

/* \brief Convert a raster map layer into a string with
 *        32Bit ARGB, 32Bit RGB or 8Bit Gray little endian encoding.
 * 
 * The raster color table is used for coloring the image. Null values are 
 * marked as transparent. Only little endian encoding is supported.
 * 
 * This function uses Rast_window_rows() and Rast_window_cols() to 
 * get rows and cols, hence use Rast_set_window() to set the required
 * region for raster access.
 * 
 * \param name The name of the raster map layer to convert
 * \param color_mode The color modes to use:
 *                  Color mode 1 -> 32Bit ARGB (0xAARRGGBB)
 *                  Color mode 2 -> 32Bit RGB  (0xffRRGGBB)
 *                  Color mode 3 -> grey scale formular: .33R+ .5G+ .17B
 *                  Color mode 4 -> grey scale formular: .30R+ .59G+ .11B
 * 
 * \param result: An unsigned char pointer to store the result. 
 *                It must have size 4*cols*rows in case of
 *                ARGB and RGB, 
 *                rows*cols in case of gray scale.
 * 
 * \return: 0 in case map not found, -1 in case the color mode is incorrect, 1 on success
 * 
 */
int Rast_map_to_img_str(char *name, int color_mode, unsigned char* result)
{
    unsigned char *set = NULL, *red = NULL, *green = NULL, 
                  *blue = NULL;
    unsigned char alpha;
    const char *mapset = NULL;
    CELL *cell_buf = NULL;
    FCELL *fcell_buf = NULL;
    DCELL *dcell_buf = NULL;
    void *voidc = NULL;
    int rtype, row, col;
    size_t i;
    int map = 0;
    
    struct Colors colors;
    int rows = Rast_window_rows();
    int cols = Rast_window_cols();

    if(color_mode > 3 || color_mode < 1)
        return(-1);

    mapset = G_find_raster2(name, "");
    
    if(!mapset)
        return(0);
    
    map = Rast_open_old(name, "");

    cell_buf = Rast_allocate_c_buf();
    fcell_buf = Rast_allocate_f_buf();
    dcell_buf = Rast_allocate_d_buf();

    red = G_malloc(cols);
    green = G_malloc(cols);
    blue = G_malloc(cols);
    set  = G_malloc(cols);

    Rast_read_colors(name, mapset, &colors);

    rtype = Rast_get_map_type(map);
    if (rtype == CELL_TYPE)
        voidc = (CELL *) cell_buf;
    else if (rtype == FCELL_TYPE)
        voidc = (FCELL *) fcell_buf;
    else if (rtype == DCELL_TYPE)
        voidc = (DCELL *) dcell_buf;

    i = 0;
    
    if(color_mode == 1 || color_mode == 2) {/* 32BIT ARGB COLOR IMAGE with transparency */
        for (row = 0; row < rows; row++) {
            Rast_get_row(map, (void *)voidc, row, rtype);
            Rast_lookup_colors((void *)voidc, red, green, blue, set,
                               cols, &colors, rtype);
                               
            alpha = (unsigned char)255;
            if ( color_mode == 1 && Rast_is_null_value( voidc, rtype ) )
            {
                alpha = (unsigned char)0;
            }
            for (col = 0; col < cols; col++) {
                /* Only little endian */
                if (set[col]) {
                    result[i++] = blue[col];
                    result[i++] = green[col];
                    result[i++] = red[col];
                    result[i++] = alpha;
                }
                else {
                    result[i++] = DEF_BLU;
                    result[i++] = DEF_GRN;
                    result[i++] = DEF_RED;
                    result[i++] = alpha;
                }
            }
        }
    }
    else {/* GREYSCALE IMAGE */
        for (row = 0; row < rows; row++) {
            Rast_get_row(map, (void *)voidc, row, rtype);
            Rast_lookup_colors((void *)voidc, red, green, blue, set,
                               cols, &colors, rtype);
            
            if(color_mode == 3) {
                for (col = 0; col < cols; col++) {
                    /*.33R+ .5G+ .17B */
                    result[i++] = ((red[col])   * 11 + 
                                   (green[col]) * 16 +
                                   (blue[col])  * 5) >> 5;
                }
            } else {
                for (col = 0; col < cols; col++) {
                    /*NTSC Y equation: .30R+ .59G+ .11B */
                    result[i++] = ((red[col])   * 19 + 
                                   (green[col]) * 38 + 
                                   (blue[col])  * 7) >> 6;
                }
            }
        }
    }

    Rast_free_colors(&colors);

    G_free(cell_buf);
    G_free(fcell_buf);
    G_free(dcell_buf);
    G_free(red);
    G_free(green);
    G_free(blue);
    G_free(set);
    Rast_close(map);
    
    return(1);
}
