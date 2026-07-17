/***********************************************************************
 *
 * MODULE:        r.support.stats
 *
 * AUTHOR(S):     Brad Douglas <rez touchofmadness com>
 *
 * PURPOSE:       Update raster statistics
 *
 * SPDX-FileCopyrightText: 2006 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ***********************************************************************/

#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

/* check.c */
int check_stats(const char *);

/* histo.c */
int do_histogram(const char *);

#endif /* __LOCAL_PROTO_H__ */
