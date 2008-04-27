/****************************************************************************
 *
 * MODULE:       r.out.mpeg
 * AUTHOR(S):    Bill Brown, CERL (original contributor)
 *               Brad Douglas <rez touchofmadness.com>, Markus Neteler <neteler itc.it>,
 *               Glynn Clements <glynn gclements.plus.com>, Hamish Bowman <hamish_nospam yahoo.com>,
 *               Jan-Oliver Wagner <jan intevation.de>, Paul Kelly <paul-grass stjohnspoint.co.uk>
 * PURPOSE:      combines a series of GRASS raster maps into a single MPEG-1
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

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

#define MAXIMAGES 400
#define DEF_MAX 500
#define DEF_MIN 200
#define MAXVIEWS    4 
#define BORDER_W    2


/* global variables */
int     nrows, ncols, numviews, quality, quiet=0;
char 	*vfiles[MAXVIEWS][MAXIMAGES];
char 	outfile[BUFSIZ];
char    encoder[15];

float 	vscale, scale;  /* resampling scale factors */
int 	irows, icols, vrows, vcols;
int 	frames;


/* function prototypes */
static int load_files(void);
static int use_r_out(void);
static char **gee_wildfiles (char *wildarg, char *element, int *num);
static void parse_command (int argc, char *argv[],
                    char *vfiles[MAXVIEWS][MAXIMAGES], int *numviews,
                    int *numframes, int *quality, int *convert);


int main (int argc, char **argv)
{
/*    int	     	i, j, d; */
    int       	*sdimp, longdim, r_out;

    G_gisinit (argv[0]);
    parse_command(argc, argv, vfiles, &numviews, &frames, &quality, &r_out);

    /* find a working encoder */
    if (256 == G_system("ppmtompeg 2> /dev/null"))
	strcpy(encoder, "ppmtompeg");
    else if (256 == G_system("mpeg_encode 2> /dev/null"))
	strcpy(encoder, "mpeg_encode");
    else
        G_fatal_error(_("Either mpeg_encode or ppmtompeg must be installed"));

    G_debug(1, "encoder = [%s]", encoder);

    vrows = G_window_rows();
    vcols = G_window_cols();
    nrows = vrows;
    ncols = vcols;

    /* short dimension */
    sdimp = nrows>ncols ? &ncols : &nrows;

    /* these proportions should work fine for 1 or 4 views, but for
    2 views, want to double the narrow dim & for 3 views triple it */
    if (numviews == 2)
	*sdimp *= 2;
    else if (numviews == 3)
	*sdimp *= 3;

    longdim = nrows > ncols ? nrows : ncols;

    scale = 1.0;

    { /* find animation image size */
        int max, min;
        char *p;

        max = DEF_MAX;
        min = DEF_MIN;

        if ((p = getenv ("GMPEG_SIZE")))
            max = min = atoi(p);

        if (longdim > max)      /* scale down */
            scale = (float)max / longdim;
        else if (longdim < min) /* scale up */
            scale = (float)min / longdim;
    }
    /* TODO: align image size to 16 pixel width & height */
    
    vscale = scale;
    if (numviews == 4)
	vscale = scale / 2.;

    nrows *= scale;
    ncols *= scale;
    /* now nrows & ncols are the size of the combined - views image */
    vrows *= vscale;
    vcols *= vscale;
    /* now vrows & vcols are the size for each sub-image */

    /* add to nrows & ncols for borders */
    /* irows, icols used for vert/horizontal determination in loop below */
    irows = nrows;
    icols = ncols;
    nrows += (1 + (nrows/vrows)) * BORDER_W;
    ncols += (1 + (ncols/vcols)) * BORDER_W;

    if (numviews == 1 && r_out)
	use_r_out();
    else
	load_files();

    return (EXIT_SUCCESS);
}


