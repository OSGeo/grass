
/****************************************************************************
 *
 * MODULE:       s.in.ascii
 * AUTHOR(S):    Michael Shapiro - US Army CERL
 *               Improvements:
 *                     Markus Neteler - neteler@geog.uni-hannover.de
 *                     Eric Miller
 *                     added timestamp 1/2002 MN
 * PURPOSE:      Import ASCII sites lists and their descriptions into
 *               a GRASS sites list file. 
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/site.h>
#include <grass/glocale.h>
#include "local_proto.h"

static int loop;		/* added #cat support for site_list 11/99 M. Neteler
				 * required for s.to.vect and s.to.rast */

int main(int argc, char *argv[])
{
    char *me;
    char *output, *input;
    char *fs;
    int dims, i, has_cat;
    struct GModule *module;
    FILE *in_fd, *out_fd;
    Site *site;
    Site_head shead;
    struct TimeStamp ts;
    struct
    {
	struct Option *input, *output, *dims, *fs, *date;
    } parm;

    G_gisinit(me = argv[0]);

    module = G_define_module();
    module->keywords = _("sites");
    module->description =
	"Convert an ASCII listing of site locations "
	"into a GRASS site list file.";

    parm.output = G_define_option();
    parm.output->key = "output";
    parm.output->type = TYPE_STRING;
    parm.output->required = YES;
    parm.output->description = "vector map to be created";
    parm.output->gisprompt = "any,vector,vector";

    parm.input = G_define_option();
    parm.input->key = "input";
    parm.input->type = TYPE_STRING;
    parm.input->required = NO;
    parm.input->description = "unix file containing sites";

    parm.dims = G_define_option();
    parm.dims->key = "d";
    parm.dims->type = TYPE_INTEGER;
    parm.dims->required = NO;
    parm.dims->description = "number of dimensions (default=2)";

    parm.fs = G_define_option();
    parm.fs->key = "fs";
    parm.fs->key_desc = "character|space|tab";
    parm.fs->type = TYPE_STRING;
    parm.fs->required = NO;
    parm.fs->description = "input field separator";
    parm.fs->answer = "space";

    parm.date = G_define_option();
    parm.date->key = "date";
    parm.date->key_desc = "timestamp";
    parm.date->required = NO;
    parm.date->type = TYPE_STRING;
    parm.date->description = "datetime or datetime1/datetime2";

    if (G_parser(argc, argv))
	exit(-1);

    if ((input = parm.input->answer)) {
	in_fd = fopen(input, "r");
	if (NULL == in_fd) {
	    fprintf(stderr, "%s - ", me);
	    perror(input);
	    exit(1);
	}
    }
    else
	in_fd = stdin;

    output = parm.output->answer;
    shead.name = G_store(parm.output->answer);
    shead.desc = G_store(G_recreate_command());
    shead.form = shead.labels = shead.stime = (char *)NULL;

    /* add here time parameter */
    if (parm.date->answer) {
	if (1 == G_scan_timestamp(&ts, parm.date->answer))
	    shead.time = &ts;
	else
	    G_fatal_error("Invalid timestamp");
    }
    else
	shead.time = (struct TimeStamp *)NULL;

    dims = 2;
    loop = 1;			/* added 11/99 MNeteler */

    if (parm.dims->answer != NULL)
	if ((i = sscanf(parm.dims->answer, "%d", &dims)) != 1)
	    G_fatal_error("error scanning number of dimensions");
    if (dims < 2)
	G_fatal_error("number of dimensions must be greater than 1");

    if (strlen(parm.fs->answer) < 1)
	G_fatal_error("field separator cannot be empty");
    else {
	fs = parm.fs->answer;
	if (strcmp(fs, "space") == 0)
	    fs = NULL;
	else if (strcmp(fs, "tab") == 0)
	    fs = NULL;
    }

    out_fd = G_fopen_sites_new(output);
    if (out_fd == NULL)
	G_fatal_error("can't create sites file [%s].", output);

    G_site_put_head(out_fd, &shead);

    while ((site = get_site(in_fd, dims, fs, &has_cat)))
	G_site_put(out_fd, site);

    G_sites_close(out_fd);
    exit(0);
}
static int format_double(double value, char *buf)
{
    int G_trim_decimal();

    sprintf(buf, "%.8f", value);
    G_trim_decimal(buf);
    return 0;
}

