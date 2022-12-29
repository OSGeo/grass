#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "rule.h"

static int scan_value(CELL *);
static const char *cur;
static int state;
int default_rule = 0;
int default_to_itself = 0;
char *default_label;
CELL DEFAULT;

int parse(const char *line, RULE ** rules, RULE ** tail, struct Categories *cats)
{
    const char *label;
    const char *save;
    CELL v;
    CELL lo[1024], hi[1024], new = (CELL) 0;
    int count;
    int i, last_null = 0;

    cur = line;
    state = 0;
    count = 0;
    label = "";

    while (*cur == ' ' || *cur == '\t' || *cur == '\n')
	cur++;
    while (*cur) {
	while (*cur == ' ' || *cur == '\t' || *cur == '\n')
	    cur++;
	if (!*cur)
	    break;

	switch (state) {
	case 0:
	    save = cur;
	    if (!strncmp(cur, "help", 4)) {	/* help text */
		fprintf(stderr, _("Enter a rule in one of these formats:\n"));
		fprintf(stderr, "1 3 5      = 1   %s\n", _("poor quality"));
		fprintf(stderr, "1 thru 10  = 1\n");
		fprintf(stderr, "20 thru 50 = 2   %s\n", _("medium quality"));
		fprintf(stderr, "*          = NULL\n");
		state = 0;
		cur += 4;
		continue;
	    }
	    if (*cur == '*') {	/* default rule */
		default_rule = 1;
		state = 1;
		cur++;
		continue;
	    }
	    if (!scan_value(&v))
		return -1;
	    if (Rast_is_c_null_value(&v)) {
		G_warning(_("Can't have null on the left-hand side of the rule"));
		return -1;
	    }
	    state = 1;
	    cur = save;
	    continue;
	case 1:
	    if (*cur == '=') {
		cur++;
		state = 4;
		continue;
	    }
	    if (default_rule)
		return -1;
	    if (!scan_value(&v))
		return -1;
	    if (Rast_is_c_null_value(&v))
		last_null = 1;
	    else
		last_null = 0;
	    lo[count] = hi[count] = v;
	    count++;
	    state = 2;
	    continue;
	case 2:
	    state = 1;
	    if (strncmp(cur, "thru", 4) != 0)
		continue;
	    if (last_null) {
		G_warning(_("Can't have null on the right-hand side of the rule"));
		return -1;
	    }
	    cur += 4;
	    if (*cur != ' ' && *cur != '\t')
		return -1;
	    state = 3;
	    continue;
	case 3:
	    if (!scan_value(&v))
		return -1;
	    if (Rast_is_c_null_value(&v)) {
		G_warning(_("Can't have null on the right-hand side of the rule"));
		return -1;
	    }

	    if (lo[count - 1] > v) {
		hi[count - 1] = lo[count - 1];
		lo[count - 1] = v;
	    }
	    else
		hi[count - 1] = v;

	    state = 1;
	    continue;
	case 4:
	    if (*cur == '*' && default_rule) {
		cur++;
		new = 0;
		default_to_itself = 1;
		state = 5;
		continue;
	    }

	    if (!scan_value(&v))
		return -1;
	    new = v;
	    state = 5;
	    continue;
	case 5:
	    label = cur;
	    cur = "";		/* force break from while */
	}
    }
    if (state > 0 && state < 5)
	return -1;
    if (default_rule) {
	DEFAULT = new;
	default_label = G_store((*label ? label : ""));
	return 1;
    }

    for (i = 0; i < count; i++) {
	add_rule(tail, lo[i], hi[i], new);
	if (*rules == NULL)
	    *rules = *tail;
	if (*label)
	    Rast_set_c_cat(&new, &new, label, cats);
    }
    return count;
}

static int scan_value(CELL * v)
{
    int i, sign, dec;
    double fv, fd;

    if (strncmp(cur, "null", 4) == 0 || strncmp(cur, "NULL", 4) == 0) {
	cur += 4;
	Rast_set_c_null_value(v, 1);
    }
    else {
	sign = 1;
	if (*cur == '-') {
	    sign = -1;
	    cur++;
	}
	/*
	   if (*cur < '0' || *cur > '9')
	   return 0;
	   *v = *cur++ - '0' ;
	 */
	dec = 0;
	fv = 0.0;

	while ((*cur >= '0' && *cur <= '9') || *cur == '.') {
	    if (*cur == '.') {
		if (!dec)
		    dec++;
		cur++;
		continue;
	    }
	    if (!dec)
		fv = fv * 10 + *cur++ - '0';
	    else {
		fd = 1.0;
		for (i = 0; i < dec; i++)
		    fd *= 0.1;
		dec++;
		fv += (*cur++ - '0') * fd;
	    }
	}

	if (dec)
	    fv += 0.5;
	*v = sign * (CELL) fv;

	if (dec && state)
	    fprintf(stderr, _("%f rounded up to %d\n"), sign * fv, *v);
    }

    switch (*cur) {
    case 0:
    case ' ':
    case '\t':
    case '\n':
    case '=':
	return 1;
    default:
	return 0;
    }
}
