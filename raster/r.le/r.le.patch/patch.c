/*
 ************************************************************
 * MODULE: r.le.patch/patch.c                               *
 *         Version 5.0                Nov. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To analyze attributes of patches in a landscape *
 *         patch.c computes and saves the patch measures    *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/

#include <grass/gis.h>
#include <grass/config.h>
#include "patch.h"




extern struct CHOICE *choice;
extern int total_patches, ntype, n_scale, n_unit, *recl_count;
extern float *shape_PA, *shape_CPA, *shape_RCC, **recl_tb, *size_cl;
FILE *a5, *a6, *a1_4, *a7, *a8, *c1_4, *c5, *c6, *c7, *c8, *c9c, *c9e, *c10c,
    *c10e, *s3, *s4, *s5, *s6, *s7_8, *s1_2, *h3, *h4, *h5, *h6, *h1_2, *n1_4,
    *p4, *p5, *p6, *p1_3, *outfile;
int i;


		     /************************************/
		     /*  RUN THE DEFAULT PATCH MEASURES  */

		     /************************************/

void df_patch(PATCH * patch_list)
{
    PATCH *tmp = patch_list;
    int *type_dens, type_coh;
    char path[GNAME_MAX+10];

    if (!total_patches)
	return;

    /*  Write the scale and unit in each file */

  /***********************************************************/

    if (choice->att[1] || choice->att[2] || choice->att[3] || choice->att[4]) {
	a1_4 = fopen0("r.le.out/a1-4.out", "a");
	fprintf(a1_4, "%5d %5d", n_scale, n_unit);
    }

    if (choice->att[5]) {
	a5 = fopen0("r.le.out/a5.out", "a");
	fprintf(a5, "%5d %5d", n_scale, n_unit);
    }

    if (choice->att[6]) {
	a6 = fopen0("r.le.out/a6.out", "a");
	fprintf(a6, "%5d %5d", n_scale, n_unit);
    }

    if (choice->att[7]) {
	a7 = fopen0("r.le.out/a7.out", "a");
	fprintf(a7, "%5d %5d", n_scale, n_unit);
    }

    if (choice->att[8]) {
	a8 = fopen0("r.le.out/a8.out", "a");
	fprintf(a8, "%5d %5d", n_scale, n_unit);
    }

  /***********************************************************/

    if (choice->size[1] || choice->size[2]) {
	s1_2 = fopen0("r.le.out/s1-2.out", "a");
	fprintf(s1_2, "%5d %5d", n_scale, n_unit);
    }

    if (choice->size[3]) {
	s3 = fopen0("r.le.out/s3.out", "a");
	fprintf(s3, "%5d %5d", n_scale, n_unit);
    }

    if (choice->size[4]) {
	s4 = fopen0("r.le.out/s4.out", "a");
	fprintf(s4, "%5d %5d", n_scale, n_unit);
    }

    if (choice->size[5]) {
	s5 = fopen0("r.le.out/s5.out", "a");
	fprintf(s5, "%5d %5d", n_scale, n_unit);
    }

    if (choice->size[6]) {
	s6 = fopen0("r.le.out/s6.out", "a");
	fprintf(s6, "%5d %5d\n", n_scale, n_unit);
    }

    if (choice->size[7] || choice->size[8]) {
	s7_8 = fopen0("r.le.out/s7-8.out", "a");
	fprintf(s7_8, "%5d %5d", n_scale, n_unit);
    }


  /***********************************************************/

    if (choice->core[1] || choice->core[2] ||
	choice->core[3] || choice->core[4]) {
	c1_4 = fopen0("r.le.out/c1-4.out", "a");
	fprintf(c1_4, "%5d %5d", n_scale, n_unit);
    }

    if (choice->core[5]) {
	c5 = fopen0("r.le.out/c5.out", "a");
	fprintf(c5, "%5d %5d", n_scale, n_unit);
    }

    if (choice->core[6]) {
	c6 = fopen0("r.le.out/c6.out", "a");
	fprintf(c6, "%5d %5d", n_scale, n_unit);
    }

    if (choice->core[7]) {
	c7 = fopen0("r.le.out/c7.out", "a");
	fprintf(c7, "%5d %5d", n_scale, n_unit);
    }

    if (choice->core[8]) {
	c8 = fopen0("r.le.out/c8.out", "a");
	fprintf(c8, "%5d %5d", n_scale, n_unit);
    }

    if (choice->core[9]) {
	c9c = fopen0("r.le.out/c9c.out", "a");
	fprintf(c9c, "%5d %5d", n_scale, n_unit);
	c9e = fopen0("r.le.out/c9e.out", "a");
	fprintf(c9e, "%5d %5d", n_scale, n_unit);
    }

    if (choice->core[10]) {
	c10c = fopen0("r.le.out/c10c.out", "a");
	fprintf(c10c, "%5d %5d\n", n_scale, n_unit);
	c10e = fopen0("r.le.out/c10e.out", "a");
	fprintf(c10e, "%5d %5d\n", n_scale, n_unit);
    }

/************************************************************/

    if (choice->shape[1] || choice->shape[2]) {
	h1_2 = fopen0("r.le.out/h1-2.out", "a");
	fprintf(h1_2, "%5d %5d", n_scale, n_unit);
    }

    if (choice->shape[3]) {
	h3 = fopen0("r.le.out/h3.out", "a");
	fprintf(h3, "%5d %5d", n_scale, n_unit);
    }

    if (choice->shape[4]) {
	h4 = fopen0("r.le.out/h4.out", "a");
	fprintf(h4, "%5d %5d", n_scale, n_unit);
    }

    if (choice->shape[5]) {
	h5 = fopen0("r.le.out/h5.out", "a");
	fprintf(h5, "%5d %5d", n_scale, n_unit);
    }

    if (choice->shape[6]) {
	h6 = fopen0("r.le.out/h6.out", "a");
	fprintf(h6, "%5d %5d\n", n_scale, n_unit);
    }


/************************************************************/


    if (choice->boundary[1] || choice->boundary[2] ||
	choice->boundary[3] || choice->boundary[4]) {
	n1_4 = fopen0("r.le.out/n1-4.out", "a");
	fprintf(n1_4, "%5d %5d", n_scale, n_unit);
    }

/************************************************************/


    if (choice->perim[1] || choice->perim[2] || choice->perim[3]) {
	p1_3 = fopen0("r.le.out/p1-3.out", "a");
	fprintf(p1_3, "%5d %5d", n_scale, n_unit);
    }

    if (choice->perim[4]) {
	p4 = fopen0("r.le.out/p4.out", "a");
	fprintf(p4, "%5d %5d", n_scale, n_unit);
    }

    if (choice->perim[5]) {
	p5 = fopen0("r.le.out/p5.out", "a");
	fprintf(p5, "%5d %5d", n_scale, n_unit);
    }

    if (choice->perim[6]) {
	p6 = fopen0("r.le.out/p6.out", "a");
	fprintf(p6, "%5d %5d", n_scale, n_unit);
    }


    type_dens = (int *)G_calloc(25, sizeof(int));
    for (i = 0; i < 25; i++)
	type_dens[i] = 0;

    /* for each patch on the patch list */

    while (tmp) {
	if ((type_coh = recl_coh(tmp->att)) >= 0)
	    type_dens[type_coh]++;
	if (choice->att[0])
	    df_att(tmp, type_coh, type_dens);
	if (choice->core[0])
	    df_core(tmp, type_coh, type_dens);
	if (choice->size[0])
	    df_size(tmp, type_coh, type_dens);
	if (choice->shape[0])
	    df_shape(tmp, type_coh, type_dens);
	if (choice->perim[0])
	    df_perim(tmp, type_coh, type_dens);
	if (choice->boundary[0])
	    df_boundary(tmp);

	if (strcmp(choice->out, "") && choice->wrum != 'm') {
	    sprintf(path, "r.le.out/%s", choice->out);
	    outfile = fopen0(path, "a");
	    fprintf(outfile, "%3d %3d %6d %7.1f %4.0f %4.0f %8.0f \
%8.0f %8.0f %8.0f %6.3f %6.3f %6.3f %8d %4.3f\n", n_scale, n_unit, tmp->num, tmp->att, tmp->c_row, tmp->c_col, tmp->area, tmp->core, tmp->edge, tmp->perim, tmp->perim / tmp->area, 0.282 * tmp->perim / sqrt(tmp->area), 2.0 * sqrt(tmp->area / PI) / tmp->long_axis, tmp->twist, tmp->omega);
	    fclose(outfile);
	}

	tmp = tmp->next;
    }

    if (choice->att[1] || choice->att[2] || choice->att[3] || choice->att[4])
	fclose(a1_4);
    if (choice->att[5])
	fclose(a5);
    if (choice->att[6])
	fclose(a6);
    if (choice->att[7])
	fclose(a7);
    if (choice->att[8])
	fclose(a8);

    if (choice->core[1] || choice->core[2] ||
	choice->core[3] || choice->core[4])
	fclose(c1_4);
    if (choice->core[5])
	fclose(c5);
    if (choice->core[6])
	fclose(c6);
    if (choice->core[7])
	fclose(c7);
    if (choice->core[8])
	fclose(c8);
    if (choice->core[9]) {
	fclose(c9c);
	fclose(c9e);
    }
    if (choice->core[10]) {
	fclose(c10c);
	fclose(c10e);
    }
    if (choice->size[1] || choice->size[2])
	fclose(s1_2);
    if (choice->size[3])
	fclose(s3);
    if (choice->size[4])
	fclose(s4);
    if (choice->size[5])
	fclose(s5);
    if (choice->size[6])
	fclose(s6);
    if (choice->size[7] || choice->size[8])
	fclose(s7_8);

    if (choice->shape[1] || choice->shape[2])
	fclose(h1_2);
    if (choice->shape[3])
	fclose(h3);
    if (choice->shape[4])
	fclose(h4);
    if (choice->shape[5])
	fclose(h5);
    if (choice->shape[6])
	fclose(h6);

    if (choice->boundary[1] || choice->boundary[2] ||
	choice->boundary[3] || choice->boundary[4])
	fclose(n1_4);

    if (choice->perim[1] || choice->perim[2] || choice->perim[3])
	fclose(p1_3);
    if (choice->perim[4])
	fclose(p4);
    if (choice->perim[5])
	fclose(p5);
    if (choice->perim[6])
	fclose(p6);

    G_free(type_dens);
    total_patches = 0;
    return;
}



		     /************************************/
		     /*  COMPUTE THE ATTRIBUTE MEASURES  */

		     /************************************/