static int load_files(void)
{
    void *voidc;
    int rtype;
    register int i, rowoff, row, col, vxoff, vyoff, offset;
    int cnt, ret, fd, size, tsiz, coff;
    int	vnum;
    int	y_rows, y_cols;
    char *pr, *pg, *pb;
    unsigned char *tr, *tg, *tb, *tset;
    char *mpfilename, *mapset, name[BUFSIZ];
    char cmd[1000], *yfiles[MAXIMAGES];
    struct Colors colors;

    size = nrows * ncols;

    pr = G_malloc (size);
    pg = G_malloc (size);
    pb = G_malloc (size);

    tsiz = G_window_cols();

    tr = (unsigned char *) G_malloc (tsiz);
    tg = (unsigned char *) G_malloc (tsiz);
    tb = (unsigned char *) G_malloc (tsiz);
    tset = (unsigned char *) G_malloc (tsiz);

    for (cnt = 0; cnt < frames; cnt++)
    {
        if (cnt > MAXIMAGES)
	{
	    cnt--;
	    break;
	}

	for (i=0; i< size; i++)
	    pr[i] = pg[i] = pb[i] = 0;

	for (vnum = 0; vnum < numviews; vnum++)
        {
	    if (icols == vcols)
            {
		vxoff =  BORDER_W;
		vyoff = (irows == vrows) ? BORDER_W : 
			    BORDER_W + vnum*(BORDER_W+vrows);
	    }
	    else if (irows == vrows)
            {
		vxoff = (icols == vcols) ? BORDER_W : 
			    BORDER_W + vnum*(BORDER_W+vcols);
		vyoff =  BORDER_W;
	    }
	    else
            { /* 4 views */
		/* assumes we want:
		    view1	view2
		    view3	view4   
		*/
		vxoff = vnum % 2 ? BORDER_W : vcols + 2 * BORDER_W;
		vyoff = vnum > 1 ? vrows + 2 * BORDER_W : BORDER_W; 
	    }

	    strcpy(name, vfiles[vnum][cnt]);
	    if (!quiet)
		G_message("\r%s <%s>", _("Reading file"), name);

	    mapset = G_find_cell2 (name, "");
	    if (mapset == NULL)
		G_fatal_error (_("Raster map <%s> not found"), name);

	    fd = G_open_cell_old (name, mapset);
	    if (fd < 0)
		exit(EXIT_FAILURE);

	    ret = G_read_colors(name, mapset, &colors);
	    if (ret < 0)
		exit(EXIT_FAILURE);

            rtype = G_get_raster_map_type(fd);
            if (rtype == CELL_TYPE)
                voidc = G_allocate_c_raster_buf();
            else if (rtype == FCELL_TYPE)
                voidc = G_allocate_f_raster_buf();
            else if (rtype == DCELL_TYPE)
                voidc = G_allocate_d_raster_buf();
            else
                exit(EXIT_FAILURE);

	    for (row = 0; row < vrows; row++)
            {
		if (G_get_raster_row (fd, voidc, 
                                      (int)(row/vscale), rtype) < 0)
		    exit(EXIT_FAILURE);

		rowoff = (vyoff+row)*ncols;
                G_lookup_raster_colors(voidc, tr, tg, tb,
				       tset, tsiz, &colors, rtype);

                for (col = 0; col < vcols; col++)
                {
                    coff   = (int)(col / vscale);
		    offset = rowoff + col + vxoff;

                    if (!tset[coff])
                        pr[offset] = pg[offset] = pb[offset] = (char) 255;
		    else
                    {
			pr[offset] = (char) tr[coff];	
			pg[offset] = (char) tg[coff];	
			pb[offset] = (char) tb[coff];	
		    }	
                }
	    }

	    G_close_cell(fd);
	}

	yfiles[cnt] = G_tempfile();

#ifdef USE_PPM
	write_ppm(pr, pg, pb, nrows, ncols, &y_rows, &y_cols, yfiles[cnt]);
#else
	write_ycc(pr, pg, pb, nrows, ncols, &y_rows, &y_cols, yfiles[cnt]);
#endif
    }

    mpfilename = G_tempfile();
    write_params(mpfilename, yfiles, outfile, cnt, quality, y_rows, y_cols, 0);

    if (quiet)
	sprintf(cmd, "%s %s 2> /dev/null > /dev/null", encoder, mpfilename);
    else
	sprintf(cmd, "%s %s", encoder, mpfilename);

    if (0 != G_system(cmd))
	G_warning(_("mpeg_encode ERROR"));

    clean_files(mpfilename, yfiles, cnt);

    G_free (voidc);
    G_free (tset);
    G_free (tr);
    G_free (tg);
    G_free (tb);
    G_free (pr);
    G_free (pg);
    G_free (pb);

    return(cnt);
}


static int use_r_out(void)
{
    char *mpfilename, cmd[1000];

    mpfilename = G_tempfile();
    write_params(mpfilename, vfiles[0], outfile, frames, quality, 0, 0, 1);

    if (quiet)
	sprintf(cmd, "%s %s 2> /dev/null > /dev/null", encoder, mpfilename);
    else
	sprintf(cmd, "%s %s", encoder, mpfilename);

    if (0 != G_system(cmd))
	G_warning(_("mpeg_encode ERROR"));

    clean_files(mpfilename, NULL, 0);

    return (1);
}


