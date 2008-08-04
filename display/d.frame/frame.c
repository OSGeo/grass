
/****************************************************************************
 *
 * MODULE:       d.frame
 * AUTHOR(S):    James Westervelt, U.S. Army CERL (original contributor)
 *               Michael Shapiro, U.S. Army CERL (original contributor)
 *               Markus Neteler <neteler itc.it>
 *               Bernhard Reiter <bernhard intevation.de>, Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, Hamish Bowman <hamish_nospam yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/*
 *   d.frame [-cps] [frame=name] [at=bottom,top,left,right]
 *
 *   at=...       create frame here (implies -c)
 *       top, bottom, left, and right are % coordinates of window;
 *       0,0 is lower left; 100,100 is upper right
 */

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/glocale.h>

int check_at(char *);
int list_all(void);

int main(int argc, char *argv[])
{
    char buf[1024];
    int create, select, print, debug, list;
    struct GModule *module;
    struct
    {
	struct Option *frame, *at;
    } parm;
    struct
    {
	struct Flag *debug;
	struct Flag *list;
	struct Flag *select;
	struct Flag *print;
	struct Flag *printall;
	struct Flag *create;
	struct Flag *erase;
    } flag;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("display");
    module->description =
	_("Manages display frames on the user's graphics monitor.");

    flag.create = G_define_flag();
    flag.create->key = 'c';
    flag.create->description = _("Create a new frame");

    flag.select = G_define_flag();
    flag.select->key = 's';
    flag.select->description = _("Select a frame");

    flag.erase = G_define_flag();
    flag.erase->key = 'e';
    flag.erase->description = _("Remove all frames and erase the screen");

    flag.print = G_define_flag();
    flag.print->key = 'p';
    flag.print->description = _("Print name of current frame");

    flag.printall = G_define_flag();
    flag.printall->key = 'a';
    flag.printall->description = _("Print names of all frames");

    flag.list = G_define_flag();
    flag.list->key = 'l';
    flag.list->description = _("List map names displayed in GRASS monitor");

    flag.debug = G_define_flag();
    flag.debug->key = 'D';
    flag.debug->description = _("Debugging output");

    parm.frame = G_define_option();
    parm.frame->key = "frame";
    parm.frame->type = TYPE_STRING;
    parm.frame->key_desc = _("name");
    parm.frame->required = NO;
    parm.frame->multiple = NO;
    parm.frame->description = _("Frame to be created/selected");

    parm.at = G_define_option();
    parm.at->key = "at";
    parm.at->key_desc = _("bottom,top,left,right");
    parm.at->type = TYPE_DOUBLE;
    parm.at->required = NO;
    parm.at->multiple = NO;
    parm.at->description =
	_("Where to place the frame, values in percent (implies -c)");
    parm.at->checker = check_at;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    create = flag.create->answer;
    print = flag.print->answer;
    select = flag.select->answer;
    debug = flag.debug->answer;
    list = flag.list->answer;

    /* if frame name is given without a control option, treat it as select */
    if (parm.frame->answer && (!create && !print && !select && !list))
	select = TRUE;

    /* at= placement implies creation */
    if (parm.at->answer)
	create = TRUE;

    if (flag.erase->answer) {
	if (R_open_driver() != 0)
	    G_fatal_error(_("No graphics device selected"));

	if (!create)
	    D_full_screen();
	else {
	    D_remove_windows();
	    R_standard_color(D_translate_color(DEFAULT_BG_COLOR));
	    R_erase();
	}

	R_close_driver();
    }

    if (create) {
	select = FALSE;
	sprintf(buf, "%s/etc/frame.create", G_gisbase());
	if (parm.frame->answer) {
	    strcat(buf, " frame='");
	    strcat(buf, parm.frame->answer);
	    strcat(buf, "'");
	}
	if (parm.at->answer) {
	    strcat(buf, " at='");
	    strcat(buf, parm.at->answer);
	    strcat(buf, "'");
	}
	if (system(buf))
	    exit(EXIT_FAILURE);
    }
    if (select) {
	sprintf(buf, "%s/etc/frame.select", G_gisbase());
	if (parm.frame->answer) {
	    strcat(buf, " frame='");
	    strcat(buf, parm.frame->answer);
	    strcat(buf, "'");
	}
	if (system(buf))
	    exit(EXIT_FAILURE);
    }

    if (debug) {
	sprintf(buf, "%s/etc/frame.dumper", G_gisbase());
	if (system(buf))
	    exit(EXIT_FAILURE);
    }

    if (list) {
	sprintf(buf, "%s/etc/frame.list", G_gisbase());
	if (system(buf))
	    exit(EXIT_FAILURE);
    }

    if (print) {
	if (R_open_driver() != 0)
	    G_fatal_error(_("No graphics device selected"));
	D_get_cur_wind(buf);
	D_set_cur_wind(buf);
	R_close_driver();
	fprintf(stdout, "%s\n", buf);
    }

    if (flag.printall->answer)
	list_all();


    exit(EXIT_SUCCESS);
}


int check_at(char *s)
{
    float top, bottom, left, right;


    if (s == NULL)
	return 0;

    if (4 != sscanf(s, "%f,%f,%f,%f", &bottom, &top, &left, &right)
	|| bottom < 0.0 || top > 100.0 || bottom >= top
	|| left < 0.0 || right > 100.0 || left >= right) {
	fprintf(stderr, "<at=%s> invalid request\n", s);
	return 1;
    }
    return 0;
}


int list_all(void)
{
    char **pads;
    int npads;
    int p;

    if (R_open_driver() != 0)
	G_fatal_error(_("No graphics device selected"));

    R_pad_list(&pads, &npads);

    for (p = npads - 1; p >= 0; p--)
	fprintf(stdout, "%s\n", pads[p]);

    R_close_driver();

    return 0;
}
