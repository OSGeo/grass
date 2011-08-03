
/****************************************************************************
 *
 * MODULE:       g.parser
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com> (original contributor)
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Cedric Shock <cedricgrass shockfamily.net>, 
 *               Hamish Bowman <hamish_b yahoo.com>, 
 *               Paul Kelly <paul-grass stjohnspoint.co.uk>, 
 *               Radim Blazek <radim.blazek gmail.com>
 * PURPOSE:      
 * COPYRIGHT:    (C) 2001-2007, 2010-2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <grass/gis.h>
#include <grass/glocale.h>

enum state
{
    S_TOPLEVEL,
    S_MODULE,
    S_FLAG,
    S_OPTION
};

struct context
{
    struct GModule *module;
    struct Option *option;
    struct Flag *flag;
    struct Option *first_option;
    struct Flag *first_flag;
    int state;
    FILE *fp;
    int line;
};

int translate_output = 0;

/* Returns translated version of a string.
   If global variable to output strings for translation is set it spits them out */
char *translate(const char *arg)
{
    static const char *domain;

    if (*arg && translate_output) {
	fputs(arg, stdout);
	fputs("\n", stdout);
    }

#if defined(HAVE_LIBINTL_H) && defined(USE_NLS)
    if (!domain) {
	domain = getenv("GRASS_TRANSLATION_DOMAIN");
	if (domain)
	    G_putenv("GRASS_TRANSLATION_DOMAIN", "grassmods");
	else
	    domain = PACKAGE;
    }

    return G_gettext(domain, arg);
#else
    return arg;
#endif
}

static int parse_boolean(struct context *ctx, const char *arg)
{
    if (strcasecmp(arg, "yes") == 0)
	return YES;

    if (strcasecmp(arg, "no") == 0)
	return NO;

    fprintf(stderr, "Unknown boolean value \"%s\" at line %d\n",
	    arg, ctx->line);

    return NO;
}

static void parse_toplevel(struct context *ctx, const char *cmd)
{
    if (strcasecmp(cmd, "module") == 0) {
	ctx->state = S_MODULE;
	ctx->module = G_define_module();
	return;
    }

    if (strcasecmp(cmd, "flag") == 0) {
	ctx->state = S_FLAG;
	ctx->flag = G_define_flag();
	if (!ctx->first_flag)
	    ctx->first_flag = ctx->flag;
	return;
    }

    if (strcasecmp(cmd, "option") == 0) {
	ctx->state = S_OPTION;
	ctx->option = G_define_option();
	if (!ctx->first_option)
	    ctx->first_option = ctx->option;
	return;
    }

    fprintf(stderr, "Unknown command \"%s\" at line %d\n", cmd, ctx->line);
}

static void parse_module(struct context *ctx, const char *cmd,
			 const char *arg)
{

    /* Label and description can be internationalized */
    if (strcasecmp(cmd, "label") == 0) {
	ctx->module->label = translate(strdup(arg));
	return;
    }

    if (strcasecmp(cmd, "description") == 0) {
	ctx->module->description = translate(strdup(arg));
	return;
    }

    if (strcasecmp(cmd, "keywords") == 0) {
	G_add_keyword(translate(strdup(arg)));
	return;
    }

    if (strcasecmp(cmd, "end") == 0) {
	ctx->state = S_TOPLEVEL;
	return;
    }

    fprintf(stderr, "Unknown module parameter \"%s\" at line %d\n",
	    cmd, ctx->line);
}

static void parse_flag(struct context *ctx, const char *cmd, const char *arg)
{
    if (strcasecmp(cmd, "key") == 0) {
	ctx->flag->key = arg[0];
	return;
    }

    if (strcasecmp(cmd, "suppress_required") == 0) {
	ctx->flag->suppress_required = parse_boolean(ctx, arg);
	return;
    }

    if (strcasecmp(cmd, "answer") == 0) {
	ctx->flag->answer = atoi(arg);
	return;
    }

    /* Label, description, and guisection can all be internationalized */
    if (strcasecmp(cmd, "label") == 0) {
	ctx->flag->label = translate(strdup(arg));
	return;
    }

    if (strcasecmp(cmd, "description") == 0) {
	ctx->flag->description = translate(strdup(arg));
	return;
    }

    if (strcasecmp(cmd, "guisection") == 0) {
	ctx->flag->guisection = translate(strdup(arg));
	return;
    }

    if (strcasecmp(cmd, "end") == 0) {
	ctx->state = S_TOPLEVEL;
	return;
    }

    fprintf(stderr, "Unknown flag parameter \"%s\" at line %d\n",
	    cmd, ctx->line);
}

