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
#endif

#ifdef HAVE_PCRE_H
#include <string.h>
#include <pcre.h>
#endif

struct buffer {
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
#ifdef HAVE_REGEX_H
    regex_t *regex = closure;

    return filename[0] != '.' && regexec(regex, filename, 0, NULL, 0) == 0;
#endif
#ifdef HAVE_PCRE_H
    const char *pcreErrorStr;
    pcre_extra *pcreExtra;
    int pcreExecRet;
    pcre *pcre_regex = closure;

    /* Optimize the regex */
    pcreExtra = pcre_study(pcre_regex, 0, &pcreErrorStr);
    pcreExecRet = pcre_exec(pcre_regex, pcreExtra, filename,
                            strlen(filename), /* length of string */
                            0,                /* Start looking at this point */
                            0,                /* OPTIONS */
                            NULL, 0);         /* Length of subStrVec */

    return filename[0] != '.' && pcreExecRet == 0;
#endif
}

void *G_ls_regex_filter(const char *pat, int exclude, int extended,
                        int ignorecase)
{
#ifdef HAVE_REGEX_H
    regex_t *regex = G_malloc(sizeof(regex_t));

    if (regcomp(regex, pat,
                REG_NOSUB | (extended ? REG_EXTENDED : 0) |
                    (ignorecase ? REG_ICASE : 0)) != 0) {
        G_free(regex);
        return NULL;
    }

    if (exclude)
        G_set_ls_exclude_filter(re_filter, regex);
    else
        G_set_ls_filter(re_filter, regex);

    return regex;
#endif

#ifdef HAVE_PCRE_H
    pcre *pcre_regex;
    const char *pcreErrorStr;
    int pcreErrorOffset;

    /* First, the regex string must be compiled */
    pcre_regex = pcre_compile(pat, 0, &pcreErrorStr, &pcreErrorOffset, NULL);
    /*
       if (regcomp(regex, pat, REG_NOSUB |
       (extended ? REG_EXTENDED : 0) |
       (ignorecase ? REG_ICASE : 0)) != 0) {
       pcre_free(pcre_regex);
       return NULL;
       }
     */
    if (exclude)
        G_set_ls_exclude_filter(re_filter, pcre_regex);
    else
        G_set_ls_filter(re_filter, pcre_regex);

    /* First, the regex string must be compiled */
    pcre_regex = pcre_compile(pat, 0, &pcreErrorStr, &pcreErrorOffset, NULL);
    /*
       if (regcomp(regex, pat, REG_NOSUB |
       (extended ? REG_EXTENDED : 0) |
       (ignorecase ? REG_ICASE : 0)) != 0) {
       pcre_free(pcre_regex);
       return NULL;
       }
     */
    if (exclude)
        G_set_ls_exclude_filter(re_filter, pcre_regex);
    else
        G_set_ls_filter(re_filter, pcre_regex);

    return pcre_regex;
#endif
}

void *G_ls_glob_filter(const char *pat, int exclude, int ignorecase)
{
    struct buffer buf;

#ifdef HAVE_REGEX_H
    regex_t *regex;
#endif
#ifdef HAVE_PCRE_H
    pcre *pcre_regex;
#endif

    init(&buf);

    if (!wc2regex(&buf, pat)) {
        fini(&buf);
        return NULL;
    }
#ifdef HAVE_REGEX_H
    regex = G_ls_regex_filter(buf.buf, exclude, 1, ignorecase);
#endif
#ifdef HAVE_PCRE_H
    pcre_regex = G_ls_regex_filter(buf.buf, exclude, 1, ignorecase);
#endif

    fini(&buf);

#ifdef HAVE_REGEX_H
    return regex;
#endif
#ifdef HAVE_PCRE_H
    return pcre_regex;
#endif
}

void G_free_ls_filter(void *regex)
{
    if (!regex)
        return;
#ifdef HAVE_REGEX_H
    regfree(regex);
#endif
#ifdef HAVE_PCRE_H
    pcre_free(regex);
#endif
}
