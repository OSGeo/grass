/****************************************************************************
*
* MODULE:       r3.out.ascii 
*   	    	
* AUTHOR(S):    Original author 
*		Mark Astley, Bill Brown
* 		USA CERL started 4/4/96
*
* PURPOSE:      Converts a 3D raster map layer into an ASCII text file  
*
* COPYRIGHT:    (C) 2005 by the GRASS Development Team
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
#include <grass/G3d.h>
#include <grass/glocale.h>

#define MAX(a,b) (a > b ? a : b)

/* structs */
typedef struct
{
  struct Option *input, *output, *decimals, *null_val;
  struct Flag *header;
  struct Flag *mask;
} paramType;

/* protos */
void fatalError (char *errorMsg);
void setParams ();
void getParams (char **input, char **output, int *decim);
void writeHeaderString (FILE * fp, char *valueString, double value);
void writeHeaderString2 (FILE * fp, char *valueString, int value);
FILE *openAscii (char *asciiFile, G3D_Region region);
void G3dToascii (FILE * fp, G3D_Region region, int decim);

/* globals */
void *map = NULL;
paramType param;

/*---------------------------------------------------------------------------*/
/* Simple error handling routine, will eventually replace this with
 * G3D_fatalError.
 */
void
fatalError (char *errorMsg)
{
  if (map != NULL)
    {
      /* should unopen map here! */
    if (!G3d_closeCell (map))
       fatalError (_("Error closing 3d raster map"));

    }

  G3d_fatalError (errorMsg);
}

/*---------------------------------------------------------------------------*/
/* Convenient way to set up the arguments we are expecting
 */
void
setParams ()
{
  param.input = G_define_option ();
  param.input->key = "input";
  param.input->type = TYPE_STRING;
  param.input->required = YES;
  param.input->gisprompt = "old,grid3,3d-raster";
  param.input->multiple = NO;
  param.input->description = _("3d raster map to be converted to ASCII");

  param.output = G_define_option ();
  param.output->key = "output";
  param.output->type = TYPE_STRING;
  param.output->gisprompt = "new_file,file,output";
  param.output->required = NO;
  param.output->description = _("Name for ASCII output file");

  param.decimals = G_define_option ();
  param.decimals->key = "dp";
  param.decimals->type = TYPE_INTEGER;
  param.decimals->required = NO;
  param.decimals->multiple = NO;
  param.decimals->answer = "8";
  param.decimals->options = "0-20";
  param.decimals->description = _("Number of decimal places for floats");

  param.null_val = G_define_option ();
  param.null_val->key = "null";
  param.null_val->type = TYPE_STRING;
  param.null_val->required = NO;
  param.null_val->description = _("Char string to represent no data cell");
  param.null_val->answer = "*";

  param.header = G_define_flag ();
  param.header->key = 'h';
  param.header->description = _("Suppress printing of header information");

  param.mask = G_define_flag ();
  param.mask->key = 'm';
  param.mask->description = _("Use G3D mask (if exists) with input map");
}

/*---------------------------------------------------------------------------*/
/* Set up the input and output file names from the user's responses
 */
void
getParams (char **input, char **output, int *decim)
{
  *input = param.input->answer;
  *output = param.output->answer;
  sscanf (param.decimals->answer, "%d", decim);
}

/*---------------------------------------------------------------------------*/
/* This function is used to write parts of the header for the output
 * ASCII file.
 */
void
writeHeaderString (FILE * fp, char *valueString, double value)
{
  static char format[100];

  sprintf (format, "%s %%lf\n", valueString);
  if (fprintf (fp, format, value) < 0)
    fatalError ("writeHeaderString: header value invalid");
}

/*---------------------------------------------------------------------------*/
void
writeHeaderString2 (FILE * fp, char *valueString, int value)
{
  static char format[100];

  sprintf (format, "%s %%d\n", valueString);
  if (fprintf (fp, format, value) < 0)
    fatalError ("writeHeaderString: header value invalid");
}

/*---------------------------------------------------------------------------*/
/* Opens the output acsii file and writes the header.
 * Returns the file handle for the output file.
 */
FILE *
openAscii (char *asciiFile, G3D_Region region)
{
  FILE *fp;

  if (asciiFile)
    {
      fp = fopen (asciiFile, "w");
      if (fp == NULL)
	{
	  perror (asciiFile);
	  G_usage ();
	  exit (-1);
	}
    }
  else
    fp = stdout;

  if (!param.header->answer)
    {
      writeHeaderString (fp, "north:", region.north);
      writeHeaderString (fp, "south:", region.south);
      writeHeaderString (fp, "east:", region.east);
      writeHeaderString (fp, "west:", region.west);
      writeHeaderString (fp, "top:", region.top);
      writeHeaderString (fp, "bottom:", region.bottom);
      writeHeaderString2 (fp, "rows:", region.rows);
      writeHeaderString2 (fp, "cols:", region.cols);
      writeHeaderString2 (fp, "levels:", region.depths);
    }

  return fp;
}

