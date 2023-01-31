/***************************************************
 * these routines determine the printf format used
 * by floating point values
 *
 * format_parms() is called for each value.
 *     before first call set eformat=0,dp=6
 *
 * format_double() does the formatting with the
 *     parms determined by format_parms()
 ***************************************************/

#include <string.h>
#include "global.h"

int format_parms(double v, int *n, int *dp, int *eformat, int e_option)
{
    char buf[50];
    int scient_dp;

    scient_dp = *dp;
    for (;;) {
        if (!*eformat)
            format_double(v, buf, *n, *dp);
        else
            scient_format(v, buf, *n, *dp);

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        if ((int)strlen(buf) <= *n)
=======
        if (strlen(buf) <= *n)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        if (strlen(buf) <= *n)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        if ((int)strlen(buf) <= *n)
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
        if (strlen(buf) <= *n)
=======
        if ((int)strlen(buf) <= *n)
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            break;

        if (*dp) {
            *dp -= 1;
        }
        else {
            if ((e_option) && (!*eformat)) {
                *eformat = 1;
                *dp = scient_dp;
            }
            else
                *n = strlen(buf);
        }
    }

    return 0;
}

int scient_format(double v, char *buf, int n, int dp)
{
    char temp[50];
    int i;

    sprintf(temp, "%#.*g", dp, v);
    for (i = 0; i <= n && temp[i] == ' '; i++) {
    }
    strcpy(buf, temp + i);

    return 0;
}

int format_double(double v, char *buf, int n, int dp)
{
    char fmt[15];
    char temp[100];
    int i, ncommas;

    sprintf(fmt, "%%%d.%df", n, dp);
    sprintf(temp, fmt, v);
    strcpy(buf, temp);
    G_insert_commas(temp);
    ncommas = strlen(temp) - strlen(buf);
    for (i = 0; i < ncommas && temp[i] == ' '; i++) {
    }
    strcpy(buf, temp + i);

    return 0;
}