/* ###################################################### */
static char **gee_wildfiles (char *wildarg, char *element, int *num)
{
    int n, cnt=0;
    char path[1000], *mapset, cmd[1000], buf[512];
    char *p, *tfile;
    static char *newfiles[MAXIMAGES];
    FILE *tf;

    *num = 0;
    tfile = G_tempfile();

    /* build list of filenames */
    for (n=0; (mapset = G__mapset_name (n)); n++)
    {
	if (strcmp (mapset,".") == 0)
	    mapset = G_mapset();

	G__file_name (path, element, "", mapset);
	if (access(path, 0) == 0)
        {
	    sprintf(cmd, "cd %s; \\ls %s >> %s 2> /dev/null", 
                    path, wildarg, tfile);
	    G_system(cmd);
	}
    }

    if (NULL == (tf = fopen(tfile, "r")))
        G_warning(_("Error reading wildcard"));
    else
    {
	while (NULL != fgets(buf,512,tf))
        {
	    /* replace newline with null */
	    if ((p = strchr(buf, '\n')))
		*p = '\0';
	    /* replace first space with null */
	    else if((p = strchr(buf, ' ')))
		*p = '\0';

	    if (strlen(buf) > 1)
		newfiles[cnt++] = G_store (buf);
	}

	fclose(tf);
    }

    *num = cnt;
    sprintf(cmd, "\\rm %s", tfile);
    G_system(cmd);
    G_free (tfile);

    return(newfiles);
}


/********************************************************************/
static void parse_command (int argc, char *argv[],
                    char *vfiles[MAXVIEWS][MAXIMAGES], int *numviews,
                    int *numframes, int *quality, int *convert)
{
    struct GModule *module;
    struct Option *viewopts[MAXVIEWS], *out, *qual; 
    struct Flag *qt, *conv;
    char buf[BUFSIZ], **wildfiles;
    int i,j,k, numi, wildnum;

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
		_("Raster File Series to MPEG Conversion Program.");

    *numviews = *numframes = 0;
    for (i=0; i<MAXVIEWS; i++)
    {
	viewopts[i] = G_define_option();
	sprintf(buf, "view%d", i+1);
	viewopts[i]->key		= G_store(buf);
	viewopts[i]->type 		= TYPE_STRING;
	viewopts[i]->required 		= (i ? NO : YES);
	viewopts[i]->multiple 		= YES;
	viewopts[i]->gisprompt 		= "old,cell,Raster";
	sprintf(buf, _("Raster file(s) for View%d"), i+1);
	viewopts[i]->description 	= G_store(buf);
    }

    out = G_define_option();
    out->key		= "output";
    out->type 		= TYPE_STRING;
    out->required 	= NO;
    out->multiple 	= NO;
    out->answer 	= "gmovie.mpg";
    out->description 	= _("Name for output file");

    qual = G_define_option();
    qual->key		= "qual";
    qual->type 		= TYPE_INTEGER;
    qual->required 	= NO;
    qual->multiple 	= NO;
    qual->answer 	= "3";
    qual->options       = "1-5" ;
    qual->description 	= 
	    _("Quality factor (1 = highest quality, lowest compression)");

    qt = G_define_flag ();
    qt->key = 'q';
    qt->description = _("Quiet - suppress progress report");
   
    conv = G_define_flag ();
    conv->key = 'c';
    conv->description = _("Convert on the fly, use less disk space\n\t(requires r.out.ppm with stdout option)");
   
    if (G_parser (argc, argv))
        exit (EXIT_FAILURE);

    *convert = 0; 
    if (qt->answer)
        quiet = 1;
    if (conv->answer)
        *convert = 1;

    *quality = 3; 
    if (qual->answer != NULL)
	sscanf(qual->answer,"%d", quality);
    if (*quality > 5 || *quality < 1)
	*quality = 3; 

    if (out->answer)
	strcpy(outfile, out->answer);
    else
	strcpy(outfile, "gmovie.mpg");

    for (i=0; i<MAXVIEWS; i++)
    {
	if (viewopts[i]->answers)
        {
	    (*numviews)++;

	    for (j = 0, numi=0; viewopts[i]->answers[j] ; j++)
            {
		if ((NULL != strchr(viewopts[i]->answers[j], '*')) || 
		    (NULL != strchr(viewopts[i]->answers[j], '?')) || 
		    (NULL != strchr(viewopts[i]->answers[j], '[')))
                {
		    wildfiles = gee_wildfiles(viewopts[i]->answers[j],
				"cell", &wildnum);

		    for (k=0; k<wildnum; k++)
			vfiles[i][numi++] = wildfiles[k];
		}
		else
		    vfiles[i][numi++] = G_store(viewopts[i]->answers[j]);
	    }

	    /* keep track of smallest number of frames */
	    *numframes = *numframes ? *numframes > numi ? numi : *numframes : numi;
	}
    }
}

/*********************************************************************/
/*********************************************************************/