void df_att(PATCH * tmp, int type_coh, int *type_dens)
{
    static double sumx = 0.0, sumx2 = 0.0, w_att = 0.0, w_att2 = 0.0, total =
	0.0;
    static double *area, total2 = 0.0;


    /* variables:
       IN:
       tmp =        the next patch on the patch list
       type_coh =   identification no. for the group for this patch
       *type_dens = array of no. of patches by group
       INTERNAL:
       sumx =       sum of all the patch attributes
       sumx2 =      sum of all the patch attributes squared
       w_att =      sum of (patch attributes x areas)
       w_att2 =     sum of (patch attributes x areas squared)
       total =      sum of all the patch areas
       *area =      array of total areas by gp
     */


    if (tmp->num == 1)
	area = (double *)G_calloc(25, sizeof(double));

    sumx += tmp->att;
    sumx2 += tmp->att * tmp->att;
    w_att += tmp->area * tmp->att;
    w_att2 += tmp->area * tmp->att * tmp->att;
    total += tmp->area;
    total2 += tmp->area * tmp->area;
    area[type_coh] += tmp->area;

    if (!tmp->next) {
	save_att(w_att, w_att2, total, total2, sumx, sumx2, type_dens, area);
	w_att = w_att2 = total = sumx = sumx2 = 0.0;
	G_free(area);
    }
    return;
}



		     /************************************/
		     /*    SAVE THE ATTRIBUTE MEASURES   */

		     /************************************/

void save_att(double w_att, double w_att2, double t_size, double t_size2,
	      double sum, double sum2, int *density, double *area)
{
    register int i;
    double wm, wstdv, m, stdv;

    /* variables:
       IN: from df_att
       w_att =    sum of (patch attributes x areas)
       w_att2 =   sum of (patch attributes x areas squared)
       t_size =   sum of all the patch areas
       t_size2 =  sum of all the patch areas squared
       sum =      sum of all the patch attributes
       sum2 =     sum of all the patch attributes squared
       *area =    array of total areas by group
       *density = array of no. of patches by group
       INTERNAL:
       wm =            mean pixel attribute (a1)
       wstdv =         st. dev. pixel attribute (a2)
       m =     mean patch attribute (a3)
       stdv =          st. dev. patch attribute (a4)
       GLOBAL:
       total_patches = tot. no. patches in sampling area (fm. trace.c)
     */

    wm = w_att / t_size;
    wstdv = w_att2 / t_size - wm * wm;
    if (wstdv > 0)
	wstdv = sqrt(wstdv);
    else
	wstdv = 0;
    m = sum / total_patches;
    stdv = sum2 / total_patches - m * m;
    if (stdv > 0)
	stdv = sqrt(stdv);
    else
	stdv = 0;

    /* write a1=mn. pix. att., a2=s.d. pix. att.,
       a3=mn. patch att., a4=s.d. patch att. */

    if (choice->att[1] || choice->att[2] || choice->att[3] || choice->att[4]) {
	fprintf(a1_4, "  %15.3f  %15.3f  %15.3f  %15.3f\n", wm, wstdv, m,
		stdv);
    }

    /* write a5 = cover by gp */

    if (choice->att[5]) {
	for (i = 0; i < ntype; i++)
	    fprintf(a5, " %11.3f", area[i] / t_size);
	fprintf(a5, "\n");
    }

    /* write a6 = density by gp */

    if (choice->att[6]) {
	for (i = 0; i < ntype; i++)
	    fprintf(a6, "  %10d", *(density + i));
	fprintf(a6, "\n");
    }

    /* write a7 = total patches */

    if (choice->att[7])
	fprintf(a7, "      %11d\n", total_patches);

    /* write a8 = eff. mesh no. */

    if (choice->att[8]) {
	if (t_size2 > 0.0)
	    fprintf(a8, "      %11.3f\n", (t_size * t_size) / t_size2);
	else
	    fprintf(a8, "      %11.3f\n", t_size2);
    }

    return;
}



		     /************************************/
		     /*    COMPUTE THE CORE MEASURES     */

		     /************************************/

void df_core(PATCH * tmp, int type_coh, int *type_dens)
{
    static int **density1c = NULL, **density1e = NULL,
	*densityc = NULL, *densitye = NULL, first = 1;
    static double mcore = 0.0, medge = 0.0, *mcore1 = NULL, *medge1 = NULL,
	sumc2 = 0.0, sume2 = 0.0, *sum22c, *sum22e;
    register int i;
    int core_coh, edge_coh;

    /* variables:
       IN:
       tmp =           the next patch on the patch list
       type_coh =   identif. no. for the group for this patch
       type_dens[] =        array of no. of patches by group
       INTERNAL:
       mcore =              sum of patch core sizes
       medge =              sum of patch edge sizes
       sumc2 =              sum of patch cores squared
       sume2 =              sum of patch edges squared
       mcore1[] =   array of patch cores by group
       medge1[] =   array of patch edges by group
       sum22c[] =   array of patch cores squared by group
       sum22e[] =   array of patch edges squared by group
       densityc[] = array of no. of patches by core size class
       densitye[] = array of no. of patches by edge size class
       density1c[] =        array of no. of patches by core size class
       by group
       density1e[] =        array of no. of patches by edge size class
       by group
     */


    if (first) {
	densityc = (int *)G_calloc(25, sizeof(int));
	densitye = (int *)G_calloc(25, sizeof(int));
	sum22c = (double *)G_calloc(25, sizeof(double));
	sum22e = (double *)G_calloc(25, sizeof(double));
	mcore1 = (double *)G_calloc(25, sizeof(double));
	medge1 = (double *)G_calloc(25, sizeof(double));
    }

    /* if output is by size class (c9 & c10) determine
       which size class the current patch is in */

    if (choice->core[9] || choice->core[10]) {
	core_coh = index_coh(tmp->core, size_cl);
	densityc[core_coh]++;
	edge_coh = index_coh(tmp->edge, size_cl);
	densitye[edge_coh]++;
    }

    mcore += tmp->core;
    medge += tmp->edge;
    sumc2 += tmp->core * tmp->core;
    sume2 += tmp->edge * tmp->edge;

    if (type_coh >= 0) {
	mcore1[type_coh] += tmp->core;
	medge1[type_coh] += tmp->edge;
	sum22c[type_coh] += tmp->core * tmp->core;
	sum22e[type_coh] += tmp->edge * tmp->edge;
    }

    /* if c10 */

    if (choice->core2) {
	if (first) {
	    density1c = (int **)G_calloc(25, sizeof(int *));
	    for (i = 0; i < 25; i++)
		density1c[i] = (int *)G_calloc(25, sizeof(int));
	    density1e = (int **)G_calloc(25, sizeof(int *));
	    for (i = 0; i < 25; i++)
		density1e[i] = (int *)G_calloc(25, sizeof(int));
	}
	if (type_coh >= 0) {
	    if (core_coh >= 0)
		density1c[type_coh][core_coh]++;
	    if (edge_coh >= 0)
		density1e[type_coh][edge_coh]++;
	}
    }

    if (first)
	first = 0;

    if (!tmp->next) {
	save_core(sumc2, sume2, mcore, medge, mcore1, medge1, sum22c, sum22e,
		  densityc, densitye, type_dens, density1c, density1e);
	mcore = medge = sumc2 = sume2 = 0;
	G_free(densityc);
	G_free(densitye);
	G_free(sum22c);
	G_free(sum22e);
	G_free(mcore1);
	G_free(medge1);
	if (choice->core2) {
	    for (i = 0; i < 25; i++) {
		G_free(density1c[i]);
		G_free(density1e[i]);
	    }
	    G_free(density1c);
	    G_free(density1e);
	}
	first = 1;
    }
    return;
}




		     /************************************/
		     /*     SAVE THE CORE MEASURES       */

		     /************************************/

