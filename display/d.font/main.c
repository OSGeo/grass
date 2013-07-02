
/****************************************************************************
 *
 * MODULE:       d.font
 * AUTHOR(S):    James Westervelt (CERL) (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      user selection of font for graphics monitor text
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/glocale.h>

static char **fonts;
static int max_fonts;
static int num_fonts;

static void read_freetype_fonts(int verbose);
static void print_font_list(FILE * fp, int verbose);

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *opt1, *opt2, *opt3;
    struct Flag *flag1, *flag2;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("settings"));
    module->description =
	_("Selects the font in which text will be displayed "
	  "on the user's graphics monitor.");

    opt1 = G_define_option();
    opt1->key = "font";
    opt1->type = TYPE_STRING;
    opt1->required = NO;
    opt1->answer = "romans";
    opt1->description = _("Choose new current font");

    opt2 = G_define_standard_option(G_OPT_F_INPUT);
    opt2->key = "path";
    opt2->required = NO;
    opt2->description =
	_("Path to Freetype-compatible font including file name");
    opt2->gisprompt = "old,font,file";

    opt3 = G_define_option();
    opt3->key = "charset";
    opt3->type = TYPE_STRING;
    opt3->required = NO;
    opt3->answer = "UTF-8";
    opt3->description = _("Character encoding");

    flag1 = G_define_flag();
    flag1->key = 'l';
    flag1->description = _("List fonts");

    flag2 = G_define_flag();
    flag2->key = 'L';
    flag2->description = _("List fonts verbosely");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* load the font */
    if (D_open_driver() != 0)
	G_fatal_error(_("No graphics device selected. "
			"Use d.mon to select graphics device."));

    if (flag1->answer) {	/* List font names */
	print_font_list(stdout, 0);
	D_close_driver();
	exit(EXIT_SUCCESS);
    }

    if (flag2->answer) {	/* List fonts verbosely */
	print_font_list(stdout, 1);
	D_close_driver();
	exit(EXIT_SUCCESS);
    }

    if (opt2->answer) {		/* Full path to freetype font */
	struct stat info;

	/* Check a valid filename has been supplied */
	if (stat(opt2->answer, &info) != 0)
	    G_fatal_error(_("Unable to access font path %s: %s"),
			  opt2->answer, strerror(errno));

	if (!S_ISREG(info.st_mode))
	    G_fatal_error(_("Font path %s is not a file"), opt2->answer);
	else
	    D_font(opt2->answer);
    }
    else if (opt1->answer) {	/* Font name from fontcap */
	int i = 0;

	/* Check the fontname given is valid */
	read_freetype_fonts(0);
	while (i < num_fonts) {
	    if (strcmp(opt1->answer, fonts[i]) == 0) {
		D_font(opt1->answer);
		break;
	    }
	    i++;
	}
	if (i >= num_fonts)
	    G_fatal_error(_("Font name <%s> is invalid. Check font name or consider running 'g.mkfontcap'"),
			  opt1->answer);
    }

    if (opt3->answer)		/* Set character encoding */
	D_encoding(opt3->answer);

    /* add this command to the list */
    D_close_driver();

    exit(EXIT_SUCCESS);
}

static void read_freetype_fonts(int verbose)
{
    char **list;
    int count;
    int i;

    if (verbose)
	D_font_info(&list, &count);
    else
	D_font_list(&list, &count);

    if (max_fonts < num_fonts + count) {
	max_fonts = num_fonts + count;
	fonts = G_realloc(fonts, max_fonts * sizeof(char *));
    }

    for (i = 0; i < count; i++)
	fonts[num_fonts++] = list[i];
}

static void print_font_list(FILE * fp, int verbose)
{
    int i;

    /* find out what fonts we have */
    read_freetype_fonts(verbose);

    for (i = 0; i < num_fonts; i++)
	fprintf(fp, "%s\n", fonts[i]);
}
