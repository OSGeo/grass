
/****************************************************************************
 *
 * MODULE:       mkdspf
 * AUTHOR(S):    Originally by Bill Brown, with contributions from
 *               Bill Hughes, Brook Milligan, Helena Mitasova
 *               Started in GRASS 4.x as makedspf program, then split
 *               into module and dspf lib in GRASS 5.0beta_2i**. 
 *               Subsequent (post-CVS) contributors:
 *               Markus Neteler <neteler itc.it> 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Bob Covill <bcovill tekmap.ns.ca>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2004 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/* 9/2/94
   This program modified to be included within tcl/tk in the
   Nvision application by M. Astley.
   Basically, this file just holds a few access routines so our
   Nvision interface can run the program from within tcl/tk
 */

/* This program implements the marching cubes surface tiler described by
 * Lorenson & Cline in the Siggraph 87 Conference Proceedings.
 *
 * This program reads in data from a grid3 formatted file containing 3D data
 * and creates a display file.  
 * 
 * The display file consists of cell_info structures.
 * The cell_info structure:
 *      threshold (specified on commandline)
 *      number of polygons 
 *      polygon vertice coordinates
 *      surface or vertex normals (depending on lighting model specified)
 * 
 * The user must specify the data file name and the thresholds and lighting
 * model desired.  
 *
 * To specify the threshold:
 *      r min_threshold max_threshold interval (note: r stands for range)
 *      i num_thresholds threshold_1 threshold_2 etc. (i stands for individual)
 *
 * To specify the lighting model:
 *      flat 
 *      grad (for gradient)
 *
 */

#define MAIN
#include "viz.h"

/* Nvision includes */
#include "interface.h"

char *rindex();

static struct_copy();

/*
   Main routine has been modified to be a tcl/tk command.
   Arguments are just a modification of the command line
   arguments you would send to the program if it weren't a
   part of tcl/tk.

   Usage:
   mkdspf in_file out_file [c | r | i] {V} [f | g]
   if c then interval size
   if r then {list min_value max_value thresh_interval_size}
   if i then {list of values}

 */
int mkdspf_main(data, interp, argc, argv)
     Nv_data *data;		/* Local data */
     Tcl_Interp *interp;	/* Current interpreter */
     int argc;			/* Number of arguments */
     char **argv;		/* Argument strings */
{
    char ofile[200];		/*name of output file */
    char *p;
    int ret;			/* check return value of subroutines */
    int i;			/* counter */
    int process_status;

    /* Commandline must contain exactly 5 args */
    if (argc != 6) {
	pr_commandline(interp);
	return (TCL_ERROR);
    }

    /* open grid3 file for reading and writing */
    if ((Headfax.datainfp = fopen(argv[1], "r")) == NULL) {
	sprintf(interp->result, "ERROR:  unable to open %s for reading\n",
		argv[1]);
	return (TCL_ERROR);
    }

    /* reads info from data file headers and places in file_info struct */
    if (viz_init_file(argv, interp) != TCL_OK)
	return (TCL_ERROR);

    /* only reading in files that are in grid3 format */
    if (Headfax.token == 1)
	ret = g3read_header(&Headfax);

    if (ret < 1) {		/*problem in reading grid3 file */
	fclose(Headfax.datainfp);
	sprintf(interp->result,
		"Error in reading %s. Cannot create display file.\n",
		argv[1]);
	return (TCL_ERROR);
    }

    if (viz_calc_linefax(&Headfax.linefax, argv, argc, interp) != TCL_OK)
	return (TCL_ERROR);


    /*create output file name 
     **will place dspf file in same directory as raw data 
     **DO WE WANT TO CHANGE THIS ?
     */

    /* Here we perform the fork since we are done parsing arguments */
    /* We do this to allow background creation for large files (Mark 9/8/94) */
    process_status = fork();
    if (process_status)
	return (TCL_OK);

    /* Output file now taken from command line (Mark 9/7/94) */
    p = argv[2];
    strcpy(ofile, p);
    if (NULL != (p = rindex(ofile, '.'))) {
	if (p != ofile)
	    *p = '\0';		/* knock off any suffix */
    }
    strcat(ofile, ".dspf");

    /* open display file for writing */
    if ((Headfax.dspfoutfp = fopen(ofile, "w")) == NULL) {
	sprintf(interp->result, "ERROR:  unable to open %s for writing\n",
		ofile);
	if (process_status)
	    return (TCL_ERROR);
	else
	    exit(0);
    }
    /* write display file header info */
    /* have to adjust dimensions  -dpg */
    {
	Headfax.xdim -= 1;
	Headfax.ydim -= 1;
	Headfax.zdim -= 1;
	if (dfwrite_header(&Headfax) < 0) {
	    fclose(Headfax.dspfoutfp);
	    Tcl_AppendResult(interp, "Error writing output file\n", NULL);
	    if (process_status)
		return (TCL_ERROR);
	    else
		exit(0);
	}
	Headfax.xdim += 1;
	Headfax.ydim += 1;
	Headfax.zdim += 1;
    }

    if (viz_iso_surface(&Headfax.linefax, interp) != TCL_OK)
	if (process_status)
	    return (TCL_ERROR);
	else
	    exit(0);

    fclose(Headfax.datainfp);
    fclose(Headfax.dspfoutfp);

    if (process_status)
	return (TCL_OK);
    else
	exit(0);
}

/**************************** pr_commandline *********************************/

/**************************** pr_commandline *********************************/

/**************************** pr_commandline *********************************/

pr_commandline(interp)
     Tcl_Interp *interp;
{
    Tcl_AppendResult(interp,
		     "Usage: mkdspf in_file out_file [c | r | i] thresh_args [f | g]\n",
		     NULL);
    Tcl_AppendResult(interp, "\tif c then thresh_args = interval size\n",
		     NULL);
    Tcl_AppendResult(interp,
		     "\tif r then thresh_args = {list min_value max_value thresh_interval_size}\n",
		     NULL);
    Tcl_AppendResult(interp, "\tif i then thresh_args = {list of values}\n",
		     NULL);
}


static struct_copy(To, From, size)
     char *To, *From;
     int size;
{
    for (; size; size--)
	*To++ = *From++;
}
