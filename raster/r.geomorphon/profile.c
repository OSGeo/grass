#include <stdio.h>
#include "local_proto.h"

#define JSON_MIN_INDENT 1
#define YAML_MIN_INDENT 0
#define XML_MIN_INDENT 1
#define MAX_STR_LEN GNAME_MAX
#define MAX_TOKENS 20000
#define MAX_STACK_ELEMS 100

#define WRITE_INDENT(f, indent) \
    if (!write_indent((f), (indent))) \
	return 0

/*
 * Instead of using a variadic macro or defining multiple arities just write
 * one value at a time.
 */
#define WRITE_VAL(f, format, v) \
    if (fprintf ((f), (format), (v)) < 0) \
	return 0

typedef enum
{
    T_INT,                      /* integer             */
    T_BLN,                      /* boolean             */
    T_DBL,                      /* double              */
    T_MTR,                      /* metres              */
    T_STR,                      /* string              */
    T_SSO,                      /* start of sub-object */
    T_ESO,                      /* end of sub-object   */
} toktype;

static struct token
{
    toktype type;
    char key[MAX_STR_LEN];
    int int_val;
    double dbl_val;
    char str_val[MAX_STR_LEN];
} token[MAX_TOKENS];

static char stack[MAX_STACK_ELEMS][MAX_STR_LEN];

/*
 * These variables will need a reset function if anybody needs to use this
 * code to write more than one document.
 */
static unsigned size;
static unsigned char overflow;
static unsigned stack_size;

static void prof_int_internal(const toktype type, const char *key,
                              const int val)
{
    if (size == MAX_TOKENS) {
        overflow = 1;
        return;
    }
    token[size].type = type;
    G_snprintf(token[size].key, MAX_STR_LEN, "%s", key);
    token[size].int_val = val;
    size++;
}

void prof_int(const char *key, const int val)
{
    prof_int_internal(T_INT, key, val);
}

void prof_bln(const char *key, const int val)
{
    prof_int_internal(T_BLN, key, val);
}

static void prof_dbl_internal(const toktype type, const char *key,
                              const double val)
{
    if (size == MAX_TOKENS) {
        overflow = 1;
        return;
    }
    token[size].type = type;
    G_snprintf(token[size].key, MAX_STR_LEN, "%s", key);
    token[size].dbl_val = val;
    size++;
}

void prof_dbl(const char *key, const double val)
{
    prof_dbl_internal(T_DBL, key, val);
}

void prof_mtr(const char *key, const double val)
{
    prof_dbl_internal(T_MTR, key, val);
}

void prof_str(const char *key, const char *val)
{
    if (size == MAX_TOKENS) {
        overflow = 1;
        return;
    }
    token[size].type = T_STR;
    G_snprintf(token[size].key, MAX_STR_LEN, "%s", key);
    G_snprintf(token[size].str_val, MAX_STR_LEN, "%s", val);
    size++;
}

void prof_utc(const char *key, const time_t val)
{
    char buf[MAX_STR_LEN];

    strftime(buf, MAX_STR_LEN, "%FT%TZ", gmtime(&val));
    prof_str(key, buf);
}

void prof_sso(const char *key)
{
    if (size == MAX_TOKENS) {
        overflow = 1;
        return;
    }
    token[size].type = T_SSO;
    G_snprintf(token[size].key, MAX_STR_LEN, "%s", key);
    size++;
}

void prof_eso()
{
    if (size == MAX_TOKENS) {
        overflow = 1;
        return;
    }
    token[size].type = T_ESO;
    size++;
}

void prof_pattern(const double o_elevation, const PATTERN * p)
{
    unsigned i;

    prof_mtr("origin_elevation_m", o_elevation);
    prof_int("num_positives", p->num_positives);
    prof_int("num_negatives", p->num_negatives);

    prof_sso("pattern");
    for (i = 0; i < NUM_DIRS; i++)
        prof_int(dirname[i], p->pattern[i]);
    prof_eso();

    prof_sso("rel_elevation_m");
    for (i = 0; i < NUM_DIRS; i++)
        prof_mtr(dirname[i], p->elevation[i]);
    prof_eso();

    prof_sso("abs_elevation_m");
    for (i = 0; i < NUM_DIRS; i++)
        prof_mtr(dirname[i], o_elevation + p->elevation[i]);
    prof_eso();

    prof_sso("distance_m");
    for (i = 0; i < NUM_DIRS; i++)
        prof_mtr(dirname[i], p->distance[i]);
    prof_eso();

    prof_sso("offset_easting_m");
    for (i = 0; i < NUM_DIRS; i++)
        prof_mtr(dirname[i], p->x[i]);
    prof_eso();

    prof_sso("offset_northing_m");
    for (i = 0; i < NUM_DIRS; i++)
        prof_mtr(dirname[i], p->y[i]);
    prof_eso();

    prof_sso("easting");
    for (i = 0; i < NUM_DIRS; i++)
        prof_dbl(dirname[i], p->e[i]);
    prof_eso();

    prof_sso("northing");
    for (i = 0; i < NUM_DIRS; i++)
        prof_dbl(dirname[i], p->n[i]);
    prof_eso();
}

void prof_map_info()
{
    prof_sso("map_info");
    prof_str("elevation_name", elevation.elevname);
    prof_int("projection", G_projection());
    prof_dbl("north", window.north);
    prof_dbl("south", window.south);
    prof_dbl("east", window.east);
    prof_dbl("west", window.west);
    prof_int("rows", Rast_window_rows());
    prof_int("cols", Rast_window_cols());
    prof_dbl("ewres", window.ew_res);
    prof_dbl("nsres", window.ns_res);
    prof_eso();
}

