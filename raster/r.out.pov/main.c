
/****************************************************************************
 *
 * MODULE:       r.out.pov
 * AUTHOR(S):    Klaus D. Meyer, <GEUM.tec geum.de> (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      converts a user-specified raster map layer into a
 *               height-field file for POVray
 * COPYRIGHT:    (C) 2000-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/*  
   Persistence of Vision (POV) raytracer can use a height-field
   defined in a Targa (.TGA) image file format where the RGB pixel
   values are 24 bits (3 bytes). A 16 bit unsigned integer height-field
   value is assigned as follows:
   RED    high byte
   GREEN  low byte
   BLUE   empty

   This code is based on code by W.D. Kirby who wrote DEM2XYZ in 1996.

   Author: Klaus Meyer, GEUM.tec GbR, eMail: GEUM.tec@geum.de
   Date: July 1998
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

void writeHeader(FILE * outf);
void processProfiles(int inputFile, FILE * outputF);

#define     YMAX     65536	/*  max length scan line */
#define     XMAX     65536	/*  max # of scan lines */

#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))

#define SW     0
#define NW     1
#define NE     2
#define SE     3


int base[XMAX];			/* array of base elevations */

double defaultElev = 0;		/* elevation for empty points */
int width, height;		/* height-field image width and height */
int hfType = 0;			/* height-field type */
double hfBias, hfNorm = 1.0;

double verticalScale = 1.0;	/* to stretch or shrink elevations */


double minValue = 50000., maxValue = -50000.;

char mapLabel[145];
int DEMlevel, elevationPattern, groundSystem, groundZone;
double projectParams[15];
int planeUnitOfMeasure, elevUnitOfMeasure, polygonSizes;
double groundCoords[4][2], elevBounds[2], localRotation;
int accuracyCode;
double spatialResolution[3];
int profileDimension[2];
int firstRow, lastRow;
int wcount = 0, hcount;


double deltaY;
char inText[24], **junk;

double eastMost, westMost, southMost, northMost;
int eastMostSample, westMostSample, southMostSample, northMostSample;
int rowCount, columnCount, r, c;
long int cellCount = 0;
int rowStr, rowEnd, colStr, colEnd;



