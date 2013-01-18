
/***************************************************************************
 *
 * MODULE:       r3.in.ascii
 *
 * AUTHOR(S):    Roman Waupotitsch, Michael Shapiro, Helena Mitasova, Bill Brown, 
 * 		Lubos Mitas, Jaro Hofierka, Soeren Gebbert 
 *
 * PURPOSE:      Convert a 3D ASCII raster text file into a (binary) 3D raster map layer 
 *
 * COPYRIGHT:    (C) 2005 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>

#define ROW_ORDER_NORTH_TO_SOUTH 1
#define ROW_ORDER_SOUTH_TO_NORTH 2
#define DEPTH_ORDER_BOTTOM_TO_TOP 3
#define DEPTH_ORDER_TOP_TO_BOTTOM 4

/*- prototypes --------------------------------------------------------------*/

static void fatalError(char *errorMsg); /*Simple Error message */

static void setParams(); /*Fill the paramType structure */

/*Puts the userdefined parameters into easier handable variables */
static void getParams(char **input, char **output, int *convertNull,
                      char *nullValue);

/*reads a g3d ascii-file headerfile-string */
static void readHeaderString(FILE * fp, char *valueString, double *value);

static FILE *openAscii(char *asciiFile, RASTER3D_Region * region); /*open the g3d ascii file */

/*This function does all the work, it reads the values from the g3d ascii-file and put 
   it into an g3d-map */
static void asciiToG3d(FILE * fp, RASTER3D_Region * region, int convertNull,
                       char *nullValue);

/*---------------------------------------------------------------------------*/

/* global variables */
void *map = NULL;
int rowOrder;
int depthOrder;

extern void *Rast3d_open_new_param();

/*---------------------------------------------------------------------------*/
void fatalError(char *errorMsg)
{
    if (map != NULL) {
        /* should unopen map here! */
        Rast3d_close(map);
    }

    Rast3d_fatal_error(errorMsg);
}

/*---------------------------------------------------------------------------*/

typedef struct {
    struct Option *input, *output, *nv;
} paramType;

static paramType param;

static void setParams()
{
    param.input = G_define_standard_option(G_OPT_F_INPUT);
    param.input->required = YES;
    param.input->description = _("Name of input file to be imported");

    param.output = G_define_standard_option(G_OPT_R3_OUTPUT);

    param.nv = G_define_option();
    param.nv->key = "nv";
    param.nv->type = TYPE_STRING;
    param.nv->required = NO;
    param.nv->multiple = NO;
    param.nv->answer = "*";
    param.nv->description =  /* TODO: '*' or 'none' in the msg ?? */
	_("String representing NULL value data cell (use 'none' if no such value)");
}

/*---------------------------------------------------------------------------*/

void
getParams(char **input, char **output, int *convertNull, char *nullValue)
{
    *input = param.input->answer;
    *output = param.output->answer;
    *convertNull = (strcmp(param.nv->answer, "none") != 0);
    if (*convertNull)
        if (sscanf(param.nv->answer, "%s", nullValue) != 1)
            fatalError("getParams: NULL-value value invalid");

    G_debug(3, "getParams: input: %s, output: %s", *input, *output);
}

/*---------------------------------------------------------------------------*/

void readHeaderString(FILE * fp, char *valueString, double *value)
{
    static char format[100];
    char line_buff[1024];

    /* to avoid buffer overflows we use G_snprintf */
    G_snprintf(format, 100, "%s %%lf", valueString);
    G_getl2(line_buff, 1024, fp);
    if (sscanf(line_buff, format, value) != 1) {
        G_debug(3, "bad value for [%s]", valueString);
        fatalError("readHeaderString: header value invalid");
    }
}

/*---------------------------------------------------------------------------*/

