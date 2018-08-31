
/**********************************************************************
 *	togif - 
 *		Convert an IRIS image to GIF format.  Converts b/w and 
 *	color images to 8 bit per pixel GIF format.  Color images
 *	are dithered with a 4 by 4 dither matrix.  GIF image files 
 *	may be uuencoded, and sent over the network.
 *
 *			Paul Haeberli @ Silicon Graphics - 1989
 *
 *    * Modified Jan 1995 - Made parallelizable via elimination of
 *    * global variables.
 **********************************************************************
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <grass/gis.h>
#include "togif.h"
#define GIFGAMMA	(1.5)	/* smaller makes output image darker */
#define MAXCOLORS 256
#define CBITS    12

typedef int (*ifunptr) (int x, int y, vgl_GIFWriter * dataPtr);


/***************************************************************************
 * These routines interface the non-GIF data to the GIF black box (GIFENCOD)
 ***************************************************************************
 */
static int getgifpix2(int x, int y, vgl_GIFWriter * dataPtr);
static void getrow2(unsigned long *buffer, short *row, unsigned short width,
		    unsigned short rownum, unsigned short comp);
static void gammawarp(short *sbuf, float gam, int n, vgl_GIFWriter * dataPtr);
static short **makedittab(int levels, int mult, int add);
static int ditherrow(unsigned short *r, unsigned short *g, unsigned short *b,
		     short *wp, int n, int y, vgl_GIFWriter * dataPtr);

/******************************************************************************
 * GIF black box routines
 ******************************************************************************
 */
static void BumpPixel(vgl_GIFWriter * dataPtr);
static int GIFNextPixel(ifunptr getpixel, vgl_GIFWriter * dataPtr);
static int GIFEncode(FILE * fp, int GWidth, int GHeight, int GInterlace,
		     int Background, int BitsPerPixel, int Red[], int Green[],
		     int Blue[], ifunptr GetPixel, vgl_GIFWriter * dataPtr);
static void Putword(int w, FILE * fp);

/******************************************************************************
 * GIF compression black box routines
 ******************************************************************************
 */
static void compress(int init_bits, FILE * outfile, ifunptr ReadValue,
		     vgl_GIFWriter * dataPtr);
static void output(code_int code, vgl_GIFWriter * dataPtr);
static void cl_block(vgl_GIFWriter * dataPtr);
static void cl_hash(register count_int hsize, vgl_GIFWriter * dataPtr);
static void writeerr();
static void flush_char(vgl_GIFWriter * dataPtr);
static void char_out(int c, vgl_GIFWriter * dataPtr);
static void char_init(vgl_GIFWriter * dataPtr);

/************************ vgl_GIFWriterBegin() ******************************/
vgl_GIFWriter *vgl_GIFWriterBegin(void)
{
    vgl_GIFWriter *gifwriter;

    gifwriter = (vgl_GIFWriter *) G_malloc(sizeof(vgl_GIFWriter));
    return gifwriter;
}

/************************** vgl_GIFWriterEnd() ********************************/
void vgl_GIFWriterEnd(vgl_GIFWriter * gifwriter)
{
    G_free(gifwriter);
}