void save_core(double sumc2, double sume2, double mcore, double medge,
	       double *mcore1, double *medge1, double *sum22c, double *sum22e,
	       int *densityc, int *densitye, int *type_dens, int **density1c,
	       int **density1e)
{
    register int i, j;
    double tmpc, tmpe, stdvc, stdve;


    /* variables:
       IN:
       sumc2 =              sum of patch cores squared
       sume2 =              sum of patch edges squared
       mcore =              sum of patch cores
       medge =              sum of patch edges
       mcore1[] =   array of sums of patch cores by gp
       medge1[] =   array of sums of patch edges by gp
       sum22c[] =   array of patch cores squared by gp
       sum22e[] =   array of patch edges squared by gp
       densityc[] = array of no. of patches by core size class
       densitye[] = array of no. of patches by edge size class
       type_dens[] =        array of no. of patches by gp ?
       INTERNAL:
       tmpc =               mean patch core (c1 & c5)
       stdvc =              st. dev. patch core (c2 & c6)
       tmpe =               mean patch edge (c3 & c7)
       stdve =              st. dev. patch edge (c4 & c8)
       GLOBAL:
       total_patches = tot. no. patches in sampling area (fm. trace.c)
     */


    /* calc. & write c1=mn. core, c2=s.d. core,
       c3=mn. edge, c4=s.d. edge */

    if (choice->core[1] || choice->core[2] ||
	choice->core[3] || choice->core[4]) {
	tmpc = mcore / total_patches;
	stdvc = sumc2 / total_patches - tmpc * tmpc;
	if (stdvc > 0)
	    stdvc = sqrt(stdvc);
	else
	    stdvc = 0;
	tmpe = medge / total_patches;
	stdve = sume2 / total_patches - tmpe * tmpe;
	if (stdve > 0)
	    stdve = sqrt(stdve);
	else
	    stdve = 0;
	fprintf(c1_4, "  %15.3f  %15.3f  %15.3f  %15.3f\n", tmpc, stdvc,
		tmpe, stdve);
    }

    /* calc. & write c5=mn. core by gp */

    if (choice->core[5]) {
	for (i = 0; i < ntype; i++) {
	    if ((tmpc = type_dens[i]))
		tmpc = mcore1[i] / tmpc;
	    fprintf(c5, " %11.3f", tmpc);
	}
	fprintf(c5, "\n");
    }

    /* calc. & write c6=s.d. core by gp */

    if (choice->core[6]) {
	for (i = 0; i < ntype; i++) {
	    stdvc = 0;
	    if (type_dens[i]) {
		tmpc = mcore1[i] / type_dens[i];
		stdvc = sum22c[i] / type_dens[i] - tmpc * tmpc;
		if (stdvc > 0)
		    stdvc = sqrt(stdvc);
		else
		    stdvc = 0;
	    }
	    fprintf(c6, " %11.3f", stdvc);
	}
	fprintf(c6, "\n");
    }

    /* calc. & write c7=mn. edge by gp */

    if (choice->core[7]) {
	for (i = 0; i < ntype; i++) {
	    if ((tmpe = type_dens[i]))
		tmpe = medge1[i] / tmpe;
	    fprintf(c7, " %11.3f", tmpe);
	}
	fprintf(c7, "\n");
    }

    /* calc. & write c8=s.d. edge by gp */

    if (choice->core[8]) {
	for (i = 0; i < ntype; i++) {
	    stdve = 0;
	    if (type_dens[i]) {
		tmpe = medge1[i] / type_dens[i];
		stdve = sum22e[i] / type_dens[i] - tmpe * tmpe;
		if (stdve > 0)
		    stdve = sqrt(stdve);
		else
		    stdve = 0;
	    }
	    fprintf(c8, " %11.3f", stdve);
	}
	fprintf(c8, "\n");
    }

    /* write c9=no. by size class */

    if (choice->core[9]) {
	for (i = 0; i < size_cl[0] - 1; i++)
	    fprintf(c9c, " %11d", *(densityc + i));
	fprintf(c9c, "\n");
	for (i = 0; i < size_cl[0] - 1; i++)
	    fprintf(c9e, " %11d", *(densitye + i));
	fprintf(c9e, "\n");
    }


    /* write c10=no. by size class by gp */

    if (choice->core2) {
	for (i = 0; i < ntype; i++) {
	    fprintf(c10c, "     Gp[%2d]", i + 1);
	    for (j = 0; j < *size_cl - 1; j++)
		fprintf(c10c, " %11d", density1c[i][j]);
	    fprintf(c10c, "\n");
	}

	for (i = 0; i < ntype; i++) {
	    fprintf(c10e, "     Gp[%2d]", i + 1);
	    for (j = 0; j < *size_cl - 1; j++)
		fprintf(c10e, " %11d", density1e[i][j]);
	    fprintf(c10e, "\n");
	}
    }
    return;
}



		     /************************************/
		     /*    COMPUTE THE SIZE MEASURES     */

		     /************************************/

void df_size(PATCH * tmp, int type_coh, int *type_dens)
{
    static int **density1 = NULL, *density = NULL, first = 1;
    static double msize = 0.0, *msize1 = NULL, sum2 = 0.0, *sum22;
    register int i;
    int size_coh;

    /* variables:
       IN:
       tmp =           the next patch on the patch list
       type_coh =   identif. no. for the gp for this patch
       type_dens[] =        array of no. of patches by gp
       INTERNAL:
       msize =              sum of patch sizes
       msize1[] =   array of patch sizes by gp
       sum2 =               sum of patch sizes squared
       sum22[] =    array of patch sizes squared by gp
       density[] =  array of no. of patches by size class
     */


    if (first) {
	density = (int *)G_calloc(25, sizeof(int));
	sum22 = (double *)G_calloc(25, sizeof(double));
	msize1 = (double *)G_calloc(25, sizeof(double));
    }

    /* if output is by size class (s5 & s6) determine which
       size class the current patch is in */

    if (choice->size[5] || choice->size[6]) {
	size_coh = index_coh(tmp->area, size_cl);
	density[size_coh]++;
    }

    msize += tmp->area;
    sum2 += tmp->area * tmp->area;

    if (type_coh >= 0) {
	msize1[type_coh] += tmp->area;
	sum22[type_coh] += tmp->area * tmp->area;
    }


    if (choice->size2) {
	if (first) {
	    density1 = (int **)G_calloc(25, sizeof(int *));
	    for (i = 0; i < 25; i++)
		density1[i] = (int *)G_calloc(25, sizeof(int));
	}
	if (type_coh >= 0 && size_coh >= 0)
	    density1[type_coh][size_coh]++;
    }


    if (first)
	first = 0;

    if (!tmp->next) {
	save_size(sum2, msize, msize1, sum22, density, type_dens, density1);
	msize = sum2 = 0.0;
	G_free(density);
	G_free(msize1);
	G_free(sum22);
	if (density1) {
	    for (i = 0; i < 25; i++)
		G_free(density1[i]);
	    G_free(density1);
	}
	first = 1;
    }
    return;
}




		     /************************************/
		     /*       SAVE THE SIZE MEASURES     */

		     /************************************/

void save_size(double sum2, double msize, double *msize1, double *sum22,
	       int *density, int *type_dens, int **density1)
{
    register int i, j;
    double tmp, stdv;

    /* variables:
       IN:
       sum2 =               sum of patch sizes squared
       msize =              sum of patch sizes
       msize1[] =   array of sums of patch sizes by gp
       sum22[] =    array of patch sizes squared by gp
       density[] =  array of no. of patches by size class
       type_dens[] =        array of no. of patches by gp
       INTERNAL:
       tmp =                mean patch size (s1 & s3)
       stdv =               st. dev. patch size (s2 & s4)
       GLOBAL:
       total_patches = tot. no. patches in sampling area (fm. trace.c)

     */


    /* calc. & write s1=mn. size, s2=s.d. size */

    if (choice->size[1] || choice->size[2]) {
	tmp = msize / total_patches;
	stdv = sum2 / total_patches - tmp * tmp;
	if (stdv > 0)
	    stdv = sqrt(stdv);
	else
	    stdv = 0.0;
	fprintf(s1_2, "  %15.3f  %15.3f\n", tmp, stdv);
    }

    /* calc. & write s3=mn. size by gp */

    if (choice->size[3]) {
	for (i = 0; i < ntype; i++) {
	    if ((tmp = type_dens[i]))
		tmp = msize1[i] / tmp;
	    fprintf(s3, " %11.3f", tmp);
	}
	fprintf(s3, "\n");
    }

    /* calc. & write s4=s.d. size by gp */

    if (choice->size[4]) {
	for (i = 0; i < ntype; i++) {
	    stdv = 0.0;
	    if (type_dens[i]) {
		tmp = msize1[i] / type_dens[i];
		stdv = sum22[i] / type_dens[i] - tmp * tmp;
		if (stdv > 0)
		    stdv = sqrt(stdv);
		else
		    stdv = 0.0;
	    }
	    fprintf(s4, " %11.3f", stdv);
	}
	fprintf(s4, "\n");
    }

    /* write s5=no. by size class */

    if (choice->size[5]) {
	for (i = 0; i < size_cl[0] - 1; i++)
	    fprintf(s5, " %11d", *(density + i));
	fprintf(s5, "\n");
    }


    /* write s6=no. by size class by gp */

    if (choice->size2) {
	/*     if(!(fp = fopen("r.le.out/s6.out", "a")))
	   G_fatal_error("Can't write file s6.out; do you have write permission?\n");
	   fprintf(fp, "%5d %5d\n", n_scale, n_unit);     */
	for (i = 0; i < ntype; i++) {
	    fprintf(s6, "     Gp[%2d]", i + 1);
	    for (j = 0; j < *size_cl - 1; j++)
		fprintf(s6, " %11d", density1[i][j]);
	    fprintf(s6, "\n");
	}
    }

    /* calculate & write s7 (eff. mesh size)
       & s8 (deg. landsc. division) */

    if (choice->size[7] || choice->size[8])
	fprintf(s7_8, "  %15.3f  %15.3f\n", (1.0 / msize) * sum2,
		1.0 - (sum2 / (msize * msize)));

    return;
}




		     /************************************/
		     /*     COMPUTE THE SHAPE MEASURES   */

		     /************************************/