int main(int argc, char *argv[])
{
    struct Cell_head region;
    struct Range range;
    CELL range_min, range_max;
    FILE *outf;
    char *outfilename;

    CELL *cell;
    char *name;
    int fd;
    int nrows, ncols;
    double bias;
    struct GModule *module;
    struct
    {
	struct Option *map;
	struct Option *tga;
	struct Option *hftype;
	struct Option *bias;
	struct Option *scaleFactor;
    } parm;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("export"));
    G_add_keyword(_("output"));
    module->description =
	_("Converts a raster map layer into a height-field file for POV-Ray.");

    /* Define the different options */
    parm.map = G_define_standard_option(G_OPT_R_INPUT);

    parm.tga = G_define_standard_option(G_OPT_F_OUTPUT);
    parm.tga->description =
	_("Name of output povray file (TGA height field file)");

    parm.hftype = G_define_option();
    parm.hftype->key = "hftype";
    parm.hftype->type = TYPE_INTEGER;
    parm.hftype->required = NO;
    parm.hftype->description =
	_("Height-field type (0=actual heights 1=normalized)");

    parm.bias = G_define_option();
    parm.bias->key = "bias";
    parm.bias->type = TYPE_DOUBLE;
    parm.bias->required = NO;
    parm.bias->description = _("Elevation bias");

    parm.scaleFactor = G_define_option();
    parm.scaleFactor->key = "scale";
    parm.scaleFactor->type = TYPE_DOUBLE;
    parm.scaleFactor->required = NO;
    parm.scaleFactor->description = _("Vertical scaling factor");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (parm.hftype->answer != NULL)
	sscanf(parm.hftype->answer, "%d", &hfType);

    if (parm.bias->answer != NULL)
	sscanf(parm.bias->answer, "%lf", &bias);

    if (parm.scaleFactor->answer != NULL)
	sscanf(parm.scaleFactor->answer, "%lf", &verticalScale);

    name = parm.map->answer;

    fd = Rast_open_old(name, "");

    outfilename = parm.tga->answer;
    if (outfilename == NULL)
	G_fatal_error(_("Invalid output filename <%s>"), outfilename);

    if (NULL == (outf = fopen(outfilename, "wb")))
	G_fatal_error(_("Unable to open output file <%s>"), outfilename);

    cell = Rast_allocate_c_buf();

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    if (nrows > YMAX || ncols > XMAX)
	G_fatal_error(_("Raster map is too big! Exceeds %d columns or %d rows"),
		      XMAX, YMAX);


    columnCount = ncols;
    rowCount = nrows;
    width = ncols;
    height = nrows;

    /* SW, NW, NE, SE corners */
    G_get_window(&region);

    eastMost = region.east;
    westMost = region.west;
    northMost = region.north;
    southMost = region.south;

    Rast_init_range(&range);
    Rast_read_range(name, "", &range);
    Rast_get_range_min_max(&range, &range_min, &range_max);
    if (range.min < 0 || range.max < 0)
	G_warning(_("Negative elevation values in input"));

    elevBounds[0] = range.min;
    elevBounds[1] = range.max;

    /* Normalize using max value of unsigned short integer (16 bit) */
    if (hfType == 1)
	hfNorm = 65535.0 / (elevBounds[1] + hfBias);

    spatialResolution[0] = region.ew_res;
    spatialResolution[1] = region.ew_res;
    spatialResolution[2] = region.ns_res;

    /* write TGA image file header */
    (void)writeHeader(outf);

    /* have to read everything */
    (void)processProfiles(fd, outf);

    fclose(outf);
    Rast_close(fd);

    exit(EXIT_SUCCESS);
}




void writeHeader(FILE * outputF)
{
    int i;

    /* Write TGA image header */
    for (i = 0; i < 10; i++)	/* 00, 00, 02, then 7 00's... */
	if (i == 2)
	    putc((short)i, outputF);
	else
	    putc(0, outputF);

    putc(0, outputF);		/* y origin set to "First_Line" */
    putc(0, outputF);

    putc((short)(width % 256), outputF);	/* write width and height */
    putc((short)(width / 256), outputF);
    putc((short)(height % 256), outputF);
    putc((short)(height / 256), outputF);
    putc(24, outputF);		/* 24 bits/pixel (16 million colors!) */
    putc(32, outputF);		/* Bitmask, pertinent bit: top-down raster */
}

/*
 * read profiles
 */

void processProfiles(int inputFile, FILE * outputF)
{
    CELL *cell;
    int c, r;
    double tempFloat;

    cell = Rast_allocate_c_buf();
    for (r = 0; r < rowCount; r++) {
	Rast_get_c_row(inputFile, cell, r);
	/* break; */

	for (c = 0; c < columnCount; c++) {
	    tempFloat = ((float)cell[c] * verticalScale) + hfBias;

	    /* Normalize */
	    tempFloat *= hfNorm;
	    if (tempFloat < 0.0)
		tempFloat = 0.0;
	    if (tempFloat > 65535.0)
		tempFloat = 65535.0;

	    if (tempFloat > maxValue)
		maxValue = tempFloat;
	    if (tempFloat < minValue)
		minValue = tempFloat;

	    /* write pixel */
	    putc((char)0, outputF);	/* Blue  empty     */
	    putc((char)((int)tempFloat % 256), outputF);	/* Green low byte  */
	    putc((char)((int)tempFloat / 256), outputF);	/* Red   high byte */
	}
	G_percent(r, rowCount, 2);
    }
    G_percent(r, rowCount, 2);	/* 100% \n */
}