/********************** vgl_GIFWriterWriteGIFFile() ****************************/
void vgl_GIFWriterWriteGIFFile(vgl_GIFWriter * gifwriter,
			       unsigned long *buffer, int xsize, int ysize,
			       int bwflag, FILE * outf)
{
    short r, g, b;
    int i;
    int bpp;
    int gifcolors;
    int rmap[MAXCOLORS];
    int gmap[MAXCOLORS];
    int bmap[MAXCOLORS];

    memset(gifwriter, '\0', sizeof(vgl_GIFWriter));
    gifwriter->maxbits = CBITS;	/* user settable max # bits/code */
    gifwriter->maxmaxcode = (code_int) 1 << CBITS;	/* should NEVER generate this code */
    gifwriter->hsize = HSIZE;
    gifwriter->in_count = 1;

    gifwriter->xsize = xsize;
    gifwriter->ysize = ysize;
    gifwriter->iscolor = !bwflag;
    gifwriter->buffer = buffer;
    if (gifwriter->iscolor) {
	gifcolors = 256;
	bpp = 8;
	for (i = 0; i < gifcolors; i++) {
	    r = (i >> 0) & 0x7;
	    g = (i >> 3) & 0x7;
	    b = (i >> 6) & 0x3;
	    rmap[i] = (255 * r) / 7;
	    gmap[i] = (255 * g) / 7;
	    bmap[i] = (255 * b) / 3;
	}
    }
    else {
	gifcolors = 16;
	bpp = 4;
	for (i = 0; i < gifcolors; i++) {
	    rmap[i] = (255 * i) / (gifcolors - 1);
	    gmap[i] = (255 * i) / (gifcolors - 1);
	    bmap[i] = (255 * i) / (gifcolors - 1);
	}
    }


    gifwriter->currow = -1;
    GIFEncode(outf, xsize, ysize, 1, 0, bpp, rmap, gmap, bmap, getgifpix2,
	      gifwriter);

}

/************************** getgifpix2() ********************************/
static int getgifpix2(int x, int y, vgl_GIFWriter * dataPtr)
{
    int pix;

    if (dataPtr->iscolor) {
	if (dataPtr->currow != y) {
	    getrow2(dataPtr->buffer, dataPtr->rbuf, dataPtr->xsize,
		    dataPtr->ysize - 1 - y, 0);
	    gammawarp(dataPtr->rbuf, 1.0 / GIFGAMMA, dataPtr->xsize, dataPtr);
	    getrow2(dataPtr->buffer, dataPtr->gbuf, dataPtr->xsize,
		    dataPtr->ysize - 1 - y, 1);
	    gammawarp(dataPtr->gbuf, 1.0 / GIFGAMMA, dataPtr->xsize, dataPtr);
	    getrow2(dataPtr->buffer, dataPtr->bbuf, dataPtr->xsize,
		    dataPtr->ysize - 1 - y, 2);
	    gammawarp(dataPtr->bbuf, 1.0 / GIFGAMMA, dataPtr->xsize, dataPtr);
	    ditherrow((unsigned short *)(dataPtr->rbuf),
		      (unsigned short *)(dataPtr->gbuf),
		      (unsigned short *)(dataPtr->bbuf), dataPtr->obuf,
		      dataPtr->xsize, y, dataPtr);
	    dataPtr->currow = y;
	}
	pix = dataPtr->obuf[x];
    }
    else {
	if (dataPtr->currow != y) {
	    getrow2(dataPtr->buffer, dataPtr->rbuf, dataPtr->xsize,
		    dataPtr->ysize - 1 - y, 0);
	    gammawarp(dataPtr->rbuf, 1.0 / GIFGAMMA, dataPtr->xsize, dataPtr);
	    ditherrow((unsigned short *)(dataPtr->rbuf),
		      (unsigned short *)(dataPtr->rbuf),
		      (unsigned short *)(dataPtr->rbuf), dataPtr->obuf,
		      dataPtr->xsize, y, dataPtr);
	    dataPtr->currow = y;
	}
	pix = dataPtr->obuf[x] & 0xf;
    }
    return pix;
}

/************************** getrow2() ********************************/
static void getrow2(unsigned long *buffer, short *row, unsigned short width,
		    unsigned short rownum, unsigned short comp)
{
    unsigned short i;
    unsigned long *ptr = buffer + width * rownum;

    switch (comp) {
    case 0:
	for (i = 0; i < width; i++) {
	    row[i] = *ptr & 0xff;
	    ptr++;
	}
	break;
    case 1:
	for (i = 0; i < width; i++) {
	    row[i] = (*ptr >> 8) & 0xff;
	    ptr++;
	}
	break;
    case 2:
	for (i = 0; i < width; i++) {
	    row[i] = (*ptr >> 16) & 0xff;
	    ptr++;
	}
	break;
    }
}

