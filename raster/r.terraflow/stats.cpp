/****************************************************************************
 *
 *  MODULE:        r.terraflow
 *
 *  COPYRIGHT (C) 2007 Laura Toma
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/

<<<<<<< HEAD
<<<<<<< HEAD
#include <cinttypes>
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#ifndef __MINGW32__
#include <sys/resource.h>
#endif
#include <stdio.h>
#include <errno.h>

#include "stats.h"

#ifdef HAS_UTRACE

struct ut {
    char buf[8];
};

void utrace __P((void *, int));

#define UTRACE(s)                     \
    {                                 \
        struct ut u;                  \
        strncpy(u.buf, s, 8);         \
        utrace((void *)&u, sizeof u); \
    }
#else /* !HAS_UTRACE */
#define UTRACE(s)
#endif /* HAS_UTRACE */

#undef UTRACE

#ifdef UTRACE_ENABLE
#define UTRACE(s) utrace(s)
#else
#define UTRACE(s)
#endif

void utrace(const char *s)
{
    void *p;
    int len = strlen(s);
    assert(len < 80);

    /* cerr << "UT " << len << endl; */
    p = malloc(0);
    /* assert(p); */
    free(p);
    p = malloc(len);
    /* assert(p); */
    free(p);

    for (int i = 0; i < len; i++) {
        p = malloc(s[i]);
        /* assert(p); */
        free(p);
    }
}

