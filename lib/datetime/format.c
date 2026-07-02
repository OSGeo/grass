/*
 * Copyright (C) 1995.  Bill Brown <brown@gis.uiuc.edu> & Michael Shapiro
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */
#include <stdio.h>
#include <string.h>
#include <grass/datetime.h>

static char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/*!
 * \brief
 *
 * formats DateTime structure as a human-readable string
 * returns 0 when successful and 'buf' is filled with the string
 * returns a negative number on error
 *
 * \param dt
 * \param buf
 * \return int
 */
static void append_sep_if_needed(char *buf, const char *sep)
{
    if (*buf)
        strcat(buf, sep);
}

static void format_absolute(const DateTime *dt, char *buf, char *temp,
                            size_t tempsz)
{
    int n;
    double sec;

    if (datetime_get_day(dt, &n) == 0) {
        snprintf(temp, tempsz, "%d", n);
        strcat(buf, temp);
    }

    if (datetime_get_month(dt, &n) == 0) {
        append_sep_if_needed(buf, " ");
        strcat(buf, months[n - 1]);
    }

    if (datetime_get_year(dt, &n) == 0) {
        append_sep_if_needed(buf, " ");
        snprintf(temp, tempsz, "%d", n);
        strcat(buf, temp);

        if (datetime_is_negative(dt))
            strcat(buf, " bc");
    }

    if (datetime_get_hour(dt, &n) == 0) {
        append_sep_if_needed(buf, " ");
        snprintf(temp, tempsz, "%02d", n);
        strcat(buf, temp);
    }

    if (datetime_get_minute(dt, &n) == 0) {
        append_sep_if_needed(buf, ":");
        snprintf(temp, tempsz, "%02d", n);
        strcat(buf, temp);
    }

    if (datetime_get_second(dt, &sec) == 0) {
        append_sep_if_needed(buf, ":");

        if (datetime_get_fracsec(dt, &n) != 0)
            n = 0;

        snprintf(temp, tempsz, "%02.*f", n, sec);
        strcat(buf, temp);
    }

    if (datetime_get_timezone(dt, &n) == 0) {
        int hour, minute;

        append_sep_if_needed(buf, " ");
        datetime_decompose_timezone(n, &hour, &minute);
        snprintf(temp, tempsz, "%s%02d%02d", n < 0 ? "-" : "+", hour, minute);
        strcat(buf, temp);
    }
}

static void format_relative(const DateTime *dt, char *buf, char *temp,
                            size_t tempsz)
{
    int n;
    double sec;

    if (datetime_is_negative(dt))
        strcat(buf, "-");

    if (datetime_get_year(dt, &n) == 0) {
        append_sep_if_needed(buf, " ");
        snprintf(temp, tempsz, "%d year%s", n, n == 1 ? "" : "s");
        strcat(buf, temp);
    }

    if (datetime_get_month(dt, &n) == 0) {
        append_sep_if_needed(buf, " ");
        snprintf(temp, tempsz, "%d month%s", n, n == 1 ? "" : "s");
        strcat(buf, temp);
    }

    if (datetime_get_day(dt, &n) == 0) {
        append_sep_if_needed(buf, " ");
        snprintf(temp, tempsz, "%d day%s", n, n == 1 ? "" : "s");
        strcat(buf, temp);
    }

    if (datetime_get_hour(dt, &n) == 0) {
        append_sep_if_needed(buf, " ");
        snprintf(temp, tempsz, "%d hour%s", n, n == 1 ? "" : "s");
        strcat(buf, temp);
    }

    if (datetime_get_minute(dt, &n) == 0) {
        append_sep_if_needed(buf, " ");
        snprintf(temp, tempsz, "%d minute%s", n, n == 1 ? "" : "s");
        strcat(buf, temp);
    }

    if (datetime_get_second(dt, &sec) == 0) {
        append_sep_if_needed(buf, " ");

        if (datetime_get_fracsec(dt, &n) != 0)
            n = 0;

        snprintf(temp, tempsz, "%.*f second%s", n, sec,
                 (sec == 1.0 && n == 0) ? "" : "s");
        strcat(buf, temp);
    }
}


int datetime_format(const DateTime *dt, char *buf)
{
    /* Format the DateTime structure as a human-readable string */
    /*  Returns 0 when successful, and buf is filled with the
       formatted data.
       Returns a negative number as an error code if the DateTime
       structure is not valid.
     */
    char temp[128];

    *buf = 0;

    if (!datetime_is_valid_type(dt))
        return datetime_error_code();

    if (datetime_is_absolute(dt))
        format_absolute(dt, buf, temp, sizeof(temp));

    if (datetime_is_relative(dt))
        format_relative(dt, buf, temp, sizeof(temp));

    return 0;
}
