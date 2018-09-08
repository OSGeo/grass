/*!
  \file lib/gis/parser_dependencies.c
  
  \brief GIS Library - Argument parsing functions (dependencies between options)
  
  (C) 2014-2015 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Glynn Clements Jun. 2014
*/

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "parser_local_proto.h"

struct vector {
    size_t elsize;
    size_t increment;
    size_t count;
    size_t limit;
    void *data;
};

static void vector_new(struct vector *v, size_t elsize, size_t increment)
{
    v->elsize = elsize;
    v->increment = increment;
    v->count = 0;
    v->limit = 0;
    v->data = NULL;
}

static void vector_append(struct vector *v, const void *data)
{
    void *p;

    if (v->count >= v->limit) {
	v->limit += v->increment;
	v->data = G_realloc(v->data, v->limit * v->elsize);
    }

    p = G_incr_void_ptr(v->data, v->count * v->elsize);
    memcpy(p, data, v->elsize);
    v->count++;
}

struct rule {
    int type;
    int count;
    void **opts;
};

static struct vector rules = {sizeof(struct rule), 50};

/*! \brief Set generic option rule

   Supported rule types:
    - RULE_EXCLUSIVE
    - RULE_REQUIRED
    - RULE_REQUIRES
    - RULE_REQUIRES_ALL
    - RULE_EXCLUDES
    - RULE_COLLECTIVE
   
   \param type rule type
   \param nopts number of options in the array
   \param opts array of options
*/
void G_option_rule(int type, int nopts, void **opts)
{
    struct rule rule;

    rule.type = type;
    rule.count = nopts;
    rule.opts = opts;

    vector_append(&rules, &rule);
}

static void make_rule(int type, void *first, va_list ap)
{
    struct vector opts;
    void *opt;

    vector_new(&opts, sizeof(void *), 10);

    opt = first;
    vector_append(&opts, &opt);
    for (;;) {
	opt = va_arg(ap, void*);
	if (!opt)
	    break;
	vector_append(&opts, &opt);
    }

    G_option_rule(type, opts.count, (void**) opts.data);
}

static int is_flag(const void *p)
{
    if (st->n_flags) {
	const struct Flag *flag;
	for (flag = &st->first_flag; flag; flag = flag->next_flag)
	    if ((const void *) flag == p)
		return 1;
    }

    if (st->n_opts) {
	const struct Option *opt;
	for (opt = &st->first_option; opt; opt = opt->next_opt)
	    if ((const void *) opt == p)
		return 0;
    }

    G_fatal_error(_("Internal error: option or flag not found"));
}

static int is_present(const void *p)
{
    if (is_flag(p)) {
	const struct Flag *flag = p;
	return (int) flag->answer;
    }
    else {
	const struct Option *opt = p;
	return opt->count > 0;
    }
}

static char *get_name(const void *p)
{
    if (is_flag(p)) {
	char *s;
	G_asprintf(&s, "-%c", ((const struct Flag *) p)->key);
	return s;
    }
    else
	return G_store(((const struct Option *) p)->key);
}

static int count_present(const struct rule *rule, int start)
{
    int i;
    int count = 0;

    for (i = start; i < rule->count; i++)
	if (is_present(rule->opts[i]))
	    count++;

    return count;
}

static const char *describe_rule(const struct rule *rule, int start,
				 int disjunction)
{
    char *s = get_name(rule->opts[start]);
    int i;

    for (i = start + 1; i < rule->count - 1; i++) {
	char *s0 = s;
	char *ss = get_name(rule->opts[i]);
	s = NULL;
	G_asprintf(&s, "%s>, <%s", s0, ss);
	G_free(s0);
	G_free(ss);
    }

    if (rule->count - start > 1) {
	char *s0 = s;
	char *ss = get_name(rule->opts[i]);
	s = NULL;
	G_asprintf(&s, disjunction ? _("<%s> or <%s>") : _("<%s> and <%s>"), s0, ss);
	G_free(s0);
	G_free(ss);
    }

    return s;
}

static void append_error(const char *msg)
{
    st->error = G_realloc(st->error, sizeof(char *) * (st->n_errors + 1));
    st->error[st->n_errors++] = G_store(msg);
}

