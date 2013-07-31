
/****************************************************************************
 *
 * MODULE:       r.mapcalc
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               rewritten 2002: Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/glocale.h>

#include "mapcalc.h"

/****************************************************************************/

int overflow_occurred;
int overwrite_flag;

volatile int floating_point_exception;
volatile int floating_point_exception_occurred;

/****************************************************************************/

static expr_list *result;

/****************************************************************************/

static RETSIGTYPE handle_fpe(int n)
{
    floating_point_exception = 1;
    floating_point_exception_occurred = 1;
}

static void pre_exec(void)
{
#ifndef __MINGW32__
#ifdef SIGFPE
    struct sigaction act;

    act.sa_handler = &handle_fpe;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    sigaction(SIGFPE, &act, NULL);
#endif
#endif

    floating_point_exception_occurred = 0;
    overflow_occurred = 0;
}

static void post_exec(void)
{
#ifndef __MINGW32__
#ifdef SIGFPE
    struct sigaction act;

    act.sa_handler = SIG_DFL;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    sigaction(SIGFPE, &act, NULL);
#endif
#endif
}

/****************************************************************************/

static expr_list *parse_file(const char *filename)
{
    expr_list *res;
    FILE *fp;

    if (strcmp(filename, "-") == 0)
	return parse_stream(stdin);

    fp = fopen(filename, "r");
    if (!fp)
	G_fatal_error(_("Unable to open input file <%s>"), filename);

    res = parse_stream(fp);

    fclose(fp);

    return res;
}

/****************************************************************************/

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *expr, *file;
    int all_ok;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("algebra"));
    module->description = _("Raster map calculator.");
    module->overwrite = 1;

    expr = G_define_option();
    expr->key = "expression";
    expr->type = TYPE_STRING;
    expr->required = NO;
    expr->description = _("Expression to evaluate");
    expr->guisection = _("Expression");
    
    file = G_define_standard_option(G_OPT_F_INPUT);
    file->key = "file";
    file->required = NO;
    file->description = _("File containing expression(s) to evaluate");
    file->guisection = _("Expression");

    if (argc == 1)
    {
	char **p = G_malloc(3 * sizeof(char *));
	p[0] = argv[0];
	p[1] = G_store("file=-");
	p[2] = NULL;
	argv = p;
	argc = 2;
    }

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    overwrite_flag = module->overwrite;

    if (expr->answer && file->answer)
	G_fatal_error(_("file= and expression= are mutually exclusive"));

    if (expr->answer)
	result = parse_string(expr->answer);
    else if (file->answer)
	result = parse_file(file->answer);
    else
	result = parse_stream(stdin);

    if (!result)
	G_fatal_error(_("parse error"));

    pre_exec();
    execute(result);
    post_exec();

    all_ok = 1;

    if (floating_point_exception_occurred) {
	G_warning(_("Floating point error(s) occurred in the calculation"));
	all_ok = 0;
    }

    if (overflow_occurred) {
	G_warning(_("Overflow occurred in the calculation"));
	all_ok = 0;
    }

    return all_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

/****************************************************************************/
