/*!
 * \file lib/gis/ls_filter.c
 *
 * \brief GIS Library - Filename filter functions
 *
 * (C) 2010 by Glynn Clements and the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author Glynn Clements
 */

#include <grass/config.h>
#include <grass/gis.h>

#ifdef HAVE_REGEX_H

#include <regex.h>

struct buffer
{
    char *buf;
    size_t len;
    size_t alloc;
};

static void init(struct buffer *buf)
{
    buf->buf = NULL;
    buf->len = 0;
    buf->alloc = 0;
}

static void add(struct buffer *buf, char c)
{
    if (buf->len >= buf->alloc) {
	buf->alloc += 50;
	buf->buf = G_realloc(buf->buf, buf->alloc);
    }

    buf->buf[buf->len++] = c;
}

static void fini(struct buffer *buf)
{
    G_free(buf->buf);
}

static const char *do_set(struct buffer *buf, const char *p)
{
    add(buf, '[');

    if (*p == '!') {
	add(buf, '^');
	p++;
    }

    if (*p == ']') {
	add(buf, ']');
	p++;
    }

    for (; *p && *p != ']'; p++)
	add(buf, *p);

    if (!*p)
	return NULL;

    add(buf, ']');

    return p;
}

static int wc2regex(struct buffer *buf, const char *pat)
{
    const char *p;
    int in_brace = 0;

    init(buf);

    add(buf, '^');

    for (p = pat; p && *p; p++) {
	switch (*p) {
	case '\\':
	    add(buf, '\\');
	    if (!*++p)
		return 0;
	    add(buf, *p);
	    break;
	case '.':
	case '|':
	case '(':
	case ')':
	case '+':
	    add(buf, '\\');
	    add(buf, *p);
	    break;
	case '*':
	    add(buf, '.');
	    add(buf, '*');
	    break;
	case '?':
	    add(buf, '.');
	    break;
	case '{':
	    in_brace++;
	    add(buf, '(');
	    break;
	case '}':
	    if (!in_brace)
		return 0;
	    in_brace--;
	    add(buf, ')');
	    break;
	case ',':
	    if (in_brace)
		add(buf, '|');
	    else
		add(buf, ',');
	    break;
	case '[':
	    if (!(p = do_set(buf, p)))
		return 0;
	    break;
	default:
	    add(buf, *p);
	    break;
	}
    }

    if (!p)
	return 0;

    if (in_brace)
	return 0;

    add(buf, '$');
    add(buf, '\0');

    return 1;
}

static int re_filter(const char *filename, void *closure)
{
    regex_t *regex = closure;

    return filename[0] != '.' && regexec(regex, filename, 0, NULL, 0) == 0;
}

void *G_ls_regex_filter(const char *pat, int exclude, int extended,
			int ignorecase)
{
    regex_t *regex = G_malloc(sizeof(regex_t));

    if (regcomp(regex, pat, REG_NOSUB |
			    (extended ? REG_EXTENDED : 0) |
			    (ignorecase ? REG_ICASE : 0)) != 0) {
	G_free(regex);
	return NULL;
    }

    if (exclude)
	G_set_ls_exclude_filter(re_filter, regex);
    else
	G_set_ls_filter(re_filter, regex);

    return regex;
}

void *G_ls_glob_filter(const char *pat, int exclude, int ignorecase)
{
    struct buffer buf;
    regex_t *regex;

    init(&buf);

    if (!wc2regex(&buf, pat)) {
	fini(&buf);
	return NULL;
    }

    regex = G_ls_regex_filter(buf.buf, exclude, 1, ignorecase);

    fini(&buf);

    return regex;
}

void G_free_ls_filter(void *regex)
{
    if (!regex)
	return;

    regfree(regex);
    G_free(regex);
}

#endif

