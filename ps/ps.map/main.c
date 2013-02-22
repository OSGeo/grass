
/****************************************************************************
 *
 * MODULE:       ps.map
 * AUTHOR(S):    Paul W. Carlson 1992 (original contributor)
 *               Radim Blazek <radim.blazek gmail.com>
 *               Bob Covill <bcovill tekmap.ns.ca>
 *               Huidae Cho <grass4u gmail.com>
 *               Glynn Clements <glynn gclements.plus.com>
 *               Hamish Bowman <hamish_b yahoo.com>
 *               Markus Neteler <neteler itc.it>
 *               Alessandro Frigeri <afrigeri unipg.it>
 *               Martin Landa <landa.martin gmail.com>
 * PURPOSE:      Hardcopy PostScript map output utility (based on p.map program)
 * COPYRIGHT:    (C) 2003-2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <grass/gis.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "map_info.h"
#include "vector.h"
#include "labels.h"
#include "header.h"
#include "border.h"
#include "comment.h"
#include "colortable.h"
#include "local_proto.h"

struct border brd;
struct map_info m_info;
struct labels labels;
struct colortable ct;
struct PS_data PS;
int WHITE = 0;
int BLACK = 1;
int GREY = 9;
int sec_draw;
struct vector vector;
struct header hdr;
struct scalebar sb;
struct comment cmt;
struct PS_group grp;




FILE *tracefd;
FILE *inputfd;
int do_mapinfo;
int do_vlegend;
char *ps_mask_file;

int rotate_plot;
int eps_output;
int ps_copies = 1;

int main(int argc, char *argv[])
{
    char buf[1024];
    int can_reset_scale;
    int copies_set;
    struct Option *input_file;
    struct Option *output_file;
    struct Option *copies;
    struct Flag *rflag, *pflag, *eflag, *bflag;
    struct GModule *module;
    static char *def_font = "Helvetica";

    /**************** begin ******************************/

    signal(SIGINT, exit);
    signal(SIGTERM, exit);

    setbuf(stderr, NULL);

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("postscript"));
    G_add_keyword(_("printing"));
    module->description = _("Produces hardcopy PostScript map output.");

    rflag = G_define_flag();
    rflag->key = 'r';
    rflag->description = _("Rotate plot 90 degrees");
    rflag->guisection = _("Output settings");

    pflag = G_define_flag();
    pflag->key = 'p';
    pflag->description =
	_("List paper formats (name width height left right top bottom(margin))");
    pflag->suppress_required = YES;
    pflag->guisection = _("Utility");

    eflag = G_define_flag();
    eflag->key = 'e';
    eflag->description =
	_("Create EPS (Encapsulated PostScript) instead of PostScript file");
    eflag->guisection = _("Output settings");

    bflag = G_define_flag();
    bflag->key = 'b';
    bflag->description =
	_("Describe map-box's position on the page and exit (inches from top-left of paper)");
    bflag->suppress_required = YES;
    bflag->guisection = _("Utility");

    input_file = G_define_standard_option(G_OPT_F_INPUT);
    input_file->label = _("File containing mapping instructions");
    input_file->description = _("Use '-' to enter instructions from keyboard)");

    output_file = G_define_standard_option(G_OPT_F_OUTPUT);
    output_file->description = _("Name for PostScript output file");

    copies = G_define_option();
    copies->key = "copies";
    copies->type = TYPE_INTEGER;
    copies->options = "1-20";
    copies->description = _("Number of copies to print");
    copies->required = NO;
    copies->guisection = _("Output settings");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* PS.map_* variables are set to 0 (not defined) and then may be
     * reset by 'maploc'.  When script is read, main() should call
     * reset_map_location() to reset map size to fit to paper */

    G_zero(&PS, sizeof(struct PS_data));

    /* Print paper sizes to stdout */
    if (pflag->answer) {
	print_papers();
	exit(EXIT_SUCCESS);
    }

    rotate_plot = rflag->answer;
    eps_output = eflag->answer;
    /* set default paper */
    set_paper("a4");

    strcpy(buf, "black");
    BLACK = get_color_number(buf);
    strcpy(buf, "white");
    WHITE = get_color_number(buf);
    strcpy(buf, "grey");
    GREY = get_color_number(buf);

    /* initialize */
    vector_init();

    copies_set = 0;
    m_info.x = m_info.y = -1.0;
    vector.x = vector.y = -1.0;
    ct.x = ct.y = -1.0;
    ct.width = -1.0;
    cmt.color = BLACK;
    m_info.font = G_store(def_font);
    vector.font = G_store(def_font);
    hdr.font = G_store(def_font);
    cmt.font = G_store(def_font);
    ct.font = G_store(def_font);
    m_info.fontsize = 10;
    vector.fontsize = 10;
    hdr.fontsize = 10;
    cmt.fontsize = 10;
    ct.fontsize = 10;
    ct.cols = 1;
    tracefd = NULL;
    inputfd = NULL;
    labels.count = 0;
    labels.other = NULL;
    can_reset_scale = 1;
    hdr.fp = NULL;
    grp.do_group = 0;
    brd.r = brd.g = brd.b = 0.;
    brd.width = 1.;

    PS.min_y = 72.0 * (PS.page_height - PS.top_marg);
    PS.set_y = 100.0 * PS.min_y;
    PS.cell_fd = -1;
    PS.do_border = TRUE;

    /* arguments */
    if (input_file->answer) {
	if (strcmp(input_file->answer, "-")) {
	    inputfd = fopen(input_file->answer, "r");
	    if (!inputfd)
		G_fatal_error(_("Unable to open file '%s': %s"),
			      input_file->answer, strerror(errno));
	}
	else {
	    inputfd = stdin;
	}
    }
    else {
	G_fatal_error(_("Required parameter <%s> not set:\n\t(%s)"),
		      input_file->key, input_file->label);
    }

    if (copies->answer) {
	if (sscanf(copies->answer, "%d", &ps_copies) != 1) {
	    ps_copies = 1;
	    error(copies->answer, "", _("illegal copies request"));
	}
	copies_set = 1;
    }

    if (!bflag->answer) {
	if (output_file->answer) {
	    if ((PS.fp = fopen(output_file->answer, "w")) == NULL)
		G_fatal_error("Unable to create file '%s': %s",
			      output_file->answer, strerror(errno));
	}
	else {
	    G_fatal_error(_("Required parameter <%s> not set:\n\t(%s)"),
			  output_file->key, output_file->description);
	}
    }
    else
	PS.fp = NULL;

    /* get current mapset */
    PS.cell_mapset = G_mapset();

    /* get current window */
    Rast_get_window(&PS.w);

    read_instructions(copies_set, can_reset_scale);

    /* reset map location base on 'paper' on 'location' */
    reset_map_location();

    if (bflag->answer) {
	map_setup();
	fprintf(stdout, "bbox=%.3f,%.3f,%.3f,%.3f\n", PS.map_left / 72.0,
		PS.page_height - (PS.map_bot / 72.0), PS.map_right / 72.0,
		PS.page_height - (PS.map_top / 72.0));
		/* +/- 0.5 ? see ps.map.c brd.* */
	exit(EXIT_SUCCESS);
    }

    /* write the PostScript output file */
    ps_mask_file = G_tempfile();
    ps_map();

    /* cleanup the tempfiles */
    unlink(ps_mask_file);
    if (PS.plfile)
	unlink(PS.plfile);
    if (PS.commentfile)
	unlink(PS.commentfile);
    /*    if(sessionfile) unlink(sessionfile);    created in session.c (how to remove?) */
    if (labels.other)
	unlink(labels.other);

    G_done_msg(_("PostScript file '%s' successfully written."),
	       output_file->answer);

    exit(EXIT_SUCCESS);
}