FILE *openAscii(char *asciiFile, RASTER3D_Region * region)
{
    FILE *fp;
    double tmp;
    char buff[1024];
    char line_buff[1024];

    G_debug(3, "openAscii: opens the ascii file and reads the header");

    fp = fopen(asciiFile, "r");
    if (fp == NULL) {
        perror(asciiFile);
        G_usage();
        exit(EXIT_FAILURE);
    }

    /* Initialize the default order */
    rowOrder = ROW_ORDER_NORTH_TO_SOUTH;
    depthOrder = DEPTH_ORDER_BOTTOM_TO_TOP;

    /* Read the first line and check for grass version */
    G_getl2(line_buff, 1024, fp);

    /* First check for new ascii format*/
    if (sscanf(line_buff, "version: %s", buff) == 1) {
        G_message("Found version information: %s\n", buff);
        if (G_strcasecmp(buff, "grass7") == 0) {

            /* Parse the row and depth order */
            G_getl2(line_buff, 1024, fp);
            if (sscanf(line_buff, "order: %s", buff) != 1)
                fatalError("Unable to parse the row and depth order");

            if (G_strcasecmp(buff, "nsbt") == 0) {
                rowOrder = ROW_ORDER_NORTH_TO_SOUTH;
                depthOrder = DEPTH_ORDER_BOTTOM_TO_TOP;
                G_message("Found north -> south, bottom -> top order (nsbt)");
            }
            if (G_strcasecmp(buff, "snbt") == 0) {
                rowOrder = ROW_ORDER_SOUTH_TO_NORTH;
                depthOrder = DEPTH_ORDER_BOTTOM_TO_TOP;
                G_message("Found south -> north, bottom -> top order (snbt)");
            }
            if (G_strcasecmp(buff, "nstb") == 0) {
                rowOrder = ROW_ORDER_NORTH_TO_SOUTH;
                depthOrder = DEPTH_ORDER_TOP_TO_BOTTOM;
                G_message("Found north -> south, top -> bottom order (nstb)");
            }
            if (G_strcasecmp(buff, "sntb") == 0) {
                rowOrder = ROW_ORDER_SOUTH_TO_NORTH;
                depthOrder = DEPTH_ORDER_TOP_TO_BOTTOM;
                G_message("Found south -> north, top -> bottom order (sntb)");
            }
        } else {
            G_fatal_error(_("Unsupported GRASS version %s"), buff);
        }
    } else {
        /* Rewind the stream if no grass version info found */
        rewind(fp);
    }

    Rast3d_get_window(region);

    readHeaderString(fp, "north:", &(region->north));
    readHeaderString(fp, "south:", &(region->south));
    readHeaderString(fp, "east:", &(region->east));
    readHeaderString(fp, "west:", &(region->west));
    readHeaderString(fp, "top:", &(region->top));
    readHeaderString(fp, "bottom:", &(region->bottom));
    readHeaderString(fp, "rows:", &tmp);
    region->rows = (int) tmp;
    readHeaderString(fp, "cols:", &tmp);
    region->cols = (int) tmp;
    readHeaderString(fp, "levels:", &tmp);
    region->depths = (int) tmp;

    return fp;
}

/*---------------------------------------------------------------------------*/

#define MAX(a,b) (a > b ? a : b)