/********************** gammawarp() ***************************/
static void gammawarp(short *sbuf, float gam, int n, vgl_GIFWriter * dataPtr)
{
    int i;
    float f;

    if (gam != dataPtr->curgamma) {
	for (i = 0; i < 256; i++)
	    dataPtr->gamtab[i] = 255 * pow(i / 255.0, gam) + 0.5;
	dataPtr->curgamma = gam;
    }
    while (n--) {
	*sbuf = dataPtr->gamtab[*sbuf];
	sbuf++;
    }
}

/***************************************************************************
 *  	dithering code follows
 */
#define XSIZE	4
#define YSIZE	4
#define TOTAL		(XSIZE*YSIZE)
#define WRAPY(y)	((y)%YSIZE)
#define WRAPX(x)	((x)%XSIZE)

static short dithmat[YSIZE][XSIZE] = {
    0, 8, 2, 10,
    12, 4, 14, 6,
    3, 11, 1, 9,
    15, 7, 13, 5,
};

/************************** makedittab() ********************************/
static short **makedittab(int levels, int mult, int add)
{
    register int val;
    register int nshades;
    register int i, j, k;
    register int matval, tabval;
    short **tab;

    nshades = XSIZE * YSIZE * (levels - 1) + 1;
    tab = (short **)G_malloc(YSIZE * sizeof(short *));
    for (j = 0; j < YSIZE; j++) {
	tab[j] = (short *)G_malloc(XSIZE * 256 * sizeof(short));
	for (i = 0; i < XSIZE; i++) {
	    matval = dithmat[i][j];
	    for (k = 0; k < 256; k++) {
		val = (nshades * k) / 255;
		if (val == nshades)
		    val = nshades - 1;
		if ((val % TOTAL) > matval)
		    tabval = (val / TOTAL) + 1;
		else
		    tabval = (val / TOTAL);
		tabval *= mult;
		tabval += add;
		tab[j][256 * i + k] = tabval;
	    }
	}
    }
    return tab;
}

/************************** ditherrow() ********************************/
static int ditherrow(unsigned short *r, unsigned short *g, unsigned short *b,
		     short *wp, int n, int y, vgl_GIFWriter * dataPtr)
{
    short *rbase;
    short *gbase;
    short *bbase;

    if (!dataPtr->rtab) {
	if (dataPtr->iscolor) {
	    dataPtr->rtab = makedittab(8, 1, 0);
	    dataPtr->gtab = makedittab(8, 8, 0);
	    dataPtr->btab = makedittab(4, 64, 0);
	}
	else {
	    dataPtr->rtab = makedittab(16, 1, 0);
	    dataPtr->gtab = makedittab(2, 16, 0);
	    dataPtr->btab = makedittab(2, 32, 0);
	}
    }
    rbase = dataPtr->rtab[WRAPY(y)];
    gbase = dataPtr->gtab[WRAPY(y)];
    bbase = dataPtr->btab[WRAPY(y)];
    while (n) {
	if (n >= XSIZE) {
	    *wp++ = rbase[*r++ + 0] + gbase[*g++ + 0] + bbase[*b++ + 0];
	    *wp++ = rbase[*r++ + 256] + gbase[*g++ + 256] + bbase[*b++ + 256];
	    *wp++ = rbase[*r++ + 512] + gbase[*g++ + 512] + bbase[*b++ + 512];
	    *wp++ = rbase[*r++ + 768] + gbase[*g++ + 768] + bbase[*b++ + 768];
	    n -= XSIZE;
	}
	else {
	    *wp++ = rbase[*r++] + gbase[*g++] + bbase[*b++];
	    rbase += 256;
	    gbase += 256;
	    bbase += 256;
	    n--;
	}
    }
    return 1;
}


