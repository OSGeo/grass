/*
 * Copyright (C) 1995.  Bill Brown <brown@gis.uiuc.edu> & Michael Shapiro
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */
#include <stdio.h>
#include <string.h>
#include <grass/datetime.h>


static int scan_absolute(DateTime *, const char *);
static int more(const char **);
static int minus_sign(const char **);
static int is_bc(const char **);
static int is_relative(const char *);
static int relative_term(const char **, double *, int *, int *, int *);
static int scan_tz(const char *, int *);
static int get_word(const char **, char *);
static char lowercase(char);
static int which_month(const char *, int *);
static int scan_relative(DateTime *, const char *);
static int is_space(char);
static int is_digit(char);
static void skip_space(const char **);
static int get_int(const char **, int *, int *);
static int get_double(const char **, double *, int *, int *);


/*!
 * \brief 
 *
 * Convert the ascii string
 * into a DateTime. This determines the mode/from/to based on the string, inits
 * 'dt' and then sets values in 'dt' based on the [???]
 * Returns 0 if 'string' is legal, -1 if not. 
 *
 *  \param dt
 *  \param buf
 *  \return int
 */

int datetime_scan(DateTime * dt, const char *buf)
{
    if (is_relative(buf)) {
	if (scan_relative(dt, buf))
	    return 0;
	return datetime_error(-1, "Invalid interval datetime format");
    }
    if (scan_absolute(dt, buf))
	return 0;
    return datetime_error(-2, "Invalid absolute datetime format");
}

static const char *month_names[] = {
    "jan", "feb", "mar", "apr", "may", "jun",
    "jul", "aug", "sep", "oct", "nov", "dec"
};

static int scan_absolute(DateTime * dt, const char *buf)
{
    char word[1024];
    int n;
    int ndigits;
    int tz = 0;
    int have_tz = 0;
    int bc = 0;
    int to, fracsec = 0;
    int year, month, day = 0, hour, minute;
    double second;
    const char *p;

    p = buf;
    if (!more(&p))
	return 0;

    if (!get_int(&p, &n, &ndigits)) {	/* no day, so must be month, like Jan */
	if (!get_word(&p, word))
	    return 0;
	if (!which_month(word, &month))
	    return 0;
	if (!get_int(&p, &year, &ndigits))	/* year following the month */
	    return 0;
	to = DATETIME_MONTH;
	if (is_bc(&p))
	    bc = 1;
	goto set;
    }

    bc = is_bc(&p);
    if (bc || !get_word(&p, word)) {	/* just a year */
	year = n;
	to = DATETIME_YEAR;
	goto set;
    }
    to = DATETIME_DAY;		/* must be at least: day Mon year [bc] */
    day = n;
    if (!which_month(word, &month))
	return 0;
    if (!get_int(&p, &year, &ndigits))
	return 0;
    if (is_bc(&p))
	bc = 1;

    /* now for the time */
    if (!get_int(&p, &hour, &ndigits))
	goto set;
    to = DATETIME_HOUR;
    if (*p != ':')
	goto set;
    p++;
    if (!get_int(&p, &minute, &ndigits))
	return 0;
    if (ndigits != 2)
	return 0;
    to = DATETIME_MINUTE;
    if (*p != ':')
	goto timezone;
    p++;
    if (!get_double(&p, &second, &ndigits, &fracsec))
	return 0;
    if (ndigits != 2)
	return 0;
    to = DATETIME_SECOND;

  timezone:
    if (!get_word(&p, word))
	goto set;
    if (!scan_tz(word, &tz))
	return 0;
    have_tz = 1;

  set:
    if (more(&p))		/* make sure there isn't anything else */
	return 0;
    if (datetime_set_type(dt, DATETIME_ABSOLUTE, DATETIME_YEAR, to, fracsec))
	return 0;
    for (n = DATETIME_YEAR; n <= to; n++) {
	switch (n) {
	case DATETIME_YEAR:
	    if (datetime_set_year(dt, year))
		return 0;
	    break;
	case DATETIME_MONTH:
	    if (datetime_set_month(dt, month))
		return 0;
	    break;
	case DATETIME_DAY:
	    if (datetime_set_day(dt, day))
		return 0;
	    break;
	case DATETIME_HOUR:
	    if (datetime_set_hour(dt, hour))
		return 0;
	    break;
	case DATETIME_MINUTE:
	    if (datetime_set_minute(dt, minute))
		return 0;
	    break;
	case DATETIME_SECOND:
	    if (datetime_set_second(dt, second))
		return 0;
	    break;
	}
    }
    if (bc)
	datetime_set_negative(dt);
    if (have_tz && datetime_set_timezone(dt, tz))
	return 0;

    return 1;
}