/*---------------------------------------------------------------------------*/
/* This function does all the work.  Basically, we just output the
 * source G3d file one layer at a time.
				  *//* * */
void
G3dToascii (FILE * fp, G3D_Region region, int decim)
{
  double d1 = 0;
  double *d1p;
  float *f1p;
  int x, y, z;
  int rows, cols, depths, typeIntern;

   /*AV*/
/* BEGIN OF ORIGINAL CODE */
/*
  G3d_getCoordsMap (map, &rows, &cols, &depths);
*/
/* END OF ORIGINAL CODE */
     /*AV*/
/* BEGIN OF MY CODE */
  rows = region.rows;
  cols = region.cols;
  depths = region.depths;
/* END OF MY CODE */

  typeIntern = G3d_tileTypeMap (map);

  d1p = &d1;
  f1p = (float *) &d1;

  for (z = 0; z < depths; z++)
    for (y = rows - 1; y >= 0; y--)
      {				/* north to south */
	for (x = 0; x < cols; x++)
	  {
	    G3d_getValue (map, x, y, z, d1p, typeIntern);
	    if (typeIntern == FCELL_TYPE)
	      {
		if (G3d_isNullValueNum (f1p, FCELL_TYPE))
		  fprintf (fp, "%s ", param.null_val->answer);
		else
		  fprintf (fp, "%.*f ", decim, *f1p);
	      }
	    else
	      {
		if (G3d_isNullValueNum (&d1, DCELL_TYPE))
		  fprintf (fp, "%s ", param.null_val->answer);
		else
		  fprintf (fp, "%.*lf ", decim, d1);
	      }
	  }
	fprintf (fp, "\n");
      }
}

/*---------------------------------------------------------------------------*/
/* Main function: open the input and output files, then call
 * G3dtoascii.
 */
int
main (int argc, char *argv[])
{
  char *input, *output;
  int decim;
  G3D_Region region;
  FILE *fp;
  int changemask = 0;
  struct GModule *module;

  /* Initialize GRASS */
  G_gisinit (argv[0]);
  module = G_define_module ();
  module->keywords = _("raster3d, voxel, export");
    module->description = _("Converts a 3D raster map layer into an ASCII text file");

  /* Get parameters from user */
  setParams ();

  /* Have GRASS get inputs */
  if (G_parser (argc, argv))
    exit (-1);

  /* Parse input parameters */
  getParams (&input, &output, &decim);

  if (NULL == G_find_grid3 (input, ""))
      G3d_fatalError (_("Requested 3d raster map not found"));

/*  map = G3d_openCellOld(input, G_find_grid3(input, ""), G3D_DEFAULT_WINDOW,
			G3D_TILE_SAME_AS_FILE,
			G3D_NO_CACHE);*/
  /* using cache mode due to bug */
  map = G3d_openCellOld (input, G_find_grid3 (input, ""), G3D_DEFAULT_WINDOW,
			 G3D_TILE_SAME_AS_FILE, G3D_USE_CACHE_DEFAULT);
  if (map == NULL)
    G3d_fatalError (_("Error opening 3d raster map"));

  /* Figure out the region from the map */
/*  G3d_getRegionStructMap(map, &region);*/
  G3d_getWindow (&region);

  /* Open the output ascii file */
  fp = openAscii (output, region);
  
  /*if requested set the Mask on */
  if (param.mask->answer)
    {
      if (G3d_maskFileExists ())
	{
	  changemask = 0;
	  if (G3d_maskIsOff (map))
	    {
	      G3d_maskOn (map);
	      changemask = 1;
	    }
	}
    }

  /* Now barf out the contents of the map in ascii form */
  G3dToascii (fp, region, decim);

  /*We set the Mask off, if it was off bevor */
  if (param.mask->answer)
    {
      if (G3d_maskFileExists ())
	if (G3d_maskIsOn (map) && changemask)
	  G3d_maskOff (map);
    }

  /* Close files and exit */
  if (!G3d_closeCell (map))
    fatalError (_("Error closing 3d raster map"));

  map = NULL;
  if (output)
    if (fclose (fp))
      fatalError (_("Error closing new ASCII file"));

  return 0;
}

/*---------------------------------------------------------------------------*/