void df_shape(PATCH * tmp, int type_coh, int *type_dens)
{
    static int *den1, *den2, *den3, **density1 = NULL, **density2 = NULL,
	**density3, new = 1;
    static double mshape = 0.0, *mshape1 = NULL, mshape_p = 0.0, *mshape1_p,
	*sqr11, *sqr21, *sqr31, mshape_r = 0.0, *mshape1_r, sq1 = 0.0,
	sq2 = 0.0, sq3 = 0.0;
    register int i;
    int shape_coh1 = 0, shape_coh2 = 0, shape_coh3 = 0;
    double shp1, shp2, shp3;


    /* variables:
       IN:
       tmp =           the next patch on the patch list
       type_coh =   identif. no. for the gp for this patch
       type_dens[] =        array of no. of patches by gp
       INTERNAL:
       shp1 =               CPA shape index
       shp2 =               PA shape index
       shp3 =               RCC shape index
       mshape =     sum of CPA shape indices for all patches
       mshape_p =   sum of PA shape indices for all patches
       mshape_r =   sum of RCC shape indices for all patches
       mshape1[] =  array of CPA indices by gp
       mshape1_p[] =        array of PA indices by gp
       mshape1_r[] =        array of RCC indices by gp
       sq1 =                sum of CPA indices squared
       sq2 =                sum of PA indices squared
       sq3 =                sum of RCC indices squared
       sqr11[] =    array of CPA indices squared by gp
       sqr21[] =    array of PA indices squared by gp
       sqr31[] =    array of RCC indices squared by gp
       den1[] =     array of CPA indices by index class
       den2[] =     array of PA indices by index class
       den3[] =     array of RCC indices by index class
       density1[] =         array of CPA indices by index class by gp
       density2[] = array of PA indices by index class by gp
       density3[] = array of RCC indices by index class by gp
     */


    shp1 = 0.282 * tmp->perim / sqrt(tmp->area);	/* CPA method m2 */
    shp2 = tmp->perim / tmp->area;	/* PA method m1 */
    shp3 = 2.0 * sqrt(tmp->area / PI) / tmp->long_axis;	/* RCC method m3 */

    if (new) {
	mshape1 = (double *)G_calloc(25, sizeof(double));
	mshape1_p = (double *)G_calloc(25, sizeof(double));
	mshape1_r = (double *)G_calloc(25, sizeof(double));
	sqr11 = (double *)G_calloc(25, sizeof(double));
	sqr21 = (double *)G_calloc(25, sizeof(double));
	sqr31 = (double *)G_calloc(25, sizeof(double));
	den1 = (int *)G_calloc(25, sizeof(int));
	den2 = (int *)G_calloc(25, sizeof(int));
	den3 = (int *)G_calloc(25, sizeof(int));
    }


    mshape += shp1;
    mshape_p += shp2;
    mshape_r += shp3;
    sq1 += shp1 * shp1;
    sq2 += shp2 * shp2;
    sq3 += shp3 * shp3;

    if (type_coh >= 0) {
	mshape1[type_coh] += shp1;
	mshape1_p[type_coh] += shp2;
	mshape1_r[type_coh] += shp3;
	sqr11[type_coh] += shp1 * shp1;
	sqr21[type_coh] += shp2 * shp2;
	sqr31[type_coh] += shp3 * shp3;
    }



    /* if sh2=h5 or h6 */

    if (choice->shape[5] || choice->shape[6]) {

	/* if sh1=m1 */

	if (choice->Mx[1]) {
	    if (0 <= (shape_coh2 = index_coh(shp2, shape_PA)))
		den2[shape_coh2]++;
	}

	/* if sh1=m2 */

	if (choice->Mx[2]) {
	    if (0 <= (shape_coh1 = index_coh(shp1, shape_CPA)))
		den1[shape_coh1]++;
	}

	/* if sh1=m3 */

	if (choice->Mx[3]) {
	    if (0 <= (shape_coh3 = index_coh(shp3, shape_RCC)))
		den3[shape_coh3]++;
	}
    }

    /* if sh2=h6 */

    if (choice->shape2) {
	if (new) {
	    density1 = (int **)G_calloc(25, sizeof(int *));
	    density2 = (int **)G_calloc(25, sizeof(int *));
	    density3 = (int **)G_calloc(25, sizeof(int *));

	    for (i = 0; i < 25; i++)
		if (!(density1[i] = (int *)G_calloc(25, sizeof(int))) ||
		    !(density2[i] = (int *)G_calloc(25, sizeof(int))) ||
		    !(density3[i] = (int *)G_calloc(25, sizeof(int))))
		    G_fatal_error
			("Failure to allocate memory for sh2, exit.\n");
	}

	if (type_coh >= 0) {
	    if (shape_coh1 >= 0)
		density1[type_coh][shape_coh1]++;
	    if (shape_coh2 >= 0)
		density2[type_coh][shape_coh2]++;
	    if (shape_coh3 >= 0)
		density3[type_coh][shape_coh3]++;
	}
    }

    if (new)
	new = 0;

    if (!tmp->next) {/** if all the patches are done **/
	save_shape(sq1, sq2, sq3, sqr11, sqr21, sqr31,
		   mshape, mshape_p, mshape_r, mshape1, mshape1_p,
		   mshape1_r, type_dens, den1, den2, den3, density1,
		   density2, density3);
	mshape = sq1 = sq2 = sq3 = mshape_p = mshape_r = 0;
	G_free(mshape1);
	G_free(mshape1_p);
	G_free(mshape1_r);
	G_free(sqr11);
	G_free(sqr21);
	G_free(sqr31);
	G_free(den1);
	G_free(den2);
	G_free(den3);
	if (density1) {
	    for (i = 0; i < 25; i++) {
		G_free(density1[i]);
		G_free(density2[i]);
		G_free(density3[i]);
	    }
	    G_free(density1);
	    G_free(density2);
	    G_free(density3);
	}
	new = 1;
    }
    return;
}





		     /************************************/
		     /*      SAVE THE SHAPE MEASURES     */

		     /************************************/