static int scan_relative(DateTime * dt, const char *buf)
{
    const char *p;
    double x;
    int ndigits, ndecimal;
    int pos;
    int neg = 0;
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, fracsec = 0;
    double second = 0.0;
    int from = DATETIME_SECOND + 1, to = DATETIME_YEAR - 1;

    p = buf;
    neg = minus_sign(&p);
    if (!more(&p))
	return 0;

    while (relative_term(&p, &x, &ndigits, &ndecimal, &pos)) {
	if (from > pos)
	    from = pos;
	if (to < pos)
	    to = pos;

	if (pos != DATETIME_SECOND && ndecimal != 0)
	    return 0;

	switch (pos) {
	case DATETIME_YEAR:
	    year = (int)x;
	    break;
	case DATETIME_MONTH:
	    month = (int)x;
	    break;
	case DATETIME_DAY:
	    day = (int)x;;
	    break;
	case DATETIME_HOUR:
	    hour = (int)x;
	    break;
	case DATETIME_MINUTE:
	    minute = (int)x;
	    break;
	case DATETIME_SECOND:
	    second = x;
	    fracsec = ndecimal;
	    break;
	}

    }

    if (more(&p))		/* make sure there isn't anything else */
	return 0;
    if (datetime_set_type(dt, DATETIME_RELATIVE, from, to, fracsec))
	return 0;
    for (pos = from; pos <= to; pos++) {
	switch (pos) {
	case DATETIME_YEAR:
	    if (datetime_set_year(dt, year))
		return 0;
	    break;
	case DATETIME_MONTH:
	    if (datetime_set_month(dt, month))
		return 0;
	    break;
	case DATETIME_DAY:
	    if (datetime_set_day(dt, day))
		return 0;
	    break;
	case DATETIME_HOUR:
	    if (datetime_set_hour(dt, hour))
		return 0;
	    break;
	case DATETIME_MINUTE:
	    if (datetime_set_minute(dt, minute))
		return 0;
	    break;
	case DATETIME_SECOND:
	    if (datetime_set_second(dt, second))
		return 0;
	    break;
	}
    }
    if (neg)
	datetime_set_negative(dt);

    return 1;
}

static int is_space(char c)
{
    return (c == ' ' || c == '\t' || c == '\n');
}

static int is_digit(char c)
{
    return (c >= '0' && c <= '9');
}

static void skip_space(const char **s)
{
    while (is_space(**s))
	(*s)++;
}

static int get_int(const char **s, int *n, int *ndigits)
{
    const char *p;

    *n = 0;
    skip_space(s);
    p = *s;
    for (*ndigits = 0; is_digit(*p); (*ndigits)++) {
	*n *= 10;
	*n += *p - '0';
	p++;
    }
    if (*ndigits > 0)
	*s = p;
    return (*ndigits > 0);
}

static int get_double(const char **s, double *x, int *ndigits,	/* number of digits before decimal */
		      int *ndecimal)
{				/* number of decimal places */
    char buf[1024];
    char *b;
    const char *p;

    skip_space(s);

    p = *s;
    *ndecimal = 0;
    b = buf;

    for (*ndigits = 0; is_digit(*p); (*ndigits)++)
	*b++ = *p++;
    if (*p == '.') {
	*b++ = *p++;
	while (is_digit(*p)) {
	    *b++ = *p++;
	    (*ndecimal)++;
	}
    }
    *b = 0;
    if (sscanf(buf, "%lf", x) != 1)
	return 0;
    *s = p;
    return 1;
}


