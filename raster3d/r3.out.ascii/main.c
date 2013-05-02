
/****************************************************************************
 *
 * MODULE:       r3.out.ascii 
 *   	    	
 * AUTHOR(S):    Original author 
 *		Mark Astley, Bill Brown, Soeren Gebbert
 * 		USA CERL started 4/4/96
 *
 * PURPOSE:      Converts a 3D raster map layer into an ASCII text file  
 *
 * COPYRIGHT:    (C) 2005 - 2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>

#define MAX(a,b) (a > b ? a : b)

/* structs */
typedef struct {
    struct Option *input, *output, *decimals, *null_val;
    struct Flag *header;
    struct Flag *row;
    struct Flag *depth;
    struct Flag *grass6;
    struct Flag *mask;
} paramType;

/* protos */
static void fatalError(char *errorMsg);
static void setParams();
static void getParams(char **input, char **output, int *decim);
static void writeHeaderString(FILE * fp, char *valueString, double value);
static void writeHeaderString2(FILE * fp, char *valueString, int value);
static void writeHeaderString3(FILE * fp, char *valueString, const char* value);
static FILE *openAscii(char *asciiFile, RASTER3D_Region region);
static void G3dToascii(FILE * fp, RASTER3D_Region region, int decim);

/* globals */
void *map = NULL;
paramType param;

/*---------------------------------------------------------------------------*/

/* Simple error handling routine, will eventually replace this with
 * RASTER3D_fatalError.
 */
void fatalError(char *errorMsg)
{
    if (map != NULL) {
        /* should unopen map here! */
        if (!Rast3d_close(map))
            fatalError(_("Unable to close 3D raster map"));

    }

    Rast3d_fatal_error(errorMsg);
}

/*---------------------------------------------------------------------------*/

/* Convenient way to set up the arguments we are expecting
 */
void setParams()
{
    param.input = G_define_option();
    param.input->key = "input";
    param.input->type = TYPE_STRING;
    param.input->required = YES;
    param.input->gisprompt = "old,grid3,3d-raster";
    param.input->multiple = NO;
    param.input->description = _("3D raster map to be converted to ASCII");

    param.output = G_define_standard_option(G_OPT_F_OUTPUT);
    param.output->required = NO;
    param.output->description = _("Name for ASCII output file");

    param.decimals = G_define_option();
    param.decimals->key = "dp";
    param.decimals->type = TYPE_INTEGER;
    param.decimals->required = NO;
    param.decimals->multiple = NO;
    param.decimals->answer = "8";
    param.decimals->options = "0-20";
    param.decimals->description = _("Number of decimal places for floats");

    param.null_val = G_define_option();
    param.null_val->key = "null";
    param.null_val->type = TYPE_STRING;
    param.null_val->required = NO;
    param.null_val->description = _("Char string to represent no data cell");
    param.null_val->answer = "*";

    param.header = G_define_flag();
    param.header->key = 'h';
    param.header->description = _("Suppress printing of header information");

    param.row = G_define_flag();
    param.row->key = 'r';
    param.row->description = _("Switch the row order in output from north->south to south->north");

    param.depth = G_define_flag();
    param.depth->key = 'd';
    param.depth->description = _("Switch the depth order in output from bottom->top to top->bottom");

    param.grass6 = G_define_flag();
    param.grass6->key = 'c';
    param.grass6->description = _("Print grass6 compatible format. Flags -d and -r are ignored.");

    param.mask = G_define_flag();
    param.mask->key = 'm';
    param.mask->description = _("Use 3D raster mask (if exists) with input map");
}

/*---------------------------------------------------------------------------*/

/* Set up the input and output file names from the user's responses
 */
void getParams(char **input, char **output, int *decim)
{
    *input = param.input->answer;
    *output = param.output->answer;
    sscanf(param.decimals->answer, "%d", decim);
}

/*---------------------------------------------------------------------------*/

/* This function is used to write parts of the header for the output
 * ASCII file.
 */
void writeHeaderString(FILE * fp, char *valueString, double value)
{
    static char format[100];

    G_snprintf(format, 100, "%s %%lf\n", valueString);
    if (fprintf(fp, format, value) < 0)
        fatalError("writeHeaderString: header value invalid");
}

/*---------------------------------------------------------------------------*/
void writeHeaderString2(FILE * fp, char *valueString, int value)
{
    static char format[100];

    G_snprintf(format, 100, "%s %%d\n", valueString);
    if (fprintf(fp, format, value) < 0)
        fatalError("writeHeaderString: header value invalid");
}

/*---------------------------------------------------------------------------*/
void writeHeaderString3(FILE * fp, char *valueString, const char* value)
{
    static char format[100];

    G_snprintf(format, 100, "%s %%s\n", valueString);
    if (fprintf(fp, format, value) < 0)
        fatalError("writeHeaderString: header value invalid");
}


/*---------------------------------------------------------------------------*/

/* Opens the output acsii file and writes the header.
 * Returns the file handle for the output file.
 */
