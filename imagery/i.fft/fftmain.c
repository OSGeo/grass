/****************************************************************************
 *
 * MODULE:       i.fft
 * AUTHOR(S):    David B. Satnik and Ali R. Vali (original contributors),
 *               Markus Neteler <neteler itc.it>
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      processes a single input raster map layer
 *               and constructs the real and imaginary Fourier
 *               components in frequency space
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/*
FFT for GRASS by:
 Central Washington University GIS Laboratory
 Programmer: David B. Satnik

Original FFT function provided by:
Programmer : Ali R. Vali
             Center for Space Research
             WRW 402
             University of Texas
             Austin, TX 78712-1085

             (512) 471-6824

*/

#define MAIN

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/gmath.h>
#include <grass/glocale.h>
#include "globals.h"
#include "local_proto.h"


int main (int argc, char *argv[])
{
        /* Global variable & function declarations */
    int Range;
    char Cellmap_orig[50];
        int inputfd, realfd, imagfd; /* the input and output file descriptors */
        char *inmapset; /* the input mapset name */
        struct Cell_head window;
        CELL *cell_row, *cell_row2;
        double max, min, scale, temp;

        int i,j;         /* Loop control variables */
        int or,oc;      /* Original dimensions of image */
        int rows,cols;  /* Smallest powers of 2 >= number of rows & columns */
        long totsize;         /* Total number of data points */
        double *data[2]; /* Data structure containing real & complex values of FFT */
        int save_args(); /* function to stash the command line arguments */
		struct GModule *module;
        struct Option *op1, *op2, *op3, *op4;
        const char *me;
        int maskfd;

        G_gisinit(argv[0]);
        me = G_program_name();

	    module = G_define_module();
	    module->keywords = _("imagery");
    module->description =
			_("Fast Fourier Transform (FFT) for image processing.");

        /* define options */
        op1=G_define_option();
        op1->key                = "input_image";
        op1->type               = TYPE_STRING;
        op1->required           =YES;
        op1->multiple           =NO;
        op1->gisprompt  = "old,cell,raster";
        op1->description        = _("Input raster map being fft");

        op2=G_define_option();
        op2->key                = "real_image";
        op2->type               = TYPE_STRING;
        op2->required           =YES;
        op2->multiple           =NO;
        op2->gisprompt  = "new,cell,raster";
        op2->description        = _("Output real part arrays stored as raster map");

        op3=G_define_option();
        op3->key                = "imaginary_image";
        op3->type               = TYPE_STRING;
        op3->required           =YES;
        op3->multiple           =NO;
        op3->gisprompt  = "new,cell,raster";
        op3->description        = _("Output imaginary part arrays stored as raster map");

        op4=G_define_option();
        op4->key                = "range";
        op4->type               = TYPE_INTEGER;
        op4->required           =NO;
        op4->multiple           =NO;
        op4->answer             ="255";
        op4->description        = _("Range of values in output display files");

        /*call parser*/
        if(G_parser(argc, argv))
                exit (EXIT_FAILURE);

        strcpy(Cellmap_orig, op1->answer) ;
        strcpy(Cellmap_real, op2->answer) ;
        strcpy(Cellmap_imag, op3->answer) ;

        /* open input cell map */
        if ((inmapset = G_find_cell(Cellmap_orig, "")) == NULL)
                G_fatal_error(_("Raster map <%s> not found"), Cellmap_orig);

        inputfd = G_open_cell_old(Cellmap_orig, inmapset);
        if(inputfd < 0)
                exit (EXIT_FAILURE);
        
        if ((maskfd = G_maskfd()) >= 0)
        	G_warning(_("Raster MASK found, consider to remove "
                          "(see man-page). Will continue..."));


        /* check command line args for validity */
        if (G_legal_filename(Cellmap_real) < 0)
                G_fatal_error (_("Illegal file name for real part: %s"), Cellmap_real);

        if (G_legal_filename(Cellmap_imag) < 0)
                G_fatal_error (_("Illegal file name for imaginary part: %s"), Cellmap_imag);

        sscanf(op4->answer, "%d", &Range);
        if (Range<=0)
                G_fatal_error(_("Range less than or equal to zero not allowed."));

        G_get_set_window(&window); /* get the current window for later */
        put_orig_window(&window);

        /* get the rows and columns in the current window */
        or = G_window_rows();
        oc = G_window_cols();
        rows = G_math_max_pow2 (or);
        cols = G_math_max_pow2 (oc);
        totsize = rows * cols;

        /*  fprintf(stderr,"Power 2 values : %d rows %d columns\n",rows,cols); */

        /* Allocate appropriate memory for the structure containing
     the real and complex components of the FFT.  DATA[0] will
     contain the real, and DATA[1] the complex component.
     */
        data[0] = (double *) G_malloc((rows*cols)*sizeof(double));
        data[1] = (double *) G_malloc((rows*cols)*sizeof(double));
        if (data[0] == NULL || data[1] == NULL)
                G_fatal_error(_("Insufficent memory for allocation of data structure"));

        /* Initialize real & complex components to zero */
        G_message(_("Initializing data...\n"));
        {
                register double *dptr1, *dptr0 ;
                dptr0=data[0] ;
                dptr1=data[1] ;
                for (i=0; i<totsize; i++) {
                        *dptr0++ = *dptr1++ = 0.0 ;
                }
        }

        /* allocate the space for one row of cell map data */
        cell_row = G_allocate_cell_buf();

        /* Read in cell map values */
        G_message(_("Reading the raster map..."));
        for (i=0; i<or; i++) {
                if (G_get_map_row(inputfd, cell_row, i)<0)
                        G_fatal_error(_("Error while reading input raster map."));
                for (j=0; j<oc; j++)
                        *(data[0]+(i*cols)+j) = (double) cell_row[j];
        }
        /* close input cell map and release the row buffer */
        G_close_cell(inputfd);
        G_free (cell_row);

        /* perform FFT */
        G_message(_("Starting FFT..."));
        fft(-1,data,totsize,cols,rows);
        G_message(_("FFT completed..."));

        /* set up a window for the transform cell map */
        window.rows = rows;
        window.cols = cols;
        window.south = window.north - window.rows * window.ns_res;
        window.east = window.cols * window.ew_res + window.west;
        G_set_window(&window);

        /* open the output cell maps and allocate cell row buffers */
        if ((realfd = G_open_cell_new(Cellmap_real)) < 0)
                exit(1);
        if ((imagfd = G_open_cell_new(Cellmap_imag)) < 0)
                exit(1);
        cell_row = G_allocate_cell_buf();
        cell_row2 = G_allocate_cell_buf();

        /* rotate the data array for standard display */
        G_message(_("Rotating data..."));
        for (i=0; i<rows; i++) {
                for (j=0; j<cols/2; j++) {
                        temp = *(data[0]+i*cols+j);
                        *(data[0]+i*cols+j) = *(data[0]+i*cols+j+cols/2);
                        *(data[0]+i*cols+j+cols/2) = temp;
                        temp = *(data[1]+i*cols+j);
                        *(data[1]+i*cols+j) = *(data[1]+i*cols+j+cols/2);
                        *(data[1]+i*cols+j+cols/2) = temp;
                }
        }
        for (i=0; i<rows/2; i++) {
                for (j=0; j<cols; j++) {
                        temp = *(data[0]+i*cols+j);
                        *(data[0]+i*cols+j) = *(data[0]+(i+rows/2)*cols+j);
                        *(data[0]+(i+rows/2)*cols+j) = temp;
                        temp = *(data[1]+i*cols+j);
                        *(data[1]+i*cols+j) = *(data[1]+(i+rows/2)*cols+j);
                        *(data[1]+(i+rows/2)*cols+j) = temp;
                }
        }

        G_message(_("Writing transformed data to file..."));
        /* write out the double arrays to cell_misc/file/FFTREAL and FFTIMAG */
        max = 0.0;
        min = 0.0;
        save_fft(totsize, data, &max, &min);

        G_message(_("Writing viewable versions of transformed data to files..."));
        /* Write out result to a new cell map */
        /*
        for (i=0; i<rows; i++) {
                for (j=0; j<cols; j++) {
                        *(cell_row+j) = (CELL) (log(1.0+fabs(*(data[0]+i*cols+j)
   )) * scale);
                        *(cell_row2+j) = (CELL) (log(1.0+fabs(*(data[1]+i*cols+j
   ))) * scale);
                }
        */
        scale = (double)Range / log(1.0 + max > -min ? max : -min);
        {
                register double *data0, *data1 ;
                register CELL *cptr1, *cptr2 ;
                for (i=0; i<rows; i++) {
                        data0 = data[0] + i * cols ;
                        data1 = data[1] + i * cols ;
                        cptr1 = cell_row ;
                        cptr2 = cell_row2 ;
                        for (j=0; j<cols; j++) {
                                *cptr1++ = (CELL) (log(1.0+fabs(*data0++)) * scale);
                                *cptr2++ = (CELL) (log(1.0+fabs(*data1++)) * scale);
                        }
                        G_put_raster_row(realfd, cell_row, CELL_TYPE);
                        G_put_raster_row(imagfd, cell_row2, CELL_TYPE);
                }
        }
        G_close_cell(realfd);
        G_close_cell(imagfd);
        G_free (cell_row);
        G_free (cell_row2);

        /* set up the color tables for histogram streched grey scale */
        fft_colors();

        /* Release memory resources */
        for (i=0 ; i<2 ; i++) G_free (data[i]);

        G_done_msg (_("Transform successful."));

    exit (EXIT_SUCCESS);
}
