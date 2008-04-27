/*  GVL3.c
    Volume access routines
    Tomas Paudits
    December 2003
*/

#include <grass/gis.h>
#include <grass/G3d.h>
#include <grass/gstypes.h>

/***********************************************************************/
int Gvl_load_colors_data(void **color_data, char *name)
{
    char *mapset;
    struct Colors *colors;

    if (NULL == (mapset = G_find_grid3(name,""))) {
        return (-1);
    }

    if (NULL == (colors = (struct Colors *) G_malloc(sizeof(struct Colors))))
         return (-1);

    if (0 > G3d_readColors(name, mapset, colors)) {
        G_free(colors);
        return (-1);
    }

    *color_data = colors;

    return (1);
}

/***********************************************************************/
int Gvl_unload_colors_data(void *color_data)
{
    if (!G_free_colors(color_data))
        return (-1);

    G_free(color_data);

    return (1);
}

/***********************************************************************/
int Gvl_get_color_for_value(void *color_data, float *value)
{
    int r, g, b;

    G_get_f_raster_color((FCELL *) value, &r, &g, &b, color_data);
    return ((r & 0xff) | ((g & 0xff) << 8) | ((b & 0xff) << 16));
}