void save_shape(double sq1, double sq2, double sq3, double *sqr11,
		double *sqr21, double *sqr31, double mshape, double mshape_p,
		double mshape_r, double *mshape1, double *mshape1_p,
		double *mshape1_r, int *type_dens, int *den1, int *den2,
		int *den3, int **density1, int **density2, int **density3)
{
    register int i, j;
    double tmp, stdv;

    /* variables:
       IN:
       sq1 =                sum of CPA indices squared
       sq2 =                sum of PA indices squared
       sq3 =                sum of RCC indices squared
       sqr11[] =    array of CPA indices squared by gp
       sqr21[] =    array of PA indices squared by gp
       sqr31[] =    array of RCC indices squared by gp
       mshape =     sum of CPA shape indices for all patches
       mshape_p =   sum of PA shape indices for all patches
       mshape_r =   sum of RCC shape indices for all patches
       mshape1[] =  array of CPA indices by gp
       mshape1_p[] =        array of PA indices by gp
       mshape1_r[] =        array of RCC indices by gp
       type_dens[] =        array of no. of patches by gp ?
       den1[] =     array of CPA indices by index class
       den2[] =     array of PA indices by index class
       den3[] =     array of RCC indices by index class
       density1[] =         array of CPA indices by index class by gp
       density2[] = array of PA indices by index class by gp
       density3[] = array of RCC indices by index class by gp
       INTERNAL:
       tmp =                mean shape index (h1)
       stdv =               st. dev. shape index (h2)
       GLOBAL:
       total_patches = tot. no. patches in sampling area (fm. trace.c)
     */


    /*CALC. & WRITE h1-h5 FOR CPA INDEX (m2) */

    /* calc. & write h1=mn. shape, h2=s.d. shape for CPA index (m2) */

    if ((choice->shape[1] || choice->shape[1]) && choice->Mx[2]) {
	tmp = mshape / total_patches;
	stdv = sq1 / total_patches - tmp * tmp;
	if (stdv > 0)
	    stdv = sqrt(stdv);
	else
	    stdv = 0.0;
	fprintf(h1_2, "  %15.3f  %15.3f\n", tmp, stdv);
    }


    /* calc. & write h3=mn. shape by gp for CPA index (m2) */

    if (choice->shape[3] && choice->Mx[2]) {
	for (i = 0; i < ntype; i++) {
	    if ((tmp = type_dens[i]))
		tmp = mshape1[i] / tmp;
	    fprintf(h3, "  %10.3f", tmp);
	}
	fprintf(h3, "\n");
    }


    /* calc. & write h4=s.d. shape by gp for CPA index (m2) */

    if (choice->shape[4] && choice->Mx[2]) {
	for (i = 0; i < ntype; i++) {
	    stdv = 0.0;
	    if (type_dens[i] > 1) {
		tmp = mshape1[i] / type_dens[i];
		stdv = sqr11[i] / type_dens[i] - tmp * tmp;
		if (stdv > 0)
		    stdv = sqrt(stdv);
		else
		    stdv = 0.0;
	    }
	    fprintf(h4, "  %10.3f", stdv);
	}
	fprintf(h4, "\n");
    }


    /* write h5=no. by shape index class for CPA index (m2) */

    if (choice->shape[5] && choice->Mx[2]) {
	for (j = 0; j < *shape_CPA - 1; j++)
	    fprintf(h5, "  %10d", den1[j]);
	fprintf(h5, "\n");
    }




    /*CALC. & WRITE h1-h5 FOR PA INDEX (m1) */

    /* calc. & write h1=mn. shape, h2=s.d. shape for PA index (m1) */

    if ((choice->shape[1] || choice->shape[2]) && choice->Mx[1]) {
	tmp = mshape_p / total_patches;
	stdv = sq2 / total_patches - tmp * tmp;
	if (stdv > 0)
	    stdv = sqrt(stdv);
	else
	    stdv = 0.0;
	fprintf(h1_2, "  %15.3f  %15.3f\n", tmp, stdv);
    }


    /* calc. & write h3=mn. shape by gp for PA index (m1) */

    if (choice->shape[3] && choice->Mx[1]) {
	for (i = 0; i < ntype; i++) {
	    if ((tmp = type_dens[i]))
		tmp = mshape1_p[i] / tmp;
	    fprintf(h3, "  %10.3f", tmp);
	}
	fprintf(h3, "\n");
    }


    /* calc. & write h4=s.d. shape by gp for PA index (m1) */

    if (choice->shape[4] && choice->Mx[1]) {
	for (i = 0; i < ntype; i++) {
	    stdv = 0.0;
	    if (type_dens[i] > 1) {
		tmp = mshape1_p[i] / type_dens[i];
		stdv = sqr21[i] / type_dens[i] - tmp * tmp;
		if (stdv > 0)
		    stdv = sqrt(stdv);
		else
		    stdv = 0;
	    }
	    fprintf(h4, "  %10.3f", stdv);
	}
	fprintf(h4, "\n");
    }


    /* write h5=no. by shape index class for PA index (m1) */

    if (choice->shape[5] && choice->Mx[1]) {
	for (j = 0; j < *shape_PA - 1; j++)
	    fprintf(h5, "  %10d", den2[j]);
	fprintf(h5, "\n");
    }




    /*CALC. & WRITE h1-h5 FOR RCC INDEX (m3) */

    /*    calc. & write h1=mn. shape, h2=s.d. shape for RCC index (m3) */

    if ((choice->shape[1] || choice->shape[2]) && choice->Mx[3]) {
	tmp = mshape_r / total_patches;
	stdv = sq3 / total_patches - tmp * tmp;
	if (stdv > 0)
	    stdv = sqrt(stdv);
	else
	    stdv = 0.0;
	fprintf(h1_2, "  %15.3f  %15.3f\n", tmp, stdv);
    }



    /*    calc. & write h3=mn. shape by gp for RCC index (m3) */

    if (choice->shape[3] && choice->Mx[3]) {
	for (i = 0; i < ntype; i++) {
	    if ((tmp = type_dens[i]))
		tmp = mshape1_r[i] / tmp;
	    fprintf(h3, "  %10.3f", tmp);
	}
	fprintf(h3, "\n");
    }



    /*    calc. & write h4=s.d. shape by gp for RCC index (m3) */

    if (choice->shape[4] && choice->Mx[3]) {
	for (i = 0; i < ntype; i++) {
	    stdv = 0;
	    if (type_dens[i] > 1) {
		tmp = mshape1_r[i] / type_dens[i];
		stdv = sqr31[i] / type_dens[i] - tmp * tmp;
		if (stdv > 0)
		    stdv = sqrt(stdv);
		else
		    stdv = 0.0;
	    }
	    fprintf(h4, "  %10.3f", stdv);
	}
	fprintf(h4, "\n");
    }



    /*    write h5=no. by shape index class for RCC index (m3) */

    if (choice->shape[5] && choice->Mx[3]) {
	for (j = 0; j < *shape_RCC - 1; j++)
	    fprintf(h5, "  %10d", den3[j]);
	fprintf(h5, "\n");
    }




    /* CALC. & WRITE h6 = NO. IN EA. SHAPE INDEX CLASS BY GP */

    if (choice->shape[6]) {
	if (density1) {

	    if (choice->Mx[1]) {
		for (i = 0; i < ntype; i++) {
		    fprintf(h6, "     Gp[%2d]", i + 1);
		    for (j = 0; j < *shape_PA - 1; j++)
			fprintf(h6, "  %10d", density2[i][j]);
		    fprintf(h6, "\n");
		}
	    }

	    if (choice->Mx[2]) {
		for (i = 0; i < ntype; i++) {
		    fprintf(h6, "     Gp[%2d]", i + 1);
		    for (j = 0; j < *shape_CPA - 1; j++)
			fprintf(h6, "  %10d", density1[i][j]);
		    fprintf(h6, "\n");
		}
	    }

	    if (choice->Mx[3]) {
		for (i = 0; i < ntype; i++) {
		    fprintf(h6, "     Gp[%2d]", i + 1);
		    for (j = 0; j < *shape_RCC - 1; j++)
			fprintf(h6, "  %10d", density3[i][j]);
		    fprintf(h6, "\n");
		}
	    }
	}
    }

    return;
}






		     /****************************************************/
		     /*  COMPUTE AND SAVE BOUNDARY COMPLEXITY MEASURES   */

		     /****************************************************/

void df_boundary(PATCH * tmp)
{

    static double sumomega = 0.0, sumomega2 = 0.0;
    static int sumtwist = 0, sumtwist2 = 0;
    double meantwist = 0.0, stdvtwist = 0.0, meanomega = 0.0, stdvomega = 0.0;

    /* variables:
       IN:
       tmp =                the next patch in the patch list
       INTERNAL:
       meantwist =     mean twist number (n1)
       stdvtwist =     st. dev. twist number (n2)
       meanomega =     mean omega index (n3)
       stdvomega =     st. dev. omega index (n4)
       GLOBAL:
       total_patches = tot. no. patches in sampling area (fm. trace.c)
     */


    sumtwist += tmp->twist;
    sumtwist2 += tmp->twist * tmp->twist;
    sumomega += tmp->omega;
    sumomega2 += tmp->omega * tmp->omega;

    if (!tmp->next) {

	meantwist = (double)sumtwist / total_patches;
	stdvtwist = (double)sumtwist2 / total_patches - meantwist * meantwist;
	if (stdvtwist > 0.0)
	    stdvtwist = sqrt(stdvtwist);
	else
	    stdvtwist = 0.0;

	meanomega = (double)sumomega / total_patches;
	stdvomega = (double)sumomega2 / total_patches - meanomega * meanomega;
	if (stdvomega > 0.0)
	    stdvomega = sqrt(stdvomega);
	else
	    stdvomega = 0.0;

	/* write n1=mn. twist, n2=s.d. twist, n3=mn. omega,
	   n4=s.d. omega */

	if (choice->boundary[1] || choice->boundary[2] ||
	    choice->boundary[3] || choice->boundary[4])
	    fprintf(n1_4, "  %15.3f  %15.3f  %15.3f  %15.3f\n",
		    meantwist, stdvtwist, meanomega, stdvomega);

	meantwist = stdvtwist = meanomega = stdvomega = 0.0;
    }

    return;
}




		     /*************************************/
		     /* COMPUTE & SAVE PERIMETER MEASURES */

		     /*************************************/

