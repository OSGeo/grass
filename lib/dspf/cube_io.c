#include <stdlib.h>
#include <grass/gis.h>
#include "viz.h"

static unsigned char Buffer[10000];	/* buffer for outputting data to file */

/*
 **  Buffer Format:
 **    n_thresholds                       //  char                      //
 **    Jump cnt                           //  short                     //
 **    n_polys        [n_thresholds]      //  char  (nybble?)           //
 **    thresh_indexes [n_thresholds]      //  char                      //
 **    poly_info      [n_thresholds]      //  char v[3][3];n[3][3];     //
 **
 **   if (n_thresholds < 0) then -n_threshlds == number of consecutive cubes
 **     on current row  that do NOT contain any threshold info, and thus any
 **     data space in draw file.
 **   If val[ n_thresholds(i) ] < 0  then next byte is  
 **       'n_thresholds(i+(-n_threholds(i)))'
 **
 **   BUT, this code will simply place a 0 in 1st byte, and send it on to
 *       lower routine that writes out compressed data.
 */

int write_cube(Cube_data * Cube,	/* array of poly info  by threshold */
	       int cur_x, file_info * headfax)
{
    register int i, j;
    register int size;		/* final size of data written */
    register int offset1;	/* pointer to n_polys */
    register int offset2;	/* pointer to thresh_indexes */
    register int offset3 = 0;	/* pointer to poly_info */
    poly_info *Poly_info;
    int t_cnt;

    t_cnt = Cube->n_thresh;

    Buffer[0] = t_cnt;

    if (t_cnt) {
	offset1 = 3;		/* pointer to n_polys */
	offset2 = 3 + t_cnt;	/* pointer to thresh_indexes */
	offset3 = 3 + t_cnt + t_cnt;	/* pointer to poly_info */

	/*poly_size = sizeof (poly_info) * t_cnt; */

	for (i = 0; i < Cube->n_thresh; i++) {	/* n_thresholds loop */
	    Buffer[offset1++] = Cube->data[i].npoly;
	    Buffer[offset2++] = Cube->data[i].t_ndx;	/* THRESHOLD INDEX */

	    for (j = 0; j < Cube->data[i].npoly; j++) {
		Poly_info = &(Cube->data[i].poly[j]);
		/*memcpy (Buffer[offset3], Cube->data[i].poly_info,poly_size); */
		Buffer[offset3++] = Poly_info->v1[0];
		Buffer[offset3++] = Poly_info->v1[1];
		Buffer[offset3++] = Poly_info->v1[2];
		Buffer[offset3++] = Poly_info->v2[0];
		Buffer[offset3++] = Poly_info->v2[1];
		Buffer[offset3++] = Poly_info->v2[2];
		Buffer[offset3++] = Poly_info->v3[0];
		Buffer[offset3++] = Poly_info->v3[1];
		Buffer[offset3++] = Poly_info->v3[2];
		Buffer[offset3++] = Poly_info->n1[0];
		Buffer[offset3++] = Poly_info->n1[1];
		Buffer[offset3++] = Poly_info->n1[2];

		/* DEBUG */
		if (headfax->linefax.litmodel > 1) {	/* 3 normals */
		    Buffer[offset3++] = Poly_info->n2[0];
		    Buffer[offset3++] = Poly_info->n2[1];
		    Buffer[offset3++] = Poly_info->n2[2];
		    Buffer[offset3++] = Poly_info->n3[0];
		    Buffer[offset3++] = Poly_info->n3[1];
		    Buffer[offset3++] = Poly_info->n3[2];
		}
	    }
	}
	size = offset3 - 3;	/* 3 is 1st 3 bytes header */
	Buffer[1] = (size >> 8) & 0xff;	/* write short Big-endian */
	Buffer[2] = size & 0xff;
    }

    /*fprintf(stderr,"before write_cube_buffer\n"); */
    write_cube_buffer(Buffer, offset3, cur_x, headfax);	/* write it out to file */

    return 0;
}

/*
 **  Still have to add code to build index table 
 **   Also I am going to incorporate this into build_output before we're done
 */
int write_cube_buffer(unsigned char *Buffer, int size,
		      int cur_x, file_info * headfax)
{
    static int num_zero = 0;
    unsigned char junk;

    if (!Buffer[0]) {
	num_zero++;
	if (num_zero == 126 || cur_x == headfax->xdim - 2) {
	    junk = 0x80 | num_zero;
	    fwrite(&junk, 1, 1, headfax->dspfoutfp);
	    num_zero = 0;
	}
    }
    else {
	/* first write out zero data */
	if (num_zero) {
	    junk = 0x80 | num_zero;
	    fwrite(&junk, 1, 1, headfax->dspfoutfp);
	    num_zero = 0;
	}

	/* then the current buffer */
	fwrite(Buffer, 1, size, headfax->dspfoutfp);
    }

    return 0;
}

static long fsize = 0;
static char *fptr = NULL;

/*
 ** expects headfax->dspfinfp to be pointing to current cube
 **  i.e. already searched up to this point  (allowing of course
 **  for 0 data already read in 
 **
 **  returns num_thresholds  or 0 for no data  or  -1 on error
 **
 **  expects linefax and headfax to be filled in.
 */