#define DQUOTE '"'
#define SPACE ' '
#define BSLASH 92
#define PIPE '|'

#define ispipe(c) (c==PIPE)
#define isnull(c) (c==(char)NULL)
#define isquote(c) (c==DQUOTE)
#define isbslash(c) (c==BSLASH)

int G_site_put_new(FILE * fptr, Site * s, int has_cat)

/* Writes a site to file open on fptr. */
{
    char ebuf[MAX_SITE_STRING], nbuf[MAX_SITE_STRING];
    char xbuf[MAX_SITE_STRING], buf[MAX_SITE_LEN];
    static int format_double();
    int fmt, i, j, k;
    int G_format_northing(), G_format_easting(), G_projection();

    fmt = G_projection();

    G_format_northing(s->north, nbuf, fmt);
    G_format_easting(s->east, ebuf, fmt);
    sprintf(buf, "%s|%s|", ebuf, nbuf);
    for (i = 0; i < s->dim_alloc; ++i) {
	format_double(s->dim[i], nbuf);
	sprintf(xbuf, "%s|", nbuf);
	G_strcat(buf, xbuf);
    }

    if (has_cat) {
	switch (s->cattype) {
	case CELL_TYPE:
	    sprintf(xbuf, "#%d ", s->ccat);
	    G_strcat(buf, xbuf);
	    break;
	case FCELL_TYPE:
	    sprintf(xbuf, "#%g ", s->fcat);
	    G_strcat(buf, xbuf);
	    break;
	case DCELL_TYPE:
	    sprintf(xbuf, "#%g ", s->dcat);
	    G_strcat(buf, xbuf);
	    break;
	}
    }
    else {			/* no cat there, so data in plain x,y,z format will be imported   12/99 MN */

	/* we create a #cat entry in site_list from the current site number 11/99 */
	sprintf(xbuf, "#%d ", loop);
	loop++;
	G_strcat(buf, xbuf);
    }

    /* now import attributes */
    for (i = 0; i < s->dbl_alloc; ++i) {
	format_double(s->dbl_att[i], nbuf);
	sprintf(xbuf, "%%%s ", nbuf);
	G_strcat(buf, xbuf);
    }

    for (i = 0; i < s->str_alloc; ++i) {
	if (strlen(s->str_att[i]) != 0) {
	    /* escape double quotes */
	    j = k = 0;
	    if (G_index(s->str_att[i], DQUOTE) != (char *)NULL) {
		while (!isnull(s->str_att[i][j])) {
		    if (isquote(s->str_att[i][j])) {
			xbuf[k++] = BSLASH;
			xbuf[k++] = DQUOTE;
		    }
		    else if (isbslash(s->str_att[i][j])) {
			xbuf[k++] = BSLASH;
			xbuf[k++] = BSLASH;
		    }
		    else
			xbuf[k++] = s->str_att[i][j];
		    j++;
		}
		xbuf[k] = (char)NULL;
	    }
	    else
		G_strcpy(xbuf, s->str_att[i]);

	    G_strcpy(s->str_att[i], xbuf);

	    if (G_index(s->str_att[i], SPACE) != (char *)NULL)
		sprintf(xbuf, "@\"%s\" ", s->str_att[i]);
	    else
		sprintf(xbuf, "@%s ", s->str_att[i]);

	    G_strcat(buf, xbuf);
	}
    }
    fprintf(fptr, "%s\n", buf);
    return 0;
}