/*****************************************************************************
 * SCARY GIF code follows . . . . sorry.
 *
 * Based on GIFENCOD by David Rowley <mgardi@watdscu.waterloo.edu>.A
 * Lempel-Zim compression based on "compress".
 *
 *****************************************************************************
 */

/*****************************************************************************
 *
 * GIFENCODE.C    - GIF Image compression interface
 *
 * GIFEncode( FName, GHeight, GWidth, GInterlace, Background,
 *            BitsPerPixel, Red, Green, Blue, GetPixel )
 *
 *****************************************************************************/

#define TRUE 1
#define FALSE 0


/************************** BumpPixel() ********************************/
/*
 * Bump the 'curx' and 'cury' to point to the next pixel
 */
static void BumpPixel(vgl_GIFWriter * dataPtr)
{
    dataPtr->curx++;
    if (dataPtr->curx == dataPtr->xsize) {
	dataPtr->curx = 0;
	if (!dataPtr->interlace) {
	    dataPtr->cury++;
	}
	else {
	    switch (dataPtr->pass) {
	    case 0:
		dataPtr->cury += 8;
		if (dataPtr->cury >= dataPtr->ysize) {
		    dataPtr->pass++;
		    dataPtr->cury = 4;
		}
		break;
	    case 1:
		dataPtr->cury += 8;
		if (dataPtr->cury >= dataPtr->ysize) {
		    dataPtr->pass++;
		    dataPtr->cury = 2;
		}
		break;
	    case 2:
		dataPtr->cury += 4;
		if (dataPtr->cury >= dataPtr->ysize) {
		    dataPtr->pass++;
		    dataPtr->cury = 1;
		}
		break;
	    case 3:
		dataPtr->cury += 2;
		break;
	    }
	}
    }
}

/************************** GIFNextPixel() ********************************/
/*
 * Return the next pixel from the image
 */
static int GIFNextPixel(ifunptr getpixel, vgl_GIFWriter * dataPtr)
{
    int r;

    if (dataPtr->countDown == 0)
	return EOF;
    dataPtr->countDown--;
    r = (*getpixel) (dataPtr->curx, dataPtr->cury, dataPtr);
    BumpPixel(dataPtr);
    return r;
}

/************************** GIFEncode () ********************************/
/*
 * public GIFEncode
 */
static int GIFEncode(FILE * fp, int GWidth, int GHeight, int GInterlace,
		     int Background, int BitsPerPixel, int Red[], int Green[],
		     int Blue[], ifunptr GetPixel, vgl_GIFWriter * dataPtr)
{
    int B;
    int RWidth, RHeight;
    int LeftOfs, TopOfs;
    int Resolution;
    int ColorMapSize;
    int InitCodeSize;
    int i;

    dataPtr->cur_bits = 0;

    dataPtr->interlace = GInterlace;
    ColorMapSize = 1 << BitsPerPixel;
    RWidth = dataPtr->xsize;
    RHeight = dataPtr->ysize;
    LeftOfs = TopOfs = 0;
    Resolution = BitsPerPixel;

    dataPtr->countDown = (long)(dataPtr->xsize) * (long)(dataPtr->ysize);
    dataPtr->pass = 0;
    if (BitsPerPixel <= 1)
	InitCodeSize = 2;
    else
	InitCodeSize = BitsPerPixel;
    dataPtr->curx = dataPtr->cury = 0;
    fwrite("GIF87a", 1, 6, fp);
    Putword(RWidth, fp);
    Putword(RHeight, fp);
    B = 0x80;			/* Yes, there is a color map */
    B |= (Resolution - 1) << 5;
    B |= (BitsPerPixel - 1);
    fputc(B, fp);
    fputc(Background, fp);
    fputc(0, fp);
    for (i = 0; i < ColorMapSize; i++) {
	fputc(Red[i], fp);
	fputc(Green[i], fp);
	fputc(Blue[i], fp);
    }
    fputc(',', fp);
    Putword(LeftOfs, fp);
    Putword(TopOfs, fp);
    Putword(dataPtr->xsize, fp);
    Putword(dataPtr->ysize, fp);
    if (dataPtr->interlace)
	fputc(0x40, fp);
    else
	fputc(0x00, fp);
    fputc(InitCodeSize, fp);
    compress(InitCodeSize + 1, fp, GetPixel, dataPtr);
    fputc(0, fp);
    fputc(';', fp);
    fclose(fp);
    return 1;
}

