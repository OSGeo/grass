/*
 ************************************************************
 * MODULE: r.le.patch/driver.c                              *
 *         Version 5.0                Nov. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyomirng               *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To analyze attributes of patches in a landscape *
 *         driver.c opens input and output files, and calls *
 *         the moving window, unit, and whole map drivers   *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/

#include <grass/config.h>
#include "patch.h"



/* DEFINE GLOBAL VARIABLES */

int ntype = 0, finput, n_scale = 1, n_unit = 1, size_cl_no = 0, *recl_count,
    shape_cl_no = 0;
static int para1, para2, para3, para4, para5;
float *shape_PA, *shape_CPA, *shape_RCC, *size_cl, **recl_tb;
extern struct CHOICE *choice;
char cmdbuf[100];
RASTER_MAP_TYPE data_type;

/*
   Variables:
   GLOBAL:
   ntype =       the number of attribute groups if by gp measures are
   calculated
   finput =      the raster map to be analyzed
   n_scale =     the number of sampling scales
   n_unit =      the number of sampling units
   para1 =       set to 1 if by gp measures are chosen
   para2 =       set to 1 if size classes are chosen
   para3 =       set to 1 if perimeter-area shape index classes are chosen
   para4 =       set to 1 if corrected perimeter-area shape index classes are
   chosen
   para5 =       set to 1 if related circumscribing circle shape index classes
   are chosen
   shape_PA[] =  array to hold the perimeter-area shape index classes
   shape_CPA[] = array to hold the corrected perimeter-area shape index
   classes
   shape_RCC[] = array to hold the related circumscribing circle shape index
   classes
   size_cl[] =   array to hold the size classes
   recl_tb[][]=  array to hold the attribute groups
   data_type =   the type of raster map: integer, floating point, or double
 */




			 /* RUN R.LE.PATCH IN FOREGROUND */

void patch_fore()
{

    fputs("\nR.LE.PATCH IS WORKING....;\n\n", stderr);

    /* check for input raster map and open it; this
       map remains open on finput while all the programs
       run, so it is globally available */

    if (0 > (finput = Rast_open_old(choice->fn, G_mapset()))) {
	fprintf(stderr, "\n");
	fprintf(stderr,
		"   ******* *************************************************\n");
	fprintf(stderr,
		"    The raster map you specified with the 'map=' parameter \n");
	fprintf(stderr,
		"    was not found in your mapset.                          \n");
	fprintf(stderr,
		"   ********************************************************\n");
	exit(EXIT_FAILURE);
    }

    /* determine whether the raster map is integer
       (CELL_TYPE),floating point (FCELL_TYPE), or
       double (DCELL_TYPE) and make globally available */

    else
	data_type = Rast_map_type(choice->fn, G_mapset());


    /* if using a moving window, get the parameters,
       and start the moving window driver */

    if (choice->wrum == 'm') {
	get_para();
	mv_driver();
	if (para1 || para2 || para3 || para4 || para5)
	    free_para();
    }

    /* get the parameters; if using the whole raster map
       as the sampling area, start the whole region driver;
       if using sampling units, start the sampling unit
       driver */

    else {
	get_para();
	open_files();
	if (choice->wrum != 'u')
	    whole_reg_driver();
	else
	    unit_driver();
	if (para1 || para2 || para3 || para4 || para5)
	    free_para();
    }

    /* when everything is done, close the raster map
       and print a completion message */

    Rast_close(finput);
    fputs("\nR.LE.PATCH IS DONE;  ", stderr);
    if (choice->wrum != 'm')
	fputs("OUTPUT FILES IN SUBDIRECTORY \"r.le.out\"\n", stderr);
    return;

}





			 /* SETUP THE OUTPUT FILES WHEN SAM=W,U,R */

