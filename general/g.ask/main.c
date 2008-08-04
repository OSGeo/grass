
/****************************************************************************
 *
 * MODULE:       g.ask
 * AUTHOR(S):    Michael Shapiro CERL (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    char file[1024], name[40], *mapset;
    char *prompt;
    char *help[2];
    FILE *fd;
    struct GModule *module;
    struct Option *opt1;
    struct Option *opt2;
    struct Option *opt3;
    struct Option *opt4;
    struct Option *opt5;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("general");
    module->description =
	"Prompts the user for the names of GRASS data base files.";

    /* Define the different options */

    opt1 = G_define_option();
    opt1->key = "type";
    opt1->type = TYPE_STRING;
    opt1->required = YES;
    opt1->description = "The type of query";
    opt1->options = "old,new,any,mapset";

    opt2 = G_define_option();
    opt2->key = "prompt";
    opt2->key_desc = "\"string\"";
    opt2->type = TYPE_STRING;
    opt2->required = NO;
    opt2->description = "The prompt to be displayed to the user";

    opt3 = G_define_option();
    opt3->key = "element";
    opt3->type = TYPE_STRING;
    opt3->required = YES;
    opt3->description = "The database element to be queried";

    opt4 = G_define_option();
    opt4->key = "desc";
    opt4->key_desc = "\"string\"";
    opt4->type = TYPE_STRING;
    opt4->required = NO;
    opt4->description = "A short description of the database element";

    opt5 = G_define_option();
    opt5->key = "unixfile";
    opt5->type = TYPE_STRING;
    opt5->required = YES;
    opt5->description =
	"The name of a unix file to store the user's response";

    if (argc < 2) {		/* no interactive version allowed */
	help[0] = argv[0];
	help[1] = "help";
	argv = help;
	argc = 2;
    }
    if (G_parser(argc, argv) < 0)
	exit(EXIT_FAILURE);

    prompt = opt2->answer;

    fd = fopen(opt5->answer, "w");
    if (fd == NULL) {
	fprintf(stderr, "%s - ", argv[0]);
	perror(opt5->answer);
	exit(EXIT_FAILURE);
    }

    if (strcmp(opt1->answer, "old") == 0)
	mapset = G_ask_old(prompt, name, opt3->answer, opt4->answer);
    else if (strcmp(opt1->answer, "new") == 0)
	mapset = G_ask_new(prompt, name, opt3->answer, opt4->answer);
    else if (strcmp(opt1->answer, "any") == 0)
	mapset = G_ask_any(prompt, name, opt3->answer, opt4->answer, 0);
    else if (strcmp(opt1->answer, "mapset") == 0)
	mapset = G_ask_in_mapset(prompt, name, opt3->answer, opt4->answer);


    if (mapset) {
	fprintf(fd, "name='%s'\n", name);
	fprintf(fd, "mapset='%s'\n", mapset);
	fprintf(fd, "fullname='%s'\n", G_fully_qualified_name(name, mapset));
	G__file_name(file, opt3->answer, name, mapset);
	fprintf(fd, "file='%s'\n", file);
	G__make_mapset_element(opt3->answer);
    }
    else {
	fprintf(fd, "name=\n");
	fprintf(fd, "mapset=\n");
	fprintf(fd, "file=\n");
    }

    fclose(fd);
    exit(EXIT_SUCCESS);
}
