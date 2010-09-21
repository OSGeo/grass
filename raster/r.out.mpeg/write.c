/* Written by Bill Brown, USACERL (brown@zorro.cecer.army.mil)
 * May, 1994
 *
 * This code is in the public domain. Specifically, we give to the public
 * domain all rights for future licensing of the source code, all resale
 * rights, and all publishing rights.
 * 
 * We ask, but do not require, that the following message be included in
 * all derived works:
 *     "Portions developed at the US Army Construction Engineering 
 *     Research Laboratories, Champaign, Illinois."
 * 
 * USACERL GIVES NO WARRANTY, EXPRESSED OR IMPLIED,
 * FOR THE SOFTWARE AND/OR DOCUMENTATION PROVIDED, INCLUDING, WITHOUT
 * LIMITATION, WARRANTY OF MERCHANTABILITY AND WARRANTY OF FITNESS FOR A
 * PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "rom_proto.h"


#ifndef USE_PPM

/*******************************************************/
void write_ycc(char *tr, char *tg, char *tb, int nrows, int ncols,
	       int *y_rows, int *y_cols, char *filename)
{
    register int x, y;
    register unsigned char *dy0, *dy1;
    register unsigned char *dcr, *dcb;
    register unsigned char src0[6], src1[6];
    static int rows, cols;
    static int first = 1;
    static float mult299[256], mult587[256], mult114[256];
    static float mult16874[256], mult33126[256], mult5[256];
    static float mult41869[256], mult08131[256];
    static unsigned char *cy, *cr, *cb;
    FILE *ofp;

    *y_rows = nrows;
    *y_cols = ncols;

    /* 16-pixel align */
    *y_rows &= ~0x0f;
    *y_cols &= ~0x0f;

    if (first) {
	register int index;

	rows = *y_rows;
	cols = *y_cols;

	for (index = 0; index <= 255; index++) {
	    mult299[index] = index * 0.29900;
	    mult587[index] = index * 0.58700;
	    mult114[index] = index * 0.11400;
	    mult16874[index] = -0.16874 * index;
	    mult33126[index] = -0.33126 * index;
	    mult5[index] = index * 0.50000;
	    mult41869[index] = -0.41869 * index;
	    mult08131[index] = -0.08131 * index;
	}

	cy = (unsigned char *)G_malloc(rows * cols * sizeof(unsigned char));
	cr = (unsigned char *)G_malloc((rows / 2) * (cols / 2)
				       * sizeof(unsigned char));
	cb = (unsigned char *)G_malloc((rows / 2) * (cols / 2)
				       * sizeof(unsigned char));

	first = 0;
    }

    if (*y_rows != rows || *y_cols != cols)
	G_fatal_error(_("Size mismatch error!"));

    for (y = 0; y < rows - 1; y += 2) {
	dy0 = &cy[y * cols];
	dy1 = &cy[(y + 1) * cols];
	dcr = &cr[(cols / 2) * (y / 2)];
	dcb = &cb[(cols / 2) * (y / 2)];

	for (x = 0; x < cols - 1; x += 2, dy0 += 2, dy1 += 2, dcr++, dcb++) {
	    src0[0] = tr[y * ncols + x];
	    src0[1] = tg[y * ncols + x];
	    src0[2] = tb[y * ncols + x];
	    src0[3] = tr[y * ncols + x + 1];
	    src0[4] = tg[y * ncols + x + 1];
	    src0[5] = tb[y * ncols + x + 1];

	    src0[0] = tr[(y + 1) * ncols + x];
	    src0[1] = tg[(y + 1) * ncols + x];
	    src0[2] = tb[(y + 1) * ncols + x];
	    src0[3] = tr[(y + 1) * ncols + x + 1];
	    src0[4] = tg[(y + 1) * ncols + x + 1];
	    src0[5] = tb[(y + 1) * ncols + x + 1];

	    *dy0 = (mult299[*src0] + mult587[src0[1]] + mult114[src0[2]]);

	    *dy1 = (mult299[*src1] + mult587[src1[1]] + mult114[src1[2]]);

	    dy0[1] = (mult299[src0[3]] + mult587[src0[4]] + mult114[src0[5]]);

	    dy1[1] = (mult299[src1[3]] + mult587[src1[4]] + mult114[src1[5]]);

	    *dcb = ((mult16874[*src0] +
		     mult33126[src0[1]] +
		     mult5[src0[2]] +
		     mult16874[*src1] +
		     mult33126[src1[1]] +
		     mult5[src1[2]] +
		     mult16874[src0[3]] +
		     mult33126[src0[4]] +
		     mult5[src0[5]] +
		     mult16874[src1[3]] +
		     mult33126[src1[4]] + mult5[src1[5]]) / 4) + 128;

	    *dcr = ((mult5[*src0] +
		     mult41869[src0[1]] +
		     mult08131[src0[2]] +
		     mult5[*src1] +
		     mult41869[src1[1]] +
		     mult08131[src1[2]] +
		     mult5[src0[3]] +
		     mult41869[src0[4]] +
		     mult08131[src0[5]] +
		     mult5[src1[3]] +
		     mult41869[src1[4]] + mult08131[src1[5]]) / 4) + 128;
	}
    }

    if (NULL == (ofp = fopen(filename, "wb")))
	G_fatal_error(_("Unable to open output file"));

    for (y = 0; y < rows; y++)
	fwrite(cy + (y * cols), 1, cols, ofp);

    for (y = 0; y < rows / 2; y++)
	fwrite(cb + ((y / 2) * cols), 1, cols / 2, ofp);

    for (y = 0; y < rows / 2; y++)
	fwrite(cr + ((y / 2) * cols), 1, cols / 2, ofp);

    fclose(ofp);
}
#endif


