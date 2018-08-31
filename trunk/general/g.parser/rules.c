#include <string.h>
#include "proto.h"

#include <grass/glocale.h>

static void *find_flag(const struct context *ctx, int key)
{
    struct Flag *flag;

    for (flag = ctx->first_flag; flag; flag = flag->next_flag)
	if (flag->key == key)
	    return flag;

    fprintf(stderr, _("Unknown flag \"-%c\" in rule\n"), key);
    return NULL;
}

static void *find_option(const struct context *ctx, const char *key)
{
    struct Option *option;

    for (option = ctx->first_option; option; option = option->next_opt)
	if (G_strcasecmp(option->key, key) == 0)
	    return option;

    fprintf(stderr, _("Unknown option \"%s\" in rule\n"), key);
    return NULL;
}

static void add_rule(struct context *ctx, int type, const char *data)
{
    char **tokens;
    int ntokens;
    void **opts;
    int i;

    tokens = G_tokenize(data, ",");
    ntokens = G_number_of_tokens(tokens);
    opts = G_malloc(ntokens * sizeof(void *));

    for (i = 0; i < ntokens; i++) {
	char buf[256];
	char *name;

	strcpy(buf, tokens[i]);
	name = G_chop(buf);
	opts[i] = (name[0] == '-')
	    ? find_flag(ctx, name[1])
	    : find_option(ctx, name);
    }

    G_free_tokens(tokens);

    G_option_rule(type, ntokens, opts);
}

void parse_rule(struct context *ctx, const char *cmd, const char *arg)
{
    if (G_strcasecmp(cmd, "exclusive") == 0) {
	add_rule(ctx, RULE_EXCLUSIVE, arg);
	return;
    }

    if (G_strcasecmp(cmd, "required") == 0) {
	add_rule(ctx, RULE_REQUIRED, arg);
	return;
    }

    if (G_strcasecmp(cmd, "requires") == 0) {
	add_rule(ctx, RULE_REQUIRES, arg);
	return;
    }

    if (G_strcasecmp(cmd, "requires_all") == 0) {
	add_rule(ctx, RULE_REQUIRES_ALL, arg);
	return;
    }

    if (G_strcasecmp(cmd, "excludes") == 0) {
	add_rule(ctx, RULE_EXCLUDES, arg);
	return;
    }

    if (G_strcasecmp(cmd, "collective") == 0) {
	add_rule(ctx, RULE_COLLECTIVE, arg);
	return;
    }

    if (G_strcasecmp(cmd, "end") == 0) {
	ctx->state = S_TOPLEVEL;
	return;
    }

    fprintf(stderr, _("Unknown rule type \"%s\" at line %d\n"),
	    cmd, ctx->line);
}