void
asciiToG3d(FILE * fp, RASTER3D_Region * region, int convertNull, char *nullValue)
{
    int x, y, z;
    int col, row, depth;
    double value;
    char buff[256];
    int tileX, tileY, tileZ;

    Rast3d_get_tile_dimensions_map(map, &tileX, &tileY, &tileZ);
    Rast3d_min_unlocked(map, RASTER3D_USE_CACHE_X);

    Rast3d_autolock_on(map);
    Rast3d_unlock_all(map);
    G_message(_("Loading data ...  (%dx%dx%d)"), region->cols, region->rows,
              region->depths);

    G_debug(3,
            "asciiToG3d: writing the 3D raster map, with rows %i cols %i depths %i",
            region->rows, region->cols, region->depths);

    for (z = 0; z < region->depths; z++) {
        G_percent(z, region->depths, 1);

        if ((z % tileZ) == 0)
            Rast3d_unlock_all(map);

        for (y = 0; y < region->rows; y++) /* go south to north */
            for (x = 0; x < region->cols; x++) {

                /* From west to east */
                col = x;
                /* The default is to read rows from north to south */
                row = y;
                /* From bottom to the top */
                depth = z;

                /* Read rows as from south to north */
                if (rowOrder == ROW_ORDER_SOUTH_TO_NORTH)
                    row = region->rows - y - 1;

                /* Read XY layer from top to bottom */
                if (depthOrder == DEPTH_ORDER_TOP_TO_BOTTOM)
                    depth = region->depths - z - 1;

                if (fscanf(fp, "%s", buff) != 1) {
                    if (feof(fp))
                        G_warning(_("End of file reached while still loading data."));
                    G_debug(3,
                            "missing data at col=%d row=%d depth=%d last_value=[%.4f]",
                            x + 1, y + 1, z + 1, value);
                    fatalError("asciiToG3d: read failed");
                }

                /* Check for null value */
                if (convertNull && strncmp(buff, nullValue, strlen(nullValue)) == 0) {
                    Rast3d_set_null_value(&value, 1, DCELL_TYPE);
                } else {
                    if (sscanf(buff, "%lf", &value) != 1) {
                        G_warning(_("Invalid value detected"));
                        G_debug(1, "invalid value at col=%d row=%d depth=%d last_value=[%s]",
                                x + 1, y + 1, z + 1, buff);
                        fatalError("asciiToG3d: read failed");
                    }
                }
                /* Write the data */
                Rast3d_put_double(map, col, row, depth, value);
            }

        if (!Rast3d_flush_tiles_in_cube(map,
                                  0, 0, MAX(0, depth - tileZ),
                                  region->rows - 1, region->cols - 1, depth))
            fatalError("asciiTog3d: error flushing tiles");
    }

    if (fscanf(fp, "%lf", &value) == 1) {
        G_warning(_("Data exists in input file after fully importing "
                    "expected data.  [%.4f ...]"), value);
    }

    if (!Rast3d_flush_all_tiles(map))
        fatalError("asciiTog3d: error flushing tiles");

    Rast3d_autolock_off(map);
    Rast3d_unlock_all(map);

    G_percent(1, 1, 1);
}

/*---------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    char *input, *output;
    int convertNull;
    char nullValue[256];
    int useTypeDefault, type, useLzwDefault, doLzw, useRleDefault, doRle;
    int usePrecisionDefault, precision, useDimensionDefault, tileX, tileY,
        tileZ;
    RASTER3D_Region region;
    FILE *fp;
    struct GModule *module;
    struct History history;

    map = NULL;

    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("voxel"));
    G_add_keyword(_("import"));
    module->description =
        _("Converts a 3D ASCII raster text file into a (binary) 3D raster map.");

    setParams();
    Rast3d_set_standard3d_input_params();

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    getParams(&input, &output, &convertNull, nullValue);
    if (!Rast3d_get_standard3d_params(&useTypeDefault, &type,
                                 &useLzwDefault, &doLzw,
                                 &useRleDefault, &doRle,
                                 &usePrecisionDefault, &precision,
                                 &useDimensionDefault, &tileX, &tileY,
                                 &tileZ))
        fatalError("Error getting standard parameters");

    Rast3d_init_defaults();

    fp = openAscii(input, &region);

    /*Open the new RASTER3D map */
    map = Rast3d_open_new_param(output, RASTER3D_TILE_SAME_AS_FILE, RASTER3D_USE_CACHE_XY,
                           &region,
                           type, doLzw, doRle, precision, tileX, tileY,
                           tileZ);

    if (map == NULL)
        fatalError(_("Unable to open 3D raster map"));

    /*Create the new RASTER3D Map */
    asciiToG3d(fp, &region, convertNull, nullValue);

    if (!Rast3d_close(map))
        fatalError(_("Unable to close 3D raster map"));

    /* write input name to map history */
    Rast3d_read_history(output, G_mapset(), &history);
    Rast_set_history(&history, HIST_DATSRC_1, input);
    Rast3d_write_history(output, &history);

    map = NULL;
    if (fclose(fp))
        fatalError(_("Unable to close ASCII file"));

    return EXIT_SUCCESS;
}