static int parse_type(struct context *ctx, const char *arg)
{
    if (strcasecmp(arg, "integer") == 0)
	return TYPE_INTEGER;

    if (strcasecmp(arg, "double") == 0)
	return TYPE_DOUBLE;

    if (strcasecmp(arg, "string") == 0)
	return TYPE_STRING;

    fprintf(stderr, "Unknown type \"%s\" at line %d\n", arg, ctx->line);

    return TYPE_STRING;
}

static void parse_option(struct context *ctx, const char *cmd,
			 const char *arg)
{
    if (strcasecmp(cmd, "key") == 0) {
	ctx->option->key = strdup(arg);
	return;
    }

    if (strcasecmp(cmd, "type") == 0) {
	ctx->option->type = parse_type(ctx, arg);
	return;
    }

    if (strcasecmp(cmd, "required") == 0) {
	ctx->option->required = parse_boolean(ctx, arg);
	return;
    }

    if (strcasecmp(cmd, "multiple") == 0) {
	ctx->option->multiple = parse_boolean(ctx, arg);
	return;
    }

    if (strcasecmp(cmd, "options") == 0) {
	ctx->option->options = strdup(arg);
	return;
    }

    if (strcasecmp(cmd, "key_desc") == 0) {
	ctx->option->key_desc = strdup(arg);
	return;
    }

    /* Label, description, descriptions, and guisection can all be internationalized */
    if (strcasecmp(cmd, "label") == 0) {
	ctx->option->label = translate(strdup(arg));
	return;
    }

    if (strcasecmp(cmd, "description") == 0) {
	ctx->option->description = translate(strdup(arg));
	return;
    }

    if (strcasecmp(cmd, "descriptions") == 0) {
	ctx->option->descriptions = translate(strdup(arg));
	return;
    }

    if (strcasecmp(cmd, "answer") == 0) {
	ctx->option->answer = strdup(arg);
	return;
    }

    if (strcasecmp(cmd, "gisprompt") == 0) {
	ctx->option->gisprompt = strdup(arg);
	return;
    }

    if (strcasecmp(cmd, "guisection") == 0) {
	ctx->option->guisection = translate(strdup(arg));
	return;
    }

    if (strcasecmp(cmd, "guidependency") == 0) {
	ctx->option->guidependency = translate(strdup(arg));
	return;
    }

    if (strcasecmp(cmd, "end") == 0) {
	ctx->state = S_TOPLEVEL;
	return;
    }

    fprintf(stderr, _("Unknown option parameter \"%s\" at line %d\n"),
	    cmd, ctx->line);
}

static int print_options(const struct context *ctx)
{
    struct Option *option;
    struct Flag *flag;
    const char *overwrite = getenv("GRASS_OVERWRITE");
    const char *verbose = getenv("GRASS_VERBOSE");

    printf("@ARGS_PARSED@\n");

    if (overwrite)
	printf("GRASS_OVERWRITE=%s\n", overwrite);

    if (verbose)
	printf("GRASS_VERBOSE=%s\n", verbose);

    for (flag = ctx->first_flag; flag; flag = flag->next_flag)
	printf("flag_%c=%d\n", flag->key, flag->answer ? 1 : 0);

    for (option = ctx->first_option; option; option = option->next_opt)
	printf("opt_%s=%s\n", option->key,
	       option->answer ? option->answer : "");

    return EXIT_SUCCESS;
}