/************************** Putword () ********************************/
/*
 * Write out a word to the GIF file
 */
static void Putword(int w, FILE * fp)
{

    fputc(w & 0xff, fp);
    fputc((w / 256) & 0xff, fp);
}


/***************************************************************************
 *
 *  GIFCOMPR.C       - GIF Image compression routines
 *
 *  Lempel-Ziv compression based on 'compress'.  GIF modifications by
 *  David Rowley (mgardi@watdcsu.waterloo.edu)
 *
 ***************************************************************************/


/*
 *
 * GIF Image compression - modified 'compress'
 *
 * Based on: compress.c - File compression ala IEEE Computer, June 1984.
 *
 * By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
 *              Jim McKie               (decvax!mcvax!jim)
 *              Steve Davies            (decvax!vax135!petsd!peora!srd)
 *              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
 *              James A. Woods          (decvax!ihnp4!ames!jaw)
 *              Joe Orost               (decvax!vax135!petsd!joe)
 *
 */
#include <ctype.h>
#include <grass/gis.h>

#define ARGVAL() (*++(*argv) || (--argc && *++argv))

# define MAXCODE(n_bits)        (((code_int) 1 << (n_bits)) - 1)

#define HashTabOf(i)    dataPtr->htab[i]
#define CodeTabOf(i)    dataPtr->codetab[i]


/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**CBITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */
#define tab_prefixof(i) CodeTabOf(i)
#define tab_suffixof(i)        ((char_type *)(dataPtr->htab))[i]
#define de_stack               ((char_type *)&tab_suffixof((code_int)1<<CBITS))



/************************** compress () ********************************
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */
static void compress(int init_bits, FILE * outfile, ifunptr ReadValue,
		     vgl_GIFWriter * dataPtr)
{
    register long fcode;
    register code_int i = 0;
    register int c;
    register code_int ent;
    register code_int disp;
    register code_int hsize_reg;
    register int hshift;

    /*
     * Set up the globals:  g_init_bits - initial number of bits
     *                      g_outfile   - pointer to output file
     */
    dataPtr->g_init_bits = init_bits;
    dataPtr->g_outfile = outfile;
    /*
     * Set up the necessary values
     */
    dataPtr->offset = 0;
    dataPtr->out_count = 0;
    dataPtr->clear_flg = 0;
    dataPtr->in_count = 1;
    dataPtr->maxcode = MAXCODE(dataPtr->n_bits = dataPtr->g_init_bits);
    dataPtr->clearCode = (1 << (init_bits - 1));
    dataPtr->EOFCode = dataPtr->clearCode + 1;
    dataPtr->free_ent = dataPtr->clearCode + 2;
    char_init(dataPtr);
    ent = GIFNextPixel(ReadValue, dataPtr);
    hshift = 0;
    for (fcode = (long)(dataPtr->hsize); fcode < 65536L; fcode *= 2L)
	hshift++;
    hshift = 8 - hshift;	/* set hash code range bound */
    hsize_reg = dataPtr->hsize;
    cl_hash((count_int) hsize_reg, dataPtr);	/* clear hash table */
    output((code_int) (dataPtr->clearCode), dataPtr);
    while ((c = GIFNextPixel(ReadValue, dataPtr)) != EOF) {
	dataPtr->in_count++;
	fcode = (long)(((long)c << dataPtr->maxbits) + ent);
	/* i = (((code_int)c << hshift) ~ ent);  */
	i = (((code_int) c << hshift) ^ ent);	/* xor hashing */
	if (HashTabOf(i) == fcode) {
	    ent = CodeTabOf(i);
	    continue;
	}
	else if ((long)HashTabOf(i) < 0)	/* empty slot */
	    goto nomatch;
	disp = hsize_reg - i;	/* secondary hash (after G. Knott) */
	if (i == 0)
	    disp = 1;
      probe:
	if ((i -= disp) < 0)
	    i += hsize_reg;
	if (HashTabOf(i) == fcode) {
	    ent = CodeTabOf(i);
	    continue;
	}
	if ((long)HashTabOf(i) > 0)
	    goto probe;
      nomatch:
	output((code_int) ent, dataPtr);
	dataPtr->out_count++;
	ent = c;
	if (dataPtr->free_ent < dataPtr->maxmaxcode) {
	    CodeTabOf(i) = dataPtr->free_ent++;	/* code -> hashtable */
	    HashTabOf(i) = fcode;
	}
	else
	    cl_block(dataPtr);
    }
    /*
     * Put out the final code.
     */
    output((code_int) ent, dataPtr);
    dataPtr->out_count++;
    output((code_int) (dataPtr->EOFCode), dataPtr);
    return;
}