static unsigned write_indent(FILE * f, unsigned char indent)
{
    while (indent--)
        WRITE_VAL(f, "%s", "  ");
    return 1;
}

static const char *quote_val(const toktype t, const char *v)
{
    static char buf[MAX_STR_LEN + 2];

    if (t != T_STR)
        return v;
    G_snprintf(buf, sizeof(buf), "\"%s\"", v);
    return buf;
}

static const char *format_token_common(const struct token *t)
{
    static char buf[MAX_STR_LEN];

    switch (t->type) {
    case T_BLN:
        return t->int_val ? "true" : "false";
    case T_INT:
        G_snprintf(buf, sizeof(buf), "%d", t->int_val);
        return buf;
    case T_DBL:
        if (isnan(t->dbl_val))
            return "null";
        G_snprintf(buf, sizeof(buf), "%.8f", t->dbl_val);
        return buf;
    case T_STR:
        return t->str_val;
    case T_MTR:
        if (isnan(t->dbl_val))
            return "null";
        G_snprintf(buf, sizeof(buf), "%.2f", t->dbl_val);
        return buf;
    default:
        return NULL;
    }
}

/*
 * In the functions below besides checking the indent value after the loop
 * make sure it never drops below the initial value within the loop, even if
 * it would bounce back later. Return 1 on no error.
 */
static unsigned write_json(FILE * f)
{
    unsigned i, indent = JSON_MIN_INDENT;

    WRITE_VAL(f, "%s\n", "{");
    for (i = 0; i < size; i++) {
        const char *val;

        /* Add a comma unless there is no data tokens immediately after. */
        const char *comma = (i + 1 == size) ||
            (i + 1 < size && token[i + 1].type == T_ESO) ? "" : ",";

        switch (token[i].type) {
        case T_SSO:
            WRITE_INDENT(f, indent);
            indent++;
            WRITE_VAL(f, "\"%s\": {\n", token[i].key);
            continue;
        case T_ESO:
            if (indent == JSON_MIN_INDENT)
                return 0;
            indent--;
            WRITE_INDENT(f, indent);
            WRITE_VAL(f, "}%s\n", comma);
            continue;
        default:
            val = quote_val(token[i].type, format_token_common(token + i));
            break;
        }
        if (!val)
            return 0;
        WRITE_INDENT(f, indent);
        WRITE_VAL(f, "\"%s\": ", token[i].key);
        WRITE_VAL(f, "%s", val);
        WRITE_VAL(f, "%s\n", comma);
    }
    if (indent != JSON_MIN_INDENT || overflow)
        return 0;
    WRITE_VAL(f, "%s\n", "}");
    return 1;
}

static unsigned write_yaml(FILE * f)
{
    unsigned i, indent = YAML_MIN_INDENT;

    for (i = 0; i < size; i++) {
        const char *val;

        switch (token[i].type) {
        case T_SSO:
            WRITE_INDENT(f, indent);
            indent++;
            WRITE_VAL(f, "%s:\n", token[i].key);
            continue;
        case T_ESO:
            if (indent == YAML_MIN_INDENT)
                return 0;
            indent--;
            continue;
        default:
            val = quote_val(token[i].type, format_token_common(token + i));
            break;
        }
        if (!val)
            return 0;
        WRITE_INDENT(f, indent);
        WRITE_VAL(f, "%s: ", token[i].key);
        WRITE_VAL(f, "%s\n", val);
    }
    return indent == YAML_MIN_INDENT && !overflow;
}

static unsigned stack_push(const char *s)
{
    if (stack_size == MAX_STACK_ELEMS || strlen(s) + 1 > MAX_STR_LEN) {
        overflow = 1;
        return 0;
    }
    G_snprintf(stack[stack_size], MAX_STR_LEN, "%s", s);
    stack_size++;
    return 1;
}

static const char *stack_pop()
{
    if (!stack_size)
        return NULL;
    return stack[--stack_size];
}

static unsigned write_xml(FILE * f)
{
    unsigned i, indent = XML_MIN_INDENT;

    WRITE_VAL(f, "%s\n", "<geomorphon_profile>");
    for (i = 0; i < size; i++) {
        const char *val;

        switch (token[i].type) {
        case T_SSO:
            WRITE_INDENT(f, indent);
            indent++;
            WRITE_VAL(f, "<%s>\n", token[i].key);
            if (!stack_push(token[i].key))
                return 0;
            continue;
        case T_ESO:
            if (indent == XML_MIN_INDENT)
                return 0;
            indent--;
            if (!(val = stack_pop()))
                return 0;
            WRITE_INDENT(f, indent);
            WRITE_VAL(f, "</%s>\n", val);
            continue;
        default:
            val = format_token_common(token + i);
            break;
        }
        if (!val)
            return 0;
        WRITE_INDENT(f, indent);
        WRITE_VAL(f, "<%s>", token[i].key);
        WRITE_VAL(f, "%s", val);
        WRITE_VAL(f, "</%s>\n", token[i].key);
    }
    if (indent != XML_MIN_INDENT || overflow)
        return 0;
    WRITE_VAL(f, "%s\n", "</geomorphon_profile>");
    return 1;
}

unsigned prof_write(FILE * f, const char *format)
{
    if (!strcmp("json", format))
        return write_json(f);
    if (!strcmp("yaml", format))
        return write_yaml(f);
    if (!strcmp("xml", format))
        return write_xml(f);
    return 0;
}