int read_cube(Cube_data * Cube, file_info * headfax)
{
    register int offset1, offset2, offset3;
    int t_cnt;
    int ret;
    int i, j, size;
    unsigned char inchar;
    poly_info *Poly_info;
    static int first = 1;
    FILE *fp;

    static int zeros_left = 0;	/* move this out if a seek routine is written */

    fp = headfax->dspfinfp;
    first = !fsize;
    if (first)
	zeros_left = 0;

    while (first) {		/* use while instead of if to utilize 'break' !! */
	/* try reading the entire file into memory */
	long start, stop, i;
	int ret;

	first = 0;

	start = G_ftell(fp);
	G_fseek(fp, 0L, 2);
	stop = G_ftell(fp);
	fsize = stop - start + 1;
	G_fseek(fp, start, 0);
	if (fptr) {
	    free(fptr);
	    fptr = NULL;
	}
	if (NULL == (fptr = malloc(fsize))) {
	     /*DEBUG*/ fprintf(stderr, "Malloc failed\n");
	    fsize = 0;
	    break;
	}

	for (i = 0; ret = fread(fptr + i, 1, 10240, fp); i += ret) ;
    }

    if (zeros_left) {
	--zeros_left;
	return Cube->n_thresh = 0;
    }

    my_fread(&inchar, 1, 1, fp);	/* use signed char */
    if (inchar & 0x80) {
	zeros_left = (0x7f & inchar) - 1;
	return Cube->n_thresh = 0;
    }
    else			/*read in cubefax data */
	t_cnt = inchar;

    /* read in size info */
    my_fread(&inchar, 1, 1, fp);	/* read in size of cube data */
    size = inchar << 8;
    my_fread(&inchar, 1, 1, fp);
    size |= inchar;

    if (0 >= (ret = my_fread(Buffer, 1, size, fp))) {
	fprintf(stderr, "Error reading display file offset %"PRI_OFF_T"\n", G_ftell(fp));
	return (-1);
    }

    if (ret != size) {
	fprintf(stderr, "Error (size) reading display file offset %"PRI_OFF_T"\n",
		G_ftell(fp));
	return (-1);
    }


    {
	offset1 = 0;		/* pointer to n_polys */
	offset2 = t_cnt;	/* pointer to thresh_indexes */
	offset3 = t_cnt + t_cnt;	/* pointer to poly_info */

	for (i = 0; i < t_cnt; i++) {	/* n_thresholds loop */
	    Cube->data[i].npoly = Buffer[offset1++];
	    Cube->data[i].t_ndx = Buffer[offset2++];	/* THRESHOLD INDEX */

	    for (j = 0; j < Cube->data[i].npoly; j++) {
		Poly_info = &(Cube->data[i].poly[j]);
		Poly_info->v1[0] = Buffer[offset3++];
		Poly_info->v1[1] = Buffer[offset3++];
		Poly_info->v1[2] = Buffer[offset3++];
		Poly_info->v2[0] = Buffer[offset3++];
		Poly_info->v2[1] = Buffer[offset3++];
		Poly_info->v2[2] = Buffer[offset3++];
		Poly_info->v3[0] = Buffer[offset3++];
		Poly_info->v3[1] = Buffer[offset3++];
		Poly_info->v3[2] = Buffer[offset3++];
		Poly_info->n1[0] = Buffer[offset3++];
		Poly_info->n1[1] = Buffer[offset3++];
		Poly_info->n1[2] = Buffer[offset3++];
		/*
		   fprintf(stderr,"# %f ",Poly_info->v1[0]);
		   fprintf(stderr,"%f ",Poly_info->v1[1]);
		   fprintf(stderr,"%f \n",Poly_info->v1[2]);
		 */
		if (headfax->linefax.litmodel > 1) {	/* 3 normals */
		    Poly_info->n2[0] = Buffer[offset3++];
		    Poly_info->n2[1] = Buffer[offset3++];
		    Poly_info->n2[2] = Buffer[offset3++];
		    Poly_info->n3[0] = Buffer[offset3++];
		    Poly_info->n3[1] = Buffer[offset3++];
		    Poly_info->n3[2] = Buffer[offset3++];
		}
	    }
	}
    }
    return Cube->n_thresh = t_cnt;
}

#ifdef NEWCODE
int my_fread(char *buf, int size, int cnt, FILE * fp)
{
    static char in_buf[10240];
    static char *start, *end;
    char *outp;
    int ret;

    if (ret = fread(in_buf, 1, 10240, fp)) ;


    return 0;
}
#else

static int cptr = 0;

int my_fread(char *buf, int size, int cnt, FILE * fp)
{
    if (!fsize)
	return fread(buf, size, cnt, fp);
    else {
	int amt;

	amt = size * cnt;
	if (cptr + amt >= fsize)
	    amt = fsize - cptr - 1;
	struct_copy(buf, fptr + cptr, amt);
	cptr += amt;
	return (amt);
    }

    return 0;
}

int reset_reads(file_info * headfax)
{
    if (!fsize)
	G_fseek(headfax->dspfinfp, headfax->Dataoff, 0);
    else
	cptr = 0;

    return 0;
}

int new_dspf(file_info * hfax)
{
    G_fseek(hfax->dspfinfp, hfax->Dataoff, 0);
    cptr = fsize = 0;

    return 0;
}
#endif