/*****************************************************************
 * TAG( output )
 *
 * Output the given code.
 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< (long)wordsize - 1.
 * Outputs:
 *      Outputs code to the file.
 * Assumptions:
 *      Chars are 8 bits long.
 * Algorithm:
 *      Maintain a CBITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static unsigned long masks[] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
    0x001F, 0x003F, 0x007F, 0x00FF,
    0x01FF, 0x03FF, 0x07FF, 0x0FFF,
    0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF
};

static void output(code_int code, vgl_GIFWriter * dataPtr)
{
    dataPtr->cur_accum &= masks[dataPtr->cur_bits];
    if (dataPtr->cur_bits > 0)
	dataPtr->cur_accum |= ((long)code << dataPtr->cur_bits);
    else
	dataPtr->cur_accum = code;
    dataPtr->cur_bits += dataPtr->n_bits;
    while (dataPtr->cur_bits >= 8) {
	char_out((unsigned int)(dataPtr->cur_accum & 0xff), dataPtr);
	dataPtr->cur_accum >>= 8;
	dataPtr->cur_bits -= 8;
    }

    /*
     * If the next entry is going to be too big for the code size,
     * then increase it, if possible.
     */
    if (dataPtr->free_ent > dataPtr->maxcode || dataPtr->clear_flg) {
	if (dataPtr->clear_flg) {
	    dataPtr->maxcode = MAXCODE(dataPtr->n_bits =
				       dataPtr->g_init_bits);
	    dataPtr->clear_flg = 0;
	}
	else {
	    dataPtr->n_bits++;
	    if (dataPtr->n_bits == dataPtr->maxbits)
		dataPtr->maxcode = dataPtr->maxmaxcode;
	    else
		dataPtr->maxcode = MAXCODE(dataPtr->n_bits);
	}
    }
    if (code == dataPtr->EOFCode) {
	/*
	 * At EOF, write the rest of the buffer.
	 */
	while (dataPtr->cur_bits > 0) {
	    char_out((unsigned int)(dataPtr->cur_accum & 0xff), dataPtr);
	    dataPtr->cur_accum >>= 8;
	    dataPtr->cur_bits -= 8;
	}
	flush_char(dataPtr);
	fflush(dataPtr->g_outfile);
	if (ferror(dataPtr->g_outfile))
	    writeerr();
    }
}

/********************** cl_block() ***************************/
/*
 * table clear for block compress
 */