int noclobberFile(char *fname)
{
    int fd = -1;

    while (fd < 0) {
        fd = open(fname, O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (fd < 0) {
            if (errno != EEXIST) {
                perror(fname);
                exit(1);
            }
            else { /* file exists */
                char buf[BUFSIZ];
                G_debug(1, "file %s exists - renaming.\n", fname);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                snprintf(buf, BUFSIZ, "%s.old", fname);
=======
                sprintf(buf, "%s.old", fname);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
                sprintf(buf, "%s.old", fname);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
                snprintf(buf, BUFSIZ, "%s.old", fname);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
                sprintf(buf, "%s.old", fname);
=======
                snprintf(buf, BUFSIZ, "%s.old", fname);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
                if (rename(fname, buf) != 0) {
                    G_fatal_error("%s", fname);
                }
            }
        }
    }
    return fd;
}

char *noclobberFileName(char *fname)
{
    int fd;
    fd = open(fname, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0) {
        if (errno != EEXIST) {
            perror(fname);
            exit(1);
        }
        else { /* file exists */
            char buf[BUFSIZ];
            G_debug(1, "file %s exists - renaming.\n", fname);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            snprintf(buf, BUFSIZ, "%s.old", fname);
=======
            sprintf(buf, "%s.old", fname);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            sprintf(buf, "%s.old", fname);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            snprintf(buf, BUFSIZ, "%s.old", fname);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
            sprintf(buf, "%s.old", fname);
=======
            snprintf(buf, BUFSIZ, "%s.old", fname);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
            if (rename(fname, buf) != 0) {
                G_fatal_error("%s", fname);
            }
            close(fd);
        }
    }
    return fname;
}

/* ********************************************************************** */

statsRecorder::statsRecorder(char *fname) : ofstream(noclobberFileName(fname))
{
    // note: in the new version of gcc there is not constructor for
    // ofstream that takes an fd; wrote another noclobber() function that
    // closes fd and returns the name;
    rt_start(tm);
}

/* ********************************************************************** */

char *statsRecorder::timestamp()
{
    static char buf[BUFSIZ];
    rt_stop(tm);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    snprintf(buf, BUFSIZ, "[%.1f] ", rt_seconds(tm));
=======
    sprintf(buf, "[%.1f] ", rt_seconds(tm));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    sprintf(buf, "[%.1f] ", rt_seconds(tm));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    snprintf(buf, BUFSIZ, "[%.1f] ", rt_seconds(tm));
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    sprintf(buf, "[%.1f] ", rt_seconds(tm));
=======
    snprintf(buf, BUFSIZ, "[%.1f] ", rt_seconds(tm));
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    return buf;
}

void statsRecorder::timestamp(const char *s)
{
    *this << timestamp() << s << endl;
}

void statsRecorder::comment(const char *s, const int verbose)
{
    *this << timestamp() << s << endl;
    if (verbose) {
        cout << s << endl;
    }
    UTRACE(s);
    cout.flush();
}

void statsRecorder::comment(const char *s1, const char *s2)
{
    char buf[BUFSIZ];
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    snprintf(buf, BUFSIZ, "%s%s", s1, s2);
=======
    sprintf(buf, "%s%s", s1, s2);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    sprintf(buf, "%s%s", s1, s2);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    snprintf(buf, BUFSIZ, "%s%s", s1, s2);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    sprintf(buf, "%s%s", s1, s2);
=======
    snprintf(buf, BUFSIZ, "%s%s", s1, s2);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    comment(buf);
}

void statsRecorder::comment(const int n)
{
    char buf[BUFSIZ];
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    snprintf(buf, BUFSIZ, "%d", n);
=======
    sprintf(buf, "%d", n);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    sprintf(buf, "%d", n);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
    snprintf(buf, BUFSIZ, "%d", n);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
    sprintf(buf, "%d", n);
=======
    snprintf(buf, BUFSIZ, "%d", n);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    comment(buf);
}

char *formatNumber(char *buf, off_t val)
{
    if (val > (1 << 30)) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        snprintf(buf, BUFSIZ, "%.2fG (%" PRId64 ")", (double)val / (1 << 30),
                 val);
=======
        sprintf(buf, "%.2fG (%" PRI_OFF_T ")", (double)val / (1 << 30), val);
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }
    else if (val > (1 << 20)) {
        snprintf(buf, BUFSIZ, "%.2fM (%" PRId64 ")", (double)val / (1 << 20),
                 val);
    }
    else if (val > (1 << 10)) {
        snprintf(buf, BUFSIZ, "%.2fK (%" PRId64 ")", (double)val / (1 << 10),
                 val);
    }
    else {
        snprintf(buf, BUFSIZ, "%" PRId64, val);
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        sprintf(buf, "%.2fG (%" PRI_OFF_T ")", (double)val / (1 << 30), val);
=======
        snprintf(buf, BUFSIZ, "%.2fG (%" PRI_OFF_T ")", (double)val / (1 << 30),
                 val);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
    }
    else if (val > (1 << 20)) {
        snprintf(buf, BUFSIZ, "%.2fM (%" PRI_OFF_T ")", (double)val / (1 << 20),
                 val);
    }
    else if (val > (1 << 10)) {
        snprintf(buf, BUFSIZ, "%.2fK (%" PRI_OFF_T ")", (double)val / (1 << 10),
                 val);
    }
    else {
<<<<<<< HEAD
        sprintf(buf, "%" PRI_OFF_T, val);
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
        snprintf(buf, BUFSIZ, "%" PRI_OFF_T, val);
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
        snprintf(buf, BUFSIZ, "%.2fG (%" PRI_OFF_T ")", (double)val / (1 << 30),
                 val);
    }
    else if (val > (1 << 20)) {
        snprintf(buf, BUFSIZ, "%.2fM (%" PRI_OFF_T ")", (double)val / (1 << 20),
                 val);
    }
    else if (val > (1 << 10)) {
        snprintf(buf, BUFSIZ, "%.2fK (%" PRI_OFF_T ")", (double)val / (1 << 10),
                 val);
    }
    else {
        snprintf(buf, BUFSIZ, "%" PRI_OFF_T, val);
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
    }
    return buf;
}

void statsRecorder::recordTime(const char *label, long secs)
{
    *this << timestamp() << "TIME " << label << ": " << secs << " secs" << endl;
    this->flush();

    UTRACE(label);
}

void statsRecorder::recordTime(const char *label, Rtimer rt)
{
    char buf[BUFSIZ];
    *this << timestamp() << "TIME " << label << ": ";
    *this << rt_sprint(buf, rt) << endl;
    this->flush();

    UTRACE(label);
}

void statsRecorder::recordLength(const char *label, off_t len, int siz,
                                 char *sname)
{
    UTRACE(label);
    UTRACE(sname);

    char lenstr[100];
    char suffix[100] = "";
    if (siz) {
        formatNumber(suffix, len * siz);
        strcat(suffix, " bytes");
    }
    formatNumber(lenstr, len);
    *this << timestamp() << "LEN " << label << ": " << lenstr << " elts "
          << suffix;
    if (sname)
        *this << " " << sname;
    *this << endl;
    this->flush();
}
