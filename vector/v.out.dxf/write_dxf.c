/* write_dxf.c is a file to facilitate the transfer of information to the dxf
 * format.  It attempts to be input nuetral (so when digit gets replaced ten
 * years from now it will still be good...).
 *
 * This file supports Version 10 of dxf.
 *
 * written by: Chuck Ehlschlaeger
 */

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include "global.h"

int dxf_open(char *filename)
{
    if ((dxf_fp = fopen(filename, "r")) != NULL) {
	fclose(dxf_fp);

	if (!overwrite)
	    G_fatal_error(_("The file '%s' already exists."), filename);

	G_warning(_("The file '%s' already exists and will be overwritten."),
		  filename);
    }
    if ((dxf_fp = fopen(filename, "w")) == NULL)
	G_fatal_error(_("%s: Cannot write dxf file."), filename);

    return 0;
}

int dxf_header(void)
{
    fprintf(dxf_fp, "  0\nSECTION\n  2\nHEADER\n");

    return 0;
}

int dxf_tables(void)
{
    fprintf(dxf_fp, "  0\nSECTION\n  2\nTABLES\n");

    return 0;
}

int dxf_blocks(void)
{
    fprintf(dxf_fp, "  0\nSECTION\n  2\nBLOCKS\n");

    return 0;
}

int dxf_entities(void)
{
    fprintf(dxf_fp, "  0\nSECTION\n  2\nENTITIES\n");

    return 0;
}

int dxf_endsec(void)
{
    fprintf(dxf_fp, "  0\nENDSEC\n");

    return 0;
}

int dxf_eof(void)
{
    fprintf(dxf_fp, "  0\nEOF\n");
    fclose(dxf_fp);

    return 0;
}

/* header stuff */

int dxf_limits(double top, double bottom, double right, double left)
{
    fprintf(dxf_fp, "  9\n$LIMMIN\n 10\n%f\n 20\n%f\n", left, bottom);
    fprintf(dxf_fp, "  9\n$LIMMAX\n 10\n%f\n 20\n%f\n", right, top);

    return 0;
}

/* tables stuff */

int dxf_linetype_table(int numlines)
{
    fprintf(dxf_fp, "  0\nTABLE\n  2\nLTYPE\n 70\n%6d\n", numlines);

    return 0;
}

int dxf_layer_table(int numlayers)
{
    fprintf(dxf_fp, "  0\nTABLE\n  2\nLAYER\n 70\n%6d\n", numlayers);

    return 0;
}

int dxf_endtable(void)
{
    fprintf(dxf_fp, "  0\nENDTAB\n");

    return 0;
}

int dxf_solidline(void)
{
    fprintf(dxf_fp, "  0\nLTYPE\n  2\nCONTINUOUS\n 70\n");
    fprintf(dxf_fp, "    64\n  3\nSolid line\n 72\n    65\n");
    fprintf(dxf_fp, " 73\n     0\n 40\n0.0\n");

    return 0;
}

int dxf_layer0(void)
{
    fprintf(dxf_fp, "  0\nLAYER\n  2\n0\n 70\n     0\n");
    fprintf(dxf_fp, " 62\n     7\n  6\nCONTINUOUS\n");

    return 0;
}

int dxf_layer(char *name, int color, char *linetype, int frozen)
{
    int is_frozen;

    if (frozen)
	is_frozen = 1;
    else
	is_frozen = 64;
    fprintf(dxf_fp, "  0\nLAYER\n  2\n%s\n 70\n", name);
    fprintf(dxf_fp, "%6d\n 62\n%6d\n  6\n%s\n", is_frozen, color, linetype);

    return 0;
}

/* entities */

int dxf_point(char *layer, double x, double y, double z)
{
    fprintf(dxf_fp, "0\nPOINT\n");
    fprintf(dxf_fp, "8\n%s\n", (layer));
    fprintf(dxf_fp, "10\n%f\n20\n%f\n30\n%f\n", x, y, z);

    return 0;
}

int dxf_polyline(char *layer)
{
    fprintf(dxf_fp, "0\nPOLYLINE\n");
    fprintf(dxf_fp, "8\n%s\n", (layer));
    fprintf(dxf_fp, "66\n1\n");
    /* fprintf(dxf_fp,"10\n0.0\n 20\n0.0\n 30\n0.0\n"); *//* ? */

    return 0;
}

int dxf_vertex(char *layer, double x, double y, double z)
{
    fprintf(dxf_fp, "0\nVERTEX\n");
    fprintf(dxf_fp, "8\n%s\n", layer);
    fprintf(dxf_fp, "10\n%f\n20\n%f\n 30\n%f\n", x, y, z);

    return 0;
}

int dxf_text(char *layer, double x, double y, double z, double size, int just,
	     char *text)
{
    fprintf(dxf_fp, "  0\nTEXT\n  8\n%s\n 10\n%f\n 20\n", layer, x);
    fprintf(dxf_fp, "%f\n 30\n%f\n 40\n%f\n  1\n%s\n", y, z, size, text);
    if (just)
	fprintf(dxf_fp, " 72\n%6d\n 11\n%f\n 21\n%f\n 31\n%f\n", just, x, y,
		z);

    return 0;
}

int dxf_poly_end(char *layer)
{
    fprintf(dxf_fp, "  0\nSEQEND\n  8\n%s\n", layer);

    return 0;
}