void df_perim(PATCH * tmp, int type_coh, int *type_dens)
{
    static double perim = 0.0, *perim1, sum2 = 0.0, *sum21, first = 1.0;
    register int i;
    double mean, stdv;

    if (first) {
	perim1 = (double *)G_calloc(25, sizeof(double));
	sum21 = (double *)G_calloc(25, sizeof(double));
	first = 0.0;
    }


    /* variables:
       IN:
       tmp =                the next patch in the patch list
       type_coh =   identif. no. of the gp for this patch
       type_dens[] =        array of no. of patches by gp
       INTERNAL:
       perim =              sum of perimeters (p1)
       sum2 =               sum of perimeters squared
       perim1[] =   array of sum of perims by gp (pp4)
       sum21[] =    array of sum of perims squared by gp
       mean =               mean perim. (p2)
       stdv =               st. dev. perim. (p3)
       GLOBAL:
       total_patches = tot. no. patches in sampling area (fm. trace.c)
     */


    perim += tmp->perim;
    sum2 += tmp->perim * tmp->perim;
    if (type_coh >= 0) {
	perim1[type_coh] += tmp->perim;
	sum21[type_coh] += tmp->perim * tmp->perim;
    }

    if (!tmp->next) {/** save the perimeter measures **/
	mean = perim / total_patches;
	stdv = sum2 / total_patches - mean * mean;
	if (stdv > 0)
	    stdv = sqrt(stdv);
	else
	    stdv = 0.0;

	/* write p1=sum per., p2=mn. per.,p3=s.d. per. */

	if (choice->perim[1] || choice->perim[2] || choice->perim[3])
	    fprintf(p1_3, "  %15.3f  %15.3f  %15.3f\n", perim, mean, stdv);


	/* write p4=sum per. by gp */

	if (choice->perim[4]) {
	    for (i = 0; i < ntype; i++)
		fprintf(p4, " %11.3f", perim1[i]);
	    fputs("\n", p4);
	}


	/* write p5=mn. per. by gp */

	if (choice->perim[5]) {
	    for (i = 0; i < ntype; i++) {
		if (type_dens[i])
		    mean = perim1[i] / type_dens[i];
		else
		    mean = 0.0;
		fprintf(p5, " %11.3f", mean);
	    }
	    fputs("\n", p5);
	}


	/* calc. & write p6=s.d. per. by gp */

	if (choice->perim[6]) {
	    for (i = 0; i < ntype; i++) {
		stdv = 0;
		if (type_dens[i]) {
		    mean = perim1[i] / type_dens[i];
		    stdv = sum21[i] / type_dens[i] - mean * mean;
		    if (stdv > 0)
			stdv = sqrt(stdv);
		    else
			stdv = 0.0;
		}
		fprintf(p6, " %11.3f", stdv);
	    }
	    fputs("\n", p6);
	}

	G_free(perim1);
	G_free(sum21);
	first = 1;
	perim = sum2 = 0;
    }
    return;
}





		     /************************************/
		     /*    RUN THE MOVING WINDOW PATCH   */
		     /*    MEASURES                      */

		     /************************************/

void mv_patch(PATCH * patch_list, double **value, int index)
{
    PATCH *tmp = patch_list;

    /* Variables:
       IN:
       patch_list = the list of patches
       value =      buffer containing a full row of the results of the moving
       window if a moving window map, otherwise 0
       index =      number of the column in the moving window
       INTERNAL:
       *tmp =       pointer to the list of patches
     */

    if (!total_patches)
	return;
    while (tmp) {
	if (choice->att[0])
	    m_att(tmp, value, index);
	if (choice->size[0])
	    m_size(tmp, value, index);
	if (choice->core[0])
	    m_core(tmp, value, index);
	if (choice->shape[0] && choice->Mx[1])
	    m_shape(tmp, 1, value, index);
	if (choice->shape[0] && choice->Mx[2])
	    m_shape(tmp, 2, value, index);
	if (choice->shape[0] && choice->Mx[3])
	    m_shape(tmp, 3, value, index);
	if (choice->boundary)
	    m_boundary(tmp, value, index);
	if (choice->perim[0])
	    m_perim(tmp, value, index);
	tmp = tmp->next;
    }
    total_patches = 0;
    return;
}




		     /************************************/
		     /* FOR DF_PATCH PROGRAM (NOT MOVING */
		     /* WINDOW) DETERMINE WHICH GROUP    */
		     /* ATT BELONGS TO                   */

		     /************************************/

int recl_coh(double att)
{
    register int i;
    extern int ntype;
    extern float **recl_tb;



    for (i = 0; i < ntype; i++) {
	if (in_group(att, recl_tb[i], i)) {
	    return i;
	}
    }
    return -999;
}



		     /************************************/
		     /* DETERMINE WHETHER ATT BELONGS TO */
		     /* THE CHOSEN rTH GP IN THE RECLASS */
		     /* TABLE                            */

		     /************************************/

int in_group(double att, float *group, int r)
{
    register int i;

    /*Variables
       EXTERNAL
       recl_count = an array containing the number of elements in each
       row of the reclass table
       IN
       att     = the attribute value
       group   = an array containing the rth row of the reclass table
       r       = the chosen row of the reclass table
       INTERNAL
       i       = index
     */

    /* For each element i in the rth row of the reclass
       table.  When this program is called by the
       moving window programs r is always 0 */

    for (i = 1; i < recl_count[r]; i++) {

	/* if the i element of the rth row of the reclass
	   table is -999, this indicates "thru" in the
	   reclass table, so check to see if the attribute
	   lies in the range expressed in the "thru"
	   statement */

	if (-999 == *(group + i)) {
	    if (*(group + i) &&
		*(group + i - 1) <= att && att <= *(group + i + 1))
		return 1;
	    else
		i++;
	}

	/* if the rth row of the reclass table does not
	   contain a "thru" statement, then just check whether
	   the i element of the rth row of the reclass table
	   is equal to the attribute */

	else if ((double)*(group + i) == att) {
	    return 1;
	}
    }

    return 0;
}




		     /************************************/
		     /*  DETERMINE WHICH INDEX CLASS ATT */
		     /*  BELONGS TO                      */

		     /************************************/

int index_coh(double att, float *group)
{
    register int i;

    /*
       Variables:
       IN
       att     = the attribute to be checked
       group   = an array of index class limits (e.g., size classes)
     */

    for (i = (int)*group - 1; i >= 1; i--) {
	if ((double)*(group + i) <= att)
	    return i - 1;
    }
    return -999;
}





		     /************************************/
		     /* MOVING WINDOW ATTRIBUTE MEASURES */

		     /************************************/

void m_att(PATCH * tmp, double **value, int index)
{
    static double sum1 = 0.0, sum12 = 0.0, sum2 = 0.0, sum22 = 0.0, sum32 =
	0.0, total1 = 0.0, total2 = 0.0, area = 0.0, area2 = 0.0;
    static int density = 0;
    double mean, stdv;

    /* choice->att  1 = mean pixel attrib.     (a1)
       2 = st. dev. pixel attrib. (a2)
       3 = mean patch attrib.     (a3)
       4 = st. dev. patch attrib. (a4)
       5 = cover in gp 1          (a5)
       6 = density in gp 1        (a6)
       7 = total density          (a7)
       8 = eff. mesh no.          (a8)

       Variables:
       IN:
       tmp =     the list of patches
       value =   buffer containing a full row of the results of the moving
       window if a moving window map, otherwise 0
       index =   number of the column in the moving window
       INTERNAL:
       sum1 =         sum of patch area x patch attrib.
       sum2 =         sum of patch attributes
       sum12 =   sum of patch area x patch attrib. squared
       sum22 =   sum of patch attributes squared
       sum32 =   sum of patch areas in gp 1
       total1 =  sum of patch areas for a1 and a2
       total2 =  sum of patch areas for a5
       area =    sum of patch areas for a8
       area2 =   sum of patch areas squared
       density = no. of patches in gp 1
       value =        output value for selected att measure
       GLOBAL:
       total_patches = tot. no. patches in sampling area (fm. trace.c)      
     */

    if (choice->att[1] || choice->att[2]) {
	sum1 += tmp->area * tmp->att;
	total1 += tmp->area;
	if (choice->att[2])
	    sum12 += tmp->area * tmp->att * tmp->att;
    }

    if (choice->att[3] || choice->att[4]) {
	sum2 += tmp->att;
	if (choice->att[4])
	    sum22 += tmp->att * tmp->att;
    }

    if (choice->att[5]) {
	total2 += tmp->area;
	if (in_group(tmp->att, recl_tb[0], 0)) {
	    sum32 += tmp->area;
	}
    }

    if (choice->att[6]) {
	if (in_group(tmp->att, recl_tb[0], 0))
	    density++;
    }

    if (choice->att[8]) {
	area += tmp->area;
	area2 += tmp->area * tmp->area;
    }

    if (!tmp->next) {

	/* calc. a1 = mn. pixel attrib. */

	if (choice->att[1] && total1) {
	    value[index][0] = sum1 / total1;
	}

	/* calc. a2 = s.d. pixel attrib. */

	if (choice->att[2] && total1) {
	    mean = sum1 / total1;
	    stdv = sum12 / total1 - mean * mean;
	    if (stdv > 0)
		value[index][1] = sqrt(stdv);
	}

	/* calc. a3 = mn. patch attrib. */

	if (choice->att[3] && total_patches) {
	    value[index][2] = sum2 / total_patches;
	}

	/* calc. a4 = s.d. patch attrib. */

	if (choice->att[4] && total_patches) {
	    mean = sum2 / total_patches;
	    stdv = sum22 / total_patches - mean * mean;
	    if (stdv > 0)
		value[index][3] = sqrt(stdv);
	}

	/* calc. a5 = cover in gp 1 */

	if (choice->att[5] && total2) {
	    value[index][4] = sum32 / total2;
	}

	/* calc. a6 = density in gp 1 */

	if (choice->att[6]) {
	    value[index][5] = density;
	}

	/* calc. a7 = total density */

	if (choice->att[7])
	    value[index][6] = total_patches;


	/* calc. a8 = eff. mesh no. */

	if (choice->att[8]) {
	    value[index][36] = (area * area) / area2;
	}

	total1 = total2 = sum1 = sum12 = 0.0;
	sum2 = sum22 = sum32 = area = area2 = 0.0;
	density = 0;

    }
    return;
}


		     /************************************/
		     /*    MOVING WINDOW SIZE MEASURES   */

		     /************************************/