FILE *openAscii(char *asciiFile, RASTER3D_Region region)
{
    FILE *fp;

    if (asciiFile) {
        fp = fopen(asciiFile, "w");
        if (fp == NULL) {
            perror(asciiFile);
            G_usage();
            exit(EXIT_FAILURE);
        }
    } else
        fp = stdout;

    if (!param.header->answer) {
        
        /* Do not print the new header in grass compatibility mode */
        if (!param.grass6->answer) {
            /* Write the version information */
            writeHeaderString3(fp, "version:", "grass7");
            /* Write the row and depth order information */
            if (!param.depth->answer && !param.row->answer)
                writeHeaderString3(fp, "order:", "nsbt");
            else if (param.depth->answer && !param.row->answer)
                writeHeaderString3(fp, "order:", "nstb");
            else if (!param.depth->answer && param.row->answer)
                writeHeaderString3(fp, "order:", "snbt");
            else if (param.depth->answer && param.row->answer)
                writeHeaderString3(fp, "order:", "sntb");
        }

        writeHeaderString(fp, "north:", region.north);
        writeHeaderString(fp, "south:", region.south);
        writeHeaderString(fp, "east:", region.east);
        writeHeaderString(fp, "west:", region.west);
        writeHeaderString(fp, "top:", region.top);
        writeHeaderString(fp, "bottom:", region.bottom);
        writeHeaderString2(fp, "rows:", region.rows);
        writeHeaderString2(fp, "cols:", region.cols);
        writeHeaderString2(fp, "levels:", region.depths);
    }

    return fp;
}

/*---------------------------------------------------------------------------*/
/* This function does all the work.  Basically, we just output the
 * source G3d file one layer at a time.
 *//* * */

void G3dToascii(FILE * fp, RASTER3D_Region region, int decim)
{
    DCELL dvalue;
    FCELL fvalue;
    int x, y, z;
    int rows, cols, depths, typeIntern;
    int col, row, depth;

    rows = region.rows;
    cols = region.cols;
    depths = region.depths;

    typeIntern = Rast3d_tile_type_map(map);

    for (z = 0; z < depths; z++) {
        G_percent(z, depths, 1);
        for (y = 0; y < rows; y++) { /* g3d rows count from south to north */
            for (x = 0; x < cols; x++) {

                /* From west to east */
                col = x;
                /* The default is to write rows from north to south
                   to be r.in.ascii compatible 
                 */
                row = y;
                /* From bottom to the top */
                depth = z;

                /* Write rows from south to north */
                if (param.row->answer)
                    row = rows - y - 1;

                /* write XY layer from top to bottom */
                if (param.depth->answer)
                    depth = depths - z - 1;

                /* Get the data and resample if nessessary */

                if (typeIntern == FCELL_TYPE) {
                    
                    Rast3d_get_value(map, col, row, depth, &fvalue, FCELL_TYPE);
                    
                    if (Rast3d_is_null_value_num(&fvalue, FCELL_TYPE))
                        fprintf(fp, "%s ", param.null_val->answer);
                    else
                        fprintf(fp, "%.*f ", decim, fvalue);
                } else {
                    
                    Rast3d_get_value(map, col, row, depth, &dvalue, DCELL_TYPE);
                    
                    if (Rast3d_is_null_value_num(&dvalue, DCELL_TYPE))
                        fprintf(fp, "%s ", param.null_val->answer);
                    else
                        fprintf(fp, "%.*lf ", decim, dvalue);
                }
            }
            fprintf(fp, "\n");
        }
    }
    G_percent(1, 1, 1);
    G_percent_reset();
}

/*---------------------------------------------------------------------------*/

/* Main function: open the input and output files, then call
 * G3dtoascii.
 */
int main(int argc, char *argv[])
{
    char *input, *output;
    int decim;
    RASTER3D_Region region;
    FILE *fp;
    int changemask = 0;
    struct GModule *module;

    /* Initialize GRASS */
    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("voxel"));
    G_add_keyword(_("export"));
    module->description =
        _("Converts a 3D raster map layer into a ASCII text file.");

    /* Get parameters from user */
    setParams();

    /* Have GRASS get inputs */
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* Parse input parameters */
    getParams(&input, &output, &decim);

    if (param.grass6->answer) {
        param.depth->answer = 0;
        param.row->answer = 0;
    }

    if (NULL == G_find_raster3d(input, ""))
        Rast3d_fatal_error(_("3D raster map <%s> not found"), input);

    /* Initiate the default settings */
    Rast3d_init_defaults();

    /* Figure out the current region settings */
    Rast3d_get_window(&region);

    /* Open the map and use XY cache mode */
    map = Rast3d_open_cell_old(input, G_find_raster3d(input, ""), &region,
                          RASTER3D_TILE_SAME_AS_FILE, RASTER3D_USE_CACHE_DEFAULT);

    if (map == NULL)
        Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"), input);

    /* Open the output ascii file */
    fp = openAscii(output, region);

    /*if requested set the Mask on */
    if (param.mask->answer) {
        if (Rast3d_mask_file_exists()) {
            changemask = 0;
            if (Rast3d_mask_is_off(map)) {
                Rast3d_mask_on(map);
                changemask = 1;
            }
        }
    }

    /* Now barf out the contents of the map in ascii form */
    G3dToascii(fp, region, decim);

    /*We set the Mask off, if it was off bevor */
    if (param.mask->answer) {
        if (Rast3d_mask_file_exists())
            if (Rast3d_mask_is_on(map) && changemask)
                Rast3d_mask_off(map);
    }

    /* Close files and exit */
    if (!Rast3d_close(map))
        fatalError(_("Unable to close 3D raster map"));

    if (output)
        if (fclose(fp))
            fatalError(_("Unable to close new ASCII file"));

    return 0;
}

/*---------------------------------------------------------------------------*/