/* if pos is non-zero, *(p-1) must be legal */
/*
   static int
   is_wordend (pos, p)
   int pos;
   char *p;
   {
   int d1, d0;

   if ('\0'==(*p)) return (1);
   if (is_space(*p)) return (1);
   if (pos){
   d0 = is_digit(*(p-1));
   d1 = is_digit(*p);
   return(d0 != d1);
   }
   return (0);

   }
 */

/* get a word (between white space) and convert to lowercase */
static int get_word(const char **s, char *word)
{
    const char *p;
    int any;

    skip_space(s);
    p = *s;
    for (any = 0; *p && !is_space(*p); any = 1)
	*word++ = lowercase(*p++);
    *word = 0;
    *s = p;
    return any;
}

static char lowercase(char c)
{
    if (c >= 'A' && c <= 'Z')
	c += 'a' - 'A';
    return c;
}

static int which_month(const char *name, int *n)
{
    int i;

    for (i = 0; i < 12; i++)
	if (strcmp(name, month_names[i]) == 0) {
	    *n = i + 1;
	    return 1;
	}
    return 0;
}

static int is_bc(const char **s)
{
    const char *p;
    char word[1024];

    p = *s;
    if (!get_word(&p, word))
	return 0;
    if (strcmp("bc", word) != 0)
	return 0;
    *s = p;
    return 1;
}

static int scan_tz(const char *word, int *tz)
{
    int neg = 0;

    if (word[0] == '+')
	neg = 0;
    else if (word[0] == '-')
	neg = 1;
    else
	return 0;

    if (!is_digit(word[1]))
	return 0;
    if (!is_digit(word[2]))
	return 0;
    if (!is_digit(word[3]))
	return 0;
    if (!is_digit(word[4]))
	return 0;

    *tz = (word[1] - '0') * 600 + (word[2] - '0') * 60 +
	(word[3] - '0') * 10 + (word[4] - '0');
    if (neg)
	*tz = -(*tz);
    return 1;
}

/* returns
   0 not a recognized term
   1 valid term, but perhaps illegal value
 */
static int relative_term(const char **s,
			 double *x, int *ndigits, int *ndecimal, int *pos)
{
    char word[1024];
    const char *p;

    p = *s;
    if (!get_double(&p, x, ndigits, ndecimal) || !get_word(&p, word))
	return 0;

    if (strcmp(word, "year") == 0 || strcmp(word, "years") == 0)
	*pos = DATETIME_YEAR;
    else if (strcmp(word, "month") == 0 || strcmp(word, "months") == 0 ||
	     strcmp(word, "mon") == 0)
	*pos = DATETIME_MONTH;
    else if (strcmp(word, "day") == 0 || strcmp(word, "days") == 0)
	*pos = DATETIME_DAY;
    else if (strcmp(word, "hour") == 0 || strcmp(word, "hours") == 0)
	*pos = DATETIME_HOUR;
    else if (strcmp(word, "minute") == 0 || strcmp(word, "minutes") == 0 ||
	     strcmp(word, "min") == 0)
	*pos = DATETIME_MINUTE;
    else if (strcmp(word, "second") == 0 || strcmp(word, "seconds") == 0 ||
	     strcmp(word, "sec") == 0)
	*pos = DATETIME_SECOND;
    else
	return 0;
    *s = p;
    return 1;
}

static int minus_sign(const char **s)
{
    skip_space(s);
    if (**s == '-') {
	(*s)++;
	return 1;
    }
    return 0;
}

static int is_relative(const char *buf)
{
    int n;
    double x;
    const char *p;

    p = buf;
    (void)minus_sign(&p);
    return relative_term(&p, &x, &n, &n, &n) != 0;
}

static int more(const char **s)
{
    skip_space(s);
    return **s != 0;
}
