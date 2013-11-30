#include <string.h>
#include <stdlib.h>

#include "proto.h"

#include <grass/glocale.h>

static char *xstrdup(const char *arg)
{
   return (G_strcasecmp(arg, "{NULL}") == 0)
	? NULL : strdup(arg);
}

int parse_boolean(struct context *ctx, const char *arg)
{
    if (G_strcasecmp(arg, "yes") == 0)
	return YES;

    if (G_strcasecmp(arg, "no") == 0)
	return NO;

    fprintf(stderr, _("Unknown boolean value \"%s\" at line %d\n"),
	    arg, ctx->line);

    return NO;
}

void parse_toplevel(struct context *ctx, const char *cmd)
{
    char **tokens;
    
    if (G_strcasecmp(cmd, "module") == 0) {
	ctx->state = S_MODULE;
	ctx->module = G_define_module();
	return;
    }

    if (G_strcasecmp(cmd, "flag") == 0) {
	ctx->state = S_FLAG;
	ctx->flag = G_define_flag();
	if (!ctx->first_flag)
	    ctx->first_flag = ctx->flag;
	return;
    }

    if (G_strncasecmp(cmd, "option", strlen("option")) == 0) {
	ctx->state = S_OPTION;
	
	tokens = G_tokenize(cmd, " ");
	if (G_number_of_tokens(tokens) > 1) {
	    /* standard option */
	    ctx->option = define_standard_option(tokens[1]);
	}
	else { 
	    ctx->option = G_define_option();
	}
	
	if (!ctx->first_option)
		ctx->first_option = ctx->option;
	
	G_free_tokens(tokens);
	return;
    }

    fprintf(stderr, _("Unknown command \"%s\" at line %d\n"), cmd, ctx->line);
}

void parse_module(struct context *ctx, const char *cmd, const char *arg)
{

    /* Label and description can be internationalized */
    if (G_strcasecmp(cmd, "label") == 0) {
	ctx->module->label = translate(xstrdup(arg));
	return;
    }

    if (G_strcasecmp(cmd, "description") == 0) {
	ctx->module->description = translate(xstrdup(arg));
	return;
    }

    if (G_strcasecmp(cmd, "keywords") == 0) {
	G_add_keyword(translate(xstrdup(arg)));
	return;
    }

    if (G_strcasecmp(cmd, "overwrite") == 0) {
        ctx->module->overwrite = parse_boolean(ctx, arg);
        return;
    }

    if (G_strcasecmp(cmd, "end") == 0) {
	ctx->state = S_TOPLEVEL;
	return;
    }

    fprintf(stderr, _("Unknown module parameter \"%s\" at line %d\n"),
	    cmd, ctx->line);
}

void parse_flag(struct context *ctx, const char *cmd, const char *arg)
{
    if (G_strcasecmp(cmd, "key") == 0) {
	ctx->flag->key = arg[0];
	return;
    }

    if (G_strcasecmp(cmd, "suppress_required") == 0) {
	ctx->flag->suppress_required = parse_boolean(ctx, arg);
	return;
    }

    /* Label, description, and guisection can all be internationalized */
    if (G_strcasecmp(cmd, "label") == 0) {
	ctx->flag->label = translate(xstrdup(arg));
	return;
    }

    if (G_strcasecmp(cmd, "description") == 0) {
	ctx->flag->description = translate(xstrdup(arg));
	return;
    }

    if (G_strcasecmp(cmd, "guisection") == 0) {
	ctx->flag->guisection = translate(xstrdup(arg));
	return;
    }

    if (G_strcasecmp(cmd, "end") == 0) {
	ctx->state = S_TOPLEVEL;
	return;
    }

    fprintf(stderr, _("Unknown flag parameter \"%s\" at line %d\n"),
	    cmd, ctx->line);
}

int parse_type(struct context *ctx, const char *arg)
{
    if (G_strcasecmp(arg, "integer") == 0)
	return TYPE_INTEGER;

    if (G_strcasecmp(arg, "double") == 0)
	return TYPE_DOUBLE;

    if (G_strcasecmp(arg, "string") == 0)
	return TYPE_STRING;

    fprintf(stderr, _("Unknown type \"%s\" at line %d\n"), arg, ctx->line);

    return TYPE_STRING;
}

void parse_option(struct context *ctx, const char *cmd, const char *arg)
{
    if (G_strcasecmp(cmd, "key") == 0) {
	ctx->option->key = xstrdup(arg);
	return;
    }

    if (G_strcasecmp(cmd, "type") == 0) {
	ctx->option->type = parse_type(ctx, arg);
	return;
    }

    if (G_strcasecmp(cmd, "required") == 0) {
	ctx->option->required = parse_boolean(ctx, arg);
	return;
    }

    if (G_strcasecmp(cmd, "multiple") == 0) {
	ctx->option->multiple = parse_boolean(ctx, arg);
	return;
    }

    if (G_strcasecmp(cmd, "options") == 0) {
	ctx->option->options = xstrdup(arg);
	return;
    }

    if (G_strcasecmp(cmd, "key_desc") == 0) {
	ctx->option->key_desc = xstrdup(arg);
	return;
    }

    /* Label, description, descriptions, and guisection can all be internationalized */
    if (G_strcasecmp(cmd, "label") == 0) {
	ctx->option->label = translate(xstrdup(arg));
	return;
    }

    if (G_strcasecmp(cmd, "description") == 0) {
	ctx->option->description = translate(xstrdup(arg));
	return;
    }

    if (G_strcasecmp(cmd, "descriptions") == 0) {
	ctx->option->descriptions = translate(xstrdup(arg));
	return;
    }

    if (G_strcasecmp(cmd, "answer") == 0) {
	ctx->option->answer = xstrdup(arg);
	return;
    }

    if (G_strcasecmp(cmd, "gisprompt") == 0) {
	ctx->option->gisprompt = xstrdup(arg);
	return;
    }

    if (G_strcasecmp(cmd, "guisection") == 0) {
	ctx->option->guisection = translate(xstrdup(arg));
	return;
    }

    if (G_strcasecmp(cmd, "guidependency") == 0) {
	ctx->option->guidependency = translate(xstrdup(arg));
	return;
    }

    if (G_strcasecmp(cmd, "end") == 0) {
	ctx->state = S_TOPLEVEL;
	return;
    }

    fprintf(stderr, _("Unknown option parameter \"%s\" at line %d\n"),
	    cmd, ctx->line);
}

int print_options(const struct context *ctx, int sep)
{
    struct Option *option;
    struct Flag *flag;
    const char *overwrite = getenv("GRASS_OVERWRITE");
    const char *verbose = getenv("GRASS_VERBOSE");

    printf("@ARGS_PARSED@%c", sep);

    if (overwrite)
	printf("GRASS_OVERWRITE=%s%c", overwrite, sep);

    if (verbose)
	printf("GRASS_VERBOSE=%s%c", verbose, sep);

    for (flag = ctx->first_flag; flag; flag = flag->next_flag)
	printf("flag_%c=%d%c", flag->key, flag->answer ? 1 : 0, sep);

    for (option = ctx->first_option; option; option = option->next_opt)
	printf("opt_%s=%s%c", option->key,
	       option->answer ? option->answer : "", sep);

    fflush(stdout);

    return EXIT_SUCCESS;
}
