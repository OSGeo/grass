/*!
 * \file lib/gis/aprintf.c
 *
 * \brief GIS Library - Print functions for aligning wide characters.
 *
 * Extracted from the aligned printf C library (libaprintf under GPL v3+) by
 * Huidae Cho.
 *
 * (C) 2020 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Huidae Cho
 *
 * \date 2020
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <grass/gis.h>
#include <grass/glocale.h>

/* printf(3) man page */
#define CONVS "diouxXeEfFgGaAcsCSpnm%"

/* % + flags + width + precision + length + conversion + NULL */
#define SPEC_BUF_SIZE 16

struct options
{
    FILE *stream;
    char *str, *_str;
    size_t size, _size;
};

static int count_wide_chars(const char *);
static int count_wide_chars_in_cols(const char *, int, int *);
static int ovprintf(struct options *, const char *, va_list);
static int oprintf(struct options *, const char *, ...);
static int oaprintf(struct options *, const char *, va_list);

/*!
 * \brief Count the number of wide characters in a string.
 *
 * \param[in] str input string
 * \return number of wide characters in str
 */
static int count_wide_chars(const char *str)
{
    int nwchars = 0, lead = 0;

    while (*str)
	/* if the first two bits are 10 (0x80 = 1000 0000), this byte is
	 * following a previous multi-byte character */
	if ((*str++ & 0xc0) != 0x80)
	    lead = 1;
	else if (lead) {
	    /* only count the second byte of a multi-byte character */
	    lead = 0;
	    nwchars++;
	}

    return nwchars;
}

/*!
 * \brief Count the numbers of wide characters and bytes in a string in a
 * number of columns.
 *
 * \param[in] str input string
 * \param[in] ncols number of columns
 * \param[out] nbytes number of bytes (NULL for not counting)
 * \return number of wide characters in str
 */
static int count_wide_chars_in_cols(const char *str, int ncols, int *nbytes)
{
    const char *p = str - 1;
    int lead = 0, nwchars = 0;

    /* count the numbers of wide characters and bytes in one loop */
    while (ncols >= 0 && *++p)
	if ((*p & 0xc0) != 0x80) {
	    /* a single-byte character or the leading byte of a multi-byte
	     * character; don't count it */
	    lead = 1;
	    ncols--;
	} else if (lead) {
	    /* only count the second byte of a multi-byte character; don't
	     * consume more than two columns (leading and second bytes) */
	    lead = 0;
	    ncols--;
	    nwchars++;
	}

    /* if the current byte after ncols is still part of a multi-byte character,
     * trash it because it's not a full wide character */
    if ((*p & 0xc0) == 0x80)
	nwchars--;

    /* see how many bytes we have advanced */
    *nbytes = p - str;

    return nwchars;
}

/*!
 * \brief Branch into vprintf(), vfprintf(), or vsprintf() depending on passed
 * options.
 *
 * \param[in] opts options for branching
 * \param[in] format string format
 * \param[in] ap variable argument list for the format string
 * \return number of bytes printed or fatal error on error
 */
static int ovprintf(struct options *opts, const char *format, va_list ap)
{
    int nbytes;

    if (opts == NULL || (opts->stream == NULL && opts->_str == NULL))
	nbytes = vprintf(format, ap);
    else if (opts->stream)
	nbytes = vfprintf(opts->stream, format, ap);
    else {
	if ((long int)opts->size >= 0) {
	    /* snprintf(str, 0, ...) does not alter str */
	    nbytes = vsnprintf(opts->_str, opts->_size, format, ap);
	    opts->_size -= nbytes;
	} else
	    /* snprintf(str, negative, ...) is equivalent to snprintf(str, ...)
	     * because size_t is unsigned */
	    nbytes = vsprintf(opts->_str, format, ap);
	opts->_str += nbytes;
    }

    if (nbytes < 0)
	G_fatal_error(_("Failed to print %s"), format);

    return nbytes;
}

/*!
 * \brief Invoke ovprintf() for branching into different *printf() functions.
 *
 * \param[in] opts options for branching
 * \param[in] format string format
 * \param[in] ... arguments for the format string
 * \return number of bytes printed or fatal error on error
 */
static int oprintf(struct options *opts, const char *format, ...)
{
    va_list ap;
    int nbytes;

    va_start(ap, format);
    nbytes = ovprintf(opts, format, ap);
    va_end(ap);

    return nbytes;
}

/*!
 * \brief Core function for aligning wide characters with Latin characters
 * using %s specifiers. G_aprintf(), G_faprintf(), and G_saprintf() wrap around
 * this function to implement printf(), fprintf(), and sprintf() counterparts,
 * respectively.
 *
 * \param[in] opts options for branching
 * \param[in] format string format
 * \param[in] ap variable argument list for the format string
 * \return number of bytes printed or fatal error on error
 */