void m_size(PATCH * tmp, double **value, int index)
{
    static double sum1 = 0.0, sum12 = 0.0, sum2 = 0.0, sum22 = 0.0;
    static int density1 = 0, density2 = 0, density3 = 0;
    double mean, stdv;


    /* choice->size == 1 - mean patch size              (s1)
       2 - st. dev. patch size          (s2)
       3 - mean patch size by gp 1      (s3)
       4 - st. dev. patch size by gp 1  (s4)
       5 - no. by size class 1          (s5)
       6 - no. by size class 1 by gp 1  (s6)
       Variables:
       IN:
       tmp =      the list of patches
       value =    buffer containing a full row of the results of the moving
       window if a moving window map, otherwise 0
       index =    number of the column in the moving window
       INTERNAL:
       sum1 =     sum of patch sizes
       sum12 =    sum of patch sizes squared
       sum2 =          sum of patch sizes in gp 1
       sum22 =    sum of patch sizes in gp 1 squared
       density1 = no. of patches in gp 1
       density2 = no. of patches in size class 1
       density3 = no. of patches in gp 1 and size class 1
     */



    if (choice->size[1] || choice->size[2] ||
	choice->size[7] || choice->size[8]) {
	sum1 += tmp->area;
	if (choice->size[2] || choice->size[7] || choice->size[8])
	    sum12 += tmp->area * tmp->area;
    }

    if (choice->size[3] || choice->size[4]) {
	if (in_group(tmp->att, recl_tb[0], 0)) {
	    density1++;
	    sum2 += tmp->area;
	    if (choice->size[4])
		sum22 += tmp->area * tmp->area;
	}
    }

    if (choice->size[5] && tmp->area < size_cl[2]) {
	density2++;
    }

    if (choice->size[6] && tmp->area < size_cl[2] &&
	in_group(tmp->att, recl_tb[0], 0)) {
	density3++;
    }

    if (!tmp->next) {

	/* calc. s1 = mn. patch size */

	if (choice->size[1] && total_patches) {
	    value[index][7] = sum1 / total_patches;
	}


	/* calc. s2 = s.d. patch size */

	if (choice->size[2] && total_patches) {
	    mean = sum1 / total_patches;
	    stdv = sum12 / total_patches - mean * mean;
	    if (stdv > 0)
		value[index][8] = sqrt(stdv);
	}


	/* calc. s3 = mn. patch size by gp 1 */

	if (choice->size[3] && density1) {
	    value[index][9] = sum2 / density1;
	}


	/* calc. s4 = s.d. patch size by gp 1 */

	if (choice->size[4] && density1 > 1) {
	    mean = sum2 / density1;
	    stdv = sum22 / density1 - mean * mean;
	    if (stdv > 0)
		value[index][10] = sqrt(stdv);
	}


	/* calc. s5 = no. by size class 1 */

	if (choice->size[5]) {
	    value[index][11] = density2;
	}

	/* calc. s6 = no. by size class 1 by gp 1 */

	if (choice->size[6]) {
	    value[index][12] = density3;
	}

	/* calc. s7 = eff. mesh size */

	if (choice->size[7]) {
	    value[index][37] = (1.0 / sum1) * sum12;
	}

	/* calc. s8 = deg. landsc. division */

	if (choice->size[8]) {
	    value[index][38] = 1.0 - (sum12 / (sum1 * sum1));
	}

	density1 = density2 = density3 = 0;
	sum1 = sum12 = sum2 = sum22 = 0.0;
    }
    return;
}




		     /************************************/
		     /*   MOVING WINDOW CORE MEASURES    */

		     /************************************/

void m_core(PATCH * tmp, double **value, int index)
{
    static double sum1c = 0.0, sum1e = 0.0, sum2c = 0.0, sum2e = 0.0,
	sum12c = 0.0, sum12e = 0.0, sum22c = 0.0, sum22e = 0.0;
    static int density1c = 0, density1e = 0, density2c = 0, density2e = 0,
	density3c = 0, density3e = 0;
    double meanc, stdvc, meane, stdve;


    /* choice->core == 1 - mean core size           (c1)
       2 - st. dev. core size       (c2)
       3 - mean edge size           (c3)
       4 - st. dev. edge size       (c4)
       5 - mean core size by gp     (c5)
       6 - st. dev. core size by gp (c6)
       7 - mean edge size by gp     (c7)
       8 - st. dev. edge size by gp (c8)
       9 - no. by size class        (c9)
       10 - no. by size class by gp  (c10)
       variables:
       IN:
       tmp =       the list of patches
       value =     buffer containing a full row of the results of the moving
       window if a moving window map, otherwise 0
       index =     number of the column in the moving window
       INTERNAL:
       sum1c =          sum of core sizes
       sum12c =    sum of core sizes squared
       sum1e =          sum of edge sizes
       sum12e =    sum of edge sizes squared
       sum2c =     sum of core sizes in gp 1
       sum22c =    sum of core sizes in gp 1 squared
       sum2e =     sum of edge sizes in gp 1
       sum22e =    sum of edge sizes in gp 1 squared
       density1c = no. of cores in gp 1
       density1e = no. of edges in gp 1
       density2c = no. of cores in size class 1
       density2e = no. of edges in size class 1
       density3c = no. of cores in size class 1 and gp 1
       density3e = no. of edges in size class 1 and gp 1
       meanc =     mean core size
       stdvc =     standard deviation of mean core size
       meane =     mean edge size
       stdve =     standard deviation of mean edge size
     */



    if (choice->core[1] || choice->core[2]) {
	sum1c += tmp->core;
	if (choice->core[2])
	    sum12c += tmp->core * tmp->core;
    }

    if (choice->core[3] || choice->core[4]) {
	sum1e += (int)tmp->edge;
	if (choice->core[4])
	    sum12e += tmp->edge * tmp->edge;
    }

    if (choice->core[5] || choice->core[6] ||
	choice->core[7] || choice->core[8])
	if (in_group(tmp->att, recl_tb[0], 0)) {
	    if (choice->core[5] || choice->core[6]) {
		density1c++;
		sum2c += tmp->core;
		if (choice->core[6])
		    sum22c += tmp->core * tmp->core;
	    }
	    if (choice->core[7] || choice->core[8]) {
		density1e++;
		sum2e += tmp->edge;
		if (choice->core[8])
		    sum22e += tmp->edge * tmp->edge;
	    }
	}

    if (choice->core[9]) {
	if (tmp->core < size_cl[2])
	    density2c++;
	if (tmp->edge < size_cl[2])
	    density2e++;
    }

    if (choice->core[10]) {
	if (tmp->core < size_cl[2] && in_group(tmp->att, recl_tb[0], 0))
	    density3c++;
	if (tmp->edge < size_cl[2] && in_group(tmp->att, recl_tb[0], 0))
	    density3e++;
    }


    if (!tmp->next) {

	/* calc. c1 = mn. core size */

	if (choice->core[1] && total_patches) {
	    value[index][13] = sum1c / total_patches;
	}

	/* calc. c2 = s.d. core size */

	if (choice->core[2] && total_patches) {
	    meanc = sum1c / total_patches;
	    stdvc = sum12c / total_patches - meanc * meanc;
	    if (stdvc > 0)
		value[index][14] = sqrt(stdvc);
	}

	/* calc. c3 = mn. edge size */

	if (choice->core[3] && total_patches) {
	    value[index][15] = sum1e / total_patches;
	}

	/* calc. c4 = s.d. edge size */

	if (choice->core[4] && total_patches) {
	    meane = sum1e / total_patches;
	    stdve = sum12e / total_patches - meane * meane;
	    if (stdve > 0)
		value[index][16] = sqrt(stdve);
	}

	/* calc. c5 = mn. core size by gp 1 */

	if (choice->core[5] && density1c) {
	    value[index][17] = sum2c / density1c;
	}


	/* calc. c6 = s.d. core size by gp 1 */

	if (choice->core[6] && density1c > 1) {
	    meanc = sum2c / density1c;
	    stdvc = sum22c / density1c - meanc * meanc;
	    if (stdvc > 0)
		value[index][18] = sqrt(stdvc);
	}


	/* calc. c7 = mn. edge size by gp 1 */

	if (choice->core[7] && density1e) {
	    value[index][19] = sum2e / density1e;
	}

	/* calc. c8 = s.d. edge size by gp 1 */

	if (choice->core[8] && density1e > 1) {
	    meane = sum2e / density1e;
	    stdve = sum22e / density1e - meane * meane;
	    if (stdve > 0)
		value[index][20] = sqrt(stdve);
	}

	/* calc. c9 = no. by size class 1 */

	if (choice->core[9]) {
	    value[index][21] = density2c;
	}

	/* calc. c10 = no. by size class 1 by gp 1 */

	if (choice->core[10]) {
	    value[index][22] = density3c;
	}

	density1c = density1e = 0;
	density2c = density2e = 0;
	density3c = density3e = 0;
	sum1c = sum1e = sum2c = sum2e = 0.0;
	sum12c = sum12e = sum22c = sum22e = 0.0;
    }
    return;
}





		     /************************************/
		     /*  MOVING WINDOW SHAPE MEASURES    */

		     /************************************/