/*******************************************************/
void write_ppm(char *tr, char *tg, char *tb, int nrows, int ncols,
	       int *y_rows, int *y_cols, char *filename)
{
    register int x, y;
    static int rows, cols;
    static int first = 1;
    FILE *ofp;

    *y_rows = nrows;
    *y_cols = ncols;

    /* 16-pixel align */
    *y_rows &= ~0x0f;
    *y_cols &= ~0x0f;

    if (first) {
	rows = *y_rows;
	cols = *y_cols;
	first = 0;
    }

    if (*y_rows != rows || *y_cols != cols)
	G_fatal_error(_("Size mismatch error!"));

    if (NULL == (ofp = fopen(filename, "w")))
	G_fatal_error(_("Unable to open output file"));

    fprintf(ofp, "P6\n");
    /* Magic number meaning rawbits, 24bit color to ppm format */
    fprintf(ofp, "%d %d\n", cols, rows);
    /* width & height */
    fprintf(ofp, "255\n");
    /* max intensity val */

    for (y = 0; y < rows; y++) {
	for (x = 0; x < cols; x++) {
	    putc(*tr++, ofp);
	    putc(*tg++, ofp);
	    putc(*tb++, ofp);
	}

	tr += (ncols - cols);
	tg += (ncols - cols);
	tb += (ncols - cols);
    }

    fclose(ofp);
}


/*******************************************************/
void write_params(char *mpfilename, char *yfiles[], char *outfile,
		  int frames, int quality, int y_rows, int y_cols, int fly)
{
    FILE *fp;
    char dir[1000], *enddir;
    int i, dirlen = 0;

    if (NULL == (fp = fopen(mpfilename, "w")))
	G_fatal_error(_("Unable to create temporary files."));

    if (!fly) {
	strcpy(dir, yfiles[0]);
	enddir = strrchr(dir, '/');

	if (enddir) {
	    *enddir = '\0';
	    dirlen = strlen(dir) + 1;
	}
    }

    switch (quality) {
    case 1:
	fprintf(fp, "PATTERN         IBPB\n");
	break;
    case 2:
    case 3:
	fprintf(fp, "PATTERN         IBBPBB\n");
	break;
    case 4:
    case 5:
	fprintf(fp, "PATTERN         IBBPBBPBB\n");
	break;
    default:
	fprintf(fp, "PATTERN         IBBPBB\n");
	break;
    }
    fprintf(fp, "FORCE_ENCODE_LAST_FRAME\n");
    fprintf(fp, "OUTPUT          %s\n", outfile);
    fprintf(fp, "\n");

    if (!fly)
	fprintf(fp, "INPUT_DIR       %s\n", dir);
    else
	fprintf(fp, "INPUT_DIR       %s\n", "in=");
    fprintf(fp, "INPUT\n");

    if (!fly)
	for (i = 0; i < frames; i++)
	    fprintf(fp, "%s\n", yfiles[i] + dirlen);
    else
	for (i = 0; i < frames; i++)
	    fprintf(fp, "%s\n", yfiles[i]);

    fprintf(fp, "END_INPUT\n");

#ifdef USE_PPM
    fprintf(fp, "BASE_FILE_FORMAT        PPM\n");
#else

    if (!fly)
	fprintf(fp, "BASE_FILE_FORMAT        YUV\n");
    else
	fprintf(fp, "BASE_FILE_FORMAT        PPM\n");
#endif

#ifndef USE_PPM
    if (!fly)
	fprintf(fp, "YUV_SIZE   %dx%d\n", y_cols, y_rows);
#endif

    if (!fly)
	fprintf(fp, "INPUT_CONVERT   *\n");
    else
	fprintf(fp, "INPUT_CONVERT   r.out.ppm -q * out=-\n");

    fprintf(fp, "GOP_SIZE        30\n");
    fprintf(fp, "SLICES_PER_FRAME  1\n");
    fprintf(fp, "\n");
    fprintf(fp, "PIXEL           HALF\n");
    fprintf(fp, "RANGE           8\n");
    fprintf(fp, "\n");
    fprintf(fp, "PSEARCH_ALG     TWOLEVEL\n");
    fprintf(fp, "BSEARCH_ALG     CROSS2\n");
    fprintf(fp, "\n");

    switch (quality) {
    case 1:
	fprintf(fp, "IQSCALE         5\n");
	fprintf(fp, "PQSCALE         8\n");
	fprintf(fp, "BQSCALE         12\n");
	break;
    case 2:
	fprintf(fp, "IQSCALE         6\n");
	fprintf(fp, "PQSCALE         10\n");
	fprintf(fp, "BQSCALE         14\n");
	break;
    case 4:
	fprintf(fp, "IQSCALE         8\n");
	fprintf(fp, "PQSCALE         14\n");
	fprintf(fp, "BQSCALE         20\n");
	break;
    case 5:
	fprintf(fp, "IQSCALE         9\n");
	fprintf(fp, "PQSCALE         16\n");
	fprintf(fp, "BQSCALE         24\n");
	break;
    default:
	fprintf(fp, "IQSCALE         7\n");
	fprintf(fp, "PQSCALE         12\n");
	fprintf(fp, "BQSCALE         16\n");
	break;
    }
    fprintf(fp, "\n");
    fprintf(fp, "REFERENCE_FRAME DECODED\n");

    fclose(fp);
}


/*******************************************************/
void clean_files(char *file, char *files[], int num)
{
    int i;

    remove(file);
    for (i = 0; i < num; i++)
	remove(files[i]);
}