void open_files()
{
    FILE *fp;
    char path[GPATH_MAX];
    int i;

    if (choice->att[1] || choice->att[2] || choice->att[3] || choice->att[4]) {
	fp = fopen0("r.le.out/a1-4.out", "w");
	fprintf(fp,
		"Scale  Unit  MN. PIXEL ATT.   S.D. PIXEL ATT.  MN. PATCH ATT.   S.D. PATCH ATT.\n");
	fclose(fp);
    }

    if (choice->att[5]) {
	fp = fopen0("r.le.out/a5.out", "w");
	fprintf(fp, "               COVER (FRACTION) BY GROUP\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < ntype; i++)
	    fprintf(fp, "   Group[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->att[6]) {
	fp = fopen0("r.le.out/a6.out", "w");
	fprintf(fp, "               DENSITY BY GROUP\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < ntype; i++)
	    fprintf(fp, "   Group[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->att[7]) {
	fp = fopen0("r.le.out/a7.out", "w");
	fprintf(fp, "Scale  Unit  TOTAL DENSITY\n");
	fclose(fp);
    }

    if (choice->att[8]) {
	fp = fopen0("r.le.out/a8.out", "w");
	fprintf(fp, "Scale  Unit  EFF. MESH NUM.\n");
	fclose(fp);
    }

    if (choice->size[1] || choice->size[2]) {
	fp = fopen0("r.le.out/s1-2.out", "w");
	fprintf(fp,
		"Scale  Unit  MN. PATCH SIZE   S.D. PATCH SIZE -- in pixels\n");
	fclose(fp);
    }

    if (choice->size[3]) {
	fp = fopen0("r.le.out/s3.out", "w");
	fprintf(fp, "               MEAN PATCH SIZE BY GROUP -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < ntype; i++)
	    fprintf(fp, "   Group[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->size[4]) {
	fp = fopen0("r.le.out/s4.out", "w");
	fprintf(fp, "               S.D. PATCH SIZE BY GROUP -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < ntype; i++)
	    fprintf(fp, "   Group[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->size[5]) {
	fp = fopen0("r.le.out/s5.out", "w");
	fprintf(fp,
		"               NUMBER OF PATCHES BY SIZE CLASS -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < size_cl_no; i++)
	    fprintf(fp, "   Class[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->size[6]) {
	fp = fopen0("r.le.out/s6.out", "w");
	fprintf(fp,
		"               NUMBER OF PATCHES BY SIZE CLASS BY GROUP -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < size_cl_no; i++)
	    fprintf(fp, "   Class[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->size[7] || choice->size[8]) {
	fp = fopen0("r.le.out/s7-8.out", "w");
	fprintf(fp, "Scale  Unit  EFF. MESH SIZE   DEG. LAND. DIV.\n");
	fclose(fp);
    }

    if (choice->core[1] || choice->core[2] ||
	choice->core[3] || choice->core[4]) {
	fp = fopen0("r.le.out/c1-4.out", "w");
	fprintf(fp,
		"Scale  Unit  MEAN CORE SIZE   S.D. CORE SIZE   MEAN EDGE SIZE   S.D. EDGE SIZE\n");
	fclose(fp);
    }

    if (choice->core[5]) {
	fp = fopen0("r.le.out/c5.out", "w");
	fprintf(fp, "               MEAN CORE SIZE BY GROUP -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < ntype; i++)
	    fprintf(fp, "   Group[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->core[6]) {
	fp = fopen0("r.le.out/c6.out", "w");
	fprintf(fp, "               S.D. CORE SIZE BY GROUP -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < ntype; i++)
	    fprintf(fp, "   Group[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->core[7]) {
	fp = fopen0("r.le.out/c7.out", "w");
	fprintf(fp, "               MEAN EDGE SIZE BY GROUP -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < ntype; i++)
	    fprintf(fp, "   Group[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->core[8]) {
	fp = fopen0("r.le.out/c8.out", "w");
	fprintf(fp, "               S.D. EDGE SIZE BY GROUP -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < ntype; i++)
	    fprintf(fp, "   Group[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->core[9]) {
	fp = fopen0("r.le.out/c9c.out", "w");
	fprintf(fp,
		"               NUMBER OF PATCH CORES BY SIZE CLASS -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < size_cl_no; i++)
	    fprintf(fp, "   Class[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
	fp = fopen0("r.le.out/c9e.out", "w");
	fprintf(fp,
		"               NUMBER OF PATCH EDGES BY SIZE CLASS -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < size_cl_no; i++)
	    fprintf(fp, "   Class[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->core[10]) {
	fp = fopen0("r.le.out/c10c.out", "w");
	fprintf(fp,
		"               NUMBER OF PATCH CORES BY SIZE CLASS BY GROUP -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < size_cl_no; i++)
	    fprintf(fp, "   Class[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
	fp = fopen0("r.le.out/c10e.out", "w");
	fprintf(fp,
		"               NUMBER OF PATCH EDGES BY SIZE CLASS BY GROUP -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < size_cl_no; i++)
	    fprintf(fp, "   Class[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->shape[1] || choice->shape[2]) {
	fp = fopen0("r.le.out/h1-2.out", "w");
	fprintf(fp, "Scale  Unit  MN. PATCH SHAPE  SD. PATCH SHAPE\n");
	fclose(fp);
    }

    if (choice->shape[3]) {
	fp = fopen0("r.le.out/h3.out", "w");
	fprintf(fp, "               MEAN PATCH SHAPE BY GROUP\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < ntype; i++)
	    fprintf(fp, "   Group[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->shape[4]) {
	fp = fopen0("r.le.out/h4.out", "w");
	fprintf(fp, "               S.D. PATCH SHAPE BY GROUP\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < ntype; i++)
	    fprintf(fp, "   Group[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->shape[5]) {
	fp = fopen0("r.le.out/h5.out", "w");
	fprintf(fp, "               NO. OF PATCHES BY SHAPE INDEX CLASS\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < shape_cl_no; i++)
	    fprintf(fp, "   Class[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->shape[6]) {
	fp = fopen0("r.le.out/h6.out", "w");
	fprintf(fp,
		"               NO. OF PATCHES BY SHAPE INDEX CLASS BY GROUP\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < shape_cl_no; i++)
	    fprintf(fp, "   Class[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->boundary[1] || choice->boundary[2] ||
	choice->boundary[3] || choice->boundary[4]) {
	fp = fopen0("r.le.out/n1-4.out", "w");
	fprintf(fp,
		"Scale  Unit  MEAN TWIST NUM.  SD. TWIST NUM.   MN. OMEGA INDEX  SD. OMEGA INDEX\n");
	fclose(fp);
    }

    if (choice->perim[1] || choice->perim[2] || choice->perim[3]) {
	fp = fopen0("r.le.out/p1-3.out", "w");
	fprintf(fp,
		"Scale  Unit  SUM PERIMETER    MEAN PERIMETER   S.D. PERIMETER -- in pixels\n");
	fclose(fp);
    }

    if (choice->perim[4]) {
	fp = fopen0("r.le.out/p4.out", "w");
	fprintf(fp,
		"               SUM OF PERIMETERS BY GROUP -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < ntype; i++)
	    fprintf(fp, "   Group[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->perim[5]) {
	fp = fopen0("r.le.out/p5.out", "w");
	fprintf(fp, "               MEAN PERIMETER BY GROUP -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < ntype; i++)
	    fprintf(fp, "   Group[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (choice->perim[6]) {
	fp = fopen0("r.le.out/p6.out", "w");
	fprintf(fp, "               S.D. PERIMETER BY GROUP -- in pixels\n");
	fprintf(fp, "Scale  Unit ");
	for (i = 0; i < ntype; i++)
	    fprintf(fp, "   Group[%2d]", i + 1);
	fprintf(fp, "\n");
	fclose(fp);
    }

    if (strcmp(choice->out, "") && choice->wrum != 'm') {
	sprintf(path, "r.le.out/%s", choice->out);
	fp = fopen0(path, "w");
	if (!strcmp(choice->out, "head"))
	    fprintf(fp, "sc-  un-                 center     patch\
     core     edge               shape index        twist omega\n");
	fprintf(fp, "ale  it    num   att    row  col     size\
     size     size      per    P/A   CP/A    RCC   number index\n");
	fclose(fp);
    }

    return;

}




			 /* OPEN R.LE OUTPUT FILE, WITH ERROR TRAP */

FILE *fopen0(char *name, char *flag)
{
    FILE *fp;

    if (!(fp = fopen(name, flag))) {
	fprintf(stderr, "\n");
	fprintf(stderr, "   ******************************************\n");
	fprintf(stderr, "    Can't open output file \"%s\"            \n",
		name);
	fprintf(stderr, "    Do you have write permission in r.le.out \n");
	fprintf(stderr, "    subdirectory?                            \n");
	fprintf(stderr, "   ******************************************\n");
    }
    return fp;
}



			 /* OPEN INPUT FILE, WITH ERROR TRAP */

FILE *fopen1(char *name, char *flag)
{
    FILE *fp;

    if (!(fp = fopen(name, flag))) {
	fprintf(stderr, "\n");
	fprintf(stderr,
		"   ******************************************************\n");
	fprintf(stderr,
		"    You chose a moving window or sampling units analysis \n");
	fprintf(stderr,
		"       but r.le.patch can't find file \"%s\"             \n",
		name);
	fprintf(stderr,
		"       which defines the moving window or sampling units \n");
	fprintf(stderr,
		"    First use r.le.setup to define a moving window or    \n");
	fprintf(stderr,
		"       sampling units to make this file                  \n");
	fprintf(stderr,
		"   ******************************************************\n");
	exit(EXIT_FAILURE);
    }
    return fp;
}


			 /* OPEN INPUT FILE, WITH ERROR TRAP */

FILE *fopen2(char *name, char *flag)
{
    FILE *fp;

    if (!(fp = fopen(name, flag))) {
	fprintf(stderr, "\n");
	fprintf(stderr,
		"   **************************************************\n");
	fprintf(stderr,
		"    You chose a 'by gp' or 'by class' analysis       \n");
	fprintf(stderr,
		"       but r.le.patch can't find file \"%s\"         \n",
		name);
	fprintf(stderr,
		"       which defines the attribute groups or classes \n");
	fprintf(stderr,
		"    First use r.le.setup to create this file         \n");
	fprintf(stderr,
		"   **************************************************\n");
	exit(EXIT_FAILURE);
    }
    return fp;
}



			 /* MOVING WINDOW DRIVER PROG. */

void mv_driver()
{
    register int i, j;
    int nr, nc, u_w, u_l, x0, y0, d, fmask, m, p;
    double *row_buf, *tmp_buf, *tmp_buf2, **buff = NULL;
    int a1, a2, a3, a4, a5, a6, a7, a8, s1, s2, s3, s4, s5, s6, s7, s8, c1,
	c2, c3, c4, c5, c6, c7, c8, c9, c10, h1, h2, h3, h4, h5, h6, p1, p2,
	p3, p4, p5, p6, n1, n2, n3, n4, b;
    long finished_time;
    float radius;
    struct Cell_head wind;
    char *nulltmp;

    /* variables:  
       nr = #rows in search area minus height of mov. wind.
       nc = #cols. in search area minus width of mov. wind.
       u_w = width of mov. wind. in cells
       u_l = width of mov. wind. in cells
       x0 = starting column for upper L corner of mov. wind.
       y0 = starting row for upper L corner of mov. wind.
       row = row for moving-window center
       col = column for moving-window center
       *row_buf = temporary array that holds one row of the
       raster map that is being analyzed
       *tmp_buf = temporary array that holds one moving wind.
       measure for a single row
       **buff = temporary array that holds the set of chosen 
       measures for a row
       radius = radius of the sampling unit, if circles are used
     */

    /* open the appropriate output moving window
       maps. All maps currently are double,
       rather than floating point or int */
    if (choice->att[1]) {
	if (G_find_raster("a1", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=a1,a1bak");
	    system(cmdbuf);
	}
	a1 = Rast_open_new("a1", DCELL_TYPE);
    }
    if (choice->att[2]) {
	if (G_find_raster("a2", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=a2,a2bak");
	    system(cmdbuf);
	}
	a2 = Rast_open_new("a2", DCELL_TYPE);
    }
    if (choice->att[3]) {
	if (G_find_raster("a3", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=a3,a3bak");
	    system(cmdbuf);
	}
	a3 = Rast_open_new("a3", DCELL_TYPE);
    }
    if (choice->att[4]) {
	if (G_find_raster("a4", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=a4,a4bak");
	    system(cmdbuf);
	}
	a4 = Rast_open_new("a4", DCELL_TYPE);
    }
    if (choice->att[5]) {
	if (G_find_raster("a5", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=a5,a5bak");
	    system(cmdbuf);
	}
	a5 = Rast_open_new("a5", DCELL_TYPE);
    }
    if (choice->att[6]) {
	if (G_find_raster("a6", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=a6,a6bak");
	    system(cmdbuf);
	}
	a6 = Rast_open_new("a6", DCELL_TYPE);
    }
    if (choice->att[7]) {
	if (G_find_raster("a7", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=a7,a7bak");
	    system(cmdbuf);
	}
	a7 = Rast_open_new("a7", DCELL_TYPE);
    }

    if (choice->att[8]) {
	if (G_find_raster("a8", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=a8,a8bak");
	    system(cmdbuf);
	}
	a8 = Rast_open_new("a8", DCELL_TYPE);
    }

    if (choice->size[1]) {
	if (G_find_raster("s1", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=s1,s1bak");
	    system(cmdbuf);
	}
	s1 = Rast_open_new("s1", DCELL_TYPE);
    }
    if (choice->size[2]) {
	if (G_find_raster("s2", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=s2,s2bak");
	    system(cmdbuf);
	}
	s2 = Rast_open_new("s2", DCELL_TYPE);
    }
    if (choice->size[3]) {
	if (G_find_raster("s3", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=s3,s3bak");
	    system(cmdbuf);
	}
	s3 = Rast_open_new("s3", DCELL_TYPE);
    }
    if (choice->size[4]) {
	if (G_find_raster("s4", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=s4,s4bak");
	    system(cmdbuf);
	}
	s4 = Rast_open_new("s4", DCELL_TYPE);
    }
    if (choice->size[5]) {
	if (G_find_raster("s5", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=s5,s5bak");
	    system(cmdbuf);
	}
	s5 = Rast_open_new("s5", DCELL_TYPE);
    }
    if (choice->size[6]) {
	if (G_find_raster("s6", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=s6,s6bak");
	    system(cmdbuf);
	}
	s6 = Rast_open_new("s6", DCELL_TYPE);
    }

    if (choice->size[7]) {
	if (G_find_raster("s7", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=s7,s7bak");
	    system(cmdbuf);
	}
	s7 = Rast_open_new("s7", DCELL_TYPE);
    }

    if (choice->size[8]) {
	if (G_find_raster("s8", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=s8,s8bak");
	    system(cmdbuf);
	}
	s8 = Rast_open_new("s8", DCELL_TYPE);
    }

    if (choice->core[1]) {
	if (G_find_raster("c1", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=c1,c1bak");
	    system(cmdbuf);
	}
	c1 = Rast_open_new("c1", DCELL_TYPE);
    }
    if (choice->core[2]) {
	if (G_find_raster("c2", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=c2,c2bak");
	    system(cmdbuf);
	}
	c2 = Rast_open_new("c2", DCELL_TYPE);
    }
    if (choice->core[3]) {
	if (G_find_raster("c3", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=c3,c3bak");
	    system(cmdbuf);
	}
	c3 = Rast_open_new("c3", DCELL_TYPE);
    }
    if (choice->core[4]) {
	if (G_find_raster("c4", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=c4,c4bak");
	    system(cmdbuf);
	}
	c4 = Rast_open_new("c4", DCELL_TYPE);
    }
    if (choice->core[5]) {
	if (G_find_raster("c5", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=c5,c5bak");
	    system(cmdbuf);
	}
	c5 = Rast_open_new("c5", DCELL_TYPE);
    }
    if (choice->core[6]) {
	if (G_find_raster("c6", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=c6,c6bak");
	    system(cmdbuf);
	}
	c6 = Rast_open_new("c6", DCELL_TYPE);
    }
    if (choice->core[7]) {
	if (G_find_raster("c7", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=c7,c7bak");
	    system(cmdbuf);
	}
	c7 = Rast_open_new("c7", DCELL_TYPE);
    }
    if (choice->core[8]) {
	if (G_find_raster("c8", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=c8,c8bak");
	    system(cmdbuf);
	}
	c8 = Rast_open_new("c8", DCELL_TYPE);
    }
    if (choice->core[9]) {
	if (G_find_raster("c9", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=c9,c9bak");
	    system(cmdbuf);
	}
	c9 = Rast_open_new("c9", DCELL_TYPE);
    }
    if (choice->core[10]) {
	if (G_find_raster("c10", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=c10,c10bak");
	    system(cmdbuf);
	}
	c10 = Rast_open_new("c10", DCELL_TYPE);
    }

    if (choice->shape[1]) {
	if (G_find_raster("h1", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=h1,h1bak");
	    system(cmdbuf);
	}
	h1 = Rast_open_new("h1", DCELL_TYPE);
    }
    if (choice->shape[2]) {
	if (G_find_raster("h2", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=h2,h2bak");
	    system(cmdbuf);
	}
	h2 = Rast_open_new("h2", DCELL_TYPE);
    }
    if (choice->shape[3]) {
	if (G_find_raster("h3", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=h3,h3bak");
	    system(cmdbuf);
	}
	h3 = Rast_open_new("h3", DCELL_TYPE);
    }
    if (choice->shape[4]) {
	if (G_find_raster("h4", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=h4,h4bak");
	    system(cmdbuf);
	}
	h4 = Rast_open_new("h4", DCELL_TYPE);
    }
    if (choice->shape[5]) {
	if (G_find_raster("h5", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=h5,h5bak");
	    system(cmdbuf);
	}
	h5 = Rast_open_new("h5", DCELL_TYPE);
    }
    if (choice->shape[6]) {
	if (G_find_raster("h6", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=h6,h6bak");
	    system(cmdbuf);
	}
	h6 = Rast_open_new("h6", DCELL_TYPE);
    }

    if (choice->boundary[1]) {
	if (G_find_raster("n1", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=n1,n1bak");
	    system(cmdbuf);
	}
	n1 = Rast_open_new("n1", DCELL_TYPE);
    }

    if (choice->boundary[2]) {
	if (G_find_raster("n2", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=n2,n2bak");
	    system(cmdbuf);
	}
	n2 = Rast_open_new("n2", DCELL_TYPE);
    }

    if (choice->boundary[3]) {
	if (G_find_raster("n3", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=n3,n3bak");
	    system(cmdbuf);
	}
	n3 = Rast_open_new("n3", DCELL_TYPE);
    }

    if (choice->boundary[4]) {
	if (G_find_raster("n4", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=n4,n4bak");
	    system(cmdbuf);
	}
	n4 = Rast_open_new("n4", DCELL_TYPE);
    }

    if (choice->perim[1]) {
	if (G_find_raster("p1", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=p1,p1bak");
	    system(cmdbuf);
	}
	p1 = Rast_open_new("p1", DCELL_TYPE);
    }
    if (choice->perim[2]) {
	if (G_find_raster("p2", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=p2,p2bak");
	    system(cmdbuf);
	}
	p2 = Rast_open_new("p2", DCELL_TYPE);
    }
    if (choice->perim[3]) {
	if (G_find_raster("p3", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=p3,p3bak");
	    system(cmdbuf);
	}
	p3 = Rast_open_new("p3", DCELL_TYPE);
    }
    if (choice->perim[4]) {
	if (G_find_raster("p4", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=p4,p4bak");
	    system(cmdbuf);
	}
	p4 = Rast_open_new("p4", DCELL_TYPE);
    }
    if (choice->perim[5]) {
	if (G_find_raster("p5", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=p5,p5bak");
	    system(cmdbuf);
	}
	p5 = Rast_open_new("p5", DCELL_TYPE);
    }
    if (choice->perim[6]) {
	if (G_find_raster("p6", G_mapset()) != NULL) {
	    sprintf(cmdbuf, "%s %s", "g.rename -o", "rast=p6,p6bak");
	    system(cmdbuf);
	}
	p6 = Rast_open_new("p6", DCELL_TYPE);
    }

    /* get the moving window parameters */

    read_mwind(&u_w, &u_l, &nc, &nr, &x0, &y0, &radius);

    /* check for an unacceptable
       moving-window size */

    if (nc < 1 || nr < 1) {
	fprintf(stderr, "\n");
	fprintf(stderr,
		"   *******************************************************\n");
	fprintf(stderr,
		"    The moving window size specified in file r.le.para/   \n");
	fprintf(stderr,
		"    move_wind is less than 1 row or column.  Check this   \n");
	fprintf(stderr,
		"    file or redefine the moving window using r.le.setup.  \n");
	fprintf(stderr,
		"   *******************************************************\n");
	exit(EXIT_FAILURE);
    }

    /* check for an unacceptable
       search area and clip it */

    G_get_set_window(&wind);
    if (wind.rows < nr + y0 || wind.cols < nc + x0) {
	fprintf(stderr, "\n");
	fprintf(stderr,
		"   *******************************************************\n");
	fprintf(stderr,
		"    Moving window search area in file r.le.para/move_wind \n");
	fprintf(stderr,
		"    does not match the dimensions of the current region.  \n");
	fprintf(stderr,
		"    You must either rerun r.le.setup to make a new        \n");
	fprintf(stderr,
		"    r.le.para/move_wind file or reset the region to match \n");
	fprintf(stderr,
		"    the r.le.para/move_wind file                          \n");
	fprintf(stderr,
		"   *******************************************************\n");
	exit(EXIT_FAILURE);
    }

    /* set the d parameter for the 
       performance meter */

    if (nr * nc > 10000)
	d = nr * nc / 1000;
    else if (nr * nc > 2500)
	d = nr * nc / 100;
    else
	d = 10;

    /* return a value > 0 to fmask if
       there is a MASK present */

    fprintf(stderr,
	    "If a MASK is not present (see r.mask) a beep may sound and a\n");
    fprintf(stderr,
	    "   warning may be printed or appear in a window; ignore this warning.\n");
    fprintf(stderr, "If a MASK is present there will be no warning.\n");
    fmask = Rast_open_old("MASK", G_mapset());
    fprintf(stderr, "\n");

    /* allocate memory for the buffer that
       will hold the set of 42 possible
       measures for a row */

    buff = (double **)G_calloc(nc + 1, sizeof(double *));

    /* allocate memory for each of 42 measures */

    for (p = 0; p < nc + 1; p++)
	buff[p] = (double *)G_calloc(42, sizeof(double));

    /* allocate memory for a buffer to hold
       a row of the MASK, if there is a MASK */

    if (fmask > 0)
	row_buf = Rast_allocate_buf(CELL_TYPE);

    /* main loop for clipping & measuring
       using the moving-window */

    for (i = 0; i < nr; i++) {

	/* zero the measure buffer before
	   filling it again */

	for (m = 0; m < nc + 1; m++) {
	    for (p = 0; p < 42; p++)
		*(*(buff + m) + p) = 0.0;
	}

	/* if there is a MASK, then read in 
	   a row of MASK - this part skips 
	   cells with the value "0" in the
	   MASK to speed up the moving window
	   process */

	if (fmask > 0) {
	    Rast_zero_buf(row_buf, CELL_TYPE);
	    Rast_get_row_nomask(fmask, row_buf, y0 + i + u_l / 2,
				    CELL_TYPE);

	    /* for each cell whose value is "1"
	       in MASK */

	    for (j = 0; j < nc; j++) {

		/* display #cells left to do */

		if (i == 0 && j == 0)
		    fprintf(stdout, "TOTAL WINDOWS = %8d\n", nr * nc);
		meter(nr * nc, (i * nc + (j + 1)), d);

		/* call the cell clip driver */

		if (row_buf[x0 + j + u_w / 2])
		    cell_clip_drv(x0 + j, y0 + i, u_w, u_l, buff, j, radius);
	    }
	}

	/* if there is no MASK, then clip
	   and measure at every cell */

	else {

	    for (j = 0; j < nc; j++) {

		/* display #cells left to do */

		if (i == 0 && j == 0)
		    fprintf(stdout, "TOTAL WINDOWS = %8d\n", nr * nc);
		meter(nr * nc, (i * nc + (j + 1)), d);

		/* call the cell clip driver.  This routine will clip
		   the rectangle at x0 + j, y0 + i and u_w X u_l wide
		   (or in a circle with radius), and put the results
		   for each chosen moving windown measure in buff;
		   note that the center of the moving window is not
		   at x0 + j, y0 + i, but at x0 + j + u_w/2, y0 + i +
		   u_l/2 */

		cell_clip_drv(x0 + j, y0 + i, u_w, u_l, buff, j, radius);
	    }
	}
	/* copy the chosen measures into a temporary row
	   buffer which is then fed into the chosen output
	   maps; the map location is adjusted to the center
	   of the moving window */

	tmp_buf = Rast_allocate_buf(DCELL_TYPE);
	nulltmp = Rast_allocate_null_buf();

	if (choice->att[1]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a1, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 0) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 0);
		}
		Rast_put_d_row(a1, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a1, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}

	if (choice->att[2]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a2, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 1) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 1);
		}
		Rast_put_d_row(a2, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a2, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->att[3]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a3, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 2) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 2);
		}
		Rast_put_d_row(a3, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a3, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->att[4]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a4, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 3) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 3);
		}
		Rast_put_d_row(a4, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a4, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->att[5]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a5, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 4) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 4);
		}
		Rast_put_d_row(a5, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a5, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->att[6]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a6, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 5) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 5);
		}
		Rast_put_d_row(a6, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a6, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->att[7]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a7, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 6) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 6);
		}
		Rast_put_d_row(a7, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a7, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->size[1]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s1, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 7) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 7);
		}
		Rast_put_d_row(s1, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s1, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->size[2]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s2, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 8) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 8);
		}
		Rast_put_d_row(s2, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s2, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->size[3]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s3, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 9) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 9);
		}
		Rast_put_d_row(s3, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s3, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->size[4]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s4, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 10) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 10);
		}
		Rast_put_d_row(s4, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s4, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->size[5]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s5, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 11) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 11);
		}
		Rast_put_d_row(s5, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s5, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->size[6]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s6, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 12) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 12);
		}
		Rast_put_d_row(s6, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s6, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->core[1]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c1, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 13) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 13);
		}
		Rast_put_d_row(c1, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c1, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->core[2]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c2, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 14) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 14);
		}
		Rast_put_d_row(c2, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c2, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->core[3]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c3, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 15) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 15);
		}
		Rast_put_d_row(c3, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c3, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->core[4]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c4, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 16) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 16);
		}
		Rast_put_d_row(c4, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c4, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->core[5]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c5, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 17) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 17);
		}
		Rast_put_d_row(c5, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c5, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->core[6]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c6, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 18) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 18);
		}
		Rast_put_d_row(c6, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c6, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->core[7]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c7, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 19) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 19);
		}
		Rast_put_d_row(c7, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c7, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->core[8]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c8, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 20) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 20);
		}
		Rast_put_d_row(c8, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c8, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->core[9]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c9, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 21) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 21);
		}
		Rast_put_d_row(c9, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c9, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->core[10]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c10, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 22) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 22);
		}
		Rast_put_d_row(c10, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(c10, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->shape[1]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(h1, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 23) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 23);
		}
		Rast_put_d_row(h1, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(h1, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->shape[2]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(h2, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 24) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 24);
		}
		Rast_put_d_row(h2, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(h2, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->shape[3]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(h3, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 25) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 25);
		}
		Rast_put_d_row(h3, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(h3, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->shape[4]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(h4, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 26) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 26);
		}
		Rast_put_d_row(h4, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(h4, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->shape[5]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(h5, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 27) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 27);
		}
		Rast_put_d_row(h5, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(h5, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->shape[6]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(h6, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 28) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 28);
		}
		Rast_put_d_row(h6, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(h6, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->boundary[1]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(n1, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 29) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 29);
		}
		Rast_put_d_row(n1, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(n1, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->perim[1]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(p1, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 30) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 30);
		}
		Rast_put_d_row(p1, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(p1, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->perim[2]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(p2, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 31) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 31);
		}
		Rast_put_d_row(p2, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(p2, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->perim[3]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(p3, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 32) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 32);
		}
		Rast_put_d_row(p3, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(p3, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->perim[4]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(p4, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 33) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 33);
		}
		Rast_put_d_row(p4, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(p4, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->perim[5]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(p5, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 34) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 34);
		}
		Rast_put_d_row(p5, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(p5, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	if (choice->perim[6]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(p6, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 35) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 35);
		}
		Rast_put_d_row(p6, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(p6, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}

	if (choice->att[8]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a8, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 36) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 36);
		}
		Rast_put_d_row(a8, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(a8, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}

	if (choice->size[7]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s7, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 37) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 37);
		}
		Rast_put_d_row(s7, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s7, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}

	if (choice->size[8]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s8, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 38) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 38);
		}
		Rast_put_d_row(s8, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(s8, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}

	if (choice->boundary[2]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(n2, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 39) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 39);
		}
		Rast_put_d_row(n2, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(n2, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}

	if (choice->boundary[3]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(n3, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 40) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 40);
		}
		Rast_put_d_row(n3, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(n3, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}

	if (choice->boundary[4]) {
	    Rast_zero_buf(tmp_buf, DCELL_TYPE);
	    Rast_set_null_value(tmp_buf, x0 + nc + u_w, DCELL_TYPE);
	    if (i == 0) {
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(n4, tmp_buf);
	    }
	    if (i < nr) {
		for (m = 0; m < nc; m++) {
		    if (*(*(buff + m) + 41) > -BIG)
			*(tmp_buf + (x0 + m + u_w / 2)) = *(*(buff + m) + 41);
		}
		Rast_put_d_row(n4, tmp_buf);
	    }
	    if (i == nr - 1) {
		tmp_buf2 = Rast_allocate_buf(DCELL_TYPE);
		Rast_set_null_value(tmp_buf2, x0 + nc + u_w, DCELL_TYPE);
		for (b = 0; b < u_l / 2; b++)
		    Rast_put_d_row(n4, tmp_buf2);
		G_free(tmp_buf2);
	    }
	}
	G_free(tmp_buf);
    }

    time(&finished_time);
    fprintf(stdout, "\nACTUAL COMPLETION = %s",
	    asctime(localtime(&finished_time)));
    fflush(stdout);


    /* free the memory allocated for the
       mask and other buffer */

    if (fmask > 0)
	G_free(row_buf);
    for (p = 0; p < nc + 1; p++)
	G_free(buff[p]);
    G_free(buff);

    /* close the raster maps, set the
       color table for the new raster
       map and compress the map */


    if (choice->att[1]) {
	Rast_close(a1);
	set_colors("a1");
	sprintf(cmdbuf, "%s %s", "r.compress", "a1");
	system(cmdbuf);
    }
    if (choice->att[2]) {
	Rast_close(a2);
	set_colors("a2");
	sprintf(cmdbuf, "%s %s", "r.compress", "a2");
	system(cmdbuf);
    }
    if (choice->att[3]) {
	Rast_close(a3);
	set_colors("a3");
	sprintf(cmdbuf, "%s %s", "r.compress", "a3");
	system(cmdbuf);
    }
    if (choice->att[4]) {
	Rast_close(a4);
	set_colors("a4");
	sprintf(cmdbuf, "%s %s", "r.compress", "a4");
	system(cmdbuf);
    }
    if (choice->att[5]) {
	Rast_close(a5);
	set_colors("a5");
	sprintf(cmdbuf, "%s %s", "r.compress", "a5");
	system(cmdbuf);
    }
    if (choice->att[6]) {
	Rast_close(a6);
	set_colors("a6");
	sprintf(cmdbuf, "%s %s", "r.compress", "a6");
	system(cmdbuf);
    }
    if (choice->att[7]) {
	Rast_close(a7);
	set_colors("a7");
	sprintf(cmdbuf, "%s %s", "r.compress", "a7");
	system(cmdbuf);
    }
    if (choice->att[8]) {
	Rast_close(a8);
	set_colors("a8");
	sprintf(cmdbuf, "%s %s", "r.compress", "a8");
	system(cmdbuf);
    }
    if (choice->size[1]) {
	Rast_close(s1);
	set_colors("s1");
	sprintf(cmdbuf, "%s %s", "r.compress", "s1");
	system(cmdbuf);
    }
    if (choice->size[2]) {
	Rast_close(s2);
	set_colors("s2");
	sprintf(cmdbuf, "%s %s", "r.compress", "s2");
	system(cmdbuf);
    }
    if (choice->size[3]) {
	Rast_close(s3);
	set_colors("s3");
	sprintf(cmdbuf, "%s %s", "r.compress", "s3");
	system(cmdbuf);
    }
    if (choice->size[4]) {
	Rast_close(s4);
	set_colors("s4");
	sprintf(cmdbuf, "%s %s", "r.compress", "s4");
	system(cmdbuf);
    }
    if (choice->size[5]) {
	Rast_close(s5);
	set_colors("s5");
	sprintf(cmdbuf, "%s %s", "r.compress", "s5");
	system(cmdbuf);
    }
    if (choice->size[6]) {
	Rast_close(s6);
	set_colors("s6");
	sprintf(cmdbuf, "%s %s", "r.compress", "s6");
	system(cmdbuf);
    }
    if (choice->size[7]) {
	Rast_close(s7);
	set_colors("s7");
	sprintf(cmdbuf, "%s %s", "r.compress", "s7");
	system(cmdbuf);
    }
    if (choice->size[8]) {
	Rast_close(s8);
	set_colors("s8");
	sprintf(cmdbuf, "%s %s", "r.compress", "s8");
	system(cmdbuf);
    }
    if (choice->core[1]) {
	Rast_close(c1);
	set_colors("c1");
	sprintf(cmdbuf, "%s %s", "r.compress", "c1");
	system(cmdbuf);
    }
    if (choice->core[2]) {
	Rast_close(c2);
	set_colors("c2");
	sprintf(cmdbuf, "%s %s", "r.compress", "c2");
	system(cmdbuf);
    }
    if (choice->core[3]) {
	Rast_close(c3);
	set_colors("c3");
	sprintf(cmdbuf, "%s %s", "r.compress", "c3");
	system(cmdbuf);
    }
    if (choice->core[4]) {
	Rast_close(c4);
	set_colors("c4");
	sprintf(cmdbuf, "%s %s", "r.compress", "c4");
	system(cmdbuf);
    }
    if (choice->core[5]) {
	Rast_close(c5);
	set_colors("c5");
	sprintf(cmdbuf, "%s %s", "r.compress", "c5");
	system(cmdbuf);
    }
    if (choice->core[6]) {
	Rast_close(c6);
	set_colors("c6");
	sprintf(cmdbuf, "%s %s", "r.compress", "c6");
	system(cmdbuf);
    }
    if (choice->core[7]) {
	Rast_close(c7);
	set_colors("c7");
	sprintf(cmdbuf, "%s %s", "r.compress", "c7");
	system(cmdbuf);
    }
    if (choice->core[8]) {
	Rast_close(c8);
	set_colors("c8");
	sprintf(cmdbuf, "%s %s", "r.compress", "c8");
	system(cmdbuf);
    }
    if (choice->core[9]) {
	Rast_close(c9);
	set_colors("c9");
	sprintf(cmdbuf, "%s %s", "r.compress", "c9");
	system(cmdbuf);
    }
    if (choice->core[10]) {
	Rast_close(c10);
	set_colors("c10");
	sprintf(cmdbuf, "%s %s", "r.compress", "c10");
	system(cmdbuf);
    }
    if (choice->shape[1]) {
	Rast_close(h1);
	set_colors("h1");
	sprintf(cmdbuf, "%s %s", "r.compress", "h1");
	system(cmdbuf);
    }
    if (choice->shape[2]) {
	Rast_close(h2);
	set_colors("h2");
	sprintf(cmdbuf, "%s %s", "r.compress", "h2");
	system(cmdbuf);
    }
    if (choice->shape[3]) {
	Rast_close(h3);
	set_colors("h3");
	sprintf(cmdbuf, "%s %s", "r.compress", "h3");
	system(cmdbuf);
    }
    if (choice->shape[4]) {
	Rast_close(h4);
	set_colors("h4");
	sprintf(cmdbuf, "%s %s", "r.compress", "h4");
	system(cmdbuf);
    }
    if (choice->shape[5]) {
	Rast_close(h5);
	set_colors("h5");
	sprintf(cmdbuf, "%s %s", "r.compress", "h5");
	system(cmdbuf);
    }
    if (choice->shape[6]) {
	Rast_close(h6);
	set_colors("h6");
	sprintf(cmdbuf, "%s %s", "r.compress", "h6");
	system(cmdbuf);
    }
    if (choice->boundary[1]) {
	Rast_close(n1);
	set_colors("n1");
	sprintf(cmdbuf, "%s %s", "r.compress", "n1");
	system(cmdbuf);
    }
    if (choice->boundary[2]) {
	Rast_close(n2);
	set_colors("n2");
	sprintf(cmdbuf, "%s %s", "r.compress", "n2");
	system(cmdbuf);
    }
    if (choice->boundary[3]) {
	Rast_close(n3);
	set_colors("n3");
	sprintf(cmdbuf, "%s %s", "r.compress", "n3");
	system(cmdbuf);
    }
    if (choice->boundary[4]) {
	Rast_close(n4);
	set_colors("n4");
	sprintf(cmdbuf, "%s %s", "r.compress", "n4");
	system(cmdbuf);
    }
    if (choice->perim[1]) {
	Rast_close(p1);
	set_colors("p1");
	sprintf(cmdbuf, "%s %s", "r.compress", "p1");
	system(cmdbuf);
    }
    if (choice->perim[2]) {
	Rast_close(p2);
	set_colors("p2");
	sprintf(cmdbuf, "%s %s", "r.compress", "p2");
	system(cmdbuf);
    }
    if (choice->perim[3]) {
	Rast_close(p3);
	set_colors("p3");
	sprintf(cmdbuf, "%s %s", "r.compress", "p3");
	system(cmdbuf);
    }
    if (choice->perim[4]) {
	Rast_close(p4);
	set_colors("p4");
	sprintf(cmdbuf, "%s %s", "r.compress", "p4");
	system(cmdbuf);
    }
    if (choice->perim[5]) {
	Rast_close(p5);
	set_colors("p5");
	sprintf(cmdbuf, "%s %s", "r.compress", "p5");
	system(cmdbuf);
    }
    if (choice->perim[6]) {
	Rast_close(p6);
	set_colors("p6");
	sprintf(cmdbuf, "%s %s", "r.compress", "p6");
	system(cmdbuf);
    }

    Rast_close(fmask);

    return;
}






			 /* SET COLOR TABLE FOR MOVING WINDOW 
			    OUTPUT MAPS TO G-Y-R */

void set_colors(char *name)
{
    struct Colors colors;
    struct FPRange fprange;

    Rast_read_fp_range(name, G_mapset(), &fprange);
    Rast_make_gyr_fp_colors(&colors, fprange.min, fprange.max);
    Rast_write_colors(name, G_mapset(), &colors);
    return;

}





			 /* READ IN THE MOVING WINDOW PARAMETERS */

void read_mwind(int *uw, int *ul, int *nc, int *nr, int *x0, int *y0,
		float *radius)
{
    FILE *fp;
    int ww, wl;
    char *buf;

    fp = fopen1("r.le.para/move_wind", "r");
    buf = G_malloc(513);

    fgets(buf, 512, fp);
    sscanf(buf, "%d%d", uw, ul);

    fgets(buf, 512, fp);
    sscanf(buf, "%f", radius);

    fgets(buf, 512, fp);
    sscanf(buf, "%d%d", &ww, &wl);

    fgets(buf, 512, fp);
    sscanf(buf, "%d%d", x0, y0);

    *nc = ww - *uw + 1;
    *nr = wl - *ul + 1;

    G_free(buf);
    fclose(fp);
    return;
}


			 /* PERFORMANCE METER - DISPLAYS
			    THE PROGRESS OF THE MOVING WINDOW
			    AS A COUNT AND ESTIMATED COMPLETION
			    TIME WHILE THE PROGRAM RUNS */

void meter(int n, int i, int div)
{
    long current_time, time_left, elapsed, complete;
    static long start;
    float window_time;
    static int k = 0;
    char done[30];
    int d;

    if (i <= 1) {
	time(&start);
    }

    if (i < 10)
	d = 1;
    else
	d = div;

    if (k > 2000) {
	if (G_fseek(stdout, 0L, 0))
	    G_fatal_error("Can't reset the \"stdout\", exit.\n");
	k = 0;
    }

    if ((n - i) % d == 0) {
	time(&current_time);
	elapsed = current_time - start;
	window_time = ((float)elapsed) / (i + 1);
	time_left = (long)((n - i) * window_time);
	complete = current_time + time_left;
	strncpy(done, asctime(localtime(&complete)), 24);
	done[24] = '\0';
	fprintf(stdout, "WINDOWS LEFT  = %8d   EST. COMPLETION = %s\r",
		(n - i), done);
	fflush(stdout);
	k++;
    }
    return;
}

			 /* READ IN THE PARAMETERS FOR
			    GROUPS & CLASSES */


void get_para()
{
    register int i, j, k;
    float *tmp;
    int counter;

    /*
       Variables:
       GLOBAL:
       para1 = set to 1 if by gp measures are chosen
       para2 = set to 1 if size classes are chosen
       para3 = set to 1 if perimeter-area shape index classes are chosen
       para4 = set to 1 if corrected perimeter-area shape index classes are chosen
       para5 = set to 1 if related circumscribing circle shape index classes are
       chosen
     */
    /* set the parameter flags to 0 */

    para1 = para2 = para3 = para4 = para5 = 0;

    /* read the reclass table for attribute gps */

    if (choice->att[5] || choice->att[6] || choice->size[3] || choice->size[4]
	|| choice->size[6] || choice->core[5] || choice->core[6]
	|| choice->core[7] || choice->core[8] || choice->core[10]
	|| choice->shape[3] || choice->shape[4] || choice->shape[6]
	|| choice->perim[4] || choice->perim[5] || choice->perim[6]) {

	para1 = 1;
	recl_tb = (float **)G_calloc(25, sizeof(float *));
	tmp = (float *)G_calloc(50, sizeof(float));
	if (choice->wrum == 'm')
	    k = 1;
	else
	    k = 25;
	recl_count = (int *)G_calloc(k, sizeof(int));
	for (i = 0; i < k; i++) {
	    read_para("recl_tb", i + 1, tmp, &counter);
	    if (counter < 2)
		break;
	    recl_tb[i] = (float *)G_malloc(50 * sizeof(float));
	    for (j = 0; j < counter; j++) {
		recl_tb[i][j] = tmp[j];
	    }
	    recl_count[i] = counter;
	}
	if (choice->wrum == 'm')
	    ntype = 1;
	else
	    ntype = i;
	G_free(tmp);
	if (!ntype) {
	    fprintf(stderr, "\n");
	    fprintf(stderr,
		    "   ********************************************************\n");
	    fprintf(stderr,
		    "    The attribute group file (r.le.para/recl_tb) seems to  \n");
	    fprintf(stderr,
		    "    be incorrect as no attribute groups were found.  Check \n");
	    fprintf(stderr,
		    "    this file or make it again using r.le.setup.           \n");
	    fprintf(stderr,
		    "   ********************************************************\n");
	    exit(EXIT_FAILURE);
	}
    }

    /* read the size classes */

    if (choice->size[5] || choice->size[6] || choice->core[9] ||
	choice->core[10]) {
	para2 = 1;
	size_cl = (float *)G_calloc(20, sizeof(float));
	read_line("size", 1, -999, NULL, size_cl, &size_cl_no);
    }

    /* read shape index classes */

    if (choice->Mx[1] && (choice->shape[5] || choice->shape[6])) {
	para3 = 1;
	shape_PA = (float *)G_calloc(20, sizeof(float));
	read_line("shape_PA", 1, -999, NULL, shape_PA, &shape_cl_no);
    }
    else if (choice->Mx[2] && (choice->shape[5] || choice->shape[6])) {
	para4 = 1;
	shape_CPA = (float *)G_calloc(20, sizeof(float));
	read_line("shape_CPA", 1, -999, NULL, shape_CPA, &shape_cl_no);
    }
    else if (choice->Mx[3] && (choice->shape[5] || choice->shape[6])) {
	para5 = 1;
	shape_RCC = (float *)G_calloc(20, sizeof(float));
	read_line("shape_RCC", 1, -999, NULL, shape_RCC, &shape_cl_no);
    }
    return;
}







			 /* RELEASE GLOBAL MEMORY */

void free_para()
{
    register int i;

    if (para1) {
	for (i = 0; i < ntype; i++)
	    G_free(recl_tb[i]);
	G_free(recl_tb);
	G_free(recl_count);
    }
    if (para2)
	G_free(size_cl);
    if (para3)
	G_free(shape_PA);
    if (para4)
	G_free(shape_CPA);
    if (para5)
	G_free(shape_RCC);
    return;

}





			 /* COUNT HOW MANY ATTRIBUTES ARE IN ONE
			    LINE OF THE RECL_TB FILE, WHICH DEFINES
			    ATTRIBUTE GROUPS */

void read_para(char *name, int line, float *value, int *count)
{
    FILE *fp;
    int i = 0, cnt = 1;
    char *buf, path[GPATH_MAX];

    /* VARIABLES
       Incoming
       name       = always "recl_tb"
       line       = the line of the recl_tb file to be read
       value      = an array of 50 values, each of which is a
       count of the number of attributes on the line;
       this is the array returned to the calling program
       count      = ??
       Internal
       fp         = pointer to recl_tb file
       i          = counter for moving through buf array
       cnt        = count of number of attributes on this line
       buf        = buffer to hold one line of recl_tb
       path       = the relative path to the recl_tb file
     */


    /* open the recl_tb, allocate enough memory,
       then read one line of the recl_tb into
       the buffer; then close the recl_tb */

    sprintf(path, "r.le.para/%s", name);
    fp = fopen2(path, "r");
    buf = G_malloc(256);
    while (fgets(buf, 256, fp) && i < line - 1)
	i++;
    fclose(fp);

    /* go through the line of the recl_tb character
       by character */

    for (i = 0;; i++) {

	/* if "e" of "end" is found or an "=" sign is found, then
	   quit and set value to 1 */

	if (*(buf + i) == 'e' || *(buf + i) == '=')
	    break;

	/* if "t" of "thru" is found, set the corresponding element
	   of the value array to -999, add one to cnt, then
	   jump to the next character after "thru" */

	else if (*(buf + i) == 't') {
	    *(value + cnt) = -999;
	    cnt++;
	    i += 4;
	}

	/* if a number is found, then scan it into the
	   corresponding element in the value array */

	else if (isdigit(*(buf + i))) {
	    sscanf(buf + i, "%f", value + cnt);
	    while (isdigit(*(buf + i)))
		i++;
	    cnt++;
	}
    }
    *count = (float)cnt;
    G_free(buf);
    return;
}





			 /* READ IN ONE CLASS LINE */

void read_line(char *name, int line, int n, int *value, int *fvalue,
	       int *number_classes)
{
    FILE *fp;
    int i;
    char path[GPATH_MAX], *buf;

    sprintf(path, "r.le.para/%s", name);

    fp = fopen2(path, "r");
    buf = G_malloc(256);

    for (i = 0; i < line - 1; i++)
	fgets(buf, 256, fp);
    G_free(buf);

    if (n > 0)
	for (i = 0; i < n; i++)
	    fscanf(fp, "%d", value + i);
    else {
	for (i = 1;; i++) {
	    fscanf(fp, "%f", (float *)fvalue + i);
	    if (fvalue[i] <= -999) {
		*number_classes = i - 1;
		break;
	    }
	}
	if (3 > (fvalue[0] = (float)i)) {
	    buf = G_malloc(40);
	    sprintf(buf,
		    "\n No data in file\"%s\"; use r.le.setup to make file\n",
		    path);
	    G_fatal_error(buf);
	    G_free(buf);
	}
    }
    fclose(fp);
    return;
}






			 /* READ IN SAMPLING UNIT PARAMETERS
			    AND RUN R.LE.PATCH */

void unit_driver()
{
    int top, left, u_w, u_l, nscl, nu, fd;
    char *buf, unitname[10], istr[3];
    register int i, j, k, m;
    struct Cell_head wind;
    FILE *fp;
    CELL **units, *unit_buf;
    float radius = 0.0;


    G_get_set_window(&wind);
    fp = fopen1("r.le.para/units", "r");

    buf = G_malloc(513);

    /* get the number of scales */

    fgets(buf, 512, fp);
    sscanf(buf, "%d", &nscl);

    /* dynamically allocate storage for the buffer 
       that will hold the map of the sampling units */

    if (choice->units) {
	units = (CELL **) G_calloc(wind.rows + 3, sizeof(CELL *));
	for (i = 0; i < wind.rows + 3; i++)
	    units[i] = (CELL *) G_calloc(wind.cols + 3, sizeof(CELL));
    }

    /* for each scale */

    for (i = 0; i < nscl; i++) {
	n_scale = i + 1;
	fgets(buf, 512, fp);
	sscanf(buf, "%d", &nu);

	/* get the width and length */

	fgets(buf, 512, fp);
	sscanf(buf, "%d%d", &u_w, &u_l);

	/* get the radius to see if sampling
	   units are circles */

	fgets(buf, 512, fp);
	sscanf(buf, "%f", &radius);

	/* if units map was chosen, zero it,
	   then copy the number of the map
	   to the end of the word "units" */

	if (choice->units) {
	    for (k = 0; k < wind.rows + 3; k++) {
		for (m = 0; m < wind.cols + 3; m++)
		    *(*(units + k) + m) = 0;
	    }

	    if (i == 0)
		strcpy(istr, "1");
	    else if (i == 1)
		strcpy(istr, "2");
	    else if (i == 2)
		strcpy(istr, "3");
	    else if (i == 3)
		strcpy(istr, "4");
	    else if (i == 4)
		strcpy(istr, "5");
	    else if (i == 5)
		strcpy(istr, "6");
	    else if (i == 6)
		strcpy(istr, "7");
	    else if (i == 7)
		strcpy(istr, "8");
	    else if (i == 8)
		strcpy(istr, "9");
	    else if (i == 9)
		strcpy(istr, "10");
	    else if (i == 10)
		strcpy(istr, "11");
	    else if (i == 11)
		strcpy(istr, "12");
	    else if (i == 12)
		strcpy(istr, "13");
	    else if (i == 13)
		strcpy(istr, "14");
	    else if (i == 14)
		strcpy(istr, "15");
	    else if (i > 14) {
		fprintf(stderr, "\n");
		fprintf(stderr,
			"   ***************************************************\n");
		fprintf(stderr,
			"    You cannot choose more than 15 scales             \n");
		fprintf(stderr,
			"   ***************************************************\n");
		exit(EXIT_FAILURE);
	    }
	}
	/* for each unit */

	for (j = 0; j < nu; j++) {
	    n_unit = j + 1;
	    fgets(buf, 512, fp);
	    sscanf(buf, "%d%d", &left, &top);

	    /* call cell_clip driver */

	    run_clip(wind.cols, wind.rows, u_w, u_l, left, top, units, j,
		     radius);
	}

	/* if a map of the sampling units
	   was requested */
	if (choice->units) {
	    strcpy(unitname, "units_");
	    strcat(unitname, istr);
	    fd = Rast_open_new(unitname, CELL_TYPE);
	    unit_buf = Rast_allocate_buf(CELL_TYPE);
	    for (k = 1; k < wind.rows + 1; k++) {
		Rast_zero_buf(unit_buf, CELL_TYPE);
		Rast_set_null_value(unit_buf, wind.cols + 1, CELL_TYPE);
		for (m = 1; m < wind.cols + 1; m++) {
		    if (*(*(units + k) + m))
			*(unit_buf + m - 1) = *(*(units + k) + m);
		}
		Rast_put_row(fd, unit_buf, CELL_TYPE);
	    }
	    Rast_close(fd);
	    G_free(unit_buf);
	}
    }

    if (choice->units) {
	for (m = 0; m < wind.rows + 3; m++)
	    G_free(units[m]);
	G_free(units);
    }
    G_free(buf);
    fclose(fp);
    return;
}





			 /* CHECK FOR OUT-OF MAP UNIT, THEN
			    CALL CELL CLIP DRIVER */

void run_clip(int ncols, int nrows, int u_w, int u_l, int left, int top,
	      CELL ** units, int id, float radius)
{
    int i, j;
    double center_row, center_col;
    double dist;

    G_sleep_on_error(0);

    /* check unit */

    if (ncols < left + u_w || nrows < top + u_l) {
	fprintf(stderr, "\n");
	fprintf(stderr,
		"   ******************************************************\n");
	fprintf(stderr,
		"    Sampling units do not fit within the current region. \n");
	fprintf(stderr,
		"    Either correct the region or redo the sampling unit  \n");
	fprintf(stderr,
		"    selection using r.le.setup.  This error message came \n");
	fprintf(stderr,
		"    from an analysis of the r.le.para/units file and the \n");
	fprintf(stderr,
		"    current region setting.                              \n");
	fprintf(stderr,
		"   ******************************************************\n");
	exit(EXIT_FAILURE);
    }

    if (choice->units) {
	if (radius) {
	    center_row = ((double)(top + 1) + ((double)u_l - 1) / 2);
	    center_col = ((double)(left + 1) + ((double)u_w - 1) / 2);

	    for (i = top + 1; i < top + 1 + u_l; i++) {
		for (j = left + 1; j < left + 1 + u_w; j++) {
		    dist =
			sqrt(((double)i - center_row) * ((double)i -
							 center_row) +
			     ((double)j - center_col) * ((double)j -
							 center_col));
		    if (dist < radius)
			*(*(units + i) + j) = id + 1;
		}
	    }
	}
	else {
	    for (i = top + 1; i < top + 1 + u_l; i++) {
		for (j = left + 1; j < left + 1 + u_w; j++)
		    *(*(units + i) + j) = id + 1;
	    }
	}
    }

    cell_clip_drv(left, top, u_w, u_l, NULL, 0, radius);

    return;
}



			 /* CLIP THE REGION, THEN
			    RUN R.LE.PATCH */

void whole_reg_driver()
{
    register int i, j;
    int regcnt, found, fr, nrows, ncols;
    REGLIST *ptrfirst, *ptrthis, *ptrnew;
    CELL *row_buf;

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    n_scale = 1;

    if (choice->wrum != 'r') {
	cell_clip_drv(0, 0, ncols, nrows, NULL, 0, 0.0);
    }
    else {
	regcnt = 0;
	fr = Rast_open_old(choice->reg, G_mapset());
	row_buf = Rast_allocate_buf(CELL_TYPE);
	for (i = 0; i < nrows; i++) {
	    Rast_zero_buf(row_buf, CELL_TYPE);
	    Rast_get_row(fr, row_buf, i, CELL_TYPE);
	    for (j = 0; j < ncols; j++) {
		if (*(row_buf + j) > 0) {
		    if (regcnt == 0)
			ptrfirst = (REGLIST *) NULL;
		    ptrthis = ptrfirst;
		    found = 0;
		    while (ptrthis) {
			if (*(row_buf + j) == ptrthis->att) {
			    if (j < ptrthis->w)
				ptrthis->w = j;
			    if (j > ptrthis->e)
				ptrthis->e = j;
			    if (i < ptrthis->n)
				ptrthis->n = i;
			    if (i > ptrthis->s)
				ptrthis->s = i;
			    found = 1;
			}
			ptrthis = ptrthis->next;
		    }
		    if (!found) {
			ptrnew = (REGLIST *) G_calloc(1, sizeof(REGLIST));
			if (ptrfirst == (REGLIST *) NULL)
			    ptrfirst = ptrthis = ptrnew;
			else {
			    ptrthis = ptrfirst;
			    while (ptrthis->next != (REGLIST *) NULL)
				ptrthis = ptrthis->next;
			    ptrthis->next = ptrnew;
			    ptrthis = ptrnew;
			}
			ptrthis->att = *(row_buf + j);
			ptrthis->n = i;
			ptrthis->s = i;
			ptrthis->e = j;
			ptrthis->w = j;
			regcnt++;
		    }
		}
	    }
	}
	n_unit = 0;
	ptrthis = ptrfirst;
	while (ptrthis) {
	    n_unit = ptrthis->att;
	    cell_clip_drv(ptrthis->w, ptrthis->n, ptrthis->e - ptrthis->w + 1,
			  ptrthis->s - ptrthis->n + 1, NULL, ptrthis->att,
			  0.0);
	    ptrthis = ptrthis->next;
	}
	Rast_close(fr);
	G_free(row_buf);
	G_free(ptrnew);
    }

    return;
}