static int reinvoke_script(const struct context *ctx, const char *filename)
{
    struct Option *option;
    struct Flag *flag;

    /* Because shell from MINGW and CygWin converts all variables
     * to uppercase it was necessary to use uppercase variables.
     * Set both until all scripts are updated */
    for (flag = ctx->first_flag; flag; flag = flag->next_flag) {
	char buff[16];

	sprintf(buff, "GIS_FLAG_%c=%d", flag->key, flag->answer ? 1 : 0);
	putenv(G_store(buff));

	sprintf(buff, "GIS_FLAG_%c=%d", toupper(flag->key),
		flag->answer ? 1 : 0);

	G_debug(2, "set %s", buff);
	putenv(G_store(buff));
    }

    for (option = ctx->first_option; option; option = option->next_opt) {
	char upper[4096];
	char *str;

	G_asprintf(&str, "GIS_OPT_%s=%s", option->key,
		   option->answer ? option->answer : "");
	putenv(str);

	strcpy(upper, option->key);
	G_str_to_upper(upper);
	G_asprintf(&str, "GIS_OPT_%s=%s", upper,
		   option->answer ? option->answer : "");

	G_debug(2, "set %s", str);
	putenv(str);
    }

#ifdef __MINGW32__
    {
	/* execlp() and _spawnlp ( _P_OVERLAY,..) do not work, they return 
	 * immediately and that breaks scripts running GRASS scripts
	 * because they dont wait until GRASS script finished */
	/* execlp( "sh", "sh", filename, "@ARGS_PARSED@", NULL); */
	/* _spawnlp ( _P_OVERLAY, filename, filename, "@ARGS_PARSED@", NULL ); */
	int ret;
	char *shell = getenv("GRASS_SH");

	if (shell == NULL)
	    shell = "sh";
	ret = G_spawn(shell, shell, filename, "@ARGS_PARSED@", NULL);
	G_debug(1, "ret = %d", ret);
	if (ret == -1) {
	    perror(_("G_spawn() failed"));
	    return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
    }
#else
    execl(filename, filename, "@ARGS_PARSED@", NULL);

    perror(_("execl() failed"));
    return EXIT_FAILURE;
#endif
}

int main(int argc, char *argv[])
{
    struct context ctx;
    const char *filename;
    int standard_output = 0;

    ctx.module = NULL;
    ctx.option = NULL;
    ctx.flag = NULL;
    ctx.first_option = NULL;
    ctx.first_flag = NULL;
    ctx.state = S_TOPLEVEL;

    /* Detect request to get strings to translate from a file */
    /* It comes BEFORE the filename to completely avoid confusion with parser.c behaviours */
    if (argc >= 2 && (strcmp(argv[1], "-t") == 0)) {
	/* Turn on translation output */
	translate_output = 1;
	argv++, argc--;
    }

    if (argc >= 2 && (strcmp(argv[1], "-s") == 0)) {
	/* write to stdout rather than re-invoking */
	standard_output = 1;
	argv++, argc--;
    }

    if ((argc < 2) || ((strcmp(argv[1], "help") == 0) ||
		       (strcmp(argv[1], "-help") == 0) ||
		       (strcmp(argv[1], "--help") == 0))) {
	fprintf(stderr, "%s: %s [-t] [-s] <filename> [<argument> ...]\n",
		_("Usage:"), argv[0]);
	exit(EXIT_FAILURE);
    }

    filename = argv[1];
    argv++, argc--;
    G_debug(2, "filename = %s", filename);

    ctx.fp = fopen(filename, "r");
    if (!ctx.fp) {
	perror(_("Unable to open script file"));
	exit(EXIT_FAILURE);
    }

    G_gisinit((char *)filename);

    for (ctx.line = 1;; ctx.line++) {
	char buff[4096];
	char *cmd, *arg;

	if (!fgets(buff, sizeof(buff), ctx.fp))
	    break;

	arg = strchr(buff, '\n');
	if (!arg) {
	    fprintf(stderr, _("Line too long or missing newline at line %d\n"),
		    ctx.line);
	    exit(EXIT_FAILURE);
	}
	*arg = '\0';

	if (buff[0] != '#' || buff[1] != '%')
	    continue;

	cmd = buff + 2;
	G_chop(cmd);

	arg = strchr(cmd, ':');

	if (arg) {
	    *(arg++) = '\0';
	    G_strip(cmd);
	    G_strip(arg);
	}

	switch (ctx.state) {
	case S_TOPLEVEL:
	    parse_toplevel(&ctx, cmd);
	    break;
	case S_MODULE:
	    parse_module(&ctx, cmd, arg);
	    break;
	case S_FLAG:
	    parse_flag(&ctx, cmd, arg);
	    break;
	case S_OPTION:
	    parse_option(&ctx, cmd, arg);
	    break;
	}
    }

    if (fclose(ctx.fp) != 0) {
	perror(_("Error closing script file"));
	exit(EXIT_FAILURE);
    }

    /* Stop here successfully if all that was desired was output of text to translate */
    /* Continuing from here would get argc and argv all wrong in G_parser. */
    if (translate_output)
	exit(EXIT_SUCCESS);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    return standard_output
	? print_options(&ctx)
	: reinvoke_script(&ctx, filename);
}