void m_shape(PATCH * tmp, int way, double **value, int index)
{
    static double sum1 = 0, sum12 = 0, sum2 = 0, sum22 = 0;
    static int density1 = 0, density2 = 0, density3 = 0;
    double mean, shp, stdv;

    /* choice->shape        1 = mean patch shape                 (h1)
       2 = st.dev. patch shape              (h2)
       3 = mean patch shape by gp 1         (h3)
       4 = st.dev. shape by gp 1            (h4)
       5 = number by shape class 1          (h5)
       6 = number by shape class 1 by gp 1  (h6)
       Variables:
       IN:
       tmp =      the list of patches
       way =      shape index choice (see shp)
       value =    buffer containing a full row of the results of the moving
       window if a moving window map, otherwise 0
       index =    number of the column in the moving window
       INTERNAL:
       shp =           P/A (way=1)
       CPA (way=2)
       RCC (way=3)
       sum1 =     sum of shape indices
       sum12 =    sum of shape indices squared
       sum2 =     sum of shape indices in gp 1
       sum22 =    sum of shape indices in gp 1 squared
       density1 = no. of patches in gp 1
       density2 = no. of patches in shape class 1
       density3 = no. of patches in shape class 1 and gp 1
     */



    if (way == 1) {
	if (tmp->area)
	    shp = tmp->perim / tmp->area;
	else
	    shp = 0.0;
    }

    else if (way == 2) {
	if (tmp->area)
	    shp = (0.282 * tmp->perim) / sqrt(tmp->area);
	else
	    shp = 0.0;
    }

    else {
	if (tmp->long_axis)
	    shp = 2 * sqrt(tmp->area / PI) / tmp->long_axis;
	else
	    shp = 0.0;
    }

    if (choice->shape[1] || choice->shape[2]) {
	sum1 += shp;
	if (choice->shape[2])
	    sum12 += shp * shp;
    }

    if (choice->shape[3] || choice->shape[4]) {
	if (in_group(tmp->att, recl_tb[0], 0)) {
	    density1++;
	    sum2 += shp;
	    if (choice->shape[4])
		sum22 += shp * shp;
	}
    }

    if (choice->shape[5] || choice->shape[6])
	if ((way == 1 && (shp < shape_PA[2] && shp >= shape_PA[1])) ||
	    (way == 2 && (shp < shape_CPA[2] && shp >= shape_CPA[1])) ||
	    (way == 3 && (shp < shape_RCC[2] && shp >= shape_RCC[1]))) {
	    if (choice->shape[5])
		density2++;
	    else if (in_group(tmp->att, recl_tb[0], 0))
		density3++;
	}

    if (!tmp->next) {

	/* calc. h1=mn. patch shape */

	if (choice->shape[1] && total_patches)
	    value[index][23] = sum1 / total_patches;

	/* calc. h2=s.d. patch shape */

	if (choice->shape[2] && total_patches > 1) {
	    mean = sum1 / total_patches;
	    stdv = sum12 / total_patches - mean * mean;
	    if (stdv > 0)
		value[index][24] = sqrt(stdv);
	}

	/* calc. h3=mn. patch shape in gp 1 */

	if (choice->shape[3] && density1)
	    value[index][25] = sum2 / density1;

	/* calc. h4=s.d. patch shape in gp 1 */

	if (choice->shape[4] && density1 > 1) {
	    mean = sum2 / density1;
	    stdv = sum22 / density1 - mean * mean;
	    if (stdv > 0)
		value[index][26] = sqrt(stdv);
	}

	/* calc. h5=no. in shape index class 1 */

	if (choice->shape[5])
	    value[index][27] = density2;

	/* calc. h6=no. in shape index class 1 & gp 1 */

	if (choice->shape[6])
	    value[index][28] = density3;

	density1 = density2 = density3 = 0;
	sum1 = sum12 = sum2 = sum22 = 0.0;
    }
    return;
}




		     /******************************************/
		     /*  COMPUTE AND SAVE BOUNDARY COMPLEXITY  */
		     /*  MEASURES FOR MOVING WINDOW ANALYSIS   */

		     /******************************************/

void m_boundary(PATCH * tmp, double **value, int index)
{

    static double sumomega = 0.0, sumomega2 = 0.0;
    static int sumtwist = 0, sumtwist2 = 0;
    double meantwist = 0.0, stdvtwist = 0.0, meanomega = 0.0, stdvomega = 0.0;

    /* Variables:
       IN:
       tmp =                the next patch in the patch list
       value =         buffer containing a full row of the results of the moving
       window if a moving window map, otherwise 0
       index =         number of the column in the moving window
       INTERNAL:
       meantwist =     mean twist number (n1)
       stdvtwist =     st. dev. twist number (n2)
       meanomega =     mean omega index (n3)
       stdvomega =     st. dev. omega index (n4)
       GLOBAL:
       total_patches = tot. no. patches in sampling area (fm. trace.c)
     */


    sumtwist += tmp->twist;
    sumtwist2 += tmp->twist * tmp->twist;
    sumomega += tmp->omega;
    sumomega2 += tmp->omega * tmp->omega;

    if (!tmp->next) {

	if (choice->boundary[1] || choice->boundary[2]) {
	    meantwist = (double)sumtwist / total_patches;
	    stdvtwist =
		(double)sumtwist2 / total_patches - meantwist * meantwist;
	    if (stdvtwist > 0.0)
		stdvtwist = sqrt(stdvtwist);
	    else
		stdvtwist = 0.0;
	}

	if (choice->boundary[3] || choice->boundary[4]) {
	    meanomega = (double)sumomega / total_patches;
	    stdvomega =
		(double)sumomega2 / total_patches - meanomega * meanomega;
	    if (stdvomega > 0.0)
		stdvomega = sqrt(stdvomega);
	    else
		stdvomega = 0.0;
	}

	/* write n1=mean twist number */

	if (choice->boundary[1]) {
	    value[index][29] = meantwist;
	}

	/* write n2=st. dev. twist number */

	if (choice->boundary[2]) {
	    value[index][39] = stdvtwist;
	}

	/* write n3=mean omega index */

	if (choice->boundary[3]) {
	    value[index][40] = meanomega;
	}

	/* write n4=st. dev. omega index */

	if (choice->boundary[4]) {
	    value[index][41] = stdvomega;
	}
	sumtwist = sumtwist2 = meantwist = stdvtwist = 0.0;
	sumomega = sumomega2 = meanomega = stdvomega = 0.0;
    }
    return;
}




		     /************************************/
		     /* MOVING WINDOW PERIMETER MEASURES */

		     /************************************/

void m_perim(PATCH * tmp, double **value, int index)
{
    static double sum1 = 0.0, sum12 = 0.0, sum2 = 0.0, sum22 = 0.0;
    static int density = 0;
    double mean, stdv;

    /* choice->perim == 1 - sum of perimeters             (p1)
       2 - mean perimeter                (p2)
       3 - st. dev. perimeters           (p3)
       4 - sum of perimeters in gp 1     (p4)
       5 - mean perimeter in gp 1        (p5)
       6 - st. dev. perimeters in gp 1   (p6)
       Variables:
       IN:
       tmp =     the list of patches
       value =   buffer containing a full row of the results of the moving
       window if a moving window map, otherwise 0
       index =   number of the column in the moving window
       INTERNAL:
       sum1 =         sum of patch perimeters
       sum12 =        sum of patch perimeters squared
       sum2 =    sum of patch perimeters in gp 1
       sum22 =   sum of patch perimeters in gp 1 squared
       density = no. of patches in gp 1
     */



    if (choice->perim[1] || choice->perim[2] || choice->perim[3]) {
	sum1 += tmp->perim;
	if (choice->perim[3])
	    sum12 += tmp->perim * tmp->perim;
    }

    if (choice->perim[4] || choice->perim[5] || choice->perim[6]) {
	if (in_group(tmp->att, recl_tb[0], 0)) {
	    sum2 += tmp->perim;
	    if (choice->perim[5] || choice->perim[6]) {
		density++;
		if (choice->perim[6])
		    sum22 += tmp->perim * tmp->perim;
	    }
	}
    }

    if (!tmp->next) {

	/* calc. p1 = sum of perims. */

	if (choice->perim[1])
	    value[index][30] = sum1;

	/* calc. p2 = mn. perim. */

	if (choice->perim[2] && total_patches)
	    value[index][31] = sum1 / total_patches;

	/* calc. p3 = s.d. perim. */

	if (choice->perim[3] && total_patches > 1) {
	    mean = sum1 / total_patches;
	    stdv = sum12 / total_patches - mean * mean;
	    if (stdv > 0)
		value[index][32] = sqrt(stdv);
	}

	/* calc. p4 = sum of perims. in gp 1 */

	if (choice->perim[4])
	    value[index][33] = sum2;

	/* calc. p5 = mn. perim. in gp 1 */

	if (choice->perim[5] && density)
	    value[index][34] = sum2 / density;

	/* calc. p6 = s.d. perims. in gp 1 */

	if (choice->perim[6] && density > 1) {
	    mean = sum2 / density;
	    stdv = sum22 / density - mean * mean;
	    if (stdv > 0)
		value[index][35] = sqrt(stdv);
	}

	density = 0;
	sum1 = sum12 = sum2 = sum22 = 0.0;
    }
    return;
}



double eu_d(double x1, double y1, double x2, double y2)
{
    return (sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)));
}