/*! \brief Sets the options to be mutually exclusive.

    When running the module, at most one option from a set can be
    provided.

    \param first first given option
*/
void G_option_exclusive(void *first, ...)
{
    va_list ap;
    va_start(ap, first);
    make_rule(RULE_EXCLUSIVE, first, ap);
    va_end(ap);
}

static void check_exclusive(const struct rule *rule)
{
    if (count_present(rule, 0) > 1) {
	char *err;
	G_asprintf(&err, _("Options %s are mutually exclusive"),
		   describe_rule(rule, 0, 0));
	append_error(err);
    }
}

/*! \brief Sets the options to be required.
  
    At least one option from a set must be given.

    \param first first given option
*/
void G_option_required(void *first, ...)
{
    va_list ap;
    va_start(ap, first);
    make_rule(RULE_REQUIRED, first, ap);
    va_end(ap);
}

static void check_required(const struct rule *rule)
{
    if (count_present(rule, 0) < 1) {
	char *err;
	G_asprintf(&err, _("At least one of the following options is required: %s"),
		   describe_rule(rule, 0, 0));
	append_error(err);
    }
}

/*! \brief Define a list of options from which at least one option
    is required if first option is present.

    If the first option is present, at least one of the other
    options must also be present.

    If you want all options to be provided use G_option_requires_all()
    function.
    If you want more than one option to be present but not all,
    call this function multiple times.

    \param first first given option
*/
void G_option_requires(void *first, ...)
{
    va_list ap;
    va_start(ap, first);
    make_rule(RULE_REQUIRES, first, ap);
    va_end(ap);
}

static void check_requires(const struct rule *rule)
{
    if (!is_present(rule->opts[0]))
	return;
    if (count_present(rule, 1) < 1) {
	char *err;
        if (rule->count > 2)
            G_asprintf(&err, _("Option <%s> requires at least one of %s"),
                       get_name(rule->opts[0]), describe_rule(rule, 1, 1));
        else
            G_asprintf(&err, _("Option <%s> requires <%s>"),
                       get_name(rule->opts[0]), describe_rule(rule, 1, 1));
        append_error(err);
    }
}

/*! \brief Define additionally required options for an option.

    If the first option is present, all the other options must also
    be present.

    If it is enough if only one option from a set is present,
    use G_option_requires() function.

    \see G_option_collective()

    \param first first given option
*/
void G_option_requires_all(void *first, ...)
{
    va_list ap;
    va_start(ap, first);
    make_rule(RULE_REQUIRES_ALL, first, ap);
    va_end(ap);
}

static void check_requires_all(const struct rule *rule)
{
    if (!is_present(rule->opts[0]))
	return;
    if (count_present(rule, 1) < rule->count - 1) {
	char *err;
	G_asprintf(&err, _("Option <%s> requires all of %s"),
		   get_name(rule->opts[0]), describe_rule(rule, 1, 0));
	append_error(err);
    }
}

/*! \brief Exclude selected options.

    If the first option is present, none of the other options may also (should?)
    be present.

    \param first first given option
*/
void G_option_excludes(void *first, ...)
{
    va_list ap;
    va_start(ap, first);
    make_rule(RULE_EXCLUDES, first, ap);
    va_end(ap);
}

static void check_excludes(const struct rule *rule)
{
    if (!is_present(rule->opts[0]))
	return;
    if (count_present(rule, 1) > 0) {
	char *err;
	G_asprintf(&err, _("Option <%s> is mutually exclusive with all of %s"),
		   get_name(rule->opts[0]), describe_rule(rule, 1, 0));
	append_error(err);
    }
}

/*! \brief Sets the options to be collective.

    If any option is present, all the other options must also be present
    all or nothing from a set.

    \param first first given option
*/
void G_option_collective(void *first, ...)
{
    va_list ap;
    va_start(ap, first);
    make_rule(RULE_COLLECTIVE, first, ap);
    va_end(ap);
}

