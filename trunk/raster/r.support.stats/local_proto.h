/*
 **********************************************************************
 *
 * MODULE:        r.support.stats
 *
 * AUTHOR(S):     Brad Douglas <rez touchofmadness com>
 *
 * PURPOSE:       Update raster statistics
 *
 * COPYRIGHT:     (C) 2006 by the GRASS Development Team
 *
 *                This program is free software under the GNU General
 *                Purpose License (>=v2). Read the file COPYING that
 *                comes with GRASS for details.
 *
 ***********************************************************************/


#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__


/* check.c */
int check_stats(const char *);

/* histo.c */
int do_histogram(const char *);


#endif /* __LOCAL_PROTO_H__ */
