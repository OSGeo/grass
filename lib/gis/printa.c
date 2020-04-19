/*!
 * \file lib/gis/printa.c
 *
 * \brief GIS Library - Print functions for aligning wide characters.
 *
 * Extracted from the print-aligned C library (libprinta under GPL v3+) by
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
#define CONVERSIONS "diouxXeEfFgGaAcsCSpnm%"

struct options
{
    FILE *stream;
    char *str, *_str;
    size_t size, _size;
};

static int wide_count(const char *);
static int _vprintf(struct options *, const char *, va_list);
static int _printf(struct options *, const char *, ...);
static int oprinta(struct options *, const char *, va_list);

/*!
 * \brief Count the number of wide characters in a string.
 *
 * \param[in] str input string
 * \return number of wide characters in str
 */
static int wide_count(const char *str)
{
    int count = 0, lead = 0;

    while (*str)
	/* if the first two bits are 10 (0x80 = 1000 0000), this byte is
	 * following a previous multi-byte character; only count the leading
	 * byte */
	if ((*str++ & 0xc0) != 0x80)
	    lead = 1;
	else if (lead) {
	    lead = 0;
	    count++;
	}

    return count;
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
static int _vprintf(struct options *opts, const char *format, va_list ap)
{
    int nbytes;

    if (opts == NULL || (opts->stream == NULL && opts->_str == NULL))
	nbytes = vprintf(format, ap);
    else if (opts->stream)
	nbytes = vfprintf(opts->stream, format, ap);
    else {
	if ((long int)opts->size >= 0) {
	    /* snprintf(str, 0, ...) prints garbage */
	    nbytes = vsnprintf(opts->_str, opts->_size, format, ap);
	    opts->_size -= nbytes;
	} else
	    /* snprintf(str, negative, ...) is equivalent to snprintf(str, ...)
	     * because size_t is unsigned */
	    nbytes = vsprintf(opts->_str, format, ap);
	opts->_str += nbytes;
    }

    if (nbytes < 0)
	G_fatal_error(_("_vprintf() failed"));

    return nbytes;
}

/*!
 * \brief Invoke _vprintf() for branching into different *printf() functions.
 *
 * \param[in] opts options for branching
 * \param[in] format string format
 * \param[in] ... arguments for the format string
 * \return number of bytes printed or fatal error on error
 */
static int _printf(struct options *opts, const char *format, ...)
{
    va_list ap;
    int nbytes;

    va_start(ap, format);
    nbytes = _vprintf(opts, format, ap);
    va_end(ap);

    return nbytes;
}

/*!
 * \brief Core function for aligning wide characters with Latin characters
 * using %s specifiers. G_printa(), G_fprinta(), and G_sprinta() wrap around
 * this function to implement printf(), fprintf(), and sprintf() counterparts,
 * respectively.
 *
 * \param[in] opts options for branching
 * \param[in] format string format
 * \param[in] ap variable argument list for the format string
 * \return number of bytes printed or fatal error on error
 */
static int oprinta(struct options *opts, const char *format, va_list ap)
{
    char *fmt, *asis, *p, spec[10];
    int nbytes = 0;

    /* make a copy so we can temporarily change the format string */
    p = asis = fmt = (char *)malloc(strlen(format) + 1);
    strcpy(fmt, format);

    while (*p) {
	if (*p == '%') {
	    char *q = p, *p_spec = spec;

	    /* print the string before this specifier */
	    *p = 0;
	    nbytes += _printf(opts, asis);
	    *p = '%';

	    /* skip % */
	    while (*++q) {
		char *c = CONVERSIONS - 1;

		while (*++c && *q != *c);
		if (*c) {
		    char tmp;
		    /* found a conversion specifier */
		    if (*c == 's') {
			int width = -1, prec = -1, use_printf = 1;
			char *p_tmp, *s;
			va_list ap_copy;

			/* save this ap and use _vprintf() for non-wide
			 * characters */
			va_copy(ap_copy, ap);

			/* if string */
			*p_spec = 0;
			p_spec = spec;
			if (*p_spec == '-')
			    p_spec++;
			if (*p_spec == '*') {
			    width = va_arg(ap, int);
			    p_spec++;
			} else if (*p_spec >= '0' && *p_spec <= '9') {
			    p_tmp = p_spec;
			    while (*p_spec >= '0' && *p_spec <= '9')
				p_spec++;
			    tmp = *p_spec;
			    *p_spec = 0;
			    width = atoi(p_tmp);
			    *p_spec = tmp;
			}
			if (*p_spec == '.') {
			    p_spec++;
			    if (*p_spec == '*') {
				prec = va_arg(ap, int);
				p_spec++;
			    } else if (*p_spec >= '0' && *p_spec <= '9') {
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
			    /* really? */
			    va_end(ap_copy);
			    G_fatal_error(_("G_printa() failed"));
			}

			s = va_arg(ap, char *);
			if (width > 0) {
			    int wcount = wide_count(s);
			    if (wcount) {
				width += wcount;
				prec += prec > 0 ? wcount : 0;
				p_spec = spec;
				p_spec += sprintf(p_spec, "%%%s%d",
					spec[0] == '-' ? "-" : "", width);
				if (prec >= 0)
				    p_spec += sprintf(p_spec, ".%d", prec);
				*p_spec++ = 's';
				*p_spec = 0;
				nbytes += _printf(opts, spec, s);
				use_printf = 0;
			    }
			}
			if (use_printf) {
			    tmp = *(q + 1);
			    *(q + 1) = 0;
			    nbytes += _vprintf(opts, p, ap_copy);
			    *(q + 1) = tmp;
			}

			va_end(ap_copy);
		    } else {
			/* else use _vprintf() */
			tmp = *(q + 1);
			*(q + 1) = 0;
			nbytes += _vprintf(opts, p, ap);
			*(q + 1) = tmp;
		    }
		    break;
		} else
		    *p_spec++ = *q;
	    }
	    asis = (p = q) + 1;
	}
	p++;
    }

    /* print the remaining string */
    *p = 0;
    nbytes += _printf(opts, asis);
    *p = '%';

    return nbytes;
}

/*!
 * \brief vprintf() version of G_printa(). See G_printa() for more details.
 *
 * \param[in] format string format
 * \param[in] ap variable argument list for the format string
 * \return number of bytes printed or fatal error on error
 */
int G_vprinta(const char *format, va_list ap)
{
    return oprinta(NULL, format, ap);
}

/*!
 * \brief vfprintf() version of G_printa(). See G_printa() for more details.
 *
 * \param[in] stream file pointer
 * \param[in] format string format
 * \param[in] ap variable argument list for the format string
 * \return number of bytes printed or fatal error on error
 */
int G_vfprinta(FILE *stream, const char *format, va_list ap)
{
    struct options opts;

    opts.stream = stream;
    opts.str = NULL;
    opts.size = -1;

    return oprinta(&opts, format, ap);
}

/*!
 * \brief vsprintf() version of G_printa(). See G_printa() for more details.
 *
 * \param[in] str string buffer
 * \param[in] format string format
 * \param[in] ap variable argument list for the format string
 * \return number of bytes printed or fatal error on error
 */
int G_vsprinta(char *str, const char *format, va_list ap)
{
    struct options opts;

    opts.stream = NULL;
    opts.str = opts._str = str;
    opts.size = -1;

    return oprinta(&opts, format, ap);
}

/*!
 * \brief vsnprintf() version of G_printa(). See G_printa() for more details.
 *
 * \param[in] str string buffer
 * \param[in] size string buffer size
 * \param[in] format string format
 * \param[in] ap variable argument list for the format string
 * \return number of bytes that would be printed if size was big enough or
 *	   fatal error on error
 */
int G_vsnprinta(char *str, size_t size, const char *format, va_list ap)
{
    struct options opts;

    opts.stream = NULL;
    opts.str = opts._str = str;
    opts.size = opts._size = size;

    return oprinta(&opts, format, ap);
}

/*!
 * \brief Adjust the width of string specifiers to the display space intead of
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
 *	G_printa("%10s|\n%10s|\n", "ABCD", "가나");
-----------
      ABCD|
      가나|
-----------
 *
 * \param[in] format string format
 * \param[in] ... arguments for the format string
 * \return number of bytes printed or fatal error on error
 */
int G_printa(const char *format, ...)
{
    va_list ap;
    int nbytes;

    va_start(ap, format);
    nbytes = G_vprinta(format, ap);
    va_end(ap);

    return nbytes;
}

/*!
 * \brief fprintf() version of G_printa(). See G_printa() for more details.
 *
 * \param[in] stream file pointer
 * \param[in] format string format
 * \param[in] ... arguments for the format string
 * \return number of bytes printed or fatal error on error
 */
int G_fprinta(FILE *stream, const char *format, ...)
{
    va_list ap;
    int nbytes;

    va_start(ap, format);
    nbytes = G_vfprinta(stream, format, ap);
    va_end(ap);

    return nbytes;
}

/*!
 * \brief sprintf() version of G_printa(). See G_printa() for more details.
 *
 * \param[in] str string buffer
 * \param[in] format string format
 * \param[in] ... arguments for the format string
 * \return number of bytes printed or fatal error on error
 */
int G_sprinta(char *str, const char *format, ...)
{
    va_list ap;
    int nbytes;

    va_start(ap, format);
    nbytes = G_vsprinta(str, format, ap);
    va_end(ap);

    return nbytes;
}

/*!
 * \brief snprintf() version of G_printa(). See G_printa() for more details.
 *
 * \param[in] str string buffer
 * \param[in] size string buffer size
 * \param[in] format string format
 * \param[in] ... arguments for the format string
 * \return number of bytes that would be printed if size was big enough or
 *	   fatal error on error
 */
int G_snprinta(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    int nbytes;

    va_start(ap, format);
    nbytes = G_vsnprinta(str, size, format, ap);
    va_end(ap);

    return nbytes;
}