static void check_collective(const struct rule *rule)
{
    int count = count_present(rule, 0);
    if (count > 0 && count < rule->count) {
	char *err;
	G_asprintf(&err, _("Either all or none of %s must be given"),
		   describe_rule(rule, 0, 0));
	append_error(err);
    }
}

/*! \brief Check for option rules (internal use only) */
void G__check_option_rules(void)
{
    unsigned int i;

    for (i = 0; i < rules.count; i++) {
	const struct rule *rule = &((const struct rule *) rules.data)[i];
	switch (rule->type) {
	case RULE_EXCLUSIVE:
	    check_exclusive(rule);
	    break;
	case RULE_REQUIRED:
	    check_required(rule);
	    break;
	case RULE_REQUIRES:
	    check_requires(rule);
	    break;
	case RULE_REQUIRES_ALL:
	    check_requires_all(rule);
	    break;
	case RULE_EXCLUDES:
	    check_excludes(rule);
	    break;
	case RULE_COLLECTIVE:
	    check_collective(rule);
	    break;
	default:
	    G_fatal_error(_("Internal error: invalid rule type: %d"),
			  rule->type);
	    break;
	}
    }
}

/*! \brief Describe option rules (stderr) */
void G__describe_option_rules(void)
{
    unsigned int i;

    for (i = 0; i < rules.count; i++) {
	const struct rule *rule = &((const struct rule *) rules.data)[i];
	switch (rule->type) {
	case RULE_EXCLUSIVE:
	    fprintf(stderr, "Exclusive: %s", describe_rule(rule, 0, 0));
	    break;
	case RULE_REQUIRED:
	    fprintf(stderr, "Required: %s", describe_rule(rule, 0, 1));
	    break;
	case RULE_REQUIRES:
	    fprintf(stderr, "Requires: %s => %s", get_name(rule->opts[0]),
		    describe_rule(rule, 1, 1));
	    break;
	case RULE_REQUIRES_ALL:
	    fprintf(stderr, "Requires: %s => %s", get_name(rule->opts[0]),
		    describe_rule(rule, 1, 0));
	    break;
	case RULE_EXCLUDES:
	    fprintf(stderr, "Excludes: %s => %s", get_name(rule->opts[0]),
		    describe_rule(rule, 1, 0));
	    break;
	case RULE_COLLECTIVE:
	    fprintf(stderr, "Collective: %s", describe_rule(rule, 0, 0));
	    break;
	default:
	    G_fatal_error(_("Internal error: invalid rule type: %d"),
			  rule->type);
	    break;
	}
    }
}

/*!
   \brief Checks if there is any rule RULE_REQUIRED (internal use only).

   \return 1 if there is such rule
   \return 0 if not
 */
int G__has_required_rule(void)
{
    size_t i;

    for (i = 0; i < rules.count; i++) {
	const struct rule *rule = &((const struct rule *) rules.data)[i];
	if (rule->type == RULE_REQUIRED)
	    return TRUE;
    }
    return FALSE;
}

static const char * const rule_types[] = {
    "exclusive",
    "required",
    "requires",
    "requires-all",
    "excludes",
    "collective"
};

/*! \brief Describe option rules in XML format (internal use only)

    \param fp file where to print XML info
*/
void G__describe_option_rules_xml(FILE *fp)
{
    unsigned int i, j;

    if (!rules.count)
	return;

    fprintf(fp, "\t<rules>\n");
    for (i = 0; i < rules.count; i++) {
	const struct rule *rule = &((const struct rule *) rules.data)[i];
	fprintf(fp, "\t\t<rule type=\"%s\">\n", rule_types[rule->type]);
	for (j = 0; j < rule->count; j++) {
	    void *p = rule->opts[j];
	    if (is_flag(p)) {
		const struct Flag *flag = (const struct Flag *) p;
		fprintf(fp, "\t\t\t<rule-flag key=\"%c\"/>\n", flag->key);
	    }
	    else {
		const struct Option *opt = (const struct Option *) p;
		fprintf(fp, "\t\t\t<rule-option key=\"%s\"/>\n", opt->key);
	    }
	}
	fprintf(fp, "\t\t</rule>\n");
    }
    fprintf(fp, "\t</rules>\n");
}