static int oaprintf(struct options *opts, const char *format, va_list ap)
{
    char *fmt, *asis, *p, spec[SPEC_BUF_SIZE];
    int nbytes = 0;

    /* make a copy so we can temporarily change the format string */
    p = asis = fmt = (char *)G_malloc(strlen(format) + 1);
    strcpy(fmt, format);

    while (*p) {
	if (*p == '%') {
	    char *q = p, *p_spec = spec;

	    /* print the string before this specifier */
	    *p = 0;
	    nbytes += oprintf(opts, asis);
	    *p = '%';

	    /* skip % */
	    while (*++q) {
		char *c = CONVS - 1;

		while (*++c && *q != *c);
		if (*c) {
		    va_list ap_copy;
		    char tmp;

		    /* copy ap for ovprintf() */
		    va_copy(ap_copy, ap);

		    /* found a conversion specifier */
		    if (*c == 's') {
			/* if this is a string specifier */
			int width = -1, prec = -1, use_ovprintf = 1;
			char *p_tmp, *s;

			*p_spec = 0;
			p_spec = spec;
			if (*p_spec == '-')
			    /* alignment */
			    p_spec++;
			if (*p_spec == '*') {
			    /* read width from next argument */
			    width = va_arg(ap, int);
			    p_spec++;
			} else if (*p_spec >= '0' && *p_spec <= '9') {
			    /* read width */
			    p_tmp = p_spec;
			    while (*p_spec >= '0' && *p_spec <= '9')
				p_spec++;
			    tmp = *p_spec;
			    *p_spec = 0;
			    width = atoi(p_tmp);
			    *p_spec = tmp;
			}
			if (*p_spec == '.') {
			    /* precision */
			    p_spec++;
			    if (*p_spec == '*') {
				/* read precision from next argument */
				prec = va_arg(ap, int);
				p_spec++;
			    } else if (*p_spec >= '0' && *p_spec <= '9') {
				/* read precision */
				p_tmp = p_spec;
				while (*p_spec >= '0' && *p_spec <= '9')
				    p_spec++;
				tmp = *p_spec;
				*p_spec = 0;
				prec = atoi(p_tmp);
				*p_spec = tmp;
			    }
			}
			if (*p_spec) {
			    /* illegal string specifier? */
			    va_end(ap_copy);
			    *(q + 1) = 0;
			    G_fatal_error(
				    _("Failed to parse string specifier: %s"),
				    p);
			}

			s = va_arg(ap, char *);
			if (width > 0) {
			    /* if width is specified */
			    int wcount = count_wide_chars(s);

			    if (wcount) {
				/* if there are wide characters */
				if (prec > 0)
				    width += count_wide_chars_in_cols(s, prec,
								      &prec);
				else if (prec < 0)
				    width += wcount;
				p_spec = spec;
				p_spec += sprintf(p_spec, "%%%s%d",
					spec[0] == '-' ? "-" : "", width);
				if (prec >= 0)
				    p_spec += sprintf(p_spec, ".%d", prec);
				*p_spec++ = 's';
				*p_spec = 0;
				nbytes += oprintf(opts, spec, s);
				use_ovprintf = 0;
			    }
			    /* else use ovprintf() as much as possible */
			}
			/* else use ovprintf() as much as possible */
			if (use_ovprintf) {
			    tmp = *(q + 1);
			    *(q + 1) = 0;
			    nbytes += ovprintf(opts, p, ap_copy);
			    *(q + 1) = tmp;
			}
		    } else {
			/* else use ovprintf() for non-string specifiers */
			tmp = *(q + 1);
			*(q + 1) = 0;
			nbytes += ovprintf(opts, p, ap_copy);
			*(q + 1) = tmp;

			/* once ap is passed to another function that calls
			 * va_arg() on it, its value becomes undefined
			 * (printf(3) man page) or indeterminate
			 * (http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1124.pdf
			 * section 7.15 paragraph 3) after the callee function
			 * returns; simply passing ap to ovprintf() works on
			 * Linux, but it doesn't on MinGW on Windows; pass its
			 * copy and skip an argument manually; argument types
			 * from printf(3) man page */
			switch (*c) {
			    case 'd':
			    case 'i':
			    case 'o':
			    case 'u':
			    case 'x':
			    case 'X':
			    case 'c':
			    case 'C':
			    case 'S':
				va_arg(ap, int);
				break;
			    case 'e':
			    case 'E':
			    case 'f':
			    case 'F':
			    case 'g':
			    case 'G':
			    case 'a':
			    case 'A':
				va_arg(ap, double);
				break;
			    case 'p':
				va_arg(ap, void *);
				break;
			    case 'n':
				va_arg(ap, int *);
				break;
			    /* otherwise, no argument is required for m% */
			}
		    }
		    va_end(ap_copy);
		    break;
		} else if (p_spec - spec < SPEC_BUF_SIZE - 2)
		    /* 2 reserved for % and NULL */
		    *p_spec++ = *q;
		else
		    G_fatal_error(
			    _("Format specifier exceeds the buffer size (%d)"),
			    SPEC_BUF_SIZE);
	    }
	    asis = (p = q) + 1;
	}
	p++;
    }

    /* print the remaining string */
    *p = 0;
    nbytes += oprintf(opts, asis);
    *p = '%';

    return nbytes;
}