static void cl_block(vgl_GIFWriter * dataPtr)
{
    cl_hash((count_int) (dataPtr->hsize), dataPtr);
    dataPtr->free_ent = dataPtr->clearCode + 2;
    dataPtr->clear_flg = 1;
    output((code_int) (dataPtr->clearCode), dataPtr);
}

/********************** cl_hash() ***************************/
/* reset code table */
static void cl_hash(register count_int hsize, vgl_GIFWriter * dataPtr)
{
    register count_int *htab_p;
    register long i;
    register long m1 = -1;

    htab_p = dataPtr->htab + hsize;

    i = hsize - 16;
    do {			/* might use Sys V memset(3) here */
	*(htab_p - 16) = m1;
	*(htab_p - 15) = m1;
	*(htab_p - 14) = m1;
	*(htab_p - 13) = m1;
	*(htab_p - 12) = m1;
	*(htab_p - 11) = m1;
	*(htab_p - 10) = m1;
	*(htab_p - 9) = m1;
	*(htab_p - 8) = m1;
	*(htab_p - 7) = m1;
	*(htab_p - 6) = m1;
	*(htab_p - 5) = m1;
	*(htab_p - 4) = m1;
	*(htab_p - 3) = m1;
	*(htab_p - 2) = m1;
	*(htab_p - 1) = m1;
	htab_p -= 16;
    } while ((i -= 16) >= 0);
    for (i += 16; i > 0; i--)
	*--htab_p = m1;
}

/********************** writeerr() ***************************/
static void writeerr()
{
}

/******************************************************************************
 *
 * GIF Specific routines used by compression stuff
 *
 ******************************************************************************/

/********************** char_init() ***************************/
/*
 * Set up the 'byte output' routine
 */
static void char_init(vgl_GIFWriter * dataPtr)
{
    dataPtr->a_count = 0;
}

/*
 * Define the storage for the packet accumulator
 */

/********************** char_out() ***************************/
/*
 * Add a character to the end of the current packet, and if it is 254
 * characters, flush the packet to disk.
 */
static void char_out(int c, vgl_GIFWriter * dataPtr)
{
    dataPtr->accum[dataPtr->a_count++] = c;
    if (dataPtr->a_count >= 254)
	flush_char(dataPtr);
}

/********************** flush_char() ***************************/
/*
 * Flush the packet to disk, and reset the accumulator
 */
static void flush_char(vgl_GIFWriter * dataPtr)
{
    if (dataPtr->a_count > 0) {
	fputc(dataPtr->a_count, dataPtr->g_outfile);
	fwrite(dataPtr->accum, 1, dataPtr->a_count, dataPtr->g_outfile);
	dataPtr->a_count = 0;
    }
}

/*
   main(int argc,char *argv[] )
   {
   FILE *of;
   int xsize, ysize,bwflag;
   unsigned long *buffer;
   int i;
   int j;
   vgl_GIFWriter *gifwriter;

   if(argc<3) {
   fprintf(stderr,"usage: togif image.rgb image.gif\n");
   exit(1);
   }
   gifwriter = vgl_GIFWriterBegin();

   xsize = ysize = 200;
   buffer = (unsigned long *)G_malloc ((xsize)*(ysize) * sizeof(unsigned long));
   for(i = 0; i < ysize; i++)
   {
   for(j = 0; j < xsize; j++)
   {


   buffer[j + i * xsize] =
   i | j << 8 | (0xff<<24);
   }

   }
   bwflag = 0;

   of = fopen(argv[2],"w");
   if(!of) {
   fprintf(stderr,"togif: can't open output image [%s]\n",argv[2]);
   exit(1);
   }
   vgl_GIFWriterWriteGIFFile(gifwriter,buffer,xsize,ysize,bwflag,of);
   vgl_GIFWriterEnd(gifwriter);
   fclose(of);
   exit(0);
   }
 */