/*!
 * \brief vprintf() version of G_aprintf(). See G_aprintf() for more details.
 *
 * \param[in] format string format
 * \param[in] ap variable argument list for the format string
 * \return number of bytes printed or fatal error on error
 */
int G_vaprintf(const char *format, va_list ap)
{
    return oaprintf(NULL, format, ap);
}

/*!
 * \brief vfprintf() version of G_aprintf(). See G_aprintf() for more details.
 *
 * \param[in] stream file pointer
 * \param[in] format string format
 * \param[in] ap variable argument list for the format string
 * \return number of bytes printed or fatal error on error
 */
int G_vfaprintf(FILE *stream, const char *format, va_list ap)
{
    struct options opts;

    opts.stream = stream;
    opts.str = NULL;
    opts.size = -1;

    return oaprintf(&opts, format, ap);
}

/*!
 * \brief vsprintf() version of G_aprintf(). See G_aprintf() for more details.
 *
 * \param[in] str string buffer
 * \param[in] format string format
 * \param[in] ap variable argument list for the format string
 * \return number of bytes printed or fatal error on error
 */
int G_vsaprintf(char *str, const char *format, va_list ap)
{
    struct options opts;

    opts.stream = NULL;
    opts.str = opts._str = str;
    opts.size = -1;

    return oaprintf(&opts, format, ap);
}

/*!
 * \brief vsnprintf() version of G_aprintf(). See G_aprintf() for more details.
 *
 * \param[in] str string buffer
 * \param[in] size string buffer size
 * \param[in] format string format
 * \param[in] ap variable argument list for the format string
 * \return number of bytes that would be printed if size was big enough or
 *	   fatal error on error
 */
int G_vsnaprintf(char *str, size_t size, const char *format, va_list ap)
{
    struct options opts;

    opts.stream = NULL;
    opts.str = opts._str = str;
    opts.size = opts._size = size;

    return oaprintf(&opts, format, ap);
}

/*!
 * \brief Adjust the width of string specifiers to the display space instead of
 * the number of bytes for wide characters and print them formatted using the
 * adjusted display width.
 *
 * compare
 *	printf("%10s|\n%10s|\n", "ABCD", "가나");
-----------
      ABCD|
    가나|
-----------
 * and
 *	G_aprintf("%10s|\n%10s|\n", "ABCD", "가나");
-----------
      ABCD|
      가나|
-----------
 *
 * \param[in] format string format
 * \param[in] ... arguments for the format string
 * \return number of bytes printed or fatal error on error
 */
int G_aprintf(const char *format, ...)
{
    va_list ap;
    int nbytes;

    va_start(ap, format);
    nbytes = G_vaprintf(format, ap);
    va_end(ap);

    return nbytes;
}

/*!
 * \brief fprintf() version of G_aprintf(). See G_aprintf() for more details.
 *
 * \param[in] stream file pointer
 * \param[in] format string format
 * \param[in] ... arguments for the format string
 * \return number of bytes printed or fatal error on error
 */
int G_faprintf(FILE *stream, const char *format, ...)
{
    va_list ap;
    int nbytes;

    va_start(ap, format);
    nbytes = G_vfaprintf(stream, format, ap);
    va_end(ap);

    return nbytes;
}

/*!
 * \brief sprintf() version of G_aprintf(). See G_aprintf() for more details.
 *
 * \param[in] str string buffer
 * \param[in] format string format
 * \param[in] ... arguments for the format string
 * \return number of bytes printed or fatal error on error
 */
int G_saprintf(char *str, const char *format, ...)
{
    va_list ap;
    int nbytes;

    va_start(ap, format);
    nbytes = G_vsaprintf(str, format, ap);
    va_end(ap);

    return nbytes;
}

/*!
 * \brief snprintf() version of G_aprintf(). See G_aprintf() for more details.
 *
 * \param[in] str string buffer
 * \param[in] size string buffer size
 * \param[in] format string format
 * \param[in] ... arguments for the format string
 * \return number of bytes that would be printed if size was big enough or
 *	   fatal error on error
 */
int G_snaprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    int nbytes;

    va_start(ap, format);
    nbytes = G_vsnaprintf(str, size, format, ap);
    va_end(ap);

    return nbytes;
}
